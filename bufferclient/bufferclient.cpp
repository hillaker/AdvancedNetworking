#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "protocol.h"
#include "bufferclient.h"
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif

#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glew.h>
#include <GL/glut.h>
#endif

#define NENDS 2           /* number of end "points" to draw */
#define ENDHWIDTH 5       /* half width of end "points," in pixels */
 
GLdouble width, height;   /* window width and height */
int wd;                   /* GLUT window handle */
int openGLSocket;
GLuint texture;
unsigned char * input;
/* SUGIH: BEGIN moved from display() */
int imageWidth = 1920;
int imageHeight = 1080;
extern unsigned char *raw_image;
/* */

/* Program initialization NOT OpenGL/GLUT dependent,
   as we haven't created a GLUT window yet */
void
init(void)
{
  width  = 1280.0;                 /* initial window width and height, */
  height = 800.0;                  /* within which we draw. */
  //ends[0][0] = (int)(0.25*width);  /* (0,0) is the lower left corner */
  //ends[0][1] = (int)(0.75*height);
  //ends[1][0] = (int)(0.75*width);
  //ends[1][1] = (int)(0.25*height);
}

/* Callback functions for GLUT */
void 
display(void)
{
  /* SUGIH: BEGIN move to initGL
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, imageWidth, imageHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, (GLvoid *)0);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); 
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); 	
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); 
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE); 
  SUGIH: move to initGL */
  glClearColor(0.7f, 0.8f, 1.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  glEnable(GL_TEXTURE_2D); 
  
  //glColor3f(1.0,0.0, 0.0);
  glBegin(GL_QUADS); 
	glTexCoord2f(1.0,1.0); glVertex3f(width, height, 0.0);
	glTexCoord2f(0.0,1.0); glVertex3f(0, height, 0.0);
	glTexCoord2f(0.0,0.0); glVertex3f(0, 0, 0.0);
	glTexCoord2f(1.0,0.0); glVertex3f(width, 0.0, 0.0);
  glEnd();
 /* if(raw_image)
	{
		std::cout << "before glFlush: " << glGetError() << std::endl;
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
		std::cout << "after glFlush: " << glGetError() << std::endl;
	}*/
  glDisable(GL_TEXTURE_2D); // state machine! 
  
  //glBindFramebuffer(GL_FRAMEBUFFER, 0);
  //glDrawBuffer(GL_BACK);
  //glDeleteFramebuffers(1, &fb);
  /* force drawing to start */
  std::cout << "redrawing" << std::endl;
  glFlush();
 // glutSwapBuffers();
}

void
reshape(int w, int h)
{
  /* save new screen dimensions */
  width = (GLdouble) w;
  height = (GLdouble) h;

  /* tell OpenGL to use the whole window for drawing */
  glViewport(0, 0, (GLsizei) w, (GLsizei) h);

  /* do an orthographic parallel projection with the coordinate
     system set to first quadrant, limited by screen/window size */
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(0.0, width, 0.0, height);
}

void
refresh(void)
{
	
  static struct timeval prev;
  struct timeval now;
  double tprev;
  double tnow;

#ifdef _WIN32
  SYSTEMTIME st;
  
  GetSystemTime(&st);
  now.tv_sec = st.wSecond;
  now.tv_usec = st.wMilliseconds*1000;
#else
  gettimeofday(&now, NULL);
#endif

  tprev = (double)prev.tv_sec + 1.0e-6*(double)prev.tv_usec;
  tnow = (double)now.tv_sec + 1.0e-6*(double)now.tv_usec;
  if ((tnow - tprev) > 0.1) {
    prev.tv_sec = now.tv_sec;
    prev.tv_usec = now.tv_usec;
    
    packetHeader header;
    //read jpeg from disk
   //readJpeg("test.jpg", header);
  // glGenTextures(1, &texture);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, header.imageWidth, header.imageHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, raw_image);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); 
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); 	
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); 
   glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE); 

    /* redisplay */
    glutPostRedisplay();
  }
}

void
initGL()
{
  glGenTextures(1, &texture);
   glBindTexture(GL_TEXTURE_2D, texture);
  /*glBindTexture(GL_TEXTURE_2D, texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); 
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); 	
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); 
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE); */
  /* SUGIH: moved from initGL*/
/*	GLuint pixelBuffer;
	glGenBuffers(1, &pixelBuffer); 
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pixelBuffer);
*/
  /* SUGIH: BEGIN move to initGL */
  /*GLuint texture;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, imageWidth, imageHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, (GLvoid *)0);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); 
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); 	
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); 
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE); */
  /* */
}

int 
setupSocket(const char * address, int port)
{
	int	sd;
	struct sockaddr_in pin;

	/* fill in the socket structure with host information */
	memset(&pin, 0, sizeof(pin));
	pin.sin_family = AF_INET;
	inet_pton(AF_INET, address, &pin.sin_addr.s_addr);
	pin.sin_port = htons(port);

	/* grab an Internet domain socket */
	if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		exit(1);
	}

	/* connect to PORT on HOST */
	if (connect(sd,(struct sockaddr *)  &pin, sizeof(pin)) == -1) {
		perror("connect");
		exit(1);
	}

   return sd;
}

void
newSocketHandler(void)
{
	  static struct timeval prev;
  struct timeval now;
  double tprev;
  double tnow;

#ifdef _WIN32
  SYSTEMTIME st;
  
  GetSystemTime(&st);
  now.tv_sec = st.wSecond;
  now.tv_usec = st.wMilliseconds*1000;
#else
  gettimeofday(&now, NULL);
#endif

  tprev = (double)prev.tv_sec + 1.0e-6*(double)prev.tv_usec;
  tnow = (double)now.tv_sec + 1.0e-6*(double)now.tv_usec;
  if ((tnow - tprev) > 0.1) {
    prev.tv_sec = now.tv_sec;
    prev.tv_usec = now.tv_usec;
    
    packetHeader header;
    //read jpeg from disk
   readJpeg("test.jpg", header);
   //glGenTextures(1, &texture);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, header.imageWidth, header.imageHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, raw_image);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); 
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); 	
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); 
   glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE); 

    /* redisplay */
    glutPostRedisplay();
  }
}

void 
socketHandler(void)
{
	/*
	int socketNum = openGLSocket;
	long long receivedBytes;
	unsigned long imageSize;
	unsigned long total;
	draw = false;
	total = 0;
	packetHeader header;
	//send a message to the server PORT on machine HOST 
	if (send(socketNum, "hello world", sizeof("hello world"), 0) == -1) {
		perror("send");
		exit(1);
	}

	// wait for a message to come back from the server
	if ( (receivedBytes = recv(socketNum, &header, sizeof( header), 0)) == -1) {
			   perror("recv");
			   exit(1);
	   }
	imageHeight = header.imageHeight;
	imageWidth = header.imageWidth;
	imageSize = imageHeight*imageWidth*header.imageComponents;
	//std::cout << "height: " << imageHeight << " width: " << imageWidth 
	//	<< " components: " << header.imageComponents << " total: " << imageSize << std::endl;
*/
  /* SUGIH: BEGIN move to setupSocket
	GLuint pixelBuffer; // make global
	glGenBuffers(1, &pixelBuffer); 
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pixelBuffer);
     SUGIH: END move to setupSocket */

	/*glBufferData(GL_PIXEL_UNPACK_BUFFER,  imageSize, NULL, GL_DYNAMIC_DRAW);  
	input = (unsigned char *)glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
	//input = (unsigned char *)malloc(imageSize*sizeof(char));
	//memset(input, 0,  header.packetLength*sizeof(char));
	//std::cout << "size of malloc: " <<  imageSize*sizeof(char) << std::endl;
	do
	{
	   if ( (receivedBytes = recv(socketNum, &input[total],  imageSize, 0)) == -1) {
			   perror("recv");
			   exit(1);
	   }
	   //std::cout << test << std::endl;
	 //  std::cout << "received " << receivedBytes << " bytes" << std::endl;
	   total += receivedBytes;
	 //  std::cout << "(" << total << " bytes of " << imageSize << " bytes recieved)" << std::endl;
   } 
   while(total <  imageSize);
   glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER); */
   glutPostRedisplay();
   //glFlush();

 }

int
main(int argc, char *argv[])
{
  //openGLSocket = setupSocket("127.0.0.1", 8999);
  init();
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_SINGLE | GLUT_RGBA);
  glutInitWindowSize((int) width, (int) height);
  wd = glutCreateWindow("Client Buffer" /* title */ );
  glutDisplayFunc(display);
  glutReshapeFunc(reshape);
  glutIdleFunc(newSocketHandler);
  //glutIdleFunc(socketHandler);
  //initGL();
#ifndef __APPLE__
  glewInit();
#endif
  /* start the GLUT main loop */
  glutMainLoop();
  std::cout << "closing main socket" << std::endl;
  //close(openGLSocket);
  return 0;
}
