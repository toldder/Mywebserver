#include <string>
#include <sys/stat.h>
#include <sstream>

#include "TcpServer.h"
#include "Logger.h"
#include "AsyncLogging.h"
#include "memoryPool.h"

static const off_t RollSize = 1 * 1024 * 1024; //日志滚动大小为1MB

class EchoServer
{
public:
    EchoServer(EventLoop* loop, const InetAddress& addr, const std::string& name)
        :loop_(loop), server_(loop, addr, name)
    {
        //注册回调函数
        server_.setConnectionCallback(std::bind(&EchoServer::onConnection, this, std::placeholders::_1));
        server_.setMessageCallback(std::bind(&EchoServer::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        //设置合适的subLoop线程个数
        server_.setThreadNum(3);
    }
    void  start()
    {
        server_.start();
    }

private:
    //连接建立/断开的函数
    void onConnection(const TcpConnectionPtr& conn)
    {
        if (conn->connected())
        {
            LOG_INFO << "connection up " << conn->peerAddress().toIpPort();
        }
        else
        {
            LOG_INFO << "connection closed " << conn->peerAddress().toIpPort();
        }
    }

    //可读事件回调
    void onMessage(const TcpConnectionPtr& conn, Buffer* buffer, Timestamp time)
    {
        std::string msg = buffer->retrieveAllAsString();//读取消息
        LOG_DEBUG << msg << "   the send msg content";
        LOG_DEBUG << conn.get();
        conn->send(msg); //发送消息
    }
private:
    EventLoop* loop_;
    TcpServer server_;
};

AsyncLogging* g_asyncLog = NULL;
AsyncLogging* getAsyncLog() {
    return g_asyncLog;
}


void asyncLog(const char* msg, int len)
{
    AsyncLogging* logging = getAsyncLog();
    if (logging)
    {
        logging->append(msg, len);
    }
}

int main(int argc, const char** argv) {
    //第一步启动日志
    const std::string LogDir = "logs";
    mkdir(LogDir.c_str(), 0755);
    std::ostringstream LogfilePath;
    LogfilePath << LogDir << "/" << ::basename(argv[0]);//完整的日志文件路径
    AsyncLogging log(LogfilePath.str(), RollSize);
    g_asyncLog = &log;
    Logger::setOutput(asyncLog);
    log.start();

    //第二步，启动内存池
    memoryPool::HashBucket::initMemoryPool();

    //第三步启动底层网络模块
    EventLoop loop;
    InetAddress addr(8080);
    EchoServer server(&loop, addr, "EchoServer");
    server.start();

    std::cout << "================================================Start Web Server================================================" << std::endl;
    loop.loop();
    std::cout << "================================================Stop Web Server=================================================" << std::endl;
    //结束日志打印
    log.stop();
    return 0;
}