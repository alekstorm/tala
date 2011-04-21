/* InfoEditor.c
 *
 * Copyright (C) 2004-2010 Paul Boersma
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
 * pb 2004/09/13
 * pb 2004/10/21 clear method also clears the info buffer, not just the visible text
 * pb 2007/05/24 wchar_t
 * pb 2007/06/09 more wchar_t
 * pb 2007/12/31 Gui
 * sdk 2008/03/24 Gui
 * pb 2010/07/29 removed GuiDialog_show
 */

#include "InfoEditor.h"

static InfoEditor *theInfoEditor;

InfoEditor::InfoEditor (GuiObject parent, const wchar_t *initialText)
	: TextEditor(parent, initialText) {}

void InfoEditor::clear () {
	Melder_clearInfo ();
}

// FIXME
void gui_information (wchar_t *message) {
	if (! theInfoEditor) {
		theInfoEditor = new InfoEditor ((GtkWidget*)Melder_topShell, L"");
		Thing_setName (theInfoEditor, L"Praat Info");
	}
	GuiText_setString (theInfoEditor -> _textWidget, message);
	GuiObject_show (theInfoEditor -> _dialog);
	GuiWindow_drain (theInfoEditor -> _shell);
}

/* End of file InfoEditor.c */
