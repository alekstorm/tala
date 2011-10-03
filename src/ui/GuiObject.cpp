/* GuiObject.c
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
 * pb 2007/12/26 abstraction from motif
 * pb 2007/12/28 _GuiObject_position: allow the combination of fixed height and automatic position
 * sdk 2008/03/24 GTK
 * sdk 2008/07/01 GTK get sizes
 * fb 2010/02/23 GTK
 * pb 2010/11/28 removed Motif
 */

#include "GuiP.h"
#include "machine.h"

static int _Gui_defaultHeight (GuiObject me) {
	#if win || mac
	WidgetClass klas = XtClass (me);
	if (klas == xmLabelWidgetClass) return Gui_LABEL_HEIGHT;
	if (klas == xmPushButtonWidgetClass) return Gui_PUSHBUTTON_HEIGHT;
	if (klas == xmTextWidgetClass) return Gui_TEXTFIELD_HEIGHT;
	if (klas == xmToggleButtonWidgetClass) return
		#ifdef UNIX
			Gui_CHECKBUTTON_HEIGHT;   // BUG
		#else
			my isRadioButton ? Gui_RADIOBUTTON_HEIGHT : Gui_CHECKBUTTON_HEIGHT;
		#endif
	#endif
	return 100;
}

void _GuiObject_position (GuiObject me, int left, int right, int top, int bottom) {
	// TODO: ...nog even te creatief
}

void * _GuiObject_getUserData (GuiObject me) {
	return (void *) g_object_get_data (G_OBJECT (me), "praat");
}

void _GuiObject_setUserData (GuiObject me, void *userData) {
	g_object_set_data (G_OBJECT (me), "praat", userData);
}

void GuiObject_destroy (GuiObject me) {
	gtk_widget_destroy (me);
}

long GuiObject_getHeight (GuiObject me) {
	return my allocation.height;
}

long GuiObject_getWidth (GuiObject me) {
	return my allocation.width;
}

long GuiObject_getX (GuiObject me) {
	return my allocation.x;
}

long GuiObject_getY (GuiObject me) {
	return my allocation.y;
}

void GuiObject_move (GuiObject me, long x, long y) {
}

void GuiObject_hide (GuiObject me) {
	GuiObject parent = gtk_widget_get_parent (me);
	if (parent != NULL && GTK_IS_DIALOG (parent)) {   // I am the top vbox of a dialog
		gtk_widget_hide (parent);
	} else {
		gtk_widget_hide (GTK_WIDGET (me));
	}
}

GuiObject GuiObject_parent (GuiObject me) {
	return gtk_widget_get_parent (me);
}

void GuiObject_setSensitive (GuiObject me, bool sensitive) {
	gtk_widget_set_sensitive (me, sensitive);
}

void GuiObject_show (GuiObject me) {
	GuiObject parent = gtk_widget_get_parent (me);
	if (GTK_IS_WINDOW (parent)) {
		// I am a window's vbox
		gtk_widget_show (me);
		gtk_window_present (GTK_WINDOW (parent));
	} else if (GTK_IS_DIALOG (parent)) {
		// I am a dialog's vbox, and therefore automatically shown
		gtk_window_present (GTK_WINDOW (parent));
	} else {
		gtk_widget_show (me);
	}
}

void GuiObject_size (GuiObject me, long width, long height) {
	if (width == Gui_AUTOMATIC || width <= 0) width = -1;
	if (height == Gui_AUTOMATIC || height <= 0) height = -1;
	gtk_widget_set_size_request (me, width, height);
}

/* End of file GuiObject.c */
