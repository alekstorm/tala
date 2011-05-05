#ifndef _StringsEditor_h_
#define _StringsEditor_h_
/* StringsEditor.h
 *
 * Copyright (C) 1993-2011 David Weenink & Paul Boersma
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 * 2011/03/02
*/

#include "Editor.h"
#include "Strings.h"

class StringsEditor : public Editor {
  public:
	StringsEditor (GuiObject parent, const wchar_t *title, Any data);

	GuiObject _list, _text;

  protected:
	virtual const wchar_t * type () { return L"StringsEditor"; }

	virtual void updateList ();
	virtual void createMenus ();
	virtual void createChildren ();
	virtual void dataChanged ();

  private:
	static int menu_cb_help (EDITOR_ARGS);
	static void gui_button_cb_insert (I, GuiButtonEvent event);
	static void gui_button_cb_append (I, GuiButtonEvent event);
	static void gui_button_cb_remove (I, GuiButtonEvent event);
	static void gui_button_cb_replace (I, GuiButtonEvent event);
	static void gui_list_cb_doubleClick (GuiObject widget, void *void_me, long item);
};

/* End of file StringsEditor.h */
#endif
