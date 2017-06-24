#include "Listener.h"
#include "EventLoop.h"
#include "Socket.h"
#include "syscall.h"
#include "fail.h"
#include <sys/epoll.h> // EPOLLIN, EPOLLONESHOT, ...
#include <sys/socket.h> // sockaddr, SOCK_NONBLOCK, ..
#include <sys/errno.h> // EAGAIN, EWOULDBLOCK, ..
#include <linux/in.h> // sockaddr_in, IPPROTO_TCP, ..

Listener::Listener(uint16_t port)
: EventHandler{create(port)}
{
  // Set one-shot level-triggered
  event_mask = EPOLLIN | EPOLLONESHOT;
}

Listener::~Listener()
{
  syscall(SYS_close, descriptor);
}

void Listener::handle(EventLoop& el, uint32_t events)
{
  if(events != EPOLLIN) {
    fail(0, "Unexpected event from socket");
  }

  // Accept new incoming connection
  sockaddr address;
  socklen_t length;
  const int flags = SOCK_NONBLOCK | SOCK_CLOEXEC;
  int result = syscall(SYS_accept4, descriptor, &address, &length, flags);
  if(result < 0) {
    // TODO: Errors on the new socket are also returned here.
    if(result == -EAGAIN || result == -EWOULDBLOCK) {
      // Some other thread has already accepted the socket
      // TODO Warning: indicates thundering herd
      return;
    }
    fail(-result, "Error accepting socket");
  }
  const int socket_descriptor = result;

  // TODO interact with socket.
  syscall(SYS_write, socket_descriptor, "Hello!\n", 7);
  syscall(SYS_close, socket_descriptor);

  // syscall(SYS_sendfile, socket_descriptor);

  // Re-arm the accept
  // TODO: EPOLLEXCLUSIVE in Linux 4.5.0
  el.update_handler(this);

  // Create Socket object
  // sockets.emplace_back(descriptor);
  // TODO: ERR: This may relocate Socker objects and invalidate the epoll data.ptr!

  // TODO: Repeat untill EAGAIN or EWOULDBLOCK
  // TODO: Or even beter: register ourselves as level-triggered
}

int Listener::create(uint16_t port)
{
  // Create socket of suitable type
  const int family = PF_INET;
  const int type = SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC;
  const int protocol = IPPROTO_TCP;
  const int socket_fd = syscall(SYS_socket, family, type, protocol);
  if(socket_fd < 0) {
    fail(-socket_fd, "Could not create socket");
  }

  // Allow binding recently bound addresses that where not closed properly
  // See: http://hea-www.harvard.edu/~fine/Tech/addrinuse.html
  const int level = SOL_SOCKET;
  const int optname = SO_REUSEADDR;
  const int optval = 1;
  const int opt_result = syscall(SYS_setsockopt, socket_fd, level, optname,
    &optval, sizeof(optval));
  if(opt_result < 0) {
    fail(-opt_result, "Error setting socket options");
  }

  // Bind to address
  const uint16_t nport = (port >> 8) | (port << 8);
  const sockaddr_in address{AF_INET, nport, {0}};
  const int bind_result = syscall(SYS_bind, socket_fd, &address, sizeof(address));
  if(bind_result < 0) {
    fail(-bind_result, "Error binding socket");
  }

  // Start listening
  const int backlog = 1; // TODO SOMAXCONN;
  const int listen_result = syscall(SYS_listen, socket_fd, backlog);
  if(listen_result < 0) {
    fail(-listen_result, "Error listening on socket");
  }

  // Return the bound and listening socket
  return socket_fd;
}
