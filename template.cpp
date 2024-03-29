/*
 * Boiler plate to include in all of Tim Connors' esp8266 projects
 */

#include <Arduino.h>
// FIXME: the preprocessor doesn't cope with these, so manually enable/disable each include per what you're building
#if defined(ESP8266)
#include <WiFiClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include "AsyncPing.h"
#elif ESP32
// #include "esp8266-compat.h"
// #include <Update.h>
// #include <WiFi.h>
// #include <ESPmDNS.h>
// #include <WiFiUdp.h>
// #include <ArduinoOTA.h>
// #include <FS.h>
// #include <SPI.h>
// #include <WebServer.h>
// #include <analogWrite.h>
// #include <RTClib.h>
#endif
#include <Arduino_JSON.h>
#include <ESP_EEPROM.h>
#include "Syslog.h"
#include "template.h"

String debug = "";
String syslog_buffer = "";

String TEMPLATE_VERSION = "$Revision: 1.3 $";
extern String CODE_VERSION;

#if defined(_SSID)
String ssid[2]           = { _SSID, _SSID2};
String password          = _PWD;
String ota_password      = _OTA_PWD;
String SYSLOG_SERVER     = _SYSLOG;

String hostname          = _HOSTNAME;
#else
String ssid[2]           = { "*************", "*************"};  //  your network SSID (name) and second fallback network SSID (name)
String password          = "********";       // your network password
String ota_password      = "********";
String SYSLOG_SERVER     = "********";       // your syslog server name/address

String hostname          = "********";
#endif

#define SYSLOG_PORT 514

int wifi_index=0;
int triggered_wifi_failover=0;
int wifi_failover_count=0;
const int wifi_failover_threshold=2;
unsigned long last_ping_time;
unsigned long last_millis=millis();
int uptime_loop=0;
#if defined(ESP8266)
extern ESP8266WebServer server;
AsyncPing Ping;
#elif ESP32
extern WebServer server;
#endif

bool wifi_started=false;
bool http_started=false;

// A UDP instance to let us send and receive packets over UDP
WiFiUDP udpClient;

Syslog syslog(udpClient, SYSLOG_SERVER.c_str(), SYSLOG_PORT, hostname.c_str(), hostname.c_str(), LOG_KERN, SYSLOG_PROTO_BSD);

int ONBOARD_LED_PIN = LED_BUILTIN;
int led_range = 100;

#if ESP32
// RTC_DATA_ATTR int bootCount = 0;
#endif

extern void setup_stub();
extern void loop_stub();
extern String http_uptime_stub();

void logQuery() {
    String args="";
    for (uint8_t i=0; i < server.args(); i++){
        int this_loop_effecting_change=1;
        if (args != "") {
            args += " ";
        }
        args += server.argName(i) + "=" + server.arg(i);
    }
    syslog.logf("query: %s %s?%s", server.client().remoteIP().toString().c_str(), server.uri().c_str(), args.c_str());
}

/********************************************************
/*  Debug Print                                         *
/********************************************************/
void dbg_printf ( const char *format, ... )
{

    static char sbuf[1400];                                                     // For debug lines
    va_list varArgs;                                                            // For variable number of params

    va_start ( varArgs, format );                                               // Prepare parameters
    vsnprintf ( sbuf, sizeof ( sbuf ), format, varArgs );                       // Format the message
    va_end ( varArgs );                                                         // End of using parameters

    Serial.print ( sbuf );

}



// https://diarmuid.ie/blog/pwm-exponential-led-fading-on-arduino-or-other-platforms/
float R;
unsigned int current_led_brightness;

void ledBright(unsigned int val, int led) {
//    debug += "ledBright(" + String(val) + ")<br>";
//    analogWrite(led, led_range-val);
//    debug += "R=" + String(R) + "<br>";
    int brightness = pow (2, (val / R)) - 1;
    // Set the LED output to the calculated brightness
//    debug += "AnalogWrite(" + String(led_range-brightness) + ")<br><br>";
    analogWrite(led, led_range - brightness
#if ESP32
                , led_range
#endif
        );
    current_led_brightness=val;
}
void ledRamp(int start, int finish, unsigned int duration, unsigned int steps) {
    for (int i=0;i<steps;i++)
    {
        ledBright(start + (finish-start)*int(i)/int(steps));
//        delay(duration/steps);
    }
}
void ledErrorBlink(unsigned int repeat, unsigned int d1, unsigned int d2) {
    for (int i=0;i<repeat;i++)
    {
        ledBright(led_range);
//        delay(d1);
        ledBright(0);
//        delay(d2);
    }
}

/*
Method to print the reason by which ESP32
has been awaken from sleep
*/
// void print_wakeup_reason(){
//   esp_sleep_wakeup_cause_t wakeup_reason;

//   wakeup_reason = esp_sleep_get_wakeup_cause();

//   switch(wakeup_reason)
//   {
//     case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
//     case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
//     case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
//     case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
//     case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
//     default : Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;
//   }
// }

void reboot(void) {
//    delay(500);
    // https://github.com/esp8266/Arduino/issues/1017   GPIO15 (D8) must be low, GPIO0 (D3), GPIO2 (D4) high.
    // https://wiki.wemos.cc/products:d1:d1_mini_pro ie, D8=0, D3,D4=1
    // http://arduino-esp8266.readthedocs.io/en/latest/boards.html#boot-messages-and-modes https://www.reddit.com/r/esp8266/comments/7398fn/resetting_wemos_d1_mini/
#if defined(ESP8266)
    digitalWrite(D8, LOW);
//                pinMode(D8, INPUT_PULLDOWN);
    digitalWrite(D3, HIGH);
//    pinMode(D3, INPUT_PULLUP);
    digitalWrite(D4, HIGH);
//    pinMode(D4, INPUT_PULLUP);
#endif
    ESP.restart(); //FIXME: pullup resistor to GPIO0: https://github.com/esp8266/Arduino/issues/1017
}

void http_reboot() {
//    server.send(200, "text/plain", "Rebooting...\n");
    server.sendHeader("Location", "/",true);   //Redirect to our html web page
    server.send(302, "text/plain","Rebooting...\n");
    delay(100);
    reboot();
}

void http_uptime() {
    unsigned long uptime = (millis()+uptime_loop*(1ll+UINT_MAX))/1000;

    unsigned long seconds = uptime;
    unsigned long minutes = seconds/60;
    unsigned long hours = minutes/60;
    unsigned long days = hours/24;

    seconds = seconds % 60;
    minutes = minutes % 60;
    hours = hours % 24;

    // https://www.reddit.com/r/esp8266/wiki/faq
    String content = "Uptime: " +  String(uptime) + " (" + String(days) + " days " + String(hours) + ":" + String(minutes) + ":" + String(seconds) + ")\n";
    content += "Memory: " + String(ESP.getFreeHeap()) + "\n";
#if defined(ESP8266)
    content += "Heap Fragmentation: " + String(ESP.getHeapFragmentation()) + "\n";
    content += "Max Free Block: " + String(ESP.getMaxFreeBlockSize()) + "\n";
#endif
    content += "EEprom % used: " + String(EEPROM.percentUsed())+"%\n";

    content += "Hostname: " + hostname + "\n";
    content += "Compiled: " + String(__DATE__) + " " + String(__TIME__) + "\n";

    content += "Template version: " + TEMPLATE_VERSION + "\n";
    content += "Code version: " + CODE_VERSION + "\n";
    content += "wifi: " + String(wifi_index) + " " + ssid[wifi_index] + "\n";

    String message=http_uptime_stub();

    if (message != "NA") {
        content += message;
    }

    content += debug; debug="";
    server.send(200, "text/plain", content);
}

void http_wifi() {
    String content = "wifi: " + String(wifi_index) + " " + ssid[wifi_index] + "\n";
    server.send(200, "text/plain", content);
}

/* void webserver() { */
/*     digitalWrite(relayPin, HIGH); // turn on relay with voltage HIGH */
/*     digitalWrite(led, !digitalRead(led)); */
/*     distanceData(); */
/*     float rate = distance*0.02; */
/*     String content = "<html> <head> <meta http-equiv='refresh' content='"; */
/*     content += rate; */
/*     content += "'> </head><body>"; */
/*     content += "<center><h2>The distance is: "; */
/*     content += distance; */
/*     content += " cm </h2></center> </body></html>"; */
/*     server.send(200, "text/html", content); */
/*     delay(100); */
/*     digitalWrite(relayPin, LOW);  // turn off relay with voltage LOW */
/* } */

void handleNotFound(){
    digitalWrite(ONBOARD_LED_PIN, 1);
    String message = "File Not Found\n\n";
    message += "URI: ";
    message += server.uri();
    message += "\nMethod: ";
    message += (server.method() == HTTP_GET)?"GET":"POST";
    message += "\nArguments: ";
    message += server.args();
    message += "\n";
    for (uint8_t i=0; i<server.args(); i++){
        message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
    }
    server.send(404, "text/plain", message);
    digitalWrite(ONBOARD_LED_PIN, 0);
}

void start_wifi() {
    WiFi.begin(ssid[0].c_str(), password.c_str());
}

void trigger_wifi_failover() {
    Serial.println("Trigger wifi failover");
    triggered_wifi_failover=1;
    wifi_failover_count=0;
    if (++wifi_index > 1) {
        wifi_index=0;
    }
    syslog_buffer="Failing over to wifi index " + String(wifi_index) + " " + ssid[wifi_index];
}

void increment_wifi_failover() {
    if (++wifi_failover_count > wifi_failover_threshold) {
        trigger_wifi_failover();
    }
}

void decrement_wifi_failover() {
    if (wifi_failover_count > 0) {
        wifi_failover_count--;
    }
}

void http_trigger_wifi_failover() {
    trigger_wifi_failover();
    String content = "Failing over to wifi index " + String(wifi_index) + " " + ssid[wifi_index] + "\n";
    server.send(200, "text/plain", content);
}

void execute_wifi_failover() {
    Serial.println("Execute wifi failover");
    triggered_wifi_failover=0;
    WiFi.begin(ssid[wifi_index].c_str(), password.c_str());
    syslog_buffer="Failed over to wifi index " + String(wifi_index) + " " + ssid[wifi_index];
}

// handles input GET CGI style args, or input PUT/POST json
int getArgValue(String name)
{
    JSONVar input_json;
    String postBody = server.arg("plain");

    input_json = JSON.parse(postBody);
    if (JSON.typeof(input_json) != "undefined") {
        if (input_json.hasOwnProperty(name)) {
            syslog.log(LOG_INFO, "put: " + name + " (" + postBody + ")");

            return input_json[name];
        }
//        syslog.log(LOG_INFO, "put no result: " + name + " (" + postBody + ")");
        return -1;
    }

    for (uint8_t i = 0; i < server.args(); i++)
        if(server.argName(i) == name)
            return server.arg(i).toInt();
    return -1;
}

// handles input GET CGI style args, or input PUT/POST json
String getArgValueStr(String name)
{
    JSONVar input_json;
    String postBody = server.arg("plain");

    input_json = JSON.parse(postBody);
    if (JSON.typeof(input_json) != "undefined") {
        if (input_json.hasOwnProperty(name)) {
            syslog.log(LOG_INFO, "put: " + name + " (" + postBody + ")");

            const char *t=input_json[name];
            return t;
        }
//        syslog.log(LOG_INFO, "put no result: " + name + " (" + postBody + ")");
        return "";
    }

    for (uint8_t i = 0; i < server.args(); i++)
        if(server.argName(i) == name)
            return server.arg(i);
    return "";
}

/********************************************************
/*  WiFi Connection Status                              *
/********************************************************/
String connectionStatus ( int which )
{
    switch ( which )
    {
      case WL_CONNECTED:
          return "Connected";
          break;

      case WL_NO_SSID_AVAIL:
          return "Network not availible";
          break;

      case WL_CONNECT_FAILED:
          return "Wrong password";
          break;

      case WL_IDLE_STATUS:
          return "Idle status";
          break;

      case WL_DISCONNECTED:
          return "Disconnected";
          break;

      default:
          return "Unknown";
          break;
    }
}

/********************************************************
/*  Handle WiFi events                                  *
/********************************************************/
void eventWiFi(WiFiEvent_t event) {

    switch(event) {
      case WIFI_EVENT_STAMODE_CONNECTED:
          dbg_printf("[WiFi] %d, Connected\n", event);
          break;

      case WIFI_EVENT_STAMODE_DISCONNECTED:
          dbg_printf("[WiFi] %d, Disconnected - Status %d, %s\n", event, WiFi.status(), connectionStatus( WiFi.status() ).c_str() );
          break;

#if defined(ESP8266)
      case WIFI_EVENT_STAMODE_AUTHMODE_CHANGE:
          dbg_printf("[WiFi] %d, AuthMode Change\n", event);
          break;
#endif

      case WIFI_EVENT_STAMODE_GOT_IP:
          dbg_printf("[WiFi] %d, Got IP: %s\n", event, WiFi.localIP().toString().c_str());
          if (syslog_buffer != "") {
              syslog.log(LOG_WARNING, syslog_buffer.c_str());
          }
          syslog.logf(LOG_INFO, "[WiFi] %d, Got IP: %s, using wifi index %i %s", event, WiFi.localIP().toString().c_str(), wifi_index, ssid[wifi_index].c_str());
          wifi_started = true;
          break;
#if defined(ESP8266)
      case WIFI_EVENT_STAMODE_DHCP_TIMEOUT:
          dbg_printf("[WiFi] %d, DHCP Timeout\n", event);
          break;
#endif

      case WIFI_EVENT_SOFTAPMODE_STACONNECTED:
          dbg_printf("[AP] %d, Client Connected\n", event);
          break;

#if defined(ESP8266)
      case WIFI_EVENT_SOFTAPMODE_STADISCONNECTED:
          dbg_printf("[AP] %d, Client Disconnected\n", event);
          break;

      case WIFI_EVENT_SOFTAPMODE_PROBEREQRECVED:
//      dbg_printf("[AP] %d, Probe Request Recieved\n", event);
          break;
#endif
    }
}

void setup(void){
#if defined(ESP8266)
    digitalWrite(D8, LOW);  // should be unused, reset pin, assign to known state
    digitalWrite(D4, HIGH); // should be unused, reset pin, assign to known state
//    digitalWrite(D3, HIGH); // should be unused, reset pin, assign to known state - overriden by input_pullup below
//    pinMode(D3, OUTPUT);  // switch installed on this pin - maybe configure as input PULLUP instead?

    pinMode(D3, INPUT_PULLUP);
    pinMode(D4, OUTPUT);
    pinMode(D8, OUTPUT);
#endif
    // Calculate the R variable (only needs to be done once at setup)
    R = (led_range * log10(2))/(log10(led_range));

    pinMode(ONBOARD_LED_PIN, OUTPUT);
#if defined(ESP8266)
    analogWriteRange(led_range);
#endif

    /* switch on led */
//    digitalWrite(ONBOARD_LED_PIN, LOW);
//    ledRamp(0,led_range,1000,30);
    ledBright(led_range);

//    pinMode(relayPin, OUTPUT);

    Serial.begin(115200);
    Serial.println("");
    Serial.println("Booting");

#if ESP32
    // //Increment boot number and print it every reboot
    // Serial.println("Boot number: " + String(bootCount));
#endif

    //Print the wakeup reason for ESP32
    // print_wakeup_reason();

    ledBright(0);

    WiFi.setAutoConnect(true);  // Autoconnect to last known Wifi on startup
    WiFi.setAutoReconnect(true);
    WiFi.onEvent(eventWiFi);      // Handle WiFi event
    WiFi.mode(WIFI_STA);
    // if (!bootCount) { //FIXME: && ! on_battery_power) {
        start_wifi();

        Serial.println("");

//    while (WiFi.waitForConnectResult() != WL_CONNECTED) {
//        Serial.println("Connection Failed! Rebooting...");
//        delay(5000);
//        reboot();
//    }

//         Serial.println("");
//         Serial.print("Connected to ");
//         Serial.println(ssid[0]);
//         Serial.print("IP address: ");
//         Serial.println(WiFi.localIP());

//     //first parameter is name of access point, second is the password
// //    wifiManager.autoConnect("ledstrip");

// //    WiFiManager wifiManager;

//         Serial.println("");
    // }

    //find it as http://lights.local
    /*if (MDNS.begin("lights"))
      {
      Serial.println("MDNS responder started");
      }*/

//    ledRamp(led_range,0,1000,30);

//    delay(1500);

//    ledRamp(0,led_range,80,30);
//    ledBright(led_range);
//    ledBright(1);  // WARNING: don't leave led at analogue value for length of time, since led strip doesn't like it even when it's operating on another pin.  Timing?
    digitalWrite(ONBOARD_LED_PIN, 1);
    Serial.println("Booted");

    setup_stub();
    Serial.println("Ready!");
    // ++bootCount;
}

void http_start() {
//    server.on("/", handleRoot);
    http_started=true;

    server.on("/uptime", http_uptime);

    server.on("/reboot", http_reboot);
    server.on("/reset", http_reboot);

    server.on("/wifi", http_wifi);

    server.on("/failover_wifi", http_trigger_wifi_failover);


    http_start_stub();

    server.onNotFound(handleNotFound);

    server.begin();
    Serial.println("HTTP server started");

    // Port defaults to 8266
    // ArduinoOTA.setPort(8266);

    // No authentication by default
    ArduinoOTA.setPassword(ota_password.c_str());

    ArduinoOTA.setHostname(hostname.c_str());
    ArduinoOTA.onStart([]() { // switch off PWM during upgrade
            analogWrite(ONBOARD_LED_PIN,0);
            // FIXME: If using https://github.com/xoseperez/eeprom_rotate:
            // EEPROM.rotate(false);
            // EEPROM.commit();
        });

    ArduinoOTA.onEnd([]() { // do a fancy thing with our board led at end
            ledRamp(0,0.8*led_range,1000,30);
            ledRamp(0,0.8*led_range,1000,30);
            ledRamp(0.8*led_range,0,2000,30);
        });
    ArduinoOTA.onProgress([](unsigned long int progress, unsigned long int total) {
//            debug += "ArduinoOTA.onProgress(" + String(progress) + ", " + String(total) + ")<br><br>";

            Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
//            ledBright(progress / (total/ led_range));
//            ledBright(progress % led_range);
        });
    ArduinoOTA.onError([](ota_error_t error) {
            Serial.printf("Error[%u]: ", error);
            if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
            else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
            else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
            else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
            else if (error == OTA_END_ERROR) Serial.println("End Failed");
            ledErrorBlink(10, 500,200);
            // FIXME: If using https://github.com/xoseperez/eeprom_rotate:
            // EEPROM.rotate(true);   // Superfluous one presumes, if just about to reboot
            ESP.restart();
        });

    /* setup the OTA server */
    ArduinoOTA.begin();

    //FIXME: port to esp32
#if defined (ESP8266)
    /* Setup the pings */
    Ping.on(true,[](const AsyncPingResponse& response){
        IPAddress addr(response.addr); //to prevent with no const toString() in 2.3.0
        if (response.answer)
            Serial.printf("%d bytes from %s: icmp_seq=%d ttl=%d time=%d ms\n", response.size, addr.toString().c_str(), response.icmp_seq, response.ttl, response.time);
        else
            Serial.printf("no answer yet for %s icmp_seq=%d\n", addr.toString().c_str(), response.icmp_seq);
        return false; //do not stop
      });
    Ping.on(false,[](const AsyncPingResponse& response){
        IPAddress addr(response.addr); //to prevent with no const toString() in 2.3.0
        Serial.printf("total answer from %s sent %d received %d time %d ms\n",addr.toString().c_str(),response.total_sent,response.total_recv,response.total_time);
        if (response.mac)
            Serial.printf("detected eth address " MACSTR "\n",MAC2STR(response.mac->addr));
        if (response.total_recv < 1) {
            increment_wifi_failover();
        } else {
            decrement_wifi_failover();
        }
        return true;
      });
#endif
    last_ping_time=millis();
}

void loop() {
    unsigned long t=millis();
    // if 49 day 32 bit rollover:
    if (last_millis > t) {
        uptime_loop++;
    }
    last_millis=t;

    if (wifi_started && !http_started) {
        http_start();
    }
    if (triggered_wifi_failover) {
        execute_wifi_failover();
    }

    server.handleClient(); //FIXME: temporarily disabled for testing of camera
    ArduinoOTA.handle();
    delay(1);  // FIXME: find a generic way of doing this.  yield() wasn't always reliable
    loop_stub();

    if ((millis() - last_ping_time)/1000 > 60) {
        if (WiFi.status() != WL_CONNECTED) {
            increment_wifi_failover();
#if defined(ESP8266)
        } else {
            Ping.begin(WiFi.gatewayIP());
#endif
        }
        last_ping_time=millis();
    }

    // FIXME: dealing with crashes:
    // esp8266/doc/faq/a02-my-esp-crashes.rst
    //       Instrumenting the code with the OOM debug option and calls to
    //       ``ESP.getFreeHeap()`` / ``ESP.getHeapFragmentation()`` /
    //       ``ESP.getMaxFreeBlockSize()`` will help the process of finding memory issues.

}

