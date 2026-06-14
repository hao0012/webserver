import asyncio
from framework import run_benchmark

# ===================== 你的本地网络库接口（替换成你自己的）=====================
# 示例：模拟本地TCP网络库连接+收发逻辑
async def net_lib_connect_and_send(host: str, port: int, data: bytes) -> bool:
    """调用你的网络库：建立连接 + 发送数据 + 等待响应"""
    reader, writer = await asyncio.open_connection(host, port)
    writer.write(data)
    await writer.drain()
    
    resp = await reader.read(1024)
    writer.close()
    await writer.wait_closed()
    return data == resp  # 返回是否请求成功
# ==========================================================================

if __name__ == "__main__":
  # 启动压测
  asyncio.run(run_benchmark(func=net_lib_connect_and_send, concurrency=10000, total_req=100000))