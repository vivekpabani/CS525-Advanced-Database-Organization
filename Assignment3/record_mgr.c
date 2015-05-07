#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include "buffer_mgr.h"
#include "storage_mgr.h"
#include "record_mgr.h"

/* Copied from the rm_serializer file for extra serialization.*/

typedef struct VarString {
    char *buf;
    int size;
    int bufsize;
} VarString;

#define MAKE_VARSTRING(var)				\
do {							\
var = (VarString *) malloc(sizeof(VarString));	\
var->size = 0;					\
var->bufsize = 100;					\
var->buf = malloc(100);				\
} while (0)

#define FREE_VARSTRING(var)			\
do {						\
free(var->buf);				\
free(var);					\
} while (0)

#define GET_STRING(result, var)			\
do {						\
result = malloc((var->size) + 1);		\
memcpy(result, var->buf, var->size);	\
result[var->size] = '\0';			\
} while (0)

#define RETURN_STRING(var)			\
do {						\
char *resultStr;				\
GET_STRING(resultStr, var);			\
FREE_VARSTRING(var);			\
return resultStr;				\
} while (0)

#define ENSURE_SIZE(var,newsize)				\
do {								\
if (var->bufsize < newsize)					\
{								\
int newbufsize = var->bufsize;				\
while((newbufsize *= 2) < newsize);			\
var->buf = realloc(var->buf, newbufsize);			\
}								\
} while (0)

#define APPEND_STRING(var,string)					\
do {									\
ENSURE_SIZE(var, var->size + strlen(string));			\
memcpy(var->buf + var->size, string, strlen(string));		\
var->size += strlen(string);					\
} while(0)

#define APPEND(var, ...)			\
do {						\
char *tmp = malloc(10000);			\
sprintf(tmp, __VA_ARGS__);			\
APPEND_STRING(var,tmp);			\
free(tmp);					\
} while(0)

/* copied from rm_serializer file for attribute functions */
RC
attrOffset (Schema *schema, int attrNum, int *result)
{
    int offset = 0;
    int attrPos = 0;

    for(attrPos = 0; attrPos < attrNum; attrPos++)
        switch (schema->dataTypes[attrPos])
    {
        case DT_STRING:
            offset += schema->typeLength[attrPos];
            break;
        case DT_INT:
            offset += sizeof(int);
            break;
        case DT_FLOAT:
            offset += sizeof(float);
            break;
        case DT_BOOL:
            offset += sizeof(bool);
            break;
    }

    *result = offset;
    return RC_OK;
}


/* Used in scan functions*/
typedef struct recordInfo {
    Expr *cond;
    int curSlot;
    int curPage;
    int numPages;
    int numSlots;

}recordInfo;

/* list of tombstones */
typedef struct tNode {
    RID id;
    struct tNode *next;
}tNode;

/* record_mgr starts. */

typedef struct tableInfo{

    int schemaLength;
    int recordStartPage;
    int recordLastPage;
    int numTuples;
    int slotSize;
    int maxSlots;
    int tNodeLen;
    tNode *tstone_root;
    BM_BufferPool *bm;
}tableInfo;


int slotSize(Schema *schema){
    int slot_size=0, i=0, tempSize;
    slot_size += (1 + 5 + 1 + 5 + 1 + 1 + 1); // 2 square brackets, 2 int, 1 hyphen, 1 space, 1 bracket.

    for(i=0; i<schema->numAttr; ++i){
        switch (schema->dataTypes[i]){
            case DT_STRING:
                tempSize = schema->typeLength[i];
                break;
            case DT_INT:
                tempSize = 5;
                break;
            case DT_FLOAT:
                tempSize = 10;
                break;
            case DT_BOOL:
                tempSize = 5;
                break;
            default:
                break;
        }
        slot_size += (tempSize + strlen(schema->attrNames[i]) + 1 + 1);     // attrname_len, dataType_length, colon, coma or ending bracket.
    }
    return slot_size;
}

char *tableInfoToStr(tableInfo *info){
    VarString *result;
    MAKE_VARSTRING(result);
    APPEND(result, "SchemaLength <%i> FirstRecordPage <%i> LastRecordPage <%i> NumTuples <%i> SlotSize <%i> MaxSlots <%i> ", info->schemaLength, info->recordStartPage, info->recordLastPage, info->numTuples, info->slotSize, info->maxSlots);
    tNode *temp_root;
    temp_root = info->tstone_root;
    int tnode_len = 0;

    while(temp_root != NULL){
        temp_root = temp_root->next;
        tnode_len++;
    }
    APPEND(result, "tNodeLen <%i> <", tnode_len);
    temp_root = info->tstone_root;
    while(temp_root != NULL){
        APPEND(result,"%i:%i%s ",temp_root->id.page, temp_root->id.slot, (temp_root->next != NULL) ? ", ": "");
        temp_root = temp_root->next;
    }
    APPEND_STRING(result, ">\n");

    char *resultStr;				\
    GET_STRING(resultStr, result);
    return resultStr;

}

tableInfo *strToTableInfo(char *info_str){
    tableInfo *info = (tableInfo*) malloc(sizeof(tableInfo));

    char info_data[strlen(info_str)];
    strcpy(info_data, info_str);

    char *temp1, *temp2;
    temp1 = strtok (info_data,"<");
    temp1 = strtok (NULL,">");
    info->schemaLength = strtol(temp1, &temp2, 10);

    temp1 = strtok (NULL,"<");
    temp1 = strtok (NULL,">");
    info->recordStartPage = strtol(temp1, &temp2, 10);

    temp1 = strtok (NULL,"<");
    temp1 = strtok (NULL,">");
    info->recordLastPage = strtol(temp1, &temp2, 10);

    temp1 = strtok (NULL,"<");
    temp1 = strtok (NULL,">");
    info->numTuples = strtol(temp1, &temp2, 10);

    temp1 = strtok (NULL,"<");
    temp1 = strtok (NULL,">");
    info->slotSize = strtol(temp1, &temp2, 10);

    temp1 = strtok (NULL,"<");
    temp1 = strtok (NULL,">");
    info->maxSlots = strtol(temp1, &temp2, 10);

    temp1 = strtok (NULL,"<");
    temp1 = strtok (NULL,">");
    int tnode_len = strtol(temp1, &temp2, 10);
    info->tNodeLen = tnode_len;
    temp1 = strtok (NULL,"<");
    int i, page, slot;

    info->tstone_root = NULL;
    tNode *temp_node;

    for(i=0; i<tnode_len; ++i){
        temp1 = strtok (NULL,":");
        page = strtol(temp1, &temp2, 10);

        if(i != (tnode_len - 1)){
            temp1 = strtok (NULL,",");
        }
        else{
            temp1 = strtok (NULL,">");
        }
        slot = strtol(temp1, &temp2, 10);

        if (info->tstone_root == NULL){
            info->tstone_root = (tNode *)malloc(sizeof(tNode));
            info->tstone_root->id.page = page;
            info->tstone_root->id.slot = slot;
            temp_node = info->tstone_root;
        }
        else{
            temp_node->next = (tNode *)malloc(sizeof(tNode));
            temp_node->next->id.page = page;
            temp_node->next->id.slot = slot;

            temp_node = temp_node->next;
        }

    }
    return info;
}

int getSchemaSize (Schema *schema){
    int i, size = 0;

    size = sizeof(int)                          // numAttr
    + sizeof(int)*(schema->numAttr)     // dataTypes
    + sizeof(int)*(schema->numAttr)     // type_lengths
    + sizeof(int)                       // keySize
    + sizeof(int)*(schema->keySize);    // keyAttrs

    for (i=0; i<schema->numAttr; ++i){
        size += strlen(schema->attrNames[i]);
    }

    return size;
}


Record *deserializeRecord(char *record_str, RM_TableData *rel){

    Schema *schema = rel->schema;
    Record *record = (Record *) malloc(sizeof(Record));
    tableInfo *info = (tableInfo *) (rel->mgmtData);
    Value *value;
    record->data = (char *)malloc(sizeof(char) * info->slotSize);
    char record_data[strlen(record_str)];
    strcpy(record_data, record_str);
    char *temp1, *temp2;

    temp1 = strtok(record_data,"-");

    /* int page; */

    /* page = strtol(temp1+1, &temp2, 10); */

    temp1 = strtok (NULL,"]");
    /* int slot; */
    /* slot = strtol(temp1, &temp2, 10); */
    temp1 = strtok (NULL,"(");

    int i;

    for(i=0; i<schema->numAttr; ++i){
        temp1 = strtok (NULL,":");
        if(i == (schema->numAttr - 1)){
            temp1 = strtok (NULL,")");
        }
        else{
            temp1 = strtok (NULL,",");
        }

        /* set attribute values as per the attributes datatype */
        switch(schema->dataTypes[i]){
            case DT_INT:
            {

                int val = strtol(temp1, &temp2, 10);

                MAKE_VALUE(value, DT_INT, val);
                setAttr (record, schema, i, value);
		freeVal(value);
            }
                break;
            case DT_STRING:
            {
                MAKE_STRING_VALUE(value, temp1);
                setAttr (record, schema, i, value);
		freeVal(value);
            }
                break;
            case DT_FLOAT:
            {
                float val = strtof(temp1, NULL);
                MAKE_VALUE(value, DT_FLOAT, val);
                setAttr (record, schema, i, value);
		freeVal(value);
            }
                break;
            case DT_BOOL:
            {
                bool val;
                val = (temp1[0] == 't') ? TRUE : FALSE;
                MAKE_VALUE(value, DT_BOOL, val);
                setAttr (record, schema, i, value);
		freeVal(value);
            }
                break;
        }
    }
    free(record_str);
    return record;
}

Schema *deserializeSchema(char *schema_str){
    Schema *schema = (Schema *) malloc(sizeof(Schema));
    int i, j;

    char schema_data[strlen(schema_str)];
    strcpy(schema_data, schema_str);

    char *temp1, *temp2;
    temp1 = strtok (schema_data,"<");
    temp1 = strtok (NULL,">");

    int numAttr;
    numAttr = strtol(temp1, &temp2, 10);
    schema->numAttr= numAttr;

    schema->attrNames=(char **)malloc(sizeof(char*)*numAttr);
    schema->dataTypes=(DataType *)malloc(sizeof(DataType)*numAttr);
    schema->typeLength=(int *)malloc(sizeof(int)*numAttr);
    char* str_ref[numAttr];
    temp1 = strtok (NULL,"(");

    for(i=0; i<numAttr; ++i){
        temp1 = strtok (NULL,": ");
        schema->attrNames[i]=(char *)calloc(strlen(temp1), sizeof(char));
        strcpy(schema->attrNames[i], temp1);

        if(i == numAttr-1){
            temp1 = strtok (NULL,") ");
        }
        else{
            temp1 = strtok (NULL,", ");
        }

        str_ref[i] = (char *)calloc(strlen(temp1), sizeof(char));

        if (strcmp(temp1, "INT") == 0){
            schema->dataTypes[i] = DT_INT;
            schema->typeLength[i] = 0;
        }
        else if (strcmp(temp1, "FLOAT") == 0){
            schema->dataTypes[i] = DT_FLOAT;
            schema->typeLength[i] = 0;
        }
        else if (strcmp(temp1, "BOOL") == 0){
            schema->dataTypes[i] = DT_BOOL;
            schema->typeLength[i] = 0;
        }
        else{
            strcpy(str_ref[i], temp1);
        }
    }

    int keyFlag = 0, keySize = 0;
    char* keyAttrs[numAttr];

    if((temp1 = strtok (NULL,"(")) != NULL){
        temp1 = strtok (NULL,")");
        char *key = strtok (temp1,", ");

        while(key != NULL){
            keyAttrs[keySize] = (char *)malloc(strlen(key)*sizeof(char));
            strcpy(keyAttrs[keySize], key);
            keySize++;
            key = strtok (NULL,", ");
        }
        keyFlag = 1;
    }

    char *temp3;
    for(i=0; i<numAttr; ++i){
        if(strlen(str_ref[i]) > 0){
            temp3 = (char *) malloc(sizeof(char)*strlen(str_ref[i]));
            memcpy(temp3, str_ref[i], strlen(str_ref[i]));
            schema->dataTypes[i] = DT_STRING;
            temp1 = strtok (temp3,"[");
            temp1 = strtok (NULL,"]");
            schema->typeLength[i] = strtol(temp1, &temp2, 10);
            free(temp3);
            free(str_ref[i]);
        }
    }

    if(keyFlag == 1){
        schema->keyAttrs=(int *)malloc(sizeof(int)*keySize);
        schema->keySize = keySize;
        for(i=0; i<keySize; ++i){
            for(j=0; j<numAttr; ++j){
                if(strcmp(keyAttrs[i], schema->attrNames[j]) == 0){
                    schema->keyAttrs[i] = j;
                    free(keyAttrs[i]);
                }
            }
        }
    }

    return schema;
}


RC tableInfoToFile(char *name, tableInfo *info){

    if(access(name, F_OK) == -1) {
        return RC_TABLE_NOT_FOUND;
    }

    SM_FileHandle fh;
    int status;

    if ((status=openPageFile(name, &fh)) != RC_OK){
        return status;
    }

    char *info_str = tableInfoToStr(info);
    if ((status=writeBlock(0, &fh, info_str)) != RC_OK){
        free(info_str);
        return status;
    }

    if ((status=closePageFile(&fh)) != RC_OK){
        free(info_str);
        return status;
    }

    free(info_str);
    return RC_OK;
}

// table and manager
extern RC initRecordManager (void *mgmtData)
{
    return RC_OK;
}
extern RC shutdownRecordManager (){
    return RC_OK;
}
extern RC createTable (char *name, Schema *schema){

    /* Check if table already exists*/

    if( access(name, F_OK) != -1 ) {
        return RC_TABLE_ALREADY_EXITS;
    }

    int status;
    SM_FileHandle fh;
    /* Create a file with the given name and create pages for info and schema*/
    if ((status=createPageFile(name)) != RC_OK)
        return status;

    int schema_size = getSchemaSize(schema);
    int slot_size = slotSize(schema);
    int file_size = (int) ceil((float)schema_size/PAGE_SIZE);
    int max_slots = (int) floor((float)PAGE_SIZE/(float)slot_size);

    if ((status=openPageFile(name, &fh)) != RC_OK)
        return status;

    if ((status=ensureCapacity((file_size + 1), &fh)) != RC_OK){
        return status;
    }
    /* First page with file info*/
    tableInfo *info = (tableInfo *) malloc(sizeof(tableInfo));
    info->numTuples = 0;
    info->schemaLength = schema_size;
    info->recordStartPage = file_size + 1;
    info->slotSize = slot_size;
    info->recordLastPage = file_size + 1;
    info->maxSlots = max_slots;
    info->tstone_root = NULL;

    char *info_str = tableInfoToStr(info);
    if ((status=writeBlock(0, &fh, info_str)) != RC_OK)
        return status;

    /* From next page, write the schema*/
    char *schema_str = serializeSchema(schema);
    if ((status=writeBlock(1, &fh, schema_str)) != RC_OK)
        return status;
    if ((status=closePageFile(&fh)) != RC_OK)
        return status;

    return RC_OK;
}

extern RC openTable (RM_TableData *rel, char *name){


    if(access(name, F_OK) == -1) {
        return RC_TABLE_NOT_FOUND;
    }

    BM_BufferPool *bm = (BM_BufferPool *)malloc(sizeof(BM_BufferPool));
    BM_PageHandle *page = (BM_PageHandle *)malloc(sizeof(BM_PageHandle));

    initBufferPool(bm, name, 3, RS_FIFO, NULL);
    pinPage(bm, page, 0);

    tableInfo *info = strToTableInfo(page->data);

    if(info->schemaLength < PAGE_SIZE){
        pinPage(bm, page, 1);
    }

    rel->schema = deserializeSchema(page->data);
    rel->name = name;

    info->bm = bm;
    rel->mgmtData = info;

    free(page);

    return RC_OK;
}

extern RC closeTable (RM_TableData *rel){

    shutdownBufferPool(((tableInfo *)rel->mgmtData)->bm);
    free(rel->mgmtData);

    free(rel->schema->dataTypes);

    free(rel->schema->attrNames);

    free(rel->schema->keyAttrs);
    free(rel->schema->typeLength);

    free(rel->schema);

    return RC_OK;
}

extern RC deleteTable (char *name){
    if(access(name, F_OK) == -1) {
        return RC_TABLE_NOT_FOUND;
    }

    if(remove(name) != 0){
        return RC_TABLE_NOT_FOUND;
    }
    return RC_OK;
}

extern int getNumTuples (RM_TableData *rel){
    return ((tableInfo *)rel->mgmtData)->numTuples;
}

// handling records in a table
extern RC insertRecord (RM_TableData *rel, Record *record){
    BM_PageHandle *page = (BM_PageHandle *)malloc(sizeof(BM_PageHandle));
    tableInfo *info = (tableInfo *) (rel->mgmtData);
    int page_num, slot;

    if (info->tstone_root != NULL){
        page_num = info->tstone_root->id.page;
        slot = info->tstone_root->id.slot;
        info->tstone_root = info->tstone_root->next;
    }
    else{

        page_num = info->recordLastPage;
        slot = info->numTuples - ((page_num - info->recordStartPage)*info->maxSlots) ;

        if (slot == info->maxSlots){
            slot = 0;
            page_num++;
        }
        info->recordLastPage = page_num;
    }
    record->id.page = page_num;
    record->id.slot = slot;

    char *record_str = serializeRecord(record, rel->schema);

    pinPage(info->bm, page, page_num);
    memcpy(page->data + (slot*info->slotSize), record_str, strlen(record_str));
    free(record_str);

    markDirty(info->bm, page);
    unpinPage(info->bm, page);
    forcePage(info->bm, page);

    record->id.tstone = false;
    (info->numTuples)++;
    tableInfoToFile(rel->name, info);
    free(page);
    return RC_OK;
}

extern RC deleteRecord (RM_TableData *rel, RID id){
    tableInfo *info = (tableInfo *) (rel->mgmtData);
    tNode *tstone_iter = info->tstone_root;
    if(info->numTuples>0){
        /* add deleted RID to end of tombstone list */
        if(info->tstone_root == NULL){
            info->tstone_root = (tNode *)malloc(sizeof(tNode));
            info->tstone_root->next = NULL;
            tstone_iter = info->tstone_root;
        }
        else{
            while (tstone_iter->next != NULL){
                tstone_iter = tstone_iter->next;
            }
            tstone_iter->next = (tNode *)malloc(sizeof(tNode));
            tstone_iter = tstone_iter->next;
            tstone_iter->next = NULL;
        }

        tstone_iter->id.page = id.page;
        tstone_iter->id.slot = id.slot;
        tstone_iter->id.tstone = TRUE;
        (info->numTuples)--;
        tableInfoToFile(rel->name, info);
    }
    else{
        return RC_WRITE_FAILED;     //temp error. will update later.
    }

    return RC_OK;

}
extern RC updateRecord (RM_TableData *rel, Record *record){
    BM_PageHandle *page = (BM_PageHandle *)malloc(sizeof(BM_PageHandle));
    tableInfo *info = (tableInfo *) (rel->mgmtData);
    int page_num, slot;

    page_num = record->id.page;
    slot = record->id.slot;

    char *record_str = serializeRecord(record, rel->schema);

    pinPage(info->bm, page, page_num);
    memcpy(page->data + (slot*info->slotSize), record_str, strlen(record_str));
    free(record_str);

    markDirty(info->bm, page);
    unpinPage(info->bm, page);
    forcePage(info->bm, page);

    tableInfoToFile(rel->name, info);

    return RC_OK;

}
extern RC getRecord (RM_TableData *rel, RID id, Record *record){
    tableInfo *info = (tableInfo *) (rel->mgmtData);
    BM_PageHandle *page = (BM_PageHandle *)malloc(sizeof(BM_PageHandle));

    int page_num, slot;
    page_num = id.page;
    slot = id.slot;

    record->id.page = page_num;
    record->id.slot = slot;
    record->id.tstone = 0;
    /* Check if tombstone*/

    tNode *root = info->tstone_root;
    int tombStoneFlag = 0;
    int tombStoneCount = 0;

    int k=0;

    for(k=0; k<info->tNodeLen; ++k){
        if (root->id.page == page_num && root->id.slot == slot){
            tombStoneFlag = 1;
            record->id.tstone = 1;
            break;
        }
        root = root->next;
        tombStoneCount++;
    }

    /* Read the page and slot*/

    if (tombStoneFlag == 0){
        int tupleNumber = (page_num - info->recordStartPage)*(info->maxSlots) + slot + 1 - tombStoneCount;
        if (tupleNumber > info->numTuples){
	    free(page);
            return RC_RM_NO_MORE_TUPLES;
        }

        pinPage(info->bm, page, page_num);
        char *record_str = (char *) malloc(sizeof(char) * info->slotSize);
        memcpy(record_str, page->data + ((slot)*info->slotSize), sizeof(char)*info->slotSize);
        unpinPage(info->bm, page);

        Record *temp_record = deserializeRecord(record_str, rel);

        record->data = temp_record->data;
        free(temp_record);
    }

    free(page);
    return RC_OK;
}

// scans
extern RC startScan (RM_TableData *rel, RM_ScanHandle *scan, Expr *cond){

	/* initialize the RM_ScanHandle data structure */
    scan->rel = rel;

	/* initialize rNode to store the information about the record to be searched and the condition to be evaluated */
    recordInfo *rNode = (recordInfo *) malloc(sizeof(recordInfo));
    rNode->curPage = ((tableInfo *)rel->mgmtData)->recordStartPage;
    rNode->curSlot = 0;
    rNode->cond = cond;
    rNode->numSlots = ((tableInfo *)rel->mgmtData)->maxSlots;
    rNode->numPages = ((tableInfo *)rel->mgmtData)->recordLastPage;

	/* assign rNode to scan->mgmtData */
    scan->mgmtData = (void *) rNode;

    return RC_OK;
}
extern RC next (RM_ScanHandle *scan, Record *record){

    recordInfo *rNode;
    Value *value;
    rNode = scan->mgmtData;
    RC status;


    record->id.slot = rNode->curSlot;
    record->id.page = rNode->curPage;

	/* fetch the record as per the page and slot id */
    status = getRecord(scan->rel, record->id, record);

    if(status == RC_RM_NO_MORE_TUPLES){

        return RC_RM_NO_MORE_TUPLES;
    }

	/* check tombstone id for a deleted record and update record node parameters accordingly */
    else if(record->id.tstone == 1){
        if (rNode->curSlot == rNode->numSlots - 1){
            rNode->curSlot = 0;
            (rNode->curPage)++;
        }
        else{
            (rNode->curSlot)++;
        }
        scan->mgmtData = rNode;
        return next(scan, record);
    }
	/* evaluate the given expression to check if the record is the one required by the client */
    else{
        evalExpr(record, scan->rel->schema, rNode->cond, &value);
        if (rNode->curSlot == rNode->numSlots - 1){
            rNode->curSlot = 0;
            (rNode->curPage)++;
        }
        else{
            (rNode->curSlot)++;
        }
        scan->mgmtData = rNode;

	/* If the record fetched is not the required one then call the next function with the updated record id parameters. */
        if(value->v.boolV != 1){
            return next(scan, record);
        }
        else{
            return RC_OK;
        }
    }

}
extern RC closeScan (RM_ScanHandle *scan){
    //free(scan);
    return RC_OK;
}

// dealing with schemas
extern int getRecordSize (Schema *schema){
    int size = 0, tempSize = 0, i;

    for(i=0; i<schema->numAttr; ++i){
        switch (schema->dataTypes[i]){
            case DT_STRING:
                tempSize = schema->typeLength[i];
                break;
            case DT_INT:
                tempSize = sizeof(int);
                break;
            case DT_FLOAT:
                tempSize = sizeof(float);
                break;
            case DT_BOOL:
                tempSize = sizeof(bool);
                break;
            default:
                break;
        }
        size += tempSize;
    }
    return size;
}
extern Schema *createSchema (int numAttr, char **attrNames, DataType *dataTypes, int *typeLength, int keySize, int *keys){

    Schema *schema = (Schema *) malloc(sizeof(Schema));

    schema->numAttr = numAttr;
    schema->attrNames = attrNames;
    schema->dataTypes = dataTypes;
    schema->typeLength = typeLength;
    schema->keySize = keySize;
    schema->keyAttrs = keys;

    return schema;
}
extern RC freeSchema (Schema *schema){
    free(schema);
    return RC_OK;
}

// dealing with records and attribute values
extern RC createRecord (Record **record, Schema *schema){

	/* allocate memory for a new record and record data as per the schema. */
    *record = (Record *)  malloc(sizeof(Record));
    (*record)->data = (char *)malloc((getRecordSize(schema)));

    return RC_OK;
}
extern RC freeRecord (Record *record){
	 /* free the memory space allocated to record and its data */
    free(record->data);
    free(record);

    return RC_OK;
}
extern RC getAttr (Record *record, Schema *schema, int attrNum, Value **value){

	/* allocate the space to the value data structre where the attribute values are to be fetched */
    *value = (Value *)  malloc(sizeof(Value));
    int offset; char *attrData;

	/* get the offset value of different attributes as per the attribute numbers */
    attrOffset(schema, attrNum, &offset);
    attrData = (record->data + offset);
    (*value)->dt = schema->dataTypes[attrNum];

	/* attribute data is assigned to the value pointer as per the different data types */
    switch(schema->dataTypes[attrNum])
    {
        case DT_INT:
        {
            memcpy(&((*value)->v.intV),attrData, sizeof(int));
        }
            break;
        case DT_STRING:
        {
            int len = schema->typeLength[attrNum];
            char *stringV;
            stringV = (char *) malloc(len + 1);
            strncpy(stringV, attrData, len);
            stringV[len] = '\0';
            //MAKE_STRING_VALUE(*value, stringV);
            //(*value)->v.stringV = (char *) malloc(len);
            //strncpy((*value)->v.stringV, stringV, len);
            (*value)->v.stringV = stringV;
            //free(stringV);
        }
            break;
        case DT_FLOAT:
        {
            memcpy(&((*value)->v.floatV),attrData, sizeof(float));
        }
            break;
        case DT_BOOL:
        {
            memcpy(&((*value)->v.boolV),attrData, sizeof(bool));
        }
            break;
    }

    return RC_OK;

}

extern RC setAttr (Record *record, Schema *schema, int attrNum, Value *value){
    int offset; char * attrData;

	/* get the offset value of different attributes as  per the attribute numbers */
    attrOffset(schema, attrNum, &offset);
    attrData = record->data + offset;

     /* set attribute values as per the attributes datatype */
    switch(schema->dataTypes[attrNum])
    {
        case DT_INT:
        {
            memcpy(attrData,&(value->v.intV), sizeof(int));
        }
            break;
        case DT_STRING:
        {
            char *stringV;
            int len = schema->typeLength[attrNum];
            stringV = (char *) malloc(len);
            stringV = value->v.stringV;
            memcpy(attrData,(stringV), len);
        }
            break;
        case DT_FLOAT:
        {
            memcpy(attrData,&((value->v.floatV)), sizeof(float));
        }
            break;
        case DT_BOOL:
        {
            memcpy(attrData,&((value->v.boolV)), sizeof(bool));
        }
            break;
    }

    return RC_OK;
}
