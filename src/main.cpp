#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "wifi_secrets.h"

//1.Added OLED display/I2C libraries
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>


//2.Added OLED pin/address configuration macros

#define I2C_SDA_PIN D2
#define I2C_SCL_PIN D1

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDRESS 0x3C
#define OLED_RESET -1


//3.Added OLED display object functionality, using I2C constructor with pin definitions from above

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


//4.Added helper function for OLED messages

void showOledMessage(const char *line1, const char *line2, const char *line3, const char *line4)
{
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);

    display.println(line1);
    display.println(line2);
    display.println(line3);
    display.println(line4);

    display.display();
}


void setup()
{
    Serial.begin(115200);
    delay(1000);

    Serial.println();
    Serial.println("=================================");
    Serial.println("Project B - Wi-Fi Connect Test");
    Serial.println("=================================");

    //5.Added OLED initialization inside setup(), before wi-fi connection, 
    //to provide visual feedback during bring-up. I2C pins defined in macros above.

    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);

    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS))
    {
        Serial.println("OLED initialization failed.");

        while (true)
        {
            delay(1000);
        }
    }

    showOledMessage("Project B WiFi", "OLED Ready", "Connecting...", "");


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


    //6.Added/ Edited wi-fi connection section to store IP, store it in a variable, 
    //and print it to serial and OLED for visual confirmation of connection details
    //during bring-up.
    IPAddress ip = WiFi.localIP();


    Serial.print("ESP8266 IP address: ");
    Serial.println(ip);


    Serial.print("Signal strength RSSI: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");


    //7.Added OLED update after Wi-Fi connects, function call showOledMessage()
    showOledMessage("Project B WiFi", "WiFi Connected", "IP:", ip.toString().c_str());

}

void loop()
{
    // Basic connection heartbeat for bring-up.
    // Later this area will become part of reconnect/status handling.
    //9.Added live RSSI display to show Wi-Fi signal strength on Serial and OLED during bring-up.
    if (WiFi.status() == WL_CONNECTED)
    {
        IPAddress ip = WiFi.localIP();
        int rssi = WiFi.RSSI();

        Serial.print("Still connected. IP: ");
        Serial.print(ip);
        Serial.print(" RSSI: ");
        Serial.print(rssi);
        Serial.println(" dBm");

        showOledMessage("Project B WiFi",
                        ip.toString().c_str(),
                        ("RSSI: " + String(rssi) + " dBm").c_str(),
                        "TCP: Not started");
    }
    else
    {
        //8.Added/edited disconnected case in loop() to print message to serial and OLED for visual 
        //feedback during bring-up.
        Serial.println("Wi-Fi disconnected.");

        showOledMessage("Project B WiFi", "WiFi Lost", "Recheck router", "");

    }
    // Acceptable for this simple test; final Project B loop must return often for lwIP/TCP and OLED timing.
    delay(5000);
}