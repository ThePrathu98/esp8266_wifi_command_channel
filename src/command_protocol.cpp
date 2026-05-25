#include "command_protocol.h"

#include <ESP8266WiFi.h>
#include <cstring>

#include "oled_ui.h"

/*
Converts one received TCP command into a firmware action and response.

Supported:
GET_STATUS
CLEAR
SHOW_TEXT <message>
SHOW_NUMBER <number>
INVERT
*/
String handleCommand(const String &command, bool clientConnected)
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
        oledUiClearAndHold();

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

        oledUiShowText(message);

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

        oledUiShowNumber(numberText);

        return "OK SHOW_NUMBER";
    }

    /*
    INVERT command.

    The OLED UI module owns the actual display invert state. The protocol
    layer only maps the command into the OLED action and returns the response.
    */
    if (trimmedCommand == "INVERT")
    {
        return oledUiToggleInvert(clientConnected);
    }

    return "ERR UNKNOWN_COMMAND";
}
