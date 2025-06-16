#ifndef __MEMORY_POOL_H__
#define __MEMORY_POOL_H__
#include<mutex>
#include<cassert>
namespace memoryPool
{
#define MEMORY_POOL_NUM 64
#define SLOT_BASE_SIZE 8
#define MAX_SLOT_SIZE 512

	//具体内存池的槽大小没法确定，因为每个内存池的槽大小不同(8的倍数)
	//所以这个槽结构体的sizeof不是实际的槽大小
	struct Slot
	{
		Slot* next;
	};

	class MemoryPool {
	public:
		MemoryPool(size_t BlockSize = 4096);
		~MemoryPool();
		void init(size_t);

		void* allocate();
		void deallocate(void*);
	private:
		void allocateNewBlock();
		size_t padPointer(char* p, size_t align);
	private:
		int BlockSize_; //内存块大小
		int SlotSize_;  //槽大小
		Slot* firstBlock_;  //指向内存池的首个实际内存块
		Slot* curSlot_;		//指向当前未被使用过的槽
		Slot* freeList_;	//指向空闲的槽(被使用过后又被释放的槽)
		Slot* lastSlot_;	//作为当前内存块中最后能够存放元素的位置标识（超过该位置需要申请新的内存块）
		std::mutex mutexForFreeList_;	//保证freeList_在多线程中操作的原子性
		std::mutex mutexForBlock_;		//保证多线程情况下避免不必要的重复开辟内存导致的浪费行为
	};

	class HashBucket
	{
	public:
		static void initMemoryPool();
		static MemoryPool& getMemoryPool(int index);

		static void* useMemory(size_t size)
		{
			if (size <= 0) return nullptr;
			if (size > MAX_SLOT_SIZE) return operator new(size);

			// 相当于size / 8 向上取整（因为分配内存只能大不能小
			return getMemoryPool(((size + 7) / SLOT_BASE_SIZE) - 1).allocate();
		}

		static void freeMemory(void* ptr, size_t size)
		{
			if (!ptr) return;
			if (size > MAX_SLOT_SIZE)
			{
				operator delete(ptr);
				return;
			}
			getMemoryPool(((size + 7) / SLOT_BASE_SIZE) - 1).deallocate(ptr);
		}
		template<typename T,typename... Args>
		friend T* newElement(Args&... args);

		template<typename T>
		friend void deleteElement(T* p);
	};

	template<typename T, typename... Args>
	T* newElement(Args&... args)
	{
		T* p = nullptr;
		//如果能分配到一个元素的空间大小
		if ((p = reinterpret_cast<T*>(HashBucket::useMemory(sizeof(T)))) != nullptr)
		{
			//使用Placement new在指定内存位置构建对象
			//std::forward则实现完美转发，即不改变参数的左右值的形式（左值按照左值传递，右值按照右值传递）
			new(p) T(std::forward<Args>(args)...);
		}
		return p;
	}

	template<typename T>
	void deleteElement(T* p)
	{
		//对象析构
		if (p)
		{
			p->~T();
			//内存回收
			HashBucket::freeMemory(reinterpret_cast<void*>(p), sizeof(T));
		}
	}
} //namespace memoryPool

#endif //! __MEMORY_POOL_H__
