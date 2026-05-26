#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

void setupWiFiEvents();
bool connectWiFi();
void reconnectWiFiIfNeeded();

bool isWiFiConnected();

#endif