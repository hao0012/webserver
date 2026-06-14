#pragma once


#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <cstring>

#include "noncopyable.h"
#include "logger.h"
#include "inet_address.h"

namespace webserver {

class inet_address;

class socket : noncopyable {
public:
  explicit socket(int fd) : fd_(fd) {}
  ~socket() { ::close(fd_); }
  int fd() const { return fd_; }

  void bind_address(const inet_address& addr) {
    int ret = ::bind(fd_, reinterpret_cast<const sockaddr*>(&addr.sock_addr()), sizeof(addr.sock_addr()));
    logger::debug("bind_address fd %d, ret %d", fd_, ret);
  }

  int listen(uint32_t backlog) {
    int ret = ::listen(fd_, backlog);
    if (ret != 0) {
      logger::error("listen fd %d fail", fd_);
    }
    return ret;
  }

  int accept(inet_address* addr) {
    sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    ::memset(&client_addr, 0, sizeof(client_addr));
    int conn_fd = ::accept4(fd_, reinterpret_cast<sockaddr*>(&client_addr), &client_addr_len, SOCK_NONBLOCK | SOCK_CLOEXEC);
    if (conn_fd >= 0) {
      logger::debug("accept fd %d success", fd_);
      addr->set_sock_addr(client_addr);
    }
    return conn_fd;
  }

  void shutdown_write() {
    if (::shutdown(fd_, SHUT_WR) < 0) {
      logger::error("shutdown_write fd %d fail", fd_);
    }
  }

  void set_tcp_nodelay(bool on = true) {
    int opt = on ? 1 : 0;
    if (::setsockopt(fd_, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt)) != 0) {
      logger::error("set_tcp_nodelay fd %d fail", fd_);
    }
  }
  void set_reuse_addr(bool on = true) {
    int opt = on ? 1 : 0;
    if (::setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) != 0) {
      logger::error("set_reuse_addr fd %d fail", fd_);
    }
  }
  void set_reuse_port(bool on = true) {
    int opt = on ? 1 : 0;
    if (::setsockopt(fd_, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) != 0) {
      logger::error("set_reuse_port fd %d fail", fd_);
    }
  }
  void set_keep_alive(bool on = true) {
    int opt = on ? 1 : 0;
    if (::setsockopt(fd_, SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof(opt)) != 0) {
      logger::error("set_keep_alive fd %d fail", fd_);
    }
  }
private:
  const int fd_;
};

}