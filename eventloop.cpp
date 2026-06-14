#include <sys/eventfd.h>


#include "eventloop.h"

namespace webserver {

eventloop::eventloop(std::thread::id thread_id)
  : quit_(false),
    thread_id_(thread_id),
    poll_return_time_(0),
    poller_(),
    wakeup_fd_(::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC)),
    wakeup_channel_(&poller_, wakeup_fd_),
    calling_pending_callbacks_(false),
    pending_wakeup_(false) {
  logger::debug("eventloop::eventloop wakeup_fd_ = %d, thread_id_ = %d", wakeup_fd_, thread_id_);

  if (wakeup_fd_ < 0) {
    logger::error("eventfd failed");
  }

  wakeup_channel_.set_read_callback([this](timestamp receive_time) { 
    uint64_t one = 1;
    ssize_t n = ::read(wakeup_fd_, &one, sizeof(one));
    if (n != sizeof(one)) {
      logger::error("read failed");
    }
  });
  wakeup_channel_.enable_reading();
}

eventloop::~eventloop() {
  for (auto &conn : connections_) {
    std::shared_ptr<tcp_connection> p(conn.second);
    conn.second.reset();
    p->get_loop().run_in_loop([p]() {
      p->destroy_connection();
    });
  }

  wakeup_channel_.disable_all();
  wakeup_channel_.remove(); // 把channel从eventloop上删除掉
  ::close(wakeup_fd_);
}

void eventloop::loop() {
  while (!quit_) {
    std::vector<channel*> active_channels;
    poll_return_time_ = poller_.poll(POLL_TIMEOUT_MS, active_channels);
    
    pending_wakeup_ = false;
    
    for (auto channel : active_channels) {
      channel->handle_event(poll_return_time_);
    }

    run_pending_callbacks();
  }
  logger::debug("eventloop::loop %p stop looping", this);
}

void eventloop::quit() {
  quit_ = true;

  if (!is_in_loop_thread()) {
    wakeup();
  }
}

void eventloop::run_in_loop(std::function<void()> cb) {
  if (is_in_loop_thread()) {
    cb();
  } else {
    queue_in_loop(cb);
  }
}

void eventloop::queue_in_loop(std::function<void()> cb) {
  {
    std::unique_lock<std::mutex> lock(pending_callbacks_mutex_);
    pending_callbacks_.push_back(cb);
  }

  if (pending_wakeup_) {
    return;
  }

  // 如果正在执行pending_callbacks，cb会在eventloop下一次被唤醒时才执行
  // 这里wakeup，可以让下次唤醒更快到来，减少cb的执行延迟
  if (!is_in_loop_thread() || calling_pending_callbacks_) {
    wakeup();

    pending_wakeup_ = true;
  }
}

void eventloop::wakeup() {
  uint64_t one = 1;
  ssize_t n = ::write(wakeup_fd_, &one, sizeof(one));
  if (n != sizeof(one)) {
    logger::error("write failed");
  }
}

void eventloop::add_connection(std::shared_ptr<tcp_connection> conn) {
  run_in_loop([this, c = std::move(conn)]() {
    const auto id = c->get_id();
    connections_.emplace(id, std::move(c));
    c->establish_connection(); 
  });
  // 等到establish_connection完成，连接才完全建立
}

void eventloop::remove_connection(tcp_connection::id id) {
  connections_.erase(id);
}

void eventloop::run_pending_callbacks() {
  std::vector<std::function<void()>> callbacks;

  // 先获取锁，确保在运行回调时不会被唤醒
  {
    std::unique_lock<std::mutex> lock(pending_callbacks_mutex_);
    calling_pending_callbacks_ = true;
    callbacks.swap(pending_callbacks_);
  }

  for (auto cb : callbacks) {
    cb();
  }

  calling_pending_callbacks_ = false;
}

} // namespace webserver