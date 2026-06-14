#pragma once

#include "noncopyable.h"
#include "eventloop.h"
#include "eventloop_thread_pool.h"
#include "common_type.h"
#include "socket.h"
#include "channel.h"
#include "inet_address.h"

namespace webserver {

// server端的socket3部曲：
// 1. socket
// 2. bind
// 3. listen
// 4. epoll ctl add

// client端连接3部曲：
// 1. socket
// 2. connect
// 3. send/recv

class acceptor : noncopyable {
public:
  using new_connect_callback = std::function<void(int fd, const inet_address& addr)>;

  acceptor(eventloop &loop, eventloop_thread_pool &thread_pool, const inet_address& addr, bool reuse_port = false);
  ~acceptor();

  void set_user_connect_callback(connection_callback cb) { user_connect_callback_ = std::move(cb); }
  void set_user_message_callback(message_callback cb) { user_message_callback_ = std::move(cb); }
  void set_user_write_complete_callback(write_complete_callback cb) { user_write_complete_callback_ = std::move(cb); }

  void listen(size_t backlog);
private:
  

  socket accept_socket_;
  channel accept_channel_;

  new_connect_callback connect_callback_;

  connection_callback user_connect_callback_;
  message_callback user_message_callback_;
  write_complete_callback user_write_complete_callback_;
  
  // thread_pool由tcp_server管理生命周期，这里只是引用
  eventloop_thread_pool &thread_pool_;
};

}