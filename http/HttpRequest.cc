#include "HttpRequest.h"

bool HttpRequest::setMethod(std::string& m)
{
    if (m == "GET") method_ = GET;
    else if (m == "POST") method_ = POST;
    else if (m == "HEAD") method_ = HEAD;
    else if (m == "PUT") method_ = PUT;
    else if (m == "DELETE")  method_ = DELETE;
    else method_ = INVALID;
    return method_ != INVALID;
}

void HttpRequest::addHeader(const char* start, const char* colon, const char* end)
{
    std::string field(start, colon);  // 要求冒号前无空格
    ++colon;
    while (colon < end && isspace(*colon)) {  // 过滤冒号后的空格
        ++colon;
    }
    std::string value(colon, end);
    //去除最后的空格
    while (!value.empty() && isspace(value[value.size() - 1])) {
        value.resize(value.size() - 1);
    }
    headers_[field] = value;
}

std::string HttpRequest::getHeader(const std::string& field)const
{
    std::string result;
    std::map<const std::string, std::string>::const_iterator it = headers_.find(field);
    if (it != headers_.end()) result = it->second;
    return result;
}

void HttpRequest::swap(HttpRequest& other) //交换
{
    std::swap(method_, other.method_);
    std::swap(version_, other.version_);
    path_.swap(other.path_);
    receiveTime_.swap(other.receiveTime_);
    headers_.swap(other.headers_);
}
