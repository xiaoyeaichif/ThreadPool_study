
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

	//����һ����ô���run�����ķ���ֵ�����Ա�ʾ���������
	//C++17 Any����------->�ǿ�����������������͵Ļ���
	//java �� python ���и��ཻobject�࣬��������Ļ���
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
	pool.start(num);//��ʼ��num���߳�

	//��ʼ���������
	int task = 10;
	for (int i = 0; i < task; i++)
	{
		pool.submitTask(std::make_shared<MyTask>());
	}
	

	getchar();
}