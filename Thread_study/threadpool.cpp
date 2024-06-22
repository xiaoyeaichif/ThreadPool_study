

#include "threadpool.h"

#include <functional>
#include <thread>
#include <iostream>
const int TASK_MAX_THRESHHOLD   = 1024;//������е��������
const int THREAD_MAX_THRESHHOLD = 100; //�̵߳�����
const int THREAD_MAX_IDLE_TIME  =  10; //��λ:��

//�̳߳ع���ĳ�ʼ��
ThreadPool::ThreadPool()
	: initThreadSize_(0)
	, idleThreadSize_(0)
	, curThreadSize_(0)
	, threadSizeThreshHold_(THREAD_MAX_THRESHHOLD)
	, taskSize_(0)
	, taskQueMaxThreshHold_(TASK_MAX_THRESHHOLD)
	, poolMode_(PoolMode::MODE_FIXED)
	, isPoolRunning_(false)
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
	//�����ǰ�Ѿ�������ģʽ�����������޸�
	if (checkRunningState())
	{
		return;
	}
	poolMode_ = mode;
}

//�����̵߳ĳ�ʼ�߳�����
//void ThreadPool::setinitThreadSize(int size)
//{
//	initThreadSize_ = size;
//}

//�����̳߳����̵߳�������ֵ(catchģʽ)
void ThreadPool::setThreadSizeThreshHold(int threahhold) {
	
	if (checkRunningState())
	{
		return; //�Ѿ������Ͳ����Ը���ģʽ
	}
	//ֻ��catchģʽ���ܸ���
	if (poolMode_ == PoolMode::MODE_CATCH)
	{
		threadSizeThreshHold_ = threahhold;
	}
	
}


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

	// catchģʽ  ������ȽϽ�����
	// ������С��������� ��Ҫ�������������Ϳ����̵߳��������ж��Ƿ���Ҫ�����߳�
	// 
	// ģʽ��Ҫ�ı䲢���������������̵߳�����,�����߳�����С������߳�����
	if (poolMode_ == PoolMode::MODE_CATCH 
		&& taskSize_ > idleThreadSize_
		&& curThreadSize_ < threadSizeThreshHold_)
	{
		//�������߳�
		/*
			auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this));
			threads_.emplace_back(std::move(ptr));
			curThreadSize_++;
		*/

		std::cout << "create new thread......" << std::endl;

		//ʹ�ù�ϣ���ع�
		auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this, std::placeholders::_1));
		int threadId = ptr->getId();
		threads_.emplace(threadId, std::move(ptr));
		//��������Ҫ�����߳�
		threads_[threadId]->start();
		//�����̺߳͵�ǰ�̵߳��ܸ�������Ҫ+1
		idleThreadSize_++;
		curThreadSize_++;

	}

	//����Result����
	return Result(sp);
}


//�����̳߳�
void ThreadPool::start(int initThreadSize )
{
	// �����̳߳ص�����״̬
	isPoolRunning_ = true;
	//��¼��ʼ�̸߳���
	initThreadSize_ = initThreadSize;
	curThreadSize_ = initThreadSize;

	//�����߳�����
	for (int i = 0; i < initThreadSize_; i++)
	{
		//ʹ�ù�ϣ���ع�
		//����thread�̶߳����ʱ��, ���̺߳�������thread�̶߳���
		auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this,std::placeholders::_1));
		int threadId = ptr->getId();
		threads_.emplace(threadId, std::move(ptr));

		//����thread�̶߳����ʱ��, ���̺߳�������thread�̶߳���
		//auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this));
		//һ��Ҫ��move,��Ϊunique_ptr���ṩ�������캯��
		//threads_.emplace_back(std::move(ptr));
		/*	
		* �ϰ汾��û��ʹ������ָ�����
		threads_.emplace_back(new Thread(std::bind(&ThreadPool::threadFunc, this)));
		*/
	}
	//���������߳�
	for (int i = 0; i < initThreadSize_; i++)
	{
		threads_[i]->start();//��Ҫִ���̺߳���
		idleThreadSize_++; // ��¼��ʼ�����̵߳�����
	}
}
//�����̺߳���
//��������(������)��ִ������
void ThreadPool::threadFunc(int threadid) //�̺߳���ִ���꣬��Ӧ���߳̾ͽ�����
{
	auto lastTime = std::chrono::high_resolution_clock().now();

	for (;;) //��while(true)һ����˼
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

			/*
				1��catchģʽ�£��п����Ѿ������˺ܶ��̣߳����ǿ���ʱ�䳬��60s
				Ӧ�ðѶ�����̻߳��յ�������Ĭ�ϴ�СinitThreadSize_�����߳�Ҫ���л��գ�
				2����ǰʱ���ȥ - ��һ��ʱ��  > 60
			*/
			if (poolMode_ == PoolMode::MODE_CATCH)
			{
				// ÿһ�뷵��һ��   
				//��ô���֣���ʱ���� ����  ������ִ�з���
				while (taskQue_.size() == 0)
				{
					//����������ʱ����
					if(std::cv_status::timeout ==
						notEmpty_.wait_for(lock, std::chrono::seconds(1)))
					{
						auto nowTime = std::chrono::high_resolution_clock().now();
						auto dur = std::chrono::duration_cast<std::chrono::seconds>(lastTime - nowTime);
						//�����߳�
						if (dur.count() >= THREAD_MAX_IDLE_TIME && curThreadSize_ >  initThreadSize_)
						{
							/*
								1�����յ�ǰ�߳�
								2����¼�߳���������ر���ֵ���޸�
								3�����̶߳�����߳�������ɾ��
							*/
							//���߳��Ƴ�
							threads_.erase(threadid);
							//�������ܸ�����Ҫ���٣������̼߳���
							curThreadSize_--;
							idleThreadSize_--;
							std::cout << "threadid: " << std::this_thread::get_id() << " exit thread"
								<< std::endl;
							return;
						}
					}
				}
			}
			else {
				//�ȴ�notEmpty����,�ж���������Ƿ�Ϊ��
				//�ͷ���
				notEmpty_.wait(lock);
				//lambda�������ʽд��
				//notEmpty_.wait(lock, [&]()->bool {return taskQue_.size() > 0; });
			}

			//���������ȡ�ˣ���Ҫ����һ�������߳�
			idleThreadSize_--;

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
		//�����߳�ִ�����ʱ��

		idleThreadSize_++; // ��������ϣ������߳�+1
		lastTime = std::chrono::high_resolution_clock().now();
	}

}

bool ThreadPool::checkRunningState() const {
	return isPoolRunning_;
}



////////////////////////////////////////////     �̷߳�����ʵ��
//���������߳�
/*
	ֻҪ�������������, ����Ҫ����
*/
int Thread::generatedId_ = 0; // ��̬������Ҫ�������ʼ��

//�̵߳Ĺ���
Thread::Thread(ThreadFunc func)
	:func_(func)
	,threadId_(generatedId_++)
{}
// �̵߳�����
Thread::~Thread() {}


void Thread::start()
{
	std::thread t(func_,threadId_); //  C++11��˵ �̶߳���t ��  �̺߳���func_
	t.detach(); //���÷����߳�
	/*
		�߳�t�ڳ���start����������ͻ�����, ���̺߳���func_�ǲ���������, ������Ҫ���÷����߳�
	*/
}

//��ȡ�߳�id
int Thread::getId() const
{
	return threadId_;
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