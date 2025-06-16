#include "HttpResponse.h"
#include "Buffer.h"
#include<cstring>
void HttpResponse::appendToBuffer(Buffer* output) const
{
    char buf[32];
    // 响应行
    snprintf(buf, sizeof buf, "HTTP/1.1 %d ", statusCode_);
    output->append(buf,strlen(buf));
    output->append(statusMessage_.c_str(),strlen(statusMessage_.c_str()));
    output->append("\r\n",2);

    // 响应头部
    if (closeConnection_) {
        output->append("Connection: close\r\n",19);
    }
    else {
        snprintf(buf, sizeof buf, "Content-Length: %zd\r\n", body_.size());
        output->append(buf,strlen(buf));
        output->append("Connection: Keep-Alive\r\n",24);
    }

    for (const auto& header : headers_) {
        output->append(header.first.c_str(), strlen(header.first.c_str()) );
        output->append(": ",2);
        output->append(header.second.c_str(), strlen(header.second.c_str()));
        output->append("\r\n",2);
    }

    output->append("\r\n",2);  // 空行
    output->append(body_.c_str(),strlen(body_.c_str()));   // 响应体
}