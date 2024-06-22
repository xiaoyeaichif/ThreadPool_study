
#include<iostream>
#include<chrono>
#include "threadpool.h"


using Ulong = unsigned long long;

using namespace std;

//�û���ʵ��
class MyTask : public Task {
public:
	MyTask();
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
		//�û��Լ������̳߳صĹ���ģʽ
		pool.setMode(PoolMode::MODE_CATCH);
		pool.start(num);//��ʼ��num���߳�

		//��¼�߳����еĿ�ʼʱ��
		//auto start_time = std::chrono::high_resolution_clock::now();

		Result res1 = pool.submitTask(std::make_shared<MyTask>(1, 10000));
		Result res2 = pool.submitTask(std::make_shared<MyTask>(10001, 20000));
		Result res3 = pool.submitTask(std::make_shared<MyTask>(20001, 30000));
		pool.submitTask(std::make_shared<MyTask>(20001, 30000));
		pool.submitTask(std::make_shared<MyTask>(20001, 30000));
		pool.submitTask(std::make_shared<MyTask>(20001, 30000));

		//get����һ��Any����
		Ulong sum1 = res1.get().cast_<Ulong>();
		Ulong sum2 = res2.get().cast_<Ulong>();
		Ulong sum3 = res3.get().cast_<Ulong>();

		//��¼���߳̽���ʱ��
		//auto end_time = std::chrono::high_resolution_clock::now();
		//����ʱ��Ĳ�ֵ
		//auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
		// ������߳�������ʱ��
		//std::cout << "Time taken by slave threads: " << duration << " ms" << std::endl;


		//Master ---- Slaveģ��
		//Master�����ֽ�����Ȼ�������Slave�̷߳�������
		//�ȴ�����Slave�߳�ִ�������񣬷��ؽ��
		//Master�̺߳ϲ����������������

		cout << (sum1 + sum2 + sum3) << endl;
	}

	// ��¼���̼߳��㿪ʼʱ��
	// 
	//auto main_start_time = std::chrono::high_resolution_clock::now();
	Ulong sum = 0;
	for (int i = 1; i <= 3000; i++)
	{
		sum += i;
	}
	// ��¼���̼߳������ʱ��
	//auto main_end_time = std::chrono::high_resolution_clock::now();
	//auto main_duration = std::chrono::duration_cast<std::chrono::milliseconds>(main_end_time - main_start_time).count();

	// ������߳�������ʱ��
	//std::cout << "Time taken by master thread: " << main_duration << " ms" << std::endl;
	cout << sum << endl;
	
	getchar();
}