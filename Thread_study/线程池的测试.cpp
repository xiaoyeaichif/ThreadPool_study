
#include<iostream>
#include<chrono>
#include "threadpool.h"


using Ulong = unsigned long long;

using namespace std;

//用户的实现
class MyTask : public Task {
public:
	MyTask();
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
		std::this_thread::sleep_for(std::chrono::seconds(4));
		Ulong sum = 0;
		for (Ulong i = begin_; i <= end_; i++)
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


int main()
{
	{

		ThreadPool pool;
		int num = 4;
		//用户自己设置线程池的工作模式
		pool.setMode(PoolMode::MODE_CATCH);
		pool.start(num);//初始化num个线程

		//记录线程运行的开始时间
		//auto start_time = std::chrono::high_resolution_clock::now();

		Result res1 = pool.submitTask(std::make_shared<MyTask>(1, 10000));
		Result res2 = pool.submitTask(std::make_shared<MyTask>(10001, 20000));
		Result res3 = pool.submitTask(std::make_shared<MyTask>(20001, 30000));
		pool.submitTask(std::make_shared<MyTask>(20001, 30000));
		pool.submitTask(std::make_shared<MyTask>(20001, 30000));
		pool.submitTask(std::make_shared<MyTask>(20001, 30000));

		//get返回一个Any类型
		Ulong sum1 = res1.get().cast_<Ulong>();
		Ulong sum2 = res2.get().cast_<Ulong>();
		Ulong sum3 = res3.get().cast_<Ulong>();

		//记录子线程结束时间
		//auto end_time = std::chrono::high_resolution_clock::now();
		//两段时间的差值
		//auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
		// 输出子线程任务总时间
		//std::cout << "Time taken by slave threads: " << duration << " ms" << std::endl;


		//Master ---- Slave模型
		//Master用来分解任务，然后给各个Slave线程分配任务
		//等待各个Slave线程执行完任务，返回结果
		//Master线程合并各个任务结果，输出

		cout << (sum1 + sum2 + sum3) << endl;
	}

	// 记录主线程计算开始时间
	// 
	//auto main_start_time = std::chrono::high_resolution_clock::now();
	Ulong sum = 0;
	for (int i = 1; i <= 3000; i++)
	{
		sum += i;
	}
	// 记录主线程计算结束时间
	//auto main_end_time = std::chrono::high_resolution_clock::now();
	//auto main_duration = std::chrono::duration_cast<std::chrono::milliseconds>(main_end_time - main_start_time).count();

	// 输出主线程任务总时间
	//std::cout << "Time taken by master thread: " << main_duration << " ms" << std::endl;
	cout << sum << endl;
	
	getchar();
}