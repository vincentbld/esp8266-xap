#ifndef BSC_H_
#define BSC_H_

#include "xap.h"
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

#define BSC_INFO_CLASS "xAPBSC.info"
#define BSC_EVENT_CLASS "xAPBSC.event"
#define BSC_CMD_CLASS "xAPBSC.cmd"
#define BSC_QUERY_CLASS "xAPBSC.query"

enum class BscDirection {
  Input, Output
};

enum class BscType {
  Binary, Level, Stream
};

enum class BscInfoEvent {
  Info, Event
};

enum class BscState {
  Off, On, Unknown
};

static int uid_id = 1;

class Bsc;

class Bsc {
  public:
    Bsc(Xap &xap, char *name, char *subaddr, BscDirection dir, BscType type);
    void onCmd(void (*cmdCB)(Bsc *));
    void onQuery(bool (*infoEventCB)(Bsc *, BscInfoEvent));
    void sendInfo();
    void sendEvent();
    void setDisplayText(const char *);
    void setState(uint8_t);
    void setState(BscState s);
    BscState getState();
    char *getStateStr();
    void setText(const char *);
    char *getSubaddr();
  private:
    char *_subaddr;
    String source;
    void _infoEvent(BscInfoEvent);
    void _sendInfoEvent(BscInfoEvent);
    int8_t _decode_state(char *msg);    
    Xap& _xap;
    char id[3];
    BscDirection io;
    BscType type;
    BscState state;
    char *text;
    char *uid;
    XapTimer *_timer;
    char *displayText;
    bool (*infoEventCB)(Bsc *, BscInfoEvent);
    void (*cmdCB)(Bsc *);
    WiFiEventHandler _WifiHandler;
};
#endif
