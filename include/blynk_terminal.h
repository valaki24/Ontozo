#ifndef BLYNK_TERMINAL_H
#define BLYNK_TERMINAL_H

#include <Arduino.h>

typedef void (*TerminalOutputCallback)(const String& text);

void terminalInit();

void terminalSetOutputCallback(
    TerminalOutputCallback callback
);

void terminalPrint(const String& text);

void terminalCommand(const String& cmd);

void terminalRelayCommand(const String& command);

#endif