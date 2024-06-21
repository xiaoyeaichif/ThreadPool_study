
#include<iostream>
#include<chrono>
#include "threadpool.h"

using namespace std;
class MyTask : public Task {
public:
	MyTask(int begin,int end)
		:begin_(begin)
		,end_(end)
	{}

	//问题一：怎么设计run函数的返回值，可以表示任意的类型
	//C++17 Any类型------->是可以任意接收其他类型的基类
	//java 和 python 中有个类交object类，是所有类的基类
	Any run()
	{
		std::cout << "tid: " << std::this_thread::get_id() << "begin!" << std::endl;
		int sum = 0;
		for (int i = begin; i < end; i++)
		{
			sum += i;
		}
		std::cout << "tid: " << std::this_thread::get_id() << "end!" << std::endl;
		return sum;
	}
private:
	int begin_;
	int end_;
};


int main_xiancehngchi()
{
	ThreadPool pool;
	int num = 4;
	pool.start(num);//初始化num个线程

	//初始化任务个数
	int task = 10;
	for (int i = 0; i < task; i++)
	{
		pool.submitTask(std::make_shared<MyTask>());
	}
	

	getchar();
}