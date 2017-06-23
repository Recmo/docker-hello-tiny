#pragma once
#include <cinttypes>
#include <limits>
#include "EventHandler.h"

class Timer: public EventHandler {
public:
  static uint64_t current_time_ns();

  Timer();
  ~Timer();
  virtual void handle(EventLoop& el, uint32_t events) final;

  void start(uint64_t interval_ns);
  void stop();

private:
  static int create_timer();
  void flush_timer();
  void set_timeout(uint64_t time_ns);
};
