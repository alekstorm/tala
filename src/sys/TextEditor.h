#ifndef _TextEditor_h_
#define _TextEditor_h_
/* TextEditor.h
 *
 * Copyright (C) 1997-2011 Paul Boersma
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

#include "InfoEditor.h"

class TextEditor : public InfoEditor {
  public:
	TextEditor (GuiObject parent, const wchar_t *initialText);

	const wchar_t * type () { return L"TextEditor"; }

	void nameChanged ();
	void newDocument ();
	int saveDocument (MelderFile file);
	void menu_new (EditorCommand *cmd);
	const wchar_t * getName ();

  protected:
	void createMenus ();
	void goAway ();
};

/* End of file TextEditor.h */
#endif
