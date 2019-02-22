#include <stdio.h>
#include <directfb.h>
#include <linux/input.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <stdint.h>

#define FRAME_THICKNESS 20
#define FONT_HEIGHT 150
#define EXIT_BUTTON_KEYCODE 102


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


static timer_t timerId;
static IDirectFBSurface *primary = NULL;
IDirectFB *dfbInterface = NULL;
static int32_t screenWidth = 0;
static int32_t screenHeight = 0;

static struct itimerspec timerSpec;
static struct itimerspec timerSpecOld;

static void drawKeycode(int32_t keycode);
static void wipeScreen(union sigval signalArg);


int32_t main(int32_t argc, char** argv)
{
	DFBSurfaceDescription surfaceDesc;
    
    /* remote device name */
    static const char* dev = "/dev/input/event0";
    static int32_t inputFileDesc;
    char deviceName[20];
    struct input_event event;
    
    /* structure for timer specification */
    struct sigevent signalEvent;
    
    int32_t ret;
    

    /* initialize DirectFB */
    
	DFBCHECK(DirectFBInit(&argc, &argv));
	DFBCHECK(DirectFBCreate(&dfbInterface));
	DFBCHECK(dfbInterface->SetCooperativeLevel(dfbInterface, DFSCL_FULLSCREEN));
	
    /* create primary surface with double buffering enabled */
    
	surfaceDesc.flags = DSDESC_CAPS;
	surfaceDesc.caps = DSCAPS_PRIMARY | DSCAPS_FLIPPING;
	DFBCHECK (dfbInterface->CreateSurface(dfbInterface, &surfaceDesc, &primary));
    
    
    /* fetch the screen size */
    DFBCHECK (primary->GetSize(primary, &screenWidth, &screenHeight));
    
    
    /* clear the screen before drawing anything (draw black full screen rectangle)*/
    
    DFBCHECK(primary->SetColor(primary, 0x00, 0x00, 0x00, 0xff));
	DFBCHECK(primary->FillRectangle(primary, 0, 0, screenWidth, screenHeight));
    
    
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
        
        return 0;
    }
    
    
    /* open device for reading remote key events */
    
    inputFileDesc = open(dev,O_RDWR);
    ioctl(inputFileDesc, EVIOCGNAME(sizeof(deviceName)), deviceName);
	printf("RC device opened succesfully [%s]\n", deviceName);
    
    do{
        ret = read(inputFileDesc,&event,(size_t)sizeof(struct input_event));
        
        /*  only react if we've read something and only on "press down" */
        if(ret > 0){
            if(event.value == 1){
                /* draw the keycode */
                drawKeycode(event.code);
            }
        }
    }while(event.code != EXIT_BUTTON_KEYCODE);
	
    
    /* clean up */
    timer_delete(timerId);
	primary->Release(primary);
	dfbInterface->Release(dfbInterface);

    return 0;
}


void drawKeycode(int32_t keycode){
    int32_t ret;
    IDirectFBFont *fontInterface = NULL;
    DFBFontDescription fontDesc;
    char keycodeString[4];
    
    
    /* clear the buffer before drawing */
    
    DFBCHECK(primary->SetColor(primary, 0x00, 0x00, 0x00, 0xff));
    DFBCHECK(primary->FillRectangle(primary, 0, 0, screenWidth, screenHeight));
    
    
    /*  draw the frame */
    
    DFBCHECK(primary->SetColor(primary, 0x40, 0x10, 0x80, 0xff));
    DFBCHECK(primary->FillRectangle(primary, screenWidth/3, screenHeight/3, screenWidth/3, screenHeight/3));
    
    DFBCHECK(primary->SetColor(primary, 0x80, 0x40, 0x10, 0xff));
    DFBCHECK(primary->FillRectangle(primary, screenWidth/3+FRAME_THICKNESS, screenHeight/3+FRAME_THICKNESS, screenWidth/3-2*FRAME_THICKNESS, screenHeight/3-2*FRAME_THICKNESS));
    
    
    /* draw keycode */
    
	fontDesc.flags = DFDESC_HEIGHT;
	fontDesc.height = FONT_HEIGHT;
	
	DFBCHECK(dfbInterface->CreateFont(dfbInterface, "/home/galois/fonts/DejaVuSans.ttf", &fontDesc, &fontInterface));
	DFBCHECK(primary->SetFont(primary, fontInterface));
    
    /* generate keycode string */
    sprintf(keycodeString,"%d",keycode);
    
    /* draw the string */
    DFBCHECK(primary->SetColor(primary, 0x10, 0x80, 0x40, 0xff));
	DFBCHECK(primary->DrawString(primary, keycodeString, -1, screenWidth/2, screenHeight/2+FONT_HEIGHT/2, DSTF_CENTER));
    
    
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
}

void wipeScreen(union sigval signalArg){
    int32_t ret;

    /* clear screen */
    DFBCHECK(primary->SetColor(primary, 0x00, 0x00, 0x00, 0xff));
    DFBCHECK(primary->FillRectangle(primary, 0, 0, screenWidth, screenHeight));
    
    /* update screen */
    DFBCHECK(primary->Flip(primary, NULL, 0));
    
    /* stop the timer */
    memset(&timerSpec,0,sizeof(timerSpec));
    ret = timer_settime(timerId,0,&timerSpec,&timerSpecOld);
    if(ret == -1){
        printf("Error setting timer in %s!\n", __FUNCTION__);
    }
}
