#ifndef __ACCEPTOR_H__
#define __ACCEPTOR_H__

#include "noncopyable.h"
#include "Socket.h"
#include "Channel.h"

class EventLoop;
class InetAddress;

//Acceptor主要用于接受新用户连接并分发连接给SubReactor，封装了服务监听套接字fd以及相关处理方法
//核心在与将监听套接字和Channel绑定在了一起，并为其注册了读回调handRead。
//读事件发生，会调用相应读回调handleRead，该回调会间接调用TcpServer的newConnection()，
//该函数负责以轮询的方式把连接分发给sub EventLoop处理
class Acceptor
{
public:
    using NewConnectionCallback = std::function<void(int sockdf, const InetAddress&)>;
    Acceptor(EventLoop* loop, const InetAddress& listenAddr, bool reuseport);
    ~Acceptor();

    //设置回调函数
    void setNewConnectionCallback(const NewConnectionCallback& cb) { newConnectionCallback_ = cb; }

    //判断是否在监听
    bool listenning()const { return islistenning_; }

    //监听本地端口,让mian EventLoop监听acceptSocket_;
    void listen();
private:
    void handleRead(); //处理新用户的连接请求,该函数注册到acceptChannel_上,同时调用成员变量newConnectionCallback_保存的函数

    EventLoop* loop_; //监听套接字的fd由哪个EventLoop负责循环监听，这个EventLoop就是main EventLoop
    Socket acceptSocket_; //用于接受新连接,服务器监听套接字，即socket套接字返回的监听套接字
    Channel acceptChannel_; //专门用于监听新连接的channel，把acceptSocket_及其感兴趣事件和事件对应的处理函数都封装进去
    NewConnectionCallback newConnectionCallback_; //新连接的回调函数,TcpServer::newConnection函数注册给了这个变量
    bool islistenning_; //是否在监听
};

#endif // !__ACCEPTOR_H__
