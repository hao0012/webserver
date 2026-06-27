#include "eventloop_thread.h"

namespace webserver {

eventloop_thread::eventloop_thread(thread_init_callback user_thread_init_callback, const std::string &name): 
  name_(name),
  thread_(),
  loop_(thread_.get_id()) {

  thread_ = std::thread([this, user_thread_init_callback]() {
    if (user_thread_init_callback) {
      user_thread_init_callback();
    }
    loop_.loop();
  });

}

eventloop_thread::~eventloop_thread() {
  loop_.quit();
  thread_.join();
}

}