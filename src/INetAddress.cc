#include <string.h>
#include <arpa/inet.h>

#include "INetAddress.h"

InetAddress::InetAddress(uint16_t port, std::string ip)
{
    ::memset(&addr_, 0, sizeof(addr_));
    addr_.sin_family = AF_INET;
    addr_.sin_port = ::htons(port); // 本地字节序转为网络字节序
    addr_.sin_addr.s_addr = ::inet_addr(ip.c_str());
}

std::string InetAddress::toIp() const
{
    // addr_
    char buf[64] = { 0 };
    //将网络字节序的ip整数转换为点分十进制的主机序列，结果存储在buf中
    ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof(buf));
    return buf;
}

std::string InetAddress::toIpPort() const
{
    // ip:port
    char buf[64] = { 0 };
    sprintf(buf , "%s:%u", toIp().c_str(),toPort());
    return buf;
}

uint16_t InetAddress::toPort() const
{
    return ::ntohs(addr_.sin_port); //需要将网络字节序转换为主机序列
}

/*
//测试代码的正确性
#include <iostream>
int main()
{
    InetAddress addr(8080);
    std::cout << addr.toIpPort() << std::endl;
}
*/