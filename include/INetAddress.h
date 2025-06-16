#ifndef __I_NET_ADDRESS_H__
#define __I_NET_ADDRESS_H__

#include<netinet/in.h>
#include<string>

//InetAddress类的作用主要就是封装sockarr_in,提供一个TCP连接专用socket
//对外接口主要有根据端口、ip创建socket,返回IP，端口号等信息。


class InetAddress
{
public:
    explicit InetAddress(uint16_t port = 0, std::string ip = "127.0.0.1");
    explicit InetAddress(const sockaddr_in& addr) :addr_(addr)
    {

    }

    std::string toIp()const;
    std::string toIpPort()const;
    uint16_t toPort()const;

    const sockaddr_in* getSockAddr()const { return &addr_; }
    void setSockAddr(const sockaddr_in& addr) { addr_ = addr; }
private:
    sockaddr_in addr_;
};

#endif // !__I_NET_ADDRESS_H__