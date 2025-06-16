#include <errno.h>
#include <unistd.h>
#include <string.h>

#include "EPollPoller.h"
#include "Logger.h"
#include "Channel.h"

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
    //epoll_wait在一段超时时间内等待一组文件描述符上的事件，发生的事件数量不超过events.size(),返回需要处理的事件处理数目
    int numEvents = ::epoll_wait(epollfd_, &*events_.begin(), static_cast<int>(events_.size()), timeoutMs);
    int saveErrno = errno;
    Timestamp now(Timestamp::now());
    if (numEvents > 0)
    {
        //有事件发生
        LOG_INFO << "events happend " << numEvents;
        fillActiveChannels(numEvents, activeChannels);
        if (numEvents == events_.size()) //进行扩容，说明下次很有可能发生事件数量大于这次的数量
        {
            events_.resize(events_.size() * 2);
        }
    }
    else if (numEvents == 0)
    {
        //超时
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
    const Channel::ChannelStatus index = channel->index();
    LOG_INFO << "func=>fd=" << channel->fd() << " events= " << channel->events() << " index=" << index;
    // 将Channel添加到本Poller中
    if (index == Channel::CHANNEL_NEW || index == Channel::CHANNEL_DEL)
    {
        if (index == Channel::CHANNEL_NEW)
        {
            int fd = channel->fd();
            channels_[fd] = channel;
        }
        else
        {
            //已经从Poller中删除了
        }
        channel->set_index(Channel::CHANNEL_ADD);
        update(EPOLL_CTL_ADD, channel);
    }
    else //index==ADDED，如果Channel已经添加过
    {
        //channel已经注册过了
        int fd = channel->fd();
        if (channel->isNoneEvent()) // 如果是空事件，则进行删除
        {
            update(EPOLL_CTL_DEL, channel);
            channel->set_index(Channel::CHANNEL_DEL);
        }else // 如果存在其他事件，则修改Channel的监听事件
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

    Channel::ChannelStatus index = channel->index();
    if (index == Channel::CHANNEL_ADD)
    {
        update(EPOLL_CTL_DEL, channel); // 删除chennel
    }
    channel->set_index(Channel::CHANNEL_NEW);
}

// 填写活跃的连接
void EPollPoller::fillActiveChannels(int numEvents, ChannelList* activeChannels) const
{
    for (int i = 0; i < numEvents; ++i)
    {
        Channel* channel = static_cast<Channel*>(events_[i].data.ptr);
        channel->set_revents(events_[i].events);
        activeChannels->push_back(channel); // 设置所有发生事件的channel列表
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
    // 对文件描述符fd进行operation操作，并且该文件描述符对应的epoll_event结构内容为event
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