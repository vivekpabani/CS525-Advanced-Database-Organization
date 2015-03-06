#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "storage_mgr.h"
#include "dberror.h"
#include "test_helper.h"

// test name
char *testName;

/* test output files */
#define TESTPF "test_pagefile.bin"

/* prototypes for test functions */
static void testSinglePageContent(void);

/* main function running all tests */
int
main (void)
{
  testName = "";
  
  initStorageManager();

  testSinglePageContent();

  return 0;
}


/* check a return code. If it is not RC_OK then output a message, error description, and exit */

/* Try to create, open, and close a page file */
void
testSinglePageContent(void)
{
  SM_FileHandle fh;
  SM_PageHandle ph;
  int i;

  testName = "test single page content";

  ph = (SM_PageHandle) malloc(PAGE_SIZE);


    TEST_CHECK(createPageFile (TESTPF));
    TEST_CHECK(openPageFile (TESTPF, &fh));
    printf("created and opened file\n");
    
    // read first page into handle
    TEST_CHECK(readFirstBlock (&fh, ph));
    // the page should be empty (zero bytes)
    for (i=0; i < PAGE_SIZE; i++)
        ASSERT_TRUE((ph[i] == 0), "expected zero byte in first page of freshly initialized page");
    printf("first block was empty\n");
    
    // change ph to be a string and write that one to disk
    for (i=0; i < PAGE_SIZE; i++)
        ph[i] = (i % 10) + '0';
    TEST_CHECK(writeBlock (0, &fh, ph));
    printf("writing first block\n");
    
    // read back the page containing the string and check that it is correct
    TEST_CHECK(readFirstBlock (&fh, ph));
    for (i=0; i < PAGE_SIZE; i++)
        ASSERT_TRUE((ph[i] == (i % 10) + '0'), "character in page read from disk is the one we expected");
    printf("reading first block\n");

    // append a page, and check the number of pages.
    TEST_CHECK(appendEmptyBlock (&fh));
    ASSERT_TRUE((fh.totalNumPages == 2), "expect 2 pages after appending to a new file");

    
    // read current page into handle
    TEST_CHECK(readCurrentBlock (&fh, ph));
    // the page should be empty (zero bytes)
    for (i=0; i < PAGE_SIZE; i++)
        ASSERT_TRUE((ph[i] == 0), "expected zero byte in newly appened page");
    printf("reading --current block-- appended block was empty\n");

    // change ph to be a string and write that one to disk on current page
    for (i=0; i < PAGE_SIZE; i++)
        ph[i] = (i % 10) + '0';
    TEST_CHECK(writeCurrentBlock (&fh, ph));
    printf("writing to the --current block-- which was appended\n");
    
    // ensure page capacity to be 3 pages.
    TEST_CHECK(ensureCapacity (3, &fh));
    printf("%d\n",fh.totalNumPages);
    ASSERT_TRUE((fh.totalNumPages == 3), "expect 3 pages after -ensureCapicity- of 3");
 
    //confirm if pages appeneded.
    ASSERT_TRUE((fh.curPagePos == 3), "After appending page position should be 3.(The last page)");
   
    // read previous block
    TEST_CHECK(readPreviousBlock (&fh, ph));
    for (i=0; i < PAGE_SIZE; i++)
        ASSERT_TRUE((ph[i] == (i % 10) + '0'), "character in page read from disk is the one we expected");
    printf("reading the -previous block- on which data was written\n");
    

    // read next page into handle
    TEST_CHECK(readNextBlock (&fh, ph));
    // the page should be empty (zero bytes)
    for (i=0; i < PAGE_SIZE; i++)
        ASSERT_TRUE((ph[i] == 0), "expected zero byte in lastly appened page");
    printf("reading the -next block- which was appended earlier. Expected empty\n");

    
    // read last page into handle
    TEST_CHECK(readLastBlock (&fh, ph));
    // the page should be empty (zero bytes)
    for (i=0; i < PAGE_SIZE; i++)
        ASSERT_TRUE((ph[i] == 0), "expected zero byte in newly appened page");
    printf("reading the -last block- which was appended earlier. Expected empty\n");

  TEST_CHECK(closePageFile (&fh));
  TEST_CHECK(destroyPageFile (TESTPF));
  free(ph);  
  TEST_DONE();
}
