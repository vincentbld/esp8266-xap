#include <Arduino.h>
#include "xaptimer.h"

#define smillis() ((long)millis())

XapTimer::XapTimer(long timeout, TimerFunction fn ) {
  _timeout = timeout;
  _fn = fn;
  reset();
}

bool XapTimer::_isExpired() {
  return smillis() - _start >= 0;
}

void XapTimer::reset() {
  _start = smillis() + _timeout;
}

void XapTimer::expire() {
  _start = 0;
}

void XapTimer::check() {
  if (_isExpired()) {
    _fn();
    reset();
  }
}
