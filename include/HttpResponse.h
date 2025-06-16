#ifndef __HTTP_RESPONSE_H__
#define __HTTP_RESPONSE_H__

#include<string>
#include<map>

class Buffer;

class HttpResponse
{
public:
    enum HttpStatusCode
    {
        Unknown,
        OK = 200,
        MOVEDPermanently = 301,
        BADREQUEST = 400,
        NOTFOUND = 404,
    };

    explicit HttpResponse(bool close) : statusCode_(Unknown), closeConnection_(close) {}

    void setStatusCode(HttpStatusCode code) { statusCode_ = code; }

    void setStatusMessage(const std::string& message) { statusMessage_ = message; }

    void setCloseConnection(bool on) { closeConnection_ = on; }
    bool closeConnection() const { return closeConnection_; }

    void setContentType(const std::string& contentType) { addHeader("Content-Type", contentType); }

    void addHeader(const std::string& key, const std::string& value) { headers_[key] = value; }

    void setBody(const std::string& body) { body_ = body; }

    void appendToBuffer(Buffer* output) const;
private:
    std::map<std::string, std::string> headers_; //相应头部，键值对
    HttpStatusCode statusCode_; //状态码
    std::string statusMessage_; //文字描述
    bool closeConnection_; //是否关闭连接
    std::string body_; //响应体
};

#endif // !__HTTP_RESPONSE_H__
