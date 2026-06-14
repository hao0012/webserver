#pragma once

#include <functional>
#include <string>

#include "common_type.h"
#include "inet_address.h"
#include "eventloop.h"
#include "eventloop_thread_pool.h"
#include "acceptor.h"

namespace webserver {

class tcp_server {
public:
  using thread_init_callback = std::function<void()>;

  enum class option {
    NO_REUSE_PORT,
    REUSE_PORT,
  };

  tcp_server(const inet_address &addr, const std::string &name, option opt = option::NO_REUSE_PORT);
  ~tcp_server() = default;

  void start();

  void set_thread_init_callback(thread_init_callback cb);
  void set_connection_callback(connection_callback cb);
  void set_message_callback(message_callback cb);
  void set_write_complete_callback(write_complete_callback cb);
  void set_thread_num(int num);
private:
  eventloop main_loop_;
  eventloop_thread_pool thread_pool_;
  acceptor acceptor_;

  std::atomic<bool> started_;
};

} // namespace webserver