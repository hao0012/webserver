#pragma once

namespace webserver {

class noncopyable {
public:
  noncopyable() = default;
  ~noncopyable() = default;
  noncopyable(const noncopyable&) = delete;
  noncopyable& operator=(const noncopyable&) = delete;

private:
  

  // 默认禁止移动语义
};

}  // namespace webserver