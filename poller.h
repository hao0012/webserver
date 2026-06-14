#pragma once

#include <vector>
#include <unordered_map>

namespace webserver {

class channel;
class timestamp;

class poller {
public:
  poller() = default;
  virtual ~poller() = default;
  // 等待channel的就绪事件
  virtual timestamp poll(int timeout_ms, std::vector<channel*>& active_channels) = 0;

  // 修改channel中的监听事件
  virtual void update_channel(channel& channel) = 0;
  // 移除channel 不再监听
  virtual void remove_channel(channel& channel) = 0;
protected:
  using channel_map = std::unordered_map<int, channel*>;
  channel_map channels_;
};

}
