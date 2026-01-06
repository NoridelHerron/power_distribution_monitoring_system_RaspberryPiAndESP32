// ============================================================
// command.c
// Noridel Herron - December 2025
// Interactive command interface for ESP32 node control
// ============================================================

#include "all_h.h"

void send_udp_command(int sock, const char* msg)
{
    struct sockaddr_in baddr = {
        .sin_family = AF_INET,
        .sin_port   = htons(CMD_PORT),
        .sin_addr.s_addr = inet_addr("192.168.xx.255")
    };

    sendto(sock, msg, strlen(msg), 0,
           (struct sockaddr*)&baddr, sizeof(baddr));

    printf("[CMD] %s\n", msg);
}

void* command_thread(void *arg)
{
    (void)arg;

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    int yes = 1;
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &yes, sizeof(yes));

    int target_node = -1;   // -1 = ALL nodes
    char msg[64];

    printf("Commands:\n");
    printf("  a   = ALL nodes\n");
    printf("  1-3 = select node\n");
    printf("  m   = MODE (1=ADC 2=SD 3=UDP)\n");
    printf("  s   = SEND (4=ON 5=OFF)\n");
    printf("  r   = ACK / RESET fault\n");

    while (1)
    {
        int ch = getchar();

        // TARGET SELECTION 
        if (ch == 'a') {
            target_node = -1;
            printf("[CMD] Target = ALL\n");
            continue;
        }

        if (ch >= '1' && ch <= '3') { 
            target_node = ch - '0';
            printf("[CMD] Target = Node %d\n", target_node);
            continue;
        }

        // ACK / RESET 
        if (ch == 'r') {
            snprintf(msg, sizeof(msg), "ACK|%d", target_node);
            send_udp_command(sock, msg);
            continue;
        }

        // MODE INPUT SELECTION 
        if (ch == 'm') {
            int m = getchar();
            const char *mode = NULL;

            if (m == '1') {
                mode = "MODE_ADC";
                current_mode = MODE_ADC;
            }
            else if (m == '2') {
                mode = "MODE_SD";
                current_mode = MODE_SD;
            }
            else if (m == '3') {
                mode = "MODE_UDP";
                current_mode = MODE_UDP;
            }
            else {
                continue;
            }

            snprintf(msg, sizeof(msg),
                     "SET_MODE|%s|%d", mode, target_node);
            send_udp_command(sock, msg);

            // UPDATE PROCESS 1 LEDs IMMEDIATELY 
            set_mode_leds(mode);

            continue;
        }

        // SEND ON / OFF 
        if (ch == 's') {
            int s = getchar();

            if (s == '4')
                snprintf(msg, sizeof(msg),
                         "SET_SEND|ON|%d", target_node);
            else if (s == '5')
                snprintf(msg, sizeof(msg),
                         "SET_SEND|OFF|%d", target_node);
            else
                continue;

            send_udp_command(sock, msg);
            continue;
        }
    }
}
