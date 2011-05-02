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

	virtual const wchar_t * type () { return L"TextEditor"; }

	virtual void nameChanged ();
	virtual void newDocument ();
	virtual int saveDocument (MelderFile file);
	virtual void menu_new (EditorCommand *cmd);
	virtual const wchar_t * getName ();

  protected:
	virtual void createMenus ();
	virtual void goAway ();

  private:
	static void gui_button_cb_saveAndOpen (void *void_me, GuiButtonEvent event);
	static void gui_button_cb_cancelOpen (void *void_me, GuiButtonEvent event);
	static void gui_button_cb_discardAndOpen (void *void_me, GuiButtonEvent event);
	static int menu_cb_open (EDITOR_ARGS);
	static int menu_cb_save (EDITOR_ARGS);
	static int menu_cb_reopen (EDITOR_ARGS);
	static void gui_button_cb_saveAndNew (void *void_me, GuiButtonEvent event);
	static void gui_button_cb_cancelNew (void *void_me, GuiButtonEvent event);
	static void gui_button_cb_discardAndNew (void *void_me, GuiButtonEvent event);
	static void gui_button_cb_saveAndClose (void *void_me, GuiButtonEvent event);
	static void gui_button_cb_cancelClose (void *void_me, GuiButtonEvent event);
	static void gui_button_cb_discardAndClose (void *void_me, GuiButtonEvent event);
	static int menu_cb_new (EDITOR_ARGS);
};

/* End of file TextEditor.h */
#endif
