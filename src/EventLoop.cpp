#include "EventLoop.h"
#include "EventHandler.h"
#include <unistd.h>
#include <sys/epoll.h>
#include <system_error>
#include <cerrno>
#include "syscall.h"
#include "fail.h"

EventLoop::EventLoop()
: epoll_fd(epoll_create())
{
}

EventLoop::~EventLoop()
{
  syscall<uint64_t>(SYS_close, epoll_fd);
}

void EventLoop::add_handler(EventHandler* handler)
{
  epoll_ctl(handler, EPOLL_CTL_ADD);
}

void EventLoop::update_handler(EventHandler* handler)
{
  epoll_ctl(handler, EPOLL_CTL_MOD);
}

void EventLoop::remove_handler(EventHandler* handler)
{
  epoll_ctl(handler, EPOLL_CTL_DEL);
}

void EventLoop::stop()
{
  running = false;
}

void EventLoop::run()
{
  // Stack allocate epoll event buffer
  constexpr std::size_t epoll_buffer_size = 10; // TODO Tune epoll buffer size
  epoll_event events[epoll_buffer_size];

  // Event loop
  running = true;
  while(running) {

    // Wait for event
    constexpr int timeout_ms = -1; // No timeout
    const int result = syscall(SYS_epoll_wait, epoll_fd, events, sizeof(events), timeout_ms);
    if(result <= 0) {
      fail(-result, "epoll_wait");
    }
    const uint num_events = result;

    // Process queued events
    for(uint i = 0; i < num_events; ++i) {
      const epoll_event& event = events[i];
      EventHandler* handler = static_cast<EventHandler*>(event.data.ptr);
      if(handler == nullptr) {
        fail(0, "No handler on event");
      }
      handler->handle(*this, event.events);
    }
  }
}

int EventLoop::epoll_create()
{
  const int result = syscall(SYS_epoll_create1, EPOLL_CLOEXEC);
  if(result < 0) {
    fail(result, "epoll_create1");
  }
  return result;
}

void EventLoop::epoll_ctl(EventHandler *handler, int op)
{
  // Create
  epoll_event event;
  event.data.ptr = handler;
  event.events = handler->event_mask;
  const int fd = handler->descriptor;

  // Add to epoll
  const int result = syscall(SYS_epoll_ctl, epoll_fd, op, fd, &event);
  if(result < 0) {
    fail(result, "epoll_ctl");
  }
}
