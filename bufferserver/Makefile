CC = g++
MKDEP=/usr/X11R6/bin/makedepend -Y
OS := $(shell uname)
ifeq ($(OS), Darwin)
	LIBS = -framework OpenGL -framework GLUT -lc -lm
  CFLAGS = -g -Wall -m32
else ifeq ($(OS), Linux)
  LIBS = -lXext -lX11 -lpthread -lGL -lGLU -lglut -lm -luuid
  CFLAGS = -g -O0 -Wall -Wno-deprecated -I./include -I./common/include -I../buffercommon/include
else
	LIBS = -lGL -lGLU -lglut -lc -lm
  CFLAGS = -g -Wall -Wno-deprecated -m32
endif

HDRS =
SRCS =
HDRS_SLN = 
SRCS_SLN = bufferserver.cpp imageCapture.cpp
OBJS = $(patsubst %.cpp, %.o, $(SRCS)) $(patsubst %.cpp,%.o,$(SRCS_SLN))
ifeq ($(OS), Linux)
  GLEW = ./common/src/glew.o
endif

bufferserver: $(OBJS) $(GLEW)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LIBS) $(GLEW)

%.o: %.cpp
	$(CC) $(CFLAGS) $(INCLUDES) -c $<

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

.PHONY: clean
clean: 
	-rm -f -r $(OBJS) *.o *~ *core* bufferclient

depend: $(SRCS) $(SRCS_SLN) $(HDRS) $(HDRS_SLN) Makefile
	$(MKDEP) $(CFLAGS) $(SRCS) $(SRCS_SLN) $(HDRS) $(HDRS_SLN) >& /dev/null

# DO NOT DELETE

shadows.o: xvec.h xmat.h
xmat.o: xvec.h
