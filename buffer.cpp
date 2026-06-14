#include <sys/uio.h>
#include <unistd.h>
#include "buffer.h"

namespace webserver {

ssize_t buffer::read_fd(int fd, int *saved_errno) {
  char extra_buf[65536] = {0};

  struct iovec vec[2];
  const size_t writable = writable_bytes();

  vec[0].iov_base = begin() + writer_index_;
  vec[0].iov_len = writable;
  vec[1].iov_base = extra_buf;
  vec[1].iov_len = sizeof extra_buf;

  const int iovcnt = (writable < sizeof extra_buf) ? 2 : 1;
  const ssize_t n = ::readv(fd, vec, iovcnt);
  if (n < 0) {
    *saved_errno = errno;
  } else if (n <= writable) {
    writer_index_ += n;
  } else {
    writer_index_ = buffer_.size();
    append(extra_buf, n - writable);
  }
  return n;
}

ssize_t buffer::write_fd(int fd, int *saved_errno) {
  ssize_t n = ::write(fd, peek(), readable_bytes());
  if (n < 0) {
    *saved_errno = errno;
  }
  return n;
}

}