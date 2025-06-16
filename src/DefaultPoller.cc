#include <stdlib.h>

#include "Poller.h"
#include "EPollPoller.h"

Poller* Poller::newDefaultPoller(EventLoop* loop)
{
    if (::getenv("MUDUO_USE_POLL"))
    {
        return nullptr; // 生成poll的实例,目前是先直接返回空指针
    }
    else // 使用epollPoller
    {
        return new EPollPoller(loop); // 生成epoll的实例
    }
}