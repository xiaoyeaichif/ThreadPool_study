

#include<iostream>
#include<thread>
#include<string>
#include<chrono> //控制时间的库函数
using namespace std;

void Thread_1()
{
	cout << "thread-1 run begin!" << endl;
	//线程1睡眠2秒，这个过程cpu的时间片可以分给其它线程
	this_thread::sleep_for(chrono::seconds(2));
	cout << "thread-1 2秒后唤醒，run end!" << endl;
}

void Thread_2(int val,string info)
{
	cout << "thread-2 run begin!" << endl;
	cout << "thread-2 args[val:" << val << ",info:" << "]" << endl;
	//线程2睡眠4秒,这个过程cpu的时间片可以分给其它线程
	this_thread::sleep_for(chrono::seconds(4));
	cout << "thread-2 4秒后醒来, run end!" << endl;
}


int main_thread01()
{
	cout << "主线程的开始" << endl;
	//线程的船舰
	thread t1(Thread_1);
	thread t2(Thread_2,20,"yejie");

	// 等待线程t1和t2执行完，main线程再继续运行
	t1.join();
	t2.join();


	cout << "主线程的结束" << endl;
	return 0;
}