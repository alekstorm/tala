/* GuiButton.c
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
 * pb & sdk 2007/12/25 gtk
 * fb 2010/02/23 GTK
 * pb 2010/06/14 HandleControlClick
 * pb 2010/08/10 removed Motif
 * pb 2011/02/07 GuiButton_ATTRACTIVE
 */

#include "GuiP.h"
#undef iam
#define iam(x)  x me = (x) void_me
#if win || mac
	#define iam_button \
		Melder_assert (widget -> widgetClass == xmPushButtonWidgetClass); \
		GuiButton me = widget -> userData
#else
	#define iam_button \
		GuiButton me = _GuiObject_getUserData (widget)
#endif

typedef struct structGuiButton {
	GuiObject widget;
	void (*activateCallback) (void *boss, GuiButtonEvent event);
	void *activateBoss;
} *GuiButton;

static void _GuiGtkButton_destroyCallback (GuiObject widget, gpointer void_me) {
	(void) widget;
	iam (GuiButton);
	Melder_free (me);
}
static void _GuiGtkButton_activateCallback (GuiObject widget, gpointer void_me) {
	iam (GuiButton);
	struct structGuiButtonEvent event = { widget, 0 };
	if (my activateCallback != NULL) {
		my activateCallback (my activateBoss, & event);
	}
}

GuiObject GuiButton_create (GuiObject parent, int left, int right, int top, int bottom,
	const wchar_t *buttonText, void (*activateCallback) (void *boss, GuiButtonEvent event), void *activateBoss, unsigned long flags)
{
	GuiButton me = Melder_calloc_f (struct structGuiButton, 1);
	my activateCallback = activateCallback;
	my activateBoss = activateBoss;
	my widget = gtk_button_new_with_label (Melder_peekWcsToUtf8 (buttonText));
	_GuiObject_setUserData (my widget, me);
//	_GuiObject_position (my widget, left, right, top, bottom);

	// TODO: use gtk_box_pack_start(GTK_BOX(parent), my widget, FALSE, FALSE, ?)
	if (parent)
		gtk_container_add (GTK_CONTAINER (parent), my widget);
	if (flags & GuiButton_DEFAULT || flags & GuiButton_ATTRACTIVE) {
		GTK_WIDGET_SET_FLAGS (my widget, GTK_CAN_DEFAULT);
		GtkWidget *shell = gtk_widget_get_toplevel (my widget);
		gtk_window_set_default (GTK_WINDOW (shell), my widget);
	} else if (1) {
		gtk_button_set_focus_on_click ((GtkButton*)my widget, false);
		GTK_WIDGET_UNSET_FLAGS (my widget, GTK_CAN_DEFAULT);
	}
	g_signal_connect (G_OBJECT (my widget), "destroy",
			G_CALLBACK (_GuiGtkButton_destroyCallback), me);
	g_signal_connect (GTK_BUTTON (my widget), "clicked",
			G_CALLBACK (_GuiGtkButton_activateCallback), me);
//	if (flags & GuiButton_CANCEL) {
//		parent -> shell -> cancelButton = parent -> cancelButton = my widget;
//	}
	if (flags & GuiButton_INSENSITIVE) {
		GuiObject_setSensitive (my widget, false);
	}

	return my widget;
}

GuiObject GuiButton_createShown (GuiObject parent, int left, int right, int top, int bottom,
	const wchar_t *buttonText, void (*clickedCallback) (void *boss, GuiButtonEvent event), void *clickedBoss, unsigned long flags)
{
	GuiObject me = GuiButton_create (parent, left, right, top, bottom, buttonText, clickedCallback, clickedBoss, flags);
	GuiObject_show (me);
	return me;
}

void GuiButton_setString (GuiObject widget, const wchar_t *text) {
	gtk_button_set_label (GTK_BUTTON (widget), Melder_peekWcsToUtf8 (text));
}

/* End of file GuiButton.c */
