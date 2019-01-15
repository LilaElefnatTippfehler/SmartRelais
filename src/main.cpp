#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266HTTPClient.h>
#include <PubSubClient.h>
#include "config.h"

#define DEVICE_NAME "Nachttisch"
#define LED D1
void printWifiStatus();
void handleOn();
void handleOff();
void handleStatus();
void handleRoot();
void handleUpTime();
String generateHTML();
void updateOnTime();
void changeState(int i);
void callback(char* topic, byte* payload, unsigned int length);
void reconnect();

//WiFiServer server(8080);
WiFiClient espClient;
PubSubClient client(MQTT_IP,MQTT_PORT,callback,espClient);
ESP8266HTTPUpdateServer httpUpdater;
ESP8266WebServer httpServer(80);
int PinStatus = 0;
unsigned long onTime = 0;
unsigned long lastCheck = 0;

const char* host = "esp8266-SmartRelais";
const char* update_path = "/firmware";
const char* update_username = USERNAME;
const char* update_password = PASSWORD;

void setup()
{
        Serial.begin(115200);
        WiFi.mode(WIFI_STA);
        WiFi.begin(WIFI_SSID,WIFI_PASS);
        while (WiFi.status() != WL_CONNECTED) {
                delay(500);
                Serial.print(".");
        }
        while(!Serial);

        Serial.print("Connected, IP address: ");
        Serial.println(WiFi.localIP());

        pinMode(LED, OUTPUT);
        //Implementing OTA Updates
        MDNS.begin(host);
        httpUpdater.setup(&httpServer, update_path, update_username, update_password);
        httpServer.begin();
        MDNS.addService("http", "tcp", 80);
        Serial.printf("HTTPUpdateServer ready! Open http://%s.local%s in your browser and login with username '%s' and password '%s'\n", host, update_path, update_username, update_password);
        //------
        printWifiStatus();

        httpServer.on("/on",handleOn);
        httpServer.on("/off",handleOff);
        httpServer.on("/status",handleStatus);
        httpServer.on("/",handleRoot);
        httpServer.on("/time",handleUpTime);
        //ESP.deepSleep(uint64_t time_us, optional RFMode mode);
        wifi_set_sleep_type(MODEM_SLEEP_T);

        //Setup Succesfull------
        digitalWrite(LED, 1);
        delay(500);
        digitalWrite(LED, 0);
        delay(200);
        digitalWrite(LED, 1);
        delay(500);
        digitalWrite(LED, 0);
}

void loop() {

        if (!client.connected()) {
                reconnect();
        }
        client.loop();
        httpServer.handleClient();
        MDNS.update();

        delay(300);
}


void printWifiStatus() {
        // print the SSID of the network you're attached to:
        Serial.print("SSID: ");
        Serial.println(WiFi.SSID());

        // print your WiFi shield's IP address:
        IPAddress ip = WiFi.localIP();
        Serial.print("IP Address: ");
        Serial.println(ip);

        // print the received signal strength:
        long rssi = WiFi.RSSI();
        Serial.print("signal strength (RSSI):");
        Serial.print(rssi);
        Serial.println(" dBm");
}

void handleOn(){
        httpServer.send(200, "text/plain", "Relais is turned on");
        changeState(1);
        char s[50];
        sprintf(s,"%s1",DEVICE_NAME);
        client.publish("/lampen",s);
}

void handleOff(){
        httpServer.send(200, "text/plain", "Relais is turned off");
        changeState(0);
        char s[50];
        sprintf(s,"%s0",DEVICE_NAME);
        client.publish("/lampen",s);
}

void handleStatus(){
        if(PinStatus) {
                httpServer.send(200, "text/plain", "Relais is turned on");
        }else{
                httpServer.send(200, "text/plain", "Relais is turned off");
        }
}

void handleNotFound() {
        String message = "File Not Found\n\n";
        message += "URI: ";
        message += httpServer.uri();
        message += "\nMethod: ";
        message += (httpServer.method() == HTTP_GET) ? "GET" : "POST";
        message += "\nArguments: ";
        message += httpServer.args();
        message += "\n";
        for (uint8_t i = 0; i < httpServer.args(); i++) {
                message += " " + httpServer.argName(i) + ": " + httpServer.arg(i) + "\n";
        }
        httpServer.send(404, "text/plain", message);
}

void handleRoot() {
        httpServer.send(200, "text/html", generateHTML());
}

void handleUpTime(){
        updateOnTime();
        unsigned long sec = (millis()/1000%60);
        unsigned long min = (millis()/1000/60%60);
        unsigned long hr = (millis()/1000/60/60);
        unsigned long Onsec = (onTime/1000%60);
        unsigned long Onmin = (onTime/1000/60%60);
        unsigned long Onhr = (onTime/1000/60/60);
        char temp[400];
        snprintf(temp, 400,

                 "<html>\
        <head>\
          <meta http-equiv='refresh' content='1'/>\
          <style>\
            body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
          </style>\
        </head>\
        <body>\
          <p>Uptime: %02ld:%02ld:%02ld</p>\
          <p>Ontime: %02ld:%02ld:%02ld</p>\
        </body>\
      </html>",

                 hr, min, sec,Onhr, Onmin, Onsec
                 );
        httpServer.send(200, "text/html", temp);
}



void callback(char* topic, byte* payload, unsigned int length){
        Serial.print("received <- ");
        char buffer[100];
        snprintf(buffer,length+1,"%s",payload);
        Serial.println(buffer);
        String data_rec = String(buffer);
        String nameOn = DEVICE_NAME;
        String nameOff = DEVICE_NAME;
        if(!data_rec.compareTo(nameOff+ '0')) {
                changeState(0);
                Serial.println("turning Off");
        }
        if(!data_rec.compareTo(nameOn+ '1')) {
                changeState(1);
                Serial.println("turning On");
        }
        if(!data_rec.compareTo("ein") || !data_rec.compareTo("an")) {
                changeState(1);
                Serial.println("turning On");
        }
        if(!data_rec.compareTo("aus") || !data_rec.compareTo("ab")) {
                changeState(0);
                Serial.println("turning off");
        }
}

void updateOnTime(){
        if(lastCheck != 0) {
                onTime += millis() - lastCheck;
                lastCheck = millis();
        }
}

void changeState(int i){
        if(PinStatus != i) {
                if(lastCheck != 0) {
                        updateOnTime();
                        lastCheck = 0;
                }else{
                        lastCheck = millis();
                }
                digitalWrite(LED, i);
                PinStatus = i;
        }
}

String generateHTML(){

        return "<html>\
<head>\
<title>Smart Relais</title>\
<style>\
body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
</style>\
</head>\
<body>\
<h1>Smart Relais</h1>\
<iframe src='/time' width='250' height='80' border='0' name='uptime' id='uptime'></iframe>\
<br>\
<iframe src='/status' width='250' height='30' border='0' name='dummyframe' id='dummyframe'></iframe>\
<form action='/on' target='dummyframe' method='post'>\
<input type='submit' value='Turn on'>\
</form>\
<form action='/off' target='dummyframe' method='post'>\
<input type='submit' value='Turn off'>\
</form>\
</form>\
<form action='/firmware'>\
<input type='submit' value='Update'>\
</form>\
</body>\
</html>";
}

void reconnect() {
        // Loop until we're reconnected
        while (!client.connected()) {
                Serial.print("Attempting MQTT connection...");
                // Create a random client ID
                String clientId = "ESP8266Client-";
                clientId += DEVICE_NAME;
                // Attempt to connect
                if (client.connect(clientId.c_str(),MQTT_USR,MQTT_PW)) {
                        Serial.print("connected as ");
                        Serial.print(clientId.c_str()), Serial.println("");
                        client.subscribe("/lampen");

                } else {
                        Serial.print("failed, rc=");
                        Serial.print(client.state());
                        Serial.println(" try again in 5 seconds");
                        // Wait 5 seconds before retrying
                        delay(5000);
                }
        }
}
