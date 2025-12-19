#ifndef CONSTANTS_H
#define CONSTANTS_H

// COMMUNICATION 
#define DATA_PORT    5005
#define CMD_PORT     6000

// Number of Nodes
#define NUM_NODES 3

// MODE SELECTION 
#define MODE_ADC   0   // Analog pins (real sensors)
#define MODE_SD    1   // CSV / SD replay
#define MODE_UDP   2   // UDP streamed input

// VOLTAGE STATUS 
#define VSTATUS_NORMAL  0
#define VSTATUS_SAG     1
#define VSTATUS_SWELL   2

// CURRENT STATUS 
#define ISTATUS_NORMAL  0
#define ISTATUS_OC      1
#define ISTATUS_SC      2

// THRESHOLDS 
#define V_SAG_LEVEL    50.0f
#define V_SWELL_LEVEL 130.0f

// Overcurrent detection uses two thresholds:
//  - 11 A (Raspberry Pi): warning-only threshold to indicate attention is required
//  - 15 A (ESP32): protection threshold that triggers automatic transmission shutdown
#define I_OC_LEVEL    11.0f  

// PI LED GPIO 
#define LED_ADC   23
#define LED_SD    24
#define LED_UDP   12

// [node][0=GREEN,1=VOLTAGE,2=CURRENT]
static const int led_pins[3][3] = {
    {27, 17, 22},
    {21, 20, 16},
    {13, 19, 26}
};

#endif
