=========================
#   Problem Statement   #
=========================

The goal of this assignment is to implement a simple storage manager - a module that is
capable of reading blocks from a file on disk into memory and writing blocks from memory 
to a file on disk. The storage manager deals with pages (blocks) of fixed size (PAGE_SIZE).
In addition to reading and writing pages from a file, it provides methods for creating,
opening, and closing files. The storage manager has to maintain several types of 
information for an open file: The number of total pages in the file, the current page 
position (for reading and writing), the file name, and a POSIX file descriptor or FILE
pointer.


=========================
# How To Run The Script #
=========================

With default test cases:
————————————————————————————
Compile : make assign1_1
Run : ./assign1_1


With additional test cases:
————————————————————————————
Compile : make assign1_2
Run : ./assign1_2


To revert:
————————————————————————————
On terminal : make clean



=========================
#         Logic         #
=========================

File Related Functions :
——————————————————————

The main purpose of these methods is to manage page files which are then used by the read
and write methods. One of the main points of our idea included that we needed to store 
the total number of pages in the file so that this information could be accessed by the
respective read and write methods.

createPageFile:
1. Creates a file based on the given fileName
2. Creates two pages - one for storing any metadata such as the total number of pages
                     - second is the first page where data will begin to be written to

openPageFile:
1. Retrieves the metadata: total number of pages
2. Assigns the contents of the SM_FileHandle struct
3. mgmtInfo is used to store the FILE pointer to be used by the read and write methods

closePageFile:
1. Closes the page file pointed to by the FILE pointer in fHandle->mgmtInfo
2. If the file could not be closed, returns RC_FILE_NOT_FOUND

destroyPageFile:
1. Deletes the page file associated with fileName.
2. If this file could not be found, returns RC_FILE_NOT_FOUND


Read Related Functions :
——————————————————————

The main purpose of these functions is to read pages from the file into memory. The function
takes the file pointer from the file handler, and read the respective page given with the 
first argument - pageNum into the memory pointed by memPage.

readBlock:
1. First check if the page number is valid or not.
2. If valid, it checks if the file pointer is available or not.
3. With valid file pointer, it reads the given page and current page position is increased.

getBlockPos:
1. This function gets the current page position from the attribute curPagePos of the 
   file handler.

readFirstBlock:
1. This function reads the first block by providing the pageNum argument as 0 to the
   readBlock function.

readPreviousBlock:
1. This function reads the previous block by providing the pageNum argument as 
   (current_position - 1) to the readBlock function.

readCurrentBlock:
1. This function reads the current block by providing the pageNum argument as
   current_position to the readBlock function.

readNextBlock:
1. This function reads the next block by providing the pageNum argument as 
   (current_position + 1) to the readBlock function.

readLastBlock:
1. This function reads the last block by providing the pageNum argument as 
   (current_position - 1) to the readBlock function.


Write related Functions:
————————————————————————

Purpose of implementing write methods is to write blocks from memory to a file present on 
a disk in an efficient manner considering given constraints and requirements.

The underlying idea behind the solution:

writeBlock: 
1. First, verify the page number before writing the data into the page.
2. Seek the file write pointer to the page number given by the user.
3. Write the data from memory block addressed by the memory pointer to the file on a disk. 

writeCurrentBlock: 

1. Call writeBlock function and pass current page position as the page number where data is
   to be written.

appendEmptyBlock: 

1. First, a memory block equal to page size is allocated and a pointer addressing this block 
   is created. 
2. Seek the file write pointer to the new last page(numberOfPages + 1) 
3. Write the data from memory block addressed by the memory pointer to the file on disk. 
4. Update the total number of pages information in the file as a new last page is added in 
   this function.

ensureCapacity: 

1. First, check If the file has less than numberOfPages then required.
2. If yes, then calculate number of pages insufficient to meet  the required size of the 
   file.
3. Call appendEmptyBlock function and append the insufficient number of pages to the file.

 
==========================
#   Additional Features  #
==========================

Additional Test Cases :
——————————————————————

In addition to the default test cases, we have implemented test cases for all the remaining
functions in the test_assign1_2.c . The instructions to run these test cases are provided
above.

No memory leaks :
——————————————————————

The additional test cases are implemented and tested with valgrind for no memory leaks.

===========================
#      Contribution       #
===========================

1. Geet Kumar :
  a. Page related functions and related documentation. 
  b. Memory checking - Valgrind

2. Hina Garg :
  a. Write related functions and related documentation

3. Vivek Pabani :
  a. Read related functions and related documentation
  b. Additional test cases.


