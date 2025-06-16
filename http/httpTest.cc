#include <map>
#include <string>
#include <sys/stat.h>
#include<sys/time.h>
#include<sstream>

#include "EventLoop.h"
#include "TcpServer.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "HttpContext.h"
#include "HttpServer.h"
#include "Logger.h"
#include "memoryPool.h"
#include "AsyncLogging.h"
#include "cellular_automate.h"


static const off_t RollSize = 1 * 1024 * 1024; //日志滚动大小为1MB
cellAutomate* cell_automate;

MyCache::MArcCache<time_t, std::string> application_cache;
time_t receiveTime=0;

void parseInit(std::string& s)
{
    int ch[2] = { 4,4 }; //默认棋盘的大小为4*4
    for (int i = 0;i < s.size();++i)
    {
        if (s[i] == 'n') {
            while (i < s.size() && (s[i] < '0' || s[i]>'9')) ++i;
            if (i >= s.size()) break; //非法的数据
            else
            {
                int num = 0;
                while (i < s.size() && s[i] >= '0' && s[i] <= '9')
                {
                    num = num * 10 + s[i] - '0';
                    ++i;
                }
                ch[0] = num ? num : ch[0];
            }
        }
        else if (s[i] == 'm')
        {
            while (i < s.size() && (s[i] < '0' || s[i]>'9')) ++i;
            if (i >= s.size()) break; //非法的数据
            else
            {
                int num = 0;
                while (i < s.size() && s[i] >= '0' && s[i] <= '9')
                {
                    num = num * 10 + s[i] - '0';
                    ++i;
                }
                ch[1] = num ? num : ch[1];
            }
        }
    }
    LOG_INFO << "N=" << ch[0] << "M=" << ch[1];
    cell_automate = new cellAutomate(ch[0], ch[1]);
}

// 实际的请求处理
void onRequest(const HttpRequest& req, HttpResponse* resp)
{
    if (req.getMethod() == HttpRequest::Method::GET)
    {
        if (req.getPath() == "/")  // 根目录请求
        {
            resp->setStatusCode(HttpResponse::OK);
            resp->setStatusMessage("OK");
            resp->setContentType("text/html");
            resp->addHeader("Server", "Muduo");
            std::string now = Timestamp::now().toFormattedString();
            resp->setBody("<html><head><title>This is title</title></head>"
                "<body><h1>Hello</h1>Now is " + now +
                "</body></html>");
        }
        else if (req.getPath() == "/hello")  // /hello，返回文本 
        {
            resp->setStatusCode(HttpResponse::OK);
            resp->setStatusMessage("OK");
            resp->setContentType("text/plain");
            resp->addHeader("Server", "Muduo");
            resp->setBody("hello, world!\n");
        }
        else  // 不存在的URL，404 错误
        {
            resp->setStatusCode(HttpResponse::NOTFOUND);
            resp->setStatusMessage("Not Found");
            resp->setCloseConnection(true);
        }
    }
    else if (req.getMethod() == HttpRequest::Method::POST)
    {
        if (req.getPath() == "/init")  // 根目录请求
        {
            resp->setStatusCode(HttpResponse::OK);
            resp->setStatusMessage("OK");
            resp->setContentType("text/html");
            resp->addHeader("Server", "Muduo");
            std::string content = req.getBody();
            //处理文本数据
            parseInit(content);
            //得到当前时间
            Timestamp t = Timestamp::now();
            receiveTime = t.secondsSinceEpoch();
        }
        else if (req.getPath() == "/cal")
        {
            resp->setStatusCode(HttpResponse::OK);
            resp->setStatusMessage("OK");
            resp->setContentType("text/html");
            resp->addHeader("Server", "Muduo");
            std::string content = req.getBody();
            size_t it = content.find(":");
            while (it < content.size() && (content[it] < '0' || content[it]>'9')) ++it;
            int time = 0;
            while (it < content.size())
            {
                if (content[it] >= '0' && content[it] <= '9') time = time * 10 + content[it] - '0';
                else break;
                it++;
            }
            //处理文本数据
            std::string res;
            if (application_cache.get(receiveTime + time,res))
            {
                std::cout << "cache hit\n";
                std::string tmp = "from cache";
                res = tmp + res;
                resp->setBody(res);
            }
            else
            {
                std::string res = cell_automate->cellState(time);
                application_cache.put(receiveTime + time, res);
                resp->setBody(res);
            }
        }
    }
}

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

int main(int argc, char* argv[])
{
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

    //启动http服务
    EventLoop loop;
    InetAddress addr(8080);
    HttpServer server(&loop, addr, "dummy");

    server.setHttpCallback(onRequest);
    server.start();
    std::cout << "================================================Start Web Server================================================" << std::endl;
    loop.loop();
    std::cout << "================================================Stop Web Server=================================================" << std::endl;
    //结束日志打印
    log.stop();
    return 0;
}

