#include "EventHandler.h"
#include <sys/epoll.h>

EventHandler::EventHandler(int descriptor)
: descriptor(descriptor)
, event_mask(EPOLLIN | EPOLLOUT | EPOLLRDHUP | EPOLLPRI | EPOLLERR | EPOLLHUP | EPOLLET)
{
  // EPOLLONESHOT | EPOLLWAKEUP| EPOLLEXCLUSIVE
}

EventHandler::~EventHandler()
{
}
