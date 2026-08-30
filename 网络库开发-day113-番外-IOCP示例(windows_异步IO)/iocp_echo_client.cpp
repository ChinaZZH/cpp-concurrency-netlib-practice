#include <winsock2.h>
#include <windows.h>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <chrono>
#include <atomic>
#include <sstream>

#define _WIN32_WINNT 0x0600
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

constexpr const char* SERVER_IP = "127.0.0.1";
constexpr int SERVER_PORT = 8888;
constexpr int BUFFER_SIZE = 4096;

std::atomic<int> g_success_count{ 0 };
std::atomic<int> g_fail_count{ 0 };

// ---------- 发送并接收数据 ----------
bool SendAndReceive(SOCKET sock, const std::string& message)
{
	// 1. 发送数据
	int ret = send(sock, message.c_str(), static_cast<int>(message.size()), 0);
	if(SOCKET_ERROR == ret)
	{
		std::cerr << "[Client] send failed, error: " << WSAGetLastError() << std::endl;
		return false;
	}
	std::cout << "[Client] Sent: " << message << std::endl;

	// 2. 接收回显数据
	char buffer[BUFFER_SIZE] = { 0 };
	int received = recv(sock, buffer, BUFFER_SIZE-1, 0);
	if (SOCKET_ERROR == received)
	{
		std::cerr << "[Client] recv failed, error: " << WSAGetLastError() << std::endl;
		return false;
	}

	if (0 == received)
	{
		std::cerr << "[Client] Server closed connection." << std::endl;
		return false;
	}

	buffer[received] = '\0';
	std::string echo(buffer, received);
	std::cout << "[Client] Received echo: " << echo << std::endl;

	// 3. 验证回显是否正确
	bool success = (echo == message);
	if (!success) {
		std::cerr << "[Client] Echo mismatch! Expected: " << message
			<< ", Got: " << echo << std::endl;
	}
	return success;
}

// ---------- 客户端线程函数 ----------
void ClientThread(int thread_id, int message_count)
{
	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
	if(sock == INVALID_SOCKET)
	{
		std::cerr << "[Thread " << thread_id << "] socket failed" << std::endl;
		g_fail_count.fetch_add(1);
		return;
	}

	// 连接服务器
	sockaddr_in server_addr{};
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(SERVER_PORT);
	inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

	if (SOCKET_ERROR == connect(sock, (sockaddr*)&server_addr, sizeof(server_addr)))
	{
		std::cerr << "[Thread " << thread_id << "] connect failed, error:" << WSAGetLastError() << std::endl;
		closesocket(sock);
		g_fail_count.fetch_add(1);
		return;
	}

	std::cout << "[Thread " << thread_id << "] Connected to server." << std::endl;
	// 发送消息并验证回显
	for(int i = 0; i < message_count; ++i)
	{
		std::stringstream ss;
		ss << "Hello from thread " << thread_id << " , msg " << (i + 1);
		if(SendAndReceive(sock, ss.str()))
		{
			g_success_count.fetch_add(1);
		}
		else {
			g_fail_count.fetch_add(1);
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	closesocket(sock);
	std::cout << "[Thread " << thread_id << "] Finished." << std::endl;
}

// ---------- 主函数 ----------
int main()
{
	std::cout << "=== IOCP Echo Client ===" << std::endl;

	// 1. Winsock 初始化
	WSADATA wsaData;
	if(0 != WSAStartup(MAKEWORD(2, 2), &wsaData))
	{
		std::cerr << "[Client] WSAStartup failed." << std::endl;
		return -1;
	}

	// ================================================================
	// 配置参数
	// ================================================================
	const int THREAD_COUNT = 4;				// 并发线程数
	const int MESSAGES_PER_THREAD = 5;		// 每个线程发送的消息数

	std::cout << "[Client] Starting " << THREAD_COUNT << " threads, "
		<< "each sending " << MESSAGES_PER_THREAD << " messages." << std::endl;


	// 2. 启动多个客户端线程
	std::vector<std::thread> threads;
	threads.reserve(THREAD_COUNT);
	for(int i = 0; i < THREAD_COUNT; ++i)
	{
		threads.emplace_back(ClientThread, i+1, MESSAGES_PER_THREAD);
	}


	// 3. 等待所有线程完成
	for(auto& th : threads)
	{
		if(th.joinable())
		{
			th.join();
		}
	}


	 // 4. 输出统计结果
	std::cout << "========================================" << std::endl;
	std::cout << "[Client] Success: " << g_success_count.load() << std::endl;
	std::cout << "[Client] Failed: " << g_fail_count.load() << std::endl;
	std::cout << "[Client] Total: " << (g_success_count.load() + g_fail_count.load()) << std::endl;

	WSACleanup();

	if (g_fail_count.load() == 0) {
		std::cout << "[Client] All tests PASSED!" << std::endl;
		return 0;
	}
	else {
		std::cout << "[Client] Some tests FAILED." << std::endl;
		return 1;
	}


	return 0;
}