#ifndef THREADPOLL_H //��ֹ�ظ�����
#define THREADPOLL_H

//ͷ�ļ�
#include <vector>
#include <queue>
#include <memory>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <functional>

//Any���ͣ����Խ����������ݵ�����

class Any {
public:
	//Ĭ�Ϲ��캯��
	Any() = default;
	~Any() = default;

	//��ֹ��������͸�ֵ����
	Any (const Any&) = delete;
	Any & operator = (const Any&) = delete;

	//��ֵ����ʹ��Ĭ�ϵľͿ���
	Any(Any && ) = default; 

	//��ֵ��ֵ���캯��
	Any& operator = ( Any&&) = default;

	//��ģ������������������
	template<typename T>
	//����ָ��ָ��������
	Any(T data):base_(std::make_unique<Derive<T>>(data))
	{}

	//���������Any��������洢��data_������ȡ����,תΪ�û���Ҫ������
	template<typename T>
	T cast_()
	{
		//	������ô�� base_�ҵ�����ָ���Derive���󣬴��������ȡdata��Ա����
		//	����ָ��תΪ������ָ��
		Derive<T>* pd = dynamic_cast<Derive<T>*>(base_.get());
		if (pd == nullptr)
		{
			throw "type is unmatch!";
		}
		return pd->data_;
	}

private:
	//Ƕ����
	// 
	//��������
	class Base {
	public:
		virtual ~Base() = default; // �ڼ̳нṹ��, �������������Ӧ������Ϊ����������
	};

	//������   
	template<typename T>
	class Derive :public Base {
	public:
		Derive(T data):data_(data){}  //���캯����ʼ��
		T data_;
	};
private:
	//����һ�������ָ��
	std::unique_ptr<Base>base_;
};


//ʵ��һ���ź�����
class Semaphore 
{
public:
	//���캯��
	Semaphore(int limit = 0) :resLimit_(limit)
	{

	}
	//��������
	~Semaphore() = default;

	//wait������ʵ��  ��ȡ�����ģ�һ���ź�����Դ
	void wait()
	{
		//����һ����Դǰ��Ҫ������
		//�൱��  P ����
		std::unique_lock<std::mutex>lock(mtx_);
		if (resLimit_ <= 0)
		{
			cond_.wait(lock);
		}
		//Ҳ����д�����µ�lambda���ʽ��ʽ
		//cond_.wait(lock, [&]()->bool {if resLimit_ > 0; };)

		//����Ŀǰ����Դ
		resLimit_--; //�ź��� -1
	}


	//post������ʵ�� ����һ���ź�����ʵ��
	//�൱�� V ����
	void post()
	{
		//����һ����Դǰ��Ҫ������
		std::unique_lock<std::mutex>lock(mtx_);
		resLimit_++; //�ź���+1
		cond_.notify_all();
	}



private:
	int resLimit_;
	std::mutex mtx_;
	std::condition_variable cond_;
};

// Task���͵�ǰ������
class Task;
//ʵ�ֽ����ύ���̳߳ص�task����ִ����ɺ�ķ���ֵ����Result
class Result {
public:
	//���캯��,Ĭ��Ϊ��Ч������
	Result(std::shared_ptr<Task>task, bool isValid = true);

	//��������
	~Result() = default;

	/*
	* ����1��setValue��������ȡ����ִ����ķ���ֵ
	* ����2��get�������û��������������ȡtask�ķ���ֵ
	*/

	//��ȡ����ִ����ķ���ֵ
	void setVal(Any any);

	//
	Any get();

private:
	Any any_;  //   �洢����ķ���ֵ
	Semaphore sem_; // �߳�ͨ���ź���
	std::shared_ptr<Task>task_; // ָ���Ӧ��ȡ����ֵ���������
	std::atomic_bool isValid_; //
};



//����������
//�û������Զ���������������,��TASK�̳У���дrun������ʵ��
class Task {
public:
	//���캯��
	Task();
	~Task() = default;



	void setResult(Result* res);
public:
	void exec();
	virtual Any run() = 0;
private:
	//����ʹ������ָ��, �ᵼ������ָ�뽻������
	Result* result_;
};

//�̳߳�֧�ֵ�ģʽ
enum class PoolMode
{
	MODE_FIXED,
	MODE_CATCH,
};




//�߳�����
class Thread{
public:
	using ThreadFunc = std::function<void(int)>;
	//�̵߳Ĺ���
	Thread(ThreadFunc func);
	// �̵߳�����
	~Thread();
	//�����߳�
	void start();

	//��ȡ�߳�id
	int getId() const;

private:
	ThreadFunc func_;
	static int generatedId_;
	int threadId_; //�����߳�id
};


/*




*/





//�̳߳�����
class ThreadPool
{
public:
	//���캯��
	ThreadPool();
	~ThreadPool();

	//�����̳߳�
	void start(int initThreadSize = 4);

	//�����̵߳Ĺ���ģʽ
	void setMode(PoolMode mode);

	//�����̵߳ĳ�ʼ�߳�����
	//void setinitThreadSize(int size);

	//�����̳߳����̵߳�������ֵ(catchģʽ)
	void setThreadSizeThreshHold(int threahhold);


	//����task�������������ֵ
	void setTaskQueMaxThreshHold(int threahhold);

	//���̳߳��ύ����,������
	Result submitTask(std::shared_ptr<Task>sp);

	//���ÿ�������͸�ֵ����
	ThreadPool(const ThreadPool&) = delete;//��������
	ThreadPool &operator = (const ThreadPool&) = delete;//��ֵ����
private:
	//�����̺߳���
	void threadFunc(int threadid); //�����������ʹ�̹߳���

	//���pool������״̬
	bool checkRunningState() const;
	
private:
	//��Thread*��Ϊ������ָ�����
	//std::vector<std::unique_ptr<Thread>>threads_;  //�߳��б�

	//
	std::unordered_map<int, std::unique_ptr<Thread>>threads_;

	size_t initThreadSize_;          //��ʼ�߳�����
	 
	size_t threadSizeThreshHold_;    //�߳�����������ֵ

	std::atomic_int curThreadSize_;  //��¼��ǰ�̳߳��е����߳�

	std::atomic_int idleThreadSize_; //��¼�����̵߳�����

	/*
	* �������Ϊʲôʹ������ָ�룿
	*/
	std::queue<std::shared_ptr<Task>>taskQue_; //�������
	std::atomic_int taskSize_; // ���������
	int taskQueMaxThreshHold_; //�����������������ֵ

	std::mutex taskQueMtx_; //������еĻ���������֤��������̰߳�ȫ

	std::condition_variable notFull_;  //��ʾ������в���,���Լ�������
	std::condition_variable notEmpty_; //��ʾ������в��գ����Լ�������

	PoolMode poolMode_;//��ǰ�̳߳صĹ���ģʽ
	std::atomic_bool isPoolRunning_;  //��ʾ��ǰ�̳߳ص�����״̬

};

#endif