#include "Timestamp.h"
#include "Logger.h"

//获取当前时间戳
Timestamp Timestamp::now()
{
	/**
	 *timeval包含两个成员，time_t tv_sec 时间的秒部分
	 *suseconds_t tv_usec 时间的微秒部分
	*/
	timeval tv;
	int re = gettimeofday(&tv, NULL);
	if (re == -1)
	{
		LOG_ERROR << "gettimeofday error:" << errno;
	}

	return Timestamp(tv.tv_sec * MicroSecondsPerSecond + tv.tv_usec);
}

//格式化输出时间戳,并选择是否输出微秒  "%4d年%02d月%02d日 星期%d %02d:%02d:%02d.%06d",时分秒.微秒"
std::string Timestamp::toFormattedString(bool showMicroseconds)const
{
	char buf[64] = { 0 };
	time_t seconds = static_cast<time_t>(microSecondsSinceEpoch_ / MicroSecondsPerSecond);
	tm* tm_time = localtime(&seconds); //将秒数转换为日历时间
	if (showMicroseconds)
	{
		//显示微秒数
		int microseconds = static_cast<int>(microSecondsSinceEpoch_ % MicroSecondsPerSecond);
		snprintf(buf, sizeof buf, "%4d-%02d-%02d %02d:%02d:%02d.%06d",
			tm_time->tm_year + 1900, tm_time->tm_mon + 1, tm_time->tm_mday,
			tm_time->tm_hour, tm_time->tm_min, tm_time->tm_sec, microseconds);
	}
	else
	{
		snprintf(buf, sizeof buf, "%4d%02d%02d %02d:%02d:%02d",
			tm_time->tm_year + 1990, tm_time->tm_mon + 1, tm_time->tm_mday,
			tm_time->tm_hour, tm_time->tm_min, tm_time->tm_sec);
	}
	return buf;
}