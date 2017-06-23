#include <cstdlib>
#include "fail.h"
#include "syscall.h"

void print_hex(uint64_t code)
{
  uint8_t buffer[20];
  buffer[0] = '0';
  buffer[1] = 'x';
  buffer[18] = ':';
  buffer[19] = ' ';
  for(uint64_t i = 0; i < 8; ++i) {
    const uint8_t hex[] = "0123456789abcdef";
    const uint8_t byte = code >> 56;
    buffer[(2 * i) + 2] = hex[byte >> 4];
    buffer[(2 * i) + 3] = hex[byte & 15];
    code <<= 8;
  }
  syscall(SYS_write, 2, buffer, sizeof(buffer));
}

void fail(uint64_t code, const char* message)
{
  print_hex(code);
  uint64_t l = 0;
  while(message[l] != 0x00) {
    l++;
  }
  syscall(SYS_write, 2, message, l);
  syscall(SYS_write, 2, "\n", 1);
  syscall(SYS_exit, EXIT_FAILURE);
}
