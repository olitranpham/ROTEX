#include "wireless_transfer.h"

#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

#include "lwip/sockets.h"
#include "lwip/inet.h"
#include "lwip/ip_addr.h"

static int udp_sock = -1;
static struct sockaddr_in dest_addr;

bool wireless_init(void)
{
    if (cyw43_arch_init()) {
        printf("cyw43_arch_init failed\n");
        return false;
    }

    cyw43_arch_enable_sta_mode();

    printf("Connecting to WiFi SSID=%s...\n", WIFI_SSID);
    int rc = cyw43_arch_wifi_connect_timeout_ms(
        WIFI_SSID,
        WIFI_PASS,
        CYW43_AUTH_WPA2_AES_PSK,
        15000
    );
    if (rc) {
        printf("WiFi connect failed: %d\n", rc);
        cyw43_arch_deinit();
        return false;
    }

    printf("Connected to WiFi\n");

    udp_sock = lwip_socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_sock < 0) {
        printf("lwip_socket failed\n");
        cyw43_arch_deinit();
        return false;
    }

    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = PP_HTONS(DEST_PORT);
    dest_addr.sin_addr.s_addr = ipaddr_addr(DEST_IP);

    return true;
}

bool wireless_send_quat(float t, const char* label, float w, float x, float y, float z)
{
    if (udp_sock < 0) return false;

    char msg[160];
    int n = snprintf(msg, sizeof(msg),
        "%.3f,%s,%.6f,%.6f,%.6f,%.6f\n",
        t, label, w, x, y, z);
    if (n <= 0) return false;

    cyw43_arch_lwip_begin();
    int sent = lwip_sendto(udp_sock, msg, (size_t)n, 0,
        (struct sockaddr*)&dest_addr, sizeof(dest_addr));
    cyw43_arch_lwip_end();

    return (sent == n);
}

void wireless_deinit(void)
{
    if (udp_sock >= 0) {
        lwip_close(udp_sock);
        udp_sock = -1;
    }
    cyw43_arch_deinit();
}