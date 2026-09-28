#include "blynk_terminal.h"

#include <Arduino.h>
#include <ESP8266WiFi.h>

#include "time_manager.h"
#include "logger.h"


static TerminalOutputCallback outputCallback = nullptr;


/******************************************************
 * OUTPUT CALLBACK BEÁLLÍTÁSA
 ******************************************************/

void terminalSetOutputCallback(
    TerminalOutputCallback callback
)
{
    outputCallback = callback;
}


/******************************************************
 * TERMINAL KIÍRÁS
 ******************************************************/

void terminalPrint(const String& text)
{
    // Serial monitor
    Serial.println(text);

    // Blynk Terminal
    if (outputCallback != nullptr)
    {
        outputCallback(text);
    }
}


/******************************************************
 * RELÉ PARANCS
 *
 * Példák:
 *
 * relay 1
 * relay 1+2+5
 * relay 1+3+7+8
 * relay off
 *
 ******************************************************/

static void handleRelayCommand(
    const String& command
)
{
    terminalRelayCommand(command);
}


/******************************************************
 * PARANCSFELDOLGOZÁS
 ******************************************************/

void terminalCommand(const String& cmd)
{
    String command = cmd;

    command.trim();

    if(command.length() == 0)
        return;


    /**************************************************
     * HELP
     **************************************************/

    if(
        command == "help" ||
        command == "?"
    )
    {
        terminalPrint(
            "Parancsok:\n"
            "help\n"
            "status\n"
            "time\n"
            "wifi\n"
            "devinfo\n"
            "log\n"
            "relay 1-8\n"
            "relay 1+2+5\n"
            "relay off\n"
            "reboot"
        );

        return;
    }


    /**************************************************
     * RELÉ
     **************************************************/

    if(
        command.startsWith("relay ")
    )
    {
        String relayCommand =
            command.substring(6);

        relayCommand.trim();

        if(relayCommand.length() == 0)
        {
            terminalPrint(
                "Hasznalat: relay 1-8"
            );

            return;
        }

        handleRelayCommand(
            relayCommand
        );

        return;
    }


    /**************************************************
     * TIME
     **************************************************/

    if(command == "time")
    {
        terminalPrint(
            "Time: " +
            getTimeString()
        );

        terminalPrint(
            "Timestamp: " +
            String(
                getUnixTime()
            )
        );

        terminalPrint(
            "Valid: " +
            String(
                timeIsValid()
                ? "YES"
                : "NO"
            )
        );

        return;
    }


    /**************************************************
     * WIFI
     **************************************************/

    if(command == "wifi")
    {
        terminalPrint(
            "SSID: " +
            WiFi.SSID()
        );

        terminalPrint(
            "IP: " +
            WiFi.localIP().toString()
        );

        terminalPrint(
            "RSSI: " +
            String(
                WiFi.RSSI()
            ) +
            " dBm"
        );

        terminalPrint(
            "MAC: " +
            WiFi.macAddress()
        );

        terminalPrint(
            "Status: " +
            String(
                WiFi.status() == WL_CONNECTED
                ? "CONNECTED"
                : "DISCONNECTED"
            )
        );

        return;
    }


    /**************************************************
     * DEVICE INFO
     **************************************************/

    if(command == "devinfo")
    {

#ifdef BLYNK_TEMPLATE_NAME

        terminalPrint(
            "Template: " +
            String(
                BLYNK_TEMPLATE_NAME
            )
        );

#endif


#ifdef BLYNK_TEMPLATE_ID

        terminalPrint(
            "Template ID: " +
            String(
                BLYNK_TEMPLATE_ID
            )
        );

#endif


#ifdef BLYNK_FIRMWARE_VERSION

        terminalPrint(
            "Firmware: " +
            String(
                BLYNK_FIRMWARE_VERSION
            )
        );

#endif


        terminalPrint(
            "Chip: ESP8266"
        );


        terminalPrint(
            "CPU: " +
            String(
                ESP.getCpuFreqMHz()
            ) +
            " MHz"
        );


        terminalPrint(
            "Flash: " +
            String(
                ESP.getFlashChipSize() /
                1024
            ) +
            " KB"
        );

        return;
    }


    /**************************************************
     * STATUS
     **************************************************/

    if(command == "status")
    {
        terminalPrint(
            "===== STATUS ====="
        );


        terminalPrint(
            "Uptime: " +
            String(
                millis() / 1000
            ) +
            " s"
        );


        terminalPrint(
            "Heap: " +
            String(
                ESP.getFreeHeap()
            ) +
            " bytes"
        );


        terminalPrint(
            "Max block: " +
            String(
                ESP.getMaxFreeBlockSize()
            ) +
            " bytes"
        );


        terminalPrint(
            "WiFi RSSI: " +
            String(
                WiFi.RSSI()
            ) +
            " dBm"
        );


        terminalPrint(
            "IP: " +
            WiFi.localIP().toString()
        );


        terminalPrint(
            "Time: " +
            getTimeString()
        );


        terminalPrint(
            "Timestamp: " +
            String(
                getUnixTime()
            )
        );


        terminalPrint(
            "Time valid: " +
            String(
                timeIsValid()
                ? "YES"
                : "NO"
            )
        );


        terminalPrint(
            "Log entries: " +
            String(
                loggerCount()
            )
        );


        terminalPrint(
            "Active source: " +
            String(
                loggerGetActiveSource()
            )
        );


        terminalPrint(
            "=================="
        );

        return;
    }


    /**************************************************
     * LOG
     **************************************************/

    if(command == "log")
    {
        terminalPrint(
            "Log entries: " +
            String(
                loggerCount()
            )
        );


        terminalPrint(
            "Active source: " +
            String(
                loggerGetActiveSource()
            )
        );

        return;
    }


    /**************************************************
     * REBOOT
     **************************************************/

    if(command == "reboot")
    {
        terminalPrint(
            "Rebooting..."
        );

        delay(200);

        ESP.restart();

        return;
    }


    /**************************************************
     * ISMERETLEN PARANCS
     **************************************************/

    terminalPrint(
        "Unknown command: " +
        command
    );

    terminalPrint(
        "Type 'help'"
    );
}


/******************************************************
 * INIT
 ******************************************************/

void terminalInit()
{
    // Jelenleg nincs külön inicializáció.
}