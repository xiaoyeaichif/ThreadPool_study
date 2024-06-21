

#include "threadpool.h"

#include <functional>
#include <thread>
#include <iostream>
const int TASK_MAX_THRESHHOLD = 4;//任务队列的最大数量


//线程池构造的初始化
ThreadPool::ThreadPool()
	:initThreadSize_(0)
	,taskSize_(0)
	,taskQueMaxThreshHold_(TASK_MAX_THRESHHOLD)
	,poolMode_(PoolMode::MODE_FIXED)
{}

//线程池的析构
/*
* 1:由于我们自己没有new,所以析构不用写
*/
ThreadPool::~ThreadPool()
{

}


//设置线程的工作模式
void ThreadPool::setMode(PoolMode mode) {
	poolMode_ = mode;
}

//设置线程的初始线程数量
//void ThreadPool::setinitThreadSize(int size)
//{
//	initThreadSize_ = size;
//}


//设置task任务队列上限阈值
void ThreadPool::setTaskQueMaxThreshHold(int threahhold)
{
	taskQueMaxThreshHold_ = threahhold;
}

//给线程池提交任务
//生产任务对象
Result ThreadPool::submitTask(std::shared_ptr<Task>sp) 
{
	//首先要获取锁
	std::unique_lock<std::mutex>lock(taskQueMtx_);
	//线程的通信  等待任务队列有空余
	//当队列大小满了以后,需要等待，此时不能生产

	//两种方式

	/*while (taskQue_.size() >= taskQueMaxThreshHold_)
	{
		notFull_.wait(lock); 
	}*/
	//用户提交任务，最长阻塞不能超过1s，否则判断提交任务失败，返回
	//wait wait_for  wait_until
	//第三个条件一旦满足，立刻返回，如果不满足只等待1s,然后打印出错 日志
	if ( !notFull_.wait_for(lock, std::chrono::seconds(1),
		[&]()->bool {return taskQue_.size() < taskQueMaxThreshHold_; }))
	{
		std::cerr << "task queue is full,submit task fail." << std::endl;
		//任务提交失败，返回无效的的值
		return Result(sp,false);
	}
	//如果有空余，将任务放进任务队列
	taskQue_.emplace(sp);//放任务
	/*
		问题：其实已经有队列中已经在统计元素个数了，为什么还需要有个单独的变量记录呢？
	*/
	taskSize_++; 

	//因为新放了任务，任务队列不为空了，notEmpty_进行通知，赶快分配线程执行任务
	notEmpty_.notify_all();

	//返回Result对象
	return Result(sp);

}

//开启线程池
void ThreadPool::start(int initThreadSize )
{
	//记录初始线程个数
	initThreadSize_ = initThreadSize;

	//创建线程数量
	for (int i = 0; i < initThreadSize_; i++)
	{
		//创建thread线程对象的时候, 把线程函数给到thread线程对象
		auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this));
		//一定要用move,因为unique_ptr不提供拷贝构造函数
		threads_.emplace_back(std::move(ptr));
		/*	
		* 老版本，没有使用智能指针管理
		threads_.emplace_back(new Thread(std::bind(&ThreadPool::threadFunc, this)));
		*/
	}
	//启动所有线程
	for (int i = 0; i < initThreadSize_; i++)
	{
		threads_[i]->start();//需要执行线程函数
	}
}
//定义线程函数
//消费任务(消费者)
void ThreadPool::threadFunc()
{
	////打印线程ID
	//std::cout << "begin threadFunc tid: " << std::this_thread::get_id() << std::endl;
	//std::cout << "end threadFunc" << std::this_thread::get_id()<<std::endl;

	for (;;)
	{
		std::shared_ptr<Task>task;
		//构造一个作用域，锁出这个作用域就会释放
		/*
			也就是线程获取了任务之后就应该释放锁，让其他线程获取任务
		*/
		{
			//先获取锁
			std::unique_lock<std::mutex>lock(taskQueMtx_);

			std::cout << "tid: " << std::this_thread::get_id() << "尝试获取任务..." << std::endl;

			//等待notEmpty条件,判断队列是否为空
			while (taskQue_.size() <= 0)
			{
				//释放锁
				notEmpty_.wait(lock);
			}
			//lambda匿名表达式写法
			//notEmpty_.wait(lock, [&]()->bool {return taskQue_.size() > 0; });

			std::cout << "tid: " << std::this_thread::get_id() << "已经获取到任务..." << std::endl;

			//从任务队列中取出一个任务来
			task = taskQue_.front();
			taskQue_.pop();
			taskSize_--;

			//如果依然有剩余任务,继续通知其他的线程执行任务
			if (taskQue_.size() > 0)
			{
				notEmpty_.notify_all();
			}
			//队列不为满，应该提醒继续加任务
			notFull_.notify_all();
		}
		//当前线程的负责执行这个任务
		if (task != nullptr)
		{
			/*
			* 负责两件事  1：执行任务
			*			  2：把任务的返回值setValue方法给到Result
			*/
			task->exec();
		}
	}

}

//////////////////////// 线程方法的实现
//启动单个线程
/*
	只要任务队列有任务, 就需要工作
*/


//线程的构造
Thread::Thread(ThreadFunc func):func_(func){}
// 线程的析构
Thread::~Thread() {}


void Thread::start()
{
	std::thread t(func_); //  C++11来说 线程对象t 和  线程函数func_
	t.detach(); //设置分离线程
	/*
		线程t在出了start这个作用域后就会消亡, 而线程函数func_是不会消亡的, 所以需要设置分离线程
	*/
}


//////////////// Task方法实现
Task::Task():result_(nullptr){}

void Task::exec()
{
	if (result_ != nullptr)
	{
		result_->setVal(run());//这里发生多态调用
	}
}

void Task::setResult(Result* res) 
{
	result_ = res;
}

/////////////////// Result方法的实现

//构造函数的实现
Result::Result(std::shared_ptr<Task>task, bool isValid )
	:task_(task)
	,isValid_(isValid)
{
	task_->setResult(this); //this  指的当前的Result对象，设置给task对象
}

void Result::setVal(Any any)
{
	//存储task的返回值
	this->any_ = std::move(any);
	sem_.post();//已经获取任务的返回值，增加信号量资源
}
//
Any Result::get()
{
	if (!isValid_)
	{
		return "";
	}
	sem_.wait();// task任务如果没有执行完，需要等待，会阻塞用户的线程
	return std::move(any_);
}