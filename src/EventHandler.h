#pragma once
#include <cinttypes>
class EventLoop;

class EventHandler {
public:
  EventHandler(int descriptor);
  ~EventHandler();

  // Note: it is important to handle only one buffer-full
  // of reading. Don't read untill blocked, this will cause
  // starvation. Instead, return to the event loop often.
  // Buffer should sized such that this context switch
  // is sufficiently amortized.
  // TODO: This is easier to implement with Level Triggering.
  // TODO: Ideally we'd have level-triggered on read and edge on write?
  virtual void handle(EventLoop& el, uint32_t events) { }

protected:
  friend class EventLoop;

  // File descriptor
  const int descriptor;

  // Event mask to supply to epoll_ctl
  uint32_t event_mask;
};
