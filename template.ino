// https://github.com/esp8266/Arduino/issues/4637
/*
 * Example that shows how to use RTC & NTP with a TZ timezone string.
 *
 * Recommended use:
 *	Bring up Serial monitor at 115200 *before* uploading
 *	Upload code and observe results
 *
 * Code is realeased to the public domain
 * 2018-11-03  Bill Perry
*/

#include <ESP8266WiFi.h>
#include "template.h"
#include <ESP8266WebServer.h>

ESP8266WebServer server(80);

void setup_stub() {

  Serial.println();

  Serial.println("Template Example");

//  while (WiFi.status() != WL_CONNECTED) {
//    delay(500);
//    Serial.print(".");
//  }

//  server.on("/time", http_gettime);
}

void loop_stub()
{
  Serial.print(".");
  delay(100);
}

String http_uptime_stub() {
    return "";
}
