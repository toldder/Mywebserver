#include<sys/epoll.h>

#include"Channel.h"
#include "EventLoop.h"
#include "Logger.h"


const int Channel::NoneEvent = 0; //空事件
const int Channel::ReadEvent = EPOLLIN | EPOLLPRI; //读事件，连接建立或存在外带数据
const int Channel::WriteEvent = EPOLLOUT;//写事件，有数据要写

Channel::Channel(EventLoop* loop, int fd)
    :fd_(fd), loop_(loop), events_(0), revents_(0), index_(-1), tied_(false)
{
    
}

Channel::~Channel() {}

void Channel::tie(const std::shared_ptr<void>& obj)
{
    tie_ = obj;//需要使其能够转为为TcpConnection 的指针
    tied_ = true;
}

//更新管道
void Channel::update()
{
    loop_->updateChannel(this);
}

void Channel::remove()
{
    loop_->removeChannel(this);
}

//连接关闭的时候会调用这个函数
void Channel::handleEvent(Timestamp receiveTime)
{
    if (tied_)
    {
        std::shared_ptr<void> guard = tie_.lock(); //转换为shared_ptr
        if (guard)
        {
            handleEventWithGuard(receiveTime);
        }
        //如果提升失败，则Channel的TcpConnection对象已经不存在了
    }else
    {
        handleEventWithGuard(receiveTime);
    }
}

//调用文件描述符实际发生事件的回调函数进行处理
//receiveTime作为读事件的回调函数的参数
void Channel::handleEventWithGuard(Timestamp receiveTime)
{
    LOG_INFO << "channel handleEvent revents:" << revents_;
    //关闭
    //当TcpConnection 对应channel 通过shutdown关闭写段，epoll触发epollhup
    if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN))
    {
        if (closeCallback_)
        {
            //通过shutdown关闭写端口
            closeCallback_();
        }
    }
    //错误
    if (revents_ & EPOLLERR)
    {
        if (errorCallback_)
        {
            errorCallback_();
        }
    }
    //读
    if (revents_ & (EPOLLIN | EPOLLPRI))
    {
        if (readCallback_)
        {
            readCallback_(receiveTime); //读事件的回调函数需要传入参数
        }
    }
    //写
    if (revents_ & EPOLLOUT)
    {
        if (writeCallback_)
        {
            writeCallback_();
        }
    }
}