/*
void wipeScreen(union sigval signalArg){
    int32_t ret;

     clear screen 
    DFBCHECK(primary->SetColor(primary, 0x00, 0x00, 0x00, 0xff));
    DFBCHECK(primary->FillRectangle(primary, 0, 0, screenWidth, screenHeight));
    
     update screen 
    DFBCHECK(primary->Flip(primary, NULL, 0));
    
     stop the timer 
    memset(&timerSpec,0,sizeof(timerSpec));
    ret = timer_settime(timerId,0,&timerSpec,&timerSpecOld);
    if(ret == -1){
        printf("Error setting timer in %s!\n", __FUNCTION__);
    }
}
*/
/*
void graph(){


	/* structure for timer specification */
    struct sigevent signalEvent;
    
    int32_t ret;
   char keycodeString[4];
    
    
    /* set the function for clearing screen */
    
    /* create timer */
    signalEvent.sigev_notify = SIGEV_THREAD; /* tell the OS to notify you about timer by calling the specified function */
    signalEvent.sigev_notify_function = wipeScreen; /* function to be called when timer runs out */
    signalEvent.sigev_value.sival_ptr = NULL; /* thread arguments */
    signalEvent.sigev_notify_attributes = NULL; /* thread attributes (e.g. thread stack size) - if NULL default attributes are applied */
    ret = timer_create(/*clock for time measuring*/CLOCK_REALTIME,
                       /*timer settings*/&signalEvent,
                       /*where to store the ID of the newly created timer*/&timerId);
    if(ret == -1){
        printf("Error creating timer, abort!\n");
        primary->Release(primary);
        dfbInterface->Release(dfbInterface);
        
    }

    /* clear the buffer before drawing */
    
    DFBCHECK(primary->SetColor(primary, 0x00, 0x00, 0x00, 0xff));
    DFBCHECK(primary->FillRectangle(primary, 0, 0, screenWidth, screenHeight));
    
    
    /*  draw the frame */
    
    DFBCHECK(primary->SetColor(primary, 0x40, 0x10, 0x80, 0xff));
    DFBCHECK(primary->FillRectangle(primary, 0, screenHeight*3/4, screenWidth, screenHeight));

    /* draw keycode */

    fontDesc.flags = DFDESC_HEIGHT;
    fontDesc.height = 50;

    DFBCHECK(dfbInterface->CreateFont(dfbInterface, "/home/galois/fonts/DejaVuSans.ttf", &fontDesc, &fontInterface));
    DFBCHECK(primary->SetFont(primary, fontInterface));

    /* generate keycode string */
    sprintf(keycodeString,"CH:3");
    
    /* draw the string */
    DFBCHECK(primary->SetColor(primary, 0x10, 0x80, 0x40, 0xff));
    DFBCHECK(primary->DrawString(primary, keycodeString, -1, 120, screenHeight*9/10 + 20, DSTF_CENTER));

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


/******************************************************************************/
/* initialize DirectFB */
    
	/*DFBCHECK(DirectFBInit(NULL, NULL));
	DFBCHECK(DirectFBCreate(&dfbInterface));
	DFBCHECK(dfbInterface->SetCooperativeLevel(dfbInterface, DFSCL_FULLSCREEN));
	
   	/* create primary surface with double buffering enabled */
    
	surfaceDesc.flags = DSDESC_CAPS;
	surfaceDesc.caps = DSCAPS_PRIMARY | DSCAPS_FLIPPING;
	DFBCHECK (dfbInterface->CreateSurface(dfbInterface, &surfaceDesc, &primary));
    
    
    	/* fetch the screen size */
   	DFBCHECK (primary->GetSize(primary, &screenWidth, &screenHeight));

/**********************************/
/*
static timer_t timerId;
static IDirectFBSurface *primary = NULL;
IDirectFB *dfbInterface = NULL;
static int32_t screenWidth = 0;
static int32_t screenHeight = 0;
*/


