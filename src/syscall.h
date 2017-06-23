#pragma once
#include <cinttypes>
#include <sys/syscall.h>

// TODO: These are somehow missing from syscall.h
// See: https://filippo.io/linux-syscall-table/
#define __NR_socket 41
#define __NR_setsockopt 54

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

template<typename R = uint64_t, typename A, typename B>
inline R syscall(uint64_t id, A a, B b)
{
  R ret;
  asm volatile(
    "syscall"
    : "=a"(ret)
    : "a"(id),
      "D"(a),
      "S"(b)
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

template<typename R = uint64_t, typename A, typename B, typename C, typename D>
inline R syscall(uint64_t id, A a, B b, C c, D d)
{
  R ret;
  register D r10 asm("r10") = d;
  asm volatile(
    "syscall"
    : "=a"(ret)
    : "a"(id),
      "D"(a),
      "S"(b),
      "d"(c),
      "r"(r10)
    : "memory", "%rdi", "%rsi", "%rdx", "%rcx", "%r8", "%r9", "r10", "%r11"
  );
  return ret;
}

template<typename R = uint64_t, typename A, typename B, typename C, typename D, typename E>
inline R syscall(uint64_t id, A a, B b, C c, D d, E e)
{
  R ret;
  register D r10 asm("r10") = d;
  register E r8 asm("r8") = e;
  asm volatile(
    "syscall"
    : "=a"(ret)
    : "a"(id),
      "D"(a),
      "S"(b),
      "d"(c),
      "r"(r10),
      "r"(r8)
    : "memory", "%rdi", "%rsi", "%rdx", "%rcx", "%r8", "%r9", "r10", "%r11"
  );
  return ret;
}
