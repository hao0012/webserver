#include "eventloop_thread_pool.h"
#include <memory>

namespace webserver {

eventloop_thread_pool::eventloop_thread_pool(eventloop &loop, const std::string &name)
  : main_loop_(loop), name_(name), started_(false), thread_num_(0), next_(0) {
}

void eventloop_thread_pool::start() {
  started_ = true;  
  for (int i = 0; i < thread_num_; ++i) {
    std::string name = name_ + "-" + std::to_string(i);
    threads_.emplace_back(std::make_unique<eventloop_thread>(user_thread_init_callback_, name));
  }

  if (thread_num_ == 0 && user_thread_init_callback_) {
    user_thread_init_callback_();
  }
}

eventloop &eventloop_thread_pool::get_next_eventloop() {
  eventloop *loop = &main_loop_;
  if (!threads_.empty()) {
    loop = &threads_[next_]->get_loop();
    next_ = (next_ + 1) % threads_.size();
  }
  return *loop;
}

}