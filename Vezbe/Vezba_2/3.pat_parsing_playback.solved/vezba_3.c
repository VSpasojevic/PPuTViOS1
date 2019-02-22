#include "tdp_api.h"
#include "tables.h"
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>

#define DESIRED_FREQUENCY 754000000	        /* Tune frequency in Hz */
#define BANDWIDTH 8    				/* Bandwidth in Mhz */
#define VIDEO_PID 101				/* Channel video pid */
#define AUDIO_PID 103				/* Channel audio pid */

static PatTable *patTable;
static pthread_cond_t statusCondition = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t statusMutex = PTHREAD_MUTEX_INITIALIZER;

static int32_t sectionReceivedCallback(uint8_t *buffer);
static int32_t tunerStatusCallback(t_LockStatus status);

int main()
{
    uint32_t playerHandle = 0;
    uint32_t sourceHandle = 0;
    uint32_t streamHandleA = 0;
    uint32_t streamHandleV = 0;
    uint32_t filterHandle = 0;
	struct timespec lockStatusWaitTime;
	struct timeval now;
    
    gettimeofday(&now,NULL);
    lockStatusWaitTime.tv_sec = now.tv_sec+10;
     
    /*Allocate memory for PAT table section*/
    patTable=(PatTable*)malloc(sizeof(PatTable));
    if(patTable==NULL)
    {
		printf("\n%s : ERROR Cannot allocate memory\n", __FUNCTION__);
        return -1;
	}  
    memset(patTable, 0x0, sizeof(PatTable));
       
    /*Initialize tuner device*/
    if(Tuner_Init())
    {
        printf("\n%s : ERROR Tuner_Init() fail\n", __FUNCTION__);
        free(patTable);
        return -1;
    }
    
    /* Register tuner status callback */
    if(Tuner_Register_Status_Callback(tunerStatusCallback))
    {
		printf("\n%s : ERROR Tuner_Register_Status_Callback() fail\n", __FUNCTION__);
	}
    
    /*Lock to frequency*/
    if(!Tuner_Lock_To_Frequency(DESIRED_FREQUENCY, BANDWIDTH, DVB_T))
    {
        printf("\n%s: INFO Tuner_Lock_To_Frequency(): %d Hz - success!\n",__FUNCTION__,DESIRED_FREQUENCY);
    }
    else
    {
        printf("\n%s: ERROR Tuner_Lock_To_Frequency(): %d Hz - fail!\n",__FUNCTION__,DESIRED_FREQUENCY);
        free(patTable);
        Tuner_Deinit();
        return -1;
    }
    
    /* Wait for tuner to lock*/
    pthread_mutex_lock(&statusMutex);
    if(ETIMEDOUT == pthread_cond_timedwait(&statusCondition, &statusMutex, &lockStatusWaitTime))
    {
        printf("\n%s:ERROR Lock timeout exceeded!\n",__FUNCTION__);
        free(patTable);
        Tuner_Deinit();
        return -1;
    }
    pthread_mutex_unlock(&statusMutex);
   
    /*Initialize player*/
    if(Player_Init(&playerHandle))
    {
		printf("\n%s : ERROR Player_Init() fail\n", __FUNCTION__);
		free(patTable);
        Tuner_Deinit();
        return -1;
	}
	
	/*Open source*/
	if(Player_Source_Open(playerHandle, &sourceHandle))
    {
		printf("\n%s : ERROR Player_Source_Open() fail\n", __FUNCTION__);
		free(patTable);
		Player_Deinit(playerHandle);
        Tuner_Deinit();
        return -1;	
	}

	/*Set PAT pid and tableID to demultiplexer*/
	if(Demux_Set_Filter(playerHandle, 0x00, 0x00, &filterHandle))
	{
		printf("\n%s : ERROR Demux_Set_Filter() fail\n", __FUNCTION__);
	}	
	/*Register section filter callback*/
    if(Demux_Register_Section_Filter_Callback(sectionReceivedCallback))
    {
		printf("\n%s : ERROR Demux_Register_Section_Filter_Callback() fail\n", __FUNCTION__);
	}

	/*Create video stream*/
    if(Player_Stream_Create(playerHandle, sourceHandle, VIDEO_PID, VIDEO_TYPE_MPEG2, &streamHandleV))
    {
        printf("\n%s : ERROR Cannot create video stream\n", __FUNCTION__);
    }
	/*Create audio stream*/
    if(Player_Stream_Create(playerHandle, sourceHandle, AUDIO_PID, AUDIO_TYPE_MPEG_AUDIO, &streamHandleA))
    {
        printf("\n%s : ERROR Cannot create audio stream\n", __FUNCTION__);
    }
    
	/* Wait for a while */
    fflush(stdin);
    getchar();
    
    /*Free demux filter*/  
    Demux_Free_Filter(playerHandle, filterHandle);

	/*Remove audio stream*/  
	Player_Stream_Remove(playerHandle, sourceHandle, streamHandleA);
    
    /*Remove video stream*/
    Player_Stream_Remove(playerHandle, sourceHandle, streamHandleV);
    
    /*Close player source*/
    Player_Source_Close(playerHandle, sourceHandle);
    
    /*Deinitialize player*/
    Player_Deinit(playerHandle);
    
    /*Deinitialize tuner device*/
    Tuner_Deinit();
    
    /*Free allocated memory*/  
    free(patTable);
    
    return 0;
}

int32_t sectionReceivedCallback(uint8_t *buffer)
{
    uint8_t tableId = *buffer;  
    if(tableId==0x00)
    {
        printf("\n%s -----PAT TABLE ARRIVED-----\n",__FUNCTION__);
        
        if(parsePatTable(buffer,patTable)==TABLES_PARSE_OK)
        {
            printPatTable(patTable);
        }
    }
    return 0;
}

int32_t tunerStatusCallback(t_LockStatus status)
{
    if(status == STATUS_LOCKED)
    {
        pthread_mutex_lock(&statusMutex);
        pthread_cond_signal(&statusCondition);
        pthread_mutex_unlock(&statusMutex);
        printf("\n%s -----TUNER LOCKED-----\n",__FUNCTION__);
    }
    else
    {
        printf("\n%s -----TUNER NOT LOCKED-----\n",__FUNCTION__);
    }
    return 0;
}


