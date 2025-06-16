#include <errno.h>
#include <unistd.h>
#include <string.h>

#include "EPollPoller.h"
#include "Logger.h"
#include "Channel.h"

const int MNew = -1;    // 某个channel还没添加至Poller          // channel的成员index_初始化为-1
const int Added = 1;   // 某个channel已经添加至Poller
const int Deleted = 2; // 某个channel已经从Poller删除

//EPOLL_CLOEXEC在创建epoll实例时设置文件描述符的执行时关闭标志
EPollPoller::EPollPoller(EventLoop* loop)
    :Poller(loop), epollfd_(::epoll_create1(EPOLL_CLOEXEC)), events_(InitEventListSize)
{
    if (epollfd_ < 0)
    {
        LOG_FATAL << "epoll_create error:%d\n" << errno;
    }
}

EPollPoller::~EPollPoller()
{
    ::close(epollfd_);
}

Timestamp EPollPoller::poll(int timeoutMs, ChannelList* activeChannels)
{
    LOG_INFO << "fd total count:" << channels_.size();
    //epoll_wait在一段超时时间内等待一组文件描述符上的事件
    int numEvents = ::epoll_wait(epollfd_, &*events_.begin(), static_cast<int>(events_.size()), timeoutMs);
    int saveErrno = errno;
    Timestamp now(Timestamp::now());
    if (numEvents > 0)
    {
        //有事件发生
        LOG_INFO << "events happend " << numEvents;
        fillActiveChannels(numEvents, activeChannels);
        if (numEvents == events_.size()) //进行扩容
        {
            events_.resize(events_.size() * 2);
        }
    }
    else if (numEvents == 0)
    {
        //没有事件发生
        LOG_DEBUG << "timeout!";
    }
    else
    {
        //EINTR表示超时或者是被其他信号调用打断
        if (saveErrno != EINTR)
        {
            errno = saveErrno;
            LOG_ERROR << "EpollPoller::poll() eror";
        }
    }
    return now;
}


void EPollPoller::updateChannel(Channel* channel)
{
    const int index = channel->index();
    LOG_INFO << "func=>fd=" << channel->fd() << "events=" << channel->events() << "index=" << index;
    if (index == MNew || index == Deleted)
    {
        if (index == MNew)
        {
            int fd = channel->fd();
            channels_[fd] = channel;
        }
        else
        {
            //已经从Poller中删除了
        }
        channel->set_index(Added);
        update(EPOLL_CTL_ADD, channel);
    }
    else //index==ADDED
    {
        //channel已经注册过了
        int fd = channel->fd();
        if (channel->isNoneEvent())
        {
            update(EPOLL_CTL_DEL, channel);
            channel->set_index(Deleted);
        }else
        {
            update(EPOLL_CTL_MOD, channel);
        }
    }
}


// 从Poller中删除channel
void EPollPoller::removeChannel(Channel* channel)
{
    int fd = channel->fd();
    channels_.erase(fd);

    LOG_INFO << "removeChannel fd=" << fd;

    int index = channel->index();
    if (index == Added)
    {
        update(EPOLL_CTL_DEL, channel);
    }
    channel->set_index(MNew);
}

// 填写活跃的连接
void EPollPoller::fillActiveChannels(int numEvents, ChannelList* activeChannels) const
{
    for (int i = 0; i < numEvents; ++i)
    {
        Channel* channel = static_cast<Channel*>(events_[i].data.ptr);
        channel->set_revents(events_[i].events);
        activeChannels->push_back(channel); // EventLoop就拿到了它的Poller给它返回的所有发生事件的channel列表了
    }
}

// 更新channel通道 其实就是调用epoll_ctl add/mod/del
void EPollPoller::update(int operation, Channel* channel)
{
    epoll_event event;
    ::memset(&event, 0, sizeof(event));

    int fd = channel->fd();

    event.events = channel->events();
    event.data.fd = fd;
    event.data.ptr = channel;

    if (::epoll_ctl(epollfd_, operation, fd, &event) < 0)
    {
        if (operation == EPOLL_CTL_DEL)
        {
            LOG_ERROR << "epoll_ctl del error:" << errno;
        }
        else
        {
            LOG_FATAL << "epoll_ctl add/mod error:" << errno;
        }
    }
}