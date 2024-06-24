

#include "threadpool.h"

#include <functional>
#include <thread>
#include <iostream>
const int TASK_MAX_THRESHHOLD   = 1024;//任务队列的最大数量
const int THREAD_MAX_THRESHHOLD = 100; //线程的上限
const int THREAD_MAX_IDLE_TIME  =  10; //单位:秒

//线程池构造的初始化
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



//线程池的析构
ThreadPool::~ThreadPool()
{
	isPoolRunning_ = false;
	notEmpty_.notify_all();

	//等待线程池所有的线程返回，有两种状态：阻塞 && 正在运行中
	std::unique_lock<std::mutex>lock(taskQueMtx_);
	exitCond_.wait(lock, [&]()->bool {return threads_.size() == 0; }); // threads_.size() == 0--->true 就不再阻塞
}


//设置线程的工作模式
void ThreadPool::setMode(PoolMode mode) {
	//如果当前模式已经在运行，则不允许再修改
	if (checkRunningState())
	{
		return;
	}
	poolMode_ = mode;
}


//设置线程池中线程的上限阈值(catch模式)
void ThreadPool::setThreadSizeThreshHold(int threahhold) {
	
	if (checkRunningState())
	{
		return; //已经启动就不可以更改模式
	}
	//只有catch模式才能更改
	if (poolMode_ == PoolMode::MODE_CATCH)
	{
		threadSizeThreshHold_ = threahhold;
	}
	
}


//设置task任务队列上限阈值
void ThreadPool::setTaskQueMaxThreshHold(int threahhold)
{
	if (checkRunningState()) return; //已经启动就不可以更改队列的上限阈值
	taskQueMaxThreshHold_ = threahhold;
}

//给线程池提交任务
//生产任务对象，线程池中的线程作为消费者取走任务
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
	//用户提交任务，，如果队列中元素一直是满的且持续一秒钟（最长阻塞不能超过1s），否则判断提交任务失败，返回
	//wait wait_for  wait_until
	//任务在1s以内, 任务队列的大小仍然是大于等于队列的最大值, 则认为加入任务失败, 
	// 也就是wait_for的返回值为false ， 此时！false  == true , 就正好进入if函数内，打印错误日志
	if ( !notFull_.wait_for(lock, std::chrono::seconds(1),
		[&]()->bool {return taskQue_.size() < (size_t)taskQueMaxThreshHold_; }))
	{
		std::cerr << "task queue is full,submit task fail." << std::endl;
		//任务提交失败，返回无效的的值
		return Result(sp,false);
	}

	// 如果在1秒内，队列的大小不在是最大值，也就是有空位之后, 将任务task放进任务队列中即可
	taskQue_.emplace(sp);//放任务
	/*
		问题：其实已经有队列中已经在统计元素个数了，为什么还需要有个单独的变量记录呢？
	*/
	taskSize_++; 

	//因为任务队列还可以加任务，所以我们认为队列不为空，此时应该提醒线程池来取任务
	// notEmpty_进行通知，赶快分配线程执行任务
	notEmpty_.notify_all();

	/* 
		1：下面的代码是因为这个线程池模型有两个模式 一个是：fixed ，一个是：catch
	*
		2：catch模式  任务处理比较紧急，
			场景：小而快的任务 需要根据任务数量和空闲线程的数量，判断是否需要增加线程，
		catch模式应对一些任务数量大于线程的数量,并且线程数量小于最大线程数量的问题
		3：举个例子，比如说线程池中只有4个线程，但是我有6个任务, 其中一些任务是比较耗时的，另外，我们又想要
		执行任务的速度足够快，固定的线程早已不满足我们的要求，所以需要动态修改线程池中线程的数量！
	*/
	//转变默认的模式
	if (poolMode_ == PoolMode::MODE_CATCH 
		&& taskSize_ > idleThreadSize_
		&& curThreadSize_ < threadSizeThreshHold_)
	{
		//创建新线程
		/*
			auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this));
			threads_.emplace_back(std::move(ptr));
			curThreadSize_++;
		*/

		std::cout << ">>>>>create new thread......" << std::endl;

		//使用哈希表重构
		auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this, std::placeholders::_1));
		int threadId = ptr->getId();
		threads_.emplace(threadId, std::move(ptr));//key:id ,value:thread
		//创建出来要启动线程
		threads_[threadId]->start();
		//空闲线程和当前线程的总个数都需要+1
		idleThreadSize_++;
		curThreadSize_++;

	}
	//返回Result对象
	return Result(sp);
}


//开启线程池
void ThreadPool::start(int initThreadSize )
{
	// 设置线程池的运行状态
	isPoolRunning_ = true;
	//记录初始线程个数
	initThreadSize_ = initThreadSize; // 默认线程池中的线程个数
	curThreadSize_ = initThreadSize; // 当前线程中的线程个数
	idleThreadSize_ = initThreadSize; // 空闲线程的个数
	//创建线程数量
	for (int i = 0; i < initThreadSize_; i++)
	{
		//使用哈希表重构
		//创建thread线程对象的时候, 把线程函数给到thread线程对象
		auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this,std::placeholders::_1));
		int threadId = ptr->getId();
		threads_.emplace(threadId, std::move(ptr));

		//创建thread线程对象的时候, 把线程函数给到thread线程对象
		//auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this));
		//一定要用move,因为unique_ptr不提供拷贝构造函数
		//threads_.emplace_back(std::move(ptr));
		/*	
		* 老版本，没有使用智能指针管理
		threads_.emplace_back(new Thread(std::bind(&ThreadPool::threadFunc, this)));
		*/
	}
	//启动所有线程
	for (int i = 0; i < initThreadSize_; i++)
	{
		threads_[i]->start();//需要执行线程函数
		//idleThreadSize_++; // 记录初始空闲线程的数量
	}
}

//定义线程函数
//消费任务(消费者)，执行任务
void ThreadPool::threadFunc(int threadid) //线程函数执行完，相应的线程就结束了
{
	auto lastTime = std::chrono::high_resolution_clock().now(); //计算一个线程持续的时间

	for (;;) //和while(true)一个意思
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

			/*
				1：catch模式下，有可能已经创建了很多线程，但是空闲时间超过60s
				应该把多余的线程回收掉（超过默认大小initThreadSize_数量线程要进行回收）
				2：当前时间减去 - 上一次时间  > 60
			*/
			while (taskQue_.size() == 0)
			{
				// 线程池要结束，回收线程资源
				if (!isPoolRunning_)
				{
					threads_.erase(threadid); // 
					std::cout << "threadid:" << std::this_thread::get_id() << " exit!"
						<< std::endl;
					exitCond_.notify_all();
					return; // 线程函数结束，线程结束
				}

				// 每一秒返回一次   
				//怎么区分：超时返回 还是  有任务执行返回
				if (poolMode_ == PoolMode::MODE_CATCH)
				{
					//条件变量超时返回
					/*
						如果线程池模式为 MODE_CATCH，线程会每隔一秒钟检查一次是否超时。
						如果超时且线程空闲时间超过 THREAD_MAX_IDLE_TIME，并且当前线程数大于初始线程数，线程将被回收。
						否则，线程等待条件变量 notEmpty_，直到有任务可用。
					*/
					if (std::cv_status::timeout ==
						notEmpty_.wait_for(lock, std::chrono::seconds(1)))
					{
						auto nowTime = std::chrono::high_resolution_clock().now();
						auto dur = std::chrono::duration_cast<std::chrono::seconds>(lastTime - nowTime);
						//回收线程
						if (dur.count() >= THREAD_MAX_IDLE_TIME && curThreadSize_ > initThreadSize_)
						{
							/*
								1：回收当前线程
								2：记录线程数量的相关变量值的修改
								3：把线程对象从线程容器中删除
							*/
							//将线程移除
							threads_.erase(threadid);
							//容器内总个数需要减少，空闲线程减少
							curThreadSize_--;
							idleThreadSize_--;
							std::cout << "threadid: " << std::this_thread::get_id() << " exit thread"
								<< std::endl;
							return;
						}
					}
				}
				else {
					//等待notEmpty条件,判断任务队列是否为空
					//释放锁
					//lambda匿名表达式写法
					//notEmpty_.wait(lock, [&]()->bool {return taskQue_.size() > 0; });

					// 等待notEmpty条件
					notEmpty_.wait(lock);
				}
			}
			// 下面都是处理任务的逻辑
			
			//队列中有任务可以取了，需要消耗一个空闲线程
			idleThreadSize_--;

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

			// 取出一个任务，进行通知，提醒消费者可以继续生产任务
			notFull_.notify_all();
		 }    // 作用域在此表示---->当前线程释放锁


		//当前线程的负责执行这个任务
		if (task != nullptr)
		{
			/*
			* 负责两件事  1：执行任务
			*			  2：把任务的返回值setValue方法给到Result
			*/
			task->exec();
		}
		//更新线程执行完的时间

		idleThreadSize_++; // 任务处理完毕，空闲线程+1
		lastTime = std::chrono::high_resolution_clock().now();
	}

}

bool ThreadPool::checkRunningState() const {
	return isPoolRunning_;
}



//***********************************     线程方法的实现
//启动单个线程
/*
	只要任务队列有任务, 就需要工作
*/
int Thread::generatedId_ = 0; // 静态对象需要再类外初始化

//线程的构造
Thread::Thread(ThreadFunc func)
	:func_(func)
	,threadId_(generatedId_++) //创建一个线程,线程的Id就会+1
{}
// 线程的析构
Thread::~Thread() {}


void Thread::start()
{
	std::thread t(func_,threadId_); //  C++11来说 线程对象t 和  线程函数func_
	t.detach(); //设置分离线程
	/*
		线程t在出了start这个作用域后就会消亡, 而线程函数func_是不会消亡的, 所以需要设置分离线程
	*/
}

//获取线程id
int Thread::getId() const
{
	return threadId_;
}


//**************************			Task方法实现
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

//************************************    Result方法的实现

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