#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266HTTPClient.h>
#include <AdafruitIO.h>
#include "config.h"

#define DEVICE_NAME "Nachttisch"
#define LED D1
void printWifiStatus();
void handleMessage(AdafruitIO_Data *data);
void handleOn();
void handleOff();
void handleStatus();

//WiFiServer server(8080);
AdafruitIO_Feed *status = io.feed("on-slash-off");
ESP8266HTTPUpdateServer httpUpdater;
ESP8266WebServer httpServer(80);
int PinStatus = 0;

const char* host = "esp8266-webupdate";
const char* update_path = "/firmware";
const char* update_username = USERNAME;
const char* update_password = PASSWORD;

void setup()
{
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  while(! Serial);

  Serial.print("Connecting to Adafruit IO");

  // connect to io.adafruit.com
  io.connect();
  status->onMessage(handleMessage);
  // wait for a connection
  while(io.status() < AIO_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  // we are connected
  Serial.println();
  Serial.println(io.statusText());
  Serial.print("Connected, IP address: ");
  Serial.println(WiFi.localIP());

  pinMode(LED, OUTPUT);
  status->get();
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

  io.run();
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
  digitalWrite(LED, 1);
  PinStatus = 1;
  status->save(DEVICE_NAME+'1');
}

void handleOff(){
  httpServer.send(200, "text/plain", "Relais is turned off");
  digitalWrite(LED, 0);
  PinStatus = 0;
  status->save(DEVICE_NAME+'0');
}

void handleStatus(){
  if(PinStatus){
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



void handleMessage(AdafruitIO_Data *data){
  Serial.print("received <- ");
  Serial.println(data->value());
  String data_rec = data->toString();
  String nameOn = DEVICE_NAME;
  String nameOff = DEVICE_NAME;
  if(!data_rec.compareTo(nameOff+ '0')){
    digitalWrite(LED, 0);
    PinStatus = 0;
    Serial.println("turning Off");
  }
  if(!data_rec.compareTo(nameOn+ '1')){
    digitalWrite(LED, 1);
    PinStatus = 1;
    Serial.println("turning On");
  }
}
