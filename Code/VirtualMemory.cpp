#include "VirtualMemory.h"
#include "PhysicalMemory.h"
#include <math.h>
#include <iostream>


using namespace std;

void clearTable(uint64_t frameIndex) {
    for (uint64_t i = 0; i < PAGE_SIZE; ++i) {
        PMwrite(frameIndex * PAGE_SIZE + i, 0);
    }
}

void VMinitialize() {
    // We are initializing the first frame (frame 0), which is the main-frame, which "is" the main page-table.
    clearTable(0);
}


/**
 * Breaking the p part of the address down to segments.
 */
void partitionAddr(uint64_t p, uint64_t addrSegs[]) {
    uint64_t segWidth = (uint64_t) ((VIRTUAL_ADDRESS_WIDTH - OFFSET_WIDTH) / TABLES_DEPTH);
    // TODO: waste of space to keep it 64?
    uint64_t currAddr = p;
    uint64_t i = 0;
    while (i < TABLES_DEPTH) {
        addrSegs[i] = currAddr & (uint64_t) pow(2, segWidth) - 1;
        currAddr = currAddr >> segWidth;
        ++i;
    }
}

bool isChildless(word_t curr_f) {
    word_t child = NULL;
    for (int i = 0; i < PAGE_SIZE; ++i) {
        PMread(curr_f * PAGE_SIZE + i, &child);
        if (child != 0) {
            return false;
        }
    }
    return true;
}

/**
 * @brief (Potentially) updates max cyclic frame number and value.
 * @param curr_f
 * @param max_cycl_f
 * @param max_cycl_dist
 * @param p
 */
void updateToEvict(word_t *currFrameParent, word_t *currFrame, word_t *currPage, word_t
*maxFrameParent, word_t *maxFrame, int *maxCyclDist, word_t *pageSwappedIn) {

    int val1 = NUM_PAGES - abs(*pageSwappedIn - *currPage);
    int val2 = abs(*pageSwappedIn - *currPage);
    int currCyclDist = (val1 < val2) ? val1 : val2;

    bool shouldUpdate = currCyclDist > *maxCyclDist;
    if (shouldUpdate) {
        *maxCyclDist = currCyclDist;
        *maxFrame = *currFrame;
        *maxFrameParent = *currFrameParent;
    }
}


/**
 * @brief During going down the tree when translating an address, we have found an address pointing to 0, so now we must find the address pointing to it.
 * @param f2Find frame we are aiming to update.
 */
void findFrame(word_t *target_parent, word_t *currFrame, word_t *currFrameParent, int
currDepth, bool *didUpdate, word_t *max, word_t *maxCurrFrame, word_t
               *maxFrameParent, uint64_t *pageSwappedIn, int *maxCyclDist, word_t *currPage) {
    if (didUpdate)        // TODO: This is wasteful (all nodes are traversed). Find a better way.
    {
        return;
    }
    if (isChildless(*currFrame))          // Base case?
    {
        *target_parent = *currFrame;
        *currFrameParent = 0;
        *didUpdate = true;
        return;
    }
    if (currDepth == TABLES_DEPTH)    // Is leaf?
    {
//        updateToEvict(currFrameParent, currFrame, currPage, maxFrameParent, maxCurrFrame, maxCyclDist, pageSwappedIn);
        (*currPage)++;
        return;
    }
    // else:
    for (int i = 0; i < PAGE_SIZE; ++i) {
        word_t child;    // TODO: Avoid creating many vars in the recursion tree.
        PMread((*currFrame) * PAGE_SIZE + i, &child);
        if (child != 0) {
            if (child > *max) {
                *max = child;
            }

//			findFrame(target_parent, curr_f, &child, currDepth + 1, didUpdate, max);
        }
    }

}


void
evictFrame(word_t *currAddr, word_t *max_cycl_f, word_t *max_cycl_parent, word_t frameToEvict, word_t pageToEvict) {
    PMevict((uint64_t) frameToEvict, (uint64_t) pageToEvict);
    PMwrite((uint64_t) frameToEvict,
            0);   // TODO: Make sure the correct cell is updated (maybe send i (as in the offset)?)
}

void setFrame(word_t *currAddr, word_t *parent, word_t *didUpdate, word_t *max_f, word_t
*max_cycl_f, word_t *max_cycl_parent) {
    if (*max_f < NUM_FRAMES) {
        *currAddr = *max_f + 1;
    } else {
//        evictFrame(currAddr, max_cycl_f, max_cycl_parent);
    }
}

/**
 * @brief Translate a virtual address to a physical one.
 * @param va
 * @return
 */
uint64_t virt2phys(uint64_t va) {
    cout << "Us: Entered virt2phys" << endl;

    uint64_t offset, p;
    uint64_t addrSegs[TABLES_DEPTH];

    offset = va & (uint64_t) pow(2, OFFSET_WIDTH) - 1;    // TODO: Pow operation is costly
    p = va >> OFFSET_WIDTH;
    // Partition the address into segments
    partitionAddr(p, addrSegs);

    int depth = TABLES_DEPTH - 1;
    word_t currAddr = 0;

    // "Go down the tree" and retrieve the address
    while (depth >= 0) {
        cout << "Us: Traversing the tree" << endl;

        PMread(currAddr * PAGE_SIZE + addrSegs[depth], &currAddr);
        if (currAddr == 0) {
            bool didUpdate = false;
            word_t mainFrame = 0, curr_f = 0, max_f = 0, max_cycl_f = 0, max_cycl_parent = 0, curr_page = 0;
            int max_cycl_val = 0;
            findFrame(&currAddr, &mainFrame, &curr_f, 0, &didUpdate, &max_f, &max_cycl_f,
                      &max_cycl_parent, &p, &max_cycl_val, &curr_page);
            //
            if (!didUpdate) {
//                setFrame(&currAddr, &didUpdate, &max_f)
            }
            PMwrite((uint64_t) currAddr * PAGE_SIZE + addrSegs[depth - 1], mainFrame);

        }
        --depth;
    }
    return currAddr * PAGE_SIZE + offset;
}


// TODO: how do we check if the process succeeded and when to return 0?
int VMread(uint64_t virtualAddress, word_t *value) {
    cout << "Us: Entered VMread" << endl;
    uint64_t physicalAddr = virt2phys(virtualAddress);
    PMread(physicalAddr, value);
    return 1;
}


int VMwrite(uint64_t virtualAddress, word_t value) {
    uint64_t physicalAddr = virt2phys(virtualAddress);
//    PMwrite(physicalAddr, value);
    return 1;
}

