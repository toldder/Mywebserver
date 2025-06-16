#ifndef __TIMER_QUEUE_H__
#define __TIMER_QUEUE_H__
#include<vector>
#include<set>
#include<utility>
#include<functional>

#include"Timestamp.h"
#include"Channel.h"

class EventLoop;
class Timer;

class TimerQueue
{
public:
	using TimerCallback = std::function<void()>;
	explicit TimerQueue(EventLoop* loop);
	~TimerQueue();

	//插入定时器(回调函数，到期时间，是否重复)
	void addTimer(TimerCallback cb, Timestamp when, double interval);
private:
	using Entry = std::pair<Timestamp, Timer*>;
	using TimerList = std::set<Entry>; //set集合排序的时候需要Timestamp的比较函数
	//在本loop中添加定时器，线程安全
	void addTimerInLoop(Timer* timer);
	//在本定时器触发读事件
	void handleRead();
	//重新设置timerfd_;
	void resetTimerfd(int timerfd_, Timestamp expiration);
	//移除所有已到期的定时器
	std::vector<Entry> getExpired(Timestamp now);
	void reset(const std::vector<Entry>& expired, Timestamp now);
	//插入定时器的内部方法
	bool insert(Timer* timer);
private:
	EventLoop* loop_; //所属的EventLoop
	const int timerfd_; //Linux提供的定时器接口
	Channel timerfdChannel_; //封装timerfd_文件描述符
	TimerList timers_; //定时器队列，内部为红黑树
	bool callingExpiredTimers_; //是否正在获取超时定时器
};

#endif // __TIMER_QUEUE_H__
