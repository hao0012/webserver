#pragma once
#include <cstddef>
#include <string>
#include <vector>

namespace webserver {

class buffer {
public:
  static constexpr size_t CHEAP_PREPEND_SIZE = 8;
  static constexpr size_t INIT_SIZE = 1024;

  explicit buffer(size_t size = INIT_SIZE): 
    buffer_(CHEAP_PREPEND_SIZE + size), 
    reader_index_(CHEAP_PREPEND_SIZE), 
    writer_index_(CHEAP_PREPEND_SIZE) {
  }

  size_t readable_bytes() const {
    return writer_index_ - reader_index_;
  }

  size_t writable_bytes() const {
    return buffer_.size() - writer_index_;
  }

  size_t prependable_bytes() const {
    return reader_index_;
  }

  const char* peek() const {
    return begin() + reader_index_;
  }

  void retrieve(size_t len) {
    if (len < readable_bytes()) {
      reader_index_ += len;
    } else {
      retrieve_all();
    }
  }
  
  void retrieve_all() {
    reader_index_ = writer_index_ = CHEAP_PREPEND_SIZE;
  }

  std::string retrieve_all_as_string() {
    return retrieve_as_string(readable_bytes());
  }

  std::string retrieve_as_string(size_t len) {
    std::string str(peek(), len);
    retrieve(len);
    return str;
  }

  void ensure_writable_bytes(size_t len) {
    if (len > writable_bytes()) {
      resize(len);
    }
  }

  void append(const char* data, size_t len) {
    ensure_writable_bytes(len);
    std::copy(data, data + len, begin_write());
    writer_index_ += len;
  }

  char *begin_write() {
    return begin() + writer_index_;
  }

  const char *begin_write() const {
    return begin() + writer_index_;
  }

  ssize_t read_fd(int fd, int *saved_errno);
  ssize_t write_fd(int fd, int *saved_errno);
private:
  char *begin() {
    return &*buffer_.begin();
  }

  const char *begin() const {
    return &*buffer_.begin();
  }

  void resize(size_t size) {
    if (writable_bytes() + prependable_bytes() < size + CHEAP_PREPEND_SIZE) {
      buffer_.resize(writer_index_ + size);
    } else {
      size_t readable = readable_bytes();
      std::copy(begin() + reader_index_, begin() + writer_index_, begin() + CHEAP_PREPEND_SIZE);
      reader_index_ = CHEAP_PREPEND_SIZE;
      writer_index_ = reader_index_ + readable;
    }
  }

  std::vector<char> buffer_;
  size_t reader_index_;
  size_t writer_index_;
};

}