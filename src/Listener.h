#pragma once
#include "EventLoop.h"

class Listener: public EventHandler {
public:
  Listener(uint16_t port);
  ~Listener();
  virtual void handle(EventLoop& el, uint32_t events) final;

protected:
  static int create(uint16_t port);
};
