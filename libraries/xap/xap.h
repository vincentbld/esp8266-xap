#ifndef XAP_H_
#define XAP_H_

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <vector>

#include "xapfilter.h"
#include "xaptimer.h"

#define XAP_PORT 3639

const char HEARTBEAT_TEMPLATE[] PROGMEM =
  "xap-hbeat\n"
  "{\n"
  "v=12\n"
  "hop=1\n"
  "uid=%s\n"
  "class=xap-hbeat.alive\n"
  "source=%s\n"
  "interval=60\n"
  "port=3639\n"
  "ip=%s\n"
  "}";

class Xap {
  public:
    Xap(const char *source, const char *uid);
    void on(const char *endpoint, XapHandlerFunction handler);
	  void on(XapFilter f, XapHandlerFunction fn);
    void handle();
    XapTimer *timedEvent(long timeout, TimerFunction fn);
    void xapSend(char *);
    const char *uid;
    const char *source;
  private:
    std::vector<XapFilter> _filters;
    std::vector<XapTimer *> _timers;
    void _heartbeat();
    IPAddress _broadcastIP;
    WiFiEventHandler _handler;
    WiFiUDP _udp;
};

#endif
