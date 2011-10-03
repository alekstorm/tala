/* GuiWindow.c
 *
 * Copyright (C) 1993-2010 Paul Boersma
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
 * pb 2004/01/07 this file separated from Gui.c
 * pb 2004/02/12 don't trust window modification feedback on MacOS 9
 * pb 2004/04/06 GuiWindow_drain separated from XmUpdateDisplay
 * pb 2006/10/28 erased MacOS 9 stuff
 * pb 2007/06/19 wchar_t
 * pb 2007/12/30 extraction
 * pb 2010/07/29 removed GuiWindow_show
 * pb 2010/11/28 removed explicit Motif
 */

#include "GuiP.h"
#include "kar/UnicodeData.h"
#undef iam
#define iam(x)  x me = (x) void_me

typedef struct structGuiWindow {
	GuiObject widget;
	void (*goAwayCallback) (void *boss);
	void *goAwayBoss;
} *GuiWindow;

static gboolean _GuiWindow_destroyCallback (GuiObject widget, GdkEvent *event, gpointer void_me) {
	(void) widget;
	iam (GuiWindow);
	Melder_free (me);
	return TRUE;
}

static gboolean _GuiWindow_goAwayCallback (GuiObject widget, GdkEvent *event, gpointer void_me) {
	(void) widget;
	iam (GuiWindow);
	if (my goAwayCallback != NULL) {
		my goAwayCallback (my goAwayBoss);
	}
	return TRUE;
}

GuiObject GuiWindow_create (GuiObject parent, int x, int y, int width, int height,
	const wchar_t *title, void (*goAwayCallback) (void *goAwayBoss), void *goAwayBoss, unsigned long flags)
{
	GuiWindow me = Melder_calloc_f (struct structGuiWindow, 1);
	my goAwayCallback = goAwayCallback;
	my goAwayBoss = goAwayBoss;
	(void) parent;
	GuiObject shell = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	g_signal_connect (G_OBJECT (shell), "delete-event", goAwayCallback ? G_CALLBACK (_GuiWindow_goAwayCallback) : G_CALLBACK (gtk_widget_hide), me);
	g_signal_connect (G_OBJECT (shell), "destroy-event", G_CALLBACK (_GuiWindow_destroyCallback), me);

	// TODO: Paul ik denk dat Gui_AUTOMATIC voor GTK gewoon -1 moet zijn veel minder (onnodig) gezeur
	if (width == Gui_AUTOMATIC) width = -1;
	if (height == Gui_AUTOMATIC) height = -1;

	gtk_window_set_default_size (GTK_WINDOW (shell), width, height);
	GuiWindow_setTitle (shell, title);

	my widget = gtk_vbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (shell), my widget);
	_GuiObject_setUserData (my widget, me);
	return my widget;
}

void GuiWindow_setTitle (GuiObject shell, const wchar_t *title) {
	gtk_window_set_title (GTK_WINDOW (shell), Melder_peekWcsToUtf8 (title));
}

int GuiWindow_setDirty (GuiObject shell, int dirty) {
	#if mac
		SetWindowModified (shell -> nat.window.ptr, dirty);
		return 1;
	#else
		(void) shell;
		(void) dirty;
		return 0;
	#endif
}

void GuiWindow_drain (GuiObject me) {
	//gdk_window_flush (gtk_widget_get_window (me));
	gdk_flush ();
}

/* End of file GuiWindow.c */
