# Project B Design Note: Wi-Fi TCP Command Channel

Project B adds a Wi-Fi TCP command channel to the OLED command system. The host PyQt6 GUI connects to the ESP8266 TCP server on port `5005`, sends command strings, receives `OK` / `ERR` responses, and the ESP8266 updates the SSD1306 OLED over I2C.

## Architecture

```text
PyQt6 GUI -> TCP client -> ESP8266 WiFiServer:5005
           -> TCP receive buffer -> command protocol
           -> OLED UI over I2C -> response back to GUI
```

The firmware is split into logical layers:

- `main.cpp`: application startup and non-blocking loop scheduling
- `tcp_server.cpp/.h`: TCP accept, newline receive buffer, response send, overflow protection
- `command_protocol.cpp/.h`: command parsing and OK/ERR response formatting
- `oled_ui.cpp/.h`: OLED status screen, command display hold, and INVERT feedback
- `host_gui/`: PyQt6 GUI and Python socket client

The TCP layer treats incoming data as a byte stream. It collects bytes until newline `\n`, then passes one complete command to the protocol layer. This keeps transport handling separate from command meaning.

The receive buffer is limited to 128 bytes. If a malformed client sends too much data without newline, the firmware closes the client and clears the partial command. This protects ESP8266 RAM and prevents stale data from mixing with the next connection.

OLED updates happen from normal loop-level code, not inside an ISR. The loop stays non-blocking and calls `yield()` so ESP8266 Wi-Fi/lwIP background work can run.

The OLED normally shows project name, IP address, RSSI, and TCP state. `SHOW_TEXT`, `SHOW_NUMBER`, and `CLEAR` hold the display for about 10 seconds. `INVERT` toggles OLED text/background color mode and shows line-5 feedback.

This design supports the Project B requirement that the GUI drives the same command set over Wi-Fi while keeping the target firmware readable, layered, and robust against malformed TCP input.
