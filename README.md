模仿muduo实现的网络库，muduo中部分模块的归属有些奇怪，所以重新组织了一下，更好理解，具体可见架构图。

## 架构
详细架构图见 [doc/架构.md](doc/架构.md)。

## 核心组件

| 文件 | 说明 |
|------|------|
| `eventloop.h/.cpp` | 事件循环核心，基于 epoll 阻塞等待与分发事件 |
| `channel.h/.cpp` | 封装 fd 与感兴趣的事件及对应回调 |
| `epoller.h/.cpp` | 对 `epoll` 的封装，管理 channel 的增删改 |
| `tcp_server.h/.cpp` | TCP 服务端封装，管理 acceptor 与线程池 |
| `tcp_connection.h/.cpp` | TCP 连接封装，管理连接的读写与生命周期 |
| `buffer.h/.cpp` | 自动扩容的应用层缓冲区 |
| `acceptor.h/.cpp` | 监听新连接并分发给子线程 |
| `eventloop_thread.h/.cpp` | 每个工作线程运行一个 eventloop |
| `eventloop_thread_pool.h/.cpp` | 管理线程池的创建与调度 |
| `timestamp.h/.cpp` | 时间戳工具类 |
| `inet_address.h` | 网络地址封装 |
| `logger.h` | 简易日志宏 |

## 快速开始
### 构建

```bash
# 默认构建（带调试符号，适合性能分析）
cd build
cmake ..
make -j$(nproc)

# 开启 AddressSanitizer（内存检测模式）
cmake .. -DUSE_ASAN=ON
make -j$(nproc)
```

### 运行 Echo 服务

```bash
# 启动服务端
./build/echo_server

# 使用示例客户端测试
./build/echo_client

# 使用 Python 压测脚本
python3 test/echo_test.py
```

### 使用库

参考 [example/echo_server.cpp](example/echo_server.cpp)。
编译时链接 `webserver_lib` 与 `pthread`：
```cmake
target_link_libraries(your_target webserver_lib pthread)
```

## 测试

项目提供多种测试与性能分析工具：

- `test/echo_test.py`：短连接 Echo 压测，支持高并发请求
- `test/long_connection_test.py`：长连接测试
- `test/flame.sh`：基于 `perf` 生成火焰图，辅助性能分析

### 压测结果

使用 `echo_test.py` 进行短连接压测，总请求数 100,000：

| 指标 | 1,000 并发 | 5,000 并发 | 10,000 并发 | 15,000 并发 | 20,000 并发 |
|------|-----------|-----------|------------|------------|------------|
| 总耗时 | 16.04 s | 18.66 s | 19.96 s | 26.88 s | 63.09 s |
| QPS | 6,234.81 | 5,358.79 | 5,009.29 | 3,719.82 | 1,585.02 |
| 成功 / 失败 | 100000 / 0 | 100000 / 0 | 100000 / 0 | 100000 / 0 | 100000 / 0 |
| 平均耗时 | 119.70 ms | 707.06 ms | 1419.39 ms | 2810.36 ms | 10089.91 ms |
| 最大耗时 | 197.65 ms | 986.36 ms | 2163.99 ms | 4195.21 ms | 13513.26 ms |
| P90 | 146.86 ms | 851.30 ms | 1814.75 ms | 3845.12 ms | 12563.53 ms |
| P95 | 153.89 ms | 878.14 ms | 1915.76 ms | 3987.03 ms | 13048.29 ms |
| P99 | 170.64 ms | 947.05 ms | 2099.30 ms | 4083.46 ms | 13423.73 ms |