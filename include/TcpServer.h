#ifndef __TCP_SERVER_H__
#define __TCP_SERVER_H__

/**
 * 用户使用muduo编写服务器程序
 **/

#include <functional>
#include <string>
#include <memory>
#include <atomic>
#include <unordered_map>

#include "EventLoop.h"
#include "Acceptor.h"
#include "INetAddress.h"
#include "noncopyable.h"
#include "EventLoopThreadPool.h"
#include "Callbacks.h"
#include "TcpConnection.h"
#include "Buffer.h"

// 对外的服务器编程使用的类
class TcpServer
{
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;
    enum Option
    {
        NoReusePort, //不允许重用本地端口
        ReusePort,  //允许重用本地端口
    };
    TcpServer(EventLoop* loop, const InetAddress& listenAddr,
        const std::string& nameArg, Option option = NoReusePort);
    ~TcpServer();

    void setThreadInitCallback(const ThreadInitCallback& cb) { threadInitCallback_ = cb; }
    void setConnectionCallback(const ConnectionCallback& cb) { connectionCallback_ = cb; }
    void setMessageCallback(const MessageCallback& cb) { messageCallback_ = cb; }
    void setWriteCompleteCallback(const WriteCompleteCallback& cb) { writeCompleteCallback_ = cb; }

    //设置底层subloop的个数
    void setThreadNum(int numThreads);

    void start();
private:
    void newConnection(int sockfd, const InetAddress& peerAddr);
    void removeConnection(const TcpConnectionPtr& conn);
    void removeConnectionInLoop(const TcpConnectionPtr& conn);
private:
    using ConnectionMap = std::unordered_map<std::string, TcpConnectionPtr>;
    EventLoop* loop_; //baseloop 用户自定义的loop

    const std::string ipPort_;//ip和端口号
    const std::string name_;//服务名称

    std::unique_ptr<Acceptor> acceptor_; //运行在mainLoop 任务是监听新连接事件

    std::shared_ptr<EventLoopThreadPool> threadPool_;//线程池

    ConnectionCallback connectionCallback_; //有新连接时的回调
    MessageCallback messageCallback_; //有读写事件完成时发生的回调
    WriteCompleteCallback writeCompleteCallback_;//消息发送完成后的回调

    ThreadInitCallback threadInitCallback_; //loop线程初始化后完成的回调
    int numThreads_; //线程池中线程的数量
    std::atomic_int started_;
    int nextConnId_;
    ConnectionMap connections_; //保存所有的连接
};

#endif // __TCP_SERVER_H__