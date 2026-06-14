#include <iostream>
// Linux网络编程核心头文件
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

int main() {
    // ===================== 1. 创建客户端套接字 =====================
    // AF_INET = IPv4协议, SOCK_STREAM = TCP协议, 0 = 默认协议
    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd == -1) {
        std::cerr << "创建socket失败！" << std::endl;
        return -1;
    }

    // ===================== 2. 配置服务器地址 =====================
    sockaddr_in server_addr{};  // 服务器地址结构体
    server_addr.sin_family = AF_INET;  // IPv4
    server_addr.sin_port = htons(8000); // 服务器端口（必须和服务器一致）
    // 服务器IP地址（改成你的服务器IP）
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    // ===================== 3. 连接服务器 =====================
    if (connect(client_fd, (sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        std::cerr << "连接服务器失败！" << std::endl;
        close(client_fd);
        return -1;
    }
    std::cout << "成功连接服务器！" << std::endl;

    // ===================== 4. 发送数据 =====================
    const char* send_data = "Hello Server! 这是Linux客户端发送的数据";
    ssize_t send_len = send(client_fd, send_data, strlen(send_data), 0);
    if (send_len == -1) {
        std::cerr << "发送数据失败！" << std::endl;
        close(client_fd);
        return -1;
    }
    std::cout << "发送成功！数据长度：" << send_len << std::endl;
    std::cout << "发送内容：" << send_data << std::endl;

    // ===================== 5. 接收服务器响应 =====================
    char buffer[1024]; // 接收缓冲区
    ssize_t recv_len = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    if (recv_len == -1) {
        std::cerr << "接收数据失败！" << std::endl;
        close(client_fd);
        return -1;
    }
    buffer[recv_len] = '\0'; // 添加字符串结束符
    std::cout << "接收成功！数据长度：" << recv_len << std::endl;
    std::cout << "接收内容：" << buffer << std::endl;

    // ===================== 6. 关闭套接字 =====================
    close(client_fd);

    return 0;
}