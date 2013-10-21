/*
 * Copyright (c) 2013 Chun-Ying Huang
 *
 * This file is part of GamingAnywhere (GA).
 *
 * GA is free software; you can redistribute it and/or modify it
 * under the terms of the 3-clause BSD License as published by the
 * Free Software Foundation: http://directory.fsf.org/wiki/License:BSD_3Clause
 *
 * GA is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * You should have received a copy of the 3-clause BSD License along with GA;
 * if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#ifndef WIN32
#include <dlfcn.h>
#endif

#include "ga-common.h"
#include "ga-conf.h"
#include "vsource.h"
#include "pipeline.h"

#include "ga-hook-common.h"
#include "ga-hook-gl.h"

#include <map>
using namespace std;

#ifndef WIN32
#ifdef __cplusplus
extern "C" {
#endif
void glFlush();
#ifdef __cplusplus
}
#endif
#endif

// for hooking
t_glFlush		old_glFlush = NULL;

static void
gl_global_init() {
	static int initialized = 0;
	pthread_t t;
	if(initialized != 0)
		return;
	//
	if(pthread_create(&t, NULL, ga_server, NULL) != 0) {
		ga_error("ga_hook: create thread failed.\n");
		exit(-1);
	}

	initialized = 1;

	return;
}

static void
gl_hook_symbols() {
#ifndef WIN32
	void *handle = NULL;
	char *ptr, soname[2048];
	if((ptr = getenv("LIBGL_SO")) == NULL) {
		strncpy(soname, "libGL.so.1", sizeof(soname));
	} else {
		strncpy(soname, ptr, sizeof(soname));
	}
	if((handle = dlopen(soname, RTLD_LAZY)) == NULL) {
		ga_error("dlopen '%s' failed: %s\n", soname, strerror(errno));
		exit(-1);
	}
	// for hooking
	old_glFlush = (t_glFlush)
				ga_hook_lookup_or_quit(handle, "glFlush");
#endif
	return;
}

void
hook_glFlush() {
	static int frame_interval;
	static struct timeval initialTv, captureTv;
	static int frameLinesize;
	static unsigned char *frameBuf;
	static int sb_initialized = 0;
	static int global_initialized = 0;
	//
	GLint vp[4];
	int vp_x, vp_y, vp_width, vp_height;
	int i;
	//
	struct pooldata *data;
	struct vsource_frame *frame;
	//
	if(global_initialized == 0) {
		gl_global_init();
		global_initialized = 1;
	}
	//
	if(old_glFlush == NULL) {
		gl_hook_symbols();
	}
	old_glFlush();
	// capture the screen
	glGetIntegerv(GL_VIEWPORT, vp);
	vp_x = vp[0];
	vp_y = vp[1];
	vp_width = vp[2];
	vp_height = vp[3];		
	//
	if(ga_hook_capture_prepared(vp_width, vp_height, 1) < 0)
		return;
	//
	if(sb_initialized == 0) {
		frame_interval = 1000000/video_fps; // in the unif of us
		frame_interval++;
		gettimeofday(&initialTv, NULL);
		frameBuf = (unsigned char*) malloc(encoder_width * encoder_height * 4);
		if(frameBuf == NULL) {
			ga_error("allocate frame failed.\n");
			return;
		}
		frameLinesize = game_width * 4;
		sb_initialized = 1;
	} else {
		gettimeofday(&captureTv, NULL);
	}
	//
	if (enable_server_rate_control && ga_hook_video_rate_control() < 0)
		return;
	//
	do {
		unsigned char *src, *dst;
		//
		frameLinesize = vp_width<<2;
		//
		data = g_pipe[0]->allocate_data();
		frame = (struct vsource_frame*) data->ptr;
		frame->pixelformat = PIX_FMT_RGBA;
		frame->realwidth = vp_width;
		frame->realheight = vp_height;
		frame->realstride = frameLinesize;
		frame->realsize = vp_height/*vp_width*/ * frameLinesize;
		frame->linesize[0] = frameLinesize;/*frame->stride*/;
		// read a block of pixels from the framebuffer (backbuffer)
		glReadBuffer(GL_BACK);
		glReadPixels(vp_x, vp_y, vp_width, vp_height, GL_RGBA, GL_UNSIGNED_BYTE, frameBuf);
		// image is upside down!
		src = frameBuf + frameLinesize * (vp_height - 1);
		dst = frame->imgbuf;
		for(i = 0; i < frame->realheight; i++) {
			bcopy(src, dst, frameLinesize);
			dst += frameLinesize/*frame->stride*/;
			src -= frameLinesize;
		}
		frame->imgpts = tvdiff_us(&captureTv, &initialTv)/frame_interval;
	} while(0);
	// duplicate from channel 0 to other channels
	ga_hook_capture_dupframe(frame);
	g_pipe[0]->store_data(data);
	g_pipe[0]->notify_all();		
	//
	return;
}

#ifndef WIN32	/* POSIX interfaces */
void
glFlush() {
	hook_glFlush();
}

__attribute__((constructor))
static void
gl_hook_loaded(void) {
	ga_error("ga-hook-gl loaded!\n");
	if(ga_hook_init() < 0) {
		ga_error("ga_hook: init failed.\n");
		exit(-1);
	}
	return;
}
#endif /* ! WIN32 */

