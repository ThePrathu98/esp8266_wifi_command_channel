#pragma once

#include <Arduino.h>

/*
Command protocol layer.

This file keeps command parsing separate from TCP transport. TCP only delivers
a completed newline-terminated command string. This layer decides what the
command means and returns the response string.
*/

String handleCommand(const String &command, bool clientConnected);
