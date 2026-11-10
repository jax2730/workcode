/*******************************************************************
*  Copyright(c) 2014-2018 PixelGame
*  All rights reserved.
*
*  文件名称: RewindHeap.h
*
*  简要描述:	可以回滚的内存管理器
*
*  创建日期: 2019-11-13
*  作者:		孟欣
******************************************************************/

#pragma once 

#include <stdint.h>
#include <assert.h>
#include <thread>

class RewindHeap {

	static const size_t DEFAULT_INIT_PAGE_SIZE = 16 * 1024;
	static const size_t DEFAULT_SHRINK_TICK = 60;

	struct page_t {

		static const size_t ALLIGN = 16UL;

		const size_t		page_size;
		char*				raw_mem;
		char*				mem_buf_allign;
		size_t				offset;
		page_t*				p_next;

		void* get(size_t n) {
			n = (n / ALLIGN + 1)*ALLIGN;
			char* p = nullptr;
			if (offset + n < page_size) {
				p = mem_buf_allign + offset;
				offset += n;
				return p;
			}
			else {
				return nullptr;
			}
		}

		void rewind() {
			offset = 0;
		}

		page_t(size_t _page_size) :page_size(_page_size) {
			raw_mem = new char[_page_size];
			offset = 0;
			size_t p = (size_t)raw_mem;
			p /= ALLIGN;
			p += 1;
			p *= ALLIGN;
			mem_buf_allign = (char*)p;
			p_next = nullptr;
		}

		~page_t() {
			delete[] raw_mem;
		}
	};

	// 组成链表
	page_t* pHeader;
	page_t* pLast;

	size_t	nPagesCap;
	size_t	nLastUsage;
	size_t	nPeakUsage;
	size_t	nCounter;
	size_t	nInitSize;

	void clear() {
		page_t* p = pHeader;
		while (p) {
			auto p_del = p;
			p = p->p_next;
			delete p_del;
		}
		nPagesCap = 0;
		nLastUsage = 0;
		nPeakUsage = 0;
		nCounter = 0;
		pHeader = nullptr;
		pLast = nullptr;
	}

public:

	RewindHeap(size_t initSize = DEFAULT_INIT_PAGE_SIZE) {
		pHeader = nullptr;
		pLast = nullptr;
		nPagesCap = 0;
		nLastUsage = 0;
		nPeakUsage = 0;
		nCounter = 0;
		nInitSize = initSize;
	}

	~RewindHeap() {
		clear();
	}

	void* Alloc(size_t n) {

		if (pHeader == nullptr) {
			pHeader = new page_t(nInitSize);
			pLast = pHeader;
			nPagesCap += pLast->page_size;
		}

		page_t* p = pHeader;
		while (p) {
			void* r = p->get(n);
			if (r) {
				nLastUsage += n;
				return r;
			}
			p = p->p_next;
		}

		// 扩大4倍分配个新的,尝试下，直到满足
		for (size_t i = 0; i < 10; ++i) {
			pLast->p_next = new page_t(pLast->page_size * 4);
			pLast = pLast->p_next;
			nPagesCap += pLast->page_size;
			void* p = pLast->get(n);
			if (p) {
				nLastUsage += n;
				return p;
			}
		}

		return nullptr;
	}

	void Rewind(const size_t _stick = DEFAULT_SHRINK_TICK) {

		if (nPeakUsage < nLastUsage) {
			nPeakUsage = nLastUsage;
		}

		nLastUsage = 0;
		page_t* p = pHeader;
		while (p) {
			p->rewind();
			p = p->p_next;
		}

		if (++nCounter >= _stick) {
			if (nPeakUsage * 4 < nPagesCap) {
				clear();
			}
			nPeakUsage = 0;
			nCounter = 0;
		}
	}
};


class CmdMemAllocator {

	// 进行调用者的线程安全检查
	// 保证所有调用都发生在同一线程
	struct caller_tid_checker_t {

		std::thread::id	CallerTID;

		bool check() {
			if (CallerTID == std::thread::id()) {
				CallerTID = std::this_thread::get_id();
				return true;
			}
			else {
				auto call_id = std::this_thread::get_id();
				return CallerTID == call_id;
			}
		}
	};

	static const size_t FRAME_IN_FLIGHT = 3;
	RewindHeap					Pools[FRAME_IN_FLIGHT];
	int								nDuty{ 0 };
	caller_tid_checker_t			Checker;

public:

	// 因为逻辑线程会重启，所以ID会变，坑呀。。。
	// 分配内存，别释放呦，而且不要存起来留着
	void* Alloc(size_t _nByte) {
		//assert(Checker.check());
		return Pools[nDuty].Alloc(_nByte);
	}

	// 每帧开始的时候调一下，换下一个值班的，
	// 先把自己 rewind 一下，这样就内存就回收了
	// 因为好几个轮班所以应该早就用过了
	void NextFrame() {
		//assert(Checker.check());
		++nDuty;
		nDuty %= FRAME_IN_FLIGHT;
		Pools[nDuty].Rewind();
	}

public:

	// 实例不知道放哪，就先放这里吧。。。
	static CmdMemAllocator* Global();
};
