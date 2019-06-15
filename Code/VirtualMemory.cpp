#include "VirtualMemory.h"
#include "PhysicalMemory.h"
#include <cmath>
#include <iostream>
#include <bitset>

using namespace std;

// Function declarations //
void getNewFrame(uint64_t prevAddr, uint64_t targetPage, uint64_t currFrameAddr, uint64_t
*maxFrameAddr, int depth, uint64_t currPage, uint64_t *pageToSwapIn, uint64_t *parentToSwapIn,
				 uint64_t *parentOffset, uint64_t *frameToSwapIn, uint64_t
				 *maxCyclicVal, bool *foundEmpty);

void clearTable(uint64_t frameIndex);


// Helpers //

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
	cout << "In writeWord with baseFrame = " << baseAddress << ", offset = " << offset << ", word"
																						  " = "
		 << word << endl;
	PMwrite(baseAddress * PAGE_SIZE + offset, word);
}

/**
 * @brief Checks if the depth coressponds to reaching the end of the (pseudo)tree.
 */
bool reachedEndOfTree(int depth)
{
	return depth >= TABLES_DEPTH - 1;
}

/**
 * @brief Gets the relative offset defined by the original page and the current depth.
 */
void getOffset(uint64_t page, int depth, uint64_t *offset)
{
	uint64_t segWidth = (uint64_t) ((VIRTUAL_ADDRESS_WIDTH - OFFSET_WIDTH) / TABLES_DEPTH);
	uint64_t shiftLeft = segWidth * (TABLES_DEPTH - depth);
	uint64_t shiftRight = segWidth * (TABLES_DEPTH - depth - 1);
	*offset = (page & ((1 << shiftLeft) - 1)) >> shiftRight;
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
	// If we didn't reach end of tree, we haven't reached a page.
	if ((reachedEndOfTree(depth)) && (currCyclic > *maxCyclicVal))
	{
		cout << "*****\nUpdating maxCyclicVal to be " << currCyclic << ", currPage = " << currPage
			 << endl;
		*maxCyclicPage = currPage;
		*maxCyclicFrame = currFrame;
		*maxCyclicParentFrame = currParentFrame;
		*maxCyclicParentOffset = currParentOffset;
		*maxCyclicVal = currCyclic;
	}
}

/**
 * @brief Gets page according to base (oldPage) and offset.
 */
uint64_t getCurrPage(uint64_t oldPage, uint64_t offset)
{
	uint64_t offseted = oldPage << (int) log2(PAGE_SIZE);    // TODO: Check effect of signed
	uint64_t ret = offseted | offset;
	return ret;
}

/**
 * @brief Returns true iff frame is childless (it's an empty table).
 */
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

/**
 * @brief Potentially updating pointers if the frame is an empty table.
 */
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

/**
 * @brief Gets the maximum frame index (that is open).
 */
void getNewFrame(uint64_t prevAddr, uint64_t targetPage, uint64_t currFrameAddr, uint64_t
*maxFrameAddr, int depth, uint64_t currPage, uint64_t *pageToSwapIn, uint64_t *parentToSwapIn,
				 uint64_t *parentOffset, uint64_t *frameToSwapIn, uint64_t
				 *maxCyclicVal, bool *foundEmpty)
{
	word_t currWord;
	// Goes over all children.
	for (uint64_t i = 0; i < PAGE_SIZE; ++i)
	{
		readWord(currFrameAddr, i, &currWord);
		if (currWord == 0)
		{
			continue;
		}
	cout << "Traversing with curr = " << currFrameAddr << " at offset = "
		 << i << endl;
		updateEmptyFrame(prevAddr, frameToSwapIn, parentToSwapIn, parentOffset, currWord,
						 currFrameAddr, i, foundEmpty, depth);
		if (*foundEmpty)
		{
			cout<<"Found empty"<<endl;
			return;
		}
		updateMaxFrame(maxFrameAddr, currWord);
		currPage = getCurrPage(currPage, i);
		updateMaxCyclicPage(targetPage, pageToSwapIn, frameToSwapIn,
							parentToSwapIn, parentOffset, maxCyclicVal,
							currPage, currWord, currFrameAddr, i, depth);
		if (reachedEndOfTree(depth))
		{
			cout<<"Reached end of tree"<<endl;
			continue;
		}
		getNewFrame(prevAddr, targetPage, currWord, maxFrameAddr, depth + 1, currPage, pageToSwapIn,
					parentToSwapIn, parentOffset, frameToSwapIn, maxCyclicVal, foundEmpty);
	}
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
		cout << "Swapping because found max cyclic" << endl;
		PMevict(frameToInsert, pageToInsert);
	}
	else
	{

		cout << "Swapping because found empty frame" << endl;
	}
	clearTable(frameToInsert);
}

/**
 * @brief Opens a frame at the given address.
 * @param frameToOpen
 */
void openFrame(uint64_t originalFrameParent, uint64_t targetPage, uint64_t *frameToOpen)
{
	uint64_t maxFrameFound = 0, mainFrame = 0, pageToSwapIn = 0, frameToSwapIn = 0,
			parentFrameToSwapIn = 0, parentOffsetToSwapIn = 0, maxCyclicVal = 0, depth = 0, currPage = 0;
	bool foundEmpty = false;
	getNewFrame(originalFrameParent, targetPage, mainFrame, &maxFrameFound, depth, currPage,
				&pageToSwapIn, &parentFrameToSwapIn, &parentOffsetToSwapIn, &frameToSwapIn,
				&maxCyclicVal, &foundEmpty);

	if (foundEmpty || (maxFrameFound + 1 >= NUM_FRAMES))
	{
		swap(parentFrameToSwapIn, parentOffsetToSwapIn, frameToSwapIn, pageToSwapIn, foundEmpty);
		*frameToOpen = frameToSwapIn;
	}
	else
	{
		cout << "Found max frame = " << maxFrameFound + 1 << endl;
		*frameToOpen = maxFrameFound + 1;
	}
}

/**
 * @brief Gets the word stored in the physical address represented by the virtual address.
 * @param va virtual address
 * @param depth depth in the frame-tree
 * @param value container for the output
 */
void getNewFrame(uint64_t targetPage, int depth, uint64_t *parentFrame, uint64_t *currFrame)
{
	uint64_t offset;
	getOffset(targetPage, depth, &offset);
	readWord(*parentFrame, offset, (word_t *) currFrame);
	cout << "In getNewAddr with depth = " << depth << ", parentFrame = " << *parentFrame << " and "
																							"offset = "
		 << offset << ". newAddr = " << *currFrame << endl;
	if (*currFrame == 0)
	{
		openFrame(*parentFrame, targetPage, currFrame);
		clearTable(*currFrame);
		writeWord(*parentFrame, offset, *currFrame);
	}
	*parentFrame = *currFrame;
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
	cout << "\nIn translateVirtAddr with page = " << page << "(" << bitset<PAGE_SIZE>(page) << ")"
		 << " and offset = "
			"" << offset << endl;
	int depth = 0;
	uint64_t currFrame = 0, parentFrame = 0;
	while (depth < TABLES_DEPTH)
	{
		getNewFrame(page, depth, &parentFrame, &currFrame);
		depth++;
	}
	PMrestore(currFrame, page);
	output = (currFrame << OFFSET_WIDTH) | offset;
	return output;
}

// API //

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
	cout << "Finally, writing " << value << " to " << physicalAddr << endl;
	PMwrite(physicalAddr, value);
	return 1;
}

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
