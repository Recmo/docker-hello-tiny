#pragma once
#include <cinttypes>
#include "Timer.h"
#include "Signals.h"
class EventHandler;

class EventLoop {
public:
  EventLoop();
  ~EventLoop();

  // Timer object connected to this event loop
  Timer timer;

  Signals signals;

  // Start the main loop. Does not return
  // unless stop is called.
  // TODO: Re-entrant and thread-safe
  void run();

  // Stop the main loop.
  // TODO: Re-entrant and thread-safe
  void stop();

  void add_handler(EventHandler* handler);
  void update_handler(EventHandler* handler);
  void remove_handler(EventHandler* handler);

private:
  const int epoll_fd;
  bool running = true;

  static int epoll_create();
  void epoll_ctl(EventHandler* handler, int op);
};
