#include <errno.h>
#include <unistd.h>
#include "epoller.h"
#include "logger.h"
#include "channel.h"

namespace webserver {

epoller::epoller() : 
  epoll_fd_(::epoll_create1(EPOLL_CLOEXEC)), 
  events_size_(INIT_EVENT_LIST_SIZE),
  events_(events_size_) {
  if (epoll_fd_ < 0) {
    logger::error("epoll_create1 failed, errno = %d", errno);
  }
}

epoller::~epoller() {
  ::close(epoll_fd_);
}

timestamp epoller::poll(int timeout_ms, std::vector<channel*>& active_channels) {
  int event_num = ::epoll_wait(epoll_fd_, events_.data(), static_cast<int>(events_size_), -1); 
  int err = errno;
  if (event_num > 0) {
    logger::debug("epoll_wait: get event_num = %d", event_num);
    for (size_t i = 0; i < event_num; ++i) {
      auto c = static_cast<channel *>(events_[i].data.ptr);
      c->set_revents(events_[i].events);
      active_channels.push_back(c);
    }
    if (event_num == static_cast<int>(events_size_)) {
      events_size_ <<= 1;
      events_.resize(events_size_);
    }
  } else if (event_num < 0) {
    if (err != EINTR) {
      logger::error("epoll_wait failed, errno = %d", err);
    }
  } else {
    logger::debug("%s epoll_wait timeout", __func__);
  }

  return timestamp::now();
}

void epoller::update_channel(channel& channel) {
  const auto status = channel.status();

  if (status == channel::status::UNREGISTERED) {
    channels_[channel.fd()] = &channel;
    channel.set_status(channel::status::REGISTERED);
    logger::debug("epoller::update_channel add channel fd = %d", channel.fd());
    update_channel(EPOLL_CTL_ADD, channel);
  } else {
    if (channel.is_listening_none()) {
      logger::debug("epoller::update_channel delete channel fd = %d", channel.fd());
      update_channel(EPOLL_CTL_DEL, channel);
      channel.set_status(channel::status::UNREGISTERED);
    } else {
      logger::debug("epoller::update_channel modify channel fd = %d", channel.fd());
      update_channel(EPOLL_CTL_MOD, channel);
    }
  }
}

void epoller::remove_channel(channel& channel) {
  logger::debug("epoller::remove_channel channel = %p", &channel);
  channels_.erase(channel.fd());
  if (channel.status() == channel::status::REGISTERED) {
    update_channel(EPOLL_CTL_DEL, channel);
  }
  channel.set_status(channel::status::UNREGISTERED);
}

void epoller::update_channel(int operation, channel& channel) {
  epoll_event event = {
    .events = channel.events(),
    .data = { .ptr = &channel },
  };

  int ret = ::epoll_ctl(epoll_fd_, operation, channel.fd(), &event);
  logger::debug("epoll_ctl operation = %d, ret = %d", operation, ret);
}

}