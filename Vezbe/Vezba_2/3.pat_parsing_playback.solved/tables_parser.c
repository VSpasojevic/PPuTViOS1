#include "tables.h"

ParseErrorCode parsePatHeader(const uint8_t* patHeaderBuffer, PatHeader* patHeader)
{    
    if(patHeaderBuffer==NULL || patHeader==NULL)
    {
        printf("\n%s : ERROR received parameters are not ok\n", __FUNCTION__);
        return TABLES_PARSE_ERROR;
    }

    patHeader->tableId = (uint8_t)* patHeaderBuffer; 
    if (patHeader->tableId != 0x00)
    {
        printf("\n%s : ERROR it is not a PAT Table\n", __FUNCTION__);
        return TABLES_PARSE_ERROR;
    }
    
    uint8_t lower8Bits = 0;
    uint8_t higher8Bits = 0;
    uint16_t all16Bits = 0;
    
    lower8Bits = (uint8_t)(*(patHeaderBuffer + 1));
    lower8Bits = lower8Bits >> 7;
    patHeader->sectionSyntaxIndicator = lower8Bits & 0x01;

    higher8Bits = (uint8_t) (*(patHeaderBuffer + 1));
    lower8Bits = (uint8_t) (*(patHeaderBuffer + 2));
    all16Bits = (uint16_t) ((higher8Bits << 8) + lower8Bits);
    patHeader->sectionLength = all16Bits & 0x0FFF;
    
    higher8Bits = (uint8_t) (*(patHeaderBuffer + 3));
    lower8Bits = (uint8_t) (*(patHeaderBuffer + 4));
    all16Bits = (uint16_t) ((higher8Bits << 8) + lower8Bits);
    patHeader->transportStreamId = all16Bits & 0xFFFF;
    
    lower8Bits = (uint8_t) (*(patHeaderBuffer + 5));
    lower8Bits = lower8Bits >> 1;
    patHeader->versionNumber = lower8Bits & 0x1F;

    lower8Bits = (uint8_t) (*(patHeaderBuffer + 5));
    patHeader->currentNextIndicator = lower8Bits & 0x01;

    lower8Bits = (uint8_t) (*(patHeaderBuffer + 6));
    patHeader->sectionNumber = lower8Bits & 0xFF;

    lower8Bits = (uint8_t) (*(patHeaderBuffer + 7));
    patHeader->lastSectionNumber = lower8Bits & 0xFF;

    return TABLES_PARSE_OK;
}

ParseErrorCode parsePatServiceInfo(const uint8_t* patServiceInfoBuffer, PatServiceInfo* patServiceInfo)
{
    if(patServiceInfoBuffer==NULL || patServiceInfo==NULL)
    {
        printf("\n%s : ERROR received parameters are not ok\n", __FUNCTION__);
        return TABLES_PARSE_ERROR;
    }
    
    uint8_t lower8Bits = 0;
    uint8_t higher8Bits = 0;
    uint16_t all16Bits = 0;

    higher8Bits = (uint8_t) (*(patServiceInfoBuffer));
    lower8Bits = (uint8_t) (*(patServiceInfoBuffer + 1));
    all16Bits = (uint16_t) ((higher8Bits << 8) + lower8Bits);
    patServiceInfo->programNumber = all16Bits & 0xFFFF; 

    higher8Bits = (uint8_t) (*(patServiceInfoBuffer + 2));
    lower8Bits = (uint8_t) (*(patServiceInfoBuffer + 3));
    all16Bits = (uint16_t) ((higher8Bits << 8) + lower8Bits);
    patServiceInfo->pid = all16Bits & 0x1FFF;
    
    return TABLES_PARSE_OK;
}

ParseErrorCode parsePatTable(const uint8_t* patSectionBuffer, PatTable* patTable)
{
    uint8_t * currentBufferPosition = NULL;
    uint32_t parsedLength = 0;
    
    if(patSectionBuffer==NULL || patTable==NULL)
    {
        printf("\n%s : ERROR received parameters are not ok\n", __FUNCTION__);
        return TABLES_PARSE_ERROR;
    }
    
    if(parsePatHeader(patSectionBuffer,&(patTable->patHeader))!=TABLES_PARSE_OK)
    {
        printf("\n%s : ERROR parsing PAT header\n", __FUNCTION__);
        return TABLES_PARSE_ERROR;
    }
    
    parsedLength = 12 /*PAT header size*/ - 3 /*Not in section length*/;
    currentBufferPosition = (uint8_t *)(patSectionBuffer + 8); /* Position after last_section_number */
    patTable->serviceInfoCount = 0; /* Number of services info presented in PAT table */
    
    while(parsedLength < patTable->patHeader.sectionLength)
    {
        if(patTable->serviceInfoCount > TABLES_MAX_NUMBER_OF_PIDS_IN_PAT - 1)
        {
            printf("\n%s : ERROR there is not enough space in PAT structure for Service info\n", __FUNCTION__);
            return TABLES_PARSE_ERROR;
        }
        
        if(parsePatServiceInfo(currentBufferPosition, &(patTable->patServiceInfoArray[patTable->serviceInfoCount])) == TABLES_PARSE_OK)
        {
            currentBufferPosition += 4; /* Size from program_number to pid */
            parsedLength += 4; /* Size from program_number to pid */
            patTable->serviceInfoCount ++;
        }    
    }
    
    return TABLES_PARSE_OK;
}

ParseErrorCode printPatTable(PatTable* patTable)
{
    uint8_t i=0;
    
    if(patTable==NULL)
    {
        printf("\n%s : ERROR received parameter is not ok\n", __FUNCTION__);
        return TABLES_PARSE_ERROR;
    }
    
    printf("\n********************PAT TABLE SECTION********************\n");
    printf("table_id                 |      %d\n",patTable->patHeader.tableId);
    printf("section_length           |      %d\n",patTable->patHeader.sectionLength);
    printf("transport_stream_id      |      %d\n",patTable->patHeader.transportStreamId);
    printf("section_number           |      %d\n",patTable->patHeader.sectionNumber);
    printf("last_section_number      |      %d\n",patTable->patHeader.lastSectionNumber);
    
    for (i=0; i<patTable->serviceInfoCount;i++)
    {
        printf("-----------------------------------------\n");
        printf("program_number           |      %d\n",patTable->patServiceInfoArray[i].programNumber);
        printf("pid                      |      %d\n",patTable->patServiceInfoArray[i].pid); 
    }
    printf("\n********************PAT TABLE SECTION********************\n");
    
    return TABLES_PARSE_OK;
}

