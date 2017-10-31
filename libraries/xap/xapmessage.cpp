#include "xapmessage.h"

XapMessage::XapMessage(uint8_t *msg, int size) {
  _parseMsg(msg, size);
}

void XapMessage::toSerialPort() {
  char *currentSection = NULL;
  for (int i = 0; i < xapMsgPairs; i++) {
    if (currentSection == NULL || currentSection != xapMsg[i].section) {
      if (currentSection != NULL) {
        Serial.println("}");
      }
      Serial.println(xapMsg[i].section);
      Serial.println("{");
      currentSection = xapMsg[i].section;
    }
    Serial.print(xapMsg[i].key);
    Serial.print("=");
    Serial.println(xapMsg[i].value);
  }
  Serial.println("}");
}

char *XapMessage::getValue(const char *section, const char *key) {
  for (int i = 0; i < xapMsgPairs; i++) {
    if (strcasecmp(section, xapMsg[i].section) == 0 && strcasecmp(key, xapMsg[i].key) == 0)
      return xapMsg[i].value;
  }
  return (char *)NULL;
}

int XapMessage::getType(void) {
  if (xapMsgPairs == 0) return XAP_MSG_NONE;
  if (strcasecmp(xapMsg[0].section, "xap-hbeat") == 0) return XAP_MSG_HBEAT;
  if (strcasecmp(xapMsg[0].section, "xap-header") == 0) return XAP_MSG_ORDINARY;
  return XAP_MSG_UNKNOWN;
}

int XapMessage::_decode_state(char *msg) {
  static const char *value[] = {"on", "off", "true", "false", "yes", "no", "1", "0"};
  static const int state[] = {1, 0, 1, 0, 1, 0, 1, 0};
  if (msg == NULL) return -1;
  for (int i = 0; i < sizeof(value); i++) {
    if (strcasecmp(msg, value[i]) == 0)
      return state[i];
  }
  return -1;
}

int XapMessage::getState(char *section, char *key) {
  return _decode_state(getValue(section, key));
}

int XapMessage::isValue(char *section, char *key, char *value) {
  char *kvalue = getValue(section, key);
  return kvalue && strcasecmp(kvalue, value) == 0;
}

void XapMessage::_rtrim( uint8_t *msg,  uint8_t *p) {
  while (*p < 32 && p > msg)
    *p-- = '\0';
}

void XapMessage::_parseMsg(uint8_t *msg, int size) {
  // msg is modified.
  enum {
    START_SECTION_NAME, IN_SECTION_NAME, START_KEYNAME, IN_KEYNAME, START_KEYVALUE, IN_KEYVALUE
  } state = START_SECTION_NAME;
  char *current_section = NULL;
  xapMsgPairs = 0;

  for (uint8_t *buf = msg; buf < msg + size; buf++) {
    switch (state) {
      case START_SECTION_NAME:
        if ( (*buf > 32) && (*buf < 128) ) {
          state = IN_SECTION_NAME;
          current_section = (char *)buf;
        }
        break;
      case IN_SECTION_NAME:
        if (*buf == '{') {
          *buf = '\0';
          _rtrim(msg, buf);
          state = START_KEYNAME;
        }
        break;
      case START_KEYNAME:
        if (*buf == '}') {
          state = START_SECTION_NAME;
        }
        else if ((*buf > 32) && (*buf < 128)) {
          xapMsg[xapMsgPairs].section = current_section;
          xapMsg[xapMsgPairs].key = (char *)buf;
          state = IN_KEYNAME;
        }
        break;
      case IN_KEYNAME:
        if ((*buf < 32) || (*buf == '=')) {
          *buf = '\0';
          _rtrim(msg, buf);
          state = START_KEYVALUE;
        }
        break;
      case START_KEYVALUE:
        if ((*buf > 32) && (*buf < 128)) {
          state = IN_KEYVALUE;
          xapMsg[xapMsgPairs].value = (char *)buf;
        }
        break;
      case IN_KEYVALUE:
        if (*buf < 32) {
          *buf = '\0';
          _rtrim(msg, buf);
          state = START_KEYNAME;
          xapMsgPairs++;
          if (xapMsgPairs >= MAX_XAP_PAIRS) {
            xapMsgPairs = 0;
          }
        }
        break;
    }
  }
}
