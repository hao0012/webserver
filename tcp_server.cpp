#include "tcp_server.h"

namespace webserver {

tcp_server::tcp_server(const inet_address &addr, 
                       const std::string &name, option opt)
  : main_loop_(std::this_thread::get_id()),
    thread_pool_(main_loop_, name),
    acceptor_(main_loop_, thread_pool_, addr, opt == option::REUSE_PORT),
    started_(false) {
}

void tcp_server::set_thread_init_callback(thread_init_callback cb) { 
  thread_pool_.set_user_thread_init_callback(cb); 
}
void tcp_server::set_connection_callback(connection_callback cb) { 
  acceptor_.set_user_connect_callback(cb); 
}
void tcp_server::set_message_callback(message_callback cb) { 
  acceptor_.set_user_message_callback(cb); 
}
void tcp_server::set_write_complete_callback(write_complete_callback cb) { 
  acceptor_.set_user_write_complete_callback(cb); 
}
void tcp_server::set_thread_num(int num) {
  thread_pool_.set_thread_num(num);
}

void tcp_server::start() {
  if (started_.exchange(true)) {
    return;
  }
  thread_pool_.start();
  // TODO backlog 配置
  main_loop_.run_in_loop([&]() { 
    acceptor_.listen(1000);
    logger::debug("server start successfully!");
  });

  main_loop_.loop();
}

}