#pragma once

#include <memory>
#include <atomic>

#include "common_type.h"
#include "inet_address.h"
#include "noncopyable.h"
#include "socket.h"
#include "channel.h"
#include "buffer.h"

namespace webserver {

class eventloop;

class tcp_connection : noncopyable, public std::enable_shared_from_this<tcp_connection> {
public:
  using id = size_t;
  enum class state {
    DISCONNECTED = 0,
    CONNECTING = 1,
    DISCONNECTING = 2,
    CONNECTED = 3,
  };


  explicit tcp_connection(
    eventloop &loop,
    int sock_fd, 
    const inet_address& local_addr, 
    const inet_address& peer_addr);
  ~tcp_connection();

  void send(const std::string& data);
  void send_file(int fd, off_t offset, size_t size);
  void shutdown();

  eventloop& get_loop() const { return loop_; }
  id get_id() const { return id_; }
  const inet_address& local_addr() const { return local_addr_; }
  const inet_address& peer_addr() const { return peer_addr_; }
  const state get_state() const { return state_.load(); }

  bool is_connected() const { return state_ == state::CONNECTED; }

  void set_connect_callback(connection_callback cb) { connection_callback_ = cb; }
  void set_message_callback(message_callback cb) { message_callback_ = cb; }
  void set_write_complete_callback(write_complete_callback cb) { write_complete_callback_ = cb; }
  void set_high_water_mark_callback(const high_water_mark_callback& cb, size_t high_water_mark) { 
    high_water_mark_callback_ = cb;
    high_water_mark_ = high_water_mark;
  }

  void establish_connection();
  void destroy_connection();
private:

  static id next_id();

  void connection_close_callback();

  void handle_read(timestamp receive_time);
  void handle_write();
  void handle_close();
  void handle_error();

  void send_in_loop(const void *data, size_t size);
  void shutdown_in_loop();
  void send_file_in_loop(int fd, off_t offset, size_t size);

  const id id_;
  std::atomic<state> state_;
  eventloop& loop_;
  
  // 是否在监听读事件
  bool is_listening_;

  socket socket_;
  channel channel_;

  const inet_address local_addr_;
  const inet_address peer_addr_;

  connection_callback connection_callback_;
  message_callback message_callback_;
  write_complete_callback write_complete_callback_;
  high_water_mark_callback high_water_mark_callback_;
  size_t high_water_mark_;

  buffer input_buffer_;
  buffer output_buffer_;

};

} // namespace webserver