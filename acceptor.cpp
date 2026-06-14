#include "acceptor.h"

namespace webserver {

acceptor::acceptor(eventloop &loop, eventloop_thread_pool &thread_pool, const inet_address& addr, bool reuse_port)
  : accept_socket_(::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP))
  , accept_channel_(&loop.get_poller(), accept_socket_.fd())
  , thread_pool_(thread_pool) {
  accept_socket_.set_reuse_addr(true);
  accept_socket_.set_reuse_port(reuse_port);
  accept_socket_.bind_address(addr);
}

acceptor::~acceptor() {
  accept_channel_.disable_all();
  accept_channel_.remove();
}

void acceptor::listen(size_t backlog) {
  int ret = accept_socket_.listen(backlog);
  logger::debug("acceptor start listen, listen_fd %d, ret %d", accept_channel_.fd(), ret);
  if (ret != 0) {
    logger::error("acceptor start listen, listen_fd %d, ret %d", accept_channel_.fd(), ret);
    return;
  }

  // 处理新用户的连接事件的回调函数
  auto handle_connect = [&](timestamp ms) {
    logger::debug("new connection event, fd %d", accept_socket_.fd());
    inet_address client_addr;
    int conn_fd = accept_socket_.accept(&client_addr);
    if (conn_fd >= 0) {
      // 选择一个eventloop管理新的用户连接
      eventloop &loop = thread_pool_.get_next_eventloop();

      // 创建新的用户连接
      inet_address local_addr(conn_fd);
      auto conn = std::make_shared<tcp_connection>(loop, conn_fd, local_addr, client_addr);
      conn->set_connect_callback(user_connect_callback_);
      conn->set_message_callback(user_message_callback_);
      conn->set_write_complete_callback(user_write_complete_callback_);

      loop.add_connection(std::move(conn));  // 此时connection的状态是CONNECTING，用户发数据来，不会执行message_callback
                                  // 不过没关系，数据会等待connection建立完成后再来处理
    } else {
      logger::error("accept connection fd %d fail", accept_socket_.fd());
      if (errno == EMFILE) {
        logger::error("accept connection fd %d fail, EMFILE", accept_socket_.fd());
      }
    }
  };

  accept_channel_.set_read_callback(handle_connect);
  accept_channel_.enable_reading();
  logger::debug("acceptor install fd to epoller");
}

} // namespace webserver