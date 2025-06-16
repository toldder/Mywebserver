#ifndef __HTTP_REQUEST_H__
#define __HTTP_REQUEST_H__

#include<string>
#include<map>

#include "Timestamp.h"

class HttpRequest 
{
public:
    enum Method
    {
        GET,
        POST,
        PUT,
        DELETE,
        HEAD,
        INVALID
    };

    enum Version
    {
        UNKNOWN,
        HTTP10,
        HTTP11,
        HTTP20,
        HTTP30
    };

    HttpRequest() :method_(INVALID), version_(UNKNOWN) {}

    void setVersion(Version v) { version_ = v; }
    Version getVersion() const { return version_; }

    bool setMethod(std::string& m);
    Method getMethod()const { return method_; }

    void setPath(const char* start, const char* end) { path_.assign(start, end); }
    const std::string& getPath()const { return path_; }

    void setQuery(const char* start, const char* end) { query_.assign(start, end); }
    const std::string& getQuery()const { return query_; }

    void setReceiveTime(Timestamp t) { receiveTime_ = t; }
    Timestamp getReceiveTime()const { return receiveTime_; }

    void addHeader(const char* start, const char* colon, const char* end);
    std::string getHeader(const std::string& field)const;

    const std::map<std::string, std::string>& getHeaders()const { return headers_; }

    std::string getBody()const { return body_; }
    void setBody(std::string& body) { body_ = body; }

    void swap(HttpRequest& other); //交换
private:
    Method method_; //请求方法
    Version version_; //http版本号
    std::string path_; //url,请求资源路径
    std::string query_; //请求体,在发送中的url中的传递的参数
    Timestamp receiveTime_; //接收到的时间,请求事件
    std::map<std::string, std::string> headers_; //请求头部
    std::string body_; //请求体
};


#endif // !__HTTP_REQUEST_H__
