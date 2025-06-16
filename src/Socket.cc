#include<sys/socket.h>
#include<sys/types.h>
#include<unistd.h>
#include<netinet/tcp.h>

#include "../include/Socket.h"
#include "../include/InetAddress.h"
#include "Logger.h"

Socket::~Socket()
{
    ::close(sockfd_);
}

void Socket::bindAddress(const InetAddress& localaddr)
{
    struct sockaddr_in addr;
    if (0 != ::bind(sockfd_, (sockaddr*)(localaddr.getSockAddr()), sizeof(sockaddr_in)))
    {
        LOG_FATAL << "bind socketfd:" << sockfd_ << "failed";
    }
    return;
}

void Socket::listen()
{
    if (0 != ::listen(sockfd_, 1024))
    {
        LOG_FATAL << "listen sockfd:" << sockfd_ << " failed";
    }
}

int Socket::accept(InetAddress* peeraddr)
{
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    memset(&addr, 0, len);
    int confd=::accept4(sockfd_, (sockaddr*)&addr,&len, SOCK_NONBLOCK | SOCK_CLOEXEC);
    if (confd >= 0)
    {
        peeraddr->setSockAddr(addr);
    }
    return confd;
}

void Socket::shutdownWrite()
{
    if (::shutdown(sockfd_, SHUT_WR)<0)
    {
        LOG_ERROR << "shutdown write error";
    }
}

//是否禁用Naggle算法,即首先要先进行等待一段时间的数据
void Socket::setTcpNoDelay(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval));
}

//重用地址
void Socket::setReuseAddr(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
}

//重用端口
void Socket::setReusePort(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
}

//保持长连接，发送周期性活报文以维持连接
void Socket::setKeepAlive(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval));
}