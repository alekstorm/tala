#ifndef _UiPause_h_
#define _UiPause_h_
/* UiPause.h
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

#include "Gui.h"
#include "Interpreter.h"

class UiPause {
  public:
	static int begin (GuiObject topShell, const wchar_t *title, Interpreter *interpreter);

	static int real (const wchar_t *label, const wchar_t *defaultValue);
	static int positive (const wchar_t *label, const wchar_t *defaultValue);
	static int integer (const wchar_t *label, const wchar_t *defaultValue);
	static int natural (const wchar_t *label, const wchar_t *defaultValue);
	static int word (const wchar_t *label, const wchar_t *defaultValue);
	static int sentence (const wchar_t *label, const wchar_t *defaultValue);
	static int text (const wchar_t *label, const wchar_t *defaultValue);
	static int boolean (const wchar_t *label, int defaultValue);
	static int choice (const wchar_t *label, int defaultValue);
	static int optionMenu (const wchar_t *label, int defaultValue);
	static int option (const wchar_t *label);
	static int comment (const wchar_t *label);

	static int end (int numberOfContinueButtons, int defaultContinueButton, int cancelContinueButton,
		const wchar_t *continueText1, const wchar_t *continueText2, const wchar_t *continueText3,
		const wchar_t *continueText4, const wchar_t *continueText5, const wchar_t *continueText6,
		const wchar_t *continueText7, const wchar_t *continueText8, const wchar_t *continueText9,
		const wchar_t *continueText10, Interpreter *interpreter);
};

/* End of file UiPause.h */
#endif
