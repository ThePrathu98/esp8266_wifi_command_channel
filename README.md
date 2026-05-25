# ESP8266 Wi-Fi Command Channel - Project B

This project implements a Wi-Fi TCP command channel on an ESP8266 NodeMCU / ESP-12E board. A host Python/PyQt6 GUI connects to the ESP8266 over Wi-Fi, sends commands over TCP port `5005`, receives `OK` / `ERR` responses, and the ESP8266 updates an SSD1306 OLED over I2C.

## Final System Flow

```text
PyQt6 GUI on laptop
    -> Wi-Fi TCP client
    -> ESP8266 TCP server on port 5005
    -> command parser
    -> OLED UI update over I2C
    -> response back to GUI
```

## Hardware Used

- ESP8266 NodeMCU / ESP-12E
- SSD1306 128x64 I2C OLED
- USB cable for power, upload, and serial monitor
- Laptop on the same Wi-Fi network as the ESP8266

## OLED Wiring

```text
ESP8266 NodeMCU / ESP-12E      SSD1306 OLED
------------------------------------------------
3V3                           VCC
GND                           GND
D1 / GPIO5                    SCL
D2 / GPIO4                    SDA
```

Use `3V3`, not `5V`, for the OLED.

## Project Structure

```text
esp8266_wi-fi_command_channel/
├── include/
│   ├── wifi_secrets.h              ignored by Git
│   └── wifi_secrets.h.template     committed example
├── src/
│   ├── main.cpp
│   ├── oled_ui.cpp
│   ├── oled_ui.h
│   ├── command_protocol.cpp
│   ├── command_protocol.h
│   ├── tcp_server.cpp
│   └── tcp_server.h
├── host_gui/
│   ├── esp8266_client.py
│   ├── gui_client.py
│   └── requirements.txt
├── docs/
│   ├── design_note.md
│   └── test_results.md
├── evidence/
│   └── screenshots / photos / video
└── platformio.ini
```

## Firmware Architecture

The firmware is split into small logical modules:

| File | Purpose |
|---|---|
| `main.cpp` | Application startup, Wi-Fi connection, TCP server start, non-blocking loop |
| `tcp_server.cpp/.h` | TCP accept, receive buffer, newline command framing, overflow protection |
| `command_protocol.cpp/.h` | Command parsing and OK/ERR response formatting |
| `oled_ui.cpp/.h` | OLED status screen, SHOW_TEXT/SHOW_NUMBER/CLEAR hold behavior, INVERT feedback |
| `host_gui/` | PyQt6 GUI and Python socket client |

This keeps the transport layer, protocol layer, OLED UI layer, and application layer separate.

## Wi-Fi Credentials

Create this file locally:

```text
include/wifi_secrets.h
```

Use:

```cpp
#pragma once

#define WIFI_SSID "YOUR_WIFI_NAME"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
```

Do not commit `wifi_secrets.h`.

The repository should include only:

```text
include/wifi_secrets.h.template
```

## Build and Upload

From PlatformIO / VS Code:

```bash
pio run
```

Upload firmware:

```bash
pio run --target upload
```

Open serial monitor:

```bash
pio device monitor
```

Expected monitor output after boot:

```text
Project B - Wi-Fi Command Channel
Connecting to Wi-Fi: <SSID>
Wi-Fi connected successfully!
ESP8266 IP address: <ip>
Signal strength RSSI: -xx dBm
TCP server started on port 5005
```

Expected OLED output:

```text
Project B WiFi
<ip>
RSSI: -xx dBm
TCP: Ready
```

## Host GUI Setup

From the project folder:

```bash
cd host_gui
python -m venv .venv
.\.venv\Scripts\activate
pip install -r requirements.txt
python gui_client.py
```

In the GUI:

```text
IP address: ESP8266 IP shown in serial monitor
Port: 5005
```

## Command Protocol

Each TCP command is ASCII text ending with newline `\n`.

| Command | Example | Expected Response | OLED Behavior |
|---|---|---|---|
| `GET_STATUS` | `GET_STATUS` | `OK STATUS IP=<ip> RSSI=<rssi> TCP=CONNECTED` | Status screen remains visible |
| `SHOW_TEXT <message>` | `SHOW_TEXT Hello ALSO` | `OK SHOW_TEXT` | Shows text for about 10 seconds |
| `SHOW_NUMBER <number>` | `SHOW_NUMBER 123` | `OK SHOW_NUMBER` | Shows number for about 10 seconds |
| `CLEAR` | `CLEAR` | `OK CLEAR` | Clears display for about 10 seconds |
| `INVERT` | `INVERT` | `OK INVERT TEXT_BG_COLORS ON/OFF` | Toggles OLED text/background color mode |

Error responses:

| Input | Response |
|---|---|
| empty newline | `ERR EMPTY_COMMAND` |
| unsupported command such as `hello` | `ERR UNKNOWN_COMMAND` |
| `SHOW_TEXT` without argument | `ERR BAD_ARGUMENT` |
| `SHOW_NUMBER` without argument | `ERR BAD_ARGUMENT` |
| too-long malformed input | client is closed and partial command is cleared |

## Network Checks

Check the ESP8266 IP from serial monitor, then run:

```powershell
ping <esp8266-ip>
```

TCP port check:

```powershell
Test-NetConnection <esp8266-ip> -Port 5005
```

Expected:

```text
TcpTestSucceeded : True
```

## Final Demo / Evidence

For final submission, the video should mainly show the GUI flow:

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
10. INVERT OFF and OLED normal
11. Monitor TCP RX / response logs
```

Recommended evidence files:

```text
evidence/
├── 01_monitor_boot_ip_tcp_server.png
├── 02_test_net_connection_success.png
├── 03_gui_get_status.png
├── 04_gui_show_text.png
├── 05_oled_show_text.jpg
├── 06_gui_show_number.png
├── 07_oled_show_number.jpg
├── 08_gui_clear.png
├── 09_gui_invert_on_off.png
├── 10_oled_invert_on.jpg
├── 11_powershell_error_tests.png
├── 12_overflow_recovery_monitor.png
└── project_b_demo_video.mp4
```

The OLED does not have a built-in screenshot feature, so OLED evidence should be captured using a phone photo, phone video, or webcam.

## Notes

- The ESP8266 and laptop must be on the same Wi-Fi network.
- Use the IP printed in serial monitor, because DHCP may assign a different address.
- The firmware supports one TCP client at a time.
- The TCP receive buffer is limited to protect ESP8266 RAM.
- `yield()` is called in `loop()` so ESP8266 Wi-Fi/lwIP background work can run.
