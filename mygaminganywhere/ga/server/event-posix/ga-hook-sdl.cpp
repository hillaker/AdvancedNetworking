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
#include "asource.h"
#include "vsource.h"
#include "pipeline.h"
#include "controller.h"
#include "ctrl-sdl.h"

#include "ga-hook-common.h"
#include "ga-hook-sdl.h"

#include <GL/glu.h>

#include <map>
using namespace std;

#ifndef WIN32
#ifdef __cplusplus
extern "C" {
#endif
int SDL_Init(unsigned int flags);
SDL12_Surface * SDL_SetVideoMode(int width, int height, int bpp, uint32_t flags);
int SDL_BlitSurface(SDL12_Surface *src, SDL12_Rect *srcrect, SDL12_Surface *dst, SDL12_Rect *dstrect);
int SDL_UpperBlit(SDL12_Surface *src, SDL12_Rect *srcrect, SDL12_Surface *dst, SDL12_Rect *dstrect);
int SDL_Flip(SDL12_Surface *screen);
void SDL_UpdateRect(SDL12_Surface *screen, int32_t x, int32_t y, int32_t w, int32_t h);
void SDL_UpdateRects(SDL12_Surface *screen, int numrects, SDL12_Rect *rects);
void SDL_FreeSurface(SDL12_Surface *surface);
void SDL_GL_SwapBuffers();
int SDL_PollEvent(SDL12_Event *event);
int SDL_WaitEvent(SDL12_Event *event);
int SDL_PeepEvents(SDL12_Event *event, int numevents, SDL12_eventaction action, uint32_t mask);
int SDL_OpenAudio(SDL12_AudioSpec *desired, SDL12_AudioSpec *obtained);
void SDL_PauseAudio(int pause_on);
void SDL_CloseAudio();
#ifdef __cplusplus
}
#endif
#endif

// for hooking
t_SDL_Init		old_SDL_Init = NULL;
t_SDL_SetVideoMode	old_SDL_SetVideoMode = NULL;
t_SDL_BlitSurface	old_SDL_BlitSurface = NULL;
t_SDL_BlitSurface	old_SDL_UpperBlit = NULL;
t_SDL_Flip		old_SDL_Flip = NULL;
t_SDL_UpdateRect	old_SDL_UpdateRect = NULL;
t_SDL_UpdateRects	old_SDL_UpdateRects = NULL;
t_SDL_GL_SwapBuffers	old_SDL_GL_SwapBuffers = NULL;
t_SDL_PollEvent		old_SDL_PollEvent = NULL;
t_SDL_WaitEvent		old_SDL_WaitEvent = NULL;
t_SDL_PeepEvents	old_SDL_PeepEvents = NULL;
t_SDL_OpenAudio		old_SDL_OpenAudio = NULL;
t_SDL_PauseAudio	old_SDL_PauseAudio = NULL;
t_SDL_CloseAudio	old_SDL_CloseAudio = NULL;
// for internal use
t_SDL_CreateRGBSurface	old_SDL_CreateRGBSurface = NULL;
t_SDL_PushEvent		old_SDL_PushEvent = NULL;
t_SDL_FreeSurface	old_SDL_FreeSurface = NULL;

static SDL12_Surface	*screensurface = NULL;
static SDL12_Surface	*dupsurface = NULL;

static map<int,int> sdl_keymap;
static map<int,unsigned short> sdl_unicodemap;
static map<unsigned short,unsigned short> sdl_shiftmap;

int
sdl_hook_init() {
	static int initialized = 0;
	pthread_t t;
	//
	if(initialized != 0)
		return 0;
	// override controller
	sdl12_mapinit();
	ctrl_server_setreplay(sdl_hook_replay_callback);
	no_default_controller = 1;
	//
	if(pthread_create(&t, NULL, ga_server, NULL) != 0) {
		ga_error("ga_hook: create thread failed.\n");
		exit(-1);
	}
	pthread_detach(t);
	//
	initialized = 1;
	//
	ga_error("SDL hook: initialized.\n");
	//
	return 0;
}

static void
sdl_hook_symbols() {
#ifndef WIN32
	void *handle = NULL;
	char *ptr, soname[2048];
	if((ptr = getenv("LIBSDL_SO")) == NULL) {
		strncpy(soname, "libSDL-1.2.so.0", sizeof(soname));
	} else {
		strncpy(soname, ptr, sizeof(soname));
	}
	if((handle = dlopen(soname, RTLD_LAZY)) == NULL) {
		ga_error("dlopen '%s' failed: %s\n", soname, strerror(errno));
		exit(-1);
	}
	// in case SDL_Init hook failed ...
	sdl_hook_init();
	// for hooking
	old_SDL_Init = (t_SDL_Init)
				ga_hook_lookup_or_quit(handle, "SDL_Init");
	old_SDL_SetVideoMode = (t_SDL_SetVideoMode)
				ga_hook_lookup_or_quit(handle, "SDL_SetVideoMode");
	old_SDL_UpperBlit = (t_SDL_BlitSurface)
				ga_hook_lookup_or_quit(handle, "SDL_UpperBlit");
	old_SDL_BlitSurface = (t_SDL_BlitSurface)
				ga_hook_lookup(handle, "SDL_BlitSurface");
	if(old_SDL_BlitSurface == NULL) {
		old_SDL_BlitSurface = old_SDL_UpperBlit;
		ga_error("SDL hook: BlitSurface == UpperBlit.\n");
	} else {
		ga_error("SDL hook: BlitSurface != UpperBlit.\n");
	}
	old_SDL_Flip = (t_SDL_Flip)
				ga_hook_lookup_or_quit(handle, "SDL_Flip");
	old_SDL_UpdateRect = (t_SDL_UpdateRect)
				ga_hook_lookup_or_quit(handle, "SDL_UpdateRect");
	old_SDL_UpdateRects = (t_SDL_UpdateRects)
				ga_hook_lookup_or_quit(handle, "SDL_UpdateRects");
	old_SDL_GL_SwapBuffers = (t_SDL_GL_SwapBuffers)
				ga_hook_lookup_or_quit(handle, "SDL_GL_SwapBuffers");
	old_SDL_PollEvent = (t_SDL_PollEvent)
				ga_hook_lookup_or_quit(handle, "SDL_PollEvent");
	old_SDL_WaitEvent = (t_SDL_WaitEvent)
				ga_hook_lookup_or_quit(handle, "SDL_WaitEvent");
	old_SDL_PeepEvents = (t_SDL_PeepEvents)
				ga_hook_lookup_or_quit(handle, "SDL_PeepEvents");
	old_SDL_OpenAudio = (t_SDL_OpenAudio)
				ga_hook_lookup_or_quit(handle, "SDL_OpenAudio");
	old_SDL_PauseAudio = (t_SDL_PauseAudio)
				ga_hook_lookup_or_quit(handle, "SDL_PauseAudio");
	old_SDL_CloseAudio = (t_SDL_CloseAudio)
				ga_hook_lookup_or_quit(handle, "SDL_CloseAudio");
	// for internal use
	old_SDL_CreateRGBSurface = (t_SDL_CreateRGBSurface)
				ga_hook_lookup_or_quit(handle, "SDL_CreateRGBSurface");
	old_SDL_FreeSurface = (t_SDL_FreeSurface)
				ga_hook_lookup_or_quit(handle, "SDL_FreeSurface");
	old_SDL_PushEvent = (t_SDL_PushEvent)
				ga_hook_lookup_or_quit(handle, "SDL_PushEvent");
#endif
	return;
}

static void
sdl12_keyinit() {
	sdl_keymap.clear();
	sdl_keymap[SDLK_UNKNOWN]	= SDLK12_UNKNOWN;
	//sdl_keymap[SDLK_FIRST]	= SDLK12_FIRST;
	sdl_keymap[SDLK_BACKSPACE]	= SDLK12_BACKSPACE;
	sdl_keymap[SDLK_TAB]		= SDLK12_TAB;
	sdl_keymap[SDLK_CLEAR]		= SDLK12_CLEAR;
	sdl_keymap[SDLK_RETURN]		= SDLK12_RETURN;
	sdl_keymap[SDLK_PAUSE]		= SDLK12_PAUSE;
	sdl_keymap[SDLK_ESCAPE]		= SDLK12_ESCAPE;
	sdl_keymap[SDLK_SPACE]		= SDLK12_SPACE;
	sdl_keymap[SDLK_EXCLAIM]	= SDLK12_EXCLAIM;
	sdl_keymap[SDLK_QUOTEDBL]	= SDLK12_QUOTEDBL;
	sdl_keymap[SDLK_HASH]		= SDLK12_HASH;
	sdl_keymap[SDLK_DOLLAR]		= SDLK12_DOLLAR;
	sdl_keymap[SDLK_AMPERSAND]	= SDLK12_AMPERSAND;
	sdl_keymap[SDLK_QUOTE]		= SDLK12_QUOTE;
	sdl_keymap[SDLK_LEFTPAREN]	= SDLK12_LEFTPAREN;
	sdl_keymap[SDLK_RIGHTPAREN]	= SDLK12_RIGHTPAREN;
	sdl_keymap[SDLK_ASTERISK]	= SDLK12_ASTERISK;
	sdl_keymap[SDLK_PLUS]		= SDLK12_PLUS;
	sdl_keymap[SDLK_COMMA]		= SDLK12_COMMA;
	sdl_keymap[SDLK_MINUS]		= SDLK12_MINUS;
	sdl_keymap[SDLK_PERIOD]		= SDLK12_PERIOD;
	sdl_keymap[SDLK_SLASH]		= SDLK12_SLASH;
	sdl_keymap[SDLK_0]		= SDLK12_0;
	sdl_keymap[SDLK_1]		= SDLK12_1;
	sdl_keymap[SDLK_2]		= SDLK12_2;
	sdl_keymap[SDLK_3]		= SDLK12_3;
	sdl_keymap[SDLK_4]		= SDLK12_4;
	sdl_keymap[SDLK_5]		= SDLK12_5;
	sdl_keymap[SDLK_6]		= SDLK12_6;
	sdl_keymap[SDLK_7]		= SDLK12_7;
	sdl_keymap[SDLK_8]		= SDLK12_8;
	sdl_keymap[SDLK_9]		= SDLK12_9;
	sdl_keymap[SDLK_COLON]		= SDLK12_COLON;
	sdl_keymap[SDLK_SEMICOLON]	= SDLK12_SEMICOLON;
	sdl_keymap[SDLK_LESS]		= SDLK12_LESS;
	sdl_keymap[SDLK_EQUALS]		= SDLK12_EQUALS;
	sdl_keymap[SDLK_GREATER]	= SDLK12_GREATER;
	sdl_keymap[SDLK_QUESTION]	= SDLK12_QUESTION;
	sdl_keymap[SDLK_AT]		= SDLK12_AT;
	/* 
	   Skip uppercase letters
	 */
	sdl_keymap[SDLK_LEFTBRACKET]	= SDLK12_LEFTBRACKET;
	sdl_keymap[SDLK_BACKSLASH]	= SDLK12_BACKSLASH;
	sdl_keymap[SDLK_RIGHTBRACKET]	= SDLK12_RIGHTBRACKET;
	sdl_keymap[SDLK_CARET]		= SDLK12_CARET;
	sdl_keymap[SDLK_UNDERSCORE]	= SDLK12_UNDERSCORE;
	sdl_keymap[SDLK_BACKQUOTE]	= SDLK12_BACKQUOTE;
	sdl_keymap[SDLK_a]		= SDLK12_a;
	sdl_keymap[SDLK_b]		= SDLK12_b;
	sdl_keymap[SDLK_c]		= SDLK12_c;
	sdl_keymap[SDLK_d]		= SDLK12_d;
	sdl_keymap[SDLK_e]		= SDLK12_e;
	sdl_keymap[SDLK_f]		= SDLK12_f;
	sdl_keymap[SDLK_g]		= SDLK12_g;
	sdl_keymap[SDLK_h]		= SDLK12_h;
	sdl_keymap[SDLK_i]		= SDLK12_i;
	sdl_keymap[SDLK_j]		= SDLK12_j;
	sdl_keymap[SDLK_k]		= SDLK12_k;
	sdl_keymap[SDLK_l]		= SDLK12_l;
	sdl_keymap[SDLK_m]		= SDLK12_m;
	sdl_keymap[SDLK_n]		= SDLK12_n;
	sdl_keymap[SDLK_o]		= SDLK12_o;
	sdl_keymap[SDLK_p]		= SDLK12_p;
	sdl_keymap[SDLK_q]		= SDLK12_q;
	sdl_keymap[SDLK_r]		= SDLK12_r;
	sdl_keymap[SDLK_s]		= SDLK12_s;
	sdl_keymap[SDLK_t]		= SDLK12_t;
	sdl_keymap[SDLK_u]		= SDLK12_u;
	sdl_keymap[SDLK_v]		= SDLK12_v;
	sdl_keymap[SDLK_w]		= SDLK12_w;
	sdl_keymap[SDLK_x]		= SDLK12_x;
	sdl_keymap[SDLK_y]		= SDLK12_y;
	sdl_keymap[SDLK_z]		= SDLK12_z;
	sdl_keymap[SDLK_DELETE]		= SDLK12_DELETE;
	/* End of ASCII mapped keysyms */
        /*@}*/

#if 0
	/** @name International keyboard syms */
        /*@{*/
	sdl_keymap[SDLK_WORLD_0]	= SDLK12_WORLD_0;		/* 0xA0 */
	sdl_keymap[SDLK_WORLD_1]	= SDLK12_WORLD_1;
	sdl_keymap[SDLK_WORLD_2]	= SDLK12_WORLD_2;
	sdl_keymap[SDLK_WORLD_3]	= SDLK12_WORLD_3;
	sdl_keymap[SDLK_WORLD_4]	= SDLK12_WORLD_4;
	sdl_keymap[SDLK_WORLD_5]	= SDLK12_WORLD_5;
	sdl_keymap[SDLK_WORLD_6]	= SDLK12_WORLD_6;
	sdl_keymap[SDLK_WORLD_7]	= SDLK12_WORLD_7;
	sdl_keymap[SDLK_WORLD_8]	= SDLK12_WORLD_8;
	sdl_keymap[SDLK_WORLD_9]	= SDLK12_WORLD_9;
	sdl_keymap[SDLK_WORLD_10]	= SDLK12_WORLD_10;
	sdl_keymap[SDLK_WORLD_11]	= SDLK12_WORLD_11;
	sdl_keymap[SDLK_WORLD_12]	= SDLK12_WORLD_12;
	sdl_keymap[SDLK_WORLD_13]	= SDLK12_WORLD_13;
	sdl_keymap[SDLK_WORLD_14]	= SDLK12_WORLD_14;
	sdl_keymap[SDLK_WORLD_15]	= SDLK12_WORLD_15;
	sdl_keymap[SDLK_WORLD_16]	= SDLK12_WORLD_16;
	sdl_keymap[SDLK_WORLD_17]	= SDLK12_WORLD_17;
	sdl_keymap[SDLK_WORLD_18]	= SDLK12_WORLD_18;
	sdl_keymap[SDLK_WORLD_19]	= SDLK12_WORLD_19;
	sdl_keymap[SDLK_WORLD_20]	= SDLK12_WORLD_20;
	sdl_keymap[SDLK_WORLD_21]	= SDLK12_WORLD_21;
	sdl_keymap[SDLK_WORLD_22]	= SDLK12_WORLD_22;
	sdl_keymap[SDLK_WORLD_23]	= SDLK12_WORLD_23;
	sdl_keymap[SDLK_WORLD_24]	= SDLK12_WORLD_24;
	sdl_keymap[SDLK_WORLD_25]	= SDLK12_WORLD_25;
	sdl_keymap[SDLK_WORLD_26]	= SDLK12_WORLD_26;
	sdl_keymap[SDLK_WORLD_27]	= SDLK12_WORLD_27;
	sdl_keymap[SDLK_WORLD_28]	= SDLK12_WORLD_28;
	sdl_keymap[SDLK_WORLD_29]	= SDLK12_WORLD_29;
	sdl_keymap[SDLK_WORLD_30]	= SDLK12_WORLD_30;
	sdl_keymap[SDLK_WORLD_31]	= SDLK12_WORLD_31;
	sdl_keymap[SDLK_WORLD_32]	= SDLK12_WORLD_32;
	sdl_keymap[SDLK_WORLD_33]	= SDLK12_WORLD_33;
	sdl_keymap[SDLK_WORLD_34]	= SDLK12_WORLD_34;
	sdl_keymap[SDLK_WORLD_35]	= SDLK12_WORLD_35;
	sdl_keymap[SDLK_WORLD_36]	= SDLK12_WORLD_36;
	sdl_keymap[SDLK_WORLD_37]	= SDLK12_WORLD_37;
	sdl_keymap[SDLK_WORLD_38]	= SDLK12_WORLD_38;
	sdl_keymap[SDLK_WORLD_39]	= SDLK12_WORLD_39;
	sdl_keymap[SDLK_WORLD_40]	= SDLK12_WORLD_40;
	sdl_keymap[SDLK_WORLD_41]	= SDLK12_WORLD_41;
	sdl_keymap[SDLK_WORLD_42]	= SDLK12_WORLD_42;
	sdl_keymap[SDLK_WORLD_43]	= SDLK12_WORLD_43;
	sdl_keymap[SDLK_WORLD_44]	= SDLK12_WORLD_44;
	sdl_keymap[SDLK_WORLD_45]	= SDLK12_WORLD_45;
	sdl_keymap[SDLK_WORLD_46]	= SDLK12_WORLD_46;
	sdl_keymap[SDLK_WORLD_47]	= SDLK12_WORLD_47;
	sdl_keymap[SDLK_WORLD_48]	= SDLK12_WORLD_48;
	sdl_keymap[SDLK_WORLD_49]	= SDLK12_WORLD_49;
	sdl_keymap[SDLK_WORLD_50]	= SDLK12_WORLD_50;
	sdl_keymap[SDLK_WORLD_51]	= SDLK12_WORLD_51;
	sdl_keymap[SDLK_WORLD_52]	= SDLK12_WORLD_52;
	sdl_keymap[SDLK_WORLD_53]	= SDLK12_WORLD_53;
	sdl_keymap[SDLK_WORLD_54]	= SDLK12_WORLD_54;
	sdl_keymap[SDLK_WORLD_55]	= SDLK12_WORLD_55;
	sdl_keymap[SDLK_WORLD_56]	= SDLK12_WORLD_56;
	sdl_keymap[SDLK_WORLD_57]	= SDLK12_WORLD_57;
	sdl_keymap[SDLK_WORLD_58]	= SDLK12_WORLD_58;
	sdl_keymap[SDLK_WORLD_59]	= SDLK12_WORLD_59;
	sdl_keymap[SDLK_WORLD_60]	= SDLK12_WORLD_60;
	sdl_keymap[SDLK_WORLD_61]	= SDLK12_WORLD_61;
	sdl_keymap[SDLK_WORLD_62]	= SDLK12_WORLD_62;
	sdl_keymap[SDLK_WORLD_63]	= SDLK12_WORLD_63;
	sdl_keymap[SDLK_WORLD_64]	= SDLK12_WORLD_64;
	sdl_keymap[SDLK_WORLD_65]	= SDLK12_WORLD_65;
	sdl_keymap[SDLK_WORLD_66]	= SDLK12_WORLD_66;
	sdl_keymap[SDLK_WORLD_67]	= SDLK12_WORLD_67;
	sdl_keymap[SDLK_WORLD_68]	= SDLK12_WORLD_68;
	sdl_keymap[SDLK_WORLD_69]	= SDLK12_WORLD_69;
	sdl_keymap[SDLK_WORLD_70]	= SDLK12_WORLD_70;
	sdl_keymap[SDLK_WORLD_71]	= SDLK12_WORLD_71;
	sdl_keymap[SDLK_WORLD_72]	= SDLK12_WORLD_72;
	sdl_keymap[SDLK_WORLD_73]	= SDLK12_WORLD_73;
	sdl_keymap[SDLK_WORLD_74]	= SDLK12_WORLD_74;
	sdl_keymap[SDLK_WORLD_75]	= SDLK12_WORLD_75;
	sdl_keymap[SDLK_WORLD_76]	= SDLK12_WORLD_76;
	sdl_keymap[SDLK_WORLD_77]	= SDLK12_WORLD_77;
	sdl_keymap[SDLK_WORLD_78]	= SDLK12_WORLD_78;
	sdl_keymap[SDLK_WORLD_79]	= SDLK12_WORLD_79;
	sdl_keymap[SDLK_WORLD_80]	= SDLK12_WORLD_80;
	sdl_keymap[SDLK_WORLD_81]	= SDLK12_WORLD_81;
	sdl_keymap[SDLK_WORLD_82]	= SDLK12_WORLD_82;
	sdl_keymap[SDLK_WORLD_83]	= SDLK12_WORLD_83;
	sdl_keymap[SDLK_WORLD_84]	= SDLK12_WORLD_84;
	sdl_keymap[SDLK_WORLD_85]	= SDLK12_WORLD_85;
	sdl_keymap[SDLK_WORLD_86]	= SDLK12_WORLD_86;
	sdl_keymap[SDLK_WORLD_87]	= SDLK12_WORLD_87;
	sdl_keymap[SDLK_WORLD_88]	= SDLK12_WORLD_88;
	sdl_keymap[SDLK_WORLD_89]	= SDLK12_WORLD_89;
	sdl_keymap[SDLK_WORLD_90]	= SDLK12_WORLD_90;
	sdl_keymap[SDLK_WORLD_91]	= SDLK12_WORLD_91;
	sdl_keymap[SDLK_WORLD_92]	= SDLK12_WORLD_92;
	sdl_keymap[SDLK_WORLD_93]	= SDLK12_WORLD_93;
	sdl_keymap[SDLK_WORLD_94]	= SDLK12_WORLD_94;
	sdl_keymap[SDLK_WORLD_95]	= SDLK12_WORLD_95;		/* 0xFF */
        /*@}*/
#endif

	/** @name Numeric keypad */
        /*@{*/
	sdl_keymap[SDLK_KP_0]		= SDLK12_KP0;
	sdl_keymap[SDLK_KP_1]		= SDLK12_KP1;
	sdl_keymap[SDLK_KP_2]		= SDLK12_KP2;
	sdl_keymap[SDLK_KP_3]		= SDLK12_KP3;
	sdl_keymap[SDLK_KP_4]		= SDLK12_KP4;
	sdl_keymap[SDLK_KP_5]		= SDLK12_KP5;
	sdl_keymap[SDLK_KP_6]		= SDLK12_KP6;
	sdl_keymap[SDLK_KP_7]		= SDLK12_KP7;
	sdl_keymap[SDLK_KP_8]		= SDLK12_KP8;
	sdl_keymap[SDLK_KP_9]		= SDLK12_KP9;
	sdl_keymap[SDLK_KP_PERIOD]	= SDLK12_KP_PERIOD;
	sdl_keymap[SDLK_KP_DIVIDE]	= SDLK12_KP_DIVIDE;
	sdl_keymap[SDLK_KP_MULTIPLY]	= SDLK12_KP_MULTIPLY;
	sdl_keymap[SDLK_KP_MINUS]	= SDLK12_KP_MINUS;
	sdl_keymap[SDLK_KP_PLUS]	= SDLK12_KP_PLUS;
	sdl_keymap[SDLK_KP_ENTER]	= SDLK12_KP_ENTER;
	sdl_keymap[SDLK_KP_EQUALS]	= SDLK12_KP_EQUALS;
        /*@}*/

	/** @name Arrows + Home/End pad */
        /*@{*/
	sdl_keymap[SDLK_UP]		= SDLK12_UP;
	sdl_keymap[SDLK_DOWN]		= SDLK12_DOWN;
	sdl_keymap[SDLK_RIGHT]		= SDLK12_RIGHT;
	sdl_keymap[SDLK_LEFT]		= SDLK12_LEFT;
	sdl_keymap[SDLK_INSERT]		= SDLK12_INSERT;
	sdl_keymap[SDLK_HOME]		= SDLK12_HOME;
	sdl_keymap[SDLK_END]		= SDLK12_END;
	sdl_keymap[SDLK_PAGEUP]		= SDLK12_PAGEUP;
	sdl_keymap[SDLK_PAGEDOWN]	= SDLK12_PAGEDOWN;
        /*@}*/

	/** @name Function keys */
        /*@{*/
	sdl_keymap[SDLK_F1]		= SDLK12_F1;
	sdl_keymap[SDLK_F2]		= SDLK12_F2;
	sdl_keymap[SDLK_F3]		= SDLK12_F3;
	sdl_keymap[SDLK_F4]		= SDLK12_F4;
	sdl_keymap[SDLK_F5]		= SDLK12_F5;
	sdl_keymap[SDLK_F6]		= SDLK12_F6;
	sdl_keymap[SDLK_F7]		= SDLK12_F7;
	sdl_keymap[SDLK_F8]		= SDLK12_F8;
	sdl_keymap[SDLK_F9]		= SDLK12_F9;
	sdl_keymap[SDLK_F10]		= SDLK12_F10;
	sdl_keymap[SDLK_F11]		= SDLK12_F11;
	sdl_keymap[SDLK_F12]		= SDLK12_F12;
	sdl_keymap[SDLK_F13]		= SDLK12_F13;
	sdl_keymap[SDLK_F14]		= SDLK12_F14;
	sdl_keymap[SDLK_F15]		= SDLK12_F15;
        /*@}*/

	/** @name Key state modifier keys */
        /*@{*/
	//sdl_keymap[SDLK_NUMLOCK]	= SDLK12_NUMLOCK;
	sdl_keymap[SDLK_CAPSLOCK]	= SDLK12_CAPSLOCK;
	sdl_keymap[SDLK_SCROLLLOCK]	= SDLK12_SCROLLOCK;
	sdl_keymap[SDLK_RSHIFT]		= SDLK12_RSHIFT;
	sdl_keymap[SDLK_LSHIFT]		= SDLK12_LSHIFT;
	sdl_keymap[SDLK_RCTRL]		= SDLK12_RCTRL;
	sdl_keymap[SDLK_LCTRL]		= SDLK12_LCTRL;
	sdl_keymap[SDLK_RALT]		= SDLK12_RALT;
	sdl_keymap[SDLK_LALT]		= SDLK12_LALT;
	//sdl_keymap[SDLK_RMETA]		= SDLK12_RMETA;
	//sdl_keymap[SDLK_LMETA]		= SDLK12_LMETA;
	sdl_keymap[SDLK_LGUI]		= SDLK12_LSUPER;		/**< Left "Windows" key */
	sdl_keymap[SDLK_RGUI]		= SDLK12_RSUPER;		/**< Right "Windows" key */
	sdl_keymap[SDLK_MODE]		= SDLK12_MODE;		/**< "Alt Gr" key */
	//sdl_keymap[SDLK_COMPOSE]	= SDLK12_COMPOSE;		/**< Multi-key compose key */
        /*@}*/

	/** @name Miscellaneous function keys */
        /*@{*/
	sdl_keymap[SDLK_HELP]		= SDLK12_HELP;
	sdl_keymap[SDLK_PRINTSCREEN]	= SDLK12_PRINT;
	sdl_keymap[SDLK_SYSREQ]		= SDLK12_SYSREQ;
	//sdl_keymap[SDLK_BREAK]		= SDLK12_BREAK;
	sdl_keymap[SDLK_MENU]		= SDLK12_MENU;
	sdl_keymap[SDLK_POWER]		= SDLK12_POWER;		/**< Power Macintosh power key */
	//sdl_keymap[SDLK_EURO]		= SDLK12_EURO;		/**< Some european keyboards */
	sdl_keymap[SDLK_UNDO]		= SDLK12_UNDO;		/**< Atari keyboard has Undo */
	return;
}

static void
sdl12_unicodeinit() {
	sdl_unicodemap.clear();
	//
	sdl_unicodemap[SDLK_BACKSPACE]	= 0x08;
	sdl_unicodemap[SDLK_TAB]	= '\t';
	sdl_unicodemap[SDLK_RETURN]	= '\r';
	sdl_unicodemap[SDLK_ESCAPE]	= 0x1b;
	sdl_unicodemap[SDLK_SPACE]	= ' ';
	sdl_unicodemap[SDLK_EXCLAIM]	= '!';
	sdl_unicodemap[SDLK_QUOTEDBL]	= '\"';
	sdl_unicodemap[SDLK_HASH]	= '#';
	sdl_unicodemap[SDLK_DOLLAR]	= '$';
	sdl_unicodemap[SDLK_AMPERSAND]	= '&';
	sdl_unicodemap[SDLK_QUOTE]	= '\'';
	sdl_unicodemap[SDLK_LEFTPAREN]	= '(';
	sdl_unicodemap[SDLK_RIGHTPAREN]	= ')';
	sdl_unicodemap[SDLK_ASTERISK]	= '*';
	sdl_unicodemap[SDLK_PLUS]	= '+';
	sdl_unicodemap[SDLK_COMMA]	= ',';
	sdl_unicodemap[SDLK_MINUS]	= '-';
	sdl_unicodemap[SDLK_PERIOD]	= '.';
	sdl_unicodemap[SDLK_SLASH]	= '/';
	sdl_unicodemap[SDLK_0]		= '0';
	sdl_unicodemap[SDLK_1]		= '1';
	sdl_unicodemap[SDLK_2]		= '2';
	sdl_unicodemap[SDLK_3]		= '3';
	sdl_unicodemap[SDLK_4]		= '4';
	sdl_unicodemap[SDLK_5]		= '5';
	sdl_unicodemap[SDLK_6]		= '6';
	sdl_unicodemap[SDLK_7]		= '7';
	sdl_unicodemap[SDLK_8]		= '8';
	sdl_unicodemap[SDLK_9]		= '9';
	sdl_unicodemap[SDLK_COLON]	= ':';
	sdl_unicodemap[SDLK_SEMICOLON]	= ';';
	sdl_unicodemap[SDLK_LESS]	= '<';
	sdl_unicodemap[SDLK_EQUALS]	= '=';
	sdl_unicodemap[SDLK_GREATER]	= '>';
	sdl_unicodemap[SDLK_QUESTION]	= '?';
	sdl_unicodemap[SDLK_AT]		= '@';
	/* 
	   Skip uppercase letters
	 */
	sdl_unicodemap[SDLK_LEFTBRACKET]= '[';
	sdl_unicodemap[SDLK_BACKSLASH]	= '\\';
	sdl_unicodemap[SDLK_RIGHTBRACKET] = ']';
	sdl_unicodemap[SDLK_CARET]	= '^';
	sdl_unicodemap[SDLK_UNDERSCORE]	= '_';
	sdl_unicodemap[SDLK_BACKQUOTE]	= '`';
	sdl_unicodemap[SDLK_a]		= 'a';
	sdl_unicodemap[SDLK_b]		= 'b';
	sdl_unicodemap[SDLK_c]		= 'c';
	sdl_unicodemap[SDLK_d]		= 'd';
	sdl_unicodemap[SDLK_e]		= 'e';
	sdl_unicodemap[SDLK_f]		= 'f';
	sdl_unicodemap[SDLK_g]		= 'g';
	sdl_unicodemap[SDLK_h]		= 'h';
	sdl_unicodemap[SDLK_i]		= 'i';
	sdl_unicodemap[SDLK_j]		= 'j';
	sdl_unicodemap[SDLK_k]		= 'k';
	sdl_unicodemap[SDLK_l]		= 'l';
	sdl_unicodemap[SDLK_m]		= 'm';
	sdl_unicodemap[SDLK_n]		= 'n';
	sdl_unicodemap[SDLK_o]		= 'o';
	sdl_unicodemap[SDLK_p]		= 'p';
	sdl_unicodemap[SDLK_q]		= 'q';
	sdl_unicodemap[SDLK_r]		= 'r';
	sdl_unicodemap[SDLK_s]		= 's';
	sdl_unicodemap[SDLK_t]		= 't';
	sdl_unicodemap[SDLK_u]		= 'u';
	sdl_unicodemap[SDLK_v]		= 'v';
	sdl_unicodemap[SDLK_w]		= 'w';
	sdl_unicodemap[SDLK_x]		= 'x';
	sdl_unicodemap[SDLK_y]		= 'y';
	sdl_unicodemap[SDLK_z]		= 'z';

	sdl_unicodemap[SDLK_KP_0]	= '0';
	sdl_unicodemap[SDLK_KP_1]	= '1';
	sdl_unicodemap[SDLK_KP_2]	= '2';
	sdl_unicodemap[SDLK_KP_3]	= '3';
	sdl_unicodemap[SDLK_KP_4]	= '4';
	sdl_unicodemap[SDLK_KP_5]	= '5';
	sdl_unicodemap[SDLK_KP_6]	= '6';
	sdl_unicodemap[SDLK_KP_7]	= '7';
	sdl_unicodemap[SDLK_KP_8]	= '8';
	sdl_unicodemap[SDLK_KP_9]	= '9';
	sdl_unicodemap[SDLK_KP_PERIOD]	= '.';
	sdl_unicodemap[SDLK_KP_DIVIDE]	= '/';
	sdl_unicodemap[SDLK_KP_MULTIPLY]= '*';
	sdl_unicodemap[SDLK_KP_MINUS]	= '-';
	sdl_unicodemap[SDLK_KP_PLUS]	= '+';
	sdl_unicodemap[SDLK_KP_ENTER]	= '\r';
	sdl_unicodemap[SDLK_KP_EQUALS]	= '=';

	return;
}

static void
sdl12_shiftinit() {
	sdl_shiftmap.clear();
	// for only a specific keyboard?
	//
	sdl_shiftmap['`']	= '~';
	sdl_shiftmap['1']	= '!';
	sdl_shiftmap['2']	= '@';
	sdl_shiftmap['3']	= '#';
	sdl_shiftmap['4']	= '$';
	sdl_shiftmap['5']	= '%';
	sdl_shiftmap['6']	= '^';
	sdl_shiftmap['7']	= '&';
	sdl_shiftmap['8']	= '*';
	sdl_shiftmap['9']	= '(';
	sdl_shiftmap['0']	= ')';
	sdl_shiftmap['-']	= '_';
	sdl_shiftmap['=']	= '+';
	//
	sdl_shiftmap['[']	= '{';
	sdl_shiftmap[']']	= '}';
	sdl_shiftmap['\\']	= '|';
	//
	sdl_shiftmap[';']	= ':';
	sdl_shiftmap['\'']	= '\"';
	//
	sdl_shiftmap[',']	= '<';
	sdl_shiftmap['.']	= '>';
	sdl_shiftmap['/']	= '?';
	//
	return;
}

void
sdl12_mapinit() {
	sdl12_keyinit();
	sdl12_unicodeinit();
	sdl12_shiftinit();
	return;
}

static void
sdl12_hook_replay(struct sdlmsg *msg) {
	SDL12_Event sdl12evt;
	map<int,int>::iterator mi;
	double scaleX, scaleY;
	//
	ctrl_server_get_scalefactor(&scaleX, &scaleY);
	//
	bzero(&sdl12evt, sizeof(sdl12evt));
	switch(msg->msgtype) {
	case SDL_EVENT_MSGTYPE_KEYBOARD:
		if((mi = sdl_keymap.find(msg->sdlkey)) == sdl_keymap.end())
			break;
		sdl12evt.key.type =
			msg->is_pressed ? SDL12_KEYDOWN : SDL12_KEYUP;
		sdl12evt.key.which = 0;
		sdl12evt.key.state =
			msg->is_pressed ? SDL_PRESSED : SDL_RELEASED;
		sdl12evt.key.keysym.scancode = 0;
		sdl12evt.key.keysym.sym = (SDL12Key) mi->second;
		sdl12evt.key.keysym.mod = (SDL_Keymod) msg->sdlmod;
		if(msg->unicode == 0) {
			map<int,unsigned short>::iterator mu;
			map<unsigned short,unsigned short>::iterator ms;
			//
			if((mu = sdl_unicodemap.find(msg->sdlkey)) != sdl_unicodemap.end()) {
				msg->unicode = mu->second;
				//
				if((msg->sdlmod & KMOD_SHIFT)
				&&((ms = sdl_shiftmap.find(mu->second)) != sdl_shiftmap.end())) {
					msg->unicode = ms->second;
				}
				if(mu->second >= 'a' && mu->second <= 'z') {
					if(msg->sdlmod & KMOD_CAPS)
						msg->unicode = mu->second & 0x0df;
					if(msg->sdlmod & KMOD_SHIFT)
						msg->unicode = mu->second ^ 0x020;
				}
			}
		}
		sdl12evt.key.keysym.unicode = msg->unicode;
		//
		old_SDL_PushEvent(&sdl12evt);
		//ga_error("XXX: PushEvent\n");
		break;
	case SDL_EVENT_MSGTYPE_MOUSEKEY:
		sdl12evt.button.type =
			msg->is_pressed ? SDL12_MOUSEBUTTONDOWN : SDL12_MOUSEBUTTONUP;
		sdl12evt.button.button =
			msg->mousebutton > 5 ? msg->mousebutton - 2 : msg->mousebutton;
		sdl12evt.button.state = msg->is_pressed ? SDL_PRESSED : SDL_RELEASED;
		sdl12evt.button.x = msg->mousex * scaleX;
		sdl12evt.button.y = msg->mousey * scaleY;
		old_SDL_PushEvent(&sdl12evt);
		//ga_error("XXX: PushEvent: x=%d*%.2f, y=%dx%.2f\n", msg->mousex, scaleX, msg->mousey, scaleY);
		break;
	case SDL_EVENT_MSGTYPE_MOUSEWHEEL:
		if(msg->mousey == 0)
			break;
		sdl12evt.button.type = SDL12_MOUSEBUTTONDOWN;
		sdl12evt.button.button =
			(((short) msg->mousey) > 0) ? SDL12_BUTTON_WHEELDOWN : SDL12_BUTTON_WHEELUP;
		sdl12evt.button.state = SDL_PRESSED;
		sdl12evt.button.x = msg->mousex * scaleX;
		sdl12evt.button.y = msg->mousey * scaleY;
		old_SDL_PushEvent(&sdl12evt);
		//ga_error("XXX: PushEvent: x=%d*%.2f, y=%dx%.2f\n", msg->mousex, scaleX, msg->mousey, scaleY);
		//
		sdl12evt.button.type = SDL12_MOUSEBUTTONUP;
		sdl12evt.button.state = SDL_RELEASED;
		old_SDL_PushEvent(&sdl12evt);
		//ga_error("XXX: PushEvent\n");
		break;
	case SDL_EVENT_MSGTYPE_MOUSEMOTION:
		sdl12evt.motion.type = SDL12_MOUSEMOTION;
		sdl12evt.motion.state = msg->mousestate;
		sdl12evt.motion.x = msg->mousex * scaleX;
		sdl12evt.motion.y = msg->mousey * scaleY;
		sdl12evt.motion.xrel = ((short) msg->mouseRelX) * scaleX;
		sdl12evt.motion.yrel = ((short) msg->mouseRelY) * scaleY;
		old_SDL_PushEvent(&sdl12evt);
		//ga_error("XXX: PushEvent: x=%d*%.2f, y=%dx%.2f\n", msg->mousex, scaleX, msg->mousey, scaleY);
		break;
	}
	return;
}

void
sdl_hook_replay_callback(void *msg, int msglen) {
	struct sdlmsg *smsg = (struct sdlmsg*) msg;
	sdlmsg_ntoh(smsg);
	sdl12_hook_replay(smsg);
	return;
}

int
hook_SDL_Init(unsigned int flags) {
	int ret;
	//
#ifndef WIN32
	if(old_SDL_Init == NULL) {
		sdl_hook_symbols();
	}
#endif
	//
	if((ret = old_SDL_Init(flags)) < 0)
		return ret;
	//
	sdl_hook_init();
	//
	return ret;
}

SDL12_Surface *
hook_SDL_SetVideoMode(int width, int height, int bpp, uint32_t flags) {
	SDL12_Surface *ret;
	//
	if(old_SDL_SetVideoMode == NULL) {
		sdl_hook_symbols();
	}
	screensurface = NULL;
	ret = old_SDL_SetVideoMode(width, height, bpp, flags);
	if(ret != NULL) {
		uint32_t rmask, gmask, bmask, amask;
		uint32_t endian_test = 0x12345678;
		unsigned char *endian_ptr = (unsigned char *) &endian_test;
		//
		if(dupsurface != NULL)
			old_SDL_FreeSurface(dupsurface);
		//
		screensurface = ret;
		dupsurface = NULL;
		//
		if(*endian_ptr == 0x12) {	// BIG endian
			rmask = 0xff000000;
			gmask = 0x00ff0000;
			bmask = 0x0000ff00;
			amask = 0x000000ff;
		} else if(*endian_ptr == 0x78) { // LITTLE endian
			rmask = 0x000000ff;
			gmask = 0x0000ff00;
			bmask = 0x00ff0000;
			amask = 0xff000000;
		} else {
			ga_error("SDL_SetVideoMode: GA detect endianness failed.\n");
			goto err_quit;
		}
		dupsurface = old_SDL_CreateRGBSurface(SDL_SWSURFACE, width, height, 32,
				rmask, gmask, bmask, amask);
		if(dupsurface == NULL) {
			ga_error("SDL_SetVideoMode: GA cannot create RGB surface.\n");
			goto err_quit;
		}
		ga_error("SDL_SetVideoMode: RGB surface created (%dx%d)\n",
			dupsurface->w, dupsurface->h);
		ctrl_server_set_resolution(dupsurface->w, dupsurface->h);
		// send active events
#if 1
		do {
			SDL12_Event evt;
			// activate
			bzero(&evt, sizeof(evt));
			evt.active.type = SDL12_ACTIVEEVENT;
			evt.active.state = SDL_APPMOUSEFOCUS
				| SDL_APPINPUTFOCUS
				| SDL_APPACTIVE;
			evt.active.gain = 1;
			old_SDL_PushEvent(&evt);
			// move mouse to (0,0)
			bzero(&evt, sizeof(evt));
			evt.motion.type = SDL12_MOUSEMOTION;
			old_SDL_PushEvent(&evt);
		} while(0);
#endif
	}
	return ret;
err_quit:
	old_SDL_FreeSurface(ret);
	if(dupsurface != NULL)
		old_SDL_FreeSurface(dupsurface);
	screensurface = NULL;
	dupsurface = NULL;
	return NULL;
}

static void
hook_SDL_capture_screen(const char *caller) {
	static int frame_interval;
	static struct timeval initialTv, captureTv;
	static int sb_initialized = 0;
	struct pooldata *data;
	struct vsource_frame *frame;
	//
	if(old_SDL_BlitSurface(screensurface, NULL, dupsurface, NULL) != 0) {
		ga_error("SDL_BlitSurface: GA cannot blit to RGB surface (called by %s).\n", caller);
		exit(-1);
	}
	//
	if(ga_hook_capture_prepared(dupsurface->w, dupsurface->h, 0) < 0)
		return;
	//
	if(sb_initialized == 0) {
		frame_interval = 1000000/video_fps; // in the unif of us
		frame_interval++;
		gettimeofday(&initialTv, NULL);
		sb_initialized = 1;
	} else {
		gettimeofday(&captureTv, NULL);
	}
	//
	if (enable_server_rate_control && ga_hook_video_rate_control() < 0)
		return;
	// copy screen
	do {
		data = g_pipe[0]->allocate_data();
		frame = (struct vsource_frame*) data->ptr;
		frame->pixelformat = PIX_FMT_RGBA;
		frame->realwidth = dupsurface->w;
		frame->realheight = dupsurface->h;
		frame->realstride = dupsurface->pitch;
		frame->realsize = dupsurface->h * dupsurface->pitch;
		frame->linesize[0] = dupsurface->pitch;
		bcopy(dupsurface->pixels, frame->imgbuf, frame->realsize);
		frame->imgpts = tvdiff_us(&captureTv, &initialTv)/frame_interval;
	} while(0);
	// duplicate from channel 0 to other channels
	ga_hook_capture_dupframe(frame);
	g_pipe[0]->store_data(data);
	g_pipe[0]->notify_all();		
	return;
}

int
hook_SDL_BlitSurface(SDL12_Surface *src, SDL12_Rect *srcrect, SDL12_Surface *dst, SDL12_Rect *dstrect) {
	int ret;
	//
	if(old_SDL_BlitSurface == NULL) {
		sdl_hook_symbols();
	}
	ret = old_SDL_BlitSurface(src, srcrect, dst, dstrect);
	//
	if(dst == screensurface) {
		if((screensurface->flags & SDL_HWSURFACE) != 0) {
			hook_SDL_capture_screen("SDL_BlitSurface");
		}
	}
	//
	return ret;
}

int
hook_SDL_Flip(SDL12_Surface *screen) {
	int ret;
	if(old_SDL_Flip == NULL) {
		sdl_hook_symbols();
	}
	ret = old_SDL_Flip(screen);
	if(ret == 0 && screen == screensurface) {
		hook_SDL_capture_screen("SDL_Flip");
	}
	return ret;
}

void
hook_SDL_UpdateRect(SDL12_Surface *screen, int32_t x, int32_t y, int32_t w, int32_t h) {
	if(old_SDL_UpdateRect == NULL) {
		sdl_hook_symbols();
	}
	//
	old_SDL_UpdateRect(screen, x, y, w, h);
	//
	if(screen == screensurface) {
		hook_SDL_capture_screen("SDL_UpdateRect");
	}
	return;
}

void
hook_SDL_UpdateRects(SDL12_Surface *screen, int numrects, SDL12_Rect *rects) {
	if(old_SDL_UpdateRects == NULL) {
		sdl_hook_symbols();
	}
	//
	old_SDL_UpdateRects(screen, numrects, rects);
	//
	if(screen == screensurface) {
		hook_SDL_capture_screen("SDL_UpdateRects");
	}
	return;
}


void
hook_SDL_GL_SwapBuffers() {
	static int frame_interval;
	static struct timeval initialTv, captureTv;
	static int frameLinesize;
	static unsigned char *frameBuf;
	static int sb_initialized = 0;
	//
	GLint vp[4];
	int vp_x, vp_y, vp_width, vp_height;
	int i;
	struct pooldata *data;
	struct vsource_frame *frame;
	//
	if(old_SDL_GL_SwapBuffers == NULL) {
		sdl_hook_symbols();
	}
	old_SDL_GL_SwapBuffers();
	//
	glGetIntegerv(GL_VIEWPORT, vp);
	vp_x = vp[0];
	vp_y = vp[1];
	vp_width = vp[2];
	vp_height = vp[3];		
	//

	if(ga_hook_capture_prepared(vp_width, vp_height, 0) < 0)
		return;

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
	
	if (enable_server_rate_control && ga_hook_video_rate_control() < 0)
		return;

	// copy screen
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
	
	return;
}

static int
filter_SDL_Event(SDL12_Event *event) {
	if(event->type == SDL12_ACTIVEEVENT) {
		ga_error("SDL_Event: ActiveEvent - state:%d gain:%d\n",
			event->active.state,
			event->active.gain);
		// will not lose
		event->active.gain = 1;
		//
		return 1;
	}
	return 0;
}

int
hook_SDL_PollEvent(SDL12_Event *event) {
	int ret;
	if(old_SDL_PollEvent == NULL) {
		sdl_hook_symbols();
	}
	ret = old_SDL_PollEvent(event);
	if(ret > 0 && event != NULL) {
		filter_SDL_Event(event);
	}
	return ret;
}

int
hook_SDL_WaitEvent(SDL12_Event *event) {
	int ret;
	if(old_SDL_WaitEvent == NULL) {
		sdl_hook_symbols();
	}
	ret = old_SDL_WaitEvent(event);
	if(ret > 0 && event != NULL) {
		filter_SDL_Event(event);
	}
	return ret;
}

int
hook_SDL_PeepEvents(SDL12_Event *event, int numevents, SDL12_eventaction action, uint32_t mask) {
	int i, ret;
	// XXX: altering event messages
	//	SDL_PeepEvents is called by SDL_WaitEvent and SDL_PollEvent
	if(old_SDL_PeepEvents == NULL) {
		sdl_hook_symbols();
	}
	if(action == SDL_ADDEVENT)
		return old_SDL_PeepEvents(event, numevents, action, mask);
	ret = old_SDL_PeepEvents(event, numevents, action, mask);
	if(ret > 0) {
		for(i = 0; i < ret; i++) {
			filter_SDL_Event(&event[i]);
		}
	}
	return ret;
}

static void
sdl_dump_audiospec(SDL12_AudioSpec *spec) {
	ga_error("SDL_OpenAudio: freq=%d format=%x channels=%d silence=%d samples=%d padding=%d size=%d callback=%p userdata=%p\n",
		spec->freq, spec->format, spec->channels, spec->silence,
		spec->samples, spec->padding, spec->size,
		spec->callback, spec->userdata);
	return;
}

static struct SwrContext *swrctx = NULL;
static unsigned char *audio_buf = NULL;
static struct SDL12_AudioSpec audio_spec;
static int audio_buf_samples = 0;

static void (*old_audio_callback)(void *, uint8_t *, int) = NULL;
static void
hook_SDL_audio_callback(void *userdata, uint8_t *stream, int len) {
	const unsigned char *srcplanes[SWR_CH_MAX];
	unsigned char *dstplanes[SWR_CH_MAX];
#if 0
	ga_error("audio-callback: userdata=%p stream=%p len=%d\n",
			userdata, stream, len);
#endif
	if(old_audio_callback != NULL)
		old_audio_callback(userdata, stream, len);
	// pipe to the encoder
	if(swrctx == NULL || audio_buf == NULL)
		goto quit;
	srcplanes[0] = stream;
	srcplanes[1] = NULL;
	dstplanes[0] = audio_buf;
	dstplanes[1] = NULL;
	swr_convert(swrctx,
			dstplanes, audio_buf_samples,
			srcplanes, audio_spec.samples);
	audio_source_buffer_fill(audio_buf, audio_buf_samples);
quit:
	// silence local outputs
	bzero(stream, len);
	return;
}

static enum AVSampleFormat
SDL2SWR_format(uint16_t format) {
	switch(format) {
	case SDL12_AUDIO_U8:
		return AV_SAMPLE_FMT_U8;
	case SDL12_AUDIO_S16:
		return AV_SAMPLE_FMT_S16;
	default:
		ga_error("SDL2SWR: format %x is not supported.\n", format);
		exit(-1);
	}
	return AV_SAMPLE_FMT_NONE;
}

static int64_t
SDL2SWR_chlayout(uint8_t channels) {
	if(channels == 1)
		return AV_CH_LAYOUT_MONO;
	if(channels == 2)
		return AV_CH_LAYOUT_STEREO;
	ga_error("SDL2SWR: channel layout (%d) is not supported.\n", channels);
	exit(-1);
}

int
hook_SDL_OpenAudio(SDL12_AudioSpec *desired, SDL12_AudioSpec *obtained) {
	int ret;
	if(old_SDL_OpenAudio == NULL) {
		sdl_hook_symbols();
	}
	//
	sdl_dump_audiospec(desired);
	if(desired->callback != NULL) {
		old_audio_callback = desired->callback;
		desired->callback = hook_SDL_audio_callback;
	}
	//
	ret = old_SDL_OpenAudio(desired, obtained);
	//
	if(ret != -1) {
		struct RTSPConf *rtspconf = rtspconf_global();
		int bufreq = 0;
		//
		obtained->callback = old_audio_callback;
		// release everything
		if(swrctx != NULL)
			swr_free(&swrctx);
		if(audio_buf != NULL)
			free(audio_buf);
		// create resample context
		swrctx = swr_alloc_set_opts(NULL,
				rtspconf->audio_device_channel_layout,
				rtspconf->audio_device_format,
				rtspconf->audio_samplerate,
				SDL2SWR_chlayout(obtained->channels),
				SDL2SWR_format(obtained->format),
				obtained->freq,
				0, NULL);
		if(swrctx == NULL) {
			ga_error("SDL_OpenAudio: cannot create resample context.\n");
			exit(-1);
		} else {
			ga_error("SDL_OpenAudio: resample context (%x,%d,%d) -> (%x,%d,%d)\n",
				(int) SDL2SWR_chlayout(obtained->channels),
				(int) SDL2SWR_format(obtained->format),
				(int) obtained->freq,
				(int) rtspconf->audio_device_channel_layout,
				(int) rtspconf->audio_device_format,
				(int) rtspconf->audio_samplerate);
		}
		if(swr_init(swrctx) < 0) {
			ga_error("SDL_OpenAudio: resample context init failed.\n");
			exit(-1);
		}
		// allocate resample buffer
		audio_buf_samples = av_rescale_rnd(obtained->samples,
					rtspconf->audio_samplerate,
					obtained->freq, AV_ROUND_UP);
		bufreq = av_samples_get_buffer_size(NULL,
				rtspconf->audio_channels,
				audio_buf_samples*2,
				rtspconf->audio_device_format,
				1/*no-alignment*/);
		if((audio_buf = (unsigned char*) malloc(bufreq)) == NULL) {
			ga_error("SDL_OpenAudio: cannot allocate resample memory.\n");
			exit(-1);
		}
		ga_error("SDL_OpenAudio: %d samples with %d byte(s) resample buffer allocated.\n",
				audio_buf_samples, bufreq);
		// setup GA audio source
		if(audio_source_setup(bufreq, rtspconf->audio_samplerate,
				/* XXX: We support only U8 and S16 now */
				obtained->format == SDL12_AUDIO_S16 ? 16 : 8,
				rtspconf->audio_channels) < 0) {
			ga_error("SDL_OpenAudio: audio source setup failed.\n");
			exit(-1);
		}
		//
		bcopy(obtained, &audio_spec, sizeof(audio_spec));
	}
	return ret;
}

void
hook_SDL_PauseAudio(int pause_on) {
	if(old_SDL_PauseAudio == NULL) {
		sdl_hook_symbols();
	}
	ga_error("SDL_PauseAudio: pause_on = %d\n", pause_on);
	return old_SDL_PauseAudio(pause_on);
}

void
hook_SDL_CloseAudio() {
	if(old_SDL_CloseAudio == NULL) {
		sdl_hook_symbols();
	}
	ga_error("SDL_CloseAudio\n");
	return old_SDL_CloseAudio();
}

#ifndef WIN32	/* POSIX interfaces */
int
SDL_Init(unsigned int flags) {
	return hook_SDL_Init(flags);
}

SDL12_Surface *
SDL_SetVideoMode(int width, int height, int bpp, uint32_t flags) {
	return hook_SDL_SetVideoMode(width, height, bpp, flags);
}

int
SDL_BlitSurface(SDL12_Surface *src, SDL12_Rect *srcrect, SDL12_Surface *dst, SDL12_Rect *dstrect) {
	return hook_SDL_BlitSurface(src, srcrect, dst, dstrect);
}

int
SDL_UpperBlit(SDL12_Surface *src, SDL12_Rect *srcrect, SDL12_Surface *dst, SDL12_Rect *dstrect) {
	if(old_SDL_UpperBlit == old_SDL_BlitSurface)
		return hook_SDL_BlitSurface(src, srcrect, dst, dstrect);
	return old_SDL_UpperBlit(src, srcrect, dst, dstrect);
}

int
SDL_Flip(SDL12_Surface *screen) {
	return hook_SDL_Flip(screen);
}

void
SDL_UpdateRect(SDL12_Surface *screen, int32_t x, int32_t y, int32_t w, int32_t h) {
	return hook_SDL_UpdateRect(screen, x, y, w, h);
}

void
SDL_UpdateRects(SDL12_Surface *screen, int numrects, SDL12_Rect *rects) {
	return hook_SDL_UpdateRects(screen, numrects, rects);
}

void
SDL_GL_SwapBuffers() {
	hook_SDL_GL_SwapBuffers();
}

int
SDL_PollEvent(SDL12_Event *event) {
	return hook_SDL_PollEvent(event);
}

int
SDL_WaitEvent(SDL12_Event *event) {
	return hook_SDL_WaitEvent(event);
}

int
SDL_PeepEvents(SDL12_Event *event, int numevents, SDL12_eventaction action, uint32_t mask) {
	return hook_SDL_PeepEvents(event, numevents, action, mask);
}

int
SDL_OpenAudio(SDL12_AudioSpec *desired, SDL12_AudioSpec *obtained) {
	return hook_SDL_OpenAudio(desired, obtained);
}

void
SDL_PauseAudio(int pause_on) {
	hook_SDL_PauseAudio(pause_on);
	return;
}

void
SDL_CloseAudio() {
	hook_SDL_CloseAudio();
	return;
}

__attribute__((constructor))
static void
sdl_hook_loaded(void) {
	ga_error("ga-hook-sdl loaded!\n");
	if(ga_hook_init() < 0) {
		ga_error("ga_hook: init failed.\n");
		exit(-1);
	}
	return;
}
#endif	/* ! WIN32 */

