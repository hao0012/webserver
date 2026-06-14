#pragma once

#include <sys/epoll.h>
#include "poller.h"

namespace webserver {

class channel;

// 每个eventloop有一个epoller，用于处理channel的epoll事件通知
// epoller并不拥有channel，只是监听channel的事件通知，channel由tcp_connection管理
class epoller : public poller {
public:
  epoller();
  ~epoller() override;
  
  timestamp poll(int timeout_ms, std::vector<channel*>& active_channels) override;

  // 增加或修改已监听channel中的监听事件
  void update_channel(channel& channel) override;
  // 移除channel 不再监听
  void remove_channel(channel& channel) override;

private:
  static constexpr size_t INIT_EVENT_LIST_SIZE = 1024;

  void update_channel(int operation, channel& channel);

  int epoll_fd_;
  size_t events_size_;
  std::vector<epoll_event> events_;
};

}