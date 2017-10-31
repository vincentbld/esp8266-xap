#ifndef XAP_FILTER_H
#define XAP_FILTER_H

#include <vector>
#include <functional>
#include "xapmessage.h"

class FilterPart {
  public:
    FilterPart(const char *section, const char *key, const char *value) {
      this->section = strdup(section);
      this->key = strdup(key);
      this->value = strdup(value);
    }
    const char *section;
    const char *key;
    const char *value;
};

typedef std::function<void(XapMessage)> XapHandlerFunction;

class XapFilter {
  public:
    const char *XAP_FILTER_ANY = "XAP_FILTER_ANY";
    const char *XAP_FILTER_ABSENT = "XAP_FILTER_ABSENT";
    XapFilter() { _handle = NULL; };
    XapFilter& on(XapHandlerFunction);
    XapFilter& add(const char *section, const char *key, const char *value);
    void dispatch(XapMessage);
  private:
    bool _isMatch(XapMessage msg);
    int _xapFilterAddrSubaddress(const char *filterAddr, const char *addr);
    std::vector<FilterPart> _filterParts;
    XapHandlerFunction _handle;
};
#endif
