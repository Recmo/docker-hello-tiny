#pragma once
#include "EventHandler.h"
#include <stdio.h>

class Socket: public EventHandler {
public:
  Socket(int descriptor);
  virtual ~Socket();

  virtual void handle(EventLoop& el, uint32_t events) final;

private:
  uint8_t state;
  uint64_t reqs = 0;
  off64_t offset = 0;
  uint64_t length = 0;
};
