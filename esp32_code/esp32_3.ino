/*
 * ESP32 RMS Power Monitor - Multi-Node System
 * Author: Noridel Herron
 * Date: December 2025
 */

#include <WiFi.h>
#include <WiFiUdp.h>
#include <math.h>

/* ==================== CONFIGURATION ==================== */
const char* ssid     = "xxxxxxxx";
const char* password = "xxxxxxxx";

#define SERVER_IP "xxx.xxx.xx.x"
#define CMD_PORT        6000
#define DATA_TX_PORT    5005
#define STREAM_RX_PORT  6001

#define NODE_ID 3

#define V_ADC_PIN  1
#define I_ADC_PIN  2

#define LED_ADC    0
#define LED_SD     38
#define LED_UDP    39
#define LED_SEND   10

#define BTN_SEND  3

#define ADC_RESOLUTION 4095.0f
#define ADC_MID        (ADC_RESOLUTION / 2.0f)

#define V_SCALE  (170.0f / (ADC_MID * 0.6f))
#define I_SCALE  0.0244f

#define RMS_BUFFER_SIZE 60
#define SEND_INTERVAL_MS 100

/* ===== LOCAL PROTECTION ===== */
#define OC_LIMIT   15.0f
#define OC_CLEAR   12.0f
#define OC_PERSIST 3

/* ==================== STATE ==================== */

WiFiUDP udp_cmd, udp_tx, udp_stream;

float vbuf[RMS_BUFFER_SIZE];
float ibuf[RMS_BUFFER_SIZE];
int   sample_idx = 0;

float vrms = 0.0f;
float irms = 0.0f;

uint32_t cycle_id = 0;
unsigned long last_send = 0;

enum Mode { MODE_ADC, MODE_SD, MODE_UDP };
Mode currentMode = MODE_UDP;

bool send_enabled = true;
bool fault_latched = false;
bool last_btn = HIGH;

uint8_t oc_counter = 0;

/* ==================== PACKET ==================== */
typedef struct {
  uint32_t node_id;
  uint32_t cycle_id;
  float vrms;
  float irms;
} __attribute__((packed)) Packet;

/* ==================== HELPERS ==================== */

void updateLEDs() {
  digitalWrite(LED_ADC,  currentMode == MODE_ADC);
  digitalWrite(LED_SD,   currentMode == MODE_SD);
  digitalWrite(LED_UDP,  currentMode == MODE_UDP);
  digitalWrite(LED_SEND, send_enabled && !fault_latched);
}

/* ==================== FAULT ==================== */

void sendFault() {
  char msg[96];
  unsigned long ts = millis();

  snprintf(msg, sizeof(msg),
           "FAULT|%d|OC_TRIP|%.2f|%.2f|%lu",
           NODE_ID, vrms, irms, ts);
  udp_tx.beginPacket(SERVER_IP, CMD_PORT);
  udp_tx.write((uint8_t*)msg, strlen(msg));
  udp_tx.endPacket();

  Serial.println(msg);
}

void checkFaults() {
  if (fault_latched) return;

  if (irms > OC_LIMIT) {
    oc_counter++;
    if (oc_counter >= OC_PERSIST) {
      fault_latched = true;
      send_enabled  = false;
      sendFault();
      updateLEDs();
    }
  } else if (irms < OC_CLEAR) {
    oc_counter = 0;
  }
}

/* ==================== INPUT ==================== */

bool getNextSample(float *v_adc, float *i_adc) {
  if (currentMode == MODE_ADC) {
    *v_adc = analogRead(V_ADC_PIN);
    *i_adc = analogRead(I_ADC_PIN);
    return true;
  }

  if (currentMode == MODE_UDP) {
    int sz = udp_stream.parsePacket();
    if (!sz) return false;

    char buf[64];
    int r = udp_stream.read(buf, sizeof(buf) - 1);
    buf[r] = '\0';

    return sscanf(buf, "WAVE|%f|%f", v_adc, i_adc) == 2;
  }

  return false;
}

/* ==================== RMS ==================== */

void computeRMS() {
  float sv = 0, si = 0;
  for (int i = 0; i < RMS_BUFFER_SIZE; i++) {
    sv += vbuf[i] * vbuf[i];
    si += ibuf[i] * ibuf[i];
  }

  vrms = sqrt(sv / RMS_BUFFER_SIZE);
  irms = sqrt(si / RMS_BUFFER_SIZE);

  cycle_id++;
  checkFaults();
}

void collectSample() {
  float v_adc, i_adc;
  if (!getNextSample(&v_adc, &i_adc)) return;

  vbuf[sample_idx] = (v_adc - ADC_MID) * V_SCALE;
  ibuf[sample_idx] = (i_adc - ADC_MID) * I_SCALE;

  if (++sample_idx >= RMS_BUFFER_SIZE) {
    computeRMS();
    sample_idx = 0;
  }
}

/* ==================== SEND ==================== */

void sendPacket() {
  if (!send_enabled || fault_latched) return;
  if (millis() - last_send < SEND_INTERVAL_MS) return;

  last_send = millis();
  Packet pkt = { NODE_ID, cycle_id, vrms, irms };

  udp_tx.beginPacket(SERVER_IP, DATA_TX_PORT);
  udp_tx.write((uint8_t*)&pkt, sizeof(pkt));
  udp_tx.endPacket();
}

/* ==================== BUTTON ==================== */

void handleButton() {
  bool btn = digitalRead(BTN_SEND);

  if (last_btn == HIGH && btn == LOW) {
    if (fault_latched && irms < OC_CLEAR) {
      fault_latched = false;
      oc_counter = 0;
      send_enabled = true;
      Serial.println("[BTN] Fault cleared");
    } else if (!fault_latched) {
      send_enabled = !send_enabled;
    }
    updateLEDs();
  }
  last_btn = btn;
}

/* ==================== COMMAND ==================== */
void processCommand(char *msg) {
  char *cmd = strtok(msg, "|");
  char *arg = strtok(NULL, "|");
  char *tgt = strtok(NULL, "|");

  if (tgt && atoi(tgt) != NODE_ID && atoi(tgt) != -1) return;

  /* ---------- ACK ---------- */
  if (!strcmp(cmd, "ACK") && fault_latched && irms < OC_CLEAR) {
    fault_latched = false;
    oc_counter = 0;
    send_enabled = true;
    Serial.println("[PI] Fault cleared");
    updateLEDs();
    return;
  }

  /* ---------- RESET_CYCLE ---------- */
  if (!strcmp(cmd, "RESET_CYCLE")) {
    cycle_id = 0;
    sample_idx = 0;
    vrms = irms = 0.0f;
    for (int i = 0; i < RMS_BUFFER_SIZE; i++) {
      vbuf[i] = ibuf[i] = 0.0f;
    }
    Serial.println("[PI] Cycle reset to 0");
    return;
  }

  /* ---------- SET_SEND ---------- */
  if (!strcmp(cmd, "SET_SEND") && !fault_latched) {
    if (!strcmp(arg, "ON")) {
      send_enabled = true;
      Serial.println("[PI] SEND ON");
    } else if (!strcmp(arg, "OFF")) {
      send_enabled = false;
      Serial.println("[PI] SEND OFF");
    }
    updateLEDs();
    return;
  }

  /* ---------- SET_MODE ---------- */
  if (!strcmp(cmd, "SET_MODE")) {
    if (!strcmp(arg, "MODE_ADC")) {
      currentMode = MODE_ADC;
      Serial.println("[PI] MODE ADC");
    } else if (!strcmp(arg, "MODE_SD")) {
      currentMode = MODE_SD;
      Serial.println("[PI] MODE SD");
    } else if (!strcmp(arg, "MODE_UDP")) {
      currentMode = MODE_UDP;
      Serial.println("[PI] MODE UDP");
    }
    updateLEDs();
    return;
  }
}


/* ==================== SETUP ==================== */

void setup() {
  Serial.begin(115200);

  pinMode(BTN_SEND, INPUT_PULLUP);
  pinMode(LED_ADC, OUTPUT);
  pinMode(LED_SD, OUTPUT);
  pinMode(LED_UDP, OUTPUT);
  pinMode(LED_SEND, OUTPUT);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) delay(200);

  Serial.print("NODE ");
  Serial.print(NODE_ID);
  Serial.print(" IP = ");
  Serial.println(WiFi.localIP());

  udp_cmd.begin(CMD_PORT);
  udp_stream.begin(STREAM_RX_PORT);

  updateLEDs();
}

/* ==================== LOOP ==================== */

void loop() {
  if (udp_cmd.parsePacket()) {
    char msg[64];
    int r = udp_cmd.read(msg, sizeof(msg) - 1);
    msg[r] = '\0';
    processCommand(msg);
  }

  handleButton();

  for (int i = 0; i < 10; i++)
    collectSample();

  sendPacket();
}
