#ifndef _CategoriesEditor_h_
#define _CategoriesEditor_h_
/* CategoriesEditor.h
 *
 * Copyright (C) 1993-2011 David Weenink
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
 djmw 19950713
 djmw 20020813 GPL header
 djmw 20110305 Latest modification.
*/

#include "dwtools/Categories.h"
#include "dwsys/Command.h"
#include "Editor.h"

class CategoriesEditor : public Editor {
  public:
	CategoriesEditor (GuiObject parent, const wchar_t *title, Any data);
	virtual ~CategoriesEditor ();

	virtual void update (long from, long to, const long *select, long nSelect);

	CommandHistory _history;
	int _position;
	GuiObject _list, _text, _outOfView, _undo, _redo;
	GuiObject _remove, _insert, _insertAtEnd, _replace, _moveUp, _moveDown;

  protected:
	virtual void dataChanged ();

  private:
	static int menu_cb_help (EDITOR_ARGS);
	static void gui_button_cb_remove (I, GuiButtonEvent event);
	static void gui_button_cb_insert (I, GuiButtonEvent event);
	static void gui_button_cb_insertAtEnd (I, GuiButtonEvent event);
	static void gui_button_cb_replace (I, GuiButtonEvent event);
	static void gui_button_cb_moveUp (I, GuiButtonEvent event);
	static void gui_button_cb_moveDown (I, GuiButtonEvent event);
	static void gui_cb_scroll (GUI_ARGS);
	static void gui_list_cb_double_click (void *void_me, GuiListEvent event);
	static void gui_list_cb_extended (void *void_me, GuiListEvent event);
	static void gui_button_cb_undo (I, GuiButtonEvent event);
	static void gui_button_cb_redo (I, GuiButtonEvent event);

	virtual const wchar_t * type () { return L"CategoriesEditor"; }

	void notifyOutOfView ();
	void update_dos ();
	void updateWidgets (); /*all buttons except undo & redo */
	void insert (int position);

	void createMenus ();
	void createChildren ();
};

#endif /* _CategoriesEditor_h_ */
