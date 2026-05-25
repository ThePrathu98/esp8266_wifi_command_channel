#include "tcp_server.h"

#include <ESP8266WiFi.h>

#include "command_protocol.h"
#include "oled_ui.h"

/*
TCP server listens on port 5005.
Laptop/Python/PyQt client connects to ESP8266_IP:5005.
*/
static WiFiServer tcpServer(TCP_SERVER_PORT);

/*
One active TCP client connection.
This project supports one laptop client at a time.
*/
static WiFiClient tcpClient;

/*
TCP is a byte stream, not message-based.
So we collect characters until newline '\n' completes one command.
*/
static String tcpRxBuffer = "";

/*
Max buffer protection to avoid memory issues if client sends too much data
without newline.
*/
static const size_t TCP_RX_BUFFER_LIMIT = 128;

bool tcpServerHasConnectedClient()
{
    return tcpClient && tcpClient.connected();
}

/*
Closes the current TCP client and clears any partial command data.

Why:
TCP is a byte stream. If a client disconnects in the middle of a command,
tcpRxBuffer may contain leftover characters. Clearing it prevents the next
client command from mixing with old data.
*/
static void closeTcpClient()
{
    if (tcpRxBuffer.length() > 0)
    {
        Serial.println("TCP partial command cleared.");
    }

    if (tcpClient)
    {
        tcpClient.stop();
    }

    tcpRxBuffer = "";
}

/*
Start TCP server after Wi-Fi is connected.
*/
void tcpServerBegin()
{
    tcpServer.begin();
    tcpServer.setNoDelay(true);
}

/*
Poll TCP server and active TCP client.

This function is intentionally non-blocking. It accepts a waiting client,
reads any available bytes, handles complete newline-terminated commands, and
then returns so loop() can continue feeding ESP8266 Wi-Fi/lwIP background work.
*/
void tcpServerPoll()
{
    /*
    If the previous client disconnected, close it cleanly and clear old RX data.
    Then check if a new laptop/Python/GUI client is waiting.
    */
    if (!tcpClient || !tcpClient.connected())
    {
        closeTcpClient();

        WiFiClient newClient = tcpServer.accept();

        if (newClient)
        {
            tcpClient = newClient;
            tcpClient.setNoDelay(true);

            Serial.println("TCP client connected.");
            oledUiUpdateStatus(true);
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

            String response = handleCommand(tcpRxBuffer, tcpServerHasConnectedClient());

            tcpClient.println(response);

            Serial.print("TCP response sent: ");
            Serial.println(response);

            tcpRxBuffer = "";
        }
        else
        {
            /*
            Append received TCP characters only while the command buffer is
            within limit.

            This prevents a very long/malformed command from growing the
            String forever and wasting ESP8266 RAM.
            */
            if (tcpRxBuffer.length() < TCP_RX_BUFFER_LIMIT)
            {
                tcpRxBuffer += rxChar;
            }
            else
            {
                Serial.println("TCP RX buffer overflow. Closing client.");
                closeTcpClient();
                break;
            }
        }
    }
}
