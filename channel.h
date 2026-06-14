#pragma once

#include <memory>
#include <functional>
#include <sys/epoll.h>

#include "noncopyable.h"
#include "timestamp.h"
#include "poller.h"

namespace webserver {

class eventloop;

// fd的封装，记录了fd以及在fd上监听的事件
class channel : public noncopyable {
public:
  using event_callback = std::function<void()>;
  using read_callback = std::function<void(timestamp)>;
  enum class status {
    UNREGISTERED,
    REGISTERED,
  };

  channel(poller* poller, int fd);
  ~channel() = default;

  void handle_event(timestamp receive_time);
  void tie(const std::shared_ptr<void> &tie);
  void remove();

  enum status status() const { return status_; }
  void set_status(enum status s) { status_ = s; }

  int revents() const { return revents_; }
  void set_revents(int revents) { revents_ = revents; }
  
  void set_read_callback(read_callback cb) { read_callback_ = std::move(cb); }
  void set_write_callback(event_callback cb) { write_callback_ = std::move(cb); }
  void set_error_callback(event_callback cb) { error_callback_ = std::move(cb); }
  void set_close_callback(event_callback cb) { close_callback_ = std::move(cb); }
  
  int fd() const { return fd_; }

  uint32_t events() const { return events_; }
  void set_events(int events) { events_ = events; update(); }
  void enable_reading() { events_ |= READ_EVENT; update(); }
  void disable_reading() { events_ &= ~READ_EVENT; update(); }
  void enable_writing() { events_ |= WRITE_EVENT; update(); }
  void disable_writing() { events_ &= ~WRITE_EVENT; update(); }
  void disable_all() { events_ = NONE_EVENT; update(); }

  bool is_listening_read_event() const { return events_ & READ_EVENT; }
  bool is_listening_write_event() const { return events_ & WRITE_EVENT; }
  bool is_listening_none() const { return events_ == NONE_EVENT; }
private:
  constexpr static int READ_EVENT = EPOLLIN | EPOLLRDHUP;
  constexpr static int WRITE_EVENT = EPOLLOUT;
  constexpr static int NONE_EVENT = 0;

  void update();

  void handle_event_with_guard(timestamp receive_time);
  
  const int fd_;

  // 当channel是acceptor中的监听新连接channel时，poller_ == nullptr
  poller* poller_;
  
  uint32_t events_;
  int revents_;
  enum status status_;
  bool tied_;
  std::weak_ptr<void> tie_;

  read_callback read_callback_;
  event_callback write_callback_;
  event_callback error_callback_;
  event_callback close_callback_;
};  

}  // namespace webserver
