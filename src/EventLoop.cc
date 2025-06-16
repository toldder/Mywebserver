#include<sys/eventfd.h>
#include<memory>
#include<errno.h>
#include<fcntl.h>
#include<unistd.h>

#include"Logger.h"
#include"EventLoop.h"
#include"Channel.h"
#include"Poller.h"

//保证一个线程只有一个EventLoop
//thread_local在每个线程里面都有单独一份副本
thread_local EventLoop* t_loopInThisThread = nullptr;

//定义默认的IO复用超时时间
const int PollTimeMs = 10000; //单位是毫秒

//eventfd是一种专门用于事件通知的描述符，是一个计数器
//eventfd不为0表示有可读事件发生，read则会将eventfd置为0，write递增计数器。
//通过eventfd在线程之间传递数据的好处是多个线程无需上锁就可以实现同步
int createEventfd()
{
	int evtf = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);//EFD_CLOEXEC设置文件描述符为非阻塞模式，在执行exec()时自动关闭文件描述符
	if (evtf < 0)
	{
		LOG_FATAL << "eventfd eroor:%d" << errno;
	}
	return evtf;
}

EventLoop::EventLoop()
	:looping_(false), quit_(false), callingPendingFunctors_(false),
	threadId_(CurrentThread::tid()), poller_(Poller::newDefaultPoller(this)),
	wakeupFd_(createEventfd()),wakeupChannel_(new Channel(this,wakeupFd_))
{
	LOG_DEBUG << " EventLoop created " << this << " in the thread" << threadId_;
	if (t_loopInThisThread)
	{
		LOG_FATAL << " Another EventLoop " << t_loopInThisThread << " exists in the thread " << threadId_;
	}else
	{
		t_loopInThisThread = this;
	}
	wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));
	wakeupChannel_->enableReading(); //注册监听wakeupChannel_的EPOLL读事件了
}

EventLoop::~EventLoop()
{
	wakeupChannel_->disableAll(); //给channel移除所有感兴趣的事件
	wakeupChannel_->remove();//把Channel从EventLoop上删除掉
	::close(wakeupFd_); //关闭事件描述符
	t_loopInThisThread = nullptr;
}

void EventLoop::loop()
{
	looping_ = true;
	quit_ = false;

	LOG_INFO << " EventLoop start looping!";
	while (!quit_)
	{
		activeChannels_.clear();
		pollReturnTime_ = poller_->poll(PollTimeMs, &activeChannels_);
		for (Channel* channel : activeChannels_)
		{
			channel->handleEvent(pollReturnTime_);
		}
		doPendingFunctors(); //执行当前EventLoop需要回调的操作
	}
	LOG_INFO << "EventLoop stop looping";
	looping_ = false;
}

/*
* 退出事件循环
* 1.如果loop在自己的线程中调用quit成功了，说明当前线程已经执行完毕了loop()函数的poller_->poll并退出
* 2.如果不是当前EventLoop所属线程中调用quit退出EventLoop 需要唤醒EventLoop所属线程的epoll_wait
*/
void EventLoop::quit()
{
	quit_ = true;
	if (!isInLoopThread())
	{
		wakeup();
	}
}

//在当前loop中执行cb
void EventLoop::runInLoop(Functor cb)
{
	if (isInLoopThread()) //在当前EventLoop中执行回调
	{
		cb();
	}
	else //在非当前EventLoop线程中执行cb,就需要唤醒EventLoop所在线程执行cb
	{
		queueInLoop(cb);
	}

}

// 将回调函数cb安全地添加到事件循环的任务队列中，并在必要时唤醒事件循环线程去执行
void EventLoop::queueInLoop(Functor cb)
{
	{
		std::unique_lock<std::mutex>lock(mutex_); //确保能够插入到队列中，因为vector中的操作并不是原子操作
		pendingFunctors_.emplace_back(cb);
	}

	/**
	 * 判断是否需要唤醒事件循环线程
	 * 1. !isInLoopThread，如果当前调用queueInLoop的线程不是EventLoop所属线程
	 *	此时必须唤醒EventLoop线程，否则新加入的回调可能无法及时执行，因为EventLoop可能正阻塞在poll等待IO事件
	 * 2. callingPendingFunctors_,当前正在执行队列中的其他回调函数，如果不唤醒
	 *	则新加入的回调会延迟到下一次poll之后才执行(可能造成延迟)
	 *  */
	 //callingPendingFunctors的意思是 当前Loop正在执行回溯中，但是loop的pendingFunctors_又加入了新的回调
	//需要wakeup写事件，唤醒相应的执行回调操作的loop所在的线程，让loop()下一次poller_->poll不在阻塞，
	//阻塞的话会延迟前一次新加入的回调的执行，然后继续执行pendingFunctors_中的回调函数
	if (!isInLoopThread() || callingPendingFunctors_)
	{
		wakeup(); //唤醒loop所在线程
	}
}

/**
 * wakeupFd_的工作流程：
 * 1.其他线程调用EventLoop::wakeup()时，会向wakeupFd_写入一个字节数据(如EventFd+1);
 * 2. 事件触发：Poller监听到wakeupFd_的可读事件，使阻塞在poll/epoll的EventLoop线程返回
 * 3. 清楚状态：EventLoop线程通过read读取wakeupFd_中的数据，重置其状态，避免重复触发
 */

void EventLoop::handleRead()
{
	uint64_t one = 1;
	ssize_t n = read(wakeupFd_, &one, sizeof(one)); // 返回实际读取到的字节数量，one存储从wakeFd_读取到的数据
	if (n != sizeof(one))
	{
		LOG_ERROR << "EventLoop:handleRead() reads " << n << "bytes instead of 8";
	}
}

//用来唤醒loop所在线程，向wakeupFd_写一个数据，wakeupChannel就发生读事件，当前loop线程就会被唤醒
void EventLoop::wakeup()
{
	uint64_t one = 1;
	ssize_t n = write(wakeupFd_, &one, sizeof(one)); // 写入数据，出发wakeupFd_的可读事件
	if (n != sizeof(one))
	{
		LOG_ERROR << " EventLoop::wakeup() writes " << n << " bytes instead of 8";
	}
}

//EventLoop的方法 =》Poller的方法
void EventLoop::updateChannel(Channel* channel)
{
	poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel* channel)
{
	poller_->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel* channel)
{
	return poller_->hasChannel(channel);
}

void EventLoop::doPendingFunctors()
{
	std::vector<Functor> functors;
	callingPendingFunctors_ = true;
	{
		std::unique_lock<std::mutex> lock(mutex_);
		// 交换的方式减少了锁的临界区范围 提升效率 同时避免了死锁 
		//如果执行functor()在临界区内 且functor()中调用queueInLoop()就会产生死锁
		functors.swap(pendingFunctors_);
	}

	for (const Functor& functor : functors)
	{
		functor();//执行当前loop需要执行的回溯操作
	}
	callingPendingFunctors_ = false;
}
