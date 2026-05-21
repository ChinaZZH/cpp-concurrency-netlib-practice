// test_client.cpp
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return 1;
    }
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8888);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("connect");
        close(sock);
        return 1;
    }

    const char* hello = "hello\n";
    send(sock, hello, strlen(hello), 0);
    char buf[1024];
    int n = recv(sock, buf, sizeof(buf), 0);
    if (n > 0) {
        buf[n] = 0;
        std::cout << "Received: " << buf;
    }

    std::cout << "Waiting for server to shutdown write...\n";
    sleep(15);  // 等待服务器空闲超时

    const char* world = "world\n";
    int ret = send(sock, world, strlen(world), 0);
    if (ret < 0) {
        perror("send");
    } else {
        std::cout << "Sent world, bytes=" << ret << std::endl;
    }

    // 尝试接收服务器可能的响应（服务器写端已关闭，应该收不到）
    n = recv(sock, buf, sizeof(buf), 0);
    if (n > 0) {
        buf[n] = 0;
        std::cout << "Received after shutdown: " << buf;
    } else if (n == 0) {
        std::cout << "Connection closed by peer\n";
    } else {
        perror("recv");
    }

    sleep(2);
    close(sock);
    return 0;
}