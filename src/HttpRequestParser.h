#pragma once
#include "Response.h"
#include <cinttypes>
#include <string>

class HttpRequestParser {
public:
  HttpRequestParser();

  static uint64_t hash(const std::string& path);

  const Response& read(const char* buffer, uint size);

  uint64_t hashv() const { return hashs[0]; }

protected:
  uint64_t hashs[2];
  uint8_t state;

  uint hash_till_space(const char* buffer, uint size);
};
