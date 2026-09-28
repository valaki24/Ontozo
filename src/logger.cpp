#include "logger.h"

#include <LittleFS.h>
#include "time_manager.h"


/******************************************************
 * BEÁLLÍTÁSOK
 ******************************************************/

static const uint32_t LOG_KEEP_DAYS = 7;
static const uint32_t LOG_KEEP_SECONDS = 7UL * 24UL * 60UL * 60UL;


/******************************************************
 * AKTUÁLIS ZÓNA NAPLÓ
 ******************************************************/

static bool zoneRunning = false;

static uint8_t activeZone = 0;

static uint32_t zoneStartTime = 0;

static float zoneStartLiter = 0.0f;


/******************************************************
 * POND NAPLÓ
 ******************************************************/

static bool pondRunning = false;

static uint32_t pondStartTime = 0;

static float pondStartLiter = 0.0f;


/******************************************************
 * HŐSZIVATTYÚ NAPLÓ
 ******************************************************/

static bool heatpumpRunning = false;

static uint32_t heatpumpStartTime = 0;

static float heatpumpStartLiter = 0.0f;


/******************************************************
 * UTOLSÓ ELKÉSZÜLT BEJEGYZÉS
 ******************************************************/

static bool newEntryAvailable = false;

static LogEntry newEntry;


/******************************************************
 * AKTÍV FORRÁS
 ******************************************************/

static LoggerSource activeSource = SOURCE_NONE;


/******************************************************
 * ZÓNA → LOGGER SOURCE
 ******************************************************/

static LoggerSource zoneToSource(
    uint8_t zone
)
{
    switch(zone)
    {
        case 1:
            return SOURCE_ZONE1;

        case 2:
            return SOURCE_ZONE2;

        case 3:
            return SOURCE_ZONE3;

        case 4:
            return SOURCE_ZONE4;

        case 5:
            return SOURCE_ZONE5;

        case 6:
            return SOURCE_ZONE6;

        case 7:
            return SOURCE_ZONE7;

        default:
            return SOURCE_NONE;
    }
}


/******************************************************
 * FORRÁS NEVE
 ******************************************************/

const char* loggerSourceName(
    uint8_t source
)
{
    switch(source)
    {
        case SOURCE_ZONE1:
            return "Zóna1";

        case SOURCE_ZONE2:
            return "Zóna2";

        case SOURCE_ZONE3:
            return "Zóna3";

        case SOURCE_ZONE4:
            return "Zóna4";

        case SOURCE_ZONE5:
            return "Zóna5";

        case SOURCE_ZONE6:
            return "Zóna6";

        case SOURCE_ZONE7:
            return "Zóna7";

        case SOURCE_POND:
            return "Tó töltés";

        case SOURCE_HEATPUMP:
            return "HŐszivattyú";

        default:
            return "NONE";
    }
}


/******************************************************
 * RÉGI NAPLÓK TÖRLÉSE
 ******************************************************/

static void cleanupOldLogs()
{
    if(!LittleFS.exists(LOGGER_FILE))
        return;


    File f = LittleFS.open(
        LOGGER_FILE,
        "r"
    );

    if(!f)
        return;


    uint32_t now = time(nullptr);


    if(now < 100000)
    {
        f.close();
        return;
    }


    String lines[128];

    uint16_t lineCount = 0;


    while(f.available())
    {
        String line =
            f.readStringUntil('\n');

        line.trim();


        if(line.length() == 0)
            continue;


        int separator =
            line.indexOf(';');


        if(separator < 0)
            continue;


        uint32_t timestamp =
            line.substring(
                0,
                separator
            ).toInt();


        if(timestamp == 0)
            continue;


        if(
            now >= timestamp &&
            now - timestamp > LOG_KEEP_SECONDS
        )
        {
            continue;
        }


        if(lineCount < 128)
        {
            lines[lineCount++] = line;
        }
    }


    f.close();


    File out = LittleFS.open(
        LOGGER_FILE,
        "w"
    );

    if(!out)
        return;


    for(uint16_t i = 0; i < lineCount; i++)
    {
        out.println(lines[i]);
    }


    out.close();
}


/******************************************************
 * BEJEGYZÉS MENTÉSE
 ******************************************************/

static bool saveEntry(
    const LogEntry& entry
)
{
    File f = LittleFS.open(
        LOGGER_FILE,
        "a"
    );


    if(!f)
    {
        Serial.println(
            "LOGGER: fajl megnyitasi hiba"
        );

        return false;
    }


    f.print(entry.startTime);
    f.print(";");

    f.print(entry.stopTime);
    f.print(";");

    f.print(entry.startLiter, 1);
    f.print(";");

    f.print(entry.stopLiter, 1);
    f.print(";");

    f.print(entry.usedLiter, 1);
    f.print(";");

    f.println(
        (uint8_t)entry.source
    );


    f.close();


    newEntry = entry;

    newEntryAvailable = true;


    return true;
}


/******************************************************
 * LOGGER INDÍTÁS
 ******************************************************/

void loggerBegin()
{
    if(!LittleFS.begin())
    {
        Serial.println(
            "LOGGER: LittleFS hiba"
        );

        return;
    }


    cleanupOldLogs();


    Serial.println(
        "LOGGER: elindult"
    );
}


/******************************************************
 * ÖNTÖZÉSI ZÓNA INDÍTÁSA
 ******************************************************/

void loggerStartZone(
    uint8_t zone,
    float startLiter
)
{
    if(zone < 1 || zone > 7)
        return;


    /*
     * Ha valamiért már fut egy zóna,
     * nem írjuk felül.
     */
    if(zoneRunning)
    {
        Serial.println(
            "LOGGER: zóna már fut"
        );

        return;
    }


    LoggerSource source =
        zoneToSource(zone);


    if(source == SOURCE_NONE)
        return;


    zoneRunning = true;

    activeZone = zone;

    zoneStartTime = time(nullptr);

    zoneStartLiter = startLiter;

    activeSource = source;


    Serial.print(
        "LOGGER: ZONE"
    );

    Serial.print(zone);

    Serial.print(
        " START, liter="
    );

    Serial.println(
        startLiter,
        1
    );
}


/******************************************************
 * ÖNTÖZÉSI ZÓNA LEZÁRÁSA
 ******************************************************/

void loggerFinishZone(
    float stopLiter
)
{
    if(!zoneRunning)
        return;


    uint32_t stopTime =
        time(nullptr);


    float usedLiter =
        stopLiter - zoneStartLiter;


    /*
     * Negatív érték ne kerüljön
     * a naplóba.
     */
    if(usedLiter < 0.0f)
        usedLiter = 0.0f;


    LogEntry entry{};


    entry.startTime =
        zoneStartTime;

    entry.stopTime =
        stopTime;

    entry.startLiter =
        zoneStartLiter;

    entry.stopLiter =
        stopLiter;

    entry.usedLiter =
        usedLiter;

    entry.source =
        zoneToSource(activeZone);


    saveEntry(entry);


    Serial.print(
        "LOGGER: ZONE"
    );

    Serial.print(activeZone);

    Serial.print(
        " STOP, felhasznalt="
    );

    Serial.print(
        usedLiter,
        1
    );

    Serial.println(
        " L"
    );


    zoneRunning = false;

    activeZone = 0;

    zoneStartTime = 0;

    zoneStartLiter = 0.0f;

    activeSource = SOURCE_NONE;


    cleanupOldLogs();
}


/******************************************************
 * ÖNTÖZÉS LEÁLLÍTÁSA
 ******************************************************/

void loggerStop(
    float stopLiter
)
{
    loggerFinishZone(
        stopLiter
    );
}


/******************************************************
 * POND INDÍTÁSA
 ******************************************************/

void loggerStartPond(
    float startLiter
)
{
    if(pondRunning)
        return;


    pondRunning = true;

    pondStartTime = time(nullptr);

    pondStartLiter = startLiter;

    activeSource = SOURCE_POND;


    Serial.print(
        "LOGGER: POND START, liter="
    );

    Serial.println(
        startLiter,
        1
    );
}


/******************************************************
 * POND LEZÁRÁSA
 ******************************************************/

void loggerFinishPond(
    float stopLiter
)
{
    if(!pondRunning)
        return;


    uint32_t stopTime =
        time(nullptr);


    float usedLiter =
        stopLiter - pondStartLiter;


    if(usedLiter < 0.0f)
        usedLiter = 0.0f;


    LogEntry entry{};


    entry.startTime =
        pondStartTime;

    entry.stopTime =
        stopTime;

    entry.startLiter =
        pondStartLiter;

    entry.stopLiter =
        stopLiter;

    entry.usedLiter =
        usedLiter;

    entry.source =
        SOURCE_POND;


    saveEntry(entry);


    Serial.print(
        "LOGGER: POND STOP, felhasznalt="
    );

    Serial.print(
        usedLiter,
        1
    );

    Serial.println(
        " L"
    );


    pondRunning = false;

    pondStartTime = 0;

    pondStartLiter = 0.0f;


    if(activeSource == SOURCE_POND)
        activeSource = SOURCE_NONE;


    cleanupOldLogs();
}


/******************************************************
 * HŐSZIVATTYÚ INDÍTÁSA
 ******************************************************/

void loggerStartHeatpump(
    float startLiter
)
{
    if(heatpumpRunning)
        return;


    heatpumpRunning = true;

    heatpumpStartTime = time(nullptr);

    heatpumpStartLiter = startLiter;


    /*
     * A hőszivattyú külön esemény.
     * Nem módosítja az öntözési zóna állapotát.
     */
    activeSource = SOURCE_HEATPUMP;


    Serial.print(
        "LOGGER: HEATPUMP START, liter="
    );

    Serial.println(
        startLiter,
        1
    );
}


/******************************************************
 * HŐSZIVATTYÚ LEZÁRÁSA
 ******************************************************/

void loggerFinishHeatpump(
    float stopLiter
)
{
    if(!heatpumpRunning)
        return;


    uint32_t stopTime =
        time(nullptr);


    float usedLiter =
        stopLiter - heatpumpStartLiter;


    if(usedLiter < 0.0f)
        usedLiter = 0.0f;


    LogEntry entry{};


    entry.startTime =
        heatpumpStartTime;

    entry.stopTime =
        stopTime;

    entry.startLiter =
        heatpumpStartLiter;

    entry.stopLiter =
        stopLiter;

    entry.usedLiter =
        usedLiter;

    entry.source =
        SOURCE_HEATPUMP;


    saveEntry(entry);


    Serial.print(
        "LOGGER: HEATPUMP STOP, felhasznalt="
    );

    Serial.print(
        usedLiter,
        1
    );

    Serial.println(
        " L"
    );


    heatpumpRunning = false;

    heatpumpStartTime = 0;

    heatpumpStartLiter = 0.0f;


    if(activeSource == SOURCE_HEATPUMP)
        activeSource = SOURCE_NONE;


    cleanupOldLogs();
}


/******************************************************
 * ÚJ BEJEGYZÉS VAN?
 ******************************************************/

bool loggerHasNewEntry()
{
    return newEntryAvailable;
}


/******************************************************
 * ÚJ BEJEGYZÉS LEKÉRÉSE
 ******************************************************/

LogEntry loggerGetEntry()
{
    return newEntry;
}


/******************************************************
 * ÚJ BEJEGYZÉS JELZÉS TÖRLÉSE
 ******************************************************/

void loggerClearEntry()
{
    newEntryAvailable = false;
}


/******************************************************
 * BEJEGYZÉSEK SZÁMA
 ******************************************************/

uint16_t loggerCount()
{
    if(!LittleFS.exists(LOGGER_FILE))
        return 0;


    File f = LittleFS.open(
        LOGGER_FILE,
        "r"
    );


    if(!f)
        return 0;


    uint16_t count = 0;


    while(f.available())
    {
        String line =
            f.readStringUntil('\n');


        line.trim();


        if(line.length() > 0)
            count++;
    }


    f.close();


    return count;
}


/******************************************************
 * BEJEGYZÉS LEKÉRÉSE INDEX ALAPJÁN
 ******************************************************/

LogEntry loggerGet(
    uint16_t index
)
{
    LogEntry empty{};


    if(!LittleFS.exists(LOGGER_FILE))
        return empty;


    File f = LittleFS.open(
        LOGGER_FILE,
        "r"
    );


    if(!f)
        return empty;


    uint16_t current = 0;


    while(f.available())
    {
        String line =
            f.readStringUntil('\n');


        line.trim();


        if(line.length() == 0)
            continue;


        if(current != index)
        {
            current++;
            continue;
        }


        int p1 =
            line.indexOf(';');


        int p2 =
            line.indexOf(
                ';',
                p1 + 1
            );


        int p3 =
            line.indexOf(
                ';',
                p2 + 1
            );


        int p4 =
            line.indexOf(
                ';',
                p3 + 1
            );


        int p5 =
            line.indexOf(
                ';',
                p4 + 1
            );


        if(
            p1 < 0 ||
            p2 < 0 ||
            p3 < 0 ||
            p4 < 0 ||
            p5 < 0
        )
        {
            f.close();
            return empty;
        }


        LogEntry entry{};


        entry.startTime =
            line.substring(
                0,
                p1
            ).toInt();


        entry.stopTime =
            line.substring(
                p1 + 1,
                p2
            ).toInt();


        entry.startLiter =
            line.substring(
                p2 + 1,
                p3
            ).toFloat();


        entry.stopLiter =
            line.substring(
                p3 + 1,
                p4
            ).toFloat();


        entry.usedLiter =
            line.substring(
                p4 + 1,
                p5
            ).toFloat();


        entry.source =
            (LoggerSource)line.substring(
                p5 + 1
            ).toInt();


        f.close();


        return entry;
    }


    f.close();


    return empty;
}


/******************************************************
 * AKTÍV FORRÁS
 ******************************************************/

uint8_t loggerGetActiveSource()
{
    return (uint8_t)activeSource;
}