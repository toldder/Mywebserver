#ifndef __EVENTLOOP_THREAD_POOL_H__
#define __EVENTLOOP_THREAD_POOL_H__

#include <functional>
#include <string>
#include <vector>
#include <memory>

#include "noncopyable.h"
#include "ConsistenHash.h"

class EventLoop;
class EventLoopThread;

class EventLoopThreadPool : noncopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop *)>;

    EventLoopThreadPool(EventLoop *baseLoop, const std::string &nameArg);
    ~EventLoopThreadPool();

    void setThreadNum(int numThreads) { numThreads_ = numThreads; }

    void start(const ThreadInitCallback &cb = ThreadInitCallback());

    // 如果工作在多线程中，baseLoop_(mainLoop)会默认以轮询的方式分配Channel给subLoop
    EventLoop *getNextLoop(const std::string& key);

    std::vector<EventLoop *> getAllLoops(); // 获取所有的EventLoop

    bool started() const { return started_; } // 是否已经启动
    const std::string name() const { return name_; } // 获取名字

private:
    EventLoop *baseLoop_; // 子线程的个数
    std::string name_;//线程池名称，通常由用户指定，线程池中EventLoopThread名称依赖于线程池名称。
    bool started_;//是否已经启动标志
    int numThreads_;//线程池中线程的数量
    int next_; // 新连接到来，所选择EventLoop的索引
    std::vector<std::unique_ptr<EventLoopThread>> threads_;//IO线程的列表
    std::vector<EventLoop *> loops_;//线程池中EventLoop的列表，指向的是EVentLoopThread线程函数创建的EventLoop对象。
    ConsistentHash hash_; // 一致性哈希对象
};

#endif // !__EVENTLOOP_THREAD_POOL_H__
