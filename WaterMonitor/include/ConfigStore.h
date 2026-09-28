#pragma once

#include <Arduino.h>
#include <EEPROM.h>


/*
 * =========================================================
 * Configuration flags
 * =========================================================
 */

#define CONFIG_FLAG_VALID       0x01
#define CONFIG_FLAG_STATIC_IP   0x02


/*
 * =========================================================
 * Blynk provisioning error codes
 * =========================================================
 */

#define BLYNK_PROV_ERR_NONE       0
#define BLYNK_PROV_ERR_CONFIG     700
#define BLYNK_PROV_ERR_NETWORK    701
#define BLYNK_PROV_ERR_CLOUD      702
#define BLYNK_PROV_ERR_TOKEN      703
#define BLYNK_PROV_ERR_INTERNAL   704


/*
 * =========================================================
 * Configuration structure
 * =========================================================
 */

struct ConfigStore
{
    uint32_t magic;

    char version[15];

    uint8_t flags;

    char wifiSSID[34];
    char wifiPass[64];

    char cloudToken[34];
    char cloudHost[34];

    uint16_t cloudPort;

    uint32_t staticIP;
    uint32_t staticMask;
    uint32_t staticGW;
    uint32_t staticDNS;
    uint32_t staticDNS2;

    int last_error;


    void setFlag(uint8_t mask, bool value)
    {
        if (value)
        {
            flags |= mask;
        }
        else
        {
            flags &= ~mask;
        }
    }


    bool getFlag(uint8_t mask) const
    {
        return (flags & mask) == mask;
    }
};


/*
 * =========================================================
 * Global configuration
 * =========================================================
 */

ConfigStore configStore;


/*
 * =========================================================
 * Default configuration
 * =========================================================
 */

const ConfigStore configDefault =
{
    0x626C6E6B,

    BLYNK_FIRMWARE_VERSION,

    0x00,

    "",
    "",

    "invalid token",
    CONFIG_DEFAULT_SERVER,

    CONFIG_DEFAULT_PORT,

    0,
    0,
    0,
    0,
    0,

    BLYNK_PROV_ERR_NONE
};


/*
 * =========================================================
 * EEPROM
 * =========================================================
 */

#define EEPROM_CONFIG_START 0


/*
 * =========================================================
 * String copy helper
 * =========================================================
 */

template<typename T, int size>
void CopyString(
    const String& s,
    T (&arr)[size]
)
{
    s.toCharArray(
        arr,
        size
    );

    arr[size - 1] = '\0';
}


/*
 * =========================================================
 * Load configuration
 * =========================================================
 */

void config_load()
{
    memset(
        &configStore,
        0,
        sizeof(configStore)
    );


    EEPROM.get(
        EEPROM_CONFIG_START,
        configStore
    );


    /*
     * Érvénytelen EEPROM.
     */

    if (
        configStore.magic !=
        configDefault.magic
    )
    {
        DEBUG_PRINT(
            "Using default configuration."
        );

        configStore =
            configDefault;

        return;
    }


    /*
     * Stringek lezárása.
     */

    configStore.wifiSSID[
        sizeof(configStore.wifiSSID) - 1
    ] = '\0';

    configStore.wifiPass[
        sizeof(configStore.wifiPass) - 1
    ] = '\0';

    configStore.cloudToken[
        sizeof(configStore.cloudToken) - 1
    ] = '\0';

    configStore.cloudHost[
        sizeof(configStore.cloudHost) - 1
    ] = '\0';


    /*
     * Verzió lezárása.
     */

    configStore.version[
        sizeof(configStore.version) - 1
    ] = '\0';


    DEBUG_PRINT(
        "Configuration loaded from EEPROM."
    );
}


/*
 * =========================================================
 * Save configuration
 * =========================================================
 */

bool config_save()
{
    EEPROM.put(
        EEPROM_CONFIG_START,
        configStore
    );


    if (!EEPROM.commit())
    {
        DEBUG_PRINT(
            "ERROR: EEPROM commit failed!"
        );

        return false;
    }


    DEBUG_PRINT(
        "Configuration stored to flash."
    );

    return true;
}


/*
 * =========================================================
 * Initialize configuration
 * =========================================================
 */

bool config_init()
{
    if (
        !EEPROM.begin(
            sizeof(ConfigStore)
        )
    )
    {
        DEBUG_PRINT(
            "ERROR: EEPROM.begin() failed!"
        );

        configStore =
            configDefault;

        return false;
    }


    config_load();

    return true;
}


/*
 * =========================================================
 * Reset configuration
 * =========================================================
 *
 * A BlynkState itt csak deklarációként kell.
 * Az enterResetConfig() tényleges állapotváltását
 * a BlynkEdgent.h végzi.
 *
 * =========================================================
 */

void resetConfigStorage()
{
    DEBUG_PRINT(
        "Resetting configuration!"
    );


    configStore =
        configDefault;


    config_save();
}


/*
 * =========================================================
 * Last error
 * =========================================================
 */

void config_set_last_error(
    int error
)
{
    /*
     * Csak konfigurációs hiba esetén tároljuk.
     */

    if (
        !configStore.getFlag(
            CONFIG_FLAG_VALID
        )
    )
    {
        configStore =
            configDefault;

        configStore.last_error =
            error;

        BLYNK_LOG2(
            "Last error code: ",
            error
        );

        config_save();
    }
}
