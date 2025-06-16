#ifndef __FIXED_BUFFER_H__
#define __FIXED_BUFFER_H__

#include<string>
#include<cstring>

#include"noncopyable.h"

constexpr int MSmallBufferSize = 4000;
constexpr int MLargeBufferSize = 4000 * 1000;

//该类提供了一个固定大小的缓冲区，允许将数据追加到缓冲区中，并提供相关的操作方法
template<int buffer_size>
class FixedBuffer :noncopyable
{
public:
	FixedBuffer() :cur_(data_), size_(0)
	{

	}

	//将指定数据追加到缓冲区中
	void append(const char* buf, size_t len) 
	{
		if (avail() > len) //剩余缓冲区大于数据长度
		{
			memcpy(cur_, buf, len);
			add(len);
		}
	}

	//返回缓冲区的起始地址
	const char* data()const { return data_; }

	//返回缓冲区当前有效数据的长度
	int length()const { return size_; }

	//返回当前指针位置
	char* current()const { return cur_; }

	//返回缓冲区中剩余可用空间的大小
	size_t avail()const { return static_cast<size_t>(buffer_size - size_); }

	//更新当前指针，并增加指定长度
	void add(size_t len) {
		cur_ += len;
		size_ += len;
	}

	//重置当前指针
	void reset()
	{
		cur_ = data_;
		size_ = 0;
	}

	//清空缓冲区的数据
	void bzero()
	{
		::bzero(data_, sizeof(data_));
	}

	//将缓冲区中的数据转换为std::string 类型并返回
	std::string toString()const
	{
		return std::string(data_, length());
	}
private:
	char data_[buffer_size];//定义固定大小的缓冲区
	char* cur_;	//当前指针，指向缓冲区下一个可写入位置
	int size_; //缓冲区大小
};

#endif // !__FIXED_BUFFER_H__
