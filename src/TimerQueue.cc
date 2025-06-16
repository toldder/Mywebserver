#include<sys/timerfd.h>
#include<unistd.h>
#include<cstring>

#include"TimerQueue.h"
#include"Logger.h"
#include"EventLoop.h"
#include"Channel.h"
#include"Timer.h"
#include"Timestamp.h"

int createTimerfd()
{
    /**
     * CLOCK_MONOTONIC 绝对时间
     * TFD_NONBLOCK 非阻塞
     */
    int timerfd = ::timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC | TFD_NONBLOCK);
    if (timerfd < 0)
    {
        LOG_ERROR << "failed in timerfd_create.";
    }
    return timerfd;
}

void ReadTimerFd(int timerfd)
{
    uint64_t read_byte;
    ssize_t readn = ::read(timerfd, &read_byte, sizeof(read_byte));
    if (readn != sizeof(read_byte))
    {
        LOG_ERROR << "TimerQueue::ReadTimerFd read_size <0";
    }
}

TimerQueue::TimerQueue(EventLoop* loop)
    :loop_(loop), timerfd_(createTimerfd()),
    timerfdChannel_(loop_, timerfd_), timers_(),callingExpiredTimers_(false)
{
    timerfdChannel_.setReadCallback(std::bind(&TimerQueue::handleRead, this));
    timerfdChannel_.enableReading();
}

TimerQueue::~TimerQueue()
{
    timerfdChannel_.disableAll();
    timerfdChannel_.remove();
    ::close(timerfd_);
    //删除所有定时器
    for (const Entry& timer : timers_)
    {
        delete timer.second;
    }
}

//插入定时器(回调函数，到期时间，是否重复)
void TimerQueue::addTimer(TimerCallback cb, Timestamp when, double interval)
{
    Timer* timer = new Timer(std::move(cb), when, interval);
    loop_->runInLoop(std::bind(&TimerQueue::addTimerInLoop, this, timer));
}

//在本loop中添加定时器，线程安全
void TimerQueue::addTimerInLoop(Timer* timer)
{
    //是否取代了最早的定时触发时间
    bool earliestChanged = insert(timer);
    //重新设置timerfd_的触发时间
    if (earliestChanged)
    {
        resetTimerfd(timerfd_, timer->expiration());
    }

}

//在本定时器触发读事件
void TimerQueue::handleRead()
{
    Timestamp now = Timestamp::now();
    ReadTimerFd(timerfd_);

    std::vector<Entry> expired = getExpired(now);

    //遍历到期的定时器，调用回调函数
    callingExpiredTimers_ = true;
    for (const Entry& it : expired)
    {
        it.second->run();
    }
    callingExpiredTimers_ = false;
    reset(expired, now);
}

//重新设置timerfd_;
void TimerQueue::resetTimerfd(int timerfd_, Timestamp expiration)
{
    struct itimerspec nowValue;
    struct itimerspec oldValue;
    memset(&nowValue, '\0', sizeof(nowValue));
    memset(&oldValue, '\0', sizeof(oldValue));

    //超时时间-现在时间
    int64_t microSecondDif = expiration.microSecondsSinceEpoch() - Timestamp::now().microSecondsSinceEpoch();
    if (microSecondDif < 100) microSecondDif = 100;
    struct timespec ts;
    ts.tv_sec = static_cast<time_t>(microSecondDif / Timestamp::MicroSecondsPerSecond);
    ts.tv_nsec = static_cast<long>((microSecondDif % Timestamp::MicroSecondsPerSecond) * 1000);
    nowValue.it_value = ts;   //it_interval为0表示只定时一次

    // 此函数会唤醒事件循环
    if (::timerfd_settime(timerfd_, 0, &nowValue, &oldValue) == -1)
    {
        LOG_ERROR << "timerfd_settime faield()" << errno << " " << strerror(errno);
    }
}

//移除所有已到期的定时器
std::vector<TimerQueue::Entry> TimerQueue::getExpired(Timestamp now)
{
    std::vector<TimerQueue::Entry> expired;
    TimerQueue::Entry sentry(now, reinterpret_cast<Timer*>(UINTPTR_MAX));
    TimerList::iterator end = timers_.lower_bound(sentry); //返回第一个>=now的迭代器
    std::copy(timers_.begin(), end, back_inserter(expired)); //如果超出容器大小，会默认分配空间
    timers_.erase(timers_.begin(), end);
    return expired;
}

void TimerQueue::reset(const std::vector<Entry>& expired, Timestamp now)
{
    for (const Entry& it : expired)
    {
        //重复任务则继续执行
        if (it.second->repeat())
        {
            Timer* timer = it.second;
            timer->restart(Timestamp::now());
            insert(timer);
        }
        else
        {
            delete it.second;
        }

        //如果重新插入了定时器，需要继续重置timerfd
        if (!timers_.empty())
        {
            resetTimerfd(timerfd_, (timers_.begin()->second)->expiration());
        }
    }
}

//插入定时器的内部方法
bool TimerQueue::insert(Timer* timer)
{
    bool earliestChanged = false;
    Timestamp when = timer->expiration();
    TimerList::iterator it = timers_.begin();
    if (it == timers_.end() || when < it->first)
    {
        //最早的定时器已经被替换了
        earliestChanged = true;
    }
    timers_.insert(Entry(when, timer));
    return earliestChanged;
}