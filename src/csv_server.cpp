#include "csv_server.h"

#include <ESP8266WebServer.h>
#include <LittleFS.h>
#include "logger.h"
#include "time_manager.h"


ESP8266WebServer csvServer(8080);


/******************************************************
 * IDŐTARTAM
 ******************************************************/

String formatDuration(
    uint32_t start,
    uint32_t stop
)
{
    if(stop <= start)
    {
        return "-";
    }


    uint32_t sec =
        stop - start;


    uint32_t h =
        sec / 3600;


    uint32_t m =
        (sec % 3600) / 60;


    uint32_t s =
        sec % 60;


    char buffer[32];


    snprintf(
        buffer,
        sizeof(buffer),
        "%02d:%02d:%02d",
        h,
        m,
        s
    );


    return String(buffer);
}


String formatFloat(float value)
{
    String s = String(value, 1);
    s.replace(".", ",");
    return s;
}

/******************************************************
 * HTML FEJLÉC
 ******************************************************/

String htmlHeader()
{
    String html;


    html += "<!DOCTYPE html><html>";

    html += "<head>";

    html += "<meta charset='UTF-8'>";

    html +=
    "<meta name='viewport' content='width=device-width, initial-scale=1'>";


    html +=
    "<title>Öntözési napló</title>";


    html +=
    "<style>"
    "body{font-family:Arial;margin:20px;}"
    "table{border-collapse:collapse;width:100%;}"
    "td,th{border:1px solid #999;padding:6px;text-align:center;}"
    "th{background:#ddd;}"
    "button{padding:10px;font-size:16px;}"
    "</style>";


    html += "</head><body>";


    return html;
}



/******************************************************
 * FŐOLDAL
 ******************************************************/

void handleRoot()
{
    csvServer.setContentLength(CONTENT_LENGTH_UNKNOWN);
    csvServer.sendHeader("Connection", "close");

    csvServer.send(
        200,
        "text/html",
        ""
    );

    csvServer.sendContent(htmlHeader());

    csvServer.sendContent("<h2>Öntözési napló</h2>");

    csvServer.sendContent(
        "<a href='/download'>"
        "<button>CSV letöltés</button>"
        "</a><br><br>"
    );

    csvServer.sendContent("<table>");

    csvServer.sendContent(
        "<tr>"
        "<th>Kezdés</th>"
        "<th>Vége</th>"
        "<th>Időtartam</th>"
        "<th>Kezdő liter</th>"
        "<th>Vég liter</th>"
        "<th>Felhasznált</th>"
        "<th>Forrás</th>"
        "</tr>"
    );

    uint16_t count = loggerCount();



    for(uint16_t i = 0; i < count; i++)
{
    LogEntry e = loggerGet(i);

    String row;

    row.reserve(256);

    row += "<tr>";

    row += "<td>";
    row += formatUnixTime(e.startTime);
    row += "</td>";

    row += "<td>";
    row += formatUnixTime(e.stopTime);
    row += "</td>";

    row += "<td>";
    row += formatDuration(e.startTime, e.stopTime);
    row += "</td>";

    row += "<td>";
    row += String(e.startLiter,1);
    row += "</td>";

    row += "<td>";
    row += String(e.stopLiter,1);
    row += "</td>";

    row += "<td>";
    row += String(e.usedLiter,1);
    row += "</td>";

    row += "<td>";
    row += loggerSourceName(e.source);
    row += "</td>";

    row += "</tr>";

    csvServer.sendContent(row);
}



    csvServer.sendContent("</table>");
    csvServer.sendContent("</body></html>");
    csvServer.sendContent("");   // lezárja a chunked választ
}



/******************************************************
 * CSV LETÖLTÉS
 ******************************************************/

void handleDownload()
{
    File f =
        LittleFS.open(
            LOGGER_FILE,
            "r"
        );


    if(!f)
    {
        csvServer.send(
            404,
            "text/plain",
            "Nincs naplo"
        );

        return;
    }


    csvServer.setContentLength(CONTENT_LENGTH_UNKNOWN);

csvServer.send(
    200,
    "text/csv; charset=utf-8",
    ""
);

csvServer.sendContent("\xEF\xBB\xBF");

    csvServer.sendContent(
        "Kezdes;"
        "Befejezes;"
        "Idotartam;"
        "Kezdo_liter;"
        "Veg_liter;"
        "Felhasznalt_liter;"
        "Forras\n"
    );


    while(f.available())
    {

        String line =
            f.readStringUntil('\n');


        if(line.length() == 0)
            continue;


        int p1 = line.indexOf(';');
        int p2 = line.indexOf(';', p1 + 1);
        int p3 = line.indexOf(';', p2 + 1);
        int p4 = line.indexOf(';', p3 + 1);
        int p5 = line.indexOf(';', p4 + 1);


        if(
            p1 < 0 ||
            p5 < 0
        )
        {
            continue;
        }


        LogEntry e{};


        e.startTime =
            line.substring(
                0,
                p1
            ).toInt();


        e.stopTime =
            line.substring(
                p1 + 1,
                p2
            ).toInt();


        e.startLiter =
            line.substring(
                p2 + 1,
                p3
            ).toFloat();


        e.stopLiter =
            line.substring(
                p3 + 1,
                p4
            ).toFloat();


        e.usedLiter =
            line.substring(
                p4 + 1,
                p5
            ).toFloat();


        e.source =
            line.substring(
                p5 + 1
            ).toInt();



        String lineOut;

        lineOut.reserve(128);


        lineOut += formatUnixTime(e.startTime);
        lineOut += ";";


        lineOut += formatUnixTime(e.stopTime);
        lineOut += ";";


        lineOut += formatDuration(
            e.startTime,
            e.stopTime
        );

        lineOut += ";";


        lineOut += formatFloat(
        e.startLiter
        );

        lineOut += ";";


        lineOut += formatFloat(
        e.stopLiter
        );

        lineOut += ";";


        lineOut += formatFloat(
        e.usedLiter
        );

        lineOut += ";";


        lineOut += loggerSourceName(
            e.source
        );


        lineOut += "\n";


        csvServer.sendContent(
            lineOut
        );
    }


    f.close();
    csvServer.sendContent("");   // lezárja a chunked választ
}


/******************************************************
 * INDÍTÁS
 ******************************************************/

void csvServerBegin()
{

    csvServer.on(
        "/",
        handleRoot
    );


    csvServer.on(
        "/download",
        handleDownload
    );


    csvServer.begin();


    Serial.println(
        "CSV szerver indult"
    );
}



/******************************************************
 * FUTTATÁS
 ******************************************************/

void csvServerRun()
{
    csvServer.handleClient();
}