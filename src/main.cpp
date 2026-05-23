#include <Arduino.h>
#include <ESP8266WiFi.h>

/*Local private header containing WIFI_SSID and WIFI_PASSWORD.
This file is excluded from Git; only wifi_secrets.h.template is committed.*/
#include "wifi_secrets.h"

/*OLED/I2C support:
Wire.h provides the I2C HAL, Adafruit_GFX provides text/graphics APIs,
 and Adafruit_SSD1306 drives the 128x64 OLED controller.
*/
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>


/*OLED hardware configuration for ESP8266 NodeMCU.
D2 maps to GPIO4/SDA and D1 maps to GPIO5/SCL for the I2C OLED.*/

#define I2C_SDA_PIN D2
#define I2C_SCL_PIN D1

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDRESS 0x3C
#define OLED_RESET -1


// TCP server object listens on Project B port 5005 so the laptop/Python GUI can connect over Wi-Fi.
WiFiServer tcpServer(5005);


/*Tracks the currently connected laptop TCP client.
The first implementation supports one client at a time, which is enough for command testing.*/

WiFiClient tcpClient;


/* Line-based receive buffer. TCP is a byte stream, so characters are collected 
until '\n' or '\r' marks one full command. */
String tcpRxBuffer = "";


// Maintains current OLED invert state so each INVERT command can toggle ON/OFF predictably.
bool oledInverted = false;


/* Non-blocking status update timer.
 millis() lets loop() refresh Serial/OLED status periodically without delay(5000),
 so TCP receive handling and the ESP8266 Wi-Fi/lwIP stack stay responsive. */

unsigned long lastStatusUpdateMs = 0;
const unsigned long STATUS_UPDATE_INTERVAL_MS = 5000;


//3.Added OLED display object functionality, using I2C constructor with pin definitions from above
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


/* Small OLED helper: writes up to four text lines to the display.
const char* is used because Adafruit print APIs accept C-style strings. */

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

/*
//15.Added a command handler function to convert TCP text commands into firmware responses,
// starting with GET_STATUS.

String handleCommand(const String &command)
{
    if (command == "GET_STATUS")
    {
        String response = "OK STATUS IP=";
        response += WiFi.localIP().toString();
        response += " RSSI=";
        response += String(WiFi.RSSI());
        response += " TCP=CONNECTED";

        return response;
    }

    return "ERR UNKNOWN_COMMAND";
}
*/

/*Command dispatcher for TCP messages.
Converts one received text command into an OLED action and/or response string.

Supported commands: GET_STATUS, CLEAR, SHOW_TEXT <msg>, SHOW_NUMBER <num>, INVERT.*/

String handleCommand(const String &command)
{
    // Work on a trimmed copy so extra spaces/newline characters do not break command matching.
    String trimmedCommand = command;
    trimmedCommand.trim();

    if (trimmedCommand.length() == 0)
    {
        return "ERR EMPTY_COMMAND";
    }

    // Status command returns board IP, live RSSI, and TCP state to the host.
    if (trimmedCommand == "GET_STATUS")
    {
        String response = "OK STATUS IP=";
        response += WiFi.localIP().toString();
        response += " RSSI=";
        response += String(WiFi.RSSI());
        response += " TCP=CONNECTED";

        return response;
    }

    // clearDisplay() updates the local framebuffer; display() pushes it to the physical OLED.
    if (trimmedCommand == "CLEAR")
    {
        display.clearDisplay();
        display.display();

        return "OK CLEAR";
    }

    // Extract text after "SHOW_TEXT " and render it on the OLED.
    if (trimmedCommand.startsWith("SHOW_TEXT "))
    {
        String message = trimmedCommand.substring(strlen("SHOW_TEXT "));
        message.trim();

        if (message.length() == 0)
        {
            return "ERR BAD_ARGUMENT";
        }

        showOledMessage("SHOW_TEXT", message.c_str(), "", "");

        return "OK SHOW_TEXT";
    }

    // Extract number text after "SHOW_NUMBER " and render it on the OLED.
    if (trimmedCommand.startsWith("SHOW_NUMBER "))
    {
        String numberText = trimmedCommand.substring(strlen("SHOW_NUMBER "));
        numberText.trim();

        if (numberText.length() == 0)
        {
            return "ERR BAD_ARGUMENT";
        }

        showOledMessage("SHOW_NUMBER", numberText.c_str(), "", "");

        return "OK SHOW_NUMBER";
    }

    // Toggle OLED inversion state and return whether inversion is now ON or OFF.
    if (trimmedCommand == "INVERT")
    {
        oledInverted = !oledInverted;
        display.invertDisplay(oledInverted);

        if (oledInverted)
        {
            return "OK INVERT ON";
        }

        return "OK INVERT OFF";
    }

    return "ERR UNKNOWN_COMMAND";
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


    /*Simple blocking Wi-Fi wait is acceptable during bring-up.
    Later robustness work can replace this with reconnect/non-blocking handling. */
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


    /*//7.Added OLED update after Wi-Fi connects, function call showOledMessage()
    showOledMessage("Project B WiFi", "WiFi Connected", "IP:", ip.toString().c_str()); */

    // Start TCP server only after Wi-Fi is connected and the board has a valid IP address.
    tcpServer.begin();
    tcpServer.setNoDelay(true);

    Serial.println("TCP server started on port 5005");

    showOledMessage("Project B WiFi",
                    ip.toString().c_str(),
                    "RSSI: Ready",
                    "TCP: Ready");

}

/*
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
                        "TCP: Ready");          //12.Added Updated OLED heartbeat TCP: Ready to show TCP server status 
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
*/



/* Active Project B loop:
1) accepts TCP client connections,
2) receives line-based commands,
3) dispatches commands through handleCommand(),
4) updates OLED/Serial status without blocking,
5) calls yield() so ESP8266 Wi-Fi/lwIP background work can run. */

void loop()
{
    if (WiFi.status() == WL_CONNECTED)
    {
        // If no client is connected, check whether the laptop has opened a new TCP connection.
        if (!tcpClient || !tcpClient.connected())
        {
            // accept() checks for a new pending TCP client connection without blocking.
            WiFiClient newClient = tcpServer.accept();

            if (newClient)
            {
                tcpClient = newClient;
                tcpClient.setNoDelay(true);

                Serial.println("TCP client connected.");

                showOledMessage("Project B WiFi",
                                WiFi.localIP().toString().c_str(),
                                ("RSSI: " + String(WiFi.RSSI()) + " dBm").c_str(),
                                "Client Connected");
            }
        }

        /*Drain available TCP bytes without blocking.
        The command is considered complete when '\n' or '\r' is received.*/
        while (tcpClient && tcpClient.connected() && tcpClient.available())
        {
            char rxChar = tcpClient.read();

            if (rxChar == '\n' || rxChar == '\r')
            {
                if (tcpRxBuffer.length() > 0)
                {
                    /*16.Added this: Replaced echo send inside TCP receive block 
                    Serial.print("TCP RX: ");
                    Serial.println(tcpRxBuffer);

                    // Echo milestone: send the same message back to prove laptop-to-board TCP communication works.
                    tcpClient.println(tcpRxBuffer);

                    Serial.print("TCP echo sent: ");
                    Serial.println(tcpRxBuffer);

                    tcpRxBuffer = "";
                    */
                    
                    //16.Added command-processing response path to replace raw TCP echo, so received TCP 
                    //messages now go through handleCommand() before replying to the laptop.

                    Serial.print("TCP RX command: ");
                    Serial.println(tcpRxBuffer);


                    // Route the received TCP line into the command dispatcher instead of echoing raw text.
                    String response = handleCommand(tcpRxBuffer);


                    // Send command result back to the laptop/Python client.
                    tcpClient.println(response);

                    Serial.print("TCP response sent: ");
                    Serial.println(response);

                    tcpRxBuffer = "";
                }
            }
            else
            {
                tcpRxBuffer += rxChar;
            }
        }

        unsigned long nowMs = millis();

        if (nowMs - lastStatusUpdateMs >= STATUS_UPDATE_INTERVAL_MS)
        {
            lastStatusUpdateMs = nowMs;

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
                            (tcpClient && tcpClient.connected()) ? "Client Connected" : "TCP: Ready");
        }
    }
    else
    {
        Serial.println("Wi-Fi disconnected.");

        showOledMessage("Project B WiFi", "WiFi Lost", "Recheck router", "");
    }

    // Let the ESP8266 background Wi-Fi/lwIP stack run.
    yield();
}