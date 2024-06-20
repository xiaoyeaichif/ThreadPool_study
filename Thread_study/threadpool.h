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
	//��ֹ��������͸�ֵ
	Any(const Any&) = delete;
	Any & operator=(const Any&) = delete;
	//��ֵ����ʹ��Ĭ��
	Any(Any&&) = default; 

	template<typename T>
	Any(T data):base_(std::make_unique<Derive<T>>(data))
	{}

	//���������Any��������洢��data_������ȡ����
	template<typename T>
	T cast_()
	{
		//������ô�� base_�ҵ�����ָ���Derive���󣬴��������ȡdata��Ա����
		//����ָ��תΪ������ָ��
		Derive<T>* pd = dynamic_cast<Derive<T>*>(base_.get());
		if (pd == nullptr)
		{
			throw "type is unmatch!";
		}
		return pd->data_
	}
private:
	//Ƕ����
	//��������
	class Base {
	public:
		virtual ~Base() = default;
	};

	//������
	template<typename T>
	class Derive :public Base {
	public:
		Derive(T data):data_(data){}
		T data_;
	};
private:
	std::unique_ptr<Base>base_;
};



//����������
//�û������Զ���������������,��TASK�̳У���дrun������ʵ��
class Task {
public:
	virtual void run() = 0;
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
	using ThreadFunc = std::function<void()>;
	//�̵߳Ĺ���
	Thread(ThreadFunc func);
	// �̵߳�����
	~Thread();
	//�����߳�
	void start();

private:
	ThreadFunc func_;
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


	//����task�������������ֵ
	void setTaskQueMaxThreshHold(int threahhold);

	//���̳߳��ύ����,������
	void submitTask(std::shared_ptr<Task>sp);

	//���ÿ�������͸�ֵ����
	ThreadPool(const ThreadPool&) = delete;//��������
	ThreadPool &operator = (const ThreadPool&) = delete;//��ֵ����
private:
	//�����̺߳���
	void threadFunc();
	
private:
	//��Thread*��Ϊ������ָ�����
	std::vector<std::unique_ptr<Thread>>threads_;  //�߳��б�
	size_t initThreadSize_;         //��ʼ�߳�����
	/*
	* �������Ϊʲôʹ������ָ�룿
	*/
	std::queue<std::shared_ptr<Task>>taskQue_; //�������
	std::atomic_int taskSize_; // ���������
	int taskQueMaxThreshHold_; //�����������������ֵ

	std::mutex taskQueMtx_; //��֤��������̰߳�ȫ

	std::condition_variable notFull_;  //��ʾ������в���,���Լ�������
	std::condition_variable notEmpty_; //��ʾ������в��գ����Լ�������

	PoolMode poolMode_;//��ǰ�̳߳صĹ���ģʽ
};

#endif