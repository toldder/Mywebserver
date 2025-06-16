#ifndef __TCP_CONNECTION_H__
#define __TCP_CONNECTION_H__

#include <memory>
#include <string>
#include <atomic>

#include "noncopyable.h"
#include "INetAddress.h"
#include "Callbacks.h"
#include "Buffer.h"
#include "Timestamp.h"

class Channel;
class EventLoop;
class Socket;

/**
 * TcpServer => Acceptor => 有一个新用户连接，通过accept函数拿到connfd
 * => TcpConnection设置回调 => 设置到Channel => Poller => Channel回调
 **/


 //TcpConnection用于sub EventLoop中，对连接套接字fd及其相关方法进行封装(读消息事件、发送消息事件、连接关闭事件、错误事件)
class TcpConnection :noncopyable, public std::enable_shared_from_this<TcpConnection>
{
public:
    TcpConnection(EventLoop* loop, const std::string& nameArg, int sockfd,
        const InetAddress& localAdddr, const InetAddress& peerAddr);
    ~TcpConnection();

    EventLoop* getLoop()const { return loop_; }
    const std::string& name()const { return name_; }
    const InetAddress& localAdrres()const { return localAddr_; }
    const InetAddress& peerAddress()const { return peerAddr_; }

    bool connected()const { return state_ == Connected; }

    //发送数据
    void send(const std::string& buf);
    void sendFile(int fileDescriptor, off_t offset, size_t count);

    //关闭半连接
    void shutdown();

    //设置回调函数
    void setConnectionCallback(const ConnectionCallback& cb) { connectionCallback_ = cb; }
    void setMessageCallback(const MessageCallback& cb) { messageCallback_ = cb; }
    void setWriteCompleteCallback(const WriteCompleteCallback& cb) { writeCompleteCallback_ = cb; }
    void setCloseCallback(const CloseCallback& cb) { closeCallback_ = cb; }
    void setHighWaterMarkCallback(const HighWaterMarkCallback& cb, size_t highWaterMark)
    {
        highWaterCallback_ = cb;
        highWaterMark_ = highWaterMark;
    }

    // 连接建立
    void connectEstablished();
    // 连接销毁
    void connectDestroyed();
    void setContext(void* context) { context_ = context; }
    void* getContext() { return context_; }
private:
    enum StateE
    {
        Disconnected,//已经断开连接
        Connecting, //正在连接
        Connected,  //已连接
        Disconnecting //正在断开连接
    };
    void setState(StateE state) { state_ = state; }

    //处理可读事件，将客户端发送来的数据拷贝到用户缓冲区(inputBuffer_),再调用connectionCallback_保存的连接建立后的处理函数messageCallback_
    void handleRead(Timestamp receiveTime);
    //处理Tcp连接的可写事件
    void handleWrite();
    //处理连接关闭事件，将channel_从事件监听器中移除，然后调用connectionCallback_和closeCallback_保存的回调函数
    void handleClose();
    void handleError();

    void sendInLoop(const void* data, size_t len);
    void shutdownInLoop();
    void sendFileInLoop(int fileDescriptor, off_t offset, size_t count);


private:
    EventLoop* loop_; //该Tcp连接的Channel注册到了哪一个sub EventLoop上，这个loop就是哪一个sub EventLoop
    const std::string name_;
    std::atomic_int state_; //标识当前TCP的连接状态(Connected,Connecting,Disconnecting,Disconnected)
    bool reading_; //连接是否在监听读事件

    //数据缓冲区
    Buffer inputBuffer_; //接收数据缓冲区
    //outputBuffer用于暂存哪些暂时发送不出去的待发送数据。因为Tcp发送缓冲区是有大小限制的
    //加入达到了高水位线，就没办法把发送的数据通过send()直接拷贝到Tcp发送缓冲区，而是暂存在这个outputBuffer中。
    //等到Tcp发送缓冲区有空间了，触发可写事件了，再把outputBuffer_中的数据拷贝到Tcp发送缓冲区中
    Buffer outputBuffer_; //发送数据缓冲区
    //回调函数
    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    HighWaterMarkCallback highWaterCallback_;
    CloseCallback closeCallback_;

    size_t highWaterMark_; //高水位阈值

    //socket Channel
    std::unique_ptr<Socket> socket_;
    //Channel是TcpConnection的成员，说明在析构的时候channel会先被销毁，
    //但是如果销毁的时候channel正在进行写事件，会产生意向不到的事情,因此使用了tie进行绑定
    std::unique_ptr<Channel> channel_;
    const InetAddress localAddr_;
    const InetAddress peerAddr_;
    void* context_; //http上下文
};

#endif // !__TCP_CONNECTION_H__