#ifndef __EVENT_LOO_H__
#define __EVENT_LOO_H__

#include<vector>
#include<atomic>
#include<memory>
#include<mutex>
#include<functional>

#include"noncopyable.h"
#include"Timestamp.h"
#include"TimerQueue.h"
#include"CurrentThread.h"

class Channel;
class Poller;


//EventLoop，持续监听、持续获取监听结果、持续处理监听结果对应的事件的能力
//即循环调用Poller::poll方法获取实际发生的Channel集合，然后调用Channel里面保管的不同类型事件的处理函数
class EventLoop :noncopyable
{
public:
	using Functor = std::function<void()>;
	EventLoop();
	~EventLoop();

	//开启事件循环
	void loop();
	//退出事件循环
	void quit();

	Timestamp pollReturnTime()const { return pollReturnTime_; }

	//在当前loop中执行
	void runInLoop(Functor cb);
	//把上层注册的回调函数cb放入队列中，唤醒Loop所在线程执行id
	void queueInLoop(Functor cb);
	//通过eventfd唤醒loop所在的线程
	void wakeup();

	//调用Poller的方法
	void updateChannel(Channel* channel);
	void removeChannel(Channel* channel);
	bool hasChannel(Channel* channel);

	//判断EventLoop对象是否在自己的线程里
	bool isInLoopThread()const
	{
		return threadId_ == CurrentThread::tid();
	}

	//定时任务相关函数
	void runAt(Timestamp timestamp, Functor&& cb)
	{
		timerQueue_->addTimer(std::move(cb), timestamp, 0.0);
	}

	void runAfter(double waitTime, Functor&& cb)
	{
		Timestamp time(addTime(Timestamp::now(), waitTime));
		runAt(time, std::move(cb));
	}

	void runEvery(double interval, Functor&& cb)
	{
		Timestamp timestamp(addTime(Timestamp::now(), interval));
		timerQueue_->addTimer(std::move(cb), timestamp, interval);
	}
private:
	//给eventfd返回的文件描述符wakeupFd_绑定的事件回调,当wakeup()时，即有事件发生，
	//调用handleRead()读取wakeupFd_的8字节，同时唤醒阻塞的epoll_wait
	void handleRead();
	void doPendingFunctors();//执行上层回调
private:
	using ChannelList = std::vector<Channel*>;

	std::atomic_bool looping_;
	std::atomic_bool quit_; //标志退出Loop循环
	const pid_t threadId_;//表示当前EventLoop所属的线程id

	Timestamp pollReturnTime_;//Poller返回发生事件的Channels的时间点
	std::unique_ptr<Poller> poller_;
	std::unique_ptr<TimerQueue> timerQueue_;

	//当MainLoop获取一个新用户的Channel时，需要通过轮询算法选择一个subLoop，通过该成员唤醒subLoop处理Channel
	int wakeupFd_;
	std::unique_ptr<Channel> wakeupChannel_;
	ChannelList activeChannels_; //事件监测结果

	std::atomic_bool callingPendingFunctors_; //是否需要执行回调操作
	std::vector<Functor> pendingFunctors_; //执行的回调操作
	std::mutex mutex_; //互斥锁，用来保护vector容器的线程安全
};


#endif // !__EVENT_LOO_H__
