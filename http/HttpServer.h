#ifndef __HTTP_SERVER_H__
#define __HTTP_SERVER_H__

#include "noncopyable.h"
#include "Logger.h"
#include "TcpServer.h"
#include "TcpConnection.h"
#include "HttpContext.h"
#include "HttpRequest.h"
#include "HttpResponse.h"


class HttpServer : noncopyable
{
public:
    typedef std::function<void(const HttpRequest&, HttpResponse*)> HttpCallback;

    HttpServer(EventLoop* loop,
        const InetAddress& listenAddr,
        const std::string& name,
        TcpServer::Option option = TcpServer::NoReusePort)
        :server_(loop, listenAddr, name, option)
    {
        server_.setConnectionCallback(std::bind(&HttpServer::onConnection, this, std::placeholders::_1));
        server_.setMessageCallback(std::bind(&HttpServer::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        //设置合适的subLoop线程个数
        server_.setThreadNum(3);
    }

    /// Not thread safe, callback be registered before calling start().
    // 设置http请求的回调函数
    void setHttpCallback(const HttpCallback& cb) { httpCallback_ = cb; }

    void setThreadNum(int numThreads) { server_.setThreadNum(numThreads); }

    void start()
    {
        server_.start();
    }
private:
    // TcpServer的新连接、新消息的回调函数
    void onConnection(const TcpConnectionPtr& conn);
    void onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp receiveTime);

    // 在onMessage中调用，并调用用户注册的httpCallback_函数，对请求进行具体的处理
    void onRequest(const TcpConnectionPtr&, const HttpRequest&);

    TcpServer server_;
    HttpCallback httpCallback_;
    HttpContext* context_;
};

void HttpServer::onConnection(const TcpConnectionPtr& conn)
{

    if (conn->connected())
    {
        context_ = new HttpContext();
        conn->setContext(context_);  // 绑定一个HttpContext到TcpConnection
        LOG_INFO << "connection up " << conn->peerAddress().toIpPort();
    }
    else
    {
        LOG_INFO << "connection closed " << conn->peerAddress().toIpPort();
    }

}

void HttpServer::onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp receiveTime)
{
    HttpContext* context = (HttpContext*)(conn->getContext());
    // 解析请求
    if (!context->parseRequest(buf, receiveTime)) {
        conn->send("HTTP/1.1 400 Bad Request\r\n\r\n");  // 解析失败， 400错误
        conn->shutdown();   // 断开本段写
    }
    // 请求解析成功
    if (context->gotAll()) {
        onRequest(conn, context->request());  // 调用onRequest()私有函数
        context->reset();					  // 复用HttpContext对象
    }
}

void HttpServer::onRequest(const TcpConnectionPtr& conn, const HttpRequest& req)
{
    // 长连接还是短连接
    const std::string& connection = req.getHeader("Connection");
    bool close = connection == "close" ||
        (req.getVersion() == HttpRequest::HTTP10 && connection != "Keep-Alive");

    HttpResponse response(close);
    httpCallback_(req, &response);  // 调用客户端的http处理函数，填充response
    Buffer buf;
    response.appendToBuffer(&buf); // 将响应格式化填充到buf中
    std::string msg = buf.retrieveAllAsString();//读取消息
    conn->send(msg);				 // 将响应回复发送给客户端
    if (response.closeConnection()) {
        conn->shutdown();  // 如果是短连接，直接关闭。
    }
}



#endif // !__HTTP_SERVER_H__

