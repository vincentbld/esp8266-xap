#include "xap.h"
//#define XAP_DEBUG

Xap::Xap(const char *source, const char *uid )
{
  this->source = source;
  // We will break in BSC if this is not 8 chars long.
  if(strlen(uid) != 8) {
	  this->uid = "FFFFFF00";
  } else {
	 this->uid = uid; 
  }

    XapTimer *heartbeatTimer = timedEvent(60000, [this]() {
      _heartbeat();
    });

  // Start UDP server on STA connection
  _handler = WiFi.onStationModeGotIP([this, heartbeatTimer](WiFiEventStationModeGotIP ipInfo) {
    _broadcastIP = IPAddress(~(uint32_t)WiFi.subnetMask() | (uint32_t)WiFi.gatewayIP());
    _udp.begin(XAP_PORT);
#ifdef XAP_DEBUG
    Serial.println("xAP UDP listener started");
#endif
    heartbeatTimer->expire(); // Send a heartbeat
  });
}

XapTimer *Xap::timedEvent(long timeout, TimerFunction fn) {
  XapTimer *t = new XapTimer(timeout, fn);
  _timers.push_back(t);
  return t;
}

void Xap::on(const char* endpoint, XapHandlerFunction fn) {
  char fqen[strlen(source) + strlen(endpoint) + 2];
  strcpy(fqen, source);
  strcat(fqen, ":");
  strcat(fqen, endpoint);

  on(XapFilter().add("xap-header", "target", fqen), fn);
}

void Xap::on(XapFilter f, XapHandlerFunction fn) {
  _filters.push_back(f.on(fn));	
}

void Xap::xapSend(char *buf) {
  _udp.beginPacket(_broadcastIP, XAP_PORT);
  _udp.write(buf);
  _udp.endPacket();
}

void Xap::_heartbeat(void) {
  int size = strlen(HEARTBEAT_TEMPLATE) + 100;
  char response[size];
  snprintf_P(response, size, HEARTBEAT_TEMPLATE, uid, source, WiFi.localIP().toString().c_str());
  xapSend(response);
}

void Xap::handle() {
  int len = _udp.parsePacket();
  if (len > 0) {
#ifdef XAP_DEBUG
    Serial.print("Got UDP packet size: ");
    Serial.println(len);
#endif
    uint8_t data[len];
    _udp.read(data, len);
    XapMessage msg(data, len);
#ifdef XAP_DEBUG
    msg.toSerialPort();
#endif

    // Dispatch to the filters
    for (uint8_t i = 0; i < _filters.size(); i++) {
        _filters[i].dispatch(msg);
    }
  }
  // Handle ttl events
  for (uint8_t i = 0; i < _timers.size(); i++) {
    _timers[i]->check();
  }
}

