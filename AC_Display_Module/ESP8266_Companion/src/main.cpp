#include "main.hpp"

ESP8266WiFiMulti wifiMulti;
AsyncWebServer server( 80 );
bool connectioWasAlive = false;

elm_comm elm;

Scheduler ts;
Task initElm( 3000 * TASK_MILLISECOND, -1, &initElmCb, &ts, true );

espComm ard( Serial );

void setup( void ) {
    delay(2000);
    ard.init();
    delay(2000);
    logln("Starting ESP, delayed by 4s");

    // Create a file `include/secrets.h` with String wifi_aps[n][2] for n access points, with each
    // AP={"SSID", "PASSWORD"}
    for ( auto ap : wifi_aps ) {
        wifiMulti.addAP( ap[0].c_str(), ap[1].c_str() );
    }

    server.on( "/", HTTP_GET, []( AsyncWebServerRequest *request ) {
        request->send( 200, "text/html", "Hi! This is ElegantOTA AsyncDemo. <a href=\"/reset\">RESET<\\a>" );
    } );
    server.on( "/reset", HTTP_GET, []( AsyncWebServerRequest *request ) {
        ESP.restart();
        request->send( 200, "text/plain", "Resetting ESP!" );
    } );

    ElegantOTA.begin( &server ); // Start ElegantOTA
    server.begin();

    elm.init();
}

bool monitorWiFi() {
    if ( wifiMulti.run() != WL_CONNECTED ) {
        if ( connectioWasAlive == true ) {
            connectioWasAlive = false;
            logln( "Looking for WiFi " );
        }
        return false;
    } else if ( connectioWasAlive == false ) {
        connectioWasAlive = true;
        logln( "Wifi connected to %s with IP %s", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str() );
    }
    return true;
}

void loop( void ) {
    if ( monitorWiFi() ) {
        ElegantOTA.loop();
    }

    elm.tick();
    ard.tick();

    delay( 5 );
}

void initElmCb() {
    logln("Trying to initialize elm");
    if ( !elm.isInitialized() ) {
        elm.initializeElm();
    }
    else {
        initElm.disable();
    }
}

void logger_log_formatted_string( const char *format, ... ) {
    va_list args;
    va_start( args, format );

    char str[256];
    vsprintf( str, format, args );

    ard.debugData( str );
    va_end( args );
}
