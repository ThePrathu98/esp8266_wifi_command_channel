#include <Arduino.h>
#include <ESP8266WiFi.h>

/*
Private Wi-Fi credentials.
wifi_secrets.h contains WIFI_SSID and WIFI_PASSWORD.
This file is ignored by Git for security.
*/
#include "wifi_secrets.h"

#include "oled_ui.h"
#include "tcp_server.h"

/*
Project B - Wi-Fi Command Channel

main.cpp is now the application layer:
- Serial startup logging
- OLED layer initialization
- Wi-Fi STA connection
- TCP server startup
- non-blocking loop scheduling

The detailed OLED, command parser, and TCP transport logic were moved into
separate files so the project better matches BSP / driver / protocol / app
layering expectations.
*/

void setup()
{
    Serial.begin(115200);
    delay(1000);

    Serial.println();
    Serial.println("=================================");
    Serial.println("Project B - Wi-Fi Command Channel");
    Serial.println("=================================");

    /*
    Initialize OLED.
    If OLED init fails, stop here because display feedback is required.
    */
    if (!oledUiInit())
    {
        Serial.println("OLED initialization failed.");

        while (true)
        {
            delay(1000);
        }
    }

    /*
    ESP8266 joins existing Wi-Fi network as station.
    */
    WiFi.mode(WIFI_STA);

    Serial.print("Connecting to Wi-Fi: ");
    Serial.println(WIFI_SSID);

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

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

    /*
    Start TCP server after Wi-Fi is connected.
    */
    tcpServerBegin();

    Serial.print("TCP server started on port ");
    Serial.println(TCP_SERVER_PORT);

    oledUiUpdateStatus(tcpServerHasConnectedClient());
}

void loop()
{
    if (WiFi.status() == WL_CONNECTED)
    {
        /*
        Poll TCP without blocking. This accepts clients, reads available bytes,
        handles newline-completed commands, and returns quickly.
        */
        tcpServerPoll();

        /*
        Periodically refresh OLED status while respecting temporary holds for
        SHOW_TEXT, SHOW_NUMBER, CLEAR, and INVERT feedback.
        */
        oledUiPeriodicUpdate(tcpServerHasConnectedClient());
    }
    else
    {
        Serial.println("Wi-Fi disconnected.");
        showOledMessage("Project B WiFi", "WiFi Lost", "Recheck router", "");
    }

    /*
    Lets ESP8266 Wi-Fi background work run.
    Helps avoid watchdog/Wi-Fi instability.
    */
    yield();
}
