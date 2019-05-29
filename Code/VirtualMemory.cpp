#include "VirtualMemory.h"
#include "PhysicalMemory.h"
#include <math.h>
#include <iostream>


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


/**
 * Breaking the p part of the address down to segments.
 */
void partitionAddr(uint64_t p, uint64_t addrSegs[])
{
	uint64_t segWidth = (uint64_t) ((VIRTUAL_ADDRESS_WIDTH - OFFSET_WIDTH) / TABLES_DEPTH);
	// TODO: waste of space to keep it 64?
	uint64_t currAddr = p;
	uint64_t i = 0;
	while (i < TABLES_DEPTH)
	{
		addrSegs[i] = currAddr & (uint64_t) pow(2, segWidth) - 1;
		currAddr = currAddr >> segWidth;
		++i;
	}
}

bool isChildless(word_t curr_f)
{
	word_t child = nullptr;
	for (int i = 0; i < PAGE_SIZE; ++i)
	{
		PMread(curr_f * PAGE_SIZE + i, &child);
		if (child != 0)
		{
			return false;
		}
	}
	return true;
}

void updateMaxCycl(word_t *curr_f, word_t *max_cycl_f, int *max_cycl_val, int p)
{
	int val1 = NUM_PAGES - (*curr_f - p);
	int val2 = *curr_f - p;
	return (val1 > val2) ? val1 : val2;
}


/**
 * @brief During going down the tree when translating an address, we have found an address pointing to 0, so now we must find the address pointing to it.
 * @param f2Find frame we are aiming to update.
 */
void findFrame2(word_t *target_parent, word_t *curr_parent, word_t *curr_f, int currDepth, bool
*didUpdate, word_t *max, word_t *max_cycl_f, int *max_cycl_val, int p)
{
	if (didUpdate)        // TODO: This is wasteful (all nodes are traversed). Find a better way.
	{
		return;
	}
	if (isChildless(curr_f))          // Base case?
	{
		*target_parent = curr_f;
		*curr_parent = 0;
		*didUpdate = true;
		return;
	}
	if (currDepth == TABLES_DEPTH)    // Is leaf?
	{
		updateMaxCycl(curr_f, max_cycl_f, max_cycl_val, p);
		return;
	}
	// else:
	for (int i = 0; i < PAGE_SIZE; ++i)
	{
		word_t child;    // TODO: Avoid creating many vars in the recursion tree.
		PMread((*curr_f) * PAGE_SIZE + i, &child);
		if (child != 0)
		{
			if (child > *max)
			{
				*max = child;
			}
			findFrame(target_parent, curr_f, &child, currDepth + 1, didUpdate, max);
		}
	}

}


void evictFrame()
{

}

void setFrame(word_t *currAddr, word_t *parent, word_t *didUpdate, word_t *max_f)
{
	if (*max_f < NUM_FRAMES)
	{
		*currAddr = max_f + 1;
	}
	else
	{
		evictFrame(word_t * currAddr);
	}
}

uint64_t virt2phys(uint64_t va)
{
	uint64_t offset, p;
	uint64_t addrSegs[TABLES_DEPTH];

	offset = va & (uint64_t) pow(2, OFFSET_WIDTH) - 1;    // TODO: Pow operation is costly
	p = va >> OFFSET_WIDTH;

	partitionAddr(p, addrSegs);
	for (int i = 0; i < TABLES_DEPTH; i++)
	{
		std::cout << i << "   " << addrSegs[i] << "\n";
	}

	int depth = TABLES_DEPTH - 1;
	word_t currAddr = 0;
	while (depth >= 0)
	{
		PMread(currAddr * PAGE_SIZE + addrSegs[depth], &currAddr);
		if (currAddr == 0)
		{
			bool didUpdate = false;
			word_t mainFrame = 0, curr_f = 0, max_f = 0, max_cycl_f = 0;
			int max_cycl_val = 0;
			findFrame(&currAddr, &mainFrame, &curr_f, 0, &didUpdate, &max_f, &max_cycl_f,
					  &max_cycl_val, p);
			if (!didUpdate)
			{
				setFrame(&currAddr, &didUpdate, &max_f)
			}
		}
		--depth;
	}
	return currAddr * PAGE_SIZE + offset;
}


// TODO: how do we check if the process succeeded and when to return 0?
int VMread(uint64_t virtualAddress, word_t *value)
{
	uint64_t physicalAddr = virt2phys(virtualAddress);
	PMread(physicalAddr, value);
	return 1;
}


int VMwrite(uint64_t virtualAddress, word_t value)
{
	uint64_t physicalAddr = virt2phys(virtualAddress);
	PMwrite(physicalAddr, value);
	return 1;
}

