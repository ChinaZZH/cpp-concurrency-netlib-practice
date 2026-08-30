#include <winsock2.h>
#include <windows.h>
#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include <sstream>
#include <mswsock.h>   // AcceptEx 需要的扩展函数
#include <string>


#pragma comment(lib, "ws2_32.lib")

// ---------- 常量 ----------
const int DEFAULT_PORT = 8888;
//const int WORKER_THREAD_COUNT = (std::thread::hardware_concurrency() * 2); // 工作线程数，通常 = CPU 核心数 * 2
const int WORKER_THREAD_COUNT = 2;
const int BUFFER_SIZE = 4096;

// ---------- 完成键 ----------
enum CompletionKey {
    COMP_KEY_LISTEN_SOCKET = 0,
    COMP_KEY_CLIENT_SOCKET = 1,
    COMP_KEY_EXIT   = 999,          // 退出事件
};

// ---------- 前向声明 ----------
struct PER_SOCKET_CONTEXT;

// ---------- 每个 I/O 操作的上下文 ---------
struct PER_IO_CONTEXT {
    OVERLAPPED overlapped;
    WSABUF wsa_buf;
    char buffer[BUFFER_SIZE];
    SOCKET client_socket; // AcceptEx 专用：存储新连接的 Socket
    PER_SOCKET_CONTEXT* owner;   // 指向所属的套接字上下文

    PER_IO_CONTEXT() {
        ZeroMemory(&overlapped, sizeof(OVERLAPPED));
        ZeroMemory(&wsa_buf, sizeof(WSABUF));
        ZeroMemory(buffer, BUFFER_SIZE);
        wsa_buf.buf = buffer;
        wsa_buf.len = BUFFER_SIZE;
        client_socket = INVALID_SOCKET;
        owner = nullptr;
    }
};

// ---------- 每个套接字的上下文 ----------
struct PER_SOCKET_CONTEXT {
    SOCKET socket;
    PER_IO_CONTEXT recv_io;
    PER_IO_CONTEXT send_io;
    bool is_closed;

    PER_SOCKET_CONTEXT(SOCKET sock) : socket(sock), is_closed(false) {
        recv_io.owner = this;
        recv_io.client_socket = sock;
        send_io.owner = this;
        send_io.client_socket = sock;
    }
};

// ---------- 全局变量 ----------
HANDLE g_iocp_handle = nullptr;                      // 完成端口句柄

SOCKET g_listen_sock = INVALID_SOCKET;

std::atomic<bool> g_server_running{ true };         // 服务器运行标志

std::vector<std::thread> g_worker_threads;          // 工作线程列表

std::atomic<int> g_active_workers{ 0 };


LPFN_ACCEPTEX g_AcceptEx = nullptr; // ---------- AcceptEx 函数指针 ----------

// ---------- 前置声明 ----------
void HandleAcceptCompletion(PER_IO_CONTEXT* io_ctx, DWORD bytes_transfterred);
void HandleClientIoCompletion(PER_IO_CONTEXT* io_ctx, DWORD bytes_transfterred);
void PostAccept();


// ---------- 辅助：打印接收数据 ----------
void PrintReceivedData(const char* data, DWORD len) {
    for (DWORD i = 0; i < len; ++i) {
        char c = data[i];
        if (c >= 32 && c <= 126) {
            std::cout << c;
        }
        else {
            std::cout << "[0x" << std::hex << (int)(unsigned char)c << std::dec << "]";
        }
    }
}

// ---------- 工作线程主循环----------
void WorkerThread()
{
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

        // 从 OVERLAPPED 获取 PER_IO_CONTEXT
        PER_IO_CONTEXT* io_ctx = CONTAINING_RECORD(overlapped, PER_IO_CONTEXT, overlapped);
        switch(completion_key)
        {
        case COMP_KEY_LISTEN_SOCKET:
            // AcceptEx 完成事件
            HandleAcceptCompletion(io_ctx, bytes_transferred);
            break;
        case COMP_KEY_CLIENT_SOCKET: 
            HandleClientIoCompletion(io_ctx, bytes_transferred);
            break;
        case COMP_KEY_EXIT:
            std::cout << "[Worker] Exit event received." << std::endl;
            g_server_running = false;
            delete io_ctx;
            break;
        default:
            std::cout << "[Worker] Unknown completion key: " << completion_key << std::endl;
            delete io_ctx;
            break;
        }
    }

    {
        ss.str(std::string());
        ss.clear();
        ss << "[Worker] Thread " << std::this_thread::get_id() << " stoped." << std::endl;
        std::cout << ss.str();
    }
}

// ---------- 处理 AcceptEx 完成 ----------
void HandleAcceptCompletion(PER_IO_CONTEXT* io_ctx, DWORD bytes_transferred) {
    SOCKET client_sock = io_ctx->client_socket;

    // 获取客户端地址信息（可选，这里简化）
    std::cout << "[Server] New client connected, socket: " << client_sock << std::endl;
    // 1. 将客户端 Socket 绑定到完成端口
    HANDLE io_result = CreateIoCompletionPort(reinterpret_cast<HANDLE>(client_sock), g_iocp_handle, COMP_KEY_CLIENT_SOCKET, 0);
    if(!io_result) {
        std::cerr << "[Server] Bind client to IOCP failed, error: " << GetLastError() << std::endl;
        closesocket(client_sock);
        delete io_ctx;
        // 投递下一个 AcceptEx
        PostAccept();
        return;
    }

    // 2. 投递第一个 WSARecv（接收客户端数据）
    // 为客户端创建新的 I/O 上下文
    PER_SOCKET_CONTEXT* sock_ctx = new PER_SOCKET_CONTEXT(client_sock);
    // 3. 投递第一个 WSARecv
    ZeroMemory(&sock_ctx->recv_io.overlapped, sizeof(OVERLAPPED));
    sock_ctx->recv_io.wsa_buf.buf = sock_ctx->recv_io.buffer;
    sock_ctx->recv_io.wsa_buf.len = BUFFER_SIZE;
    sock_ctx->recv_io.client_socket = client_sock;

    DWORD flags = 0;
    int ret = WSARecv(client_sock, &(sock_ctx->recv_io.wsa_buf),1, nullptr, &flags, &(sock_ctx->recv_io.overlapped), nullptr);
    if (SOCKET_ERROR == ret) {
        DWORD error = WSAGetLastError();
        if (error != WSA_IO_PENDING) {
            std::cerr << "[Server] WSARecv failed, error: " << error << std::endl;
            closesocket(client_sock);
            delete sock_ctx;
            delete io_ctx;
            PostAccept();
            return;
        }
    }

    // 3. 投递下一个 AcceptEx（继续接受新连接）
    delete io_ctx;  // 释放 AcceptEx 的上下文
    PostAccept();
}

// ---------- 处理 Client IO 读写请求 ----------
void HandleClientIoCompletion(PER_IO_CONTEXT* io_ctx, DWORD bytes_transferred)
{
    if (!io_ctx)
    {
        std::cerr << "[Worker] io_ctx is null!" << std::endl;
        return;
    }

    PER_SOCKET_CONTEXT* sock_ctx = io_ctx->owner;
    if (!sock_ctx) {
        std::cerr << "[Worker] sock_ctx is null!" << std::endl;
        delete io_ctx;
        return;
    }

    if(bytes_transferred == 0) {
        std::cout << "[Server] Client disconnected, socket: " << sock_ctx->socket << std::endl;
        closesocket(sock_ctx->socket);
        sock_ctx->is_closed = true;
        delete sock_ctx;
        return;
    }

    // 发送完成
    if (io_ctx == &sock_ctx->send_io) {
        std::cout << "[Server] Sent " << bytes_transferred << " bytes, socket: " << sock_ctx->socket << std::endl;
    }
    else if (io_ctx == &sock_ctx->recv_io) {
        // 接收完成
        std::cout << "[Server] Received " << bytes_transferred << " bytes from socket " << sock_ctx->socket << ": ";
        PrintReceivedData(sock_ctx->recv_io.buffer, bytes_transferred);
        std::cout << std::endl;

        
        std::string receiveData(sock_ctx->recv_io.buffer, bytes_transferred);
        

        // ===== 2. 投递下一个 WSARecv（先投递，让接收持续进行） =====
        ZeroMemory(&sock_ctx->recv_io.overlapped, sizeof(OVERLAPPED));
        sock_ctx->recv_io.wsa_buf.buf = sock_ctx->recv_io.buffer;
        sock_ctx->recv_io.wsa_buf.len = BUFFER_SIZE;
        sock_ctx->recv_io.client_socket = sock_ctx->socket;

        DWORD flags = 0;
        int ret_recv = WSARecv(
            sock_ctx->socket,
            &sock_ctx->recv_io.wsa_buf,
            1,
            nullptr,
            &flags,
            &sock_ctx->recv_io.overlapped,
            nullptr
        );

        if (ret_recv == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
            std::cerr << "[Server] WSARecv (next) failed, error: " << WSAGetLastError() << std::endl;
            closesocket(sock_ctx->socket);
            sock_ctx->is_closed = true;
            delete sock_ctx;
            return;
        }

        // ===== 1. Echo 发送（使用 recv_io 中的数据） =====
        sock_ctx->send_io.wsa_buf.buf = receiveData.data();
        sock_ctx->send_io.wsa_buf.len = bytes_transferred;
        int ret_send = WSASend(
            sock_ctx->socket,
            &sock_ctx->send_io.wsa_buf,
            1,
            nullptr,
            0,
            &sock_ctx->send_io.overlapped,
            nullptr
        );
        if (ret_send == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
            std::cerr << "[Server] WSASend failed, error: " << WSAGetLastError() << std::endl;
            closesocket(sock_ctx->socket);
            sock_ctx->is_closed = true;
            delete sock_ctx;
            return;
        }
    }
}

// ---------- 投递 AcceptEx ----------
void PostAccept() {
    if(!g_server_running)
    {
        return;
    }

    // 创建用于 AcceptEx 的客户端 Socket（暂不绑定地址）
    SOCKET accept_sock = WSASocket(AF_INET, SOCK_STREAM, 0, nullptr, 0, WSA_FLAG_OVERLAPPED);
    if (accept_sock == INVALID_SOCKET) {
        std::cerr << "[Server] WSASocket for AcceptEx failed, error: " << WSAGetLastError() << std::endl;
        return;
    }

    // 准备缓冲区（AcceptEx 需要额外的地址缓冲区）
   // 简化：使用 io_ctx->buffer 存储地址数据
    PER_IO_CONTEXT* io_ctx = new PER_IO_CONTEXT();
    io_ctx->client_socket = accept_sock;

    DWORD bytes_received = 0;
    BOOL result = g_AcceptEx(
        g_listen_sock, 
        accept_sock, 
        io_ctx->buffer,
        0, // 0 代表不接收数据，只接受连接
        sizeof(SOCKADDR_IN) + 16, 
        sizeof(SOCKADDR_IN) + 16, 
        &bytes_received, 
        &io_ctx->overlapped
    );

    if(FALSE == result)
    {
        DWORD error = WSAGetLastError();
        if (error != WSA_IO_PENDING) {
            std::cerr << "[Server] AcceptEx failed, error: " << error << std::endl;
            closesocket(accept_sock);
            delete io_ctx;
            return;
        }
    }

    std::cout << "[Server] AcceptEx posted." << std::endl;
}

// ---------- 加载 AcceptEx ----------
bool LoadAcceptEx(SOCKET listen_socket)
{
    GUID guidAcceptEx = WSAID_ACCEPTEX;
    DWORD bytes = 0;
    int ret = WSAIoctl(
        listen_socket,
        SIO_GET_EXTENSION_FUNCTION_POINTER,
        &guidAcceptEx,
        sizeof(guidAcceptEx),
        &g_AcceptEx,
        sizeof(g_AcceptEx),
        &bytes,
        nullptr,
        nullptr
    );

    if (SOCKET_ERROR == ret)
    {
        std::cerr << "[Server] WSAIoctl for AcceptEx failed, error: " << WSAGetLastError() << std::endl;
        return false;
    }

    return true;
}

// ---------- 主函数 ----------
int main() {
    std::cout << "=== IOCP Step 4: AcceptEx + IOCP ===" << std::endl;

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
    g_listen_sock = WSASocket(
        AF_INET,            // IPv4
        SOCK_STREAM,        // 流式
        0,                  // 协议（0 = TCP）
        nullptr,            // 协议信息
        0,                  // 保留
        WSA_FLAG_OVERLAPPED // 关键：支持重叠 I/O
    );

    if(g_listen_sock == INVALID_SOCKET)
    {
        std::cerr << "[Step2] WSASocket failed, error: " << WSAGetLastError() << std::endl;
        CloseHandle(g_iocp_handle);
        WSACleanup();
        return -1;
    }
    std::cout << "[Step2] Listen socket created (overlapped)." << std::endl;

    // 加载AcceptEx
    if(!LoadAcceptEx(g_listen_sock))
    {
        closesocket(g_listen_sock);
        CloseHandle(g_iocp_handle);
        WSACleanup();
        return -1;
    }
    std::cout << "[Step4] AcceptEx loaded." << std::endl;
    
    // ================================================================
    // 2.4 绑定地址和端口
    // ================================================================
    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;            // 监听所有网卡
    server_addr.sin_port = htons(DEFAULT_PORT);

    if(SOCKET_ERROR == bind(g_listen_sock, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)))
    {
        std::cerr << "[Step2] bind failed, error: " << WSAGetLastError() << std::endl;
        closesocket(g_listen_sock);
        CloseHandle(g_iocp_handle);
        WSACleanup();
        return -1;
    }
    std::cout << "[Step2] Bound to port " << DEFAULT_PORT << "." << std::endl;

    // ================================================================
    // 2.5 开始监听
    // ================================================================
    if(SOCKET_ERROR == listen(g_listen_sock, SOMAXCONN))
    {
        std::cerr << "[Step2] listen failed, error: " << WSAGetLastError() << std::endl;
        closesocket(g_listen_sock);
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
        reinterpret_cast<HANDLE>(g_listen_sock),
        g_iocp_handle,
        COMP_KEY_LISTEN_SOCKET,  // 完成键：标识这是监听 Socket
        0
    );

    if (!iocp_result) {
        std::cerr << "[Step2] Bind listen socket to IOCP failed, error: " << GetLastError() << std::endl;
        closesocket(g_listen_sock);
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

    // 9. 投递第一个 AcceptEx 启动监听
    PostAccept();
    std::cout << "[Step4] First AcceptEx posted." << std::endl;

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
    closesocket(g_listen_sock);
    CloseHandle(g_iocp_handle);
    WSACleanup();

    std::cout << "[Step2] Cleanup complete." << std::endl;
    return 0;
}