#pragma once

#include <memory>
#include <functional>

namespace webserver {

class buffer;
class tcp_connection;
class timestamp;

using tcp_connection_ptr = std::shared_ptr<tcp_connection>;
using thread_init_callback = std::function<void()>;

using connection_callback = std::function<void(const std::shared_ptr<tcp_connection> &)>;
using close_callback = std::function<void(const std::shared_ptr<tcp_connection> &)>;
using write_complete_callback = std::function<void(const std::shared_ptr<tcp_connection> &)>;
using high_water_mark_callback = std::function<void(const std::shared_ptr<tcp_connection> &, size_t)>;

using message_callback = std::function<void(const std::shared_ptr<tcp_connection> &,
                                           buffer *,
                                           timestamp)>;                                            
}