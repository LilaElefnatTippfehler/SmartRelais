#include "config.h"
#include <Arduino.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <homie.hpp>
#include <PubSubClient.h>

#define CLIENTID "ESP8266Client-"
#define SERIAL false
#define PIN D3

void httpServer_ini();
void handleStatus();
void callback(char *topic, byte *payload, unsigned int length);
boolean reconnect();

const char *update_path = "/firmware";
const char *update_username = USERNAME;
const char *update_password = PASSWORD;
String ClientID;
unsigned long lastReconnectAttempt = 0;

ESP8266HTTPUpdateServer httpUpdater;
ESP8266WebServer httpServer(80);
WiFiClient espClient;
PubSubClient client(MQTT_IP, MQTT_PORT, callback, espClient);
Homie homieCTRL = Homie(&client);

void setup()
{
        if(SERIAL) Serial.begin(115200);
        if(SERIAL) while (!Serial);
        WiFi.mode(WIFI_STA);
        WiFi.begin(WIFI_SSID, WIFI_PASS);
        while (WiFi.status() != WL_CONNECTED) {
                delay(500);
                if(SERIAL) Serial.print(".");
        }
        if(SERIAL) Serial.println("");
        if(SERIAL) Serial.print("Connected, IP address: ");
        if(SERIAL) Serial.println(WiFi.localIP());
        pinMode(PIN, OUTPUT);

        HomieDevice homieDevice = HomieDevice(DEVICE_NAME, F_DEVICE_NAME, WiFi.localIP().toString().c_str(),
                                              WiFi.macAddress().c_str(), FW_NAME, FW_VERSION,
                                              "esp01-s", "60");
        HomieNode relais = HomieNode("relais", "Relais Socket", "Relais Socket");
        HomieProperties power = HomieProperties("power", "power", true, true, "",
                                                homie::boolean_t, "");
        relais.addProp(power);
        homieDevice.addNode(relais);
        homieCTRL.setDevice(homieDevice);

        httpServer_ini();
        ClientID = String(CLIENTID) + DEVICE_NAME;
        homieCTRL.connect(ClientID.c_str(), MQTT_USR, MQTT_PW);

        //ESP.deepSleep(uint64_t time_us, optional RFMode mode);
        wifi_set_sleep_type(MODEM_SLEEP_T);

        //Setup Succesfull------
        digitalWrite(PIN, 1);
}

void loop() {
        if (!homieCTRL.connected()) {
                unsigned long now = millis();
                if (now - lastReconnectAttempt > 5000) {
                        lastReconnectAttempt = now;
                        // Attempt to reconnect
                        if (reconnect()) {
                                lastReconnectAttempt = 0;
                        }
                }
        }
        homieCTRL.loop();
        httpServer.handleClient();
        MDNS.update();
}


void httpServer_ini() {
        char buffer[100];
        sprintf(buffer, "%s", DEVICE_NAME);
        MDNS.begin(buffer);
        httpUpdater.setup(&httpServer, update_path, update_username, update_password);
        httpServer.on("/status",handleStatus);
        httpServer.begin();
        MDNS.addService("http", "tcp", 80);
        if(SERIAL) Serial.printf("HTTPUpdateServer ready! Open http://%s.local%s in your "
                                 "browser and login with username '%s' and password '%s'\n",
                                 buffer, update_path, update_username, update_password);
        //------
}

boolean reconnect() {
        // Loop until we're reconnected
        return homieCTRL.connect(ClientID.c_str(), MQTT_USR, MQTT_PW);
}

void handleStatus() {
        String message;
        message += "name: " + String(DEVICE_NAME) + "\n";
        message += "IP: " + WiFi.localIP().toString() + "\n";
        message +="free Heap: " + String(ESP.getFreeHeap()) + "\n";
        message += "heap Fragmentation: " + String(ESP.getHeapFragmentation()) + "\n";
        message += "MaxFreeBlockSize: " + String(ESP.getMaxFreeBlockSize()) + "\n";
        message += "ChipId: " + String(ESP.getChipId()) + "\n";
        message += "CoreVersion: " + String(ESP.getCoreVersion()) + "\n";
        message += "SdkVersion: " + String(ESP.getSdkVersion()) + "\n";
        message += "SketchSize: " + String(ESP.getSketchSize()) + "\n";
        message += "FreeSketchSpace: " + String(ESP.getFreeSketchSpace()) + "\n";
        message += "FlashChipId: " + String(ESP.getFlashChipId()) + "\n";
        message += "FlashChipSize: " + String(ESP.getFlashChipSize()) + "\n";
        message += "FlashChipRealSize: " + String(ESP.getFlashChipRealSize()) + "\n";
        httpServer.send(200, "text/plain", message);
}

void callback(char *topic, byte *payload, unsigned int length) {
        string topicString = string(topic);
        string searchString = string(DEVICE_NAME) + "/relais/power/set";
        if(SERIAL) Serial.println(topicString.c_str());

        std::size_t found = topicString.find(searchString);
        if(found!=std::string::npos) {
                char buffer[6];
                snprintf(buffer, length + 1, "%s", payload);
                if (!strcmp(buffer, "true")) {
                        digitalWrite(PIN, 0);
                        return;
                }
                if (!strcmp(buffer, "false")) {
                        digitalWrite(PIN, 1);
                        return;
                }
        }
}
