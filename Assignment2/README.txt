
=========================
#   Problem Statement   #
=========================

The goal of this assignment is to implement a buffer manager. The buffer manager manages a fixed number 
of pages in memory that represent pages from a page file managed by the storage manager implemented in 
assignment 1. The memory pages managed by the buffer manager are called page frames or frames for short. 
We call the combination of a page file and the page frames storing pages from that file a Buffer Pool. 
The Buffer manager should be able to handle more than one open buffer pool at the same time. However, 
there can only be one buffer pool for each page file. Each buffer pool uses one page replacement 
strategy that is determined when the buffer pool is initialized. At least implement two replacement 
strategies FIFO and LRU.


=========================
# How To Run The Script #
=========================

With default test cases:
————————————————————————————
Compile : make assign2_1
Run : ./assign2_1


With additional test cases:
————————————————————————————
Compile : make assign2_2
Run : ./assign2_2


To revert:
————————————————————————————
On terminal : make clean


=========================
#         Logic         #
=========================

Data Structures and Design :
————————————————————————————

We have implemented the frames as a doubly-linked list. Each node of the frames is called pageFrameNode, and it contains
below attributes :
1. pageNum      - the page number of the page in the pageFile.
2. frameNum     - the number of the frame in the frame list.
3. dirty        - the dirty bit of the frame.( 1 = dirty, 0 = not dirty).
4. fixCount     - fixCount of the page based on the pinning/un-pinning request.
5. rf           - reference bit per node to be used by any page replacement algorithm, if required.
6. hist         - history of reference of each page in memory.
7. data         - actual data pointed by the frameNode.

There are few attributes defined or required at BufferPool level. We have implemented a structure of such attributes
called bmInfo, and assigned it to the BM_BufferPool->mgmtData. The attributes of this structure are :
1. numFrames    - filled number of frames in the frame list.
2. numRead      - total number of read done on the buffer pool.
3. numWrite     - total number of write done on the buffer pool.
4. countPinning - total number of pinning done for the buffer pool.
5. stratData    - value of BM_BufferPool->stratData
6. pageToFrame  - an array from pageNumber to frameNumber. Value : FrameNum, if page in memory; otherwise -1.
7. frameToPage  - an array from frameNumber to pageNumber. Value : PageNum, if frame is full; otherwise -1.
8. dirtyFlags   - an array of dirtyflags of all the frames.
9. fixedCounts  - an array of fixed count of all the frames.


Buffer Pool Functions :
——————————————————————

initBufferPool:
1. This function takes a BM_BufferPool instance as an argument and initializes its attributes based on other arguments.
2. It first validates the arguments provided. If any of them is invalid (for example, numPages <= 0), it returns
   an appropriate error message.
2. It then initializes the attributes of BM_BufferPool instance.
3. Finally it initializes the frame list with empty frames.

shutdownBufferPool:
1. This function takes a BM_BufferPool instance as an argument and deallocates all the memory.
2. It first validates the arguments provided, and returns an error in case of invalid arguments.
3. It then empties the frames of frame list and deallocates the memory of each node.
4. It finally deallocates the memory of BM_BufferPool->mgmtData, which refers to the bmInfo structure.

forceFlushPool:
1. This functions takes a BM_BufferPool instance as an argument and writes all the dirty frames to the file on the disk.
2. It first validates the arguments provided, and returns an error in case of invalid arguments.
3. It then iterate through the frame list starting from head. If any frame has dirty bit set as 1 :
   a. It writes the data back to the file on disk.
   b. It sets the dirty bit as 0.
   c. It increases the value of numWrite by 1;
4. Once all the frames are iterated, it returns RC_OK if no error faced.


Page Management Functions :
———————————————————————————

pinPage:
1. This function calls pinpage functions defined as per different strategies like: pinpage_FIFO, pinpage_LRU, 
   pinpage_CLOCK etc.
2. These functions pins the page with the given pagenumber pageNum.
3. Buffer manager uses different strategies to locate the page requested and tp provide the details of the page to client.

unpinPage:
1. This function first iterate through the available pages in the frames to locate the page to be unpinned.
2. If the page is not found it returns an exception "RC_NON_EXISTING_PAGE_IN_FRAME"
3. If the page is found, "fixCount" is decreased by 1 and the function returns RC_OK.

markDirty:
1. This function first iterate through the available pages in the frames to locate the page to be marked as dirty.
2. If the page is found then set the dirty bit of the page node as 1 and return RC_OK.

forcePage:
1. This function first iterate through the available pages in the frames to locate the page to be forced to disk.
2. If the page is found it opens the file and write current content of the page back to the page file on disk.
3. If page is found and the write operation is successful it returns RC_OK otherwise returns RC_NON_EXISTING_PAGE_IN_FRAME.


Statistics Functions:
————————————————————————
The main purpose of the statistics functions was to provide information regarding the buffer pool and its contents.
The print debug functions use the statistics functions to provide information about the pool.

getFrameContents:
1. frameToPage is included in BM_BufferPool->mgmtData. This is an array of PageNumbers where the ith element was
   the page stored in the ith page frame. This is updated whenever a new frame is added in the function updateNewFrame.
   getFrameContents returns this array.

getDirtyFlags:
1. This function returns an array of booleans where the ith element is true if the ith page frame is dirty.
2. This array is stored in BM_bufferPool->mgmtData->dirtyFlags. getDirtyFlags returns this array.
3. dirtyFlags is populated by traversing the list of frames and checking to see which frames are marked as dirty.

getFixCounts:
1. This function returns an array of ints where the ith element is the fix count of the page stored in the ith page frame.
2. 0 is returned for empty page frames.
3. This array is stored in BM_bufferPool->mgmtData->fixedCounts. getFixCounts returns this array.
4. fixedCounts is populated by traversing the list of frames and using the fixCount value of each frame.

getNumReadIO:
1. This function returns the number of pages that have been read from disk since a buffer pool was initialized.
2. This function returns the value of BM_bufferPool->mgmtData->numRead which is set in updateNewFrame and in initBufferPool.

getNumWriteIO:
1. This function returns the number of pages that have been written to the page file since the buffer pool was initialized.
2. This function returns the value of BM_bufferPool->mgmtData->numWrite which is set in updateNewFrame, initBufferPool,
   forcePage, and forceFlushPool.


The Page Replacement Strategies:
————————————————————————————————

pinPage_FIFO:

This function implements the FIFO page replacement strategy.

1. First, it checks to see if the page is in memory. If the page is found it calls the pageInMemory function described in
   "Helper Functions" later, and returns RC_OK.
2. If the page is required to be loaded in the memory then first free frame is located starting from head. 
   If an empty frame is found then the page is loaded in the found frame and page details are set. 
   Also, the found frame is updated to be the head of the linked list.
3. For new page, if all the frames are filled, then the function starts iterating from trail of the list to 
   locate the oldest frame with fix count 0. The found frame is updated to be the head of the linked list.
4. If the frame is found following above strategy then updateNewFrame function described in "Helper Functions" later; 
   otherwise the function returns no more space in buffer error.

pinPage_LRU:

This function implements the LRU replacement policy as described in the lecture.

1. First, it checks to see if the page is in memory. If it is, it calls the pageInMemory function described in
   Helper Functions later, and returns immediately with RC_OK.
2. Every time a frame is referenced, it is moved to the head of the framelist. So, at any moment the head will be the
   latest used frame, and the tail will be the least used frame.
3. If the page is not in memory, it starts iterating from the tail of the list to look for a frame with fixcount 0.
4. If any such frame is found, it calls the updateNewFrame function described in Helper Functions later; otherwise it
   returns no more space in buffer error.


Helper Functions:
—————————————————

There are mainly 4 helper function used throughout the application :

updateHead:
1. This function takes a framelist and a framenode as arguments, and makes the node the head of given list.
2. This function is used by different page replacement strategies in order to keep the logical order of the frames
   in the frame list.

findNodeByPageNum:
1. This function takes a framelist and a pageNumber as arguments, and searches for the page in the framelist.
2. If the page is found, it returns the frameNode; otherwise returns NULL.
3. This function is used by different page replacement strategies in order to find the required frame from the frame list.

pageInMemory:
1. This function takes a BM_BufferPool, BM_PageHandle and a pageNumber as arguments, and searches for the page in the
   framelist.
2. If the page is found :
   a. It sets the BM_PageHandle to refer to this page in memory.
   b. It increases the fixcount of the page.
3. This function is used by different page replacement strategies in case when the page is already available in
   the frame list.

updateNewFrame:
1. This function takes a BM_BufferPool, BM_PageHandle, the destination frame and a pageNumber as arguments.
2. If the destination frame has a dirty page, it writes the page back to the disk and updates related attributes.
3. It then reads the destination page from the disk into memory, and store in the given frame.
4. It updates the pageNum, numRead, dirty, fixcount and rf attributes of the destination frame.
5. This function is used by different page replacement strategies in case when the page is not in the memory. It calls the
   function with the frame to be replaced, and the new page to be loaded.



==========================
#   Additional Features  #
==========================


Additional Page Replacement Strategies :
———————————————————————————————————————

pinPage_CLOCK:

This function implements the clock replacement policy as described in the lecture.

1. First, it checks to see if the page is in memory. If it is, it returns immediately with RC_OK.
2. If the page is not in memory, it looks for the first frame with a reference bit that is equal to zero. Along the way,
   it sets all the reference bits to zero. The reference bit (rf) is set in pageFrameNode->rf.
3. The new value of found is used in updateNewFrame.

pinPage_LRU_K:

This function implements the LRU_K replacement policy as explained in the paper.

1. First, it checks to see if the page is in memory. If it is, it calls the pageInMemory function described in
   Helper Functions later, and returns immediately with RC_OK.
2. Every time a frame is referenced, the reference number(current count of pinning) is updated in the history array
   (bminfo->khist).
3. If the page is not in memory, it starts iterating from the head of the list and calculate the distance as the
   difference of current count of pinning and kth reference of the page, for all pages in memory having fixcount 0.
4. The page with the max distance is replaced. If no page is called k time (kth reference is -1 for all pages), then
   it works just as the LRU, and checks for the least recently used page.
5. In any case, if any such frame is found, it calls the updateNewFrame function described in Helper Functions later; 
   otherwise it returns no more space in buffer error.

pinPage_LFU:

This function implements the LFU page replacement strategy as explained in the paper.

1. First, it checks to see if the page is in memory. If the page is found it calls the pageInMemory function described in
   "Helper Functions" later, and returns RC_OK.
2. If the page is required to be loaded in the memory then first free frame is located starting from head. 
   If an empty frame is found then the page is loaded in the found frame and page details are set. 
   Also, the found frame is updated to be the head of the linked list.
3. For new page, if all the frames are filled, then the function starts iterating from trail of the list to 
   locate the frame having the page with minimum page frequency. The found frame is updated to be the head of the 
   linked list.
4. If the frame is found following above strategy then updateNewFrame function described in "Helper Functions" later; 
   otherwise the function returns no more space in buffer error.


Additional Error checks :
————————————————————————
Below error cases are checked and tested :
1. Try to initialize an invalid buffer pool.(0 or negative number of frames, invalid strategy)
2. Try to pin a page into a full bufferpool.
3. Try to pin an invalid page.(uninitialized page instance, or a negative page number.)
4. Try to unpin a page which is not available in framelist.
5. Try to forceflush a page which is not available in framelist.
6. Try to markdirty a page which is not available in framelist.
7. Try to unpin a page which is available in the frame list, but not pinned by any one.
8. Try to shutdown a bufferpool which is not initialized.


Additional Test Cases :
——————————————————————

In addition to the default test cases, we have implemented test cases for all the additional page replacement strategies
and the additional error checks in the test_assign2_2.c. The instructions to run these test cases are provided above.


No memory leaks :
—————————————————

The essential and the additional test cases are implemented and tested with valgrind for no memory leaks.



===========================
#      Contribution       #
===========================

1. Geet Kumar :
  a. Statistic Functions.
  b. Page Replacement Strategy : Clock.
  c. Related test cases and documentation
  d. Checking valgrind/memory leak issues

2. Hina Garg :
  a. Page Management Related Functions 
  b. Page Replacement Strategies : FIFO and LFU.
  c. Related test cases and documentation

3. Vivek Pabani :
  a. Data structure and design.
  b. Buffer Pool Functions.
  c. Page Replacement Strategies : LRU and LRU-K.
  d. Additional Error checks.
  e. Checking valgrind/memory leak issues
  f. Related test cases and documentation.
