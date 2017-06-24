#include "Socket.h"
#include "EventLoop.h"
#include "HttpRequestParser.h"
#include "syscall.h"
#include "fail.h"
#include <sys/epoll.h> // EPOLLIN, EPOLLONESHOT, ...
#include <sys/errno.h> // EAGAIN, EWOULDBLOCK, ..
#include <linux/in.h> // sockaddr_in, IPPROTO_TCP, ..
#include <sys/stat.h> // stat
#include <fcntl.h> // O_CLOEXEC

Socket::Socket(EventLoop& el, int descriptor)
: EventHandler{descriptor}
, state{0}
{
  // TODO: We are not interested in the EPOLLOUT before we read the header.
  event_mask = EPOLLIN | EPOLLHUP | EPOLLRDHUP;

  // Try handling immediately!
  handle(el, EPOLLIN | EPOLLOUT);
  added = true;
}

Socket::~Socket()
{
}

void Socket::handle(EventLoop& el, uint32_t events)
{
  // Was the connection closed?
  if(events & EPOLLHUP || events & EPOLLRDHUP) {
    // Close already deregisters us, like el.remove_handler(this);
    const int result = syscall(SYS_close, descriptor);
    if(result < 0) {
      fail(-result, "Error closing socket");
    }
    // TODO delete this;
    closed = true;
    return;
  }

  // Read some
  if(state == 0 && events & EPOLLIN) {
    char buffer[10240];
    const int result = syscall(SYS_read, descriptor, buffer, sizeof(buffer) - 16);
    if(result == -EAGAIN) {
      return;
    }
    if(result < 0) {
      fail(-result, "Error reading socket");
    }
    const uint size = result;
    HttpRequestParser parser;
    const Response& response = parser.read(buffer, size);
    offset = response.offset;
    length = response.length;
    state = 1;

    // Start listening for EPOLLOUT
    event_mask = EPOLLOUT | EPOLLHUP | EPOLLRDHUP;
    if(added) {
      el.update_handler(this);
    }
  }

  // Write some
  if(state == 1 && events & EPOLLOUT) {
    const ssize_t result = syscall(SYS_sendfile, descriptor, Response::descriptor, &offset, length);
    if(result < 0) {
      fail(-result, "Error sending file");
    }
    length -= result;
    if(length == 0) {
      reqs++;
      state = 2;
    }
  }

  // Close when done
  // TODO HTTP keep-alive
  if(state == 2) {
    const int result = syscall(SYS_close, descriptor);
    if(result < 0) {
      fail(-result, "Error closing socket");
    }
    closed = true;
  }

  // TODO: Timeouts
  // TODO: Max header sizes
}
