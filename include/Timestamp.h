#ifndef __TIME_STAMP_H__
#define __TIME_STAMP_H__

#include<iostream>
#include<sys/time.h>
#include<string>


class Timestamp
{
public:
	Timestamp() :microSecondsSinceEpoch_(0) {}
	explicit Timestamp(int64_t microSecondsSinceEpoch) :microSecondsSinceEpoch_(microSecondsSinceEpoch) {}

	//获取当前时间戳
	static Timestamp now();

	//格式化输出时间戳,并选择是否输出微秒
	std::string toFormattedString(bool showMicroseconds = false)const;

	//返回当前时间戳的微秒
	int64_t microSecondsSinceEpoch()const { return microSecondsSinceEpoch_; }

	//返回当前时间戳的秒数
	time_t secondsSinceEpoch()const
	{
		return static_cast<time_t>(microSecondsSinceEpoch_ / MicroSecondsPerSecond);//time_t表示从epoch开始经历的秒数
	}

	//失效的时间戳，返回一个值为0的时间戳
	static Timestamp invalid() { return Timestamp(); }
	static const int MicroSecondsPerSecond = 1000 * 1000;

	void swap(Timestamp& other)
	{
		std::swap(microSecondsSinceEpoch_, other.microSecondsSinceEpoch_);
	}
private:
	int64_t microSecondsSinceEpoch_;//表示时间戳的微秒数(自epoch开始经历的微妙数)
};

//定时器需要比较时间戳，因此需要重载运算符
inline bool operator<(const Timestamp& lhs, const Timestamp& rhs)
{
	return lhs.microSecondsSinceEpoch() < rhs.microSecondsSinceEpoch();
}

inline bool operator==(const Timestamp& lhs, const Timestamp& rhs)
{
	return lhs.microSecondsSinceEpoch() == rhs.microSecondsSinceEpoch();
}

//如果是重复任务需要对时间戳进行增加
inline Timestamp addTime(Timestamp stamp, double seconds)
{
	//将延时的秒数转换为毫秒
	int64_t microSeconds = seconds * Timestamp::MicroSecondsPerSecond;
	return Timestamp(stamp.microSecondsSinceEpoch() + microSeconds);
}

#endif // !__TIME_STAMP_H__

