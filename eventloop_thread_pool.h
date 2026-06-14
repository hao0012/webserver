#pragma once

#include <vector>
#include <string>

#include "noncopyable.h"
#include "eventloop_thread.h"

namespace webserver {

class eventloop_thread_pool: noncopyable {
public:
  eventloop_thread_pool(eventloop &loop, const std::string &name);
  ~eventloop_thread_pool() = default;

  void set_thread_num(int num) { thread_num_ = num; }
  void start();

  eventloop &get_next_eventloop();

  void set_user_thread_init_callback(thread_init_callback cb) { user_thread_init_callback_ = std::move(cb); }

  bool started() const { return started_; }
  const std::string name() const { return name_; }
private:
  // tcp_server中的主eventloop
  eventloop &main_loop_;
  std::string name_;
  bool started_;
  int next_;
  // 各子线程，其中含有各自的eventloop
  int thread_num_;
  // 用户自定义的线程初始化回调
  thread_init_callback user_thread_init_callback_;
  std::vector<std::unique_ptr<eventloop_thread>> threads_;
};

}