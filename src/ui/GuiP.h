#ifndef _GuiP_h_
#define _GuiP_h_
/* GuiP.h
 *
 * Copyright (C) 1993-2011 Paul Boersma
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 * pb 2011/03/02
 */

#include "Gui.h"

#ifdef __cplusplus
	extern "C" {
#endif

/*
 * In GUI implementations, we order everything by ease of programming: Unix, Windows, Macintosh.
 */
#if defined (UNIX)
	#define uni 1
	#define win 0
	#define mac 0
#endif
#if defined (_WIN32)
	#define uni 0
	#define win 1
	#define mac 0
#endif
#if defined (macintosh)
	#define uni 0
	#define win 0
	#define mac 1
#endif

void _GuiObject_position (GuiObject me, int left, int right, int top, int bottom);
void * _GuiObject_getUserData (GuiObject me);
void _GuiObject_setUserData (GuiObject me, void *userData);

#ifdef __cplusplus
	}
#endif

/* End of file GuiP.h */
#endif
