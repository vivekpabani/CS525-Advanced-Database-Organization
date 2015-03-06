#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "storage_mgr.h"
#include "dberror.h"

void initStorageManager (void){
}

/*
*******************  Page Functions  *************************
*/

RC createPageFile (char *fileName){
    FILE *pagef = fopen(fileName, "w");
    char *totalpg_str, *first_page;

    totalpg_str = (char *) calloc(PAGE_SIZE, sizeof(char));  /* allocate "first" page to store total number of pages information */
    first_page = (char *) calloc(PAGE_SIZE, sizeof(char));   /* considered as actual first page for the data */

    strcat(totalpg_str,"1\n");

    fwrite(totalpg_str, sizeof(char), PAGE_SIZE, pagef);
    fwrite(first_page, sizeof(char), PAGE_SIZE, pagef);

    free(totalpg_str);
    free(first_page);

    fclose(pagef);

    return RC_OK;
}


RC openPageFile (char *fileName, SM_FileHandle *fHandle){
    FILE *pagef = fopen(fileName, "r+");

    if (pagef){

        char *str;

        str = (char *) calloc(PAGE_SIZE, sizeof(char));
        fgets(str, PAGE_SIZE, pagef);

        str = strtok(str, "\n"); /* remove trailing newline char */

        fHandle->fileName = fileName; /* assign values to SM_FileHandle struct components */
        fHandle->totalNumPages = atoi(str);
        fHandle->curPagePos = 0;
        fHandle->mgmtInfo = pagef;

        free(str);
        return RC_OK;
    }
    return RC_FILE_NOT_FOUND;
}

RC closePageFile (SM_FileHandle *fHandle){

  int check = fclose(fHandle->mgmtInfo); /* close open file descriptor at fHandle->mgmtInfo */

  if(!check){
    return RC_OK;
  }

  return RC_FILE_NOT_FOUND;
}

RC destroyPageFile (char *fileName){
  int check = remove(fileName);
  if (!check){
    return RC_OK;
  }

  return RC_FILE_NOT_FOUND;
}

/*
*******************  Read Functions  *************************
*/

/*
 *This function reads a page numbered with pageNum into memPage.
 *It first checks if the page number is valid or not.
 *If valid, it checks if the file pointer is available or not.
 *With valid file pointer, it reads the given page and current page position is increased.
 */
RC readBlock (int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage){

    int seekSuccess;
    size_t readBlockSize;

    /* checks for the valid page number */
    if (pageNum > fHandle->totalNumPages || pageNum < 0){
        return RC_READ_NON_EXISTING_PAGE;
    }

    /* checks if file is open, and pointer is available */
    if (fHandle->mgmtInfo == NULL){
        return RC_FILE_NOT_FOUND;
    }

    seekSuccess = fseek(fHandle->mgmtInfo, (pageNum+1)*PAGE_SIZE*sizeof(char), SEEK_SET);

    /* checks if the file seek was successful. If yes, reads the file page into mempage. */
    if (seekSuccess == 0){
        readBlockSize = fread(memPage, sizeof(char), PAGE_SIZE, fHandle->mgmtInfo);
        fHandle->curPagePos = pageNum;
        return RC_OK;
    }
    else{
        return RC_READ_NON_EXISTING_PAGE;
    }
}

/*
 *This function gets the current page position from the attribute curPagePos.
 */
int getBlockPos (SM_FileHandle *fHandle){
    return fHandle->curPagePos;
}

/*
 *This function reads the first block by providing the pageNum argument as 0 to the readBlock function.
 */
RC readFirstBlock (SM_FileHandle *fHandle, SM_PageHandle memPage){
    return readBlock(0, fHandle, memPage);
}

/*
 *This function reads the previous block by providing the pageNum argument as (current_position - 1) to the readBlock function.
 */
RC readPreviousBlock (SM_FileHandle *fHandle, SM_PageHandle memPage){
    return readBlock(fHandle->curPagePos-1, fHandle, memPage);
}

/*
 *This function reads the current block by providing the pageNum argument as current_position to the readBlock function.
 */
RC readCurrentBlock (SM_FileHandle *fHandle, SM_PageHandle memPage){
    return readBlock(fHandle->curPagePos, fHandle, memPage);
}

/*
 *This function reads the next block by providing the pageNum argument as (current_position + 1) to the readBlock function.
 */
RC readNextBlock (SM_FileHandle *fHandle, SM_PageHandle memPage){
    return readBlock(fHandle->curPagePos+1, fHandle, memPage);
}

/*
 *This function reads the last block by providing the pageNum argument as (current_position - 1) to the readBlock function.
 */
RC readLastBlock (SM_FileHandle *fHandle, SM_PageHandle memPage){
    return readBlock(fHandle->totalNumPages, fHandle, memPage);
}


/*
*******************  Write Functions  *************************
*/

RC writeBlock (int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage){

    int seekSuccess;
    size_t writeBlockSize;

	/* Checks if page number given by the user is less than total number of pages in a file i.e page number is valid */
    if (pageNum > (fHandle->totalNumPages) || (pageNum < 0)){
        return RC_WRITE_FAILED;
    }

    seekSuccess = fseek(fHandle->mgmtInfo, (pageNum+1)*PAGE_SIZE*sizeof(char), SEEK_SET); /* seeks file write pointer to the pagenumber given by the user */

    if (seekSuccess == 0){
        writeBlockSize = fwrite(memPage, sizeof(char), PAGE_SIZE, fHandle->mgmtInfo); /* writes data from the memory block pointed by memPage to the file. */
        fHandle->curPagePos = pageNum;

        return RC_OK;
    }
    else{
        return RC_WRITE_FAILED;
    }
}


RC writeCurrentBlock (SM_FileHandle *fHandle, SM_PageHandle memPage){
	return writeBlock (fHandle->curPagePos, fHandle, memPage); /* calls writeblock function and passes current page position */
}

RC appendEmptyBlock (SM_FileHandle *fHandle){
	int seekSuccess;
	size_t writeBlockSize;
    SM_PageHandle eb;

    eb = (char *) calloc(PAGE_SIZE, sizeof(char)); /* allocates memory and return a pointer to it */

    seekSuccess = fseek(fHandle->mgmtInfo,(fHandle->totalNumPages + 1)*PAGE_SIZE*sizeof(char) , SEEK_END); /*seeks file write pointer to the last page */

    if (seekSuccess == 0){
        writeBlockSize = fwrite(eb, sizeof(char), PAGE_SIZE, fHandle->mgmtInfo); /* writes data from the memory block pointed by eb to the file i.e last page is filled with zero bytes. */
        fHandle->totalNumPages = fHandle->totalNumPages + 1;
        fHandle->curPagePos = fHandle->totalNumPages;
		rewind(fHandle->mgmtInfo);
		fprintf(fHandle->mgmtInfo, "%d\n" , fHandle->totalNumPages); /* updates total number of pages information in the file */
        fseek(fHandle->mgmtInfo, (fHandle->totalNumPages + 1)*PAGE_SIZE*sizeof(char), SEEK_SET);
        free(eb);
        return RC_OK;
	}
	else{
        free(eb);
		return RC_WRITE_FAILED;
	}
}

RC ensureCapacity (int numberOfPages, SM_FileHandle *fHandle){

	if (fHandle->totalNumPages < numberOfPages){
		int numPages = numberOfPages - fHandle->totalNumPages; /* calculates number of pages required to meet the required size of the file */
        int i;
        for (i=0; i < numPages; i++){
			appendEmptyBlock(fHandle); /* increases the size of the file to required size by appending required pages. */
		}
    }
    return RC_OK;
}
