
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <fauxmoESP.h>
#include "xap.h"
#include "bsc.h"

const char *ssid = "iot";
const char *password = "socrates";
const char *uid = "FFDB5500";
const char *source = "dbzoo.esp.demo";

Xap xap(source, uid);
Bsc lamp1(xap, "lamp", "1", BscDirection::Output, BscType::Binary);
Bsc lamp2(xap, "lamp", "2", BscDirection::Output, BscType::Binary);
fauxmoESP fauxmo;

#define lampPin1 13
#define lampPin2 5
/*
  == Toggling lamps ==
xap-header
{
v=12
hop=1
uid=FFDB4400
class=xapbsc.cmd
target=dbzoo.esp.demo:lamp.2
source=dbzoo.acme.test
}
output.state.1
{
id=*
state=toggle
}
*/
void lampCmdCB(Bsc *e) {
  char *subaddr = e->getSubaddr();
  Serial.printf("[Xap] CMD lamp %s %s\n", subaddr, e->getStateStr());
  int v = e->getState() == BscState::On ? HIGH : LOW;
  int pin = *subaddr == '1' ? lampPin1 : lampPin2;
  digitalWrite(pin, v);
}

void setup() {
  Serial.begin(115200);
  Serial.println("xAP setup");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("WiFi Failed");
    while (1) {
      delay(1000);
    }
  }

  // Passing a function as the callback
  pinMode(lampPin1, OUTPUT);
  pinMode(lampPin2, OUTPUT);
  lamp1.onCmd(lampCmdCB);
  lamp2.onCmd(lampCmdCB);

  // The device ID's number sequentially as they are added.
  const unsigned char lampMo1 = fauxmo.addDevice("lamp one"); // device_id 0
  const unsigned char lampMo2 = fauxmo.addDevice("lamp two"); // device_id 1
  
  fauxmo.onMessage([&](unsigned char device_id, const char * device_name, bool state) {
        Serial.printf("[Alexa] Device #%d (%s) state: %s\n", device_id, device_name, state ? "ON" : "OFF");
        Bsc *lamp;
        if (device_id == lampMo1)
          lamp = &lamp1;
        else if (device_id == lampMo2)
          lamp = &lamp2;
        else
          return;
        lamp->setState(state ? BscState::On : BscState::Off);
        lampCmdCB(lamp);
        lamp->sendEvent();
    });
}

void loop() {
  xap.handle();
  fauxmo.handle();
}
