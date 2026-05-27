#include "EventLoop.h"
#include "TcpClient.h"
#include "LibRpc/RpcClient.h"
#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>

int main() {
    // 创建 EventLoop 对象（将在独立线程中运行）
    EventLoop loop;

    // 创建 TcpClient 和 RpcClient
    auto client = std::make_shared<TcpClient>(&loop, "127.0.0.1", 8888);
    RpcClient rpcClient(nullptr);

    // 用于等待连接建立的同步
    std::mutex mtx;
    std::condition_variable cv;
    bool connected = false;

    client->SetConnectionCallBack([&](const TcpConnectionPtr& conn) {
        std::cout << "Connected to server" << std::endl;
        rpcClient.SetConnection(conn);
        {
            std::lock_guard<std::mutex> lock(mtx);
            connected = true;
        }
        cv.notify_one();
    });
    client->SetMessageCallBack([&](const TcpConnectionPtr&, std::string& msg) {
        rpcClient.OnResponse(msg);
    });
    client->Connect();

    // 启动 I/O 线程
    std::thread io_thread([&loop]() {
        loop.Loop();
    });

    // 等待连接建立
    {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [&] { return connected; });
    }

    // 主线程发起 RPC 调用
    try {
        std::string result = rpcClient.Call("add", R"({"a":10,"b":32})", 5000);
        std::cout << "RPC result: " << result << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "RPC error: " << e.what() << std::endl;
    }

    // 停止 I/O 线程（可选，简单退出）
    loop.Quit();
    io_thread.join();
    return 0;
}