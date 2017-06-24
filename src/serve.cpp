#include <cinttypes>
#include <fcntl.h>
#include "syscall.h"
#include "EventLoop.h"
#include "Listener.h"
#include "fail.h"
#include "Pool.h"
#include "Timer.h"
#include "Signals.h"

const uint16_t port = 8080;

const char str[] = "Hello, World!\n";

char buffer[1000000];


uint num_cores()
{
  const char* path = "/sys/devices/system/cpu/online";
  const int result_open = syscall(SYS_open, path, O_RDONLY|O_CLOEXEC);
  if(result_open < 0) {
    fail(-result_open, "Could not open file");
  }
  const int file = result_open;

  char buffer[512];
  const int result_read = syscall(SYS_read, file, buffer, sizeof(buffer));
  if(result_read < 0 || result_read >= sizeof(buffer)) {
    fail(-result_read, "Could not read file");
  }

  const int result_close = syscall(SYS_close, file);
  if(result_close < 0) {
    fail(-result_close, "Could not close file");
  }

  // Parse format [0-9]+ '-' [0-9]+
  uint start = -1;
  uint end = 0;
  for(uint i = 0; i < result_read; ++i) {
    const char c = buffer[i];
    if(c >= '0' && c <= '9') {
      end *= 10;
      end += c - '0';
    } else if (c == '-' && start == -1) {
      start = end;
      end = 0;
    } else if (c == '\n' && start != -1 && i == result_read - 1) {
      break;
    } else {
      fail(0, "Unexpected character");
    }
  }

  return end - start + 1;
}

extern "C" void _start ()
{
  // Fetch number of cores
  const uint cores = num_cores();

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
