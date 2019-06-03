#include "VirtualMemory2.h"
#include "PhysicalMemory.h"
#include <math.h>
#include <iostream>

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
	cout << "Us: Getting offset and page for " << va << endl;
	*offset = va & ((1 << OFFSET_WIDTH) - 1);
	*page = va >> OFFSET_WIDTH;
	cout << "Us: Got offset=" << *offset << ", page=" << *page << endl;
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
void getOffsetByDepth(uint64_t page, int depth, uint64_t *offset)
{
	cout << "Us: Getting offset by depth for page=" << page << ", depth=" << depth << endl;
	uint64_t segWidth = (uint64_t) ((VIRTUAL_ADDRESS_WIDTH - OFFSET_WIDTH) / TABLES_DEPTH);
	uint64_t shiftLeft = segWidth * depth;
	uint64_t shiftRight = VIRTUAL_ADDRESS_WIDTH - segWidth;
	*offset = (page << shiftLeft) >> shiftRight;
	cout << "Us: Got: " << *offset << endl;
}

/**
 * @brief Gets the maximum frame index (that is open).
 */
void getMaxFrame(uint64_t *maxFrame)
{
	cout << "Us: Getting max frame" << endl;
	bool foundMax = false;
	word_t currWord;
	for (int i = 0; i < PAGE_SIZE; ++i)
	{

		readWord(currFrame, i, currWord);
	}
}

/**
 * @brief Opens a frame at the given address.
 * @param frameToOpen
 */
void openFrame(uint64_t *frameToOpen)
{
	cout<<"Us: Opening frame"<<endl;
	uint64_t maxFrame;
	getMaxFrame(&maxFrame);
}

/**
 * @brief Gets the word stored in the physical address represented by the virtual address.
 * @param va virtual address
 * @param depth depth in the frame-tree
 * @param value container for the output
 */
void getNewAddr(uint64_t page, int depth, word_t *oldAddr)
{
	cout << "Us: Getting new address for " << *oldAddr << endl;
	uint64_t offset, openedFrame;
	word_t newAddr;
	getOffsetByDepth(page, depth, &offset);
	readWord(*oldAddr, offset, &newAddr);
	if (newAddr == 0)
	{
		openFrame(&openedFrame);
		clearTable(openedFrame);
		writeWord(*oldAddr, offset, openedFrame);
		*oldAddr = newAddr;
	}
	cout << "Us: Got new address for " << *oldAddr << endl;
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
	word_t currAddr = 0;
	while (depth < TABLES_DEPTH)
	{
		cout << "Us: In depth=" << depth << endl;
		getNewAddr(page, depth, &currAddr);
		depth++;
	}
	readWord(currAddr, offset, &output);
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
	return 1;
}

