
#include "xap.h"
#include "bsc.h"

const char *ssid = "iot";
const char *password = "socrates";
const char *uid = "FFDB5500";
const char *source = "dbzoo.esp.demo";

Xap xap(source, uid);
Bsc relay(xap, "relay", NULL, BscDirection::Output, BscType::Binary);
Bsc lamp1(xap, "lamp", "1", BscDirection::Output, BscType::Binary);
Bsc lamp2(xap, "lamp", "2", BscDirection::Output, BscType::Binary);
Bsc door(xap, "door", NULL, BscDirection::Output, BscType::Binary);
Bsc button(xap, "button", NULL, BscDirection::Input, BscType::Binary);
Bsc temperature(xap,"temperature", NULL, BscDirection::Output, BscType::Stream);

/*
== Toggle relay endpoint ==
xap-header
{
v=12
hop=1
uid=FFDB4400
class=xapbsc.cmd
target=dbzoo.esp.demo:relay
source=dbzoo.acme.test
}
output.state.1
{
id=*
state=toggle
}

== Send something to the LCD ==
xap-header
{
v=12
hop=1
uid=FFDB4400
class=xap-osd.display
target=dbzoo.esp.demo:lcd
source=dbzoo.acme.test
}
display
{
text=hello world
}

== Toggling lamps ==
xap-header
{
v=12
hop=1
uid=FFDB4400
class=xapbsc.cmd
target=dbzoo.esp.demo:lamp.1
source=dbzoo.acme.test
}
output.state.1
{
id=*
state=toggle
}  
*/
void lampCmdCB(Bsc *e) {
  Serial.print("lamp ");
  Serial.print(e->getSubaddr());
  Serial.print(" ");
  Serial.println(e->getStateStr());  
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

  // Setup an LCD endpoint
  xap.on("lcd", [](XapMessage msg) {
    if (msg.isValue("xap-header","class","xap-osd.display")) {
      char *text = msg.getValue("display", "text");
      if (text) {
        Serial.print("LCD display: ");
        Serial.println(text);
      }
    }
  });

  // Passing a function as the callback
  lamp1.onCmd(lampCmdCB);
  lamp2.onCmd(lampCmdCB);

  // using a lambda as the callback
  relay.onCmd([](Bsc * e) {
    if (e->getState() == BscState::On) {
      Serial.println("relay on");
    } else {
      Serial.println("relay off");
    }
  });

  // Display text is used to annote a binary state
  door.onCmd([](Bsc * e) {
    if (e->getState() == BscState::On) {
      Serial.println("door closed");
      e->setDisplayText("closed");
    } else {
      Serial.println("door open");
      e->setDisplayText("open");
    }
  });

  button.setState(BscState::Off);
  // Simulate a button press every 10s
  xap.timedEvent(10000, []() {
    button.setState(2);
    button.sendEvent();    
  });

  randomSeed(analogRead(0));
  temperature.setText("32");
  temperature.setDisplayText("32 F");
  // Simulate temp flucations
  xap.timedEvent(20000, []() {
    char ns[6];
    long n = random(0,50);
    sprintf(ns,"%d",n);
    temperature.setText(ns);
    sprintf(ns,"%d C",n);
    temperature.setDisplayText(ns);
    temperature.sendEvent();    
  });
}

void loop() {
  xap.handle();
}
