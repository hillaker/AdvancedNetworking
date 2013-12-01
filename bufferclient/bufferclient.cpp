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
#include <queue>
#include <utility>
#include <fstream>
#include <signal.h>
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
#include <GL/freeglut_ext.h> //needed to for the glutCloseFunc that I'm using. 
//if you don't have freeglut, it can be removed, but results will only be reported on 
//a sigterm or sigint instead of when the glut window is closed. 

#endif

#define NENDS 2           /* number of end "points" to draw */
#define ENDHWIDTH 5       /* half width of end "points," in pixels */
 
GLdouble width, height;   /* window width and height */
int wd;                   /* GLUT window handle */
int openGLSocket;
GLuint texture;
GLuint pixelBuffer;
double floatingAvgMs = 0;
std::queue< timeval > pendingTimes;
std::queue<double> completeTimes;
/* SUGIH: BEGIN moved from display() */
//int imageWidth = 1920;
//int imageHeight = 1080;
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

void 
storeTime(timeval time)
{
	static unsigned long numSamples = 0;
	struct timeval now;
	gettimeofday(&now, NULL);
	double currentMs = ((now.tv_sec-time.tv_sec) * 1000) + ((now.tv_usec-time.tv_usec) / 1000); //calculate time in ms between when this was requested and now
	floatingAvgMs = ((numSamples*floatingAvgMs)+currentMs)/(numSamples+1);
	numSamples++;
	completeTimes.push(currentMs);
	//std::cout << "This sample was " << currentMs << " ms\t";
	//std::cout << "Averaging " << floatingAvgMs << " ms ( "  << (1000/floatingAvgMs) << " FPS)"<< std::endl; 
}

/* Callback functions for GLUT */
void 
display(void)
{
  glClear(GL_COLOR_BUFFER_BIT);
  glEnable(GL_TEXTURE_2D); 
  
  glBegin(GL_QUADS); 
	glTexCoord2f(1.0,1.0); glVertex3f(width, height, 0.0);
	glTexCoord2f(0.0,1.0); glVertex3f(0, height, 0.0);
	glTexCoord2f(0.0,0.0); glVertex3f(0, 0, 0.0);
	glTexCoord2f(1.0,0.0); glVertex3f(width, 0.0, 0.0);
  glEnd();
  glDisable(GL_TEXTURE_2D); // state machine! 
  
  /* force drawing to start */
  glDeleteBuffers(1, &pixelBuffer);
  glFlush();
  storeTime(pendingTimes.front());
  pendingTimes.pop();
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
socketHandler(void)
{
	unsigned char * input;
	int socketNum = openGLSocket;
	int imageSize;
	int total = 0;
	int receivedBytes;
	packetHeader header;

	//do socket read
	//if (send(socketNum, "hello world", sizeof("hello world"), 0) == -1) 
	//{
	//	perror("send");
	//	exit(1);
	//}

		/* wait for a message to come back from the server */
	if ( (receivedBytes = recv(socketNum, &header, sizeof( header), 0)) == -1) 
	{
			   perror("recv");
			   exit(1);
	}
	//std::pair<uuid_t, timeval>timePair = std::make_pair<uuid_t, timeval>(header.packetID, header.packetTime);
	pendingTimes.push(header.packetTime);
	imageSize = header.imageHeight*header.imageWidth*header.imageComponents;

	glGenBuffers(1, &pixelBuffer); 
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pixelBuffer);
	glBufferData(GL_PIXEL_UNPACK_BUFFER,  imageSize, NULL, GL_STREAM_DRAW);  
	input = (unsigned char *)glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
	do
	{
	   if ( (receivedBytes = recv(socketNum, &input[total],  imageSize, 0)) == -1) 
	   {
			   perror("recv");
			   std::cout << "input: " << &input << " OpenGL error: " << glGetError() << std::endl;
			   exit(1);
	   }
	   total += receivedBytes;
	} 
	while(total <  imageSize);
	glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER); 
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, header.imageWidth, header.imageHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); 	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); 
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE); 
	/* redisplay */
	glFinish();
	glutPostRedisplay();
}

void
keyboardHandler(unsigned char key, int x, int y)
{
	keyPacket thisKey;
	thisKey.packetType =  htons(NORMAL_KEY_PACKET);
	thisKey.key = htons(key);
	thisKey.modifier = htons(glutGetModifiers());
	send(openGLSocket, &thisKey, sizeof(thisKey), 0);
}

void
specialKeyHandler(int key, int x, int y)
{
	keyPacket thisKey;
	thisKey.packetType = htons(SPECIAL_KEY_PACKET);
	thisKey.key = htons(key);
	thisKey.modifier = htons(glutGetModifiers());
	send(openGLSocket, &thisKey, sizeof(thisKey), 0);
}

void writeToDisk(int signal)
{
	std::fstream fs;
	fs.open("results.txt", std::fstream::out | std::fstream::trunc);
	while(completeTimes.size() > 0)
	{
		fs << completeTimes.front() << std::endl;
		completeTimes.pop();
	}
	fs << "final average: " << floatingAvgMs << " ms." << std::endl;
	fs.close();
	close(openGLSocket);
	exit(0);
}

void glutWriteToDisk()
{
	writeToDisk(0);
}

int
main(int argc, char *argv[])
{
  signal(SIGTERM, &writeToDisk);
  signal(SIGINT, &writeToDisk);
  if(argc < 2)
  {
	  std::cout << "no IP address specified, defaulting to loopback..." << std::endl;
	  openGLSocket = setupSocket("127.0.0.1", 8999);
  }
  else
  {
	  std::cout << "using provided address: " << argv[1] << std::endl;
	  openGLSocket = setupSocket(argv[1], 8999);
  }
  init();
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_SINGLE | GLUT_RGBA);
  glutInitWindowSize((int) width, (int) height);
  wd = glutCreateWindow("Client Buffer" /* title */ );
  glutDisplayFunc(display);
  glutKeyboardFunc(keyboardHandler);
  glutSpecialFunc(specialKeyHandler);
  glutReshapeFunc(reshape);
  glutIdleFunc(socketHandler);
  glutCloseFunc(glutWriteToDisk);
#ifndef __APPLE__
  glewInit();
#endif
  /* start the GLUT main loop */
  glutMainLoop();
  return 0;
}
