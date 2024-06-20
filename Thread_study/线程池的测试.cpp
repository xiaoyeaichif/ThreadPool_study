
#include<iostream>
#include<chrono>
#include "threadpool.h"

using namespace std;
class MyTask : public Task {
public:
	void run()
	{
		std::cout << "tid: " << std::this_thread::get_id() << "begin!" << std::endl;
		//˯������
		std::this_thread::sleep_for(std::chrono::seconds(5));
		std::cout << "tid: " << std::this_thread::get_id() << "end!" << std::endl;
	}
};


int main()
{
	ThreadPool pool;
	int num = 4;
	pool.start(num);//��ʼ��num���߳�

	//��ʼ���������
	int task = 10;
	for (int i = 0; i < task; i++)
	{
		pool.submitTask(std::make_shared<MyTask>());
	}
	

	getchar();
}