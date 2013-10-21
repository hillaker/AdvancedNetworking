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

#include "ga-common.h"
#include "ga-conf.h"
#include "ga-module.h"
#include "rtspconf.h"
#include "server.h"
#include "controller.h"
#include "encoder-common.h"

// image source pipeline:
//	vsource -- [vsource-%d] --> filter -- [filter-%d] --> encoder

static const char *imagepipefmt = "image-%d";
static const char *imagepipe0 = "image-0";
static const char *filterpipe0 = "filter-0";

static struct gaRect *prect = NULL;
static struct gaRect rect;

static struct ga_module *m_vsource, *m_filter, *m_vencoder, *m_asource, *m_aencoder, *m_ctrl;

int
load_modules() {
	if((m_vsource = ga_load_module("mod/vsource-desktop", "vsource_")) == NULL)
		return -1;
	if((m_filter = ga_load_module("mod/filter-rgb2yuv", "filter_RGB2YUV_")) == NULL)
		return -1;
	if((m_vencoder = ga_load_module("mod/encoder-video", "vencoder_")) == NULL)
		return -1;
	if(ga_conf_readbool("enable-audio", 1) != 0) {
	//////////////////////////
#ifndef __APPLE__
	if((m_asource = ga_load_module("mod/asource-system", "asource_")) == NULL)
		return -1;
#endif
	if((m_aencoder = ga_load_module("mod/encoder-audio", "aencoder_")) == NULL)
		return -1;
	//////////////////////////
	}
	if((m_ctrl = ga_load_module("mod/ctrl-sdl", "sdlmsg_replay_")) == NULL)
		return -1;
	return 0;
}

int
init_modules() {
	struct RTSPConf *conf = rtspconf_global();
	static const void *vsourcearg[] = { (void*) imagepipefmt, (void*) prect };
	static const char *filterpipe[] = { imagepipe0, filterpipe0 };
	if(conf->ctrlenable) {
		ga_init_single_module_or_quit("controller", m_ctrl, (void *) prect);
	}
	// controller server is built-in - no need to init
	// note the order of the two modules ...
	ga_init_single_module_or_quit("image source", m_vsource, (void*) /*imagepipefmt*/vsourcearg);
	ga_init_single_module_or_quit("filter", m_filter, (void*) filterpipe);
	//
	ga_init_single_module_or_quit("video encoder", m_vencoder, NULL);
	if(ga_conf_readbool("enable-audio", 1) != 0) {
	//////////////////////////
#ifndef __APPLE__
	ga_init_single_module_or_quit("audio source", m_asource, NULL);
#endif
	ga_init_single_module_or_quit("audio encoder", m_aencoder, NULL);
	//////////////////////////
	}
	return 0;
}

int
run_modules() {
	struct RTSPConf *conf = rtspconf_global();
	static const char *filterpipe[] =  { imagepipe0, filterpipe0 };
	// controller server is built-in, but replay is a module
	if(conf->ctrlenable) {
		ga_run_single_module_or_quit("control server", ctrl_server_thread, conf);
		ga_run_single_module_or_quit("control replayer", m_ctrl->threadproc, conf);
	}
	// video
	ga_run_single_module_or_quit("image source", m_vsource->threadproc, (void*) imagepipefmt);
	ga_run_single_module_or_quit("filter 0", m_filter->threadproc, (void*) filterpipe);
	encoder_register_vencoder(m_vencoder->threadproc, (void*) filterpipe0);
	// audio
	if(ga_conf_readbool("enable-audio", 1) != 0) {
	//////////////////////////
#ifndef __APPLE__
	ga_run_single_module_or_quit("audio source", m_asource->threadproc, NULL);
#endif
	encoder_register_aencoder(m_aencoder->threadproc, NULL);
	//////////////////////////
	}
	return 0;
}

int
main(int argc, char *argv[]) {
	int notRunning = 0;
#ifdef WIN32
	if(CoInitializeEx(NULL, COINIT_MULTITHREADED) < 0) {
		fprintf(stderr, "cannot initialize COM.\n");
		return -1;
	}
#endif
	//
	if(argc < 2) {
		fprintf(stderr, "usage: %s config-file\n", argv[0]);
		return -1;
	}
	//
	if(ga_init(argv[1], NULL) < 0)	{ return -1; }
	//
	ga_openlog();
	//
	if(rtspconf_parse(rtspconf_global()) < 0)
					{ return -1; }
	//
	prect = NULL;
	//
	if(ga_crop_window(&rect, &prect) < 0) {
		return -1;
	} else if(prect == NULL) {
		ga_error("*** Crop disabled.\n");
	} else if(prect != NULL) {
		ga_error("*** Crop enabled: (%d,%d)-(%d,%d)\n", 
			prect->left, prect->top,
			prect->right, prect->bottom);
	}
	//
	if(load_modules() < 0)	 	{ return -1; }
	if(init_modules() < 0)	 	{ return -1; }
	if(run_modules() < 0)	 	{ return -1; }
	//
	rtspserver_main(NULL);
	// alternatively, it is able to create a thread to run rtspserver_main:
	//	pthread_create(&t, NULL, rtspserver_main, NULL);
	//
	ga_deinit();
	//
	return 0;
}

