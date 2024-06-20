

#include<iostream>
#include<thread>
#include<mutex>
#include<condition_variable>
#include<chrono> //控制时间的库函数
#include<queue>
using namespace std;

//全局需要的变量
//锁 条件变量 临界资源大小
mutex mtx;
condition_variable cv;
static const int Max_buffer = 10;
queue<int>buffer;


//生产者
void producer(int id)
{
	int data = 0;
	while (true)
	{
		/*
			1:一旦缓冲区大小已经满了，那么此时生产者就不能生产数据，此时应该释放锁的资源，让消费者
			线程获取锁！
			2：如果缓存区大小没满，应该继续生产数据，并通知消费者线程来取数据
		*/
		//获取锁的资源
		unique_lock<mutex>lock(mtx);
		if (buffer.size() >= Max_buffer)//队列大小大于最大能放下的数据,需要立即释放锁，并且等待
		{
			cv.wait(lock);
		}
		//缓冲区内的元素还没满
		buffer.push(data);
		cout << "Producer " << id << " produced " << data << endl;
		data++;
		//生产完之后释放锁并且通知消费者
		lock.unlock();
		cv.notify_all();//通知所有线程获取资源

	}
}




//消费者
void consumer(int id)
{
	while (true)
	{
		//获取资源前先上锁
		unique_lock<mutex>lock(mtx);
		while (buffer.empty())
		{
			/*
				1:如果缓冲区没有元素,消费者线程会释放锁
				2:此时虽然消费者线程在等待状态，但是此时还不能消费资源，因为没有获取到锁
			*/
			cv.wait(lock);
		}
		//缓冲区有资源
		int data = buffer.front();
		buffer.pop();//资源被获取之后弹出
		cout << "Consumer get " << data << endl;
		//消费者获取资源后提醒消费者继续生产
		//释放锁资源
		lock.unlock();
		//通知生产者继续生产
		cv.notify_all();
		//消费者等待2秒
		std::this_thread::sleep_for(std::chrono::seconds(2));
	}
}

//int main()
//{
//	cout << "主线程的开始" << endl;
//	//线程的创建
//	thread Producer(producer,1);
//	thread Consumer(consumer,2);
//
//	//主线程等待两个子线程结束
//	Producer.join();
//	Consumer.join();
//
//	cout << "主线程的结束" << endl;
//	return 0;
//}

int main_thread02()
{
	cout << "主线程的开始" << endl;
	//线程的创建
	thread Producer[2];
	thread Consumer[2];

	//创建2个消费者线程 2个消费者线程
	for (int i = 0; i < 2; i++)
	{
		Producer[i] = thread(producer, i);
		Consumer[i] = thread(consumer, i);
	}

	//主线程等待两个子线程结束
	for (int i = 0; i < 2; i++)
	{
		Producer[i].join();
		Consumer[i].join();
	}
	

	cout << "主线程的结束" << endl;
	return 0;
}