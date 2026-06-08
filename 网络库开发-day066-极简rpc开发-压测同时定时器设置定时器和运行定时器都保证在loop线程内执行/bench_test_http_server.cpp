#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include <algorithm>
#include <numeric>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

class HttpClient {
public:
    HttpClient(const std::string& host, int port)
        : host_(host), port_(port), sock_(-1) {
    }

    ~HttpClient() { close(); }

    bool connect() {
        if (sock_ >= 0) close();
        sock_ = socket(AF_INET, SOCK_STREAM, 0);
        if (sock_ < 0) return false;
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port_);
        if (inet_pton(AF_INET, host_.c_str(), &addr.sin_addr) <= 0) {
            close();
            return false;
        }
        if (::connect(sock_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            close();
            return false;
        }
        return true;
    }

    bool send_post(const std::string& path, const std::string& body, std::string& response) {
        if (sock_ < 0) return false;
        std::string request = "POST " + path + " HTTP/1.1\r\n"
            "Host: " + host_ + "\r\n"
            "Content-Type: application/json\r\n"
            "Content-Length: " + std::to_string(body.size()) + "\r\n"
            "Connection: keep-alive\r\n\r\n" + body;
        if (::send(sock_, request.c_str(), request.size(), 0) < 0) return false;

        char buffer[4096];
        std::string raw;
        bool headers_complete = false;
        size_t content_length = 0;
        size_t header_end_pos = 0;   // 保存 "\r\n\r\n" 的位置

        while (true) {
            ssize_t n = recv(sock_, buffer, sizeof(buffer) - 1, 0);
            if (n <= 0) return false;
            buffer[n] = '\0';
            raw.append(buffer, n);

            if (!headers_complete) {
                size_t pos = raw.find("\r\n\r\n");
                if (pos != std::string::npos) {
                    headers_complete = true;
                    header_end_pos = pos;
                    // 解析 Content-Length
                    size_t cl_pos = raw.find("Content-Length:");
                    if (cl_pos != std::string::npos) {
                        size_t start = cl_pos + 15;
                        while (start < raw.size() && (raw[start] == ' ' || raw[start] == '\t')) ++start;
                        content_length = std::stoul(raw.substr(start));
                    }
                    else {
                        // 没有 Content-Length，无法确定 body 长度，只能返回全部已读数据
                        response = raw;
                        return true;
                    }
                }
            }

            if (headers_complete) {
                // 预期总长度 = 头部结束位置 + 4（"\r\n\r\n"） + body 长度
                size_t expected = header_end_pos + 4 + content_length;
                if (raw.size() >= expected) {
                    response = raw;
                    return true;
                }
            }
        }
    }

    void close() {
        if (sock_ >= 0) {
            ::close(sock_);
            sock_ = -1;
        }
    }

private:
    std::string host_;
    int port_;
    int sock_;
};

void worker(int thread_id, int requests, std::vector<uint64_t>& latencies) {
    
    const std::string host = "127.0.0.1";
    const int port = 8888;          // 你的 HTTP 服务端口
    const std::string path = "/add";
    const std::string body = R"({"a":10,"b":32})"; // "{\"a\":10,\"b\":32}";

    HttpClient client(host, port);
    if (!client.connect()) {
        std::cerr << "Thread " << thread_id << " connection failed\n";
        return;
    }
    

    latencies.reserve(requests);
    for (int i = 0; i < requests; ++i) {
        auto start = std::chrono::steady_clock::now();
        std::string resp;
        auto overall_start = std::chrono::steady_clock::now();
        if (client.send_post(path, body, resp)) {
            auto overall_end = std::chrono::steady_clock::now();
            auto cost_sec = std::chrono::duration_cast<std::chrono::microseconds>(overall_end - overall_start).count();
            latencies.emplace_back(cost_sec);
        }
    }
    
}

int ClientPressTest(int threads, int req_per_threads) {
   
    std::vector<std::thread> pool;
    std::vector<std::vector<uint64_t>> all_latencies(threads);

    auto overall_start = std::chrono::steady_clock::now();
    for (int i = 0; i < threads; ++i) {
        pool.emplace_back(worker, i, req_per_threads, std::ref(all_latencies[i]));
    }
    for (auto& t : pool) t.join();

    auto overall_end = std::chrono::steady_clock::now();
    double total_sec = std::chrono::duration<double>(overall_end - overall_start).count();
    int total_req = threads * req_per_threads;
    double qps = total_req / total_sec;

    // 合并所有延迟
    std::vector<uint64_t> all_us;
    for (auto& latencies : all_latencies)
    {
        all_us.insert(all_us.end(), latencies.begin(), latencies.end());
    }

    double avg_us = std::accumulate(all_us.begin(), all_us.end(), 0.0) / all_us.size();
    std::sort(all_us.begin(), all_us.end());

    auto precentile_us_func = [&](double percent) -> uint64_t {
        if (all_us.empty())
        {
            return 0;
        }

        size_t idx = percent * (all_us.size() - 1);
        if (idx >= all_us.size())
        {
            idx = all_us.size() - 1;
        }

        return all_us[idx];
        };

    std::cout << "all_us vector size(): " << all_us.size() << std::endl;
    std::cout << "Total request: " << total_req << std::endl;
    std::cout << "Total time(second): " << total_sec << std::endl;
    std::cout << "QPS: " << qps << std::endl;
    std::cout << "Average latency: " << avg_us << std::endl;

    std::cout << "P50 latency: " << precentile_us_func(0.50) << std::endl;
    std::cout << "P90 latency: " << precentile_us_func(0.90) << std::endl;
    std::cout << "P99 latency: " << precentile_us_func(0.99) << std::endl;
    std::cout << "P999 latency: " << precentile_us_func(0.999) << std::endl;


    return 0;
}



int main()
{
    
    std::cout << "start 50 thread and req_per_threads 10000: " << std::endl;
    ClientPressTest(50, 10000);
    std::cout << std::endl << std::endl;

    std::cout << "start 20 thread and req_per_threads 10000: " << std::endl;
    ClientPressTest(20, 10000);
    std::cout << std::endl << std::endl;


    std::cout << "start 10 thread and req_per_threads 10000: " << std::endl;
    ClientPressTest(10, 10000);
    std::cout << std::endl << std::endl;


    std::cout << "start 5 thread and req_per_threads 10000: " << std::endl;
    ClientPressTest(5, 10000);
    std::cout << std::endl << std::endl;



    std::cout << "start 1 thread and req_per_threads 10000: " << std::endl;
    ClientPressTest(1, 10000);
    std::cout << std::endl << std::endl;
    return 0;
}