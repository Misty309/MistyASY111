#include "PageCache.h"
#include "PageMap.h"

PageCache PageCache::_sInst;

//获取一个k页span
Span* PageCache::NewSpan(size_t k)
{
	assert(k > 0 && k < NPAGES);
	if (k > NPAGES - 1)
	{
		void* ptr = SystemAlloc(k);
		//Span* span = new Span;
		Span* span = _spanPool.New();
		span->_pageId = (PAGE_ID)ptr >> PAGE_SHIFT;
		span->_n = k;
		/*_idSpanMap[span->_pageId] = span;*/
		_idSpanMap.set(span->_pageId, span);
		return span;
	}
	//先检查第k个桶里有没有span	
	if (!_spanLists[k].Empty())
	{
		Span* kSpan = _spanLists[k].PopFront();
		// 建立id和span的映射，方便central cache回收小块内存时，查找对应的span
		for (PAGE_ID i = 0; i < kSpan->_n; ++i)
		{
			/*_idSpanMap[kSpan->_pageId + i] = kSpan;*/
			_idSpanMap.set(kSpan->_pageId + i, kSpan);
		}

		return kSpan;
	}


	//检查一下后面的桶有没有span
	for (int i = k ; i < NPAGES; i++)
	{
		if (!_spanLists[i].Empty())
		{
			Span* nSpan = _spanLists[i].PopFront();
			Span* kSpan = _spanPool.New();
	
			//在nSpan的头部切一个k页下来
			//k页span返回
			//nSpan回到对应映射的位置
			kSpan->_pageId = nSpan->_pageId;
			kSpan->_n = k;

			nSpan->_pageId += k;
			nSpan->_n -= k;

			_spanLists[nSpan->_n].PushFront(nSpan);
			_idSpanMap.set(nSpan->_pageId, nSpan);
			//建立id和span的映射方便centralcache回收小块内存时，查找对应的span
			for (PAGE_ID i = 0; i < kSpan->_n; ++i)
			{
				/*_idSpanMap[kSpan->_pageId + i] = kSpan;*/
				_idSpanMap.set(kSpan->_pageId + i, kSpan);
			}

			return kSpan;
		}
	}
	//走到这说明没有大页span了
	//这时找堆要一个128页的span
	Span* bigSpan = _spanPool.New();
	void* ptr = SystemAlloc(NPAGES - 1);
	bigSpan->_pageId = (PAGE_ID)ptr << PAGE_SHIFT;
	bigSpan->_n = NPAGES - 1;

	_spanLists[bigSpan->_n].PushFront(bigSpan);

	return NewSpan(k);
}	

Span* PageCache::MapObjectToSpan(void* obj)
{
	PAGE_ID id = ((PAGE_ID)obj >> PAGE_SHIFT);

	/*std::unique_lock<std::mutex> lock(_pageMtx);	 
	auto ret = _idSpanMap.find(id);
	if (ret != _idSpanMap.end())
	{
		return ret->second;
	}
	else
	{
		assert(false);
		return nullptr;
	}*/


	auto ret = (Span*)_idSpanMap.get(id);
	assert(ret != nullptr);
	return ret;
}

void PageCache::ReleaseSpanToPageCache(Span* span)
{
	if (span->_n > NPAGES - 1)
	{
		void* ptr = (void*)(span->_pageId << PAGE_SHIFT);
		SystemFree(ptr);
		//delete span;
		_spanPool.Delete(span);
		return;
	}
	//对span的前后页尝试合并，缓解内存碎片问题
	//向前合并
	while (1)
	{
		PAGE_ID prevId = span->_pageId - 1;
		//auto ret = _idSpanMap.find(prevId);
		////前面的页号没有
		//if (ret == _idSpanMap.end())
		//{
		//	break;
		//}

		auto ret = (Span*)_idSpanMap.get(prevId);
		if (ret == nullptr)
		{
			break;
		}
	
		
		Span* prevSpan = ret;
		if (prevSpan->_isUse == true)
		{
			break;
		}
		if (prevSpan->_n + span->_n > NPAGES - 1)
		{
			break;
		}
		span->_pageId = prevSpan->_pageId;
		span->_n += prevSpan->_n;

		_spanLists[prevSpan->_n].Erase(prevSpan);
		_spanPool.Delete(prevSpan);
	}

	//向后合并
	while (1)
	{
		PAGE_ID nextId = span->_pageId+ span->_n;
		auto ret = (Span*)_idSpanMap.get(nextId);
		if (ret == nullptr)
		{
			break;
		}

		Span* nextSpan = ret;
		if (nextSpan->_isUse == true)
		{
			break;
		}


		if (nextSpan->_n + span->_n > NPAGES - 1)
		{
			break;
		}


		span->_n += nextSpan->_n;

		_spanLists[nextSpan->_n].Erase(nextSpan);
		_spanPool.Delete(nextSpan);
	}

	_spanLists[span->_n].PushFront(span);
	span->_isUse = false;
	_idSpanMap.set(span->_pageId, span);
	_idSpanMap.set(span->_pageId + span->_n - 1, span);
}