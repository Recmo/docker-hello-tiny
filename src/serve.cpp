#include <cinttypes>
#include <fcntl.h>
#include "syscall.h"
#include "EventLoop.h"
#include "Listener.h"
#include "fail.h"
#include "Pool.h"
#include "Timer.h"
#include "Signals.h"
#include "Thread.h"
#include "Response.h"

const uint16_t port = 8080;

const char str[] = "Hello, World!\n";

char buffer[1000000];


int thread(void* arg)
{
  syscall(SYS_write, 2, arg, 20);
  return 1337;
}

extern "C" void _start ()
{
  Response::initialize();

  // Fetch number of cores
  const uint cores = Thread::num_cores();

  // Allocate memory (todo: mmap?)
  uint64_t brk = syscall(SYS_brk, 0);
  brk = syscall(SYS_brk, brk + cores * 1024);
  // TODO allocate pools using SYS_brk
  Pool<EventLoop, 100> loops;

  // TODO: seccomp-bpf

  Signals signals;
  Timer timer;

  // Open port
  Listener server{port};

  EventLoop loop;
  loop.add_handler(&signals);
  loop.add_handler(&timer);
  loop.add_handler(&server);
  timer.start(25000000000ULL);
  loop.run();

  syscall(SYS_write, 1, str, sizeof(str) - 1);
  syscall(SYS_exit, 0);
}
