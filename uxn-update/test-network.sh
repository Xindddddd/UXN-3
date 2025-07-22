#!/bin/bash

echo "=== Uxn 网络设备测试 ==="
echo

# 创建测试目录
mkdir -p test-network
cd test-network

echo "1. 创建测试ROM文件..."

# 编译Echo服务器
../bin/uxnasm ../echo-server.tal echo-server.rom
if [ $? -eq 0 ]; then
    echo "✓ Echo服务器编译成功"
else
    echo "✗ Echo服务器编译失败"
    exit 1
fi

# 编译HTTP客户端  
../bin/uxnasm ../http-client.tal http-client.rom
if [ $? -eq 0 ]; then
    echo "✓ HTTP客户端编译成功"
else
    echo "✗ HTTP客户端编译失败"
    exit 1
fi

echo
echo "2. 测试Echo服务器..."

# 启动Echo服务器（后台运行）
timeout 30s ../bin/uxnemu echo-server.rom &
SERVER_PID=$!
echo "服务器启动 (PID: $SERVER_PID)"

# 等待服务器启动
sleep 3

# 测试连接
echo "发送测试数据..."
echo "Hello from bash!" | nc localhost 8888 > test-output.txt &
NC_PID=$!

# 等待响应
sleep 2

# 检查响应
if [ -f test-output.txt ] && [ -s test-output.txt ]; then
    echo "✓ 收到服务器响应:"
    cat test-output.txt
    echo
else
    echo "✗ 没有收到服务器响应"
fi

# 清理
kill $SERVER_PID 2>/dev/null
kill $NC_PID 2>/dev/null
wait $SERVER_PID 2>/dev/null

echo
echo "3. 测试HTTP客户端..."

# 测试HTTP客户端（需要网络连接）
timeout 10s ../bin/uxnemu http-client.rom

echo
echo "=== 手动测试说明 ==="
echo "Echo服务器测试:"
echo "  1. 运行: ../bin/uxnemu echo-server.rom"  
echo "  2. 另一个终端: echo 'hello' | nc localhost 8888"
echo "  3. 或者: telnet localhost 8888"
echo
echo "HTTP客户端测试:"
echo "  1. 运行: ../bin/uxnemu http-client.rom"
echo "  2. 应该连接到httpbin.org并显示JSON响应"
echo
echo "如果遇到问题:"
echo "  - 检查防火墙设置"  
echo "  - 确保端口8888未被占用: lsof -i :8888"
echo "  - 检查网络连接"

# 清理
rm -f test-output.txt

echo
echo "=== 测试完成 ==="