#pragma once
#include "EventHandler.h"
#include "Socket.h"
#include "Pool.h"
#include <cinttypes>

class Listener: public EventHandler {
public:
  Listener(uint16_t port);
  ~Listener();
  virtual void handle(EventLoop& el, uint32_t events) final;

protected:
  int filefd;
  uint64_t size;
  static int create(uint16_t port);

  Pool<Socket, 100> socket_pool;
};
