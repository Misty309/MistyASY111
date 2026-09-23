#pragma once
#include "Common.h"


template<class T>
class ObjectPool
{
public:
	T* New()
	{
		T* obj = nullptr;
		//优先把还回来的内存对象重复利用
		if (_freeList)
		{
			void* next = *((void**)_freeList);
			obj = (T*)_freeList;
			_freeList = next;
			return obj;
		}
		else
		{
			//剩余内存不够一个块大小时，重新开大块空间
			if (_remainBytes < sizeof(T))
			{

				_remainBytes = 128 * 1024;
				_memory = (char*)malloc(_remainBytes); //这里调用的是malloc函数的
				if (_memory == nullptr)
				{
					throw std::bad_alloc();
				}
			}
			obj = (T*)_memory;
			size_t objSize = sizeof(T) < sizeof(void*) ? sizeof(void*): sizeof(T);
			_memory += objSize;
			_remainBytes -= objSize;
			
		}

		//定位new，显示T构造函数初始化
		new(obj)T;
		return obj;
		
	}

	void Delete(T* obj)
	{
		obj-> ~T();
		 //头插
		
	    *(void**)obj = _freeList;
		_freeList = obj;
		
	}

private:
	char* _memory = nullptr;//指向大块内存的指针
	size_t _remainBytes = 0;//大块内存在切分过程中剩余字节数
	void* _freeList = nullptr;//还回来过程中链接的自由链表的头指针

};


struct TreeNode
	 {
	 int _val;
	 TreeNode * _left;
	 TreeNode * _right;
	
		 TreeNode()
		 :_val(0)
		, _left(nullptr)
		, _right(nullptr)
	  {}
	 };

inline void TestObjectPool() //头文件会在多个.cpp文件中展开 那就相当于你有两个这个函数所以才有重定义的问题
 {
	// 申请释放的轮次
	 const size_t Rounds = 3;
	
		 // 每轮申请释放多少次
     const size_t N = 100000;
	
	 size_t begin1 = clock();
	 std::vector<TreeNode*> v1;
	 v1.reserve(N);
	
		 for (size_t j = 0; j < Rounds; ++j)
		 {
		 for (int i = 0; i < N; ++i)
			 {
			 v1.push_back(new TreeNode);
			 }
		 for (int i = 0; i < N; ++i)
			 {
			 delete v1[i];
			 }
		 v1.clear();
		 }
	
		 size_t end1 = clock();
	
		 ObjectPool<TreeNode> TNPool;
	     size_t begin2 = clock();
	     std::vector<TreeNode*> v2;
	      v2.reserve(N);
	
		 for (size_t j = 0; j < Rounds; ++j)
		 {
		 for (int i = 0; i < N; ++i)
			 {
			 v2.push_back(TNPool.New());
			 }
		 for (int i = 0; i < 100000; ++i)
			 {
			 TNPool.Delete(v2[i]);
			 }
		v2.clear();
		 }
	 size_t end2 = clock();
	
		 std::cout << "new cost time:" << end1 - begin1 << std::endl;
		 std::cout << "object pool cost time:" << end2 - begin2 << std::endl;
 }
 
