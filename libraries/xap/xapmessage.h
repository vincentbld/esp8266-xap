#ifndef XAP_MESSAGE_H
#define XAP_MESSAGE_H

#include <Arduino.h>

#define MAX_XAP_PAIRS 20
#define XAP_MSG_NONE 0
#define XAP_MSG_HBEAT  1
#define XAP_MSG_ORDINARY 2
#define XAP_MSG_UNKNOWN 0
#define XAP_DATA_LEN 1500

struct xapmsg_buffer {
  char *section;
  char *key;
  char *value;
};

class XapMessage {
  public:
    XapMessage(uint8_t *msg, int size);
    void toSerialPort();
    char *getValue(const char *section, const char *key);
    int getType();
    int getState(char *section, char *key); // 1 true, 0 false, -1 invalid
    int isValue(char *section, char *key, char *value);
  private:
    void _parseMsg(uint8_t *msg, int size);
    void _rtrim( uint8_t *msg,  byte *p);
    int _decode_state(char *msg);
    uint8_t xapMsgPairs;
    struct xapmsg_buffer xapMsg[MAX_XAP_PAIRS];
};

#endif