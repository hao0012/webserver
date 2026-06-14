#include "logger.h"

#include "event_util.h"
#include "channel.h"

namespace webserver {

channel::channel(poller *poller, int fd) : 
  poller_(poller), 
  fd_(fd), 
  events_(NONE_EVENT), 
  revents_(NONE_EVENT), 
  status_(channel::status::UNREGISTERED), 
  tied_(false) {}

void channel::handle_event(timestamp receive_time) {
  if (tied_) {
    std::shared_ptr<void> guard = tie_.lock();
    if (guard != nullptr) {
      handle_event_with_guard(receive_time);
    } else {
      // tcp_connection已经被关闭
    }
  } else {
    handle_event_with_guard(receive_time);
  }
}

void channel::tie(const std::shared_ptr<void> &tie) { 
  tie_ = tie; 
  tied_ = true;
}

void channel::update() {
  poller_->update_channel(*this); 
}

void channel::remove() { poller_->remove_channel(*this); }

void channel::handle_event_with_guard(timestamp receive_time) {
  if ((event_util::has_readable_event(revents_) || 
       event_util::has_priority_event(revents_)) &&
      read_callback_ != nullptr) {
    logger::debug("read event, fd %d", fd_);
    read_callback_(receive_time);
  }

  if (event_util::has_writable_event(revents_) &&
      write_callback_ != nullptr) {
    logger::debug("write event, fd %d", fd_);
    write_callback_();
  }

  if (event_util::has_peer_close_event(revents_) &&
      close_callback_ != nullptr) {
    logger::debug("close event, fd %d", fd_);
    close_callback_();
  }

  if (event_util::has_error_event(revents_) &&
      error_callback_ != nullptr) {
    logger::debug("error event, fd %d", fd_);
    error_callback_();
  }
}

}