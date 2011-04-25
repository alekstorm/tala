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

#ifndef _Editor_h_
	#include "Editor.h"
#endif
#include "UiFile.h"

class TextEditor : public Editor {
  public:
	static void prefs (void);

	TextEditor (GuiObject parent, const wchar_t *initialText);
	~TextEditor ();

	const wchar_t * type () { return L"TextEditor"; }
	bool isFileBased () { return true; }

	void showOpen ();
	void clear ();
	void setFontSize (int fontSize);
	void nameChanged ();
	int openDocument (MelderFile file);
	void newDocument ();
	int saveDocument (MelderFile file);
	void closeDocument ();
	bool getSelectedLines (long *firstLine, long *lastLine);
	void do_find ();
	void do_replace ();

	structMelderFile _file;
	GuiObject _textWidget;
	UiInfile *_openDialog;
	UiOutfile *_saveDialog;
	UiForm *_printDialog, *_findDialog;
	int _dirty, _fontSize;
	GuiObject _dirtyNewDialog, _dirtyOpenDialog, _dirtyCloseDialog;
	GuiObject _fontSizeButton_10, _fontSizeButton_12, _fontSizeButton_14, _fontSizeButton_18, _fontSizeButton_24;

  protected:
	void goAway ();
	void updateSizeMenu ();
	void createMenus ();
	void createChildren ();
};

/* End of file TextEditor.h */
#endif
