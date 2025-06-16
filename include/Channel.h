#ifndef __CHANNEL_H__
#define __CHANNEL_H__

#include <functional>
#include <memory>

#include "noncopyable.h"


class Timestamp;
class EventLoop;

//Channel将文件描述符和该文件描述符对应的回调函数绑定在了一起。
//封装文件描述符和fd感兴趣事件、实际发生事件。并提供注册、移除、保存回调函数的方法。
class Channel :noncopyable
{
public:
    // 分别表示Channel未添加到Poller,已经添加到Poller,从Poller中删除
    enum ChannelStatus { CHANNEL_NEW, CHANNEL_ADD, CHANNEL_DEL };
    using EventCallback = std::function<void()>; //文件描述符对应的回调函数
    using ReadEventCallback = std::function<void(Timestamp)>;
    Channel(EventLoop* loop, int fd);
    ~Channel();

    //处理事件
    void handleEvent(Timestamp receiveTime);

    //设置回调函数
    void setReadCallback(ReadEventCallback cb) { readCallback_ = std::move(cb); }
    void setWriteCallback(EventCallback cb) { writeCallback_ = std::move(cb); }
    void setCloseCallback(EventCallback cb) { closeCallback_ = std::move(cb); }
    void setErrorCallback(EventCallback cb) { errorCallback_ = std::move(cb); }

    //防止当channel被手动remove掉，channel还在执行回调操作
    void tie(const std::shared_ptr<void>&);

    int fd()const { return fd_; }
    int events()const { return events_; }
    void set_revents(int revt) { revents_ = revt; }
    ChannelStatus index() { return index_; }
    void set_index(ChannelStatus idx) { index_ = idx; }


    //设置事件状态,类似epoll_ctl函数
    void enableReading() { events_ |= ReadEvent; update(); }
    void disableReading() { events_ &= ~ReadEvent; update(); }
    void enableWriting() { events_ |= WriteEvent; update(); }
    void disableWriting() { events_ &= ~WriteEvent; update(); }
    void disableAll() { events_ = NoneEvent; update(); }

    //返回事件当前的状态
    bool isNoneEvent()const { return events_ == NoneEvent; }
    bool isWriting()const { return events_ & WriteEvent; }
    bool isReading()const { return events_ & ReadEvent; }

    //one loop per thread
    EventLoop* ownerLoop() { return loop_; }
    void remove();
private:
    void update();
    void handleEventWithGuard(Timestamp receiveTime);
private:
    const int fd_; //监听的文件描述符
    int events_; //感兴趣的事件
    int revents_;//注册fd实际发生事件
    EventLoop* loop_; //fd属于哪个EventLoop对象
    ChannelStatus index_; //为-1表示channel还没有添加到Poller,1表示已经添加，2表示已经删除

    //发生的事件类型,通常分为四类，读、写、连接关闭、发生错误
    ReadEventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback errorCallback_;

    static const int NoneEvent;
    static const int ReadEvent;
    static const int WriteEvent;

    std::weak_ptr<void> tie_;  
    bool tied_;
};

#endif // !__CHANNEL_H__
