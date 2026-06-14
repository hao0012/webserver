#include <sys/sendfile.h>

#include "tcp_connection.h"
#include "eventloop.h"

namespace webserver {

tcp_connection::id tcp_connection::next_id() {
  static std::atomic<tcp_connection::id> id = 1;
  return id.fetch_add(1);
}

tcp_connection::tcp_connection(
  eventloop& loop,
  int fd, 
  const inet_address& local_addr, 
  const inet_address& peer_addr)
  : id_(next_id()),
    state_(state::CONNECTING),
    loop_(loop),
    is_listening_(false),
    socket_(fd),
    channel_(&loop_.get_poller(), fd),
    local_addr_(local_addr),
    peer_addr_(peer_addr),
    high_water_mark_(64 * 1024 * 1024) {

  logger::debug("tcp_connection constructor");
  channel_.set_read_callback([&](timestamp receive_time) { handle_read(receive_time); });
  channel_.set_write_callback([&]() { handle_write(); });
  channel_.set_close_callback([&]() { handle_close(); });
  channel_.set_error_callback([&]() { handle_error(); });

  socket_.set_keep_alive(true);
}

tcp_connection::~tcp_connection() {
  logger::debug("tcp_connection destructor");
}

void tcp_connection::connection_close_callback() {
  auto p = shared_from_this();
  loop_.run_in_loop([pp = std::move(p)]() {
    pp->loop_.remove_connection(pp->id_);
    pp->destroy_connection();
  });
}

void tcp_connection::send(const std::string& data) {
  if (state_ != state::CONNECTED) {
    return;
  }

  loop_.run_in_loop([data, this]() {
    send_in_loop(data.c_str(), data.size());
  });
}

void tcp_connection::send_in_loop(const void *data, size_t size) {
  ssize_t nwrote = 0;
  size_t remaining = size;
  bool fault_error = false;

  // channel第一次开始开始写数据或output_buffer_没有待发送数据
  if (!channel_.is_listening_write_event() && output_buffer_.readable_bytes() == 0) {
    nwrote = ::write(channel_.fd(), data, size);
    if (nwrote >= 0) {
      remaining = size - nwrote;
      logger::debug("remaining %d bytes to write", remaining);
      if (remaining == 0 && write_complete_callback_) {
        auto p = shared_from_this();
        loop_.queue_in_loop([pp = std::move(p)]() { 
          pp->write_complete_callback_(pp); 
        });
      }
    } else {
      nwrote = 0;
      if (errno != EWOULDBLOCK) {
        logger::error("send_in_loop error: {}", strerror(errno));
        if (errno == EPIPE || errno == ECONNRESET) {
          fault_error = true;
        }
      }
    }
  }

  if (!fault_error && remaining > 0) {
    size_t old_len = output_buffer_.readable_bytes();
    if (old_len + remaining >= high_water_mark_ && 
        old_len < high_water_mark_ && 
        high_water_mark_callback_) {
      auto p = shared_from_this();
      loop_.queue_in_loop([pp = std::move(p), old_len, remaining]() {
        pp->high_water_mark_callback_(pp, old_len + remaining); });
    }
    output_buffer_.append(static_cast<const char*>(data) + nwrote, remaining);
    if (!channel_.is_listening_write_event()) {
      channel_.enable_writing();
    }
  }
}

void tcp_connection::shutdown() {
  if (state_ == state::CONNECTED) {
    state_ = state::DISCONNECTING;
    loop_.run_in_loop([&]() { shutdown_in_loop(); });
  }
}

void tcp_connection::shutdown_in_loop() {
  if (!channel_.is_listening_write_event()) {
    socket_.shutdown_write();
  }
}

void tcp_connection::establish_connection() {
  auto p = shared_from_this();
  channel_.tie(p);
  channel_.enable_reading();  // 需要在subloop中执行，否则mainloop和subloop
                              // 同时操作epoller会有并发问题
  if (connection_callback_) {
    connection_callback_(p);
  }
  state_ = state::CONNECTED;
  logger::debug("connection set state to connected");
}

void tcp_connection::destroy_connection() {
  if (state_ == state::CONNECTED) {
    state_ = state::DISCONNECTED;
    channel_.disable_all();
  }
  channel_.remove();
}

void tcp_connection::handle_read(timestamp receive_time) {
  int save_errno = 0;
  ssize_t n = input_buffer_.read_fd(channel_.fd(), &save_errno);
  logger::debug("read fd %d get %d bytes", channel_.fd(), n);
  if (n > 0) {
    if (message_callback_) {
      message_callback_(shared_from_this(), &input_buffer_, receive_time);
    }
  } else if (n == 0) {
    // 收到FIN包，客户端断开连接
    handle_close();
  } else {
    errno = save_errno;
    logger::error("handle_read error: {}", strerror(errno));
    handle_error();
  }
}

void tcp_connection::handle_write() {
  if (!channel_.is_listening_write_event()) {
    logger::debug("handle_write channel is not writable");
    return;
  }

  int save_errno = 0;
  ssize_t n = output_buffer_.write_fd(channel_.fd(), &save_errno);
  if (n <= 0) {
    logger::error("handle_write error");
    return;
  }
  output_buffer_.retrieve(n);
  if (output_buffer_.readable_bytes() == 0) {
    channel_.disable_writing();
    if (write_complete_callback_) {
      auto p = shared_from_this();
      loop_.queue_in_loop([pp = std::move(p)]() { 
        pp->write_complete_callback_(pp); 
      });
    }
    if (state_ == state::DISCONNECTING) {
      shutdown_in_loop();
    }
  }
}

void tcp_connection::handle_close() {
  logger::debug("handle_close");
  state_ = state::DISCONNECTED;
  channel_.disable_all();

  connection_close_callback();
}

void tcp_connection::handle_error() {
  int opt_val;
  socklen_t opt_len = sizeof opt_val;
  int err = 0;
  if (::getsockopt(channel_.fd(), SOL_SOCKET, SO_ERROR, &opt_val, &opt_len) < 0) {
    err = errno;
  } else {
    err = opt_val;
  }
  logger::error("handle_error error: {}", strerror(err));
}

void tcp_connection::send_file(int fd, off_t offset, size_t size) {
  if (state_ == state::CONNECTED) {
    if (loop_.is_in_loop_thread()) {
      send_file_in_loop(fd, offset, size);
    } else {
      auto p = shared_from_this();
      loop_.run_in_loop([pp = std::move(p), fd, offset, size]() {
        pp->send_file_in_loop(fd, offset, size);
      });
    }
  }
}

void tcp_connection::send_file_in_loop(int fd, off_t offset, size_t size) {
  ssize_t bytes_sent = 0;
  size_t remaining = size;
  bool fault_error = false;

  if (state_ == state::DISCONNECTING) {
    logger::debug("send_file_in_loop state is DISCONNECTING");
    return;
  }

  if (!channel_.is_listening_write_event() && output_buffer_.readable_bytes() == 0) {
    bytes_sent = sendfile(socket_.fd(), fd, &offset, remaining);
    if (bytes_sent >= 0) {
      remaining -= bytes_sent;
      if (remaining == 0 && write_complete_callback_) {
        auto p = shared_from_this();
        loop_.queue_in_loop([pp = std::move(p)]() { 
          pp->write_complete_callback_(pp); 
        });
      }
    } else {
      if (errno != EWOULDBLOCK) {
        logger::error("send_file_in_loop error: {}", strerror(errno));
      } 
      if (errno == EPIPE || errno == ECONNRESET) {
        fault_error = true;
      }
    }
  }

  if (!fault_error && remaining > 0) {
    auto p = shared_from_this();
    loop_.queue_in_loop([p, fd, offset, remaining]() {
      p->send_file_in_loop(fd, offset, remaining);
    });
  }
}

}