#include<string>
#include <iostream>

#include "HttpContext.h"
#include "Buffer.h"
#include "Logger.h"

bool HttpContext::parseRequest(Buffer* buf, Timestamp receiveTime)
{
    bool ok = true;
    bool hasMore = true;
    while (hasMore)
    {
        if (state_ == ExpectRequestLine)  // 解析请求行
        { // 查找Buffer中第一次出现“\r\n”的位置
            const char* crlf = buf->findCRLF();
            if (crlf)
            { // 若找到“\r\n”，说明至少有一行数据，进行实际解析
              // buf->peek()为数据开始部分，crlf为结束部分
                ok = processRequestLine(buf->peek(), crlf);
                if (ok) { // 解析成功
                    request_.setReceiveTime(receiveTime);
                    size_t len = crlf - buf->peek() + 2;  // 后移2个字符
                    buf->retrieve(len);
                    state_ = ExpectHeaders;
                }
                else { hasMore = false; }
            }
            else { hasMore = false; }
        }
        else if (state_ == ExpectHeaders)  // 解析请求头
        {
            const char* crlf = buf->findCRLF(); // 找到“\r\n”位置
            if (crlf)
            {
                const char* colon = std::find(buf->peek(), crlf, ':'); // 定位分隔符
                if (colon != crlf) {
                    request_.addHeader(buf->peek(), colon, crlf); // 键值对解析
                }
                else {
                    // empty line, end of header,接下来解析body部分
                    state_ = ExpectBody;
                }
                size_t len = crlf - buf->peek() + 2;  // 后移2个字符
                buf->retrieve (len);
            }
            else { hasMore = false; }
        }
        else if (state_ == ExpectBody)  // 解析请求体，未实现
        {
            //将内容读取出来
            std::string content = buf->retrieveAllAsString();            
            request_.setBody(content);
            hasMore = false;
            state_ = GotAll;
        }
    }
    return ok;
}

//请求行格式:请求方法  URL  版本号

bool HttpContext::processRequestLine(const char* begin, const char* end)
{
    bool succeed = false;
    const char* start = begin;
    const char* space = std::find(start, end, ' ');
    // 第一个空格前的字符串，请求方法
    std::string s(start, space - start);
    LOG_INFO << "request_method:" + s;
    if (space != end && request_.setMethod(s))
    {
        start = space + 1;
        space = std::find(start, end, ' ');
        if (space != end)
        {  // 第二个空格前的字符串，URL
            const char* question = std::find(start, space, '?');
            if (question != space) {  // 如果有"?"，分割成path和请求参数
                request_.setPath(start, question);
                request_.setQuery(question, space);
            }
            else {
                request_.setPath(start, space); // 仅path
            }
            start = space + 1;
            // 最后一部分，解析HTTP协议
            succeed = end - start == 8 && std::equal(start, end - 1, "HTTP/1.");
            if (succeed)
            {
                if (*(end - 1) == '1') {  // 1.1版本
                    request_.setVersion(HttpRequest::HTTP11);
                }
                else if (*(end - 1) == '0') {  // 1.0版本
                    request_.setVersion(HttpRequest::HTTP10);
                }
                else {
                    succeed = false;
                }
            }
        }
    }
    return succeed;
}
