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


//10.Added TCP server object with port number 5005
WiFiServer tcpServer(5005);



/*13.Added TCP client object and receive buffer for first laptop-to-board echo test
Tracks the currently connected TCP client from the laptop.
For now we support one client at a time, which is enough for the Project B echo test. */

WiFiClient tcpClient;


// Temporary receive buffer used to collect TCP bytes until a full line is received.
String tcpRxBuffer = "";


/*Non-blocking status timer.
This replaces delay(5000) so the loop can keep checking TCP data frequently. */

unsigned long lastStatusUpdateMs = 0;
const unsigned long STATUS_UPDATE_INTERVAL_MS = 5000;



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


    /*//7.Added OLED update after Wi-Fi connects, function call showOledMessage()
    showOledMessage("Project B WiFi", "WiFi Connected", "IP:", ip.toString().c_str()); */
    //11.Added: Start the TCP server after wi-fi connects
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


/*14.Added this: Replaced entire loop() with an active loop for Project B TCP echo milestone.
Handles client accept, TCP receive/echo, OLED status updates, and Wi-Fi stack servicing.*/

void loop()
{
    if (WiFi.status() == WL_CONNECTED)
    {
        // If no client is connected, check whether the laptop has opened a new TCP connection.
        if (!tcpClient || !tcpClient.connected())
        {
            WiFiClient newClient = tcpServer.available();

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

        /* Drain all available TCP bytes without blocking.
        Newline or carriage return marks the end of one test message.*/
        while (tcpClient && tcpClient.connected() && tcpClient.available())
        {
            char rxChar = tcpClient.read();

            if (rxChar == '\n' || rxChar == '\r')
            {
                if (tcpRxBuffer.length() > 0)
                {
                    Serial.print("TCP RX: ");
                    Serial.println(tcpRxBuffer);

                    // Echo milestone: send the same message back to prove laptop-to-board TCP communication works.
                    tcpClient.println(tcpRxBuffer);

                    Serial.print("TCP echo sent: ");
                    Serial.println(tcpRxBuffer);

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