#pragma once
#include <cinttypes>

class Response {
public:
  static void initialize();

  static int descriptor;
  static uint64_t etag_offset;
  static uint64_t etag_length;
  static Response not_modified;
  static Response bad_request;
  static Response not_found;
  static Response method_not_allowed;
  static Response request_timeout;
  static Response uri_to_long;
  static Response request_to_large;
  static Response internal_server_error;
  static Response not_implemented;
  static Response http_version_not_supported;

  static const Response& get(uint64_t hash) {
    return not_found;
  }

public:
  uint64_t offset;
  uint64_t length;
};
