

#include<iostream>
#include<thread>
#include<mutex>
#include<condition_variable>
#include<chrono> //����ʱ��Ŀ⺯��
#include<queue>
using namespace std;

//ȫ����Ҫ�ı���
//�� �������� �ٽ���Դ��С
mutex mtx;
condition_variable cv;
static const int Max_buffer = 10;
queue<int>buffer;


//������
void producer(int id)
{
	int data = 0;
	while (true)
	{
		/*
			1:һ����������С�Ѿ����ˣ���ô��ʱ�����߾Ͳ����������ݣ���ʱӦ���ͷ�������Դ����������
			�̻߳�ȡ����
			2�������������Сû����Ӧ�ü����������ݣ���֪ͨ�������߳���ȡ����
		*/
		//��ȡ������Դ
		unique_lock<mutex>lock(mtx);
		if (buffer.size() >= Max_buffer)//���д�С��������ܷ��µ�����,��Ҫ�����ͷ��������ҵȴ�
		{
			cv.wait(lock);
		}
		//�������ڵ�Ԫ�ػ�û��
		buffer.push(data);
		cout << "Producer " << id << " produced " << data << endl;
		data++;
		//������֮���ͷ�������֪ͨ������
		lock.unlock();
		cv.notify_all();//֪ͨ�����̻߳�ȡ��Դ

	}
}




//������
void consumer(int id)
{
	while (true)
	{
		//��ȡ��Դǰ������
		unique_lock<mutex>lock(mtx);
		while (buffer.empty())
		{
			/*
				1:���������û��Ԫ��,�������̻߳��ͷ���
				2:��ʱ��Ȼ�������߳��ڵȴ�״̬�����Ǵ�ʱ������������Դ����Ϊû�л�ȡ����
			*/
			cv.wait(lock);
		}
		//����������Դ
		int data = buffer.front();
		buffer.pop();//��Դ����ȡ֮�󵯳�
		cout << "Consumer get " << data << endl;
		//�����߻�ȡ��Դ�����������߼�������
		//�ͷ�����Դ
		lock.unlock();
		//֪ͨ�����߼�������
		cv.notify_all();
		//�����ߵȴ�2��
		std::this_thread::sleep_for(std::chrono::seconds(2));
	}
}

//int main()
//{
//	cout << "���̵߳Ŀ�ʼ" << endl;
//	//�̵߳Ĵ���
//	thread Producer(producer,1);
//	thread Consumer(consumer,2);
//
//	//���̵߳ȴ��������߳̽���
//	Producer.join();
//	Consumer.join();
//
//	cout << "���̵߳Ľ���" << endl;
//	return 0;
//}

int main_thread02()
{
	cout << "���̵߳Ŀ�ʼ" << endl;
	//�̵߳Ĵ���
	thread Producer[2];
	thread Consumer[2];

	//����2���������߳� 2���������߳�
	for (int i = 0; i < 2; i++)
	{
		Producer[i] = thread(producer, i);
		Consumer[i] = thread(consumer, i);
	}

	//���̵߳ȴ��������߳̽���
	for (int i = 0; i < 2; i++)
	{
		Producer[i].join();
		Consumer[i].join();
	}
	

	cout << "���̵߳Ľ���" << endl;
	return 0;
}