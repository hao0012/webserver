#pragma once

#include <sys/epoll.h>

namespace webserver {

class event_util {
public:
  static bool has_error_event(int events) { return events & EPOLLERR; }
  static bool has_readable_event(int events) { return events & EPOLLIN; }
  // 紧急、优先级数据可读
  static bool has_priority_event(int events) { return events & EPOLLPRI; }
  static bool has_writable_event(int events) { return events & EPOLLOUT; }
  // 对端关闭事件
  static bool has_peer_close_event(int events) { return events & EPOLLHUP; }


};

}