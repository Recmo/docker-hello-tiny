#pragma once
#include "EventHandler.h"

class Signals: public EventHandler {
public:
  Signals();
  ~Signals();

  virtual void handle(EventLoop& el, uint32_t events) final;

protected:
  static int create();
};
