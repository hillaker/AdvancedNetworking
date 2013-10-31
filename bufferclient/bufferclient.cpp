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
#include <stdlib.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif

#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <SOIL/SOIL.h>
#include <GL/glew.h>
#include <GL/glut.h>
#endif

#define NENDS 2           /* number of end "points" to draw */
#define ENDHWIDTH 5       /* half width of end "points," in pixels */
 
GLdouble width, height;   /* window width and height */
int wd;                   /* GLUT window handle */
int ends[NENDS][2];       /* array of 2D points */
int lastx, lasty;         /* where was the mouse last clicked? */
int end;                  /* which of the above points was
                             the last mouse click close to?
                             -1 if neither */
int drift;                /* whether to drift the coordinate system */
double offset=0.0;

/* Program initialization NOT OpenGL/GLUT dependent,
   as we haven't created a GLUT window yet */
void
init(void)
{
  drift = 1;
  width  = 1280.0;                 /* initial window width and height, */
  height = 800.0;                  /* within which we draw. */
  ends[0][0] = (int)(0.25*width);  /* (0,0) is the lower left corner */
  ends[0][1] = (int)(0.75*height);
  ends[1][0] = (int)(0.75*width);
  ends[1][1] = (int)(0.25*height);
}

/* Callback functions for GLUT */

/* Draw the window - this is where all the GL actions are */
#if 0
void
display(void)
{

  /* clear the screen to white */

     /* set the clearing color (color to clear to) to white (default black) */
  glClearColor(1.0, 1.0, 1.0, 0.0);
     /* actually perform clearing of the color buffer */
  glClear(GL_COLOR_BUFFER_BIT);

  /* draw a black line of width 3 pixels
     between the two points stored in the ends[] global array */

     /* set the drawing color to black (default white) */
  glColor3f(0.0, 0.0, 0.0);
     /* set point size to 5 so that the endpoints are visible */
  glPointSize(5.0);
     /* Task 3: set line thickness to 3 pixels (default 1) */
  glLineWidth(3.0);
     /* draw the line */
	/* Tasks 1 and 2 */
	glBegin(GL_LINES); 
	 glVertex2f(50,50); 
	 glVertex2f(150,150); 
	glEnd(); 

  /* Task 4: draw a red square of width 2*ENDHWIDTH pixels
     around each ends[] to highlight the end points. */
     /* set the squares' color to red */
     /* form quadrilaterals around each ends[]
        - each quad takes a group of 4 points */
    glColor3f(1.0,0.0, 0.0);
	glBegin(GL_QUADS);
		glVertex2f(50+(2*ENDHWIDTH), 50-(2*ENDHWIDTH));
		glVertex2f(50-(2*ENDHWIDTH), 50-(2*ENDHWIDTH));
		glVertex2f(50-(2*ENDHWIDTH), 50+(2*ENDHWIDTH));
		glVertex2f(50+(2*ENDHWIDTH), 50+(2*ENDHWIDTH));
	glEnd();
	glBegin(GL_QUADS);
		glVertex2f(150+(2*ENDHWIDTH), 150-(2*ENDHWIDTH));
		glVertex2f(150-(2*ENDHWIDTH), 150-(2*ENDHWIDTH));
		glVertex2f(150-(2*ENDHWIDTH), 150+(2*ENDHWIDTH));
		glVertex2f(150+(2*ENDHWIDTH), 150+(2*ENDHWIDTH));
	glEnd();

  /* Task 5: draw a thin (1 pixel) green line on the x- and y-axes */
	 glColor3f(0.0,1.0, 0.0);
	 glLineWidth(1.0);
	 glBegin(GL_LINES);
		glVertex2f(-10000, 0);
		glVertex2f(10000, 0);
	 glEnd();
	 glBegin(GL_LINES);
		glVertex2f(0, -10000);
		glVertex2f(0, 10000);
	 glEnd();
	 
  /* Task 8: draw a 10x10 blue filled square around the Origin, *
   *         use glPushAttrib() and glPopAttrib().              */

  /* force drawing to start */
  glFlush();
  /*glutSwapBuffers();*/
}
#endif
#if 1
void display(void)
{
  glLoadIdentity();
  std::cout << "called display()\n";
  //glClearColor(0.0f, 0.0f, 0.0f, 0.5f);
  glClear(GL_COLOR_BUFFER_BIT);
  
 /* glColor3f(0.0,1.0, 0.0);
	 glLineWidth(5.0);
	 glBegin(GL_LINES);
		glVertex2f(-10000, 0);
		glVertex2f(10000, 0);
	 glEnd();
	 glBegin(GL_LINES);
		glVertex2f(0, -10000);
		glVertex2f(0, 10000);
	 glEnd();*/
  //int width, height, channels;
  GLuint fb;
  
  GLuint texture;
  glGenTextures(1, &texture);
  glGenFramebuffers(1, &fb);

  //glBindFramebuffer(GL_FRAMEBUFFER, fb);
  //unsigned char* ship = SOIL_load_image("ShipatSea.jpg", &width, &height, &channels, SOIL_LOAD_RGB);
  texture = SOIL_load_OGL_texture("ShipatSea.jpg", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_INVERT_Y);
  glBindTexture(GL_TEXTURE_2D, texture);
 // glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, ship);
  
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); 
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT); 
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); 	
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); 
  
  
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE); 
  glEnable(GL_TEXTURE_2D); 
 
  glBegin(GL_QUADS); 
	glTexCoord2f(1.0,1.0); glVertex3f(1.0, 1.0, 0.0);
	glTexCoord2f(0.0,1.0); glVertex3f(-1.0, 1.0, 0.0);
	glTexCoord2f(0.0,0.0); glVertex3f(-1.0, -1.0, 0.0);
	glTexCoord2f(1.0,0.0); glVertex3f(1.0, -1.0, 0.0);
  glEnd(); 
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
  glDisable(GL_TEXTURE_2D); // state machine! 

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glDrawBuffer(GL_BACK);
  glDeleteFramebuffers(1, &fb);
  //SOIL_free_image_data(ship);
  
  /* force drawing to start */
  glFlush();
 // glutSwapBuffers();
}
#endif
void dummy(void)
{
	return;
}
/* Called when window is resized,
   also when window is first created,
   before the first call to display(). */
void
reshape(int w, int h)
{
  // save new screen dimensions 
  width = (GLdouble) w;
  height = (GLdouble) h;

  // tell OpenGL to use the whole window for drawing 
  glViewport(0, 0, (GLsizei) w, (GLsizei) h);

  // do an orthographic parallel projection with the coordinate
  //   system set to first quadrant, limited by screen/window size 
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(0.0, width, 0.0, height);
}

/* Refresh the display, called when system is idle */
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

    if (drift) {
      // shift the coordinate system by (-1, -1) every 100 ms 
      offset -= 1.0;
      glMatrixMode(GL_PROJECTION);
      glLoadIdentity();
      gluOrtho2D(offset, width+offset, offset, height+offset);
    }     

    // redisplay 
    glutPostRedisplay();
  }
}

/* Trap and process keyboard events */
void
kbd(unsigned char key, int x, int y)
{

}

/* Trap and process keyboard events */
/*void
kbd(unsigned char key, int x, int y)
{
  switch((char)key) {
  case 'q':
  case 27:    // ESC 
    glutDestroyWindow(wd);
    exit(0);
    break;
  case 'a':
    drift = drift ? 0 : 1;
    break;
  case 'b':   // Task 7: draw outline squares at both end points 
    // draw only the boundary/edges of the end squares, 1 pixel wide. 
       // set edge line width to 1 pixel 
       glLineWidth(1.0);
       //set front facing polygon to draw only line, no fill, 
       //   use glPolygonMode(). 
       glPolygonMode(GL_FRONT, GL_LINE); 
       // redisplay, see how it's done in refresh() 
       glutPostRedisplay();
    break;
  }
}*/

/*int
find_closest_end(int x, int y)
{
  int n;
  for (n = 0; n < NENDS; ++n) {
    if (abs(x - ends[n][0]) <= ENDHWIDTH && abs(y - ends[n][1]) <= ENDHWIDTH) {
      return (n);
    }
  }
  
  return (-1);
}*/

/* Trap and process "mouse moved and button NOT pressed" events */
/*void
move(int x, int y)
{
  static int cursor = GLUT_CURSOR_LEFT_ARROW;
  int ncursor;

  end = find_closest_end(x+(int)offset, (int)height - y+(int)offset);

  ncursor = (end < 0) ? GLUT_CURSOR_LEFT_ARROW : GLUT_CURSOR_CROSSHAIR;
  if (cursor != ncursor) {
    cursor = ncursor;
    glutSetCursor(cursor);
  }

  return;
}*/

void
move(int x, int y)
{
  return;
}

/* Trap and process mouse click (down and up) events */
/*void
click(int button, int state, int x, int y)
{
  // Trap left-clicks 
  if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN && end >= 0) {
    lastx = x;
    lasty = y;
  }

  return;
}*/
void click(int button, int state, int x, int y)
{
  return;
}

/* Trap and process "mouse moved with button pressed" events */
/*void
drag(int x, int y)
{
  if (end >= 0) {
    ends[end][0] += (x - lastx);
    ends[end][1] -= (y - lasty);
    lastx = x;
    lasty = y;
    glutPostRedisplay();
  }

  return;
}*/

void
drag(int x, int y)
{
  return;
}

void
initGL()
{
  /* clear with a pale blue */
  glClearColor(0.7f, 0.8f, 1.0f, 1.0f);
        
  /* enable depth testing */
  glEnable(GL_DEPTH_TEST);
  //glDepthFunc(GL_LEQUAL);
        
  /* setup one light */
  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION, 0.5f);
        
  /* set a polygon offset. its use is disabled
     by default, but it will be enabled later. it
     puts an offset into the depth buffer, used
     here to avoid z-fighting when drawing shadows */
  glPolygonOffset(-1.0f, -5.0f);
        
  /* let OpenGL calculate material properties
     from calls to glColor instead of glMaterial */
  glEnable(GL_COLOR_MATERIAL);
        
  /* load a simple projection (it will not
     be changed anywhere else) */
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glFrustum(-1.0, 1.0, -1.0, 1.0, 2.0, 7.0);
}

int
main(int argc, char *argv[])
{
	/* perform initialization NOT OpenGL/GLUT dependent,
       as we haven't created a GLUT window yet */
  init();
  
	/* initialize GLUT, let it extract command-line 
       GLUT options that you may provide 
       - NOTE THE '&' BEFORE argc */
	glutInit(&argc, argv);
	/* specify the display to be single 
       buffered and color as RGBA values */
  glutInitDisplayMode(GLUT_SINGLE | GLUT_RGBA);

  /* set the initial window size */
  glutInitWindowSize((int) width, (int) height);

  /* create the window and store the handle to it */
  wd = glutCreateWindow("Client Buffer" /* title */ );

  /* --- register callbacks with GLUT --- */

  /* register function that draws in the window */
  glutDisplayFunc(display);

  /* register mouse-moved movement event callback */
  glutPassiveMotionFunc(move);

  /* register mouse-click event callback */
  glutMouseFunc(click);
  /* register mouse-drag event callback */  
  glutMotionFunc(drag);
  
  initGL();
  glewInit();
  /* start the GLUT main loop */
  glutMainLoop();

  return 0;
}
