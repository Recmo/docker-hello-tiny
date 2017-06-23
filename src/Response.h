#pragma once
#include <cinttypes>
#include <string>
#include <unordered_map>
#include <fstream>

class Response {
public:
  static void initialize(
    const std::string& root, const std::string& cache_control = std::string{});

  static std::string tempfile;
  static int descriptor;
  static uint etag_offset;
  static uint etag_length;
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
  static std::unordered_map<uint64_t, Response> responses;

  static const Response& get(uint64_t hash) {
    const auto it = responses.find(hash);
    if(it == responses.end()) {
      return not_found;
    }
    return it->second;
  }

public:
  uint64_t offset;
  uint64_t length;

private:
  static void write_file(const std::string& filename, const std::string& data);
  static std::string read_file(const std::string& filename);
  static std::string pipe_through(const std::string& command,
    const std::string& data);
  static bool has_find();
  static bool has_zopfli();
  static bool has_gzip();
  static std::string zopfli(const std::string& data);
  static std::string gzip(const std::string& data);
  static Response write_response(std::ofstream& out, const std::string& data);
  static Response status_response(std::ofstream& out, const std::string& code);
  static Response status_response_405(std::ofstream& out);
  static Response file_response(std::ofstream& out,
    const std::string& filename, const std::string& cache_control);
  static std::unordered_map<uint64_t, Response> path_responses(
    std::ofstream& out, const std::string& root,
    const std::string& cache_control);

  static std::string make_etag(const std::string& data);
  static std::string make_etag_safe(std::string etag);
};
