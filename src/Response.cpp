#include "Response.h"
#include "syscall.h"
#include "fail.h"
#include <sys/stat.h> // stat
#include <fcntl.h> // O_CLOEXEC

int Response::descriptor;
uint64_t Response::etag_offset = 57;
uint64_t Response::etag_length = 16;
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

void Response::initialize()
{
  // Open as file descriptor for sendfile
  const int result = syscall(SYS_open, "bundle", O_RDONLY | O_CLOEXEC | O_NONBLOCK, 0);
  if(result < 0) {
    fail(-result, "Error while waiting for events");
  }
  descriptor = result;

  // Get size
  struct stat buf;
  int result_stat = syscall(SYS_fstat, descriptor, &buf);
  if(result_stat < 0) {
    fail(-result_stat, "Error stat-ing bundle");
  }

  // TODO
  not_found.offset = 0;
  not_found.length = buf.st_size;
}
