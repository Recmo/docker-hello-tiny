#include "AddressIterator.h"
#include <cstring>

AddressIterator::addrinfo_error_category AddressIterator::addrinfo_error;

const char* AddressIterator::addrinfo_error_category::name() const noexcept
{
  return "getaddrinfo";
}

std::string AddressIterator::addrinfo_error_category::message(int code) const
{
  return gai_strerror(code);
}

AddressIterator::AddressIterator(const std::string& address)
: addr_list(create(address))
{
}

AddressIterator::~AddressIterator()
{
  freeaddrinfo(addr_list);
}

addrinfo* AddressIterator::create(const std::string& address)
{
  const char* node = nullptr; //address.c_str();
  const char* service = address.c_str();
  addrinfo* list;

  // Configure hints
  addrinfo hints;
  std::memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC; // Allow IPv4 and IPv6
  hints.ai_socktype = SOCK_STREAM; // Stream (tcp) sockets
  hints.ai_flags = AI_PASSIVE; // Allow wildcard IP address

  const int result = getaddrinfo(node, service, &hints, &list);
  if(result != 0) {
    if(result == EAI_SYSTEM) {
      throw std::system_error{
        std::error_code{errno, std::system_category()},
        "Error getting addresses"
      };
    } else {
      throw std::system_error{
        std::error_code{result, addrinfo_error},
        "Error getting addresses"
      };
    }
  }
  return list;
}
