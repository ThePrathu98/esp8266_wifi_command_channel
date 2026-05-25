# Project B Test Results

Use the full command list as an internal verification checklist. The final submission does not need screenshots/video for every test. The main video should focus on GUI-driven commands because the final Project B objective is GUI -> TCP -> ESP8266 -> OLED.

## Minimum Evidence Set

```text
1. PlatformIO build/upload success
2. Monitor boot log showing Wi-Fi connected, IP address, and TCP server on port 5005
3. Test-NetConnection or ping success
4. GUI GET_STATUS success
5. GUI SHOW_TEXT success + OLED photo/video
6. GUI SHOW_NUMBER success + OLED photo/video
7. GUI CLEAR success
8. GUI INVERT ON and INVERT OFF success + OLED photo/video
9. One PowerShell robustness/error screenshot
10. Overflow/recovery monitor screenshot if available
11. Final output video showing GUI -> TCP -> ESP8266 -> OLED
```

## Firmware Startup

Expected monitor evidence:

```text
Project B - Wi-Fi Command Channel
Connecting to Wi-Fi: <SSID>
Wi-Fi connected successfully!
ESP8266 IP address: <ip>
Signal strength RSSI: -xx dBm
TCP server started on port 5005
```

Expected OLED:

```text
Project B WiFi
<ip>
RSSI: -xx dBm
TCP: Ready
```

## Network Reachability

Recommended screenshot:

```powershell
Test-NetConnection <ip> -Port 5005
```

Expected:

```text
TcpTestSucceeded : True
```

## GUI Command Tests

These are the main final demo tests.

| Test | GUI Action | Expected GUI Response | Expected OLED |
|---|---|---|---|
| G1 | GET_STATUS | `OK STATUS IP=<ip> RSSI=-xx TCP=CONNECTED` | Status screen |
| G2 | SHOW_TEXT `Hello ALSO` | `OK SHOW_TEXT` | Shows `SHOW_TEXT` and `Hello ALSO` |
| G3 | SHOW_NUMBER `123` | `OK SHOW_NUMBER` | Shows `SHOW_NUMBER` and `123` |
| G4 | CLEAR | `OK CLEAR` | Blank display for about 10 seconds |
| G5 | INVERT first click | `OK INVERT TEXT_BG_COLORS ON` | Inverted display, line 5 ON |
| G6 | INVERT second click | `OK INVERT TEXT_BG_COLORS OFF` | Normal display, line 5 OFF briefly |

## GUI Local Validation

Recommended to test, but not necessary to show in final video.

| Test | GUI Action | Expected Result |
|---|---|---|
| Empty SHOW_TEXT | clear text field, click SHOW_TEXT | `ERR GUI_EMPTY_TEXT` |
| Empty SHOW_NUMBER | clear number field, click SHOW_NUMBER | `ERR GUI_EMPTY_NUMBER` |
| Invalid port | enter `abc`, click GET_STATUS | `ERR INVALID_PORT` |
| Wrong IP | enter unused IP, click GET_STATUS | host timeout/socket error |

## PowerShell / Robustness Tests

Use these for screenshots or test-results evidence, not for the main demo video.

Recommended parser tests:

```text
hello       -> ERR UNKNOWN_COMMAND
SHOW_TEXT   -> ERR BAD_ARGUMENT
SHOW_NUMBER -> ERR BAD_ARGUMENT
```

Recommended robustness test:

```text
Send long malformed command -> monitor shows buffer overflow / client close
Then send GET_STATUS -> OK STATUS ...
```

This proves malformed input does not freeze the board and the next clean client still works.

## Final Video Script

Keep the video around 3 to 5 minutes.

Show:

```text
1. ESP8266 + OLED hardware
2. PlatformIO Monitor with IP address and TCP server started
3. OLED default status screen
4. PyQt6 GUI open with IP and port 5005
5. GET_STATUS
6. SHOW_TEXT Hello ALSO and OLED update
7. SHOW_NUMBER 123 and OLED update
8. CLEAR and OLED blank
9. INVERT ON and OLED inverted
10. INVERT OFF and OLED returns normal
11. Monitor TCP RX / response logs
```

## OLED Image Capture

There is no direct screenshot function for a basic SSD1306 OLED. Capture OLED evidence using:

```text
Phone photo
Phone video
Webcam recording
```

CLEAR is better shown in video because a blank OLED photo is less informative.

## Final Result

Final result is PASS when:

- firmware builds and uploads
- ESP8266 joins Wi-Fi
- TCP server starts on port 5005
- GUI sends all five required commands
- OLED visibly updates for display commands
- monitor logs show TCP RX commands and responses
- malformed input does not crash or freeze the board
