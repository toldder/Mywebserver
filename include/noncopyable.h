#ifndef __NON_COPYABLE_H__
#define __NON_COPYABLE_H__

//noncopyable及其派生类对象本身无法进行拷贝和赋值，只有派生类对象能够进行构造和析构
class noncopyable {
public:
	noncopyable(const noncopyable&) = delete;
	noncopyable& operator=(const noncopyable&) = delete;
protected:
	noncopyable() = default;
	virtual ~noncopyable() = default;
};

#endif // !__NON_COPYABLE_H__