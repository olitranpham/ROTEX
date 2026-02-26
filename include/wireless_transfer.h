#pragma once
#include <stdbool.h>

#ifndef WIFI_SSID
#define WIFI_SSID "PicoHotspot"
#endif

#ifndef WIFI_PASS
#define WIFI_PASS "pico1234"
#endif

#ifndef DEST_IP
#define DEST_IP   "192.0.0.2"
#endif

#ifndef DEST_PORT
#define DEST_PORT 5005
#endif

bool wireless_init(void);
bool wireless_send_quat(float t, const char* label, float w, float x, float y, float z);
void wireless_deinit(void);