#ifndef WIFI_CONN_H
#define WIFI_CONN_H
#include "pico/stdlib.h"
#define DEBUG_WIFI
bool connect_to_wifi(const char *ssid, const char *password);
#endif