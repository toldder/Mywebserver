#ifndef __HTTP_CONTEXT_H__
#define __HTTP_CONTEXT_H__

#include "HttpRequest.h"
#include "Timestamp.h"

class Buffer;

class HttpContext
{
public:
    enum HttpRequestParseState
    {
        ExpectRequestLine,		// 请求行
        ExpectHeaders,			// 请求头
        ExpectBody,		    // 请求体
        GotAll,             //已经处理完成
    };
    // 构造函数，默认从请求行开始解析
    HttpContext() : state_(ExpectRequestLine) {}

    // return false if any error 
    bool parseRequest(Buffer* buf, Timestamp receiveTime); // 解析请求Buffer

    bool gotAll() const { return state_ == GotAll; }

    void reset()  // 为了复用HttpContext
    {
        state_ = ExpectRequestLine;
        HttpRequest dummy;
        request_.swap(dummy);
    }

    const HttpRequest& request() const { return request_; }
    HttpRequest& request() { return request_; }

private:
    bool processRequestLine(const char* begin, const char* end);
    bool processRequestBody();
    HttpRequestParseState state_; // 解析状态
    HttpRequest request_;
};


#endif // !__HTTP_CONTEXT_H__