/*
 * Copyright (c) 2009 University of Michigan, Ann Arbor.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of Michigan, Ann Arbor. The name of the University 
 * may not be used to endorse or promote products derived from this 
 * software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * This lab consists of the following tasks:
 *   1. Draw two points on the screen
 *   2. Connect the two points into a line
 *   3. Change the line to 3-pixel thick
 *   4. Draw filled squares at both end points:
 *      - perturb coordinates, use GL_QUADS
 *   5. Draw a thin line at the x- and y-axes 
 *   6. Register the provided refresh() function that shifts the coordinate 
 *      system by (-1, -1) every 100 ms when the system is idle
 *   7. Add keyboard command `b' to draw outline squares at both end points
 *      Use glPolygonMode().
 *   8. Draw a filled square around the Origin, 
 *      use glPushAttrib() and glPopAttrib().
 * 
 * Authors: Manoj Rajagopalan, Sugih Jamin
*/
#include <stdio.h>
#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "protocol.h"
#include "imageCapture.h"
#include <pthread.h>
#include <sys/time.h>

#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glew.h>
#include <GL/glut.h>
#endif

#define NENDS 2           /* number of end "points" to draw */
#define ENDHWIDTH 5       /* half width of end "points," in pixels */
 
GLdouble width, height;   /* window width and height */

void dummy(void) //TODO: can't remove this for some reason, need to look into it when there's time-- looks like make is removing required libraries
{
  glDisable(GL_TEXTURE_2D); 
}

void *serverHandler(void * args)
{
	char test[150];
	long long sentBytes;
	int sd_current = *(int *)args;
	
	while(1)
	{
		packetHeader header;
		char * input = captureImage(header.imageHeight, header.imageWidth, header.imageComponents);
		int imageSize = header.imageHeight*header.imageWidth*header.imageComponents;
		uuid_generate(header.packetID);
		gettimeofday(&header.packetTime, NULL); 
		memset(test, 0, sizeof(test));
		/* get a message from the client */
		if (recv(sd_current, test, sizeof(test), 0) == -1) {
			perror("recv");
			exit(1);
		}
		//unsigned char* ship = SOIL_load_image("ShipatSea.jpg", &width, &height, &channels, SOIL_LOAD_RGB);
		if( (sentBytes = send(sd_current, &header, sizeof(header), 0)) == -1)
		{
			perror("send failed");
			exit(1);
		}
		//if( (sentBytes = send(sd_current, raw_image, imageSize, 0)) == -1)
		if( (sentBytes = send(sd_current, input, imageSize, 0)) == -1)
		{
			perror("send failed");
			exit(1);
		}
		free(input);
		usleep(1);
	}
	close(sd_current);
}

int setupServer(int port)
{	
	int 	 sd, sd_current;
	//int width, height, channels;
	socklen_t addrlen;
	struct   sockaddr_in sin;
	struct   sockaddr_in pin;
 
	/* get an internet domain socket */
	if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		exit(1);
	}

	/* complete the socket structure */
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons(port);

	/* bind the socket to the port number */
	if (bind(sd, (struct sockaddr *) &sin, sizeof(sin)) == -1) {
		perror("bind");
		exit(1);
	}

	/* show that we are willing to listen */
	if (listen(sd, 5) == -1) {
		perror("listen");
		exit(1);
	}
	/* wait for a client to talk to us */
        addrlen = sizeof(pin); 
	if ((sd_current = accept(sd, (sockaddr *)&pin, &addrlen)) == -1) {
		perror("accept");
		exit(1);
	}
	return sd_current;

}

int
main(int argc, char *argv[])
{ 
	int openGLSocket;
	pthread_t handlingThread;
	initializeCapture();
	openGLSocket = setupServer(8999);
	pthread_create(&handlingThread, NULL, serverHandler, &openGLSocket);
	while(1)
	{
	  sleep(1);
	}
	std::cout << "about to close socket" << std::endl;
	close(openGLSocket);
	return 0;
}
