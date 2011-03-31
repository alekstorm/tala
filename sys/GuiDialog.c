/* GuiDialog.c
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
 * pb 2007/12/30
 * fb 2010/02/23 gtk
 * pb 2010/05/29 repaired memory leak; made dialog front on show
 * pb 2010/07/29 removed GuiDialog_show
 */

#include "GuiP.h"
#undef iam
#define iam(x)  x me = (x) void_me
#if win || mac
	#define iam_dialog \
		GuiDialog me = widget -> userData
#else
	#define iam_dialog \
		GuiDialog me = _GuiObject_getUserData (widget)
#endif

typedef struct structGuiDialog {
	GuiObject widget;
	void (*goAwayCallback) (void *boss);
	void *goAwayBoss;
} *GuiDialog;

#if gtk
	static void _GuiGtkDialog_destroyCallback (GuiObject widget, gpointer void_me) {
		(void) widget;
		iam (GuiDialog);
		Melder_free (me);
	}
	static gboolean _GuiGtkDialog_goAwayCallback (GuiObject widget, GdkEvent *event, gpointer void_me) {
		(void) event;
		iam (GuiDialog);
		if (my goAwayCallback != NULL) {
			my goAwayCallback (my goAwayBoss);
		}
		return TRUE;   // signal handled (don't destroy dialog)
	}
#elif win || mac
	static void _GuiMotifDialog_destroyCallback (GuiObject widget, XtPointer void_me, XtPointer call) {
		(void) widget; (void) call;
		iam (GuiDialog);
		Melder_free (me);
	}
	static void _GuiMotifDialog_goAwayCallback (GuiObject widget, XtPointer void_me, XtPointer call) {
		(void) widget; (void) call;
		iam (GuiDialog);
		if (my goAwayCallback != NULL) {
			my goAwayCallback (my goAwayBoss);
		}
	}
#endif

GuiObject GuiDialog_create (GuiObject parent, int x, int y, int width, int height,
	const wchar_t *title, void (*goAwayCallback) (void *goAwayBoss), void *goAwayBoss, unsigned long flags)
{
	GuiDialog me = Melder_calloc_f (struct structGuiDialog, 1);
	my goAwayCallback = goAwayCallback;
	my goAwayBoss = goAwayBoss;
	#if gtk
		GuiObject shell = gtk_dialog_new ();
		if (parent) {
			GuiObject toplevel = gtk_widget_get_ancestor (parent, GTK_TYPE_WINDOW);
			if (toplevel) {
				gtk_window_set_transient_for (GTK_WINDOW (shell), GTK_WINDOW (toplevel));
				gtk_window_set_destroy_with_parent (GTK_WINDOW (shell), TRUE);
			}
		}
		g_signal_connect (G_OBJECT (shell), "delete-event",
			goAwayCallback ? G_CALLBACK (_GuiGtkDialog_goAwayCallback) : G_CALLBACK (gtk_widget_hide_on_delete), me);
		if (width == Gui_AUTOMATIC) width = -1;
		if (height == Gui_AUTOMATIC) height = -1;
		gtk_window_set_default_size (GTK_WINDOW (shell), width, height);
		gtk_window_set_modal (GTK_WINDOW (shell), flags & GuiDialog_MODAL);
		GuiWindow_setTitle (shell, title);
		my widget = GTK_DIALOG (shell) -> vbox;
		g_signal_connect (G_OBJECT (my widget), "destroy", G_CALLBACK (_GuiGtkDialog_destroyCallback), me);
	#elif win || mac
		GuiObject shell = XmCreateDialogShell (parent, "dialogShell", NULL, 0);
		XtVaSetValues (shell, XmNdeleteResponse, goAwayCallback ? XmDO_NOTHING : XmUNMAP, XmNx, x, XmNy, y, NULL);
		if (goAwayCallback) {
			XmAddWMProtocolCallback (shell, 'delw', _GuiMotifDialog_goAwayCallback, (void *) me);
		}
		GuiWindow_setTitle (shell, title);
		my widget = XmCreateForm (shell, "dialog", NULL, 0);
		if (width != Gui_AUTOMATIC) XtVaSetValues (my widget, XmNwidth, (Dimension) width, NULL);
		if (height != Gui_AUTOMATIC) XtVaSetValues (my widget, XmNheight, (Dimension) height, NULL);
		_GuiObject_setUserData (my widget, me);
		XtAddCallback (my widget, XmNdestroyCallback, _GuiMotifDialog_destroyCallback, me);
		XtVaSetValues (my widget, XmNdialogStyle,
			(flags & GuiDialog_MODAL) ? XmDIALOG_FULL_APPLICATION_MODAL : XmDIALOG_MODELESS,
			XmNautoUnmanage, False, NULL);
	#endif
	return my widget;
}

GuiObject GuiDialog_getButtonArea (GuiObject widget) {
	#if gtk
		GuiObject shell = GuiObject_parent (widget);
		Melder_assert (GTK_IS_DIALOG (shell));
		return GTK_DIALOG (shell) -> action_area;
	#else
		return widget;
	#endif
}

/* End of file GuiDialog.c */
