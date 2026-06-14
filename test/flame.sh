# 1. 获取 echo_server 进程 PID
PID=$(pgrep echo_server)

# 2. 清理旧文件
rm -f perf.*
rm -f in-fb.data*

# 3. 采样（直接用上面拿到的 $PID）
perf record -F 99 -p $PID -g -o in-fb.data -- sleep 20

# 后续解析、生成火焰图不变
perf script -i in-fb.data &> perf.unfold
/home/hao/cpp/FlameGraph/stackcollapse-perf.pl perf.unfold &> perf.folded
/home/hao/cpp/FlameGraph/flamegraph.pl perf.folded > perf.svg