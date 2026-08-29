#include <winsock2.h>
#include <windows.h>
#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include <sstream>


#pragma comment(lib, "ws2_32.lib")

// ---------- 常量 ----------
const int WORKER_THREAD_COUNT = (std::thread::hardware_concurrency() * 2); // 工作线程数，通常 = CPU 核心数 * 2


// ---------- 全局变量 ----------
HANDLE g_iocp_handle = nullptr;                      // 完成端口句柄

std::atomic<bool> g_server_running{ true };         // 服务器运行标志

std::vector<std::thread> g_worker_threads;          // 工作线程列表

// ---------- 工作线程函数 ----------
void WorkerThread()
{
    std::stringstream ss;
    ss << "[Worker] Thread " << std::this_thread::get_id() << " started." << std::endl;
    std::cout << ss.str();
    while(g_server_running)
    {
        // 暂时只做等待，后续会填充 GetQueuedCompletionStatus
        // 这里先用 Sleep 模拟等待，避免 CPU 空转
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::stringstream server_end;
    server_end << "[Worker] Thread " << std::this_thread::get_id() << " stoped." << std::endl;
    std::cout << server_end.str();
}


// ---------- 主函数 ----------
int main() {
    std::cout << "=== IOCP Step 1: Framework ===" << std::endl;

    // ================================================================
    // 1.1 Winsock 初始化
    // ================================================================
    WSADATA wsaData;
    int ret = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if(ret != 0) {
        std::cerr << "[Step1] WSAStartup failed, error:" << ret << std::endl;
        return -1;
    }
    std::cout << "[Step1] Winsock initialized successfully." << std::endl;
    
    // ================================================================
    // 1.2 创建完成端口
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
    // 1.3 启动工作线程
    // ================================================================
    g_worker_threads.reserve(WORKER_THREAD_COUNT);
    for(int i = 0; i < WORKER_THREAD_COUNT; ++i)
    {
        g_worker_threads.emplace_back(WorkerThread);
    }


    // ================================================================
    // 1.4 运行一段时间后退出（演示用）
    // ================================================================
    std::cout << "[Step1] Server is running. Press Enter to stop..." << std::endl;
    std::cin.get();

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
    CloseHandle(g_iocp_handle);
    WSACleanup();

    std::cout << "[Step1] Cleanup complete. Exiting." << std::endl;
    return 0;
}