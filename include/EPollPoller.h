#ifndef __EPOLL_POLLER_H__
#define __EPOLL_POLLER_H__

#include<vector>
#include<sys/epoll.h>

#include "Poller.h"
#include "Timestamp.h"

class Channel;

class EPollPoller :public Poller
{
public:
    EPollPoller(EventLoop* loop);
    ~EPollPoller() override;

    //重写基类方法
    Timestamp poll(int timeoutMs, ChannelList* activeChannels)override;
    void updateChannel(Channel* channel)override;
    void removeChannel(Channel* channel)override;
private:
    static const int InitEventListSize = 16;
    //epoll_event包含两个成员,events和data,events记录感兴趣事件/已经发生事件，data记录用户相关数据
    using EventList = std::vector<epoll_event>;
    int epollfd_; //epoll_create方法返回的epoll句柄
    EventList events_; //存放epoll_wait返回的实际发生时间的文件描述符集合

    //填写活跃连接
    void fillActiveChannels(int numEvents, ChannelList* channel)const;

    //更新channel通道
    void update(int operaion, Channel* channel);
};

#endif // !__EPOLL_POLLER_H__