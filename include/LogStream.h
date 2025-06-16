#ifndef __LOG_STREAM_H__
#define __LOG_STREAM_H__

#include<string>

#include"noncopyable.h"
#include"FixedBuffer.h"

class GeneralTemplate : noncopyable //默认private继承
{
public:
	GeneralTemplate():data_(nullptr),len_(0)
	{
	
	}
	explicit GeneralTemplate(const char* data, int len)
		:data_(data), len_(len)
	{

	}
	const char* data_;
	int len_;
};

class LogStream:noncopyable
{
public:
	using Buffer = FixedBuffer<MSmallBufferSize>;
	
	//将指定字符追加到缓冲区
	void append(const char* buffer, int len)
	{
		buffer_.append(buffer, len);
	}

	//返回当前缓冲区
	const Buffer& buffer()const { return buffer_; }

	//重置缓冲区
	void reset_buffer()
	{
		buffer_.reset();
	}

	//重载流运算操作符
	LogStream& operator<<(bool express);
	LogStream& operator<<(short number);
	LogStream& operator<<(unsigned short);
	LogStream& operator<<(int);
	LogStream& operator<<(unsigned int);
	LogStream& operator<<(long);
	LogStream& operator<<(unsigned long);
	LogStream& operator<<(long long);
	LogStream& operator<<(unsigned long long);
	LogStream& operator<<(float number);
	LogStream& operator<<(double);
	LogStream& operator<<(char str);
	LogStream& operator<<(const char*);
	LogStream& operator<<(const unsigned char*);
	LogStream& operator<<(const std::string&);
	//(const char*,int)类型的重载
	LogStream& operator<<(const GeneralTemplate& g);
private:
	//对整形进行特殊处理
	template<typename T>
	void formatInteger(T num);
	static constexpr int MaxNumberSize = 32;
	Buffer buffer_;
};

#endif // !__LOG_STREAM_H__
