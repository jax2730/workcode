#include "RewindHeap.h"

static CmdMemAllocator g_Allocator;

CmdMemAllocator* CmdMemAllocator::Global() {
	return &g_Allocator;
}
