#include "time_manager.h"

#include <ESP8266WiFi.h>
#include <time.h>

static bool timeReady = false;


/******************************************************
 * IDŐZÓNA
 *
 * Magyarország:
 *
 * télen UTC+1
 * nyáron UTC+2
 *
 ******************************************************/

static int getHungaryOffsetHours(time_t utc)
{
    struct tm tmUTC;

    gmtime_r(
        &utc,
        &tmUTC
    );

    const int year =
        tmUTC.tm_year + 1900;


    /*
     * Március utolsó vasárnapja
     */

    struct tm march31 = {};

    march31.tm_year =
        year - 1900;

    march31.tm_mon = 2;

    march31.tm_mday = 31;

    march31.tm_hour = 1;


    time_t march31Time =
        mktime(&march31);


    struct tm march31UTC;

    gmtime_r(
        &march31Time,
        &march31UTC
    );


    int daysBack =
        march31UTC.tm_wday;

    if(daysBack == 0)
    {
        daysBack = 0;
    }


    time_t dstStart =
        march31Time -
        (daysBack * 86400);


    /*
     * Október utolsó vasárnapja
     */

    struct tm october31 = {};

    october31.tm_year =
        year - 1900;

    october31.tm_mon = 9;

    october31.tm_mday = 31;

    october31.tm_hour = 1;


    time_t october31Time =
        mktime(&october31);


    struct tm october31UTC;

    gmtime_r(
        &october31Time,
        &october31UTC
    );


    int daysBackOct =
        october31UTC.tm_wday;


    time_t dstEnd =
        october31Time -
        (daysBackOct * 86400);


    /*
     * Magyarországon:
     *
     * DST kezdete:
     * utolsó vasárnap március
     * 01:00 UTC
     *
     * DST vége:
     * utolsó vasárnap október
     * 01:00 UTC
     */

    if(
        utc >= dstStart &&
        utc < dstEnd
    )
    {
        return 2;
    }


    return 1;
}


/******************************************************
 * INDÍTÁS
 ******************************************************/

void timeManagerBegin()
{
    /*
     * Az NTP UTC időt szolgáltat.
     *
     * NEM állítunk timezone offsetet
     * a configTime() segítségével.
     */

    configTime(
        0,
        0,
        "pool.ntp.org",
        "time.nist.gov"
    );


    for(int i = 0; i < 20; i++)
    {
        time_t now =
            time(nullptr);


        if(now > 1700000000)
        {
            timeReady = true;
            break;
        }


        delay(500);
    }
}


/******************************************************
 * ÉRVÉNYES?
 ******************************************************/

bool timeIsValid()
{
    return timeReady;
}


/******************************************************
 * UNIX TIMESTAMP
 *
 * Mindig UTC!
 ******************************************************/

uint32_t getUnixTime()
{
    time_t now =
        time(nullptr);


    if(now < 1700000000)
    {
        return 0;
    }


    return (uint32_t)now;
}


/******************************************************
 * UNIX -> MAGYAR HELYI IDŐ
 ******************************************************/

String formatUnixTime(uint32_t ts)
{
    if(ts < 1700000000)
    {
        return "-";
    }


    time_t utc =
        (time_t)ts;


    /*
     * UTC -> magyar idő
     */

    int offsetHours =
        getHungaryOffsetHours(utc);


    time_t local =
        utc +
        offsetHours * 3600;


    struct tm tmLocal;

    gmtime_r(
        &local,
        &tmLocal
    );


    char buffer[64];


    snprintf(
        buffer,
        sizeof(buffer),
        "%04d-%02d-%02d %02d:%02d:%02d",
        tmLocal.tm_year + 1900,
        tmLocal.tm_mon + 1,
        tmLocal.tm_mday,
        tmLocal.tm_hour,
        tmLocal.tm_min,
        tmLocal.tm_sec
    );


    return String(buffer);
}


/******************************************************
 * AKTUÁLIS MAGYAR IDŐ
 ******************************************************/

String getTimeString()
{
    uint32_t ts =
        getUnixTime();


    if(ts == 0)
    {
        return "--:--";
    }


    return formatUnixTime(ts);
}


/******************************************************
 * NTP FRISSÍTÉS
 ******************************************************/

void timeManagerUpdate()
{
    if(timeReady)
    {
        return;
    }


    time_t now =
        time(nullptr);


    if(now > 1700000000)
    {
        timeReady = true;
    }
}