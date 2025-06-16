#ifndef __BUFFER_H__
#define __BUFFER_H__

#include<vector>
#include<stddef.h>
#include<string>
#include<algorithm>

//封装了一个用户缓冲区，以及向这个缓冲区写数据等一系列控制方法
class Buffer
{
public:
    static const size_t CheapPrepend = 8;//初始预留的prepend的空间大小
    static const size_t InitialSize = 1024;
    explicit Buffer(size_t initialSize = InitialSize)
        :buffer_(initialSize + CheapPrepend), readerIndex_(CheapPrepend), writeIndex_(CheapPrepend)
    {

    }

    size_t readableBytes()const { return writeIndex_ - readerIndex_; }
    size_t writableBytes()const { return buffer_.size() - writeIndex_; }
    size_t prependableBytes()const { return readerIndex_; }

    //返回缓冲区中可读数据的起始位置
    const char* peek()const { return begin() + readerIndex_; }

    //读取指定长度数据
    void retrieve(size_t len)
    {
        if (len < readableBytes())
        {
            readerIndex_ += len;
        }
        else
        {
            retrieveAll();
        }
    }

    void retrieveAll()
    {
        readerIndex_ = CheapPrepend;
        writeIndex_ = CheapPrepend;
    }

    //读取的时候将缓冲区数据转成string类型的数据返回
    std::string retrieveAllAsString()
    {
        return retrieveAsString(readableBytes());
    }

    std::string retrieveAsString(size_t len)
    {
        std::string result(peek(), len);
        retrieve(len);
        return result;
    }

    void ensureWritableBaytes(size_t len)
    {
        if (writableBytes() < len)
        {
            makeSpace(len);
        }
    }

    void append(const char* data, size_t len)
    {
        ensureWritableBaytes(len);
        std::copy(data, data + len, beginWrite());
        writeIndex_ += len;
    }

    char* beginWrite() { return begin() + writeIndex_; }
    const char* beginWrite()const { return begin() + writeIndex_; }

    const char* findCRLF();

    ssize_t readFd(int fd, int* saveErrno);
    ssize_t writeFd(int fd, int* saveErrno);
private:
    char* begin() { return &*buffer_.begin(); }
    const char* begin()const { return &*buffer_.begin(); }
    //分配出更多的空间
    void makeSpace(size_t len)
    {
        if (writableBytes() + prependableBytes() < len + CheapPrepend)
        {
            buffer_.resize(writeIndex_ + len);
        }
        else
        {
            size_t readable = readableBytes();
            std::copy(begin() + readerIndex_, begin() + writeIndex_, begin() + CheapPrepend);
            readerIndex_ = CheapPrepend;
            writeIndex_ = readerIndex_ + readable;
        }
    }
private:
    std::vector<char> buffer_;
    size_t readerIndex_;
    size_t writeIndex_;
};

#endif // !__BUFFER_H__
