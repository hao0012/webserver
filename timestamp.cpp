#include "timestamp.h"
#include <sys/time.h>

namespace webserver {

timestamp::timestamp(size_t ms) : ms_(ms) {}

timestamp timestamp::now() {
  struct timeval tv;
  gettimeofday(&tv, nullptr);
  return timestamp(tv.tv_sec * 1000 + tv.tv_usec / 1000);
}

std::string timestamp::to_string() const {
  char buf[128] = {0};
  tm tm_time;
  time_t s = ms_ / 1000;
  localtime_r(&s, &tm_time);
  snprintf(buf, 128, "%4d/%02d/%02d %02d:%02d:%02d",
            tm_time.tm_year + 1900,
            tm_time.tm_mon + 1,
            tm_time.tm_mday,
            tm_time.tm_hour,
            tm_time.tm_min,
            tm_time.tm_sec);
  return buf;
}

}  // namespace webserver
