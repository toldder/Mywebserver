#ifndef __THREAD_H__
#define __THREAD_H__

#include<memory>
#include<thread>
#include<atomic>
#include<unistd.h>
#include<string>
#include<functional>

#include"noncopyable.h"

class Thread:noncopyable 
{
public:
	using ThreadFunc = std::function<void()>;
	explicit Thread(ThreadFunc, const std::string& name = std::string());
	~Thread();

	void start();
	void join();

	bool started() const { return started_; }
	pid_t tid() const { return tid_; }
	const std::string& name() const { return name_;}

	static int numCreated() { return numCreated_; }
private:
	void setDefaultName();
	bool started_;
	bool joined_;
	std::shared_ptr<std::thread> thread_; //thread对象一旦构建成功就开始运行了，使用智能指针是为了自己决定什么时候运行
	pid_t tid_; //在线程创建时在绑定
	ThreadFunc func_; //线程回调函数
	std::string name_;
	static std::atomic_int numCreated_;
};

#endif // !__THREAD_H__
