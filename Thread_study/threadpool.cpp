

#include "threadpool.h"

#include <functional>
#include <thread>
#include <iostream>
const int TASK_MAX_THRESHHOLD = 4;//������е��������


//�̳߳ع���ĳ�ʼ��
ThreadPool::ThreadPool()
	:initThreadSize_(0)
	,taskSize_(0)
	,taskQueMaxThreshHold_(TASK_MAX_THRESHHOLD)
	,poolMode_(PoolMode::MODE_FIXED)
{}

//�̳߳ص�����
/*
* 1:���������Լ�����new,������������л
*/
ThreadPool::~ThreadPool()
{

}


//�����̵߳Ĺ���ģʽ
void ThreadPool::setMode(PoolMode mode) {
	poolMode_ = mode;
}

//�����̵߳ĳ�ʼ�߳�����
//void ThreadPool::setinitThreadSize(int size)
//{
//	initThreadSize_ = size;
//}


//����task�������������ֵ
void ThreadPool::setTaskQueMaxThreshHold(int threahhold)
{
	taskQueMaxThreshHold_ = threahhold;
}

//���̳߳��ύ����
//�����������
void ThreadPool::submitTask(std::shared_ptr<Task>sp) 
{
	//����Ҫ��ȡ��
	std::unique_lock<std::mutex>lock(taskQueMtx_);
	//�̵߳�ͨ��  �ȴ���������п���
	//�����д�С�����Ժ�,��Ҫ�ȴ�����ʱ��������

	//���ַ�ʽ

	/*while (taskQue_.size() >= taskQueMaxThreshHold_)
	{
		notFull_.wait(lock); 
	}*/
	//�û��ύ������������ܳ���1s�������ж��ύ����ʧ�ܣ�����
	//wait wait_for  wait_until
	//����������һ�����㣬���̷��أ����������ֻ�ȴ�1s,Ȼ���ӡ���� ��־
	if ( !notFull_.wait_for(lock, std::chrono::seconds(1),
		[&]()->bool {return taskQue_.size() < taskQueMaxThreshHold_; }))
	{
		std::cerr << "task queue is full,submit task fail." << std::endl;
		return;
	}
	//����п��࣬������Ž��������
	taskQue_.emplace(sp);//������
	taskSize_++;

	//��Ϊ�·�������������в�Ϊ���ˣ�notEmpty_����֪ͨ���Ͽ�����߳�ִ������
	notEmpty_.notify_all();
}

//�����̳߳�
void ThreadPool::start(int initThreadSize )
{
	//��¼��ʼ�̸߳���
	initThreadSize_ = initThreadSize;

	//�����߳�����
	for (int i = 0; i < initThreadSize_; i++)
	{
		//����thread�̶߳����ʱ��, ���̺߳�������thread�̶߳���
		auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this));
		//һ��Ҫ��move,��Ϊunique_ptr���ṩ�������캯��
		threads_.emplace_back(std::move(ptr));
		/*	
		* �ϰ汾��û��ʹ������ָ�����
		threads_.emplace_back(new Thread(std::bind(&ThreadPool::threadFunc, this)));
		*/
	}
	//���������߳�
	for (int i = 0; i < initThreadSize_; i++)
	{
		threads_[i]->start();//��Ҫִ���̺߳���
	}
}
//�����̺߳���
//��������(������)
void ThreadPool::threadFunc()
{
	////��ӡ�߳�ID
	//std::cout << "begin threadFunc tid: " << std::this_thread::get_id() << std::endl;
	//std::cout << "end threadFunc" << std::this_thread::get_id()<<std::endl;

	for (;;)
	{
		std::shared_ptr<Task>task;
		//����һ���������������������ͻ��ͷ�
		/*
			Ҳ�����̻߳�ȡ������֮���Ӧ���ͷ������������̻߳�ȡ����
		*/
		{
			//�Ȼ�ȡ��
			std::unique_lock<std::mutex>lock(taskQueMtx_);

			std::cout << "tid: " << std::this_thread::get_id() << "���Ի�ȡ����..." << std::endl;

			//�ȴ�notEmpty����,�ж϶����Ƿ�Ϊ��
			while (taskQue_.size() <= 0)
			{
				//�ͷ���
				notEmpty_.wait(lock);
			}
			//lambda�������ʽд��
			//notEmpty_.wait(lock, [&]()->bool {return taskQue_.size() > 0; });

			std::cout << "tid: " << std::this_thread::get_id() << "�Ѿ���ȡ������..." << std::endl;

			//�����������ȡ��һ��������
			task = taskQue_.front();
			taskQue_.pop();
			taskSize_--;

			//�����Ȼ��ʣ������,����֪ͨ�������߳�ִ������
			if (taskQue_.size() > 0)
			{
				notEmpty_.notify_all();
			}
			//ִ���������֪ͨ
			notFull_.notify_all();
		}
		//��ǰ�̵߳ĸ���ִ���������
		if (task != nullptr)
		{
			task->run();
		}
	}

}

//////////////////////// �̷߳�����ʵ��
//���������߳�
/*
	ֻҪ�������������, ����Ҫ����
*/


//�̵߳Ĺ���
Thread::Thread(ThreadFunc func):func_(func){}
// �̵߳�����
Thread::~Thread() {}


void Thread::start()
{
	std::thread t(func_); //  C++11��˵ �̶߳���t ��  �̺߳���func_
	t.detach(); //���÷����߳�
	/*
		�߳�t�ڳ���start����������ͻ�����, ���̺߳���func_�ǲ���������, ������Ҫ���÷����߳�
	*/
}