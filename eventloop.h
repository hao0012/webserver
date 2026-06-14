#pragma once

#include "noncopyable.h"

#include <functional>
#include <atomic>
#include <mutex>
#include <memory>
#include <thread>
#include <vector>

#include "channel.h"
#include "epoller.h"
#include "timestamp.h"
#include "tcp_connection.h"

namespace webserver {

class eventloop : public noncopyable {
public:
  eventloop(std::thread::id thread_id);
  ~eventloop();

  void loop();
  void quit();

  void run_in_loop(std::function<void()> cb);
  void queue_in_loop(std::function<void()> cb);

  void add_connection(std::shared_ptr<tcp_connection> conn);
  void remove_connection(tcp_connection::id id);

  // 唤醒eventloop所在线程
  void wakeup();

  bool is_in_loop_thread() const { return thread_id_ == std::this_thread::get_id(); }

  timestamp poll_return_time() const { return poll_return_time_; }
  poller &get_poller() { return poller_; }
private:
  static constexpr size_t POLL_TIMEOUT_MS = 1000;

  // 执行上层的回调
  void run_pending_callbacks();

  std::atomic<bool> quit_;

  const std::thread::id thread_id_;
  timestamp poll_return_time_;
  epoller poller_;

  // 避免多次调用wakeup，调用一次就够了
  std::atomic<bool> pending_wakeup_;
  
  int wakeup_fd_;
  channel wakeup_channel_;

  std::atomic<bool> calling_pending_callbacks_;
  std::vector<std::function<void()>> pending_callbacks_;
  std::mutex pending_callbacks_mutex_;

  using ConnectionMap = std::unordered_map<tcp_connection::id, std::shared_ptr<tcp_connection>>;
  ConnectionMap connections_;
};

}
