#include <ESP8266WiFi.h>
#include <xap.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>

const char *ssid = "miguet_automation";
const char *password = "vincentbld196300";
const int pinLDR = A0;   // Analog pin for the LDR sensor
const int pinDHT = D7;   // Digital pin for the DHT sensor (GPIO13)
const int valeur_max = 1024; // Max value for a 10-bit analog input

Xap xap("dbzoo.ldr_dht.sensor", "FFDB2500");
DHT dht(pinDHT, DHT11); // Use DHT11 if that's the sensor you have

int previousMillis = 0;
int interval = 60000; // Interval in milliseconds (1 minute)

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  Serial.println("Connected to WiFi");
  dht.begin(); // Initialize the DHT sensor
}

void loop() {
  // Handle incoming XAP messages
  xap.handle();

  // Get the current time
  unsigned long currentMillis = millis();

  // Check if a minute has passed since the last transmission
  if (currentMillis - previousMillis >= interval) {
    // Update the previous time
    previousMillis = currentMillis;

    // Read the LDR value
    int valeurLDR = analogRead(pinLDR);
    // Invert the LDR value
    int valeurInverseLDR = valeur_max - valeurLDR;

    // Read temperature and humidity from DHT sensor
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();

    // Send XAP message with specific format
    sendXAPMessage(valeurInverseLDR, temperature, humidity);
  }

  delay(1000); // Small delay to stabilize the program
}

void sendXAPMessage(int valeurInverseLDR, float temperature, float humidity) {
  char messageBuffer[200]; // Adjust the size based on your needs

  // Format the XAP message
  snprintf(messageBuffer, sizeof(messageBuffer),
           "xap-header\n"
           "{\n"
           "v=12\n"
           "hop=1\n"
           "uid=FFDB2500\n"
           "class=wheather.report\n"
           "source=dbzoo.ldr_dht.sensor\n"
           "}\n"
           "sensor.report\n"
           "{\n"
           "valeurLDR=%d\n"
           "temperature=%.2f\n"
           "humidity=%.2f\n"
           "}\n",
           valeurInverseLDR, temperature, humidity);

  // Display the message on the serial monitor
  Serial.println(messageBuffer);

  // Send the XAP message via UDP
  xap.xapSend(messageBuffer);
}
