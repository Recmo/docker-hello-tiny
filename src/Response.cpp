#include "Response.h"
#include "HttpRequestParser.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <unistd.h>
#include <system_error>
#include <cerrno>
#include <string>
#include <iostream>

// NOTE This set-up process makes no attempt at being fast.
//      This doesn't affect run-time performance.

// NOTE This is implementation is crap, but C++ is terrible at
//      file/process management.

std::string Response::tempfile;
int Response::descriptor;
uint Response::etag_offset = 57;
uint Response::etag_length = 16;
Response Response::not_modified;
Response Response::bad_request;
Response Response::not_found;
Response Response::method_not_allowed;
Response Response::request_timeout;
Response Response::uri_to_long;
Response Response::request_to_large;
Response Response::internal_server_error;
Response Response::not_implemented;
Response Response::http_version_not_supported;
std::unordered_map<uint64_t, Response> Response::responses;

void Response::initialize(
  const std::string &root, const std::string& cache_control)
{
  std::cerr << "Preparing canned responses";
  if(has_zopfli()) {
    std::cerr << " using zopfli";
  } else if(has_gzip()) {
    std::cerr << " using gzip";
  }
  std::cerr << ":\n";

  // Prepare canned responses
  const std::string tempfile{std::tmpnam(nullptr)};
  std::ofstream out{tempfile, std::ios::binary | std::ios::trunc};
  not_modified               = status_response(out, "304");
  bad_request                = status_response(out, "400");
  not_found                  = status_response(out, "404");
  method_not_allowed         = status_response_405(out);
  request_timeout            = status_response(out, "408");
  uri_to_long                = status_response(out, "414");
  request_to_large           = status_response(out, "431");
  internal_server_error      = status_response(out, "500");
  not_implemented            = status_response(out, "501");
  http_version_not_supported = status_response(out, "505");
  responses                  = path_responses(out, root, cache_control);
  out.close();

  // Open as file descriptor for sendfile
  const int result = open(tempfile.c_str(), O_RDONLY | O_CLOEXEC | O_NONBLOCK);
  if(result < 0) {
    std::remove(tempfile.c_str());
    throw std::system_error{
      std::error_code{errno, std::system_category()},
      "Error while waiting for events"
    };
  }
  descriptor = result;

  // Remove tempfile (but keep descriptor)
  std::remove(tempfile.c_str());
}

void Response::write_file(const std::string& filename, const std::string& data)
{
  std::ofstream{filename, std::ios::binary | std::ios::trunc} << data;
}

std::string Response::read_file(const std::string& filename)
{
  std::ifstream stream{filename, std::ios::binary};
  return std::string{
    std::istreambuf_iterator<char>{stream},
    std::istreambuf_iterator<char>{}};
}

std::string Response::pipe_through(const std::string& command, const std::string& data)
{
  const std::string in{std::tmpnam(nullptr)};
  const std::string out{std::tmpnam(nullptr)};
  const std::string cmd = command + " < " + in + " > " + out;
  write_file(in, data);
  const int result = std::system(cmd.c_str());
  if(result != 0) {
    std::remove(in.c_str());
    std::remove(out.c_str());
    throw std::runtime_error{
      "Error executing system command"
    };
  }
  const std::string output = read_file(out);
  std::remove(in.c_str());
  std::remove(out.c_str());
  return output;
}

bool Response::has_find()
{
  static bool cached = false;
  static bool result = false;
  if(!cached) {
    result = std::system("find --help > /dev/null") == 0;
    cached = true;
  }
  return result;
}

bool Response::has_zopfli()
{
  static bool cached = false;
  static bool result = false;
  if(!cached) {
    result = std::system("zopfli -h 2> /dev/null") == 0;
    cached = true;
  }
  return result;
}

bool Response::has_gzip()
{
  static bool cached = false;
  static bool result = false;
  if(!cached) {
    result = std::system("gzip -h > /dev/null") == 0;
    cached = true;
  }
  return result;
}

std::string Response::zopfli(const std::string& data)
{
  return pipe_through("zopfli --deflate -i1000 -c /dev/stdin", data);
}

std::string Response::gzip(const std::string& data)
{
  return pipe_through("gzip -9 -c -", data);
}

Response Response::write_response(std::ofstream& out, const std::string& data)
{
  const uint64_t offset = out.tellp();
  out << data;
  return Response{offset, data.size()};
}

Response Response::status_response(std::ofstream& out, const std::string& code)
{
  const std::string response{
    std::string{"HTTP/1.1 "} + code +
    std::string{" \r\nContent-Length:0\r\n\r\n"}
  };
  return write_response(out, response);
}

Response Response::status_response_405(std::ofstream& out)
{
  return write_response(out,
    "HTTP/1.1 405 \r\n"
    "Allow:GET\r\n"
    "Content-Length:0\r\n"
    "\r\n");
}

Response Response::file_response(std::ofstream& out,
  const std::string& filename, const std::string& cache_control)
{
  std::stringstream ss;
  ss << "HTTP/1.1 200 \r\n";
  if(!cache_control.empty()) {
    ss << "Cache-Control:" << cache_control << "\r\n";
  }
  std::string data = read_file(filename);
  ss << "ETag:\"";
  if(ss.tellp() != etag_offset) {
    throw std::runtime_error{"ETag not in expected location"};
  }
  ss << make_etag(data);
  if(ss.tellp() != etag_offset + etag_length) {
    throw std::runtime_error{"ETag not in expected location"};
  }
  ss << "\"\r\n";
  if(has_zopfli()) {
    ss << "Content-Encoding:deflate\r\n";
    data = zopfli(data);
  } else {
    ss << "Content-Encoding:gzip\r\n";
    data = gzip(data);
  }
  ss << "Content-Length:" << data.size() << "\r\n\r\n" << data;
  return write_response(out, ss.str());
}

std::unordered_map<uint64_t, Response> Response::path_responses(
  std::ofstream& out, const std::string& root, const std::string& cache_control)
{
  std::unordered_map<uint64_t, Response> responses;
  std::istringstream files{pipe_through(
    std::string{"find "} + root + std::string{" -type f"}, "")
  };
  std::string file;
  while(std::getline(files, file)) {
    if(file.size() <= root.size()) {
      throw std::runtime_error{
        "Error processing output from find"
      };
    }
    const std::string path{file, root.size()};
    const uint64_t hash = HttpRequestParser::hash(path);
    std::cerr << " * " << std::hex << hash << " " << path << "\n";
    if(responses.find(hash) != responses.end()) {
      throw std::runtime_error{"Hash collision"};
    }
    responses[hash] = file_response(out, file, cache_control);
  }
  return responses;
}

uint8_t hex_digit(uint8_t c)
{
  if(c >= '0' && c <= '9') {
    return c - '0';
  } else if(c >= 'a' && c <= 'f') {
    return c - 'a' + 10;
  } else if(c >= 'A' && c <= 'F') {
    return c - 'A' + 10;
  }
  throw std::invalid_argument{"Invalid hex digit"};
}

std::string Response::make_etag(const std::string &data)
{
  const std::string result = pipe_through("sha256sum -b", data);

  // Convert hex to binary (C++ is a disaster)
  std::string hash;
  hash.reserve(32);
  for(uint i = 0; i < 32; ++i) {
    hash.push_back((hex_digit(result[2 * i]) << 4) | hex_digit(result[2 * i + 1]));
  }

  // Take the first 128 bits and make them etag safe
  return make_etag_safe(std::string{hash, 0, 16});
}

std::string Response::make_etag_safe(std::string etag)
{
  // Valid etag chars are 0x21 / 0x23-0x7E / 0x80-0xFF
  // Invalids are 0x00—0x20, 0x22, 0x7F
  // https://tools.ietf.org/html/rfc7232#section-2.3
  for(char& c: etag) {
    if(c <= 0x20 || c == 0x22 || c == 0x7D) {
      c |= 0x80;
    }
  }
  return std::move(etag);
}
