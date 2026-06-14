#pragma once

#include <thread>
#include "eventloop.h"

namespace webserver {

class eventloop_thread {
public:

  eventloop_thread(thread_init_callback user_thread_init_callback, const std::string &name = "");
  ~eventloop_thread();

  eventloop &get_loop() { return loop_; }

private:
  void thread_task();

  std::string name_;

  // eventloop先析构，在析构函数中会调用run_in_loop，此时thread还未析构
  std::thread thread_;
  eventloop loop_;
};

}