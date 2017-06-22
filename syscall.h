#pragma once
#include <cinttypes>
#include <sys/syscall.h>

template<typename R = uint64_t, typename A>
inline R syscall(uint64_t id, A a)
{
  R ret;
  asm volatile(
    "syscall"
    : "=a"(ret)
    : "a"(id),
      "D"(a)
    : "memory", "%rdi", "%rsi", "%rdx", "%rcx", "%r8", "%r9", "r10", "%r11"
  );
  return ret;
}

template<typename R = uint64_t, typename A, typename B, typename C>
inline R syscall(uint64_t id, A a, B b, C c)
{
  R ret;
  asm volatile(
    "syscall"
    : "=a"(ret)
    : "a"(id),
      "D"(a),
      "S"(b),
      "d"(c)
    : "memory", "%rdi", "%rsi", "%rdx", "%rcx", "%r8", "%r9", "r10", "%r11"
  );
  return ret;
}
