/* melder.c
 *
 * Copyright (C) 1992-2010 Paul Boersma
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

#include "ui/Graphics.h"
#include "ui/machine.h"
#ifdef macintosh
	#include "sys/macport_on.h"
	//#include <Events.h>
	#include <Dialogs.h>
	#include <MacErrors.h>
	#include "sys/macport_off.h"
#endif
#include "ui/Gui.h"

#include <assert.h>

int Melder_batch;   /* Don't we have a GUI?- Set once at application start-up. */
int Melder_backgrounding;   /* Are we running a script?- Set and unset dynamically. */

void *Melder_appContext;   /* XtAppContext* */
void *Melder_topShell;   /* GuiObject */

static int waitWhileProgress (double progress, const wchar_t *message, GuiObject dia, GuiObject scale, GuiObject label1, GuiObject label2, GuiObject cancelButton) {
	#if gtk
		// Wait for all pending events to be processed. If anybody knows how to inspect GTK's
		// event queue for specific events, dump the code here, please.
		// Until then, the button click attaches a g_object data key named "pressed" to the cancelButton
		// which this function reads out in order to tell whether interruption has occurred
		while (gtk_events_pending ())
			gtk_main_iteration ();
	#elif defined (macintosh)
	{
		EventRecord event;
		while (GetNextEvent (mDownMask, & event)) {
			WindowPtr macWindow;
			int part = FindWindow (event. where, & macWindow);
			if (part == inContent) {
				if (GetWindowKind (macWindow) == userKind) {
					SetPortWindowPort (macWindow);
					GlobalToLocal (& event. where);
					ControlPartCode controlPart;
					ControlHandle macControl = FindControlUnderMouse (event. where, macWindow, & controlPart);
					if (macControl) {
						GuiObject control = (GuiObject) GetControlReference (macControl);
						if (control == cancelButton) {
							FlushEvents (everyEvent, 0);
							XtUnmanageChild (dia);
							return 0;
						} else {
							break;
						}
					} else {
						XtDispatchEvent ((XEvent *) & event);
					}
				} else {
					XtDispatchEvent ((XEvent *) & event);
				}
			} else {
				XtDispatchEvent ((XEvent *) & event);
			}
		}
		do { XtNextEvent ((XEvent *) & event); XtDispatchEvent ((XEvent *) & event); } while (event.what);
	}
	#elif defined (_WIN32)
	{
		XEvent event;
		while (PeekMessage (& event, 0, 0, 0, PM_REMOVE)) {
			if (event. message == WM_KEYDOWN) {
				/*
				 * Ignore all key-down messages, except Escape.
				 */
				if (LOWORD (event. wParam) == VK_ESCAPE) {
					XtUnmanageChild (dia);
					return 0;
				}
			} else if (event. message == WM_LBUTTONDOWN) {
				/*
				 * Ignore all mouse-down messages, except click in Interrupt button.
				 */
				GuiObject me = (GuiObject) GetWindowLong (event. hwnd, GWL_USERDATA);
				if (me == cancelButton) {
					XtUnmanageChild (dia);
					return 0;
				}
			} else if (event. message != WM_SYSKEYDOWN) {
				/*
				 * Process paint messages etc.
				 */
				DispatchMessage (& event);
			}
		}
	}
	#else
	{
		XEvent event;
		if (XCheckTypedWindowEvent (XtDisplay (cancelButton), XtWindow (cancelButton), ButtonPress, & event)) {
			XtUnmanageChild (dia);
			return 0;
		}
	}
	#endif
	if (progress >= 1.0) {
		GuiObject_hide (dia);
	} else {
		if (progress <= 0.0) progress = 0.0;
		GuiObject_show (dia);   // TODO: prevent raising to the front
		wchar_t *newline = (wchar_t*)wcschr (message, '\n');
		if (newline != NULL) {
			static MelderString buffer = { 0 };
			MelderString_copy (& buffer, message);
			buffer.string [newline - message] = '\0';
			GuiLabel_setString (label1, buffer.string);
			buffer.string [newline - message] = '\n';
			GuiLabel_setString (label2, buffer.string + (newline - message) + 1);
		} else {
			GuiLabel_setString (label1, message);
			GuiLabel_setString (label2, L"");
		}
		#if gtk
			// update progress bar
			gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (scale), progress);
			while (gtk_events_pending ())
				gtk_main_iteration ();
			// check whether cancelButton has the "pressed" key set
			if (g_object_steal_data (G_OBJECT (cancelButton), "pressed"))
				return 0;
		#else
			XmScaleSetValue (scale, floor (progress * 1000.0));
			XmUpdateDisplay (dia);
		#endif
	}
	return 1;
}

#if gtk
static void progress_dia_close (void *wid) {
	g_object_set_data (G_OBJECT (* (GuiObject *) wid), "pressed", (gpointer) 1);
}
static void progress_cancel_btn_press (void *wid, GuiButtonEvent event) {
	(void) event;
	g_object_set_data (G_OBJECT (* (GuiObject *) wid), "pressed", (gpointer) 1);
}
#endif

static void _Melder_dia_init (GuiObject *dia, GuiObject *scale, GuiObject *label1, GuiObject *label2, GuiObject *cancelButton) {
	*dia = GuiDialog_create ((GtkWidget*)Melder_topShell, 200, 100, Gui_AUTOMATIC, Gui_AUTOMATIC, L"Work in progress",
		#if gtk
			progress_dia_close, cancelButton,
		#else
			NULL, NULL,
		#endif
		0);

	GuiObject form = *dia;
	GuiObject buttons = GuiDialog_getButtonArea (*dia);

	*label1 = GuiLabel_createShown (form, 3, 403, 0, Gui_AUTOMATIC, L"label1", 0);
	*label2 = GuiLabel_createShown (form, 3, 403, 30, Gui_AUTOMATIC, L"label2", 0);

	#if gtk
		*scale = gtk_progress_bar_new ();
		gtk_container_add (GTK_CONTAINER (form), *scale);
		GuiObject_show (*scale);
	#elif motif
		*scale = XmCreateScale (*dia, "scale", NULL, 0);
		XtVaSetValues (*scale, XmNy, 70, XmNwidth, 400, XmNminimum, 0, XmNmaximum, 1000,
			XmNorientation, XmHORIZONTAL,
			#if ! defined (macintosh)
				XmNscaleHeight, 20,
			#endif
			NULL);
		GuiObject_show (*scale);
	#endif

	#if ! defined (macintoshXXX)
		*cancelButton = GuiButton_createShown (buttons, 0, 400, 170, Gui_AUTOMATIC,
			L"Interrupt",
			#if gtk
				progress_cancel_btn_press, cancelButton,
			#else
				NULL, NULL,
			#endif
			0);
	#endif
}

static int theProgressDepth = 0;
void Melder_progressOff (void) { theProgressDepth --; }
void Melder_progressOn (void) { theProgressDepth ++; }

static int _Melder_progress (double progress, const wchar_t *message) {
	(void) progress;
	if (! Melder_batch && theProgressDepth >= 0 && Melder_debug != 14) {
		static clock_t lastTime;
		static GuiObject dia = NULL, scale = NULL, label1 = NULL, label2 = NULL, cancelButton = NULL;
		clock_t now = clock ();
		if (progress <= 0.0 || progress >= 1.0 ||
			now - lastTime > CLOCKS_PER_SEC / 4)   /* This time step must be much longer than the null-event waiting time. */
		{
			if (dia == NULL) {
				_Melder_dia_init (& dia, & scale, & label1, & label2, & cancelButton);
			}
			bool interruption = waitWhileProgress (progress, message, dia, scale, label1, label2, cancelButton);
			if (! interruption) Melder_error1 (L"Interrupted!");
			lastTime = now;
			return interruption;
		}
	}
	return 1;   /* Proceed. */
}

static void * _Melder_monitor (double progress, const wchar_t *message) {
	(void) progress;
	if (! Melder_batch && theProgressDepth >= 0) {
		static clock_t lastTime;
		static GuiObject dia = NULL, scale = NULL, label1 = NULL, label2 = NULL, cancelButton = NULL, drawingArea = NULL;
		clock_t now = clock ();
		static Any graphics = NULL;
		if (progress <= 0.0 || progress >= 1.0 ||
			now - lastTime > CLOCKS_PER_SEC / 4)   /* This time step must be much longer than the null-event waiting time. */
		{
			if (dia == NULL) {
				_Melder_dia_init (& dia, & scale, & label1, & label2, & cancelButton);
				drawingArea = GuiDrawingArea_createShown (dia, 0, 400, 230, 430, NULL, NULL, NULL, NULL, NULL, 0);
				GuiObject_show (dia);
				graphics = Graphics_create_xmdrawingarea (drawingArea);
			}
			bool interruption = waitWhileProgress (progress, message, dia, scale, label1, label2, cancelButton);
			if (! interruption) Melder_error1 (L"Interrupted!");
			lastTime = now;
			if (progress == 0.0)
				return graphics;
			if (! interruption) return NULL;
		}
	}
	return progress <= 0.0 ? NULL /* No Graphics. */ : & progress /* Any non-NULL pointer. */;
}

#if defined (macintosh)
static void mac_message (int macAlertType, wchar_t *messageW) {
	DialogRef dialog;
	static UniChar messageU [4000];
	int messageLength = wcslen (messageW);
	int j = 0;
	for (int i = 0; i < messageLength && j <= 4000 - 2; i ++) {
		uint32_t kar = messageW [i];
		if (kar <= 0xFFFF) {
			messageU [j ++] = kar;
		} else if (kar <= 0x10FFFF) {
			kar -= 0x10000;
			messageU [j ++] = 0xD800 | (kar >> 10);
			messageU [j ++] = 0xDC00 | (kar & 0x3FF);
		}
	}
	CFStringRef messageCF = CFStringCreateWithCharacters (NULL, messageU, j);
	CreateStandardAlert (macAlertType, messageCF, NULL, NULL, & dialog);
	CFRelease (messageCF);
	RunStandardAlert (dialog, NULL, NULL);
}
#endif

#define theMessageFund_SIZE  100000
static char * theMessageFund = NULL;

static void gui_fatal (wchar_t *message) {
	free (theMessageFund);
	#if gtk
		GuiObject dialog = gtk_message_dialog_new (GTK_WINDOW (Melder_topShell), GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "%s", Melder_peekWcsToUtf8 (message));
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
	#elif defined (macintosh)
		mac_message (kAlertStopAlert, message);
		SysError (11);
	#elif defined (_WIN32)
		MessageBox (NULL, message, L"Fatal error", MB_OK);
	#endif
}

static void gui_error (wchar_t *message) {
	bool memoryIsLow = wcsstr (message, L"Out of memory");
	if (memoryIsLow) {
		free (theMessageFund);
	}
	#if gtk
		GuiObject dialog = gtk_message_dialog_new (GTK_WINDOW (Melder_topShell), GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "%s", Melder_peekWcsToUtf8 (message));
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
	#elif defined (macintosh)
		mac_message (kAlertStopAlert, message);
		XmUpdateDisplay (0);
	#elif defined (_WIN32)
		MessageBox (NULL, message, L"Message", MB_OK);
	#endif
	if (memoryIsLow) {
		theMessageFund = (char*)malloc (theMessageFund_SIZE);
		if (theMessageFund == NULL) {
			#if gtk
				GuiObject dialog = gtk_message_dialog_new (GTK_WINDOW (Melder_topShell), GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "Praat is very low on memory.\nSave your work and quit Praat.\nIf you don't do that, Praat may crash.");
				gtk_dialog_run (GTK_DIALOG (dialog));
				gtk_widget_destroy (dialog);
			#elif defined (macintosh)
				mac_message (kAlertStopAlert, L"Praat is very low on memory.\nSave your work and quit Praat.\nIf you don't do that, Praat may crash.");
				XmUpdateDisplay (0);
			#elif defined (_WIN32)
				MessageBox (NULL, L"Praat is very low on memory.\nSave your work and quit Praat.\nIf you don't do that, Praat may crash.", L"Message", MB_OK);
			#endif
		}
	}
}

static void gui_warning (wchar_t *message) {
	#if gtk
		GuiObject dialog = gtk_message_dialog_new (GTK_WINDOW (Melder_topShell), GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_MESSAGE_WARNING, GTK_BUTTONS_OK, "%s", Melder_peekWcsToUtf8 (message));
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
	#elif defined (macintosh)
		mac_message (kAlertNoteAlert, message);
		XmUpdateDisplay (0);
	#elif defined (_WIN32)
		MessageBox (NULL, message, L"Warning", MB_OK);
	#endif
}

void MelderGui_create (void *appContext, void *parent) {
	extern void gui_information (wchar_t *);   // BUG: no prototype
	theMessageFund = (char*)malloc (theMessageFund_SIZE);
	assert (theMessageFund != NULL);
	Melder_appContext = appContext;
	Melder_topShell = (GuiObject) parent;
	Melder_setInformationProc (gui_information);
	Melder_setFatalProc (gui_fatal);
	Melder_setErrorProc (gui_error);
	Melder_setWarningProc (gui_warning);
}
