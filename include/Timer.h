#ifndef __TIMER_H__
#define __TIMER_H__
#include<functional>

#include"noncopyable.h"
#include"Timestamp.h"

class Timer :noncopyable
{
public:
	using TimerCallback = std::function<void()>;
	Timer(TimerCallback cb, Timestamp when, double interval)
		:callback_(std::move(cb)),expiration_(when),interval_(interval),repeat_(interval>0.0)
	{
		
	}

	void run()const
	{
		callback_();
	}

	Timestamp expiration() { return expiration_; }
	bool repeat()const { return repeat_; }

	//重启定时器，如果是非重复事件则到期时间设置为0
	void restart(Timestamp now);
private:
	const TimerCallback callback_; //定时器回调函数
	Timestamp expiration_; //下一次的超时时刻
	const double interval_; //超时的间隔时刻，如果是一次性定时器，则为0
	const bool repeat_; //是否重复，如果为false表示是一次性定时器
};

#endif // !__TIMER_H__
