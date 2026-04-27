#include <thread>
#include <iostream>
#include <mutex>
#include <queue>
#include <chrono>


std::queue<int> qTest;
std::mutex mtx;
const int MAX_QUEUE_COUNT = 5;

void proceducer()
{
	for(int i = 0; i < 10; )
	{
		std::this_thread::sleep_for(std::chrono::microseconds(100));
		if(qTest.size() < MAX_QUEUE_COUNT)
		{
			{
				std::lock_guard<std::mutex> lk(mtx);
				qTest.push(i);
				
			}
			
			std::string str("Proceduced=");
			str.append(std::to_string(i));
			str.append("\r\n");
			std::cout << str.c_str();

			++i;
		}
		else {
			std::this_thread::sleep_for(std::chrono::microseconds(10));
		}
	}
}


void consumer()
{
	for (int i = 0; i < 10; )
	{
		std::this_thread::sleep_for(std::chrono::microseconds(100));
		if(!qTest.empty())
		{
			int nGetValue = 0;
			{
				std::lock_guard<std::mutex> lk(mtx);
				nGetValue = qTest.front();
				qTest.pop();
				//std::cout << "Consumed=" << nGetValue << std::endl;
			}
			
			std::string str("Consumed=");
			str.append(std::to_string(nGetValue));
			str.append("\r\n");
			std::cout << str.c_str();

			++i;
		}
		else {
			std::this_thread::sleep_for(std::chrono::microseconds(10));
		}
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