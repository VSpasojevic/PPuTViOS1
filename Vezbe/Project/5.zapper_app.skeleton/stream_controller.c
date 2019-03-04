#include "stream_controller.h"
#include <directfb.h>
#include <signal.h>
#include <time.h>



/* helper macro for error checking */
#define DFBCHECK(x...)                                      \
{                                                           \
DFBResult err = x;                                          \
                                                            \
if (err != DFB_OK)                                          \
  {                                                         \
    fprintf( stderr, "%s <%d>:\n\t", __FILE__, __LINE__ );  \
    DirectFBErrorFatal( #x, err );                          \
  }                                                         \
}

#define FONT_HEIGHT 40

static PatTable *patTable;
static PmtTable *pmtTable;
static pthread_cond_t statusCondition = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t statusMutex = PTHREAD_MUTEX_INITIALIZER;

static int32_t sectionReceivedCallback(uint8_t *buffer);
static int32_t tunerStatusCallback(t_LockStatus status);

static uint32_t playerHandle = 0;
static uint32_t sourceHandle = 0;
static uint32_t streamHandleA = 0;
static uint32_t streamHandleV = 0;
static uint32_t filterHandle = 0;
static uint8_t threadExit = 0;
static bool changeChannel = false;
static int16_t programNumber = 0;
static ChannelInfo currentChannel;
static bool isInitialized = false;

static uint16_t argVideoPid;
static uint16_t argAudioPid;

tStreamType argVideoType;
tStreamType argAudioType;

uint8_t firstV = 0;
uint8_t firstA = 0;

static timer_t timerId;
static timer_t timerVolId;

static IDirectFBSurface *primary = NULL;
IDirectFB *dfbInterface = NULL;
static int32_t screenWidth = 0;
static int32_t screenHeight = 0;

static DFBRegion programInfoBannerRegion;
static DFBRegion programVolumeRegion;

bool txt = false;
int32_t ret;


static uint32_t volumeNumber = 0;


static struct itimerspec timerSpec;
static struct itimerspec timerSpecOld;

static struct itimerspec timerSpecVol;
static struct itimerspec timerSpecOldVol;

static struct timespec lockStatusWaitTime;
static struct timeval now;
static pthread_t scThread;
static pthread_t drawThread;
static pthread_t drawVolThread;

static pthread_cond_t demuxCond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t demuxMutex = PTHREAD_MUTEX_INITIALIZER;

static void* streamControllerTask(argStruct* arg_struct);
static StreamControllerError startChannel(int32_t channelNumber);
static char keycodeString[10];



void wipeVolScreen(){


	if(currentChannel.videoPid == -1){

	 /* clear screen */
   	 DFBCHECK(primary->SetColor(primary, 0x00, 0x00, 0x00, 0x00));
    	 DFBCHECK(primary->FillRectangle(primary, 0, 0, screenWidth, screenHeight));


	//set for region
	    DFBCHECK(primary->SetColor(primary, 0x00, 0x00, 0x00, 0xff));
	    DFBCHECK(primary->FillRectangle(primary, screenWidth/10, screenHeight/10, screenWidth/10 + 200, screenHeight/10 +200));
	    


	programVolumeRegion.x1 = screenWidth/10;
	programVolumeRegion.y1 = screenHeight/10; 
	programVolumeRegion.x2 = screenWidth/10 + 200;
	programVolumeRegion.y2 = screenHeight/10 + 200;

	    /* update screen */
	    DFBCHECK(primary->Flip(primary, &programVolumeRegion, 0));
	    
	    /* stop the timer */
	    memset(&timerSpecVol,0,sizeof(timerSpecVol));
	    ret = timer_settime(timerVolId,0,&timerSpecVol,&timerSpecOldVol);
	    if(ret == -1){
		printf("Error setting timer in %s!\n", __FUNCTION__);
	    }


	printf("RADIO WIPE VOL\n");



	}else{

 	/* clear screen */
	//set for region
	    DFBCHECK(primary->SetColor(primary, 0x00, 0x00, 0x00, 0x00));
	    DFBCHECK(primary->FillRectangle(primary, screenWidth/10, screenHeight/10, screenWidth/10 + 200, screenHeight/10 +200));
	    


	programVolumeRegion.x1 = screenWidth/10;
	programVolumeRegion.y1 = screenHeight/10; 
	programVolumeRegion.x2 = screenWidth/10 + 200;
	programVolumeRegion.y2 = screenHeight/10 + 200;

	    /* update screen */
	    DFBCHECK(primary->Flip(primary, &programVolumeRegion, 0));
	    
	    /* stop the timer */
	    memset(&timerSpecVol,0,sizeof(timerSpecVol));
	    ret = timer_settime(timerVolId,0,&timerSpecVol,&timerSpecOldVol);
	    if(ret == -1){
		printf("Error setting timer in %s!\n", __FUNCTION__);
	    }
		printf("VIDEO WIPE VOL\n");
	}

}


void wipeScreen(/*union sigval signalArg*/){
    int32_t ret;

    if(currentChannel.videoPid == -1){
	printf("VIDEO PID -1 u WIPE\n");
	    /* clear screen */
	    DFBCHECK(primary->SetColor(primary, 0x00, 0x00, 0x00, 0xff));
	    DFBCHECK(primary->FillRectangle(primary, 0, screenHeight*5/6, screenWidth, screenHeight/6));

	    /* generate keycode string for channel*/
		/*sprintf(keycodeString,"%s","RADIO ");

	DFBCHECK(primary->SetColor(primary, 0xff, 0xff, 0xff, 0x55));
	DFBCHECK(primary->DrawString(primary, keycodeString, -1, screenWidth/3, screenHeight/6, DSTF_CENTER));
*/
	programInfoBannerRegion.x1 = 0;
	programInfoBannerRegion.y1 = screenHeight*5/6; 
	programInfoBannerRegion.x2 = screenWidth;
	programInfoBannerRegion.y2 = screenHeight;

	    /* update screen */
	    DFBCHECK(primary->Flip(primary, &programInfoBannerRegion, 0));
	    
	    /* stop the timer */
	    memset(&timerSpec,0,sizeof(timerSpec));
	    ret = timer_settime(timerId,0,&timerSpec,&timerSpecOld);
	    if(ret == -1){
		printf("Error setting timer in %s!\n", __FUNCTION__);
	    }

    }
    else{
	    /* clear screen */
	    DFBCHECK(primary->SetColor(primary, 0x00, 0x00, 0x00, 0x00));
	    DFBCHECK(primary->FillRectangle(primary, 0, screenHeight*5/6, screenWidth, screenHeight/6));
	    


	programInfoBannerRegion.x1 = 0;
	programInfoBannerRegion.y1 = screenHeight*5/6; 
	programInfoBannerRegion.x2 = screenWidth;
	programInfoBannerRegion.y2 = screenHeight;

	    /* update screen */
	    DFBCHECK(primary->Flip(primary, &programInfoBannerRegion, 0));
	    
	    /* stop the timer */
	    memset(&timerSpec,0,sizeof(timerSpec));
	    ret = timer_settime(timerId,0,&timerSpec,&timerSpecOld);
	    if(ret == -1){
		printf("Error setting timer in %s!\n", __FUNCTION__);
	    }
	}
}




uint16_t initRb(){
	DFBSurfaceDescription surfaceDesc;
	DFBFontDescription fontDesc;
	IDirectFBFont *fontInterface = NULL;
	//0, screenHeight*5/6, screenWidth, screenHeight/6

	
	

	/* structure for timer specification */
        struct sigevent signalEvent;
        struct sigevent signalEventVol;


	int32_t ret;
	
	/* initialize DirectFB */
    
	DFBCHECK(DirectFBInit(NULL, NULL));
	DFBCHECK(DirectFBCreate(&dfbInterface));
	DFBCHECK(dfbInterface->SetCooperativeLevel(dfbInterface, DFSCL_FULLSCREEN));
	printf("\n/* initialize DirectFB */\n");
	
    	/* create primary surface with double buffering enabled */
    
	surfaceDesc.flags = DSDESC_CAPS;
	surfaceDesc.caps = DSCAPS_PRIMARY | DSCAPS_FLIPPING;
	DFBCHECK (dfbInterface->CreateSurface(dfbInterface, &surfaceDesc, &primary));
	printf("\n/* create primary surface with double buffering enabled */\n");
    
    
    	/* fetch the screen size */
    	DFBCHECK (primary->GetSize(primary, &screenWidth, &screenHeight));
	printf("\n/* fetch the screen size */\n");
	printf("\nSirina ekrana: %d, Visina ekrana: %d \n",screenWidth, screenHeight);
    
       	/* draw keycode */
    
	fontDesc.flags = DFDESC_HEIGHT;
	fontDesc.height = FONT_HEIGHT;
	
	DFBCHECK(dfbInterface->CreateFont(dfbInterface, "/home/galois/fonts/DejaVuSans.ttf", &fontDesc, &fontInterface));
	DFBCHECK(primary->SetFont(primary, fontInterface));

	/* create timer */
    signalEvent.sigev_notify = SIGEV_THREAD; /* tell the OS to notify you about timer by calling the specified function */
    signalEvent.sigev_notify_function = wipeScreen; /* function to be called when timer runs out */
    signalEvent.sigev_value.sival_ptr = NULL; /* thread arguments */
    signalEvent.sigev_notify_attributes = NULL; /* thread attributes (e.g. thread stack size) - if NULL default attributes are applied */
    ret = timer_create(/*clock for time measuring*/CLOCK_REALTIME,
                       /*timer settings*/&signalEvent,
                       /*where to store the ID of the newly created timer*/&timerId);

	printf("\n/* create timer %d */\n",ret);
	


    if(ret == -1){
        printf("Error creating timer, abort!\n");
        primary->Release(primary);
        dfbInterface->Release(dfbInterface);
        
        return 0;
    }

/* create timer2 */
    signalEventVol.sigev_notify = SIGEV_THREAD; /* tell the OS to notify you about timer by calling the specified function */
    signalEventVol.sigev_notify_function = wipeVolScreen; /* function to be called when timer runs out */
    signalEventVol.sigev_value.sival_ptr = NULL; /* thread arguments */
    signalEventVol.sigev_notify_attributes = NULL; /* thread attributes (e.g. thread stack size) - if NULL default attributes are applied */
    ret = timer_create(/*clock for time measuring*/CLOCK_REALTIME,
                       /*timer settings*/&signalEventVol,
                       /*where to store the ID of the newly created timer*/&timerVolId);

	printf("\n/* create timer2 %d */\n",ret);
	


    if(ret == -1){
        printf("Error creating timer, abort!\n");
        primary->Release(primary);
        dfbInterface->Release(dfbInterface);
        
        return 0;
    }


}

void deinitRB(){
	timer_delete(timerId);

	timer_delete(timerVolId);
	primary->Release(primary);
	dfbInterface->Release(dfbInterface);
	printf("\n/* clean up */\n");
}


void* drawingVol(){
	
	printf("DRAWING VOL\n");
    
	/* draw image from file */
    
	IDirectFBImageProvider *provider;
	IDirectFBSurface *logoSurface = NULL;
	int32_t logoHeight, logoWidth;
	DFBSurfaceDescription surfaceDesc;


	if(currentChannel.videoPid == -1){
		printf("\n\n!!!!RADIO!!!\n\n");


	/* clear screen */
   	 DFBCHECK(primary->SetColor(primary, 0x00, 0x00, 0x00, 0xff));
    	 DFBCHECK(primary->FillRectangle(primary, 0, 0, screenWidth, screenHeight));

	/* generate keycode string for channel*/
	sprintf(keycodeString,"%s","RADIO ");

	DFBCHECK(primary->SetColor(primary, 0xff, 0xff, 0xff, 0x55));
	DFBCHECK(primary->DrawString(primary, keycodeString, -1, screenWidth/3, screenHeight/6, DSTF_CENTER));


	/* create the image provider for the specified file */
	DFBCHECK(dfbInterface->CreateImageProvider(dfbInterface, "volume_0.png", &provider));
    /* get surface descriptor for the surface where the image will be rendered */
	DFBCHECK(provider->GetSurfaceDescription(provider, &surfaceDesc));
    /* create the surface for the image */
	DFBCHECK(dfbInterface->CreateSurface(dfbInterface, &surfaceDesc, &logoSurface));
    /* render the image to the surface */
	DFBCHECK(provider->RenderTo(provider, logoSurface, NULL));
	
    /* cleanup the provider after rendering the image to the surface */
	provider->Release(provider);
	
    /* fetch the logo size and add (blit) it to the screen */
	DFBCHECK(logoSurface->GetSize(logoSurface, &logoWidth, &logoHeight));
	printf("w: %d h:%d\n",logoWidth,logoHeight);
	DFBCHECK(primary->Blit(primary,
                           /*source surface*/ logoSurface,
                           /*source region, NULL to blit the whole surface*/ NULL,
                           /*destination x coordinate of the upper left corner of the image*/screenWidth/10,
                           /*destination y coordinate of the upper left corner of the image*/screenHeight/10));

    /* switch between the displayed and the work buffer (update the display) */
	DFBCHECK(primary->Flip(primary,
                           /*region to be updated, NULL for the whole surface*/NULL,
                           /*flip flags*/0));
    
	/* set the timer for clearing the screen */
    
    	memset(&timerSpecVol,0,sizeof(timerSpecVol));
    
    	/* specify the timer timeout time */
    	timerSpecVol.it_value.tv_sec = 5;
    	timerSpecVol.it_value.tv_nsec = 0;
    
    	/* set the new timer specs */
    	ret = timer_settime(timerVolId,0,&timerSpecVol,&timerSpecOldVol);
    	if(ret == -1){
        	printf("Error setting timer in %s!\n", __FUNCTION__);
    	}

	}else{
	
	
    /* create the image provider for the specified file */
	DFBCHECK(dfbInterface->CreateImageProvider(dfbInterface, "volume_0.png", &provider));
    /* get surface descriptor for the surface where the image will be rendered */
	DFBCHECK(provider->GetSurfaceDescription(provider, &surfaceDesc));
    /* create the surface for the image */
	DFBCHECK(dfbInterface->CreateSurface(dfbInterface, &surfaceDesc, &logoSurface));
    /* render the image to the surface */
	DFBCHECK(provider->RenderTo(provider, logoSurface, NULL));
	
    /* cleanup the provider after rendering the image to the surface */
	provider->Release(provider);
	
    /* fetch the logo size and add (blit) it to the screen */
	DFBCHECK(logoSurface->GetSize(logoSurface, &logoWidth, &logoHeight));
	printf("w: %d h:%d\n",logoWidth,logoHeight);
	DFBCHECK(primary->Blit(primary,
                           /*source surface*/ logoSurface,
                           /*source region, NULL to blit the whole surface*/ NULL,
                           /*destination x coordinate of the upper left corner of the image*/screenWidth/10,
                           /*destination y coordinate of the upper left corner of the image*/screenHeight/10));

    /* switch between the displayed and the work buffer (update the display) */
	DFBCHECK(primary->Flip(primary,
                           /*region to be updated, NULL for the whole surface*/NULL,
                           /*flip flags*/0));
    
	/* set the timer for clearing the screen */
    
    	memset(&timerSpecVol,0,sizeof(timerSpecVol));
    
    	/* specify the timer timeout time */
    	timerSpecVol.it_value.tv_sec = 5;
    	timerSpecVol.it_value.tv_nsec = 0;
    
    	/* set the new timer specs */
    	ret = timer_settime(timerVolId,0,&timerSpecVol,&timerSpecOldVol);
    	if(ret == -1){
        	printf("Error setting timer in %s!\n", __FUNCTION__);
    	}
}

}


void* drawingBanner(){

	if(currentChannel.videoPid == -1){
	printf("\n\n!!!!RADIO!!!\n\n");

	 /* clear screen */
   	 DFBCHECK(primary->SetColor(primary, 0x00, 0x00, 0x00, 0xff));
    	 DFBCHECK(primary->FillRectangle(primary, 0, 0, screenWidth, screenHeight));

	/* generate keycode string for channel*/
	sprintf(keycodeString,"%s","RADIO ");

	DFBCHECK(primary->SetColor(primary, 0xff, 0xff, 0xff, 0x55));
	DFBCHECK(primary->DrawString(primary, keycodeString, -1, screenWidth/3, screenHeight/6, DSTF_CENTER));


	DFBCHECK(primary->SetColor(primary, 0x00, 0x10, 0x80, 0xff));
	DFBCHECK(primary->FillRectangle(primary, 0, screenHeight*5/6, screenWidth, screenHeight/6));
	

	/* generate keycode string for channel*/
	sprintf(keycodeString,"%s","CH: ");

	DFBCHECK(primary->SetColor(primary, 0xff, 0xff, 0xff, 0xff));
	DFBCHECK(primary->DrawString(primary, keycodeString, -1, screenWidth/16, screenHeight*11/12, DSTF_CENTER));


	/* generate keycode string for chnannel number */
	sprintf(keycodeString,"%d",(currentChannel.programNumber));

	DFBCHECK(primary->SetColor(primary, 0xff, 0xff, 0xff, 0xff));
	DFBCHECK(primary->DrawString(primary, keycodeString, -1, screenWidth/16+50, screenHeight*11/12, DSTF_CENTER));

	/* generate keycode string for AudioPid*/ 
	sprintf(keycodeString,"%s","AudioPID: ");

	DFBCHECK(primary->SetColor(primary, 0xff, 0xff, 0xff, 0xff));
	DFBCHECK(primary->DrawString(primary, keycodeString, -1, screenWidth/6, screenHeight*11/12, DSTF_CENTER));

	
	/* generate keycode string for AudioPid number */
	sprintf(keycodeString,"%d", currentChannel.audioPid);

	DFBCHECK(primary->SetColor(primary, 0xff, 0xff, 0xff, 0xff));
	DFBCHECK(primary->DrawString(primary, keycodeString, -1, screenWidth/6+140, screenHeight*11/12, DSTF_CENTER));	

	/* generate keycode string for VideoPid*/
	sprintf(keycodeString,"%s","VideoPID: ");

	DFBCHECK(primary->SetColor(primary, 0xff, 0xff, 0xff, 0xff));
	DFBCHECK(primary->DrawString(primary, keycodeString, -1, screenWidth/3, screenHeight*11/12, DSTF_CENTER));

	/* generate keycode string for VideoPid number */
	sprintf(keycodeString,"%d", currentChannel.videoPid);

	DFBCHECK(primary->SetColor(primary, 0xff, 0xff, 0xff, 0xff));
	DFBCHECK(primary->DrawString(primary, keycodeString, -1, screenWidth/3+140, screenHeight*11/12, DSTF_CENTER));

	/* generate keycode string for Teletext*/
	sprintf(keycodeString,"%s","Teletext: ");

	DFBCHECK(primary->SetColor(primary, 0xff, 0xff, 0xff, 0xff));
	DFBCHECK(primary->DrawString(primary, keycodeString, -1, screenWidth/2, screenHeight*11/12, DSTF_CENTER));
	if(txt == true)
	{
		DFBCHECK(primary->SetColor(primary, 0xff, 0xff, 0xff, 0xff));
		DFBCHECK(primary->DrawString(primary, "YES", -1, screenWidth/2+120, screenHeight*11/12, DSTF_CENTER));
	}
	else
	{
		DFBCHECK(primary->SetColor(primary, 0xff, 0xff, 0xff, 0xff));
		DFBCHECK(primary->DrawString(primary, "NO", -1, screenWidth/2+120, screenHeight*11/12, DSTF_CENTER));
	}	


	
    
	 /* update screen */
    	 DFBCHECK(primary->Flip(primary, NULL, 0));

	
	/* set the timer for clearing the screen */
    
    	memset(&timerSpec,0,sizeof(timerSpec));
    
    	/* specify the timer timeout time */
    	timerSpec.it_value.tv_sec = 3;
    	timerSpec.it_value.tv_nsec = 0;
    
    	/* set the new timer specs */
    	ret = timer_settime(timerId,0,&timerSpec,&timerSpecOld);
    	if(ret == -1){
        	printf("Error setting timer in %s!\n", __FUNCTION__);
    	}
	

	}else{
	

	 /* clear screen */
   	 DFBCHECK(primary->SetColor(primary, 0x00, 0x00, 0x00, 0x00));
    	 DFBCHECK(primary->FillRectangle(primary, 0, 0, screenWidth, screenHeight));

	DFBCHECK(primary->SetColor(primary, 0x00, 0x10, 0x80, 0xff));
	DFBCHECK(primary->FillRectangle(primary, 0, screenHeight*5/6, screenWidth, screenHeight/6));
	printf("\n\n!!!!ISCRTAVANJE!!!\n\n");

	/*-----------------------------------------------------------------------------------*/
	/* generate keycode string for channel*/
	sprintf(keycodeString,"%s","CH: ");

	DFBCHECK(primary->SetColor(primary, 0xff, 0xff, 0xff, 0xff));
	DFBCHECK(primary->DrawString(primary, keycodeString, -1, screenWidth/16, screenHeight*11/12, DSTF_CENTER));


	/* generate keycode string for chnannel number */
	sprintf(keycodeString,"%d",(currentChannel.programNumber));

	DFBCHECK(primary->SetColor(primary, 0xff, 0xff, 0xff, 0xff));
	DFBCHECK(primary->DrawString(primary, keycodeString, -1, screenWidth/16+50, screenHeight*11/12, DSTF_CENTER));

	/* generate keycode string for AudioPid*/ 
	sprintf(keycodeString,"%s","AudioPID: ");

	DFBCHECK(primary->SetColor(primary, 0xff, 0xff, 0xff, 0xff));
	DFBCHECK(primary->DrawString(primary, keycodeString, -1, screenWidth/6, screenHeight*11/12, DSTF_CENTER));

	
	/* generate keycode string for AudioPid number */
	sprintf(keycodeString,"%d", currentChannel.audioPid);

	DFBCHECK(primary->SetColor(primary, 0xff, 0xff, 0xff, 0xff));
	DFBCHECK(primary->DrawString(primary, keycodeString, -1, screenWidth/6+140, screenHeight*11/12, DSTF_CENTER));	

	/* generate keycode string for VideoPid*/
	sprintf(keycodeString,"%s","VideoPID: ");

	DFBCHECK(primary->SetColor(primary, 0xff, 0xff, 0xff, 0xff));
	DFBCHECK(primary->DrawString(primary, keycodeString, -1, screenWidth/3, screenHeight*11/12, DSTF_CENTER));

	/* generate keycode string for VideoPid number */
	sprintf(keycodeString,"%d", currentChannel.videoPid);

	DFBCHECK(primary->SetColor(primary, 0xff, 0xff, 0xff, 0xff));
	DFBCHECK(primary->DrawString(primary, keycodeString, -1, screenWidth/3+140, screenHeight*11/12, DSTF_CENTER));

	/* generate keycode string for Teletext*/
	sprintf(keycodeString,"%s","Teletext: ");

	DFBCHECK(primary->SetColor(primary, 0xff, 0xff, 0xff, 0xff));
	DFBCHECK(primary->DrawString(primary, keycodeString, -1, screenWidth/2, screenHeight*11/12, DSTF_CENTER));
	if(txt == true)
	{
		DFBCHECK(primary->SetColor(primary, 0xff, 0xff, 0xff, 0xff));
		DFBCHECK(primary->DrawString(primary, "YES", -1, screenWidth/2+120, screenHeight*11/12, DSTF_CENTER));
	}
	else
	{
		DFBCHECK(primary->SetColor(primary, 0xff, 0xff, 0xff, 0xff));
		DFBCHECK(primary->DrawString(primary, "NO", -1, screenWidth/2+120, screenHeight*11/12, DSTF_CENTER));
	}	

	/* update screen */ 
	DFBCHECK(primary->Flip(primary, NULL, 0));
	//printf("\n\n!!!!IZMENA EKRANA!!!\n\n");   
    
    	/* set the timer for clearing the screen */
    
    	memset(&timerSpec,0,sizeof(timerSpec));
    
    	/* specify the timer timeout time */
    	timerSpec.it_value.tv_sec = 3;
    	timerSpec.it_value.tv_nsec = 0;
    
    	/* set the new timer specs */
    	ret = timer_settime(timerId,0,&timerSpec,&timerSpecOld);
    	if(ret == -1){
        	printf("Error setting timer in %s!\n", __FUNCTION__);
    	}

	}

} 


StreamControllerError streamControllerInit(argStruct* arg_struct)
{
	
	programNumber = arg_struct->programNumber - 1;
	
	argVideoPid = arg_struct->videoPid;
	argAudioPid = arg_struct->audioPid;

	argVideoType = arg_struct->videoType;
	argAudioType = arg_struct->audioType;
	
	
	initRb();

	
    if (pthread_create(&scThread, NULL, &streamControllerTask, arg_struct))
    {
        printf("Error creating input event task!\n");
        return SC_THREAD_ERROR;
    }

    return SC_NO_ERROR;
}

StreamControllerError streamControllerDeinit()
{
    if (!isInitialized) 
    {
        printf("\n%s : ERROR streamControllerDeinit() fail, module is not initialized!\n", __FUNCTION__);
        return SC_ERROR;
    }
    
    threadExit = 1;
    if (pthread_join(scThread, NULL))
    {
        printf("\n%s : ERROR pthread_join fail!\n", __FUNCTION__);
        return SC_THREAD_ERROR;
    }
    
    /* free demux filter */  
    Demux_Free_Filter(playerHandle, filterHandle);

	/* remove audio stream */
	Player_Stream_Remove(playerHandle, sourceHandle, streamHandleA);
    
    /* remove video stream */
    Player_Stream_Remove(playerHandle, sourceHandle, streamHandleV);
    
    /* close player source */
    Player_Source_Close(playerHandle, sourceHandle);
    
    /* deinitialize player */
    Player_Deinit(playerHandle);
    
    /* deinitialize tuner device */
    Tuner_Deinit();
    
    /* free allocated memory */  
    free(patTable);
    free(pmtTable);
    
    /* set isInitialized flag */
    isInitialized = false;

    deinitRB();

    return SC_NO_ERROR;
}


StreamControllerError volumeUp()
{   
	//Player_Volume_Get(playerHandle,&volumeNumber);
	//printf("VOL1: %d\n",volumeNumber);

	//107374182
	Player_Volume_Set(playerHandle,volumeNumber + (2147483640/10));
	Player_Volume_Get(playerHandle,&volumeNumber);
	if(volumeNumber > 2147483640){
		volumeNumber = 2147483640;
		Player_Volume_Set(playerHandle,volumeNumber);
		Player_Volume_Get(playerHandle,&volumeNumber);
	}
	printf("VOL: %d\n",volumeNumber);
	drawingVol();


    
    return SC_NO_ERROR;
}

StreamControllerError volumeDown()
{   
	

	//Player_Volume_Get(playerHandle,&volumeNumber);
	//printf("VOL1: %d\n",volumeNumber);	
	if(volumeNumber <= 0 ){
		volumeNumber = 0;
		Player_Volume_Set(playerHandle,volumeNumber);
		Player_Volume_Get(playerHandle,&volumeNumber);
	}else{
		Player_Volume_Set(playerHandle,volumeNumber - (2147483640/10));
		Player_Volume_Get(playerHandle,&volumeNumber);
	}	
	printf("VOL: %d\n",volumeNumber);
	drawingVol();

    return SC_NO_ERROR;
}

StreamControllerError mute()
{   	
	//volumeNumber = 0;
	Player_Volume_Set(playerHandle,0);	
	Player_Volume_Get(playerHandle,&volumeNumber);
	printf("VOL: %d\n",volumeNumber);
	
	drawingVol();
    return SC_NO_ERROR;
}

StreamControllerError info()
{   
    int check;
    check = timer_gettime(timerId, &timerSpec);
	
    if(timerSpec.it_interval.tv_sec != 0 && timerSpec.it_interval.tv_sec < 3){
	timerSpec.it_interval.tv_sec = 0;
    }
    
    drawingBanner();	
    
	

    return SC_NO_ERROR;
}

StreamControllerError channelUp()
{   
    if (programNumber >= patTable->serviceInfoCount - 2)
    {
        programNumber = 0;
    } 
    else
    {
        programNumber++;
    }

    /* set flag to start current channel */
    changeChannel = true;

    return SC_NO_ERROR;
}


StreamControllerError channel(int16_t number)
{   
    
        programNumber = number;
    

    /* set flag to start current channel */
    changeChannel = true;

    return SC_NO_ERROR;
}

StreamControllerError channelDown()
{
    if (programNumber <= 0)
    {
        programNumber = patTable->serviceInfoCount - 2;
    } 
    else
    {
        programNumber--;
    }
   
    /* set flag to start current channel */
    changeChannel = true;

    return SC_NO_ERROR;
}

StreamControllerError getChannelInfo(ChannelInfo* channelInfo)
{
    if (channelInfo == NULL)
    {
        printf("\n Error wrong parameter\n", __FUNCTION__);
        return SC_ERROR;
    }
    
    channelInfo->programNumber = currentChannel.programNumber;
    channelInfo->audioPid = currentChannel.audioPid;
    channelInfo->videoPid = currentChannel.videoPid;
    
    return SC_NO_ERROR;
}

/* Sets filter to receive current channel PMT table
 * Parses current channel PMT table when it arrives
 * Creates streams with current channel audio and video pids
 */
StreamControllerError startChannel(int32_t channelNumber)
{
    
    /* free PAT table filter */
    Demux_Free_Filter(playerHandle, filterHandle);
    
    /* set demux filter for receive PMT table of program */
    if(Demux_Set_Filter(playerHandle, patTable->patServiceInfoArray[channelNumber + 1].pid, 0x02, &filterHandle))
	{
		printf("\n%s : ERROR Demux_Set_Filter() fail\n", __FUNCTION__);
        return SC_ERROR;
	}
    
    /* wait for a PMT table to be parsed*/
    pthread_mutex_lock(&demuxMutex);
	if (ETIMEDOUT == pthread_cond_wait(&demuxCond, &demuxMutex))
	{
		printf("\n%s : ERROR Lock timeout exceeded!\n", __FUNCTION__);
        streamControllerDeinit();
	}
	pthread_mutex_unlock(&demuxMutex);
    
   
 /* get audio and video pids */
    int16_t audioPid = -1;
    int16_t videoPid = -1;	
    
    uint8_t i = 0;
    
    

    for (i = 0; i < pmtTable->elementaryInfoCount; i++)
    {
        if (((pmtTable->pmtElementaryInfoArray[i].streamType == 0x1) || (pmtTable->pmtElementaryInfoArray[i].streamType == 0x2) || (pmtTable->pmtElementaryInfoArray[i].streamType == 0x1b))
            && (videoPid == -1))
        {
		//cheking video pid and type
		         
	   videoPid = pmtTable->pmtElementaryInfoArray[i].elementaryPid;
		printf("VIDEO PID from PMT: %d\n",videoPid);
		printf("First video: %d\n",firstV);

	   if(firstV == 0){
		if(videoPid != argVideoPid){
			printf("ERROR: Wrong video PID\nPressed exit and try again with new video PID\n");
			return SC_ERROR;
		}
		if(argVideoType!=42 ){
			if(argVideoType!=43){
				if(argVideoType!=44){
					if(argVideoType!=39){
						printf("ERROR: Wrong video TYPE\nPressed exit and try again with new video TYPE\n");
						return SC_ERROR;
					}
					printf("ERROR: Wrong video TYPE\nPressed exit and try again with new video TYPE\n");
					return SC_ERROR;
				}
				printf("ERROR: Wrong video TYPE\nPressed exit and try again with new video TYPE\n");
				return SC_ERROR;
			}
			printf("ERROR: Wrong video TYPE\nPressed exit and try again with new video TYPE\n");
			return SC_ERROR;
		}		
	   }
		firstV = 1;
        } 
        else if (((pmtTable->pmtElementaryInfoArray[i].streamType == 0x3) || (pmtTable->pmtElementaryInfoArray[i].streamType == 0x4))
            && (audioPid == -1))
        {
   	    //cheking audio pid and type
            audioPid = pmtTable->pmtElementaryInfoArray[i].elementaryPid;
            printf("AUDIO PID from PMT: %d\n",audioPid);
	    printf("First audio: %d\n",firstA);

	    if(firstA == 0){
		if(audioPid != argAudioPid){
			printf("ERROR: Wrong audio PID\nPressed exit and try again with new audio PID\n");
			return SC_ERROR;
		}
		if(argAudioType!=10 /*|| argAudioType!=11 || argAudioType!=12 || argAudioType!=1*/){
			if(argAudioType!=11 ){
				if(argAudioType!=12){
					if(argAudioType!=1){
						printf("ERROR: Wrong audio TYPE\nPressed exit and try again with new video TYPE\n");
						return SC_ERROR;
					}

					printf("ERROR: Wrong audio TYPE\nPressed exit and try again with new video TYPE\n");
					return SC_ERROR;
				}				
				printf("ERROR: Wrong audio TYPE\nPressed exit and try again with new video TYPE\n");
				return SC_ERROR;
			}			
			printf("ERROR: Wrong audio TYPE\nPressed exit and try again with new video TYPE\n");
			return SC_ERROR;
		}
				
	   }	
		firstA = 1;
        }
    } 
    //check teletext
    if(pmtTable->pmtElementaryInfoArray[i].streamType == 0x06)
	{
		txt = true;
	}

    if (videoPid != -1) 
    {
        /* remove previous video stream */
        if (streamHandleV != 0)
        {
		    Player_Stream_Remove(playerHandle, sourceHandle, streamHandleV);
            streamHandleV = 0;
        }

        /* create video stream */
        if(Player_Stream_Create(playerHandle, sourceHandle, videoPid, argVideoType, &streamHandleV))
        {
            printf("\n%s : ERROR Cannot create video stream\n", __FUNCTION__);
            streamControllerDeinit();
        }
    }else{
	/* remove previous video stream */
	if (streamHandleV != 0)
		{
			    Player_Stream_Remove(playerHandle, sourceHandle, streamHandleV);
		    streamHandleV = 0;
			
		}

	}

    if (audioPid != -1)
    {   
        /* remove previos audio stream */
        if (streamHandleA != 0)
        {
            Player_Stream_Remove(playerHandle, sourceHandle, streamHandleA);
            streamHandleA = 0;
        }

	    /* create audio stream */
        if(Player_Stream_Create(playerHandle, sourceHandle, audioPid, argAudioType, &streamHandleA))
        {
            printf("\n%s : ERROR Cannot create audio stream\n", __FUNCTION__);
            streamControllerDeinit();
        }
    }
    printf("CRTANJE\n");
	//drawing info baner
	sleep(3);
    
    /* store current channel info */
    currentChannel.programNumber = channelNumber + 1;
    currentChannel.audioPid = audioPid;
    currentChannel.videoPid = videoPid;

	//pokretanje niti
    if (pthread_create(&drawThread, NULL, &drawingBanner, NULL))
    {
        printf("Error creating info banner!\n");
        return SC_THREAD_ERROR;
    }



}

void* streamControllerTask(argStruct* arg_struct)
{
    gettimeofday(&now,NULL);
    lockStatusWaitTime.tv_sec = now.tv_sec+10;

    /* allocate memory for PAT table section */
    patTable=(PatTable*)malloc(sizeof(PatTable));
    if(patTable==NULL)
    {
		printf("\n%s : ERROR Cannot allocate memory\n", __FUNCTION__);
        return (void*) SC_ERROR;
	}  
    memset(patTable, 0x0, sizeof(PatTable));

    /* allocate memory for PMT table section */
    pmtTable=(PmtTable*)malloc(sizeof(PmtTable));
    if(pmtTable==NULL)
    {
		printf("\n%s : ERROR Cannot allocate memory\n", __FUNCTION__);
        return (void*) SC_ERROR;
	}  
    memset(pmtTable, 0x0, sizeof(PmtTable));
       
    /* initialize tuner device */
    if(Tuner_Init())
    {
        printf("\n%s : ERROR Tuner_Init() fail\n", __FUNCTION__);
        free(patTable);
        free(pmtTable);
        return (void*) SC_ERROR;
    }
    
    /* register tuner status callback */
    if(Tuner_Register_Status_Callback(tunerStatusCallback))
    {
		printf("\n%s : ERROR Tuner_Register_Status_Callback() fail\n", __FUNCTION__);
	}
    
    /* lock to frequency */
    if(!Tuner_Lock_To_Frequency(arg_struct->frequency, arg_struct->bandwidth, arg_struct->modul))
    {
        printf("\n%s: INFO Tuner_Lock_To_Frequency(): %d Hz - success!\n",__FUNCTION__,arg_struct->frequency);
    }
    else
    {
        printf("\n%s: ERROR Tuner_Lock_To_Frequency(): %d Hz - fail!\n",__FUNCTION__,arg_struct->frequency);
        free(patTable);
        free(pmtTable);
        Tuner_Deinit();
        return (void*) SC_ERROR;
    }
    
    /* wait for tuner to lock */
    pthread_mutex_lock(&statusMutex);
    if(ETIMEDOUT == pthread_cond_timedwait(&statusCondition, &statusMutex, &lockStatusWaitTime))
    {
        printf("\n%s : ERROR Lock timeout exceeded!\n",__FUNCTION__);
        free(patTable);
        free(pmtTable);
        Tuner_Deinit();
        return (void*) SC_ERROR;
    }
    pthread_mutex_unlock(&statusMutex);
   
    /* initialize player */
    if(Player_Init(&playerHandle))
    {
		printf("\n%s : ERROR Player_Init() fail\n", __FUNCTION__);
		free(patTable);
        free(pmtTable);
        Tuner_Deinit();
        return (void*) SC_ERROR;
	}
	
	/* open source */
	if(Player_Source_Open(playerHandle, &sourceHandle))
    {
		printf("\n%s : ERROR Player_Source_Open() fail\n", __FUNCTION__);
		free(patTable);
        free(pmtTable);
		Player_Deinit(playerHandle);
        Tuner_Deinit();
        return (void*) SC_ERROR;	
	}

	/* set PAT pid and tableID to demultiplexer */
	if(Demux_Set_Filter(playerHandle, 0x00, 0x00, &filterHandle))
	{
		printf("\n%s : ERROR Demux_Set_Filter() fail\n", __FUNCTION__);
	}
	
	/* register section filter callback */
    if(Demux_Register_Section_Filter_Callback(sectionReceivedCallback))
    {
		printf("\n%s : ERROR Demux_Register_Section_Filter_Callback() fail\n", __FUNCTION__);
	}

    pthread_mutex_lock(&demuxMutex);
	if (ETIMEDOUT == pthread_cond_wait(&demuxCond, &demuxMutex))
	{
		printf("\n%s:ERROR Lock timeout exceeded!\n", __FUNCTION__);
        free(patTable);
        free(pmtTable);
		Player_Deinit(playerHandle);
        Tuner_Deinit();
        return (void*) SC_ERROR;
	}
	pthread_mutex_unlock(&demuxMutex);
    

	//graph();

    /* start current channel */
    startChannel(programNumber);
   
    /* set isInitialized flag */
    isInitialized = true;

    while(!threadExit)
    {
        if (changeChannel)
        {
            changeChannel = false;
            startChannel(programNumber);
        }
    }
}

int32_t sectionReceivedCallback(uint8_t *buffer)
{
    uint8_t tableId = *buffer;  
    if(tableId==0x00)
    {
        //printf("\n%s -----PAT TABLE ARRIVED-----\n",__FUNCTION__);
        
        if(parsePatTable(buffer,patTable)==TABLES_PARSE_OK)
        {
            //printPatTable(patTable);
            pthread_mutex_lock(&demuxMutex);
		    pthread_cond_signal(&demuxCond);
		    pthread_mutex_unlock(&demuxMutex);
            
        }
    } 
    else if (tableId==0x02)
    {
        //printf("\n%s -----PMT TABLE ARRIVED-----\n",__FUNCTION__);
        
        if(parsePmtTable(buffer,pmtTable)==TABLES_PARSE_OK)
        {
            //printPmtTable(pmtTable);
            pthread_mutex_lock(&demuxMutex);
		    pthread_cond_signal(&demuxCond);
		    pthread_mutex_unlock(&demuxMutex);
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
