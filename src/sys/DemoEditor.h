#ifndef _DemoEditor_h_
#define _DemoEditor_h_
/* DemoEditor.h
 *
 * Copyright (C) 2009-2011 Paul Boersma
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

#ifndef _Editor_h_
	#include "Editor.h"
#endif

class DemoEditor : public Editor {
  public:
	static int open (void);
	static void close (void);

	static DemoEditor *theDemoEditor;

	DemoEditor (GuiObject parent);
	~DemoEditor ();

	wchar_t * type () { return L"DemoEditor"; }
	bool hasMenuBar () { return false; }
	bool canFullScreen () { return true; }
	bool isScriptable () { return false; }

	int windowTitle (const wchar_t *title);
	int show (void);
	bool waitForInput (Interpreter *interpreter);
	bool clicked (void);
	double x (void);
	double y (void);
	bool keyPressed (void);
	wchar_t key (void);
	bool shiftKeyPressed (void);
	bool commandKeyPressed (void);
	bool optionKeyPressed (void);
	bool extraControlKeyPressed (void);
	void info ();
	void goAway ();
	void createChildren ();

	/* Shortcuts: */
	bool input (const wchar_t *keys);
	bool clickedIn (double left, double right, double bottom, double top);

	GuiObject _drawingArea;
	Graphics _graphics;
	void *_praatPicture;
	bool _clicked, _keyPressed, _shiftKeyPressed, _commandKeyPressed, _optionKeyPressed, _extraControlKeyPressed;
	long _x, _y;
	wchar_t _key;
	bool _waitingForInput, _userWantsToClose, _fullScreen;
};

/* End of file DemoEditor.h */
#endif
