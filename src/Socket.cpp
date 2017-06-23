#include "Socket.h"
#include "EventLoop.h"
#include "HttpRequestParser.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/epoll.h>
#include <sys/syscall.h>
#include <system_error>
#include <cerrno>
#include <sys/sendfile.h>
#include <unistd.h>
#include <iostream>

// Own implementation of read that foregoes pthread_cancel handling
inline long read_nocancel(int fd, void *buf, size_t count)
{
  return syscall(SYS_read, fd, buf, count);
}

inline long close_nocancel(int fd)
{
  return syscall(SYS_close, fd);
}

Socket::Socket(int descriptor)
: EventHandler{descriptor}
{
}

Socket::~Socket()
{
}

void Socket::handle(EventLoop& el, uint32_t events)
{
  // Was the connection closed?
  if(events & EPOLLHUP || events & EPOLLRDHUP) {
    // Close already deregisters us, like el.remove_handler(this);
    const int result = close_nocancel(descriptor);
    if(result < 0) {
      throw std::system_error{
        std::error_code{errno, std::system_category()},
        "Error closing socket"
      };
    }
    delete this;
    return;
  }

  // Read some
  if(state == 0 && events & EPOLLIN) {
    char buffer[10240];
    const int result = read_nocancel(descriptor, buffer, sizeof(buffer) - 16);
    if(result < 0) {
      throw std::system_error{
        std::error_code{errno, std::system_category()},
        "Error reading socket"
      };
    }
    const uint size = result;
    HttpRequestParser parser;
    const Response& response = parser.read(buffer, size);
    offset = response.offset;
    length = response.length;
    state = 1;
  }

  // Write some
  if(state == 1 && events & EPOLLOUT) {
    const ssize_t result = sendfile64(
      descriptor, Response::descriptor, &offset, length);
    if(result < 0) {
      throw std::system_error{
        std::error_code{errno, std::system_category()},
        "Error sending file"
      };
    }
    length -= result;
    if(length == 0) {
      reqs++;
      state = 0;
    }
  }

  // TODO: Timeouts
  // TODO: Max header sizes
  // TODO: Close
}
