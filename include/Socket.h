#ifndef __SOCKET_H__
#define __SOCKET_H__

#include "noncopyable.h"

class InetAddress;

class Socket
{
public:
    explicit Socket(int sockfd) :sockfd_(sockfd) {}
    ~Socket();

    int fd()const { return sockfd_; }
    void bindAddress(const InetAddress& localaddr); //绑定服务器的IP和端口号
    void listen();//调用底层listen
    int accept(InetAddress* peeraddr);  //调用accept接受客户端请求，这里封装的是accept4

    //调用shutdown来关闭写通道
    void shutdownWrite();

    //调用sockopt设置socket选项
    void setTcpNoDelay(bool on);
    void setReuseAddr(bool on);
    void setReusePort(bool on);
    void setKeepAlive(bool on);
private:
    const int sockfd_;
};

#endif // !__SOCKET_H__
