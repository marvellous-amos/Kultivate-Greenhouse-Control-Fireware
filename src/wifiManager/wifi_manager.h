#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi.h>

#define WIFI_SSID     "Oracle"
#define WIFI_PASSWORD "12345678"
#define WIFI_TIMEOUT  30000

void connectToWiFi();

#endif