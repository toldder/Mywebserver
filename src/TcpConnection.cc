#include <functional>
#include <string>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/tcp.h>
#include <sys/sendfile.h>
#include <fcntl.h> // for open
#include <unistd.h> // for close

#include <TcpConnection.h>
#include <Logger.h>
#include <Socket.h>
#include <Channel.h>
#include <EventLoop.h>


static EventLoop* CheckLoopNotNull(EventLoop* loop)
{
    if (loop == nullptr)
    {
        LOG_FATAL << "mainLoop is null";
    }
    return loop;
}

TcpConnection::TcpConnection(EventLoop* loop, const std::string& nameArg, int sockfd,
    const InetAddress& localAddr, const InetAddress& peerAddr)
    :loop_(CheckLoopNotNull(loop)), name_(nameArg), state_(Connecting),
    reading_(true), socket_(new Socket(sockfd)), channel_(new Channel(loop_, sockfd)),
    localAddr_(localAddr), peerAddr_(peerAddr), highWaterMark_(64 * 1024 * 1024) //64M
{
    //设置channel相应的回调函数
    channel_->setReadCallback(std::bind(&TcpConnection::handleRead, this, std::placeholders::_1));
    channel_->setWriteCallback(std::bind(&TcpConnection::handleWrite, this));
    channel_->setCloseCallback(std::bind(&TcpConnection::handleClose, this));
    channel_->setErrorCallback(std::bind(&TcpConnection::handleError, this));
    LOG_INFO << "Tcp connection:ctor:[" << name_.c_str() << "] at fd=" << sockfd;
    socket_->setKeepAlive(true);
}

TcpConnection::~TcpConnection()
{
    LOG_INFO << "Tcp connection::dtor[" << name_.c_str() << "]at fd=" << channel_->fd() << "state=" << (int)state_;
}

void TcpConnection::send(const std::string& buf)
{
    LOG_DEBUG << "send  "<<state_;
    if (state_ == Connected)
    {
        if (loop_->isInLoopThread())
        {
            LOG_DEBUG << "isInLoopThread";
            sendInLoop(buf.c_str(), buf.size());
        }
        else
        {
            LOG_DEBUG << "run In Loop";
            loop_->runInLoop(std::bind(&TcpConnection::sendInLoop, this, buf.c_str(), buf.size()));
        }
    }
}

//发送数据，应用写的快，内核发送慢，需将待发送数据写入缓冲区，而且设置了水位回调
void TcpConnection::sendInLoop(const void* data, size_t len)
{
    LOG_INFO << "the msg is " << (char*)data;
    ssize_t nwrote = 0;
    size_t remaining = len;
    bool faultError = false;
    if (state_ == Disconnected) //连接已经断开，不能发送消息
    {
        LOG_ERROR << "disconnected, give up writing";
    }
    //chennel_第一次开始写数据并且缓冲区没有待发送数据
    if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0)
    {
        nwrote = ::write(channel_->fd(), data, len);
        if (nwrote >= 0)
        {
            remaining = len - nwrote;
            if (remaining == 0 && writeCompleteCallback_)
            {
                //数据发送完成，不用设置eplooout事件
                loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
            }
        }
        else
        {
            nwrote = 0;
            if (errno != EWOULDBLOCK) //EWOULDBLOCK表示非阻塞情况下没有数据后的正常返回
            {
                faultError = true;
            }
        }
    }
  
    //如果write没有把数据全部发送出去，剩余数据需要保存到缓冲区中
    if (!faultError && remaining)
    {
        LOG_DEBUG << "THE REAMINING ";
        size_t oldLen = outputBuffer_.readableBytes();
        if (oldLen + remaining >= highWaterMark_ && oldLen < highWaterMark_ && highWaterCallback_)
        {
            loop_->queueInLoop(std::bind(highWaterCallback_, shared_from_this(), oldLen + remaining));
        }
        outputBuffer_.append((char*)data + nwrote, remaining);
        if (!channel_->isWriting())
        {
            //如果没有在发送消息了
            channel_->enableWriting(); //注册写事件，发消息发送缓冲区中的数据
        }
    }
}

void TcpConnection::shutdown()
{
    LOG_DEBUG << "CONNECT SHUTDOWN";
    if (state_ == Connected)
    {
        //关闭连接
        setState(Disconnecting);
        loop_->runInLoop(std::bind(&TcpConnection::shutdown, this));
    }
}


void TcpConnection::shutdownInLoop()
{
    if (!channel_->isWriting())
    {
        //如果数据发送完成
        socket_->shutdownWrite();
    }
}


// 连接建立
void TcpConnection::connectEstablished()
{
    setState(Connected);
    channel_->tie(shared_from_this());
    channel_->enableReading(); // 向poller注册channel的EPOLLIN读事件

    // 新连接建立 执行回调
    connectionCallback_(shared_from_this());
}

// 连接销毁
void TcpConnection::connectDestroyed()
{
    LOG_DEBUG << "CONNECT DESTROYED";
    if (state_ == Connected)
    {
        setState(Disconnected);
        channel_->disableAll(); // 把channel的所有感兴趣的事件从poller中删除掉
        connectionCallback_(shared_from_this()); //需要将this指针传递出去并且被智能指针安全地管理需要使用shared_from_this
    }
    channel_->remove(); // 把channel从poller中删除掉
}

// 读是相对服务器而言的 当对端客户端有数据到达 服务器端检测到EPOLLIN 就会触发该fd上的回调 handleRead取读走对端发来的数据
void TcpConnection::handleRead(Timestamp receiveTime)
{
    int savedErrno = 0;
    ssize_t n = inputBuffer_.readFd(channel_->fd(), &savedErrno);
    if (n > 0)//有数据到达
    {
        //有可读事件发生，调用用户传入的回调操作onMessage
        messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
    }
    else if(n==0) //客户端断开
    {
        handleClose();
    }else //产生错误
    {
        errno = savedErrno;
        LOG_ERROR << "TcpConnection::handleRead";
        handleError();
    }
}

void TcpConnection::handleWrite()
{
    //如果正在发送消息
    if (channel_->isWriting())
    {
        int savedErrno = 0;
        ssize_t n = outputBuffer_.writeFd(channel_->fd(), &savedErrno); //先写入缓冲区
        if (n > 0)
        {
            outputBuffer_.retrieve(n); //从缓冲区读取readble区域数据移动readindex
            if (outputBuffer_.readableBytes() == 0)
            {
                channel_->disableWriting();
                if (writeCompleteCallback_)
                {
                    //TcpConnection对象在其所在的subLoop中 向pendingFunctors_中加入回调
                    loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
                }
                if (state_ == Disconnecting)
                {
                    shutdownInLoop(); //在当前所属的loop中把TcpConnection删除掉
                }
            }
        }
        else {
            LOG_ERROR << "TcpConnection::handleWrite";
        }
    }else
    {
        LOG_ERROR << "TcpConnection fd=" << channel_->fd() << "is down,no more writing";
    }
}


void TcpConnection::handleClose()
{
    LOG_INFO << "TcpConnection::handleClose fd=" << channel_->fd() << "state=" << (int)state_;
    setState(Disconnected);
    channel_->disableAll();

    TcpConnectionPtr connPtr(shared_from_this());
    connectionCallback_(connPtr); // 连接回调
    closeCallback_(connPtr);      // 执行关闭连接的回调 执行的是TcpServer::removeConnection回调方法   // must be the last line
}

void TcpConnection::handleError()
{
    int optval;
    socklen_t optlen = sizeof optval;
    int err = 0;
    if (::getsockopt(channel_->fd(), SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0)
    {
        err = errno;
    }
    else
    {
        err = optval;
    }
    LOG_ERROR << "TcpConnection::handleError name:" << name_.c_str() << "- SO_ERROR:%" << err;
}

// 新增的零拷贝发送函数
void TcpConnection::sendFile(int fileDescriptor, off_t offset, size_t count) {
    if (connected()) {
        if (loop_->isInLoopThread()) { // 判断当前线程是否是loop循环的线程
            sendFileInLoop(fileDescriptor, offset, count);
        }
        else { // 如果不是，则唤醒运行这个TcpConnection的线程执行Loop循环
            loop_->runInLoop(
                std::bind(&TcpConnection::sendFileInLoop, shared_from_this(), fileDescriptor, offset, count));
        }
    }
    else {
        LOG_ERROR << "TcpConnection::sendFile - not connected";
    }
}

// 在事件循环中执行sendfile
void TcpConnection::sendFileInLoop(int fileDescriptor, off_t offset, size_t count)
{
    ssize_t byteSent = 0;//发送了多少字节数
    size_t remaining = count; //还有多少数据要发送
    bool faultError = false; //错误的标志位

    if (state_ == Disconnecting) {
        //连接断开，不需要发送数据
        LOG_ERROR << "disconnected,give up writing";
        return;
    }
    if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0)
    {
        byteSent = sendfile(socket_->fd(), fileDescriptor, &offset, remaining);
        if (byteSent >= 0)
        {
            remaining -= byteSent;
            if (remaining == 0 && writeCompleteCallback_) {
                //数据全部发送完成
                loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
            }
        }else //byteSent<0
        {
            if (errno != EWOULDBLOCK)
            {
                LOG_ERROR << "TcpConnection::senFileInLoop";
            }
            if (errno == EPIPE || errno == ECONNRESET)
            {
                faultError = true;
            }
        }
    }
    //处理剩余数据
    if (!faultError && remaining > 0)
    {
        loop_->queueInLoop(std::bind(&TcpConnection::sendFileInLoop, shared_from_this(), fileDescriptor, offset, remaining));
    }
}
