#ifndef _InfoEditor_h_
#define _InfoEditor_h_
/* InfoEditor.h
 *
 * Copyright (C) 1996-2011 Paul Boersma
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

#ifndef _Editor_h_
	#include "Editor.h"
#endif
#include "UiFile.h"

class InfoEditor : public Editor {
  public:
	static void prefs (void);

	InfoEditor (GuiObject parent, const wchar_t *initialText);
	virtual ~InfoEditor ();

	virtual const wchar_t * type () { return L"InfoEditor"; }

	virtual bool isScriptable () { return false; }

	virtual void showOpen ();
	virtual void clear ();
	virtual void setFontSize (int fontSize);
	virtual int openDocument (MelderFile file);
	virtual void newDocument ();
	virtual int saveDocument (MelderFile file);
	virtual void closeDocument ();
	virtual bool getSelectedLines (long *firstLine, long *lastLine);
	virtual void do_find ();
	virtual void do_replace ();
	virtual const wchar_t * getName ();
	virtual void menu_new (EditorCommand *cmd);

	structMelderFile _file;
	GuiObject _textWidget;
	UiInfile *_openDialog;
	UiOutfile *_saveDialog;
	UiForm *_printDialog, *_findDialog;
	int _dirty, _fontSize;
	GuiObject _dirtyNewDialog, _dirtyOpenDialog, _dirtyCloseDialog;
	GuiObject _fontSizeButton_10, _fontSizeButton_12, _fontSizeButton_14, _fontSizeButton_18, _fontSizeButton_24;

  protected:
	virtual void goAway ();
	virtual void updateSizeMenu ();
	virtual void createMenus ();
	virtual void createChildren ();
};

/* End of file InfoEditor.h */
#endif
