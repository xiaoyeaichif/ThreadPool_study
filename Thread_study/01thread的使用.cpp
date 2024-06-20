

#include<iostream>
#include<thread>
#include<string>
#include<chrono> //����ʱ��Ŀ⺯��
using namespace std;

void Thread_1()
{
	cout << "thread-1 run begin!" << endl;
	//�߳�1˯��2�룬�������cpu��ʱ��Ƭ���Էָ������߳�
	this_thread::sleep_for(chrono::seconds(2));
	cout << "thread-1 2����ѣ�run end!" << endl;
}

void Thread_2(int val,string info)
{
	cout << "thread-2 run begin!" << endl;
	cout << "thread-2 args[val:" << val << ",info:" << "]" << endl;
	//�߳�2˯��4��,�������cpu��ʱ��Ƭ���Էָ������߳�
	this_thread::sleep_for(chrono::seconds(4));
	cout << "thread-2 4�������, run end!" << endl;
}


int main_thread01()
{
	cout << "���̵߳Ŀ�ʼ" << endl;
	//�̵߳Ĵ���
	thread t1(Thread_1);
	thread t2(Thread_2,20,"yejie");

	// �ȴ��߳�t1��t2ִ���꣬main�߳��ټ�������
	t1.join();
	t2.join();


	cout << "���̵߳Ľ���" << endl;
	return 0;
}