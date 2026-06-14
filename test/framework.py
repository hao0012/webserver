import asyncio
import time
from typing import List

# 单任务：单次请求
async def single_task(func, host: str, port: int, data: bytes) -> tuple[bool, float]:
  start = time.perf_counter()
  try:
    ok = await func(host, port, data)
  except Exception as e:
    print(e)
    ok = False
  cost = time.perf_counter() - start
  return ok, cost

# 一组并发任务（N个协程同时执行）
async def batch_tasks(func, host: str, port: int, data: bytes, count: int) -> List[tuple[bool, float]]:
  tasks = [single_task(func, host, port, data) for _ in range(count)]
  return await asyncio.gather(*tasks)

# 计算分位数 P95 / P99
def calculate_percentile(data: list[float], percent: float) -> float:
    if not data:
        return 0.0
    data_sorted = sorted(data)
    n = len(data_sorted)
    idx = int(n * percent)
    # 防止下标越界
    idx = min(idx, n - 1)
    return data_sorted[idx]

# 压测入口
async def run_benchmark(
  func,
  host: str = "127.0.0.1",
  port: int = 8000,
  concurrency: int = 100,   # 并发协程数（模拟并发客户端）
  total_req: int = 10000,   # 总请求数
  payload: bytes = b"hello network lib"
):
  print(f"开始压测 | 并发数:{concurrency} 总请求:{total_req}")
  start_time = time.perf_counter()

  success = 0
  fail = 0
  all_cost = []

  # 分批执行（避免一次性创建上万协程内存抖动）
  batch = concurrency
  for i in range(0, total_req, batch):
    cur_num = min(batch, total_req - i)
    results = await batch_tasks(func, host, port, payload, cur_num)
    for ok, cost in results:
      if ok:
        success += 1
      else:
        fail += 1
      all_cost.append(cost)

  total_time = time.perf_counter() - start_time
  # 计算指标
  qps = total_req / total_time
  avg_cost = sum(all_cost) / len(all_cost) if all_cost else 0
  max_cost = max(all_cost) if all_cost else 0
  err_rate = fail / total_req * 100

  p90 = calculate_percentile(all_cost, 0.90)
  p95 = calculate_percentile(all_cost, 0.95)
  p99 = calculate_percentile(all_cost, 0.99)

  # 输出报告
  print("=" * 50)
  print(f"总耗时: {total_time:.2f} s")
  print(f"QPS: {qps:.2f}")
  print(f"成功: {success} | 失败: {fail} | 错误率: {err_rate:.2f}%")
  print(f"平均耗时: {avg_cost * 1000:.2f} ms")
  print(f"最大耗时: {max_cost * 1000:.2f} ms")
  print(f"P90 耗时: {p90 * 1000:.2f} ms")
  print(f"P95 耗时: {p95 * 1000:.2f} ms")
  print(f"P99 耗时: {p99 * 1000:.2f} ms")
  print("=" * 50)