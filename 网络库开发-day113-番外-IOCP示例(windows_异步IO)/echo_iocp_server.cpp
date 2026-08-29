#include <winsock2.h>
#include <windows.h>
#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include <sstream>


#pragma comment(lib, "ws2_32.lib")

// ---------- 常量 ----------
const int DEFAULT_PORT = 8888;
const int WORKER_THREAD_COUNT = (std::thread::hardware_concurrency() * 2); // 工作线程数，通常 = CPU 核心数 * 2

// ---------- 完成键 ----------
enum CompletionKey {
    COMP_KEY_LISTEN_SOCKET = 0,
    COMP_KEY_CLIENT_SOCKET = 1,
    COMP_KEY_EXIT   = 999,          // 退出事件
};

// ---------- 全局变量 ----------
HANDLE g_iocp_handle = nullptr;                      // 完成端口句柄

std::atomic<bool> g_server_running{ true };         // 服务器运行标志

std::vector<std::thread> g_worker_threads;          // 工作线程列表

std::atomic<int> g_active_workers{ 0 };

// ---------- 工作线程主循环----------
void WorkerThread()
{
    g_active_workers.fetch_add(1);
    std::stringstream ss;
    {
        ss << "[Worker] Thread " << std::this_thread::get_id() << " started." << std::endl;
        std::cout << ss.str();
    }
    
    
    while(g_server_running)
    {
        DWORD bytes_transferred = 0;
        ULONG_PTR completion_key = 0;
        OVERLAPPED* overlapped = nullptr;

        // ================================================================
        // 核心：等待完成端口上的事件
        // ================================================================
        // INFINITE 无限等待，直到有事件或退出
        BOOL result = GetQueuedCompletionStatus(g_iocp_handle, &bytes_transferred, &completion_key, &overlapped, INFINITE);

        // 检查退出事件
        if(!g_server_running)
        {
            break;
        }

        // 检查 GetQueuedCompletionStatus 是否失败
        if(!result) {
            DWORD error = GetLastError();
            if (error == ERROR_ABANDONED_WAIT_0 || error == ERROR_OPERATION_ABORTED) {
                // 正常的关闭或取消操作
                std::cout << "[Worker] I/O operation canceled." << std::endl;
                continue;
            }
            std::cerr << "[Worker] GetQueuedCompletionStatus failed, error: " << error << std::endl;
            continue;
        }

        switch(completion_key)
        {
        case COMP_KEY_LISTEN_SOCKET:
            std::cout << "[Worker] Listen socket event (AcceptEx complete) - not implemented yet." << std::endl;
            break;
        case COMP_KEY_CLIENT_SOCKET:
            std::cout << "[Worker] Client socket event - bytes: " << bytes_transferred << std::endl;
            break;
        case COMP_KEY_EXIT:
            std::cout << "[Worker] Exit event received." << std::endl;
            g_server_running = false;
            break;
        default:
            std::cout << "[Worker] Unknown completion key: " << completion_key << std::endl;
            break;
        }
    }

    g_active_workers.fetch_add(-1);
    {
        ss.str("");
        ss.clear();
        ss << "[Worker] Thread " << std::this_thread::get_id() << " stoped." << std::endl;
        std::cout << ss.str();
    }
}


// ---------- 主函数 ----------
int main() {
    std::cout << "=== IOCP Step 1: Framework ===" << std::endl;
    std::cout << "=== IOCP Step 2: Bind Listen Socket ===" << std::endl;

    // ================================================================
    // 2.1 Winsock 初始化
    // ================================================================
    WSADATA wsaData;
    int ret = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if(ret != 0) {
        std::cerr << "[Step1] WSAStartup failed, error:" << ret << std::endl;
        return -1;
    }
    std::cout << "[Step1] Winsock initialized successfully." << std::endl;
    
    // ================================================================
    // 2.2 创建完成端口
    // ================================================================
    g_iocp_handle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
    if(!g_iocp_handle)
    {
        std::cerr << "[Step1] CreateIoCompletionPort failed, error: " << GetLastError() << std::endl;
        WSACleanup();
        return -1;
    }
    std::cout << "[Step1] IOCP handle created successfully." << std::endl;

    // ================================================================
    // 2.3 创建监听Socket(支持重叠IO)
    // ================================================================
    SOCKET listen_socket = WSASocket(
        AF_INET,            // IPv4
        SOCK_STREAM,        // 流式
        0,                  // 协议（0 = TCP）
        nullptr,            // 协议信息
        0,                  // 保留
        WSA_FLAG_OVERLAPPED // 关键：支持重叠 I/O
    );

    if(listen_socket == INVALID_SOCKET)
    {
        std::cerr << "[Step2] WSASocket failed, error: " << WSAGetLastError() << std::endl;
        CloseHandle(g_iocp_handle);
        WSACleanup();
        return -1;
    }
    std::cout << "[Step2] Listen socket created (overlapped)." << std::endl;

    // ================================================================
    // 2.4 绑定地址和端口
    // ================================================================
    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;            // 监听所有网卡
    server_addr.sin_port = htons(DEFAULT_PORT);

    if(SOCKET_ERROR == bind(listen_socket, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)))
    {
        std::cerr << "[Step2] bind failed, error: " << WSAGetLastError() << std::endl;
        closesocket(listen_socket);
        CloseHandle(g_iocp_handle);
        WSACleanup();
        return -1;
    }
    std::cout << "[Step2] Bound to port " << DEFAULT_PORT << "." << std::endl;

    // ================================================================
    // 2.5 开始监听
    // ================================================================
    if(SOCKET_ERROR == listen(listen_socket, SOMAXCONN))
    {
        std::cerr << "[Step2] listen failed, error: " << WSAGetLastError() << std::endl;
        closesocket(listen_socket);
        CloseHandle(g_iocp_handle);
        WSACleanup();
        return -1;
     }
    std::cout << "[Step2] Listening..." << std::endl;
    
    // ================================================================
    // 2.6 【新增】将监听 Socket 关联到完成端口
    // ================================================================
    // 虽然当前我们用 accept（同步方式）接受连接，
    // 但将监听 Socket 绑定到 IOCP 是后续使用 AcceptEx 的前提。
    // 这里先做绑定，为后续扩展做准备。
    HANDLE iocp_result = CreateIoCompletionPort(
        reinterpret_cast<HANDLE>(listen_socket),
        g_iocp_handle,
        COMP_KEY_LISTEN_SOCKET,  // 完成键：标识这是监听 Socket
        0
    );

    if (!iocp_result) {
        std::cerr << "[Step2] Bind listen socket to IOCP failed, error: " << GetLastError() << std::endl;
        closesocket(listen_socket);
        CloseHandle(g_iocp_handle);
        WSACleanup();
        return -1;
    }
    std::cout << "[Step2] Listen socket bound to IOCP." << std::endl;

    // ================================================================
    // 2.7 启动工作线程
    // ================================================================
    g_worker_threads.reserve(WORKER_THREAD_COUNT);
    for(int i = 0; i < WORKER_THREAD_COUNT; ++i)
    {
        g_worker_threads.emplace_back(WorkerThread);
    }

    std::stringstream ss;
    ss << "[Step2] Started " << WORKER_THREAD_COUNT << " worker threads." << std::endl;
    std::cout << ss.str();

    // ================================================================
   // 2.8 运行
   // ================================================================
    std::cout << "[Step2] Server is running. Press Enter to stop..." << std::endl;
    
    ss.str("");
    ss.clear();
    ss << "[Step2] Use telnet 127.0.0.1 " << DEFAULT_PORT << " to test." << std::endl;
    std::cout << ss.str();

    std::cin.get();
    // ================================================================
    // 9. 【新增】发送退出事件，唤醒所有工作线程
    // ================================================================
    std::cout << "[Step3] Sending exit events to workers..." << std::endl;
    // 向完成端口发送 WORKER_THREAD_COUNT 个退出事件
    // 每个工作线程收到一个退出事件后都会退出
    for(auto& th : g_worker_threads)
    {
        PostQueuedCompletionStatus(
            g_iocp_handle,              // 完成端口句柄
            0,                          // 传输字节数（无意义）
            COMP_KEY_EXIT,              // 完成键：标识为退出事件
            nullptr                     // OVERLAPPED（无意义）
        );
    }

    // 停止服务器
    g_server_running = false;
    for(auto& th : g_worker_threads)
    {
        if(th.joinable())
        {
            th.join();
        }
    }

    // 清理资源
    closesocket(listen_socket);
    CloseHandle(g_iocp_handle);
    WSACleanup();

    std::cout << "[Step2] Cleanup complete." << std::endl;
    return 0;
}