#include <cinttypes>
#include "syscall.h"
#include "EventLoop.h"
#include "Listener.h"
#include "fail.h"

const char str[] = "Hello, World!\n";

char buffer[1000000];

extern "C" void _start ()
{
  EventLoop loop;
  loop.timer.start(2500000000ULL);

  Listener server{8080};
  loop.add_handler(&server);
  loop.run();

  syscall<uint64_t>(SYS_write, 1, str, sizeof(str) - 1);
  syscall<uint64_t>(SYS_exit, 0);
}
