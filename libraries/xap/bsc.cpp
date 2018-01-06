#include <time.h>
#include "xap.h"
#include "xapfilter.h"
#include "xapmessage.h"
#include "bsc.h"

// Implements Basic Status and Control Schema
// http://xapautomation.org/index.php?title=Basic_Status_and_Control_Schema

static bool _defaultInfoEvent(Bsc *e, BscInfoEvent type) {
  return true;
}

Bsc::Bsc(Xap &xap, char *name, char *subaddr, BscDirection dir, BscType type) : _xap(xap) {
  this->io = dir;
  this->type = type;
  this->uid = strdup(_xap.uid);
  this->infoEventCB = _defaultInfoEvent;
  this->cmdCB = NULL;
  this->text = NULL;
  this->displayText = NULL;
  _subaddr = NULL;

  sprintf(id, "%02X", uid_id++);
  // UID of the endpoint is the xAP UID with the latest 2 digits replaced with the endpoint ID.
  strncpy(&uid[6], id, 2);

  if (io == BscDirection::Input) {
    state = BscState::Unknown;
  } else {
    state = type == BscType::Binary ? BscState::Off : BscState::On;
  }

  if (type != BscType::Binary) {
    text = strdup("?");
  }

  source = String(_xap.source) + ":" + String(name);
  if (subaddr) {
    source += "." + String(subaddr);
    _subaddr = strdup(subaddr);
  }

  if (io == BscDirection::Output) {
    _xap.on(XapFilter()
            .add("xap-header", "class", BSC_CMD_CLASS)
            .add("xap-header", "target", source.c_str()),
    [this](XapMessage msg) {
      if (io == BscDirection::Input) return;
      char section[16];
      for (int i = 1; i < 99; i++) {
        sprintf(section, "output.state.%d", i);
        char *state = msg.getValue(section, "state");
        char *id = msg.getValue(section, "id");
        if (state == NULL || id == NULL) break;
        if (*id == '*' || strcmp(this->id, id) == 0) {
          setState(_decode_state(state));
          if (this->type == BscType::Level) {
            setText(msg.getValue(section, "level"));
          } else if (this->type == BscType::Stream) {
            setText(msg.getValue(section, "text"));
          }
        }
      }

      if (cmdCB) (*cmdCB)(this);
      sendEvent();
    });
  }

  _xap.on(XapFilter()
          .add("xap-header", "class", BSC_QUERY_CLASS)
          .add("xap-header", "target", source.c_str()),
  [this](XapMessage msg) {
    sendInfo();
  }
         );

  _timer = _xap.timedEvent(120000, [this]() {
    sendInfo();
  });  

  _WifiHandler = WiFi.onStationModeGotIP([this](WiFiEventStationModeGotIP ipInfo) {
    _timer->expire(); // SendInfo
  });
}

int8_t Bsc::_decode_state(char *msg) {
  static const char *value[] = {"on", "off", "true", "false", "yes", "no", "1", "0", "toggle"};
  static const int state[] = {1, 0, 1, 0, 1, 0, 1, 0, 2};
  if (msg == NULL) return -1;
  for (int i = 0; i < sizeof(value); i++) {
    if (strcasecmp(msg, value[i]) == 0)
      return state[i];
  }
  return -1;
}

char *Bsc::getSubaddr() {
    return _subaddr;
}

void Bsc::setText(const char *s) {
  if (text) free(text);
  text = strdup(s);
}

void Bsc::setDisplayText(const char *s) {
  if (displayText) free(displayText);
  displayText = strdup(s);
}

void Bsc::setState(BscState s) {
  state = s;
}

void Bsc::setState(uint8_t istate) {
  if (istate == 2) // toggle
    istate = state == BscState::On ? 0 : 1;
  state = istate == 0 ? BscState::Off : BscState::On;
}

BscState Bsc::getState() {
  return state;
}
char *Bsc::getStateStr() {
  switch (state) {
    case BscState::On: return "on";
    case BscState::Off: return "off";
  }
  return "unknown";
}

void Bsc::sendInfo() {
  _sendInfoEvent( BscInfoEvent::Info );
}

void Bsc::sendEvent() {
  _sendInfoEvent( BscInfoEvent::Event );
}

void Bsc::_sendInfoEvent(BscInfoEvent type) {
  if ((*infoEventCB)(this, type)) {
    _infoEvent(type);
  }
}

void Bsc::onQuery(bool (*CB)(Bsc *, BscInfoEvent)) {
  infoEventCB = CB;
}

void Bsc::onCmd(void (*CB)(Bsc *)) {
  cmdCB = CB;
}

void Bsc::_infoEvent(BscInfoEvent ie) {
  const char *ioString = io == BscDirection::Output ? "output" : "input";
  const char *clazz = ie == BscInfoEvent::Info ? "xAPBSC.info" : "xAPBSC.event";

  char buff[XAP_DATA_LEN];
  int len = snprintf(buff, XAP_DATA_LEN, "xap-header\n"
                     "{\n"
                     "v=12\n"
                     "hop=1\n"
                     "uid=%s\n"
                     "class=%s\n"
                     "source=%s\n"
                     "}\n"
                     "%s.state\n"
                     "{\n"
                     "state=%s\n",
                     this->uid, clazz, source.c_str(), ioString, getStateStr());

  if (this->type == BscType::Level) {
    len += snprintf(&buff[len], XAP_DATA_LEN - len, "level=%s\n", text);
  } else if (this->type == BscType::Stream) {
    len += snprintf(&buff[len], XAP_DATA_LEN - len, "text=%s\n", text);
  }
  if (displayText) {
    len += snprintf(&buff[len], XAP_DATA_LEN - len, "displaytext=%s\n", displayText);
  }
  len += snprintf(&buff[len], XAP_DATA_LEN - len, "}\n");

  if (len < XAP_DATA_LEN) {
    // if message buffer not truncated
    _xap.xapSend(buff);
  }
  _timer->reset();
}

