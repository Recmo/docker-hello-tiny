#pragma once
#include <cstdint>

class Thread {
public:
  static void clone(int (*func)(void *), void* arg);
  static uint64_t num_cores();
};
