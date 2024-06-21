

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
* 1:���������Լ�û��new,������������д
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
Result ThreadPool::submitTask(std::shared_ptr<Task>sp) 
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
		//�����ύʧ�ܣ�������Ч�ĵ�ֵ
		return Result(sp,false);
	}
	//����п��࣬������Ž��������
	taskQue_.emplace(sp);//������
	/*
		���⣺��ʵ�Ѿ��ж������Ѿ���ͳ��Ԫ�ظ����ˣ�Ϊʲô����Ҫ�и������ı�����¼�أ�
	*/
	taskSize_++; 

	//��Ϊ�·�������������в�Ϊ���ˣ�notEmpty_����֪ͨ���Ͽ�����߳�ִ������
	notEmpty_.notify_all();

	//����Result����
	return Result(sp);

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
			//���в�Ϊ����Ӧ�����Ѽ���������
			notFull_.notify_all();
		}
		//��ǰ�̵߳ĸ���ִ���������
		if (task != nullptr)
		{
			/*
			* ����������  1��ִ������
			*			  2��������ķ���ֵsetValue��������Result
			*/
			task->exec();
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


//////////////// Task����ʵ��
Task::Task():result_(nullptr){}

void Task::exec()
{
	if (result_ != nullptr)
	{
		result_->setVal(run());//���﷢����̬����
	}
}

void Task::setResult(Result* res) 
{
	result_ = res;
}

/////////////////// Result������ʵ��

//���캯����ʵ��
Result::Result(std::shared_ptr<Task>task, bool isValid )
	:task_(task)
	,isValid_(isValid)
{
	task_->setResult(this); //this  ָ�ĵ�ǰ��Result�������ø�task����
}

void Result::setVal(Any any)
{
	//�洢task�ķ���ֵ
	this->any_ = std::move(any);
	sem_.post();//�Ѿ���ȡ����ķ���ֵ�������ź�����Դ
}
//
Any Result::get()
{
	if (!isValid_)
	{
		return "";
	}
	sem_.wait();// task�������û��ִ���꣬��Ҫ�ȴ����������û����߳�
	return std::move(any_);
}