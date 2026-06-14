#pragma once

#include <string>

namespace webserver {
class timestamp {
public:
  timestamp() = default;
  explicit timestamp(size_t ms);
  ~timestamp() = default;

  static timestamp now();
  std::string to_string() const;
private:
  size_t ms_;
};
  
}  // namespace webserver
