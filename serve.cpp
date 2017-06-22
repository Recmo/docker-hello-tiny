#include <cinttypes>
#include "syscall.h"

const char str[] = "Hello, World!\n";

extern "C" void _start ()
{
  syscall<uint64_t>(SYS_write, 1, str, sizeof(str) - 1);
  syscall<uint64_t>(SYS_exit, 0);
}
