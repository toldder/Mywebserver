#include "Logger.h"
#include"CurrentThread.h"

namespace ThreadInfo
{
	thread_local char t_errnobuf[512];//每个线程独立的错误信息缓冲
	thread_local char t_timer[64]; //每个线程独立的时间格式化缓冲区
	thread_local time_t t_lastSecond;//每个线程记录上次格式化的时间
}

const char* getErrnoMsg(int savedErrno)
{
	return strerror_r(savedErrno, ThreadInfo::t_errnobuf, sizeof(ThreadInfo::t_errnobuf));
}
//根据Level返回level名字
const char* getLevelName[Logger::LogLevel::LEVEL_COUNT]
{
	"TRACE",
	"DDEBUG",
	"INFO",
	"WARN",
	"ERROR",
	"FATAL",
};

/**
* 默认的日志输出函数
* 将日志内容写入标准输出流
* @param data 要输出的日志数据
* @param len 日志数据的长度w
*/
static void defaultOutput(const char* data, int len)
{
	fwrite(data, len, sizeof(char), stdout);
}

/** 
* 默认的刷新函数
* 刷新标准输出流的缓冲区，确保日志及时输出
* 在发生错误或需要立即看到日志时会被调用
*/
static void defaultFlush()
{
	fflush(stdout);
}

Logger::OutputFunc g_output = defaultOutput;
Logger::FlushFunc g_flush = defaultFlush;

Logger::Logger(const char* filename, int line, LogLevel level):impl_(level,0,filename,line)
{

}

Logger::~Logger()
{
	impl_.finish();
	const LogStream::Buffer& buffer = stream().buffer();
	g_output(buffer.data(), buffer.length());
	if (impl_.level_ == FATAL)
	{
		g_flush();
		abort();
	}
}

void Logger::setOutput(OutputFunc output)
{
	g_output = output;
}

void Logger::setFlush(FlushFunc flush)
{
	g_flush = flush;
}

Logger::Impl::Impl(LogLevel level, int savedErrno, const char* filename, int line)
	:time_(Timestamp::now()), stream_(), level_(level), line_(line), basename_(filename)
{
	//根据时区格式化当前时间字符串，也是一条log消息
	formatTime();
	stream_ << GeneralTemplate(getLevelName[level], 6);
	if (savedErrno != 0)
	{
		stream_ << getErrnoMsg(savedErrno) << "(errno=" << savedErrno << ")";
	}
}

void Logger::Impl::formatTime()
{
	Timestamp now = Timestamp::now();
	//计算秒数
	time_t seconds = static_cast<time_t>(now.microSecondsSinceEpoch() / Timestamp::MicroSecondsPerSecond);
	int microseconds = static_cast<int> (now.microSecondsSinceEpoch() % Timestamp::MicroSecondsPerSecond);
	//计算剩余微秒数
	struct tm* tm_timer = localtime(&seconds);
	//写入此线程存储的时间buf中
	snprintf(ThreadInfo::t_timer, sizeof(ThreadInfo::t_timer), "%4d/%02d/%02d %02d:%02d:%02d",
		tm_timer->tm_year + 1900,
		tm_timer->tm_mon + 1,
		tm_timer->tm_mday,
		tm_timer->tm_hour,
		tm_timer->tm_min,
		tm_timer->tm_sec);
	//更新最后一次时间调用
	ThreadInfo::t_lastSecond = seconds;

	char buf[32] = { 0 };
	snprintf(buf, sizeof(buf), "%06d", microseconds);

	stream_ << GeneralTemplate(ThreadInfo::t_timer, 17) << GeneralTemplate(buf, 7);
}

//添加一条Log消息的后缀
void Logger::Impl::finish()
{
	stream_ << " - " << GeneralTemplate(basename_.data_, basename_.size_)
		<< ':' << line_ << '\n';
}