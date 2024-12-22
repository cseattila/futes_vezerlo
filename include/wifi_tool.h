#pragma once
#include <WiFi.h>

extern const char* defaut_ssid;
extern const char* default_password; 

void connectWiFi();
void initWifi();
void wifiScan();