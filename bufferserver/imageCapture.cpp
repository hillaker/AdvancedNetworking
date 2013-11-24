#include "imageCapture.h"

static int screenNumber;
static Display *display = NULL;
static Window rootWindow;
//static Screen *screen = NULL;
static XImage *image = NULL;
static XShmSegmentInfo xshmInfo;

void initializeCapture()
{
	if((display = XOpenDisplay(NULL  /*use the default display*/)) == NULL)
	{
		std::cout << "display open failed" << std::endl; 
		exit(1);
	}
	int dummy;
	if(XQueryExtension(display, "MIT-SHM", &dummy, &dummy, &dummy)) 
	{
		int major, minor;
		Bool pixmaps;
		if(XShmQueryVersion(display, &major, &minor, &pixmaps) == True)
		{
			std::cout << "XShm extention version " << major << "." << minor << std::endl;
		} 
		else 
		{
			std::cout << "XShm extension not supported." << std::endl;
		}
	}
	screenNumber = XDefaultScreen(display);
	int width = XDisplayWidth(display, screenNumber);
	int height = XDisplayHeight(display, screenNumber);
	int depth = XDisplayPlanes(display, screenNumber);
	rootWindow = XRootWindow(display, screenNumber);
	if((image = XShmCreateImage(display,
		XDefaultVisual(display, screenNumber),
		depth, ZPixmap, NULL, &xshmInfo,
		width, height)) == NULL)
	{
		std::cout << "XShmCreateImage failed\n" << std::endl;
		exit(1);
	}
	xshmInfo.shmid = shmget(IPC_PRIVATE, image->bytes_per_line*image->height,
                         IPC_CREAT | 0777 );
	if(xshmInfo.shmid < 0) 
	{
	  std::cout << "failed to allocate shared memory." << std::endl;
	  exit(1);
	}
	xshmInfo.shmaddr = image->data = (char*) shmat(xshmInfo.shmid, 0, 0);
	xshmInfo.readOnly = False;
	if(XShmAttach(display, &xshmInfo) == 0) {
		std::cout << "XShmAttach failed." << std::endl;
		exit(1);
	}
}

char * captureImage(int &height, int &width, int &depth)
{
	std::cout << "beginning image capture" << std::endl;
	char * outputBuffer;
	unsigned long pixel;
	width = image->width;
	height = image->height;
	depth = XDisplayPlanes(display, screenNumber)/8;
	
	//std::cout << "masks:\nred: " << image->red_mask << " green: " << image->green_mask << " blue: " << image->blue_mask << std::endl;
	outputBuffer = (char *)malloc(width*height*depth);
	
	if(XShmGetImage(display, rootWindow, image, 0, 0, XAllPlanes()) == 0)
	{
		std::cout << "Capture attempt failed" << std::endl;
	}
	for (int y = 0; y < image->height; y++) 
	{
        for (int x = 0; x < image->width; x++) 
        {
            pixel = XGetPixel(image,x,image->height-y);
            outputBuffer[y*image->width*3+x*3+0] = (char)((pixel&image->red_mask)>>16);
            outputBuffer[y*image->width*3+x*3+1] = (char)((pixel&image->green_mask)>>8);
            outputBuffer[y*image->width*3+x*3+2] = (char)(pixel&image->blue_mask);
        }
    }
	//memcpy(outputBuffer, image->data, height*width*depth);
	return outputBuffer;
}
