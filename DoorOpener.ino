/******************************************************************************
* Relay to give impulses to a door opener/closer
* MQTT send ms to defined topic to how long the relay should be on
*
* Libs
*
* https://github.com/jandrassy/ArduinoOTA
* https://github.com/hideakitai/MQTTPubSubClient Use Version 0.1.3 this is smaller when your have ota problems
* https://github.com/hideakitai/ArxContainer // dependency for MQTT
* https://github.com/hideakitai/ArxTypeTraits // dependency for MQTT
*
* https://github.com/tzapu/WiFiManager
*
* Licence: AGPL3
* Author: S. Fambach
* Website: http://Fambach.net
*******************************************************************************/

// switches for different functionalities
#define MQTT_ACTIVE
#define WEB_ACTIVE
#define OTA_ACTIVE
#define WIFI_MANAGER_ACTIVE  // if not activated normal wlan is active
#define DEBUG

#include "credentials.h"

#ifdef DEBUG
#define DEBUG_PRINTLN(x) Serial.println(x)
#define DEBUG_PRINT(x) Serial.print(x)
#else
#define DEBUG_PRINTLN(x)
#define DEBUG_PRINT(x)
#endif

/******************************************************************************/
// Door

long relayCurrentTS = 0;
long relayDelay = 0;
bool relayOn = false;

void relaySwitchOn(long ms) {
  relayCurrentTS = millis();
  relayDelay = ms;
  relayOn = true;

  digitalWrite(RELAIS_PIN, HIGH);
}

void relayLoop() {
  if (relayOn) {
    long elapsedTime = millis() - relayCurrentTS;
    if (elapsedTime >= relayDelay) {
      digitalWrite(RELAIS_PIN, LOW);
      relayOn = false;
    }
  }
}


/******************************************************************************/
// WLAN & WiFi Manager
#define HOST_NAME "Door2"

#ifdef ESP8266
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266mDNS.h>
#elif defined ESP32
#include <WiFi.h>
#include <mDNS.h>
#endif
#include <WiFiUdp.h>


#ifdef WIFI_MANAGER_ACTIVE
#include <WiFiManager.h>  // https://github.com/tzapu/WiFiManager
WiFiManager wm;
WiFiManagerParameter* customField;


String getParam(String name) {
  //read parameter from server, for customhmtl input
  String value;
  if (wm.server->hasArg(name)) {
    value = wm.server->arg(name);
  }
  return value;
}

void saveParamCallback() {
  DEBUG_PRINTLN("[CALLBACK] saveParamCallback fired");
  DEBUG_PRINTLN("PARAM customfieldid = " + getParam("CUST_ID"));
}

void setupWifiManager() {
  //wm.resetSettings();  // if you want to set wifi credentials again

  // custom field web creation
  customField = new WiFiManagerParameter("CUST_ID", "Custom Field", "/home/esp/", 40);  // custom html input

  wm.addParameter(customField);
  wm.setSaveParamsCallback(saveParamCallback);

  // walk on the dark side ...
  wm.setClass("invert");
  wm.setConfigPortalTimeout(160);  // Auto close cofig portal after 1 minute

  bool res = false;
  // res = wm.autoConnect(); // auto generated AP name from chipid
  res = wm.autoConnect("WifiMan");  // anonymous ap
  //res = wm.autoConnect("WifiMan","letMeIn1234"); // password protected ap

  if (!res) {
    DEBUG_PRINTLN("Failed to connect");
    //ESP.restart();
  } else {
    //if you get here you have connected to the WiFi
    DEBUG_PRINTLN("connected... :)");
  }

  wm.setWiFiAutoReconnect(true);
}

#else
ESP8266WiFiMulti wifiMulti;  

void setupWiFi() {

  WiFi.disconnect();
  delay(400);
  WiFi.mode(WIFI_STA);
  WiFi.hostname(HOST_NAME);
  delay(400);
  DEBUG_PRINTLN(HOST_NAME);
  DEBUG_PRINTLN(WLAN_SSID);
  DEBUG_PRINTLN(WLAN_PASSWD);
  wifiMulti.addAP(WLAN_SSID, WLAN_PASSWD);
  
/*  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    DEBUG_PRINTLN("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }*/

  while (wifiMulti.run() != WL_CONNECTED) { // Wait for the Wi-Fi to connect: scan for Wi-Fi networks, and connect to the strongest of the networks above
    delay(1000);
    Serial.print('.');
  }
  DEBUG_PRINTLN("Connection success");
}



#endif WIFI_MANAGER_ACTIVE




/******************************************************************************/
// OTA
#ifdef OTA_ACTIVE
#include <ArduinoOTA.h>  // https://github.com/jandrassy/ArduinoOTA

void setupOTA() {
  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname(HOST_NAME);

  // No authentication by default it is currently not working with Arduino gui 2.0
  // ArduinoOTA.setPassword("Gemuese76");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("ef84180df45cf2f22993e3a03dc71a27");
  //ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else {  // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    DEBUG_PRINTLN("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    DEBUG_PRINTLN("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      DEBUG_PRINTLN("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      DEBUG_PRINTLN("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      DEBUG_PRINTLN("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      DEBUG_PRINTLN("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      DEBUG_PRINTLN("End Failed");
    }
  });
  ArduinoOTA.begin();
}

#endif  // OTA_ACTIVE
/******************************************************************************/
#ifdef MQTT_ACTIVE

#include <MQTTPubSubClient.h>  // https://github.com/hideakitai/MQTTPubSubClient

WiFiClient client;
MQTTPubSubClient mqtt;

void setupMQTT() {
  DEBUG_PRINTLN("connecting to host...");
  while (!client.connect(MQTT_HOST_ADDRESS, MQTT_HOST_PORT)) {
    DEBUG_PRINT(".");
    delay(1000);
  }
  DEBUG_PRINTLN(" connected!");

  // initialize mqtt client
  mqtt.begin(client);

  DEBUG_PRINTLN("connecting to mqtt broker...");
  while (!mqtt.connect(HOST_NAME, MQTT_USR, MQTT_PASSWD)) {
    DEBUG_PRINT(".");
    delay(1000);
  }
  DEBUG_PRINTLN(" connected!");

  // subscribe topic and callback which is called when /hello has come
  mqtt.subscribe(String(MQTT_TOPIC) + String(HOST_NAME), [](const String& payload, const size_t size) {
    DEBUG_PRINT("Received: ");
    DEBUG_PRINTLN(payload);
    int ms = payload.toInt();
    relaySwitchOn(ms);
  });
}

#endif  // MQTT_ACTIVE
/******************************************************************************/
// Web server

#ifdef WEB_ACTIVE
WiFiServer server(WEB_SERVER_PORT);
String header;

void webLoop() {
  WiFiClient client = server.available();  // Hört auf Anfragen von Clients

  if (client) {                      // Falls sich ein neuer Client verbindet,
    DEBUG_PRINTLN("Neuer Client.");  // Ausgabe auf den seriellen Monitor
    String currentLine = "";         // erstelle einen String mit den eingehenden Daten vom Client
    while (client.connected()) {     // wiederholen so lange der Client verbunden ist
      if (client.available()) {      // Fall ein Byte zum lesen da ist,
        char c = client.read();      // lese das Byte, und dann
        DEBUG_PRINTLN(c);            // gebe es auf dem seriellen Monitor aus
        header += c;
        if (c == '\n') {  // wenn das Byte eine Neue-Zeile Char ist
          // wenn die aktuelle Zeile leer ist, kamen 2 in folge.
          // dies ist das Ende der HTTP-Anfrage vom Client, also senden wir eine Antwort:
          if (currentLine.length() == 0) {
            // HTTP-Header fangen immer mit einem Response-Code an (z.B. HTTP/1.1 200 OK)
            // gefolgt vom Content-Type damit der Client weiss was folgt, gefolgt von einer Leerzeile:
            client.println(F("HTTP/1.1 200 OK"));
            client.println(F("Content-type:text/html"));
            client.println(F("Connection: close"));
            client.println();

            // Hier werden die GPIO Pins ein- oder ausgeschaltet
            if (header.indexOf(F("GET /button/pressed")) >= 0) {
              DEBUG_PRINTLN(F("Button pressed"));
              relaySwitchOn(DEFAUL_DURATION_MS);
            }

            // Hier wird nun die HTML Seite angezeigt:
            client.println(F("<!DOCTYPE html><html>"));
            client.println(F("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"));
            if(relayOn){
               client.println(F("<meta http-equiv=\"refresh\" content=\"1,URL=http:\/\/"));
               client.println(WiFi.localIP());
               client.println(F("\/\" >"));
            }
            client.println(F("<link rel=\"icon\" href=\"data:,\">"));
            // Es folgen der CSS-Code um die Ein/Aus Buttons zu gestalten
            // Hier können Sie die Hintergrundfarge (background-color) und Schriftgröße (font-size) anpassen
            client.println(F("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}"));
            client.println(F(".button { background-color: #333344; border: none; color: white; padding: 16px 40px;"));
            client.println(F("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}"));
            client.println(F(".button2 {background-color: #888899;}</style></head>"));

            // Webseiten-Überschrift
            client.println(F("<body><h1>Door Opener</h1>"));

            // Zeige den aktuellen Status, und AN/AUS Buttons for GPIO 5
            client.println("<p>Relay - State " + String(relayOn) + "</p>");
            // wenn output5State = off, zeige den EIN Button

           // client.println(F("<p><a href=\"/button/pressed\">"));
           client.println(F("<p>"));
           client.println(F("<button class=\"button\" onclick=\"window.location.href='/button/pressed';\" >PUSH</button"));
           client.println(relayOn ? "disabled" : "");
           client.println(F("></p> </body></html>"));



            // Die HTTP-Antwort wird mit einer Leerzeile beendet
            client.println();
            // und wir verlassen mit einem break die Schleife
            break;
          } else {  // falls eine neue Zeile kommt, lösche die aktuelle Zeile
            currentLine = "";
          }
        } else if (c != '\r') {  // wenn etwas kommt was kein Zeilenumbruch ist,
          currentLine += c;      // füge es am Ende von currentLine an
        }
      }
    }
    // Die Header-Variable für den nächsten Durchlauf löschen
    header = "";
    // Die Verbindung schließen
    client.stop();
    DEBUG_PRINTLN("Client getrennt.");
  }
}
#endif  // WEB_ACTIVE


/** Main Programm ************************************************************/

void setup() {
  Serial.begin(BOUD);
  DEBUG_PRINTLN("Booting");

#ifdef WIFI_MANAGER_ACTIVE
  setupWifiManager();
#else
  setupWiFi();
#endif  // ndef WIFI_MANAGER_ACTIVE

#ifdef OTA_ACTIVE
  setupOTA();
#endif  // OTA_ACTIVE

#ifdef MQTT_ACTIVE
  setupMQTT();
#endif  //MQTT_ACTIVE

  DEBUG_PRINTLN("Ready");
  DEBUG_PRINT("IP address: ");
  DEBUG_PRINTLN(WiFi.localIP());

  // init relais
  pinMode(RELAIS_PIN, OUTPUT);
  digitalWrite(RELAIS_PIN, LOW);

  // webserver
#ifdef WEB_ACTIVE
  server.begin();
#endif  // WEB_ACTIVE
}

void loop() {

#ifdef OTA_ACTIVE
  // ota
  ArduinoOTA.handle();
#endif  // OTA_ACTIVE

#ifdef MQTT_ACTIVE
  // mqtt
  mqtt.update();
#endif  // MQTT_ACTIVE

  relayLoop();

#ifdef WEB_ACTIVE
  // webinterface
  webLoop();
#endif  // WEB_ACTIVE
}
