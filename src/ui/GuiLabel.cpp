/* GuiLabel.c
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
 * pb & sdk 2007/12/28 gtk
 * pb 2010/11/28 removed Motif
 */

#include "GuiP.h"
#undef iam
#define iam(x)  x me = (x) void_me
#if win || mac
	#define iam_label \
		Melder_assert (widget -> widgetClass == xmLabelWidgetClass); \
		GuiLabel me = widget -> userData
#else
	#define iam_label \
		GuiLabel me = _GuiObject_getUserData (widget)
#endif

typedef struct structGuiLabel {
	GuiObject widget;
} *GuiLabel;

static void _GuiGtkLabel_destroyCallback (GuiObject widget, gpointer void_me) {
	(void) widget;
	iam (GuiLabel);
	Melder_free (me);
}

GuiObject GuiLabel_create (GuiObject parent, int left, int right, int top, int bottom,
	const wchar_t *labelText, unsigned long flags)
{
	GuiLabel me = Melder_calloc_f (struct structGuiLabel, 1);
	my widget = gtk_label_new (Melder_peekWcsToUtf8 (labelText));
	_GuiObject_setUserData (my widget, me);
	_GuiObject_position (my widget, left, right, top, bottom);
	if (GTK_IS_BOX (parent)) {
		gtk_box_pack_start (GTK_BOX (parent), my widget, FALSE, FALSE, 0);
	}
	g_signal_connect (G_OBJECT (my widget), "destroy",
			  G_CALLBACK (_GuiGtkLabel_destroyCallback), me);
	//gtk_label_set_justify (GTK_LABEL (my widget),
	//	flags & GuiLabel_RIGHT ? GTK_JUSTIFY_RIGHT : flags & GuiLabel_CENTRE ? GTK_JUSTIFY_CENTER : GTK_JUSTIFY_LEFT);
	gtk_misc_set_alignment (GTK_MISC (my widget),
		flags & GuiLabel_RIGHT ? 1.0 : flags & GuiLabel_CENTRE ? 0.5 : 0.0, 0.5);
	return my widget;
}

GuiObject GuiLabel_createShown (GuiObject parent, int left, int right, int top, int bottom,
	const wchar_t *labelText, unsigned long flags)
{
	GuiObject me = GuiLabel_create (parent, left, right, top, bottom, labelText, flags);
	GuiObject_show (me);
	return me;
}

void GuiLabel_setString (GuiObject widget, const wchar_t *text) {
	gtk_label_set_text (GTK_LABEL (widget), Melder_peekWcsToUtf8 (text));
}

/* End of file GuiLabel.c */
