#include <Arduino.h>
#include <ESP8266WiFi.h>

/*
Private Wi-Fi credentials.
wifi_secrets.h contains WIFI_SSID and WIFI_PASSWORD.
This file is ignored by Git for security.
*/
#include "wifi_secrets.h"

/*
OLED libraries:
Wire.h              -> I2C communication
Adafruit_GFX.h      -> text/graphics functions
Adafruit_SSD1306.h  -> SSD1306 OLED driver
*/
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>


/*
ESP8266 NodeMCU OLED I2C pins:
D2 = GPIO4 = SDA
D1 = GPIO5 = SCL
*/
#define I2C_SDA_PIN D2
#define I2C_SCL_PIN D1

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDRESS 0x3C
#define OLED_RESET -1


/*
TCP server listens on port 5005.
Laptop/Python/PyQt client connects to ESP8266_IP:5005.
*/
WiFiServer tcpServer(5005);

/*
One active TCP client connection.
This project supports one laptop client at a time.
*/
WiFiClient tcpClient;

/*
TCP is a byte stream, not message-based.
So we collect characters until newline '\n' completes one command.
*/
String tcpRxBuffer = "";


/*
OLED invert state.
Each INVERT command toggles this value.
*/
bool oledInverted = false;


/*
Normal OLED/Serial status refresh timer.
Using millis() avoids blocking delay().
*/
unsigned long lastStatusUpdateMs = 0;
const unsigned long STATUS_UPDATE_INTERVAL_MS = 5000;


/*
OLED command hold feature.

Problem:
SHOW_TEXT and SHOW_NUMBER were being overwritten by the normal status screen.

Solution:
After command output is shown, hold it for 10 seconds before returning to status.
*/
bool oledHoldActive = false;
unsigned long oledHoldStartMs = 0;
const unsigned long OLED_HOLD_DURATION_MS = 10000;


/*
OLED display object using I2C.
*/
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


/*
Writes four text lines to OLED.
display.display() pushes the framebuffer to the real OLED over I2C.
*/
void showOledMessage(const char *line1,
                     const char *line2,
                     const char *line3,
                     const char *line4)
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


/*
Starts OLED hold mode.
Used after CLEAR, SHOW_TEXT, and SHOW_NUMBER.
*/
void startOledHold()
{
    oledHoldActive = true;
    oledHoldStartMs = millis();
}


/*
Displays normal board status on OLED.
This avoids repeating IP/RSSI/TCP display code in many places.
*/
void updateStatusDisplay()
{
    IPAddress ip = WiFi.localIP();
    int rssi = WiFi.RSSI();

    showOledMessage("Project B WiFi",
                    ip.toString().c_str(),
                    ("RSSI: " + String(rssi) + " dBm").c_str(),
                    (tcpClient && tcpClient.connected()) ? "Client Connected" : "TCP: Ready");
}


/*
Converts one received TCP command into a firmware action and response.

Supported:
GET_STATUS
CLEAR
SHOW_TEXT <message>
SHOW_NUMBER <number>
INVERT
*/
String handleCommand(const String &command)
{
    String trimmedCommand = command;
    trimmedCommand.trim();

    if (trimmedCommand.length() == 0)
    {
        return "ERR EMPTY_COMMAND";
    }

    if (trimmedCommand == "GET_STATUS")
    {
        String response = "OK STATUS IP=";
        response += WiFi.localIP().toString();
        response += " RSSI=";
        response += String(WiFi.RSSI());
        response += " TCP=CONNECTED";
        return response;
    }

    if (trimmedCommand == "CLEAR")
    {
        display.clearDisplay();
        display.display();

        startOledHold();

        return "OK CLEAR";
    }

    /*
    If user sends only SHOW_TEXT without a message,
    the command is valid but the argument is missing.
    */
    if (trimmedCommand == "SHOW_TEXT")
    {
        return "ERR BAD_ARGUMENT";
    }

    if (trimmedCommand.startsWith("SHOW_TEXT "))
    {
        String message = trimmedCommand.substring(strlen("SHOW_TEXT "));
        message.trim();

        if (message.length() == 0)
        {
            return "ERR BAD_ARGUMENT";
        }

        showOledMessage("SHOW_TEXT", message.c_str(), "", "");
        startOledHold();

        return "OK SHOW_TEXT";
    }

    /*
    If user sends only SHOW_NUMBER without a number,
    the command is valid but the argument is missing.
    */
    if (trimmedCommand == "SHOW_NUMBER")
    {
        return "ERR BAD_ARGUMENT";
    }
    
    if (trimmedCommand.startsWith("SHOW_NUMBER "))
    {
        String numberText = trimmedCommand.substring(strlen("SHOW_NUMBER "));
        numberText.trim();

        if (numberText.length() == 0)
        {
            return "ERR BAD_ARGUMENT";
        }

        showOledMessage("SHOW_NUMBER", numberText.c_str(), "", "");
        startOledHold();

        return "OK SHOW_NUMBER";
    }

    if (trimmedCommand == "INVERT")
    {
        oledInverted = !oledInverted;
        display.invertDisplay(oledInverted);

        return oledInverted ? "OK INVERT ON" : "OK INVERT OFF";
    }

    return "ERR UNKNOWN_COMMAND";
}


void setup()
{
    Serial.begin(115200);
    delay(1000);

    Serial.println();
    Serial.println("=================================");
    Serial.println("Project B - Wi-Fi Command Channel");
    Serial.println("=================================");

    /*
    Start I2C on NodeMCU OLED pins.
    */
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);

    /*
    Initialize OLED.
    If OLED init fails, stop here because display feedback is required.
    */
    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS))
    {
        Serial.println("OLED initialization failed.");

        while (true)
        {
            delay(1000);
        }
    }

    showOledMessage("Project B WiFi", "OLED Ready", "Connecting...", "");

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
    tcpServer.begin();
    tcpServer.setNoDelay(true);

    Serial.println("TCP server started on port 5005");

    updateStatusDisplay();
}


void loop()
{
    if (WiFi.status() == WL_CONNECTED)
    {
        /*
        Accept new client only when no client is currently connected.
        */
        if (!tcpClient || !tcpClient.connected())
        {
            WiFiClient newClient = tcpServer.accept();

            if (newClient)
            {
                tcpClient = newClient;
                tcpClient.setNoDelay(true);

                Serial.println("TCP client connected.");
                updateStatusDisplay();
            }
        }

        /*
        Read all available TCP bytes.
        '\r' is ignored.
        '\n' completes one command.
        */
        while (tcpClient && tcpClient.connected() && tcpClient.available())
        {
            char rxChar = tcpClient.read();

            if (rxChar == '\r')
            {
                continue;
            }

            if (rxChar == '\n')
            {
                Serial.print("TCP RX command: ");
                Serial.println(tcpRxBuffer);

                String response = handleCommand(tcpRxBuffer);

                tcpClient.println(response);

                Serial.print("TCP response sent: ");
                Serial.println(response);

                tcpRxBuffer = "";
            }
            else
            {
                tcpRxBuffer += rxChar;
            }
        }

        /*
        OLED status refresh with hold protection.

        If command output is active, do not overwrite OLED until 10 seconds pass.
        If no command output is active, refresh normal status every 5 seconds.
        */
        unsigned long nowMs = millis();

        if (oledHoldActive)
        {
            if (nowMs - oledHoldStartMs >= OLED_HOLD_DURATION_MS)
            {
                oledHoldActive = false;
                updateStatusDisplay();
                lastStatusUpdateMs = nowMs;
            }
        }
        else if (nowMs - lastStatusUpdateMs >= STATUS_UPDATE_INTERVAL_MS)
        {
            lastStatusUpdateMs = nowMs;

            Serial.print("Still connected. IP: ");
            Serial.print(WiFi.localIP());
            Serial.print(" RSSI: ");
            Serial.print(WiFi.RSSI());
            Serial.println(" dBm");

            updateStatusDisplay();
        }
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