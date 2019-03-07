#include "remote_controller.h"
#include "stream_controller.h"

static inline void textColor(int32_t attr, int32_t fg, int32_t bg)
{
   char command[13];

   /* command is the control command to the terminal */
   sprintf(command, "%c[%d;%d;%dm", 0x1B, attr, fg + 30, bg + 40);
   printf("%s", command);
}

/* macro function for error checking */
#define ERRORCHECK(x)                                                       \
{                                                                           \
if (x != 0)                                                                 \
 {                                                                          \
    textColor(1,1,0);                                                       \
    printf(" Error!\n File: %s \t Line: <%d>\n", __FILE__, __LINE__);       \
    textColor(0,7,0);                                                       \
    return -1;                                                              \
 }                                                                          \
}

static void remoteControllerCallback(uint16_t code, uint16_t type, uint32_t value);
static pthread_cond_t deinitCond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t deinitMutex = PTHREAD_MUTEX_INITIALIZER;
static ChannelInfo channelInfo;



static argStruct* confStruct;

static uint8_t channelFlag;



int main(int argc,char** argv)
{
	char datName[10];
	memset(datName,0,10);
	FILE* file;
	char str[999];
	int argCounter = 0;

	if(argc != 2){
		printf("Not enough parametars in command line!!!\nPut program and configuration file\n");
		return 0;

	}



	//alocate memory for argument structure
	
	confStruct = (argStruct*) malloc(sizeof(argStruct));

	if(confStruct==NULL)
    	{
		printf("\n%s : ERROR Cannot allocate memory\n", __FUNCTION__);
        return -1;
	} 

	//read arguments form command line
	strcpy(datName,argv[1]);

	printf("\nime datoteke new : %s\n",datName);

	file = fopen(datName, "r");
	
	if(file == NULL){
		printf("ERROR in open file %s\n",datName);	
		return 0;
	} 	
	
	printf("File is open\n");	

	if (file) {
	    while (fscanf(file, "%s", str)!=EOF){

		switch(argCounter){
			case 0: confStruct->frequency = atoi(str);
				printf("Frequence: %d\n",confStruct->frequency);
				break;
			
			case 1:	confStruct->bandwidth = atoi(str);
				printf("Bandwidth: %d\n",confStruct->bandwidth);
				break;

			case 2:	if(strcmp(str,"DVB_T") == 0){
					confStruct->modul = 0;
				}
				else{
					confStruct->modul = 1;
				}
				printf("Module: %d\n",confStruct->modul);
				break;
			case 3:	confStruct->programNumber = atoi(str);
				printf("Program Number: %d\n",confStruct->programNumber);
				channelFlag = confStruct->programNumber;
				break;
			
			case 4:	confStruct->videoPid = atoi(str);
				printf("Video pid: %d\n",confStruct->videoPid);
				break;
			case 5:	confStruct->audioPid = atoi(str);
				printf("Audio pid: %d\n",confStruct->audioPid);
				break;

			case 6:	if(strcmp(str,"VIDEO_TYPE_MPEG2") == 0){
					confStruct->videoType = 42;
				}
				if(strcmp(str,"VIDEO_TYPE_H264") == 0){
					confStruct->videoType = 39;
				}
				
				if(strcmp(str,"VIDEO_TYPE_MPEG1") == 0){
					confStruct->videoType = 43;
				}
				if(strcmp(str,"VIDEO_TYPE_MPEG4") == 0){
					confStruct->videoType = 44;
				}else{
				printf("Bad video type\n");

				}
				
				printf("Video type: %d\n",confStruct->videoType);
				break;
			case 7:	if(strcmp(str,"AUDIO_TYPE_MPEG_AUDIO") == 0){
					confStruct->audioType = 10;
				}
				if(strcmp(str,"AUDIO_TYPE_MP3") == 0){
					confStruct->audioType = 11;
				}
				if(strcmp(str,"AUDIO_TYPE_HE_AAC") == 0){
					confStruct->audioType = 12;
				}
				if(strcmp(str,"AUDIO_TYPE_DOLBY_AC3") == 0){
					confStruct->audioType = 1;
				}
				printf("Audio type: %d\n",confStruct->audioType);
				break;
			
			default: printf("Bad argument\n");
				break;
	

		}


		argCounter++;
		
		}
	    fclose(file);
	}

    /* initialize remote controller module */
    ERRORCHECK(remoteControllerInit());
    
    /* register remote controller callback */
    ERRORCHECK(registerRemoteControllerCallback(remoteControllerCallback));
    
    /* initialize stream controller module */
    ERRORCHECK(streamControllerInit(confStruct));

    /* wait for a EXIT remote controller key press event */
    pthread_mutex_lock(&deinitMutex);
	if (ETIMEDOUT == pthread_cond_wait(&deinitCond, &deinitMutex))
	{
		printf("\n%s : ERROR Lock timeout exceeded!\n", __FUNCTION__);
	}
	pthread_mutex_unlock(&deinitMutex);
    
    /* unregister remote controller callback */
    ERRORCHECK(unregisterRemoteControllerCallback(remoteControllerCallback));

    /* deinitialize remote controller module */
    ERRORCHECK(remoteControllerDeinit());

    /* deinitialize stream controller module */
    ERRORCHECK(streamControllerDeinit());
	
	free(confStruct);
  
    return 0;
}

void remoteControllerCallback(uint16_t code, uint16_t type, uint32_t value)
{
    switch(code)
	{
		case KEYCODE_INFO:
            printf("\nInfo pressed\n");     
		info();     
            if (getChannelInfo(&channelInfo) == SC_NO_ERROR)
            {
                printf("\n********************* Channel info *********************\n");
                printf("Program number: %d\n", channelInfo.programNumber);
                printf("Audio pid: %d\n", channelInfo.audioPid);
                printf("Video pid: %d\n", channelInfo.videoPid);
                printf("**********************************************************\n");
            }
			break;
		case KEYCODE_P_PLUS:
			printf("\nCH+ pressed\n");
            		channelUp();
			break;
		case KEYCODE_P_MINUS:
		        printf("\nCH- pressed\n");
            		channelDown();
			break;
		case KEYCODE_VOLUME_PLUS:
		        printf("\nVOLUME+ pressed\n");
            		volumeUp();
			break;
		case KEYCODE_VOLUME_MINUS:
		        printf("\nVOLUME- pressed\n");
            		volumeDown();
			break;
		case KEYCODE_CH1:
		        printf("\nCH1 pressed\n");
				if(channelFlag != 1){
					channelFlag = 1;
					channel(KEYCODE_CH1 - 2);
				}           		
			break;
		case KEYCODE_CH2:
		        printf("\nCH2 pressed\n");
            	if(channelFlag != 2){
					channelFlag = 2;
					channel(KEYCODE_CH2 - 2);
				}     
			break;
		case KEYCODE_CH3:
		        printf("\nCH3 pressed\n");
            	if(channelFlag != 3){
					channelFlag = 3;
					channel(KEYCODE_CH3 - 2);
				}     
			break;
		case KEYCODE_CH4:
		        printf("\nCH4 pressed\n");
            	if(channelFlag != 4){
					channelFlag = 4;
					channel(KEYCODE_CH4 - 2);
				}
			break;
		case KEYCODE_CH5:
		        printf("\nCH5 pressed\n");
            	if(channelFlag != 5){
					channelFlag = 5;
					channel(KEYCODE_CH5 - 2);
				}
			break;
		case KEYCODE_CH6:
		        printf("\nCH6 pressed\n");
            	if(channelFlag != 6){
					channelFlag = 6;
					channel(KEYCODE_CH6 - 2);
				}     
			break;
		case KEYCODE_CH7:
		        printf("\nCH7 pressed\n");
            	if(channelFlag != 7){
					channelFlag = 7;
					channel(KEYCODE_CH7 - 2);
				}
			break;		
		case KEYCODE_MUTE:
		        printf("\nMUTE pressed\n");
            		mute();
			break;
		case KEYCODE_EPG:
		        printf("\nEPG pressed\n");
            		epg();
			break;
		case KEYCODE_EXIT:
			printf("\nExit pressed\n");
            pthread_mutex_lock(&deinitMutex);
		    pthread_cond_signal(&deinitCond);
		    pthread_mutex_unlock(&deinitMutex);
			break;
		default:
			printf("\nPress P+, P-,CH1,CH2,CH3,CH4,CH5,CH6,CH7, info or exit! \n\n");
	}
}

