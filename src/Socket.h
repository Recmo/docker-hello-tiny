#pragma once
#include "EventHandler.h"
#include <stdio.h>

class Socket: public EventHandler {
public:
  Socket(EventLoop& el, int descriptor);
  ~Socket();

  virtual void handle(EventLoop& el, uint32_t events) final;

  bool added = false;
  bool closed = false;

private:
  uint8_t state;
  uint64_t reqs = 0;
  off64_t offset = 0;
  uint64_t length = 0;
};
