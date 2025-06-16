#ifndef __POLLER_H__
#define __POLLER_H__

#include <unordered_map>
#include <vector>
#include "Timestamp.h"

class Channel;
class EventLoop;

//多路事件分发器的核心IO复用模块，负责监听文件描述符事件
// 抽象类，拥有updateChannel,poll,removeChannel三个纯虚函数
class Poller
{
public:
    using ChannelList = std::vector<Channel*>;
    Poller(EventLoop* loop):ownerLoop_(loop){}
    virtual ~Poller() = default;

    //给IO复用保留统一接口
    //调用epoll_wait获取事件监听器上发生的事件的fd及其对应事件
    //根据fd找到对应的channel,更新channel上的revents_变量，并将这个channel装入activeChannels中。

    //调用poll后能够得到事件监听器的监听结果，即activeChannels
    virtual Timestamp poll(int timeoutMs, ChannelList* activeChannels) = 0; //相当于epoll_wait
    virtual void updateChannel(Channel* channel) = 0; 
    virtual void removeChannel(Channel* channel) = 0;

    //判断参数channel是否在当前的Poller中
    bool hasChannel(Channel* channel)const;

    //获取默认IO复用的实现方式
    static Poller* newDefaultPoller(EventLoop* loop);
protected:
    using ChannelMap = std::unordered_map<int, Channel*>;
    ChannelMap channels_; //保管注册在该Poller上的所有Channel
private:
    EventLoop* ownerLoop_; 
};

#endif // !__POLLER_H__