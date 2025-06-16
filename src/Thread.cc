#include<semaphore.h>

#include"Thread.h"
#include"CurrentThread.h"

std::atomic_int Thread::numCreated_(0);

Thread::Thread(ThreadFunc func, const std::string& name)
	:started_(false),joined_(false),tid_(0),func_(std::move(func)),name_(name)
{
	if(name_.empty()) setDefaultName();
}

Thread::~Thread()
{
	if (started_ && !joined_)
	{
		thread_->detach();
	}
}

void Thread:: start()
{
	started_ = true;
	sem_t sem;
	sem_init(&sem, false, 0); //false 表示这个信号不在线程之间进行贡献，0表示指定信号量的初始值为0
	//开启线程
	thread_ = std::shared_ptr<std::thread>(new std::thread(
		[&]() {
			tid_ = CurrentThread::tid();
			sem_post(&sem);   //对信号量进行加1，即解锁
			func_(); //新线程的任务是执行该func_函数
		} //新建的线程执行该Lambda函数
	));

	// 这里必须等待获取上面新创建的线程的tid值
	sem_wait(&sem); //对信号量减1，如果sem为0，则将阻塞，直到信号量大于0
}

void Thread::join()
{
	joined_ = true;
	thread_->join();
}

void Thread::setDefaultName()
{
	int num = ++numCreated_;
	if (name_.empty())
	{
		char buf[32] = { 0 };
		snprintf(buf, sizeof(buf), "Thread%d", num);
		name_ = buf;
	}
}