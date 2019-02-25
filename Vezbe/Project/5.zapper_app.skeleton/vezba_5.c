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





int main(int agrc,char** argv)
{
	char datName[10];
	memset(datName,0,10);
	FILE* file;
	char str[999];
	int argCounter = 0;

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
		case KEYCODE_EXIT:
			printf("\nExit pressed\n");
            pthread_mutex_lock(&deinitMutex);
		    pthread_cond_signal(&deinitCond);
		    pthread_mutex_unlock(&deinitMutex);
			break;
		default:
			printf("\nPress P+, P-, info or exit! \n\n");
	}
}

