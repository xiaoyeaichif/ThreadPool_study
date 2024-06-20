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
//任务抽象基类
//用户可以自定义任意任务类型,从TASK继承，重写run方法，实现
class Task {
public:
	virtual void run() = 0;
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
	using ThreadFunc = std::function<void()>;
	//线程的构造
	Thread(ThreadFunc func);
	// 线程的析构
	~Thread();
	//启动线程
	void start();

private:
	ThreadFunc func_;
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


	//设置task任务队列上限阈值
	void setTaskQueMaxThreshHold(int threahhold);

	//给线程池提交任务,生产者
	void submitTask(std::shared_ptr<Task>sp);

	//禁用拷贝构造和赋值构造
	ThreadPool(const ThreadPool&) = delete;//拷贝构造
	ThreadPool &operator = (const ThreadPool&) = delete;//赋值构造
private:
	//定义线程函数
	void threadFunc();
	
private:
	//把Thread*改为用智能指针管理
	std::vector<std::unique_ptr<Thread>>threads_;  //线程列表
	size_t initThreadSize_;         //初始线程数量
	/*
	* 任务队列为什么使用智能指针？
	*/
	std::queue<std::shared_ptr<Task>>taskQue_; //任务队列
	std::atomic_int taskSize_; // 任务的数量
	int taskQueMaxThreshHold_; //任务队列数量上限阈值

	std::mutex taskQueMtx_; //保证任务队列线程安全

	std::condition_variable notFull_;  //表示任务队列不满,可以继续生产
	std::condition_variable notEmpty_; //表示任务队列不空，可以继续加入

	PoolMode poolMode_;//当前线程池的工作模式
};

#endif