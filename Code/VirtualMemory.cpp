#include "VirtualMemory.h"
#include "PhysicalMemory.h"
#include <math.h>



void clearTable(uint64_t frameIndex)
{
	for (uint64_t i = 0; i < PAGE_SIZE; ++i)
	{
		PMwrite(frameIndex * PAGE_SIZE + i, 0);
	}
}

void VMinitialize()
{
	// We are initializing the first frame (frame 0), which is the main-frame, which "is" the main page-table.
	clearTable(0);
}

uint64_t virt2phys(uint64_t va)
{
	uint64_t offset, p;
	uint64_t addrSegs[TABLES_DEPTH];

	offset = va && pow(2, OFFSET_WIDTH);		// TODO: Pow operation is costly
	p = va >> OFFSET_WIDTH;

	partitionAddr(p, &addrSegs);

	int depth = 0;
	uint64_t currAddr;
	PMread(0 + ??, &currAddr)
	while (depth < TABLES_DEPTH - 1)
	{
		PMread(currAddr * PAGE_SIZE + addrSegs[depth], &currAddr);
		++depth;
	}
	return currAddr * PAGE_SIZE + addrSegs[depth];
}
// TODO: how do we check if the process succeeded and when to return 0?
int VMread(uint64_t virtualAddress, word_t *value)
{
	uint64_t physicalAddr = virt2phys(virtualAddress);
	PMread(physicalAddr, *value);
	return 1;
}


int VMwrite(uint64_t virtualAddress, word_t value)
{
	uint64_t physicalAddr = virt2phys(virtualAddress);
	PMwrite(physicalAddr, value);
	return 1;
}

/**
 * Breaking the p part of the address down to segments.
 */
void partitionAddr(uint64_t p, uint64_t addrSegs[])
{
	uint64_t segWidth = VIRTUAL_ADDRESS_WIDTH / TABLES_DEPTH;
	// TODO: waste of space to keep it 64?
	uint64_t currAddr;
	uint64_t i = 0;
	while (i < TABLES_DEPTH)
	{
		addrSegs[i] = currAddr && pow(2, segWidth);
		currAddr = currAddr >> segWidth;
		++i;
	}
}
