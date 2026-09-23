#pragma once
#include "Common.h"
#include "ObjectPool.h"
#include "PageMap.h"
class PageCache
{
public:
	static PageCache* GetInstance()
	{
		return &_sInst;
	}
	//获取从对象到span的映射
	Span* MapObjectToSpan(void* obj);
	void ReleaseSpanToPageCache(Span* span);
	//获取一个k页的span
	Span* NewSpan (size_t k);

	std::mutex _pageMtx;
private:
	SpanList _spanLists[NPAGES];
	ObjectPool<Span> _spanPool;

	/*std::unordered_map<PAGE_ID, Span*> _idSpanMap;
	std::unordered_map<PAGE_ID, size_t*> _idSizeMap;*/
	TCMalloc_PageMap1<32 - PAGE_SHIFT> _idSpanMap;

	PageCache() 
	{ }
	PageCache(const PageCache&) = delete;

	static PageCache _sInst;

};