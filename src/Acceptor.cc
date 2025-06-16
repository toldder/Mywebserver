#include<sys/types.h>
#include<sys/socket.h>
#include<unistd.h>
#include<errno.h>

#include "Acceptor.h"
#include "Logger.h"
#include "INetAddress.h"

static int createNonblocking()
{
    int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
    if (sockfd < 0)
    {
        LOG_FATAL << "listen socket create err" << errno;
    }
    return sockfd;
}


Acceptor::Acceptor(EventLoop* loop, const InetAddress& listenAddr, bool reuseport)
    :loop_(loop),acceptSocket_(createNonblocking()),acceptChannel_(loop_,acceptSocket_.fd()),islistenning_(false)
{
    acceptSocket_.setReuseAddr(true);
    acceptSocket_.setReusePort(true);
    acceptSocket_.bindAddress(listenAddr);
    acceptChannel_.setReadCallback(std::bind(&Acceptor::handleRead, this));
}

Acceptor::~Acceptor()
{
    acceptChannel_.disableAll();
    acceptChannel_.remove();
}

void Acceptor::listen()
{
    islistenning_=true;
    acceptSocket_.listen();
    acceptChannel_.enableReading();
}

//有新用户连接了
void Acceptor::handleRead()
{
    InetAddress peerAddr;
    int connfd = acceptSocket_.accept(&peerAddr);
    if (connfd >= 0)
    {
        if (newConnectionCallback_)
        {
            newConnectionCallback_(connfd, peerAddr);
        }
        else
        {
            ::close(connfd);
        }
    }
    else
    {
        LOG_ERROR << "accept err";
        if (errno == EMFILE)
        {
            LOG_ERROR << "sockfd reached limit";
        }
    }
}
