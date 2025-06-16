#ifndef _ASYNC_LOGGING_H__
#define _ASYNC_LOGGING_H__
#include<vector>
#include<mutex>
#include<memory>
#include<condition_variable>

#include"FixedBuffer.h"
#include"Thread.h"
#include"LogFile.h"


class AsyncLogging {
public:
	AsyncLogging(const std::string& basename, off_t rollSize, int flushInterval = 3);
	~AsyncLogging()
	{
		if (running_)
		{
			stop();
		}
	}

	void append(const char* logline, int len);
	void start()
	{
		running_ = true;
		thread_.start();
	}

	void stop()
	{
		running_ = false;
		cond_.notify_one();
	}
private:
	using LargeBuffer = FixedBuffer<MLargeBufferSize>;
	using BufferVector = std::vector<std::unique_ptr<LargeBuffer>>;
	using BufferPtr = BufferVector::value_type;

	void threadFunc();
	const int flushInterval_; //日志刷新时间
	std::atomic<bool> running_;
	const std::string basename_;
	const off_t rollSize_;
	Thread thread_;
	std::mutex mutex_;
	std::condition_variable cond_;

	BufferPtr currentBuffer_; //准备两个缓冲区
	BufferPtr nextBuffer_;
	BufferVector buffers_;
};

#endif // !_ASYNC_LOGGING_H__
