#ifndef imageCapture_h
#define imageCapture_h
#include <sys/ipc.h>
#include <sys/shm.h>
#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>

void initializeCapture();
char * captureImage(int &height, int &width, int &depth);
#endif
