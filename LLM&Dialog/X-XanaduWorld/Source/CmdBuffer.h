/*******************************************************************
*  Copyright(c) 2014-2018 PixelGame
*  All rights reserved.
*
*  文件名称: CmdBuffer.h
*
*  简要描述:	特别写的投递到引擎命令的队列，因为频繁且数量巨大，尽可能避免
*			额外的内存开销
*
*  创建日期: 2019-11-13
*  作者:		孟欣
******************************************************************/

#pragma once 

#include <functional>
#include <map>
#include <chrono>
#include <assert.h>

#include "RewindHeap.h"

struct CmdExeTimer {

	std::map<std::pair<const char*,int>
		,std::pair<int,std::chrono::nanoseconds>>		stat;
	std::chrono::steady_clock::time_point				begin_time;
	std::pair<int,std::chrono::nanoseconds>*			p_item;
	
	void Begin(const char* f, int l) {
		auto& item = stat[std::make_pair(f,l)];
		p_item = &item;
		begin_time = std::chrono::steady_clock::now();
	}

	void End() {
		auto end_time = std::chrono::steady_clock::now();
		p_item->first++;
		p_item->second = end_time - begin_time;
	}
};

class CmdBuffer {

	typedef std::function<void()> func_t;
	// 命令节点
	struct _node_t {

		const char*	file;
		int			line;
		func_t		func;
		_node_t*	pnext;

		_node_t(const char*	_file, int _line, func_t& _func) {
			file = _file;
			line = _line;
			func.swap(_func);
			pnext = nullptr;
		}

		~_node_t() {
			;
		}
	};

	// 组成链表
	_node_t*			pHead;
	_node_t*			pLast;
	size_t				nCmd;
	// 分配器
	RewindHeap			Pool;

public:

	CmdBuffer() {
		pHead = nullptr;
		pLast = nullptr;
		nCmd = 0;
	}

	~CmdBuffer() {
		Rewind();
	}

	size_t GetCmdCount() const {
		return nCmd;
	}

	void Push(const char* _file, int _line, func_t& _func) {
		void* pMem = Pool.Alloc(sizeof(_node_t));
		_node_t* pNode = new(pMem) _node_t(_file, _line, _func);
		if (pHead == NULL) {
			pHead = pNode;
			pLast = pNode;
		}
		else {
			pLast->pnext = pNode;
			pLast = pNode;
		}
		++nCmd;
	}

	void Execute() {
		_node_t* p = pHead;
		while (p) {
			p->func();
			p = p->pnext;
		}
	}

	void Execute(CmdExeTimer* pTimer) {
		_node_t* p = pHead;
		while (p) {
			pTimer->Begin(p->file,p->line);
			p->func();
			pTimer->End();
			p = p->pnext;
		}
	}

	void Rewind() {
		_node_t* p = pHead;
		while (p) {
			auto pn = p->pnext;
			p->~_node_t();
			p = pn;
		}
		pHead = nullptr;
		pLast = nullptr;
		nCmd = 0;
		Pool.Rewind();
	}
};

