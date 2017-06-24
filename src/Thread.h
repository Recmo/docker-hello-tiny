#pragma once
#include <cstdint>

class Thread {
public:
  static int64_t clone(int (*func)(void *), void* arg);
  static uint64_t num_cores();
};
