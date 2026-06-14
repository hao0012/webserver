#pragma once

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <string>
#include <cstring>
#include "logger.h"

namespace webserver {

class inet_address {
public:
  explicit inet_address(size_t port = 0, std::string ip = "127.0.0.1") {
    ::memset(&addr_in_, 0, sizeof(addr_in_));
    addr_in_.sin_family = AF_INET;
    addr_in_.sin_addr.s_addr = ::inet_addr(ip.c_str());
    addr_in_.sin_port = ::htons(port);
  }

  explicit inet_address(const sockaddr_in& addr_in): addr_in_(addr_in) {}
  
  explicit inet_address(int fd) {
    ::memset(&addr_in_, 0, sizeof(addr_in_));
    socklen_t addr_len = sizeof(addr_in_);
    
    if (::getsockname(fd, reinterpret_cast<sockaddr *>(&addr_in_), &addr_len) < 0) {
      logger::error("getsockname failed");
    }
  }
  
  ~inet_address() = default;

  std::string to_ip() const {
    char buf[64] = {0};
    ::inet_ntop(AF_INET, &addr_in_.sin_addr, buf, sizeof(buf));
    return buf;
  }
  
  std::string to_ip_port() const {
    char buf[64] = {0};
    ::inet_ntop(AF_INET, &addr_in_.sin_addr, buf, sizeof(buf));
    size_t end = ::strlen(buf);
    uint16_t port = ::ntohs(addr_in_.sin_port);
    snprintf(buf + end, sizeof(buf) - end, ":%d", port);
    return buf;
  }
  
  size_t to_port() const {
    return ::ntohs(addr_in_.sin_port); 
  }

  const sockaddr_in& sock_addr() const {
    return addr_in_;
  }

  void set_sock_addr(const sockaddr_in& addr_in) {
    addr_in_ = addr_in;
  }

private:
  sockaddr_in addr_in_;
};

}