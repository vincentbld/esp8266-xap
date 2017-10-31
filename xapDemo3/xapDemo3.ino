#include <Adafruit_Sensor.h>
#include <DHT.h>
#include "xap.h"

const char *ssid = "iot";
const char *password = "socrates";
const char *uid = "FFDB5500";
const char *source = "dbzoo.dht11.sensor";

#define DHTPIN 13
#define DHTTYPE DHT11

Xap xap(source, uid);
DHT dht(DHTPIN, DHTTYPE);

const char TEMPLATE[] PROGMEM =
  "xap-header\n"
  "{\n"
  "v=12\n"
  "hop=1\n"
  "uid=%s\n"
  "class=sensor.data\n"
  "source=%s\n"
  "}\n"
  "sensor.report\n"
  "{\n"
  "tempC=%d.%d\n"
  "tempF=%d\n"
  "humidity=%d\n"
  "}";

void setup() {
  Serial.begin(115200);
  Serial.println("DHT11 Sensor Example");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("WiFi Failed");
    while (1) {
      delay(1000);
    }
  }

  dht.begin();

  xap.timedEvent(60000, [&]() {
    float h = dht.readHumidity();
    // Read temperature as Celsius (the default)
    float t = dht.readTemperature();
    // Read temperature as Fahrenheit (isFahrenheit = true)
    float f = dht.readTemperature(true);

    // Check if any reads failed and exit early.
    if (isnan(h) || isnan(t) || isnan(f)) {
      Serial.println("Failed to read from DHT sensor!");
      return;
    }

    char response[200];
    // sprintf does not do %f (workaround).  tempC we want 1 decimal of precision.
    // Yeah I know the DHT11 can't do that resolution it makes the code ready for the DHT22/21 :)
    sprintf_P(response, TEMPLATE, uid, source,
              (int)t,
              (int)(t * 10 - (((int)t) * 10)),
              (int)f,
              (int)h);
    xap.xapSend(response);
  });
}
void loop() {
  xap.handle();
}
