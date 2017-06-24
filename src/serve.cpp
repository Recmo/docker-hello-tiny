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
  // Fetch number of cores
  const uint cores = Thread::num_cores();

  Thread::clone(thread, (void*)"Hello from thread 1\n");
  Thread::clone(thread, (void*)"Hello from thread 2\n");
  Thread::clone(thread, (void*)"Hello from thread 3\n");
  Thread::clone(thread, (void*)"Hello from thread 4\n");

  syscall(SYS_write, 2, "Hello from parent!\n", 19);
  syscall(SYS_exit, 0);


  // Allocate memory
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
  timer.start(2500000000ULL);
  loop.run();

  syscall<uint64_t>(SYS_write, 1, str, sizeof(str) - 1);
  syscall<uint64_t>(SYS_exit, 0);
}
