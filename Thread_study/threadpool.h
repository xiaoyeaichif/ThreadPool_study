#ifndef THREADPOLL_H //防止重复包含
#define THREADPOLL_H

//头文件
#include <vector>
#include <queue>
#include <memory>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <functional>

//Any类型：可以接收任意数据的类型

class Any {
public:
	//默认构造函数
	Any() = default;
	~Any() = default;

	//禁止拷贝构造和赋值构造
	Any (const Any&) = delete;
	Any & operator = (const Any&) = delete;

	//右值构造使用默认的就可以
	Any(Any && ) = default; 

	//右值赋值构造函数
	Any& operator = ( Any&&) = default;

	//用模板接收任意的数据类型
	template<typename T>
	//基类指针指向派生类
	Any(T data):base_(std::make_unique<Derive<T>>(data))
	{}

	//这个方法把Any对象里面存储的data_数据提取出来,转为用户想要的类型
	template<typename T>
	T cast_()
	{
		//	我们怎么从 base_找到它所指向的Derive对象，从他里面获取data成员变量
		//	基类指针转为派生类指针
		Derive<T>* pd = dynamic_cast<Derive<T>*>(base_.get());
		if (pd == nullptr)
		{
			throw "type is unmatch!";
		}
		return pd->data_;
	}

private:
	//嵌套类
	// 
	//基类类型
	class Base {
	public:
		virtual ~Base() = default; // 在继承结构中, 基类的析构函数应该设置为虚析构函数
	};

	//派生类   
	template<typename T>
	class Derive :public Base {
	public:
		Derive(T data):data_(data){}  //构造函数初始化
		T data_;
	};
private:
	//定义一个基类的指针
	std::unique_ptr<Base>base_;
};


//实现一个信号量类
class Semaphore 
{
public:
	//构造函数
	Semaphore(int limit = 0) :resLimit_(limit)
	{

	}
	//析构函数
	~Semaphore() = default;

	//wait函数的实现  获取（消耗）一个信号量资源
	void wait()
	{
		//进入一个资源前需要先上锁
		//相当于  P 操作
		std::unique_lock<std::mutex>lock(mtx_);
		if (resLimit_ <= 0)
		{
			cond_.wait(lock);
		}
		//也可以写成如下的lambda表达式形式
		//cond_.wait(lock, [&]()->bool {if resLimit_ > 0; };)

		//代表目前有资源
		resLimit_--; //信号量 -1
	}


	//post函数的实现 增加一个信号量的实现
	//相当于 V 操作
	void post()
	{
		//进入一个资源前需要先上锁
		std::unique_lock<std::mutex>lock(mtx_);
		resLimit_++; //信号量+1
		cond_.notify_all();
	}



private:
	int resLimit_;
	std::mutex mtx_;
	std::condition_variable cond_;
};

// Task类型的前置声明
class Task;
//实现接受提交到线程池的task任务执行完成后的返回值类型Result
class Result {
public:
	//构造函数,默认为有效的任务
	Result(std::shared_ptr<Task>task, bool isValid = true);

	//析构函数
	~Result() = default;

	/*
	* 问题1：setValue方法，获取任务执行完的返回值
	* 问题2：get方法，用户调用这个方法获取task的返回值
	*/

	//获取任务执行完的返回值
	void setVal(Any any);

	//
	Any get();

private:
	Any any_;  //   存储任务的返回值
	Semaphore sem_; // 线程通信信号量
	std::shared_ptr<Task>task_; // 指向对应获取返回值的任务对象
	std::atomic_bool isValid_; //
};



//任务抽象基类
//用户可以自定义任意任务类型,从TASK继承，重写run方法，实现
class Task {
public:
	//构造函数
	Task();
	~Task() = default;



	void setResult(Result* res);
public:
	void exec();
	virtual Any run() = 0;
private:
	//不能使用智能指针, 会导致智能指针交叉引用
	Result* result_;
};

//线程池支持的模式
enum class PoolMode
{
	MODE_FIXED,
	MODE_CATCH,
};




//线程类型
class Thread{
public:
	using ThreadFunc = std::function<void(int)>;
	//线程的构造
	Thread(ThreadFunc func);
	// 线程的析构
	~Thread();
	//启动线程
	void start();

	//获取线程id
	int getId() const;

private:
	ThreadFunc func_;
	static int generatedId_;
	int threadId_; //保存线程id
};


/*




*/





//线程池类型
class ThreadPool
{
public:
	//构造函数
	ThreadPool();
	~ThreadPool();

	//开启线程池
	void start(int initThreadSize = 4);

	//设置线程的工作模式
	void setMode(PoolMode mode);

	//设置线程的初始线程数量
	//void setinitThreadSize(int size);

	//设置线程池中线程的上限阈值(catch模式)
	void setThreadSizeThreshHold(int threahhold);


	//设置task任务队列上限阈值
	void setTaskQueMaxThreshHold(int threahhold);

	//给线程池提交任务,生产者
	Result submitTask(std::shared_ptr<Task>sp);

	//禁用拷贝构造和赋值构造
	ThreadPool(const ThreadPool&) = delete;//拷贝构造
	ThreadPool &operator = (const ThreadPool&) = delete;//赋值构造
private:
	//定义线程函数
	void threadFunc(int threadid); //这个函数用来使线程工作

	//检查pool的运行状态
	bool checkRunningState() const;
	
private:
	//把Thread*改为用智能指针管理
	//std::vector<std::unique_ptr<Thread>>threads_;  //线程列表

	//
	std::unordered_map<int, std::unique_ptr<Thread>>threads_;

	size_t initThreadSize_;          //初始线程数量
	 
	size_t threadSizeThreshHold_;    //线程数量上限阈值

	std::atomic_int curThreadSize_;  //记录当前线程池中的总线程

	std::atomic_int idleThreadSize_; //记录空闲线程的数量

	/*
	* 任务队列为什么使用智能指针？
	*/
	std::queue<std::shared_ptr<Task>>taskQue_; //任务队列
	std::atomic_int taskSize_; // 任务的数量
	int taskQueMaxThreshHold_; //任务队列数量上限阈值

	std::mutex taskQueMtx_; //任务队列的互斥锁，保证任务队列线程安全

	std::condition_variable notFull_;  //表示任务队列不满,可以继续生产
	std::condition_variable notEmpty_; //表示任务队列不空，可以继续加入

	PoolMode poolMode_;//当前线程池的工作模式
	std::atomic_bool isPoolRunning_;  //表示当前线程池的启动状态

};

#endif