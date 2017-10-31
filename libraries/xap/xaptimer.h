#ifndef XAP_TIMER_H
#define XAP_TIMER_H

#include <functional>

typedef std::function<void(void)> TimerFunction;

class XapTimer {
  public:
    XapTimer(long timeout, TimerFunction fn);
    void reset();
    void check();
    void expire();
  private:
    bool _isExpired();
    long _start;
    TimerFunction _fn;
    long _timeout;
};

#endif
