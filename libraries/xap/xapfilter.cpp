#include "xapfilter.h"

XapFilter& XapFilter::add(const char *section, const char *key, const char *value) {
  _filterParts.push_back(FilterPart(section, key, value));
  return *this;
}

int XapFilter::_xapFilterAddrSubaddress(const char *filterAddr, const char *addr)
{
  int match = true;

  if (filterAddr == NULL || *filterAddr == '\0') {
    // empty filterAddr matches all
    match = true;
  } else if (strchr(filterAddr, '*') == NULL && strchr(filterAddr, '>') == NULL) {
    // no wildcards - simple compare
    match = strcasecmp(filterAddr, addr) == 0;
  } else {
    // match using wildcard logic
    while (*filterAddr && *addr && match) {
      switch (*filterAddr) {
        case '>':  // FilterAddr matches rest of addr up to : if present.
          while (*addr && *addr != ':') {
            addr++;
          }
          filterAddr++;
          break;
        case '*': // skip a field in the addr
          while (*addr && !(*addr == '.' || *addr == ':')) {
            addr++;
          }
          filterAddr++;
          break;
        default:
          match = tolower(*filterAddr) == tolower(*addr);
      }
      if (*filterAddr) filterAddr++;
      if (*addr) addr++;
    }
    if (match) {
      // Make sure the pattern and target where fully compared.
      match = *filterAddr == '\0' && *addr == '\0';
    }
  }
  return match;
}

bool XapFilter::_isMatch(XapMessage msg) {
  bool match = true;

  for (int i = 0; i < _filterParts.size() && match; i++) {
    FilterPart f = _filterParts[i];
    char *value = msg.getValue(f.section, f.key);

    if (strcmp(f.value, XAP_FILTER_ABSENT) == 0) {
      match = value == NULL;
      continue;
    }

    if (strcmp(f.value, XAP_FILTER_ANY) == 0) {
      match = value ? true : false;
      continue;
    }

    // No section key and we need something to strcmp()
    if (value == NULL) {
      match = false;
      break;
    }

    if (strcasecmp("xap-header", f.section) == 0 &&
        (strcasecmp("target", f.key) == 0 ||
         strcasecmp("source", f.key) == 0 ||
         strcasecmp("class", f.key) == 0)) {
      if (strcasecmp("target", f.key) == 0) {
        // for target the WILD CARD will be in inbound xAP message
        match = _xapFilterAddrSubaddress(value, f.value);
      } else if (strcasecmp("source", f.key) == 0 ||
                 strcasecmp("class", f.key) == 0 ) {
        // for these the WILD CARD will be in the FILTER itself.
        match = _xapFilterAddrSubaddress(f.value, value);
      }
    } else {
      match = strcasecmp(value, f.value) == 0;
    }
  }
  return match;
}

XapFilter& XapFilter::on(XapHandlerFunction hndlr) {
  _handle = hndlr;
  return *this;
}

void XapFilter::dispatch(XapMessage msg) {
   if (_handle && _isMatch(msg)) {
        _handle(msg);
    }
}


