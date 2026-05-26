#include "wifi_manager.h"

#include <Arduino.h>
#include <WiFi.h>

#include "config.h"
#include "secrets.h"

// ======================================================
// WiFi reconnect timer
// ======================================================

unsigned long lastWifiReconnectAttempt = 0;

// ======================================================
// WiFi events
// ======================================================

void onWiFiEvent(WiFiEvent_t event) {
    switch (event) {
        case ARDUINO_EVENT_WIFI_STA_CONNECTED:
            Serial.println("WiFi connected to access point");
            break;

        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            Serial.print("Got IP address: ");
            Serial.println(WiFi.localIP());
            break;

        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            Serial.println("WiFi disconnected");
            break;

        default:
            break;
    }
}

void setupWiFiEvents() {
    WiFi.onEvent(onWiFiEvent);
}

// ======================================================
// WiFi connection
// ======================================================

bool connectWiFi() {
    Serial.print("Connecting to WiFi: ");
    Serial.println(WIFI_SSID);

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    int attempts = 0;

    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("WiFi connection successful!");
        return true;
    }

    Serial.println("WiFi connection failed!");
    return false;
}

void reconnectWiFiIfNeeded() {
    if (WiFi.status() == WL_CONNECTED) {
        return;
    }

    unsigned long now = millis();

    if (now - lastWifiReconnectAttempt >= WIFI_RECONNECT_INTERVAL_MS) {
        lastWifiReconnectAttempt = now;
        Serial.println("Reconnecting to WiFi...");
        WiFi.reconnect();
    }
}

bool isWiFiConnected() {
    return WiFi.status() == WL_CONNECTED;
}