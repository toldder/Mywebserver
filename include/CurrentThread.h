#ifndef __CURRENT_THREAD_H__
#define __CURRENT_THREAD_H__
#include<unistd.h>
#include<sys/syscall.h>

namespace CurrentThread
{
	extern thread_local int t_cachedTid;//保存tid缓存，因为系统调用非常耗时，拿到tid后将其保存
	void cacheTid();
	inline int tid()
	{
		if (__builtin_expect(t_cachedTid == 0, 0)) 
		{
			// __builtin_expect 是一种底层优化 此语句意思是如果还未获取tid 进入if 通过cacheTid()系统调用获取tid
			cacheTid();
		}
		return t_cachedTid;
	}
}

#endif // !__CURRENT_THREAD_H__
