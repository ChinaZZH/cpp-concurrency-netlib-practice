#include <thread>
#include <iostream>
#include <mutex>
#include <queue>
#include <chrono>
#include <condition_variable>


std::queue<int> qTest;
std::mutex mtx;
std::condition_variable cv;
const int MAX_QUEUE_COUNT = 5;

void proceducer()
{
	for (int i = 0; i < 10; ++i)
	{
		std::this_thread::sleep_for(std::chrono::microseconds(100));

		{
			std::unique_lock<std::mutex> lk(mtx);
			cv.wait(lk, [] { return qTest.size() < MAX_QUEUE_COUNT; });
			qTest.push(i);
			cv.notify_one();
		}

		
		std::string str("Proceduced=");
		str.append(std::to_string(i));
		str.append("\r\n");
		std::cout << str.c_str();
	}
}


void consumer()
{
	for (int i = 0; i < 10; ++i)
	{
		std::this_thread::sleep_for(std::chrono::microseconds(100));
		int nGetValue = 0;
		{
			std::unique_lock<std::mutex> lk(mtx);
			cv.wait(lk, [] { return !qTest.empty(); });
			nGetValue = qTest.front();
			qTest.pop();
			cv.notify_one();
		}

		
		std::string str("Consumed=");
		str.append(std::to_string(nGetValue));
		str.append("\r\n");
		std::cout << str.c_str();
	}
}


int main()
{
	std::thread t1(proceducer);
	std::thread t2(consumer);
	t1.join();
	t2.join();
	return 0;
}