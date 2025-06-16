#include "memoryPool.h"

namespace memoryPool {
	MemoryPool::MemoryPool(size_t BlockSize):BlockSize_(BlockSize)
	{

	}

	MemoryPool::~MemoryPool()
	{
		//把连续的block删除
		Slot* cur = firstBlock_;
		while(cur)
		{
			Slot* next = cur->next;
			//转换为void类型，只释放空间不需要调用析构函数
			operator delete(reinterpret_cast<void*>(cur));
			cur = next;
		}
	}

	void MemoryPool::init(size_t size)
	{
		assert(size > 0);
		SlotSize_ = size;
		firstBlock_ = nullptr;
		curSlot_ = nullptr;
		freeList_ = nullptr;
		lastSlot_ = nullptr;
	}

	void* MemoryPool::allocate()
	{
		//优先使用空闲链表中的内存槽
		if (freeList_ != nullptr)
		{
			{
				std::lock_guard<std::mutex> lock(mutexForFreeList_);
				if (freeList_ != nullptr)
				{
					Slot* temp = freeList_;
					freeList_ = freeList_->next;
					return temp;
				}
			}
		}

		Slot* temp = nullptr;
		{
			std::lock_guard<std::mutex> lock(mutexForBlock_);
			if (curSlot_ >= lastSlot_)
			{
				//当前内存槽已无内存可用
				allocateNewBlock();
			}
			temp = curSlot_;
			// 这里不能直接 curSlot_ += SlotSize_ 因为curSlot_是Slot*类型，所以需要除以SlotSize_再加1
			curSlot_ += SlotSize_ / sizeof(Slot);
		}
		return temp;
	}

	void MemoryPool::deallocate(void* ptr)
	{
		if (ptr)
		{
			//回收内存，将内存通过头插法插入到空闲链表中
			std::lock_guard<std::mutex> lock(mutexForFreeList_);
			reinterpret_cast<Slot*>(ptr)->next = freeList_;
			freeList_ = reinterpret_cast<Slot*>(ptr);
		}
	}

	void MemoryPool::allocateNewBlock()
	{
		//头插法插入新的内存块
		void* newBlock = operator new(BlockSize_);
		reinterpret_cast<Slot*>(newBlock)->next = firstBlock_;
		firstBlock_ = reinterpret_cast<Slot*> (newBlock);

		char* body = reinterpret_cast<char*>(newBlock) + sizeof(Slot*);
		size_t paddingSize = padPointer(body, SlotSize_);
		curSlot_ = reinterpret_cast<Slot*>(body + paddingSize);

		//应该是使用了左闭右开区间
		//超过该标志位置，则说明该内存块已无内存槽可用，需向系统申请新的内存块
		lastSlot_ = reinterpret_cast<Slot*>(reinterpret_cast<size_t>(newBlock) + BlockSize_ - SlotSize_ + 1);
		freeList_ = nullptr;
	}

	//让指针对齐到槽大小的位置
	size_t MemoryPool::padPointer(char* p, size_t align)
	{
		return (align - reinterpret_cast<size_t>(p)) % align;
	}

	void HashBucket::initMemoryPool()
	{
		for (int i = 0; i < MEMORY_POOL_NUM; i++)
		{
			getMemoryPool(i).init((i + 1) * SLOT_BASE_SIZE);
		}
	}

	//单例模式
	MemoryPool& HashBucket::getMemoryPool(int index)
	{
		static MemoryPool memoryPool[MEMORY_POOL_NUM];
		return memoryPool[index];
	}
}