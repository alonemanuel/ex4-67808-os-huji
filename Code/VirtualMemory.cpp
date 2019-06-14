#include "VirtualMemory2.h"
#include "PhysicalMemory.h"
#include <cmath>
#include <iostream>
#include <bitset>

using namespace std;

/**
 * @brief Clear table at given frame
 */
void clearTable(uint64_t frameIndex)
{
	for (uint64_t i = 0; i < PAGE_SIZE; ++i)
	{
		PMwrite(frameIndex * PAGE_SIZE + i, 0);
	}
}

/**
 * @brief Initialize virtual address space
 */
void VMinitialize()
{
	// We are initializing the first frame (frame 0), which is the main-frame, which "is" the main page-table.
	clearTable(0);
}

/**
 * @brief Gets the offset and the page from a given virtual address.
 * @param va virtual address
 */
void getOffsetAndPage(uint64_t va, uint64_t *offset, uint64_t *page)
{
	cout << "Us: Getting offset and page for " << va << "=" << bitset<4>(va) << endl;
	*offset = va & ((1 << OFFSET_WIDTH) - 1);
	*page = va >> OFFSET_WIDTH;
	cout << "Us: Got offset=" << *offset << ", page=" << *page << "=" << bitset<4>(*page) << endl;
}

/**
 * @brief Gets word in the given offset in the given physical address and puts into word.
 */
void readWord(uint64_t baseAddress, uint64_t offset, word_t *word)
{
	cout << "Us: Reading word at base=" << baseAddress << ", offset=" << offset << endl;
	PMread(baseAddress * PAGE_SIZE + offset, word);
	cout << "Us: Read: " << *word << endl;
}

/**
 * @brief Writes a word to the given address given by its base address and offset.
 */
void writeWord(uint64_t baseAddress, uint64_t offset, word_t word)
{
	cout << "Us: Writing word at base=" << baseAddress << ", offset=" << offset << endl;
	PMwrite(baseAddress * PAGE_SIZE + offset, word);
	cout << "Us: Wrote: " << word << endl;
}

/**
 * @brief Gets the relative offset defined by the original page and the current depth.
 */
void getBaseAndOffset(uint64_t page, int depth, uint64_t *base, uint64_t *offset)
{
	cout << "Us: Getting base and offset by depth for page=" << page << "=" << bitset<4>(page) <<
		 ", "
		 "depth="
		 << depth << endl;
	uint64_t segWidth = (uint64_t) ((VIRTUAL_ADDRESS_WIDTH - OFFSET_WIDTH) / TABLES_DEPTH);
	uint64_t shiftLeft = segWidth * (TABLES_DEPTH - depth);
	uint64_t shiftRight = segWidth * (TABLES_DEPTH - depth - 1);
	cout << "Us: leftShift=" << shiftLeft << endl;
	cout << "Us: rightShift=" << shiftRight << endl;
	cout << "Us: after left: " << bitset<8>((page & ((1 << shiftLeft) - 1)));
	*offset = (page & ((1 << shiftLeft) - 1)) >> shiftRight;
	*base = depth;
	cout << "Us: Got offset " << *offset << ", base=" << *base << "=" << bitset<4>(*base) << endl;
}

/**
 * @brief Updates the max frame by putting it in maxFrame
 */
void updateMaxFrame(uint64_t *maxFrame, uint64_t frame)
{
	*maxFrame = max(*maxFrame, frame);
}

/**
 * @brief Returns the cyclic value of the current page, w.r.t the target page.
 */
uint64_t getCyclicVal(uint64_t targetPage, uint64_t currPage)
{
	cout << "Getting cycl val" << endl;
	int val1 = NUM_PAGES - abs((int) (targetPage - currPage));
	int val2 = abs((int) (targetPage - currPage));
	uint64_t currCyclDist = (uint64_t) ((val1 < val2) ? val1 : val2);
	cout << "Got: " << currCyclDist << endl;
	return currCyclDist;
}

/**
* @brief Updates the max page by putting it in maxPage
*/
void updateMaxPage(uint64_t targetPage, uint64_t *maxCyclicPage, uint64_t *maxCyclicFrame,
				   uint64_t *maxCyclicParentFrame, uint64_t *maxCyclicParentOffset, uint64_t
				   *maxCyclicVal, uint64_t currPage,
				   uint64_t currFrame, uint64_t currParentFrame, uint64_t currParentOffset)
{
	cout << "Maybe updating max page. Max=" << *maxCyclicPage << ", curr=" << currPage << endl;
	uint64_t currCyclic = getCyclicVal(targetPage, currPage);
	if (currCyclic > *maxCyclicVal)
	{
		*maxCyclicPage = currPage;
		*maxCyclicFrame = currFrame;
		*maxCyclicParentFrame = currParentFrame;
		*maxCyclicParentOffset = currParentOffset;
		*maxCyclicVal = currCyclic;
	}
}

uint64_t getCurrPage(uint64_t oldPage, uint64_t depth, uint64_t offset)
{
	cout << "Getting new page for oldPage=" << oldPage << ", offset=" << offset << endl;
	uint64_t offseted = oldPage << (int) log2(PAGE_SIZE);    // TODO: Check effect of signed
	cout << (int) log2(PAGE_SIZE) << endl;
	cout << "Offseted: " << offseted << "=" << bitset
			<4>(offseted) << endl;

	uint64_t ret = offseted | offset;
	cout << "Got: " << ret << "=" << bitset
			<4>(ret) << endl;
	return ret;
}

/**
 * @brief Gets the maximum frame index (that is open).
 */
void getMaxFrame(uint64_t targetPage, uint64_t currFrameAddr, uint64_t *maxFrame, int depth,
				 uint64_t currPage, uint64_t *maxCyclicPage, uint64_t *maxCyclicParentFrame,
				 uint64_t *maxCyclicParentOffset, uint64_t *maxCyclicFrame, uint64_t *maxCyclicVal)
{
	cout << "Us: Getting max frame" << endl;
	bool foundMax = false;
	word_t currWord;
	for (uint64_t i = 0; i < PAGE_SIZE; ++i)
	{
		readWord(currFrameAddr, i, &currWord);
		updateMaxFrame(maxFrame, currWord);
		cout << "Us: depth=" << depth << endl;
		if (currWord != 0)
		{
			currPage = getCurrPage(currPage, depth, i);
			if (depth < TABLES_DEPTH - 1)
			{
				getMaxFrame(targetPage, currWord, maxFrame, depth + 1, currPage, maxCyclicPage,
							maxCyclicFrame, maxCyclicParentFrame, maxCyclicParentOffset,
							maxCyclicVal);
			}
			else
			{
				updateMaxPage(targetPage, maxCyclicPage, maxCyclicFrame,
							  maxCyclicParentFrame, maxCyclicParentOffset, maxCyclicVal, currPage,
							  currWord, currFrameAddr, i);
			}
		}
	}
}

void removeLink(uint64_t parentFrame, uint64_t parentOffset)
{
	cout << "Should remove link from frame=" << parentFrame << ", offset=" << parentOffset << endl;
	uint64_t toRemove = (parentFrame << OFFSET_WIDTH) | parentOffset;
	PMwrite(toRemove, 0);
}

void swap(uint64_t maxCyclicParentFrame, uint64_t maxCyclicParentOffset, uint64_t maxCyclicFrame,
		  uint64_t maxCyclicPage)
{
	cout << "Swapping" << endl;
	cout << "Us: Should swap maxPage=" << maxCyclicPage << ", maxFrame=" << maxCyclicFrame
		 << ", maxParentFrame=" << maxCyclicParentFrame << endl;
	removeLink(maxCyclicParentFrame, maxCyclicParentOffset);
	cout << "Potentially removed link" << endl;
	PMevict(maxCyclicFrame, maxCyclicPage);
	cout << "Evicted" << endl;
	clearTable(maxCyclicFrame);
}

/**
 * @brief Opens a frame at the given address.
 * @param frameToOpen
 */
void openFrame(uint64_t targetPage, uint64_t *frameToOpen)
{
	cout << "Us: Opening frame" << endl;
	uint64_t maxFrame = 0, mainFrame = 0, maxCyclicPage = 0, maxCyclicFrame = 0,
			maxCyclicParentFrame = 0, maxCyclicParentOffset = 0, maxCyclicVal = 0;
	// MOTHERFUCKERRRRRR
	getMaxFrame(targetPage, mainFrame, &maxFrame, 0, 0, &maxCyclicPage,
				&maxCyclicParentFrame, &maxCyclicParentOffset, &maxCyclicFrame, &maxCyclicVal);
	cout << "Us: Max frame=" << maxFrame << endl;
	if (maxFrame + 1 < NUM_FRAMES)
	{
		*frameToOpen = maxFrame + 1;
	}
	else
	{
		swap(maxCyclicParentFrame, maxCyclicParentOffset, maxCyclicFrame, maxCyclicPage);
		*frameToOpen = maxCyclicFrame;
	}
}

/**
 * @brief Gets the word stored in the physical address represented by the virtual address.
 * @param va virtual address
 * @param depth depth in the frame-tree
 * @param value container for the output
 */
void getNewAddr(uint64_t page, int depth, word_t *prevAddr, word_t *currAddr, uint64_t *frameIdx)
{
	cout << "Us: Getting new address for page=" << page << "=" << bitset<4>(page) << ", depth="
		 << depth << ", prevAdd=" << *prevAddr << endl;
	uint64_t base, offset, openedFrame;
	word_t newAddr;
	getBaseAndOffset(page, depth, &base, &offset);
	readWord(*prevAddr, offset, &newAddr);
	*frameIdx = newAddr;
	if (newAddr == 0)
	{
		openFrame(page, &openedFrame);
		cout << "Opened frame" << endl;
		*frameIdx = openedFrame;
		clearTable(openedFrame);
		writeWord(*prevAddr, offset, openedFrame);
		*prevAddr = openedFrame;
	}
	*prevAddr = *frameIdx;
	*currAddr = newAddr;
	cout << "Us: Got new frame for " << *currAddr << endl;
}

/**
 * @brief Translates a virtual address to its coressponding physical address.
 * @param va virtual address
 * @return physical address
 */
uint64_t translateVirtAddr(uint64_t va)
{
	cout << "Us: Entered translateVirtAddr with va=" << va << endl;
	uint64_t offset, page;
	word_t output;
	getOffsetAndPage(va, &offset, &page);

	int depth = 0;
	uint64_t frameIdx;
	word_t currAddr = 0;
	word_t prevAddr = 0;
	while (depth < TABLES_DEPTH)
	{
		cout << endl;
		cout << "Us: In depth=" << depth << endl;
		getNewAddr(page, depth, &prevAddr, &currAddr, &frameIdx);
		depth++;
	}
	if (currAddr == 0)
	{
		PMrestore(frameIdx, page);
	}
	output = (frameIdx << OFFSET_WIDTH) | offset;
	cout << "Us: physical address=" << output << endl;
	return output;
}

/**
 * @brief Reads a word from the given virtual address.
 */
int VMread(uint64_t virtualAddress, word_t *value)
{
	cout << "Us: Entered VMread" << endl;
	uint64_t physicalAddr = translateVirtAddr(virtualAddress);
	PMread(physicalAddr, value);
	return 1;
}

/**
 * @brief Writes a word to the given virtual address.
 */
int VMwrite(uint64_t virtualAddress, word_t value)
{
	cout << "Us: Entered VMwrite" << endl;
	uint64_t physicalAddr = translateVirtAddr(virtualAddress);
	PMwrite(physicalAddr, value);
	return 1;
}


////////**************////////////////////

///**
// * @brief (Potentially) updates max cyclic frame number and value.
// * @param curr_f
// * @param max_cycl_f
// * @param max_cycl_dist
// * @param p
// */
//void updateToEvict(word_t *currFrameParent, word_t *currFrame, word_t *currPage, word_t
//*maxFrameParent, word_t *maxFrame, int *maxCyclDist, word_t *pageSwappedIn)
//{
//	int val1 = NUM_PAGES - abs(*pageSwappedIn - currPage);
//	int val2 = abs(*pageSwappedIn - currPage);
//	int currCyclDist = (val1 < val2) ? val1 : val2;
//
//	bool shouldUpdate = currCyclDist > *maxCyclDist;
//	if (shouldUpdate)
//	{
//		*maxCyclDist = currCyclDist;
//		*maxFrame = *currFrame;
//		*maxFrameParent = *currFrameParent;
//	}
//}
//
///**
// * @brief During going down the tree when translating an address, we have found an address pointing to 0, so now we must find the address pointing to it.
// * @param f2Find frame we are aiming to update.
// */
//void findFrame(word_t *target_parent, word_t *curr_parent, word_t *curr_f, word_t *curr_parent, int
//currDepth, bool *didUpdate, word_t *max, word_t *max_cycl_f, int *max_cycl_val, word_t
//			   *max_cycl_parent, int pageSwappedIn, int currPage)
//{
//	if (didUpdate)        // TODO: This is wasteful (all nodes are traversed). Find a better way.
//	{
//		return;
//	}
//	if (isChildless(curr_f))          // Base case?
//	{
//		*target_parent = curr_f;
//		*curr_parent = 0;
//		*didUpdate = true;
//		return;
//	}
//	if (currDepth == TABLES_DEPTH)    // Is leaf?
//	{
//		updateToEvict(currFrameParent, currFrame, currPage, maxFrameParent, maxCurrFrame,
//					  pageSwappedIn)
//		currPage++;
//		return;
//	}
//	// else:
//	for (int i = 0; i < PAGE_SIZE; ++i)
//	{
//		word_t child;    // TODO: Avoid creating many vars in the recursion tree.
//		PMread((*curr_f) * PAGE_SIZE + i, &child);
//		if (child != 0)
//		{
//			if (child > *max)
//			{
//				*max = child;
//			}
//
////			findFrame(target_parent, curr_f, &child, currDepth + 1, didUpdate, max);
//		}
//	}
//
//}
//
//void evictFrame(word_t *currAddr, word_t *max_cycl_f, word_t *max_cycl_parent)
//{
//	PMevict(frameToEvict, pageToEvict);
//	PMwrite()
//}
//
//void setFrame(word_t *currAddr, word_t *parent, word_t *didUpdate, word_t *max_f, word_t
//*max_cycl_f, word_t *max_cycl_parent)
//{
//	if (*max_f < NUM_FRAMES)
//	{
//		*currAddr = max_f + 1;
//	}
//	else
//	{
//		evictFrame(currAddr, max_cycl_f, max_cycl_parent);
//
