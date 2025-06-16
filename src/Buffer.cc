#include<errno.h>
#include<sys/uio.h>
#include<unistd.h>

#include "Buffer.h"

ssize_t Buffer::readFd(int fd, int* saveErrno)
{
    char extrabuf[65536] = { 0 };
    struct iovec vec[2]; //分散内存,iovec结构体中包含两个变量，第一个指向内存的起始地址，第二个指定了内存长度的大小
    const size_t writable = writableBytes();

    //第一块缓冲区，指向可写空间,后面读入文件的内容放入写缓冲区中
    vec[0].iov_base = begin() + writeIndex_;
    vec[0].iov_len = writable;

    //第二块缓冲区，指向栈空间
    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof(extrabuf);

    const int iovcnt = (writable < sizeof(extrabuf)) ? 2 : 1;
    const ssize_t n = readv(fd, vec, iovcnt); //将文件描述符中对应内容读取到iovec指定内存，iocnt指定iovec的数量

    if (n < 0)
    {
        *saveErrno = errno;
    }
    else if (n <= writable)
    {
        writeIndex_ += n;
    }
    else
    {
        writeIndex_ = buffer_.size();
        append(extrabuf, n - writable); //对buf进行扩容，将extrabuf里面存储的另一部分数据也写入缓冲区
    }
    return n;
}

ssize_t Buffer::writeFd(int fd, int* saveErrno)
{
    ssize_t n = ::write(fd, peek(), readableBytes());
    if (n < 0)
    {
        *saveErrno = errno;
    }
    return n;
}

const char* Buffer::findCRLF()
{
    for (int i = readerIndex_;i < writeIndex_;++i)
    {
        if (buffer_[i] == '\r')
        {
            if (i < writeIndex_ - 1 && buffer_[i + 1] == '\n') return &buffer_[i];
        }
    }
    return nullptr;
}