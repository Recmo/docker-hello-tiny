#pragma once
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <system_error>

class AddressIterator {
public:
  class addrinfo_error_category: public std::error_category {
  public:
    virtual const char* name() const noexcept final;
    virtual std::string message(int code) const final;
  };
  struct iterator {
    iterator(addrinfo* pos): pos(pos) { };
    addrinfo* pos;
    bool operator!=(iterator& other) const { return pos != other.pos; }
    iterator& operator++() { pos = pos->ai_next; return *this; }
    const addrinfo& operator*() const { return *pos; }
  };
  static addrinfo_error_category addrinfo_error;

  AddressIterator(const std::string& address);
  ~AddressIterator();

  iterator begin() const { return iterator{addr_list}; }
  iterator end() const { return iterator{nullptr}; }

protected:
  static addrinfo* create(const std::string& address);
  addrinfo* const addr_list;
};
