// Benq HT2050 Alexa projector control with ESP8266
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "fauxmoESP.h"

#define SERIAL_BAUDRATE     115200
#define WIFI_SSID "xxx"
#define WIFI_PASS "xxx"
#define ID_POWER  "projector"
#define ID_3D  "3d"
#define HOST_NAME "benq"
#define MAX_BUF_LEN 32

int telnetBufLen = 0;
char telnetBuf[MAX_BUF_LEN];
fauxmoESP fauxmo;
WiFiServer TelnetServer(23);
WiFiClient RemoteClient;
bool _connected = false;

//#define DEBUG_FAUXMO_VERBOSE_UDP true
//#define DEBUG_FAUXMO_VERBOSE_TCP true
//#define DEBUG_FAUXMO if (_connected) RemoteClient
#define debug(fmt,...) if (_connected) RemoteClient.printf(fmt, ##__VA_ARGS__);
#define debugChar(c) if (_connected) RemoteClient.write(c)

typedef enum {IDLE, POKE, GET_GT, SEND, RESPONSE} states_t;

class Benq {
  private:
    states_t state = IDLE;
    const char *action;
    int retry = 0;
    unsigned long now;
  public:
    // Non-blocking serial interactions
    void handle() {
      switch (state) {
        case IDLE:
          // Whilst idle drain the Rx buffer. Shouldn't be anything in there.
          while (Serial.available()) {
            Serial.read();
          }
          break;

        case POKE:
          // We write \r and expect > the states POKE/GET_GT handle this.
          Serial.write('\r');
          retry++;
          if (retry == 5) {
            debug("Handshake failed\n");
            state = IDLE;
            retry = 0;
          } else {
            state = GET_GT;
            now = millis();
          }
          break;

        case GET_GT:
          if (millis() - now > 1000) { // 1s
            debug("Poke timeout. Attempt %d\n", retry);
            state = POKE; // try again
          } else if (Serial.available()) {
            char c = Serial.read();
            if (c == '>')
              state = SEND;
          }
          break;

        case SEND:
          Serial.write('*');
          Serial.write(action);
          Serial.write("#\r");
          state = RESPONSE;
          now = millis();
          break;

        case RESPONSE:
          // We don't care about the responses but for debugging its useful to see them.
          if (millis() - now > 2000) { // Display 2s worth of data
            state = IDLE;
            debugChar('\n');
          } else
            while (Serial.available()) {
              debugChar(Serial.read());
            }
          break;
      }
    }

    // A raw command minus the <CR>* #<CR> wrapper.
    void sendCmd(const char *cmd) {
      action = cmd;
      state = POKE;
    }

    void power(bool state) {
      sendCmd(state ? "pow=on" : "pow=off");
    }

    void d3(bool state) {
      sendCmd(state ? "3d=sbs" : "3d=off");
    }
};

Benq projector = Benq();

// A telnet interface for projector control and remote debugging - non-blocking.
void telnetHandle()
{
  // Check for connections
  if (TelnetServer.hasClient())
  {
    // If we are already connected to another computer,
    // then reject the new connection. Otherwise accept
    // the connection.
    if (RemoteClient.connected()) {
      TelnetServer.available().stop();
    } else {
      RemoteClient = TelnetServer.available();
      RemoteClient.println("Commands: poweron, poweroff, status, free, !<raw cmd>");
    }
  }

  // Handle input from a connection
  _connected = RemoteClient && RemoteClient.connected();
  if (_connected) {
    while (RemoteClient.available()) {
      char c = RemoteClient.read();
      if (! ((c >= '!' && c <= 'z') || c == '\n')) continue;
      telnetBuf[telnetBufLen] = c;
      if (telnetBuf[telnetBufLen] == '\n') {
        telnetBuf[telnetBufLen] = 0;
        if (strcmp("poweron", telnetBuf) == 0) {
          projector.power(true);
        } else if (strcmp("poweroff", telnetBuf) == 0) {
          projector.power(false);
        } else if (strcmp("status", telnetBuf) == 0) {
          projector.sendCmd("pow=?");
        } else if (telnetBuf[0] == '!') {
          projector.sendCmd(&telnetBuf[1]);
        } else if (strcmp("free", telnetBuf) == 0) {
          RemoteClient.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
        } else {
          RemoteClient.println("Unknown command");
        }
        telnetBufLen = 0;
      } else {
        telnetBufLen++;
        if (telnetBufLen == MAX_BUF_LEN) {
          telnetBufLen = 0;
        }
      }
    }
  }
}

void wifiSetup() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
  }
}

void setup() {
  wifiSetup();
  Serial.begin(SERIAL_BAUDRATE);
  TelnetServer.begin();
  TelnetServer.setNoDelay(true);

  fauxmo.createServer(true); // not needed, this is the default value
  fauxmo.setPort(80); // This is required for gen3 devices
  fauxmo.enable(true);

  fauxmo.addDevice(ID_POWER);
  fauxmo.addDevice(ID_3D);

  fauxmo.onSetState([](unsigned char device_id, const char * device_name, bool state, unsigned char value) {
    debug("Device #%d (%s) state: %s value: %d", device_id, device_name, state ? "ON" : "OFF", value);
    if (strcmp(device_name, ID_POWER) == 0) {
      projector.power(state);
    } else if (strcmp(device_name, ID_3D) == 0) {
      projector.d3(state);
    }
  });
}

void loop() {
  fauxmo.handle();
  projector.handle();
  telnetHandle();
}
