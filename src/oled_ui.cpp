#include "oled_ui.h"

#include <ESP8266WiFi.h>

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
Normal OLED/Serial status refresh timer.
Using millis() avoids blocking delay().
*/
static unsigned long lastStatusUpdateMs = 0;
static const unsigned long STATUS_UPDATE_INTERVAL_MS = 5000;

/*
OLED command hold feature.

SHOW_TEXT, SHOW_NUMBER, and CLEAR use this because they temporarily replace
the normal Wi-Fi/TCP status screen.
*/
static bool oledHoldActive = false;
static unsigned long oledHoldStartMs = 0;
static const unsigned long OLED_HOLD_DURATION_MS = 10000;

/*
Temporary OLED line-5 notice for INVERT.

Default OLED screen:
Line 1: Project B WiFi
Line 2: IP address
Line 3: RSSI
Line 4: TCP: Ready / Client Connected

After INVERT is clicked:
Line 5 briefly shows INVERT MODE: ON/OFF.
After 5 seconds, line 5 disappears.
*/
static bool invertNoticeActive = false;
static unsigned long invertNoticeStartMs = 0;
static const unsigned long INVERT_NOTICE_DURATION_MS = 5000;
static String invertNoticeText = "";

/*
OLED invert state.
Each INVERT command toggles this value.
*/
static bool oledInverted = false;

/*
OLED display object using I2C.
*/
static Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

/*
Initialize the OLED UI layer.

This keeps I2C/OLED setup in the OLED module instead of main.cpp.
The same pins and OLED address from the original working project are used.
*/
bool oledUiInit()
{
    /*
    Start I2C on NodeMCU OLED pins.
    */
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);

    /*
    Initialize OLED.
    If OLED init fails, return false so setup() can stop, because display
    feedback is required for this project.
    */
    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS))
    {
        return false;
    }

    showOledMessage("Project B WiFi", "OLED Ready", "Connecting...", "");
    return true;
}

/*
Writes up to five text lines to OLED.

line5 is optional because it has a default value of "".
That means older calls with only four lines still work.
*/
void showOledMessage(const char *line1,
                     const char *line2,
                     const char *line3,
                     const char *line4,
                     const char *line5)
{
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);

    display.println(line1);
    display.println(line2);
    display.println(line3);
    display.println(line4);
    display.println(line5);

    display.display();
}

/*
Starts OLED hold mode.
Used after CLEAR, SHOW_TEXT, and SHOW_NUMBER.
*/
static void startOledHold()
{
    oledHoldActive = true;
    oledHoldStartMs = millis();
}

/*
Starts a temporary line-5 OLED notice for INVERT.

This does not replace the whole screen.
It only adds a short status line below the normal Wi-Fi/TCP status.
*/
static void startInvertNotice(const String &noticeText)
{
    invertNoticeActive = true;
    invertNoticeStartMs = millis();
    invertNoticeText = noticeText;
}

/*
Displays normal board status on OLED.

Default:
Line 1: Project B WiFi
Line 2: IP address
Line 3: RSSI
Line 4: TCP: Ready or Client Connected

Temporary:
Line 5: INVERT MODE: ON/OFF for a few seconds after INVERT command.
*/
void oledUiUpdateStatus(bool clientConnected)
{
    IPAddress ip = WiFi.localIP();
    int rssi = WiFi.RSSI();

    String line4 = clientConnected ? "Client Connected" : "TCP: Ready";
    String line5 = invertNoticeActive ? invertNoticeText : "";

    showOledMessage("Project B WiFi",
                    ip.toString().c_str(),
                    ("RSSI: " + String(rssi) + " dBm").c_str(),
                    line4.c_str(),
                    line5.c_str());
}

/*
CLEAR command display behavior.

The display is blanked, then hold mode prevents the normal status screen from
immediately overwriting the blank display.
*/
void oledUiClearAndHold()
{
    display.clearDisplay();
    display.display();

    startOledHold();
}

/*
SHOW_TEXT display behavior.

The command temporarily replaces the normal Wi-Fi/TCP status screen, so hold
mode keeps the message visible for about 10 seconds.
*/
void oledUiShowText(const String &message)
{
    showOledMessage("SHOW_TEXT", message.c_str(), "", "");
    startOledHold();
}

/*
SHOW_NUMBER display behavior.

The command temporarily replaces the normal Wi-Fi/TCP status screen, so hold
mode keeps the number visible for about 10 seconds.
*/
void oledUiShowNumber(const String &numberText)
{
    showOledMessage("SHOW_NUMBER", numberText.c_str(), "", "");
    startOledHold();
}

/*
INVERT command OLED behavior.

This toggles the OLED controller's text/background color mode.
It does not flip or mirror the text.

Behavior:
- When inversion turns ON, line 5 shows "INVERT MODE: ON" and stays visible.
- When inversion turns OFF, line 5 briefly shows "INVERT MODE: OFF" and then disappears.
*/
String oledUiToggleInvert(bool clientConnected)
{
    oledInverted = !oledInverted;
    display.invertDisplay(oledInverted);

    if (oledInverted)
    {
        /*
        Inverted mode is now active.

        Keep this notice visible until the next INVERT command turns inverted
        mode OFF.
        */
        invertNoticeActive = true;
        invertNoticeText = "INVERT MODE: ON";
    }
    else
    {
        /*
        Inverted mode is now inactive.

        Show OFF message temporarily. The periodic OLED timing logic will
        remove it after INVERT_NOTICE_DURATION_MS.
        */
        startInvertNotice("INVERT MODE: OFF");
    }

    oledUiUpdateStatus(clientConnected);

    return oledInverted ? "OK INVERT TEXT_BG_COLORS ON" :
                          "OK INVERT TEXT_BG_COLORS OFF";
}

/*
OLED status refresh with hold protection.

SHOW_TEXT, SHOW_NUMBER, and CLEAR use oledHoldActive because they
temporarily replace the whole display.

INVERT uses invertNoticeActive because it only adds a temporary line 5.
*/
void oledUiPeriodicUpdate(bool clientConnected)
{
    unsigned long nowMs = millis();

    if (oledHoldActive)
    {
        if (nowMs - oledHoldStartMs >= OLED_HOLD_DURATION_MS)
        {
            oledHoldActive = false;
            oledUiUpdateStatus(clientConnected);
            lastStatusUpdateMs = nowMs;
        }
    }
    else
    {
        /*
        Remove the temporary INVERT OFF notice after 5 seconds.

        Important:
        - If oledInverted == true, keep "INVERT MODE: ON" visible.
        - If oledInverted == false, the OFF notice is temporary and disappears.
        */
        if (invertNoticeActive &&
            !oledInverted &&
            (nowMs - invertNoticeStartMs >= INVERT_NOTICE_DURATION_MS))
        {
            invertNoticeActive = false;
            oledUiUpdateStatus(clientConnected);
            lastStatusUpdateMs = nowMs;
        }

        if (nowMs - lastStatusUpdateMs >= STATUS_UPDATE_INTERVAL_MS)
        {
            lastStatusUpdateMs = nowMs;

            Serial.print("Still connected. IP: ");
            Serial.print(WiFi.localIP());
            Serial.print(" RSSI: ");
            Serial.print(WiFi.RSSI());
            Serial.println(" dBm");

            oledUiUpdateStatus(clientConnected);
        }
    }
}
