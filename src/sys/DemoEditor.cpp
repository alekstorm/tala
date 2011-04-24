/* DemoEditor.c
 *
 * Copyright (C) 2009-2010 Paul Boersma
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
 * pb 2009/05/04 created
 * pb 2009/05/06 Demo_waitForInput ()
 * pb 2009/05/08 Demo_input ()
 * pb 2009/06/30 removed interpreter member (could cause Praat to crash when the editor was closed after an "execute")
 * pb 2009/08/21 Demo_windowTitle ()
 * pb 2009/12/25 error messages if the user tries to handle the Demo window while it is waiting for input
 * pb 2010/07/13 GTK
 */

#include "DemoEditor.h"
#include "machine.h"
#include "praatP.h"
#include "kar/UnicodeData.h"

DemoEditor *DemoEditor::theDemoEditor = NULL;

/***** DemoEditor methods *****/

static void gui_drawingarea_cb_resize (I, GuiDrawingAreaResizeEvent event) {
	DemoEditor *demoEditor = (DemoEditor *)void_me;
	if (demoEditor->_graphics == NULL) return;   // Could be the case in the very beginning.
	Dimension marginWidth = 0, marginHeight = 0;
	Graphics_setWsViewport (demoEditor->_graphics, marginWidth, event -> width - marginWidth, marginHeight, event -> height - marginHeight);
	Graphics_setWsWindow (demoEditor->_graphics, 0, 100, 0, 100);
	Graphics_setViewport (demoEditor->_graphics, 0, 100, 0, 100);
	Graphics_updateWs (demoEditor->_graphics);
}

DemoEditor::DemoEditor (GuiObject parent)
	: Editor (parent, 0, 0, 1024, 768, L"", NULL) {
	createChildren ();
	_graphics = Graphics_create_xmdrawingarea (_drawingArea);
	Graphics_setColour (_graphics, Graphics_WHITE);
	Graphics_setWindow (_graphics, 0, 1, 0, 1);
	Graphics_fillRectangle (_graphics, 0, 1, 0, 1);
	Graphics_setColour (_graphics, Graphics_BLACK);
	Graphics_startRecording (_graphics);
	//Graphics_setViewport (_graphics, 0, 100, 0, 100);
	//Graphics_setWindow (_graphics, 0, 100, 0, 100);
	//Graphics_line (_graphics, 0, 100, 100, 0);

	struct structGuiDrawingAreaResizeEvent event = { _drawingArea, 0 };
	event. width = GuiObject_getWidth (_drawingArea);
	event. height = GuiObject_getHeight (_drawingArea);
	gui_drawingarea_cb_resize (this, & event);
}

DemoEditor::~DemoEditor () {
	Melder_free (_praatPicture);
	forget (_graphics);
}

void DemoEditor::info () {
	Editor::info ();
	MelderInfo_writeLine2 (L"Colour: ", Graphics_Colour_name (((PraatPicture) _praatPicture) -> colour));
	MelderInfo_writeLine2 (L"Font: ", kGraphics_font_getText (((PraatPicture) _praatPicture) -> font));
	MelderInfo_writeLine2 (L"Font size: ", Melder_integer (((PraatPicture) _praatPicture) -> fontSize));
}

void DemoEditor::goAway () {
	if (_waitingForInput)
		_userWantsToClose = true;
	// FIXME
}

static void gui_drawingarea_cb_expose (I, GuiDrawingAreaExposeEvent event) {
	DemoEditor *demoEditor = (DemoEditor *)void_me;
	(void) event;
	if (demoEditor->_graphics == NULL) return;   // Could be the case in the very beginning.
	/*
	 * Erase the background. Don't record this erasure!
	 */
	Graphics_stopRecording (demoEditor->_graphics);   // the only place in Praat (the Picture window has a separate Graphics for erasing)?
	Graphics_setColour (demoEditor->_graphics, Graphics_WHITE);
	Graphics_setWindow (demoEditor->_graphics, 0, 1, 0, 1);
	Graphics_fillRectangle (demoEditor->_graphics, 0, 1, 0, 1);
	Graphics_setColour (demoEditor->_graphics, Graphics_BLACK);
	Graphics_startRecording (demoEditor->_graphics);
	Graphics_play (demoEditor->_graphics, demoEditor->_graphics);
}

static void gui_drawingarea_cb_click (I, GuiDrawingAreaClickEvent event) {
	DemoEditor *demoEditor = (DemoEditor *)void_me;
	if (demoEditor->_graphics == NULL) return;   // Could be the case in the very beginning.
	if (gtk && event -> type != BUTTON_PRESS) return;
	demoEditor->_clicked = true;
	demoEditor->_keyPressed = false;
	demoEditor->_x = event -> x;
	demoEditor->_y = event -> y;
	demoEditor->_key = UNICODE_BULLET;
	demoEditor->_shiftKeyPressed = event -> shiftKeyPressed;
	demoEditor->_commandKeyPressed = event -> commandKeyPressed;
	demoEditor->_optionKeyPressed = event -> optionKeyPressed;
	demoEditor->_extraControlKeyPressed = event -> extraControlKeyPressed;
}

static void gui_drawingarea_cb_key (I, GuiDrawingAreaKeyEvent event) {
	DemoEditor *demoEditor = (DemoEditor *)void_me;
	if (demoEditor->_graphics == NULL) return;   // Could be the case in the very beginning.
	demoEditor->_clicked = false;
	demoEditor->_keyPressed = true;
	demoEditor->_x = 0;
	demoEditor->_y = 0;
	demoEditor->_key = event -> key;
	demoEditor->_shiftKeyPressed = event -> shiftKeyPressed;
	demoEditor->_commandKeyPressed = event -> commandKeyPressed;
	demoEditor->_optionKeyPressed = event -> optionKeyPressed;
	demoEditor->_extraControlKeyPressed = event -> extraControlKeyPressed;
}

void DemoEditor::createChildren () {
	_drawingArea = GuiDrawingArea_createShown (_dialog, 0, 0, 0, 0,
		gui_drawingarea_cb_expose, gui_drawingarea_cb_click, gui_drawingarea_cb_key, gui_drawingarea_cb_resize, this, 0);
	#if gtk
		gtk_widget_set_double_buffered (_drawingArea, FALSE);
	#endif
}

int DemoEditor::open (void) {
	#ifndef CONSOLE_APPLICATION
		if (Melder_batch) {
			/*
			 * Batch scripts have to be able to run demos.
			 */
			//Melder_batch = 0;
		}
		if (theDemoEditor == NULL) {
			theDemoEditor = new DemoEditor ((GtkWidget*)Melder_topShell);
			Melder_assert (theDemoEditor != NULL);
			//GuiObject_show (theDemoEditor -> dialog);
			theDemoEditor -> _praatPicture = Melder_calloc_f (structPraatPicture, 1);
			theCurrentPraatPicture = (structPraatPicture*)theDemoEditor -> _praatPicture;
			theCurrentPraatPicture -> graphics = theDemoEditor -> _graphics;
			theCurrentPraatPicture -> font = kGraphics_font_HELVETICA;
			theCurrentPraatPicture -> fontSize = 10;
			theCurrentPraatPicture -> lineType = Graphics_DRAWN;
			theCurrentPraatPicture -> colour = Graphics_BLACK;
			theCurrentPraatPicture -> lineWidth = 1.0;
			theCurrentPraatPicture -> arrowSize = 1.0;
			theCurrentPraatPicture -> x1NDC = 0;
			theCurrentPraatPicture -> x2NDC = 100;
			theCurrentPraatPicture -> y1NDC = 0;
			theCurrentPraatPicture -> y2NDC = 100;
		}
		if (theDemoEditor -> _waitingForInput)
			return Melder_error1 (L"You cannot work with the Demo window while it is waiting for input. "
				"Please click or type into the Demo window or close it.");
		theCurrentPraatPicture = (structPraatPicture*)theDemoEditor -> _praatPicture;
	#endif
	return 1;
}

void DemoEditor::close (void) {
	theCurrentPraatPicture = & theForegroundPraatPicture;
}

int DemoEditor::windowTitle (const wchar_t *title) {
	_name = Melder_wcsdup_f (title);
	return 1;
}

int DemoEditor::show (void) {
	GuiObject_show (_dialog);
	GuiWindow_drain (_shell);
	return 1;
}

bool DemoEditor::waitForInput (Interpreter *interpreter) {
	if (_waitingForInput) {
		Melder_error1 (L"You cannot work with the Demo window while it is waiting for input. "
			"Please click or type into the Demo window or close it.");
		return false;
	}
	//GuiObject_show (_dialog);
	_clicked = false;
	_keyPressed = false;
	_waitingForInput = true;
	#if ! defined (CONSOLE_APPLICATION)
		int wasBackgrounding = Melder_backgrounding;
		structMelderDir dir = { { 0 } };
		Melder_getDefaultDir (& dir);
		if (wasBackgrounding) praat_foreground ();
		#if gtk
			do {
				gtk_main_iteration ();
			} while (! _clicked && ! _keyPressed && ! _userWantsToClose);
		#else
			do {
				XEvent event;
				XtAppNextEvent (Melder_appContext, & event);
				XtDispatchEvent (& event);
			} while (! _clicked && ! _keyPressed && ! _userWantsToClose);
		#endif
		if (wasBackgrounding) praat_background ();
		Melder_setDefaultDir (& dir);
	#endif
	_waitingForInput = false;
	if (_userWantsToClose) {
		interpreter->stop ();
		Melder_error1 (L"You interrupted the script.");
		forget (theDemoEditor);
		return false;
	}
	return true;
}

bool DemoEditor::clicked (void) {
	if (_waitingForInput) {
		Melder_error1 (L"You cannot work with the Demo window while it is waiting for input. "
			"Please click or type into the Demo window or close it.");
		return false;
	}
	return _clicked;
}

double DemoEditor::x (void) {
	if (_waitingForInput) {
		Melder_error1 (L"You cannot work with the Demo window while it is waiting for input. "
			"Please click or type into the Demo window or close it.");
		return NUMundefined;
	}
	Graphics_setInner (_graphics);
	double xWC, yWC;
	Graphics_DCtoWC (_graphics, _x, _y, & xWC, & yWC);
	Graphics_unsetInner (_graphics);
	return xWC;
}

double DemoEditor::y (void) {
	if (_waitingForInput) {
		Melder_error1 (L"You cannot work with the Demo window while it is waiting for input. "
			"Please click or type into the Demo window or close it.");
		return NUMundefined;
	}
	Graphics_setInner (_graphics);
	double xWC, yWC;
	Graphics_DCtoWC (_graphics, _x, _y, & xWC, & yWC);
	Graphics_unsetInner (_graphics);
	return yWC;
}

bool DemoEditor::keyPressed (void) {
	if (_waitingForInput) {
		Melder_error1 (L"You cannot work with the Demo window while it is waiting for input. "
			"Please click or type into the Demo window or close it.");
		return false;
	}
	return _keyPressed;
}

wchar_t DemoEditor::key (void) {
	if (_waitingForInput) {
		Melder_error1 (L"You cannot work with the Demo window while it is waiting for input. "
			"Please click or type into the Demo window or close it.");
		return 0;
	}
	return _key;
}

bool DemoEditor::shiftKeyPressed (void) {
	if (_waitingForInput) {
		Melder_error1 (L"You cannot work with the Demo window while it is waiting for input. "
			"Please click or type into the Demo window or close it.");
		return false;
	}
	return _shiftKeyPressed;
}

bool DemoEditor::commandKeyPressed (void) {
	if (_waitingForInput) {
		Melder_error1 (L"You cannot work with the Demo window while it is waiting for input. "
			"Please click or type into the Demo window or close it.");
		return false;
	}
	return _commandKeyPressed;
}

bool DemoEditor::optionKeyPressed (void) {
	if (_waitingForInput) {
		Melder_error1 (L"You cannot work with the Demo window while it is waiting for input. "
			"Please click or type into the Demo window or close it.");
		return false;
	}
	return _optionKeyPressed;
}

bool DemoEditor::extraControlKeyPressed (void) {
	if (_waitingForInput) {
		Melder_error1 (L"You cannot work with the Demo window while it is waiting for input. "
			"Please click or type into the Demo window or close it.");
		return false;
	}
	return _extraControlKeyPressed;
}

bool DemoEditor::input (const wchar_t *keys) {
	if (_waitingForInput) {
		Melder_error1 (L"You cannot work with the Demo window while it is waiting for input. "
			"Please click or type into the Demo window or close it.");
		return false;
	}
	return wcschr (keys, _key) != NULL;
}

bool DemoEditor::clickedIn (double left, double right, double bottom, double top) {
	if (! _clicked) return false;
	if (_waitingForInput) {
		Melder_error1 (L"You cannot work with the Demo window while it is waiting for input. "
			"Please click or type into the Demo window or close it.");
		return false;
	}
	double xWC = x (), yWC = y ();
	return xWC >= left && xWC < right && yWC >= bottom && yWC < top;
}

/* End of file DemoEditor.c */
