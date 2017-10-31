
#include "xap.h"
#include "bsc.h"
#include "Bounce2.h"

const char *ssid = "iot";
const char *password = "socrates";
const char *uid = "FFDB5500";
const char *source = "dbzoo.esp.demo";

Xap xap(source, uid);
Bsc lamp1(xap, "lamp", "1", BscDirection::Output, BscType::Binary);
Bsc lamp2(xap, "lamp", "2", BscDirection::Output, BscType::Binary);
Bsc button(xap, "button", NULL, BscDirection::Input, BscType::Binary);

Bounce debouncer = Bounce();

#define lampPin1 13
#define lampPin2 5
#define buttonPin 14
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
  Serial.print("lamp ");
  Serial.print(subaddr);
  Serial.print(" ");
  Serial.println(e->getStateStr());
  int v = e->getState() == BscState::On ? HIGH : LOW;
  int pin = *subaddr == '1' ? lampPin1 : lampPin2;
  digitalWrite(pin, v);
}

bool buttonInfoEventCB(Bsc *e, BscInfoEvent ie) {
  if (e->getState() == BscState::On) {
    e->setDisplayText("pressed");
  } else {
    e->setDisplayText("unpressed");
  }
  return true;
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

  pinMode(buttonPin, INPUT);
  digitalWrite(buttonPin, HIGH);
  button.setState(BscState::Off);
  button.onQuery(buttonInfoEventCB);
  debouncer.attach(buttonPin);
  debouncer.interval(5);

}

void loop() {
  xap.handle();
  debouncer.update();

  int v = debouncer.read();
  BscState b = v == LOW ? BscState::On : BscState::Off;
  if (b != button.getState()) {
    button.setState(b);
    button.sendEvent();
  }
}
