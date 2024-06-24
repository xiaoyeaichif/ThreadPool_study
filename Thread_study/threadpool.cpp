

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
ThreadPool::~ThreadPool()
{
	isPoolRunning_ = false;
	notEmpty_.notify_all();

	//�ȴ��̳߳����е��̷߳��أ�������״̬������ && ����������
	std::unique_lock<std::mutex>lock(taskQueMtx_);
	exitCond_.wait(lock, [&]()->bool {return threads_.size() == 0; }); // threads_.size() == 0--->true �Ͳ�������
}


//�����̵߳Ĺ���ģʽ
void ThreadPool::setMode(PoolMode mode) {
	//�����ǰģʽ�Ѿ������У����������޸�
	if (checkRunningState())
	{
		return;
	}
	poolMode_ = mode;
}


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
	if (checkRunningState()) return; //�Ѿ������Ͳ����Ը��Ķ��е�������ֵ
	taskQueMaxThreshHold_ = threahhold;
}

//���̳߳��ύ����
//������������̳߳��е��߳���Ϊ������ȡ������
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
	//�û��ύ���񣬣����������Ԫ��һֱ�������ҳ���һ���ӣ���������ܳ���1s���������ж��ύ����ʧ�ܣ�����
	//wait wait_for  wait_until
	//������1s����, ������еĴ�С��Ȼ�Ǵ��ڵ��ڶ��е����ֵ, ����Ϊ��������ʧ��, 
	// Ҳ����wait_for�ķ���ֵΪfalse �� ��ʱ��false  == true , �����ý���if�����ڣ���ӡ������־
	if ( !notFull_.wait_for(lock, std::chrono::seconds(1),
		[&]()->bool {return taskQue_.size() < (size_t)taskQueMaxThreshHold_; }))
	{
		std::cerr << "task queue is full,submit task fail." << std::endl;
		//�����ύʧ�ܣ�������Ч�ĵ�ֵ
		return Result(sp,false);
	}

	// �����1���ڣ����еĴ�С���������ֵ��Ҳ�����п�λ֮��, ������task�Ž���������м���
	taskQue_.emplace(sp);//������
	/*
		���⣺��ʵ�Ѿ��ж������Ѿ���ͳ��Ԫ�ظ����ˣ�Ϊʲô����Ҫ�и������ı�����¼�أ�
	*/
	taskSize_++; 

	//��Ϊ������л����Լ���������������Ϊ���в�Ϊ�գ���ʱӦ�������̳߳���ȡ����
	// notEmpty_����֪ͨ���Ͽ�����߳�ִ������
	notEmpty_.notify_all();

	/* 
		1������Ĵ�������Ϊ����̳߳�ģ��������ģʽ һ���ǣ�fixed ��һ���ǣ�catch
	*
		2��catchģʽ  ������ȽϽ�����
			������С��������� ��Ҫ�������������Ϳ����̵߳��������ж��Ƿ���Ҫ�����̣߳�
		catchģʽӦ��һЩ�������������̵߳�����,�����߳�����С������߳�����������
		3���ٸ����ӣ�����˵�̳߳���ֻ��4���̣߳���������6������, ����һЩ�����ǱȽϺ�ʱ�ģ����⣬��������Ҫ
		ִ��������ٶ��㹻�죬�̶����߳����Ѳ��������ǵ�Ҫ��������Ҫ��̬�޸��̳߳����̵߳�������
	*/
	//ת��Ĭ�ϵ�ģʽ
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

		std::cout << ">>>>>create new thread......" << std::endl;

		//ʹ�ù�ϣ���ع�
		auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this, std::placeholders::_1));
		int threadId = ptr->getId();
		threads_.emplace(threadId, std::move(ptr));//key:id ,value:thread
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
	initThreadSize_ = initThreadSize; // Ĭ���̳߳��е��̸߳���
	curThreadSize_ = initThreadSize; // ��ǰ�߳��е��̸߳���
	idleThreadSize_ = initThreadSize; // �����̵߳ĸ���
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
		//idleThreadSize_++; // ��¼��ʼ�����̵߳�����
	}
}

//�����̺߳���
//��������(������)��ִ������
void ThreadPool::threadFunc(int threadid) //�̺߳���ִ���꣬��Ӧ���߳̾ͽ�����
{
	auto lastTime = std::chrono::high_resolution_clock().now(); //����һ���̳߳�����ʱ��

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
			while (taskQue_.size() == 0)
			{
				// �̳߳�Ҫ�����������߳���Դ
				if (!isPoolRunning_)
				{
					threads_.erase(threadid); // 
					std::cout << "threadid:" << std::this_thread::get_id() << " exit!"
						<< std::endl;
					exitCond_.notify_all();
					return; // �̺߳����������߳̽���
				}

				// ÿһ�뷵��һ��   
				//��ô���֣���ʱ���� ����  ������ִ�з���
				if (poolMode_ == PoolMode::MODE_CATCH)
				{
					//����������ʱ����
					/*
						����̳߳�ģʽΪ MODE_CATCH���̻߳�ÿ��һ���Ӽ��һ���Ƿ�ʱ��
						�����ʱ���߳̿���ʱ�䳬�� THREAD_MAX_IDLE_TIME�����ҵ�ǰ�߳������ڳ�ʼ�߳������߳̽������ա�
						�����̵߳ȴ��������� notEmpty_��ֱ����������á�
					*/
					if (std::cv_status::timeout ==
						notEmpty_.wait_for(lock, std::chrono::seconds(1)))
					{
						auto nowTime = std::chrono::high_resolution_clock().now();
						auto dur = std::chrono::duration_cast<std::chrono::seconds>(lastTime - nowTime);
						//�����߳�
						if (dur.count() >= THREAD_MAX_IDLE_TIME && curThreadSize_ > initThreadSize_)
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
				else {
					//�ȴ�notEmpty����,�ж���������Ƿ�Ϊ��
					//�ͷ���
					//lambda�������ʽд��
					//notEmpty_.wait(lock, [&]()->bool {return taskQue_.size() > 0; });

					// �ȴ�notEmpty����
					notEmpty_.wait(lock);
				}
			}
			// ���涼�Ǵ���������߼�
			
			//���������������ȡ�ˣ���Ҫ����һ�������߳�
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

			// ȡ��һ�����񣬽���֪ͨ�����������߿��Լ�����������
			notFull_.notify_all();
		 }    // �������ڴ˱�ʾ---->��ǰ�߳��ͷ���


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



//***********************************     �̷߳�����ʵ��
//���������߳�
/*
	ֻҪ�������������, ����Ҫ����
*/
int Thread::generatedId_ = 0; // ��̬������Ҫ�������ʼ��

//�̵߳Ĺ���
Thread::Thread(ThreadFunc func)
	:func_(func)
	,threadId_(generatedId_++) //����һ���߳�,�̵߳�Id�ͻ�+1
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


//**************************			Task����ʵ��
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

//************************************    Result������ʵ��

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