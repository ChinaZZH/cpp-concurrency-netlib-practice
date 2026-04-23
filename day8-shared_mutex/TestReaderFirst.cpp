#include <iostream>
#include <thread>
#include <shared_mutex>
#include <chrono>

std::shared_mutex mtx;

void reader(int id) {
    std::shared_lock lock(mtx);
    std::cout << "Reader " << id << " start\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::cout << "Reader " << id << " end\n";
}

void writer(int id) {
    std::unique_lock lock(mtx);
    std::cout << "Writer " << id << " start\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    std::cout << "Writer " << id << " end\n";
}

int main() {
    std::thread t1(reader, 1);
    std::this_thread::sleep_for(std::chrono::milliseconds(10)); // 确保 t1 获得读锁
    std::thread t2(writer, 1);
    std::this_thread::sleep_for(std::chrono::milliseconds(10)); // t2 开始等待
    std::thread t3(reader, 2);
    t1.join(); t2.join(); t3.join();
    return 0;
}