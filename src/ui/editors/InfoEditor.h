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

#include "Editor.h"

class UiInfile;
class UiOutfile;

class InfoEditor : public Editor {
  public:
	static void prefs (void);

	InfoEditor (GuiObject parent, const wchar_t *initialText);
	virtual ~InfoEditor ();

	virtual void showOpen ();

	structMelderFile _file;
	GuiObject _textWidget;
	UiInfile *_openDialog;
	UiOutfile *_saveDialog;
	UiForm *_printDialog, *_findDialog;
	int _dirty, _fontSize;
	GuiObject _dirtyNewDialog, _dirtyOpenDialog, _dirtyCloseDialog;
	GuiObject _fontSizeButton_10, _fontSizeButton_12, _fontSizeButton_14, _fontSizeButton_18, _fontSizeButton_24;

  protected:
	static int menu_cb_saveAs (EDITOR_ARGS); // FIXME
	static void cb_showOpen (EditorCommand *cmd, UiForm *sendingForm, const wchar_t *sendingString, Interpreter *interpreter);

	virtual bool isScriptable () { return false; }

	virtual int openDocument (MelderFile file);
	virtual void newDocument ();
	virtual int saveDocument (MelderFile file);
	virtual void closeDocument ();
	virtual void menu_new (EditorCommand *cmd);

	virtual void goAway ();
	virtual void updateSizeMenu ();

  private:
	static void gui_text_cb_change (void *void_me, GuiTextEvent event);
	static int cb_open_ok (UiForm *sendingForm, const wchar_t *sendingString, Interpreter *interpreter, const wchar_t *invokingButtonTitle, bool modified, void *void_me);
	static int cb_saveAs_ok (UiForm *sendingForm, const wchar_t *sendingString, Interpreter *interpreter, const wchar_t *invokingButtonTitle, bool modified, I);
	static int menu_cb_clear (EDITOR_ARGS);
	static int menu_cb_undo (EDITOR_ARGS);
	static int menu_cb_redo (EDITOR_ARGS);
	static int menu_cb_cut (EDITOR_ARGS);
	static int menu_cb_copy (EDITOR_ARGS);
	static int menu_cb_paste (EDITOR_ARGS);
	static int menu_cb_erase (EDITOR_ARGS);
	static int menu_cb_find (EDITOR_ARGS);
	static int menu_cb_findAgain (EDITOR_ARGS);
	static int menu_cb_replace (EDITOR_ARGS);
	static int menu_cb_replaceAgain (EDITOR_ARGS);
	static int menu_cb_whereAmI (EDITOR_ARGS);
	static int menu_cb_goToLine (EDITOR_ARGS);
	static int menu_cb_convertToCString (EDITOR_ARGS);
	static int menu_cb_10 (EDITOR_ARGS);
	static int menu_cb_12 (EDITOR_ARGS);
	static int menu_cb_14 (EDITOR_ARGS);
	static int menu_cb_18 (EDITOR_ARGS);
	static int menu_cb_24 (EDITOR_ARGS);
	static int menu_cb_fontSize (EDITOR_ARGS);

	virtual const wchar_t * type () { return L"InfoEditor"; }

	void clear ();
	void setFontSize (int fontSize);
	bool getSelectedLines (long *firstLine, long *lastLine);
	void do_find ();
	void do_replace ();
	const wchar_t * getName ();

	void createMenus ();
	void createChildren ();
};

/* End of file InfoEditor.h */
#endif
