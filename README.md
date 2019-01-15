# SmartRelais

Small project using the ESP8266, a relais and my own MQTT Server on an RaspberryPi to controll a lamp.

MQTT Server is bridged with AdafruitIO, wich is connected with IFTTT.
MQTT Server is mosquitto on an RaspberryPi. The idea is that all other devices I'm planning to build communicate on my own network and not on external servers.

OTA Updates are implemented. IP_Adress/firmware or http://esp8266-SmartRelais.local/firmware

Can also be controlled with a browser.

All the fun happens on: http://esp8266-SmartRelais.local/    or IP_Adress/
