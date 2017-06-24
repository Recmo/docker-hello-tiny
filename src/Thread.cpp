#include "Thread.h"
#include "syscall.h"
#include "fail.h"
#include <fcntl.h> // For O_RDONLY, O_CLOEXEC
#include <sched.h> // For CLONE_*
#include <sys/mman.h> // For PROT_*

int64_t Thread::clone(int (*func)(void *), void* arg)
{
  // Allocate stack
  const uint64_t stack_size = 1048576;
  const int prot = PROT_READ | PROT_WRITE;
  const int mmap_flags = MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK | MAP_GROWSDOWN;
  const int64_t result_mmap = syscall(SYS_mmap, nullptr, stack_size, prot, mmap_flags);
  if(result_mmap < 0) {
    fail(-result_mmap, "Could not guard top of stack");
  }
  const uint64_t stack_top = static_cast<uint64_t>(result_mmap);
  const uint64_t stack_base = stack_top + stack_size;

  // Store argument in register
  register void* arg_reg = arg;

  // Clone the thread
  const uint64_t flags = CLONE_FILES | CLONE_FS | CLONE_IO | CLONE_PARENT_SETTID | CLONE_SIGHAND | CLONE_SYSVSEM | CLONE_THREAD | CLONE_VM;
  int parent_tid;
  int child_tid;
  const uint64_t tld = 0;
  const int64_t clone_result = syscall(SYS_clone, flags, stack_base, &parent_tid, &child_tid, tld);
  if(clone_result < 0) {
    fail(-clone_result, "Could not clone thread");
  }
  // NOTE: ESP, the stackpointer, has been maninpulated and the compiler does
  //       not know about it. Expect dragons.
  if(clone_result == 0) {
    // TODO: This only works if arg is passed as a register (can't trust the stack)
    const int exit_code = func(arg_reg);
    syscall(SYS_exit, exit_code);
    // TODO Have parent unmap stack.
  }
  // NOTE: At this point, we know we have our original stack. It is safe
  //       to return.
  return clone_result;
}

uint64_t Thread::num_cores()
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
  uint64_t start = -1;
  uint64_t end = 0;
  for(uint64_t i = 0; i < result_read; ++i) {
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
