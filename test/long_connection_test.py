import asyncio
from framework import run_benchmark

async def long_conn_worker(host, port, queue):
  # 单个长连接协程，循环处理请求
  reader, writer = await asyncio.open_connection(host, port)
  while True:
    try:
      # 从队列取任务（解耦）
      await queue.get()
      writer.write(b"test data")
      await writer.drain()
      await reader.read(1024)
      queue.task_done()
    except Exception as e:
      print(e)
      break
  writer.close()
  await writer.wait_closed()

if __name__ == "__main__":
  # 启动压测
  asyncio.run(run_benchmark(concurrency=10000, total_req=100000))