#pragma once

#include <Arduino.h>

/*
TCP transport layer.

This file owns:
- WiFiServer on port 5005
- one active WiFiClient
- TCP receive buffer
- newline command framing
- malformed/too-long input protection

It does not own command behavior. Completed commands are passed to the
command protocol layer.
*/

static const uint16_t TCP_SERVER_PORT = 5005;

void tcpServerBegin();
void tcpServerPoll();
bool tcpServerHasConnectedClient();
