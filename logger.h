#pragma once

#include "noncopyable.h"
#include "timestamp.h"

#include <iostream>
#include <string_view>
#include <cstring>
#include <mutex>
#include <fstream>
#include <stdexcept>

namespace webserver {

class logger : noncopyable {
public:
    enum class level {
      NONE  = 0,
      INFO  = 1,
      DEBUG = 2,
      ERROR = 3,
    };

    static inline const level GLOBAL_LEVEL = level::NONE;

    template<typename... Args>
    static void info(std::string_view fmt, Args&&... args) {
        if (level::INFO > GLOBAL_LEVEL) return;
        do_write("[INFO] ", fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void error(std::string_view fmt, Args&&... args) {
        if (level::ERROR > GLOBAL_LEVEL) return;
        do_write("[ERROR] ", fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void debug(std::string_view fmt, Args&&... args) {
        if (level::DEBUG > GLOBAL_LEVEL) return;
        do_write("[DEBUG] ", fmt, std::forward<Args>(args)...);
    }

private:
    explicit logger() = delete;
    static constexpr int LOG_BUF_SIZE = 1024;

    // 懒初始化：文件流 + 互斥锁（局部静态，初始化安全）
    static std::ofstream& get_file() {
      static std::ofstream file("log.txt", std::ios::app | std::ios::out);
      return file;
    }

    static std::mutex& get_mutex() {
      static std::mutex mtx;
      return mtx;
    }

    // 统一写入逻辑：栈上缓冲区，无全局堆/静态数组
    template<typename... Args>
    static void do_write(std::string_view prefix, std::string_view fmt, Args&&... args) {
      std::lock_guard<std::mutex> lock(get_mutex());
      std::ofstream& file = get_file();
      if (!file.is_open()) return;

      // 栈上缓冲区，安全无全局初始化问题
      char buf[LOG_BUF_SIZE] = {0};
      int pos = 0;
      const int remain = LOG_BUF_SIZE - 1;

      // 1. 拼接日志前缀
      int ret = std::snprintf(buf + pos, remain - pos, "%s", prefix.data());
      if (ret < 0 || pos + ret >= remain) return;
      pos += ret;

      // 2. 拼接时间戳（单独处理，避免初始化时序问题）
      std::string ts = timestamp::now().to_string();
      ret = std::snprintf(buf + pos, remain - pos, "%s: ", ts.c_str());
      if (ret < 0 || pos + ret >= remain) return;
      pos += ret;

      // 3. 格式化日志内容
      if constexpr (sizeof...(Args) == 0) {
        size_t copy_len = std::min(fmt.size(), static_cast<size_t>(remain - pos));
        std::memcpy(buf + pos, fmt.data(), copy_len);
        pos += copy_len;
      } else {
        ret = std::snprintf(buf + pos, remain - pos, fmt.data(), std::forward<Args>(args)...);
        if (ret < 0) return;
        pos += ret;
      }

      // 追加换行，不用 endl 避免强制刷盘
      if (pos < remain) {
        buf[pos++] = '\n';
      }
      buf[pos] = '\0';

      // 一次性写入
      file << buf;
    }
};

}  // namespace webserver