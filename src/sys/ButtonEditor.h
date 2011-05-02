#ifndef _ButtonEditor_h_
#define _ButtonEditor_h_
/* ButtonEditor.h
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

/*
 * pb 2011/03/02
 */

#include "HyperPage.h"
#include "praatP.h"

class ButtonEditor : public HyperPage {
  public:
	ButtonEditor (GuiObject parent);

	virtual const wchar_t * type () { return L"ButtonEditor"; }
	virtual bool isScriptable () { return false; }

	virtual void which (int show);
	virtual void draw ();
	virtual void drawMenuCommand (praat_Command cmd, long i);
	virtual void drawAction (praat_Command cmd, long i);
	virtual int goToPage (const wchar_t *title);
	virtual void createChildren ();
	virtual void createMenus ();

	int _show;
	GuiObject _button1, _button2, _button3, _button4, _button5;

  private:
	static void gui_radiobutton_cb_objects (I, GuiRadioButtonEvent event);
	static void gui_radiobutton_cb_picture (I, GuiRadioButtonEvent event);
	static void gui_radiobutton_cb_editors (I, GuiRadioButtonEvent event);
	static void gui_radiobutton_cb_actionsAM (I, GuiRadioButtonEvent event);
	static void gui_radiobutton_cb_actionsNZ (I, GuiRadioButtonEvent event);
	static int menu_cb_ButtonEditorHelp (EDITOR_ARGS);
};

/* End of file ButtonEditor.h */
#endif
