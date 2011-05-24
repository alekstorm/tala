#ifndef _melder_gui_h_
#define _melder_gui_h_
/* melder.h
 *
 * Copyright (C) 1992-2011 Paul Boersma
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

/* Procedures to enforce interactive behaviour of the Melder_XXXXXX routines. */

void MelderGui_create (/* XtAppContext* */ void *appContext, /* GuiObject */ void *parent);
/*
	'appContext' is the XtAppContext* output from Xt(Va)AppInitialize;
		if you used Xt(Va)Initialize it should be NULL.
	'parent' is the top-level widget returned by Xt(Va)(App)Initialize.
*/

extern int Melder_batch;   /* True if run from the batch or from an interactive command-line interface. */
extern int Melder_backgrounding;   /* True if running a script. */
extern bool Melder_consoleIsAnsi;

extern void *Melder_appContext;   /* XtAppContext* */
extern void *Melder_topShell;   /* GuiObject */

#endif
