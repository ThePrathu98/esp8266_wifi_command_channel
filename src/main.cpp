#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "wifi_secrets.h"

void setup()
{
    Serial.begin(115200);
    delay(1000);

    Serial.println();
    Serial.println("=================================");
    Serial.println("Project B - Wi-Fi Connect Test");
    Serial.println("=================================");
    // STA mode: ESP8266 joins an existing router/hotspot.
    // This matches Project B architecture where the host PC
    // connects to the board over Wi-Fi/TCP.
    WiFi.mode(WIFI_STA);

    Serial.print("Connecting to Wi-Fi: ");
    Serial.println(WIFI_SSID);

    // Start Wi-Fi association using credentials kept outside Git in wifi_secrets.h.
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    // Initial bring-up uses a simple blocking wait.
    // Later this should become non-blocking/reconnect-aware so TCP and OLED updates stay responsive.
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }

    Serial.println();
    Serial.println("Wi-Fi connected successfully!");

    Serial.print("ESP8266 IP address: ");
    Serial.println(WiFi.localIP());

    Serial.print("Signal strength RSSI: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
}

void loop()
{
    // Basic connection heartbeat for bring-up.
    // Later this area will become part of reconnect/status handling.
    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.print("Still connected. IP: ");
        Serial.println(WiFi.localIP());
    }
    else
    {
        Serial.println("Wi-Fi disconnected.");
    }
    // Acceptable for this simple test; final Project B loop must return often for lwIP/TCP and OLED timing.
    delay(5000);
}