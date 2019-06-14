#include "VirtualMemory.h"
#include "PhysicalMemory.h"
#include <cmath>
#include <iostream>
#include <bitset>

using namespace std;

void getNewFrame(uint64_t prevAddr, uint64_t targetPage, uint64_t currFrameAddr, uint64_t
*maxFrame, int depth, uint64_t currPage, uint64_t *maxCyclicPage, uint64_t *maxCyclicParentFrame,
				 uint64_t *maxCyclicParentOffset, uint64_t *maxCyclicFrame, uint64_t
				 *maxCyclicVal, bool *foundEmpty);

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
	*offset = va & ((1 << OFFSET_WIDTH) - 1);
	*page = va >> OFFSET_WIDTH;
}

/**
 * @brief Gets word in the given offset in the given physical address and puts into word.
 */
void readWord(uint64_t baseAddress, uint64_t offset, word_t *word)
{
	PMread(baseAddress * PAGE_SIZE + offset, word);
}

/**
 * @brief Writes a word to the given address given by its base address and offset.
 */
void writeWord(uint64_t baseAddress, uint64_t offset, word_t word)
{
	PMwrite(baseAddress * PAGE_SIZE + offset, word);
}

bool reachedEndOfTree(int depth)
{
	return depth >= TABLES_DEPTH - 1;
}

/**
 * @brief Gets the relative offset defined by the original page and the current depth.
 */
void getBaseAndOffset(uint64_t page, int depth, uint64_t *base, uint64_t *offset)
{
	uint64_t segWidth = (uint64_t) ((VIRTUAL_ADDRESS_WIDTH - OFFSET_WIDTH) / TABLES_DEPTH);
	uint64_t shiftLeft = segWidth * (TABLES_DEPTH - depth);
	uint64_t shiftRight = segWidth * (TABLES_DEPTH - depth - 1);
	*offset = (page & ((1 << shiftLeft) - 1)) >> shiftRight;
	*base = depth;
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
	int val1 = NUM_PAGES - abs((int) (targetPage - currPage));
	int val2 = abs((int) (targetPage - currPage));
	uint64_t currCyclDist = (uint64_t) ((val1 < val2) ? val1 : val2);
	return currCyclDist;
}

/**
* @brief Updates the max page by putting it in maxPage
*/
void updateMaxCyclicPage(uint64_t targetPage, uint64_t *maxCyclicPage, uint64_t *maxCyclicFrame,
						 uint64_t *maxCyclicParentFrame, uint64_t *maxCyclicParentOffset, uint64_t
						 *maxCyclicVal, uint64_t currPage, uint64_t currFrame,
						 uint64_t currParentFrame, uint64_t currParentOffset, int depth)
{
	uint64_t currCyclic = getCyclicVal(targetPage, currPage);
	if ((reachedEndOfTree(depth)) && (currCyclic > *maxCyclicVal))
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
	uint64_t offseted = oldPage << (int) log2(PAGE_SIZE);    // TODO: Check effect of signed

	uint64_t ret = offseted | offset;
	return ret;
}

bool isChildless(uint64_t frame)
{
	word_t currWord = 0;

	for (uint64_t i = 0; i < PAGE_SIZE; ++i)
	{
		readWord(frame, i, &currWord);
		if (currWord != 0)
		{
			return false;
		}
	}
	return true;
}

void updateEmptyFrame(uint64_t targetFrame, uint64_t *frameSwappedIn,
					  uint64_t *parentSwappedIn, uint64_t *parentOffset, uint64_t currFrameAddr,
					  uint64_t currParentFrame, uint64_t currParentOffset, bool *foundEmpty, int
					  depth)
{
	if (reachedEndOfTree(depth))
	{
		return;
	}
	if ((targetFrame != currFrameAddr) && (isChildless(currFrameAddr)))
	{
		*foundEmpty = true;
		*frameSwappedIn = currFrameAddr;
		*parentSwappedIn = currParentFrame;
		*parentOffset = currParentOffset;
	}
}

void probeChildren(uint prevAddr, uint64_t targetPage, uint64_t currFrameAddr, uint64_t
*maxFrameAddr, int depth, uint64_t currPage, uint64_t *pageToSwapIn, uint64_t *parentToSwapIn,
				   uint64_t *parentOffset, uint64_t *frameToSwapIn, uint64_t
				   *maxCyclicVal, bool *foundEmpty)
{
	word_t currWord;
	for (uint64_t i = 0; (i < PAGE_SIZE) && (!*foundEmpty); ++i)
	{
		readWord(currFrameAddr, i, &currWord);
		if (currWord == 0)
		{
			continue;
		}
		currPage = getCurrPage(currPage, depth, i);
		updateEmptyFrame(prevAddr, frameToSwapIn, parentToSwapIn, parentOffset, currWord,
						 currFrameAddr, i, foundEmpty, depth);
		if (*foundEmpty)
		{
			return;
		}
		updateMaxFrame(maxFrameAddr, currWord);
		updateMaxCyclicPage(targetPage, pageToSwapIn, frameToSwapIn,
							parentToSwapIn, parentOffset, maxCyclicVal,
							currPage, currWord, currFrameAddr, i, depth);
		if (reachedEndOfTree(depth))
		{
			return;
		}
		getNewFrame(prevAddr, targetPage, currWord, maxFrameAddr, depth + 1, currPage, pageToSwapIn,
					parentToSwapIn, parentOffset, frameToSwapIn, maxCyclicVal, foundEmpty);
	}
}

/**
 * @brief Gets the maximum frame index (that is open).
 */
void getNewFrame(uint64_t prevAddr, uint64_t targetPage, uint64_t currFrameAddr, uint64_t
*maxFrame, int depth, uint64_t currPage, uint64_t *maxCyclicPage, uint64_t *maxCyclicParentFrame,
				 uint64_t *maxCyclicParentOffset, uint64_t *maxCyclicFrame, uint64_t
				 *maxCyclicVal, bool *foundEmpty)
{
	if (*foundEmpty)
	{
		return;
	}
	probeChildren(prevAddr, targetPage, currFrameAddr, maxFrame, depth,
				  currPage, maxCyclicPage, maxCyclicParentFrame,
				  maxCyclicParentOffset, maxCyclicFrame, maxCyclicVal, foundEmpty);
}

/**
 * @brief Removes link from parent.
 */
void removeLink(uint64_t parentFrame, uint64_t parentOffset)
{
	uint64_t toRemove = (parentFrame << OFFSET_WIDTH) | parentOffset;
	PMwrite(toRemove, 0);
}

/**
 * @brief Swaps that frame and handles evicting parent.
 */
void swap(uint64_t parentFrame, uint64_t parentOffset, uint64_t frameToInsert,
		  uint64_t pageToInsert, bool foundEmpty)
{
	removeLink(parentFrame, parentOffset);
	if (!foundEmpty)
	{
		PMevict(frameToInsert, pageToInsert);
	}
	clearTable(frameToInsert);
}

/**
 * @brief Opens a frame at the given address.
 * @param frameToOpen
 */
void openFrame(uint64_t prevAddr, uint64_t targetPage, uint64_t *frameToOpen)
{
	uint64_t maxFrame = 0, mainFrame = 0, pageToSwapIn = 0, frameToSwapIn = 0,
			parentToSwapInFrame = 0, parentToSwapInOffset = 0, maxCyclicVal = 0;
	bool foundEmpty = false;
	getNewFrame(prevAddr, targetPage, mainFrame, &maxFrame, 0, 0, &pageToSwapIn,
				&parentToSwapInFrame, &parentToSwapInOffset, &frameToSwapIn, &maxCyclicVal,
				&foundEmpty);
	if (foundEmpty || (maxFrame + 1 >= NUM_FRAMES))
	{
		swap(parentToSwapInFrame, parentToSwapInOffset, frameToSwapIn, pageToSwapIn, foundEmpty);
		*frameToOpen = frameToSwapIn;
	}
	else
	{
		*frameToOpen = maxFrame + 1;
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
	uint64_t base, offset, openedFrame;
	word_t newAddr;
	getBaseAndOffset(page, depth, &base, &offset);
	readWord(*prevAddr, offset, &newAddr);
	*frameIdx = newAddr;
	if (newAddr == 0)
	{
		openFrame(*prevAddr, page, &openedFrame);
		*frameIdx = openedFrame;
		clearTable(openedFrame);
		writeWord(*prevAddr, offset, openedFrame);
		*prevAddr = openedFrame;
	}
	*prevAddr = *frameIdx;
	*currAddr = newAddr;
}

/**
 * @brief Translates a virtual address to its coressponding physical address.
 * @param va virtual address
 * @return physical address
 */
uint64_t translateVirtAddr(uint64_t va)
{
	uint64_t offset, page;
	word_t output;
	getOffsetAndPage(va, &offset, &page);

	int depth = 0;
	uint64_t frameIdx;
	word_t currAddr = 0;
	word_t prevAddr = 0;
	while (depth < TABLES_DEPTH)
	{
		getNewAddr(page, depth, &prevAddr, &currAddr, &frameIdx);
		depth++;
	}
	if (currAddr == 0)
	{
		PMrestore(frameIdx, page);
	}
	output = (frameIdx << OFFSET_WIDTH) | offset;
	return output;
}

/**
 * @brief Reads a word from the given virtual address.
 */
int VMread(uint64_t virtualAddress, word_t *value)
{
	uint64_t physicalAddr = translateVirtAddr(virtualAddress);
	PMread(physicalAddr, value);
	return 1;
}

/**
 * @brief Writes a word to the given virtual address.
 */
int VMwrite(uint64_t virtualAddress, word_t value)
{
	uint64_t physicalAddr = translateVirtAddr(virtualAddress);
	PMwrite(physicalAddr, value);
	return 1;
}
