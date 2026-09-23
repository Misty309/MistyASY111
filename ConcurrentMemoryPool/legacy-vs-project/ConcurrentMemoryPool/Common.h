#pragma once
#include <iostream>
#include <vector>
#include <unordered_map>
#include <map>
#include <algorithm>
#include <unordered_map>

#include <time.h>
#include <assert.h>


#include <thread>
#include <mutex>
#include <atomic>

#ifdef   _WIN32 
 #include <windows.h>
#else
 //....
#endif 


static const size_t MAX_BYTES = 256 * 1024;
static const size_t NFREELIST = 208;
static const size_t NPAGES = 129;
static const size_t PAGE_SHIFT = 13;

#ifdef  _WIN64 
typedef unsigned long long PAGE_ID;
#elif  _WIN32 
typedef size_t PAGE_ID;
#else //linux
#endif //

inline static void* SystemAlloc(size_t kpage)
{
	void* ptr = nullptr; // 
#ifdef _WIN32
	ptr = VirtualAlloc(0, kpage << 13, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else
	// Linux mmap实现示例
	size_t alloc_size = kpage << 13;
	ptr = mmap(nullptr, alloc_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (ptr == MAP_FAILED) ptr = nullptr;
#endif

	if (ptr == nullptr)
		throw std::bad_alloc();
	return ptr;
}

inline static void SystemFree(void* ptr)
{
#ifdef _WIN32
	VirtualFree(ptr, 0, MEM_RELEASE);
#else
	// sbrk unmmap等
#endif
}

static void*& NextObj(void* obj)
{
	return *(void**)obj;
}


class FreeList
{
public:
	void Push(void* obj)
	{
		//头插
		//*(void** )obj  = _freeList;
		NextObj(obj) = _freeList;
		++_size;
		_freeList = obj;
	}
	void PushRange(void* start, void* end ,size_t n)
	{
		NextObj(end) = _freeList;
		_freeList = start;

		_size += n;
	}
	
	void* Pop()
	{
		assert(_freeList);
		void* obj = _freeList;
		_freeList = NextObj(obj);
		--_size;
		return obj;
	}

	void PopRange(void*& start, void*& end, size_t n)
	{
		assert(n <= _size);
		start = _freeList;
		end = start;

		for (size_t i = 0; i < n - 1; ++i)
		{
			end = NextObj(end);
		}

		_freeList = NextObj(end);
		NextObj(end) = nullptr;
		_size -= n;
	}

	bool Empty()
	{
		return _freeList == nullptr;
	}

	size_t& MaxSize()
	{
		return _maxSize;	
	}

	size_t Size()
	{
		return _size;
	}
private:
	void* _freeList = nullptr;
	size_t _maxSize = 1;
	size_t _size = 0;
};

//计算对象大小的对齐映射规则
class SizeClass
{
public:
	// 整体控制在最多10%左右的内碎?浪费
	// [1,128] 8byte对? freelist[0,16)
    // [128+1,1024] 16byte对? freelist[16,72)
	// [1024+1,81024] 128byte对? freelist[72,128)
	// [8*1024+1,641024] 1024byte对? freelist[128,184)
	// [64*1024+1,256*1024] 8*1024byte对? freelist[184,208)

	static inline size_t _RoundUp(size_t bytes, size_t align)
	{
		return	(((bytes)+align - 1) & ~(align - 1));
	}


	static inline size_t RoundUp(size_t bytes)
	{
		if (bytes <= 128)
		{
			return _RoundUp(bytes, 8);
		}
		else if (bytes <= 1024)
		{
			return _RoundUp(bytes, 16);
		}
		else if (bytes <= 8 * 1024)
		{
			return _RoundUp(bytes, 128);
		}
		else if (bytes <= 64 * 1024)
		{
			return _RoundUp(bytes, 1024);
		}
		else if (bytes <= 256 * 1024)
		{
			return _RoundUp(bytes, 8*1024);
		}
		else
		{
			return _RoundUp(bytes, 1 << PAGE_SHIFT);
		}
	}

	static inline size_t _Index(size_t bytes, size_t align_shift)
	{
		return ((bytes + (1 << align_shift) - 1) >> align_shift) - 1;
	}

	//计算映射的哪一个自由链表桶
	static inline size_t Index(size_t bytes)
	{
		assert(bytes <= MAX_BYTES);

		//每个区间有多少链
		static int group_array[4] = { 16,56,56,56 };
		if (bytes <= 128)
		{
			return _Index(bytes, 3);
		}
		else if (bytes <= 1024) {
			return _Index(bytes - 128, 4) + group_array[0];
			
		}
		else if (bytes <= 8*1024) {
			 return _Index(bytes - 1024, 7) + group_array[1] + group_array[0];
			
		}
		else if (bytes <= 64 * 1024) {
			 return _Index(bytes - 8 * 1024, 10) + group_array[2] +
				group_array[1] + group_array[0];
			
		}
		else if (bytes <= 256 * 1024) {
		     return _Index(bytes - 64 * 1024, 13) + group_array[3] +
				group_array[2] + group_array[1] + group_array[0];
		
		}
		 else {
			 assert(false);
			
		}
	
			 return -1;
		
	}

	//一次从缓存中心要多少个
	static size_t NumMoveSize(size_t size)
	{
		int num = MAX_BYTES / size;
		if (num < 2)
			num = 2;

		if (num > 512)
			num = 512;
		return num; 
	}

	static size_t NumMovePage(size_t size)
	{
		size_t num = NumMoveSize(size);
		size_t npage = num * size;

		npage >>= PAGE_SHIFT;
		if (npage == 0)
		{
			npage = 1;
		}
		return npage;
	}
	//计算一次向系统获取几页

	

};

//管理多个连续页大块内存跨度结构
struct Span
{
	PAGE_ID _pageId=0;//大块内存起始页页号
	size_t _n=0;//页数量

	Span* _next=nullptr;//双向链表结构
	Span* _prev=nullptr;

	size_t _objSize = 0;//切好的小对象的大小
	size_t _useCount=0;//切好小块内存，被分配给thread cache的计数
	void* _freeList=nullptr;

	bool _isUse = false;

};

//带头双向循环链表
class SpanList
{
public:
	SpanList()
	{
		_head = new Span;
		_head->_next = _head;
		_head->_prev = _head;

	}
	Span* Begin()
	{
		return _head->_next;
	}

	Span* End()
	{
		return _head;
	}
	void PushFront(Span* span)
	{
		Insert(Begin(), span);

	}

	Span*  PopFront()
	{
		Span* front = _head->_next;
		Erase(front);
		return front;
	}
	bool Empty()
	{
		return _head->_next == _head;
	}

	void Insert(Span* pos, Span* newSpan)
	{
		assert(pos);
		assert(newSpan);

		Span* prev = pos->_prev;//prev newSpan pos
		prev->_next = newSpan;
		newSpan->_prev = prev;
		newSpan->_next = pos;
		pos->_prev = newSpan;

	}

	void Erase(Span* pos)
	{
		assert(pos);
		assert(pos != _head);
		
		Span* prev = pos->_prev;
		Span* next = pos->_next;

		prev->_next = next;
		next->_prev = prev;
	}
	
private:
	Span* _head;
public:
	std::mutex _mtx;
};