/* StringsEditor.c
 *
 * Copyright (C) 2007-2008 Paul Boersma
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
 * pb 2007/12/19 created
 * pb 2008/02/06 const
 * pb 2008/03/20 split off Help menu
 */

#include "StringsEditor.h"
#include "EditorM.h"
#include "machine.h"

StringsEditor::StringsEditor (GuiObject parent, const wchar_t *title, Any data)
	: Editor (parent, 20, 40, 600, 600, title, data) {
	createMenus ();
	createChildren ();
	updateList ();
}

static int menu_cb_help (EDITOR_ARGS) {
	Melder_help (L"StringsEditor");
	return 1;
}

void StringsEditor::createMenus () {
	EditorMenu *menu = getMenu (L"Help");
	menu->addCommand (L"StringsEditor help", '?', menu_cb_help);
}

void StringsEditor::updateList () {
	Strings strings = (structStrings*)_data;
	GuiList_deleteAllItems (_list);
	for (long i = 1; i <= strings -> numberOfStrings; i ++)
		GuiList_insertItem (_list, strings -> strings [i], 0);
}

static void gui_button_cb_insert (I, GuiButtonEvent event) {
	(void) event;
	StringsEditor *stringsEditor = (StringsEditor *)void_me;
	Strings strings = (structStrings*)stringsEditor->_data;
	/*
	 * Find the first selected item.
	 */
	long numberOfSelected, *selected = GuiList_getSelectedPositions (stringsEditor->_list, & numberOfSelected);
	long position = selected == NULL ? strings -> numberOfStrings + 1 : selected [1];
	NUMlvector_free (selected, 1);
	wchar_t *text = GuiText_getString (stringsEditor->_text);
	/*
	 * Change the data.
	 */
	Strings_insert (strings, position, text);
	/*
	 * Change the list.
	 */
	GuiList_insertItem (stringsEditor->_list, text, position);
	GuiList_deselectAllItems (stringsEditor->_list);
	GuiList_selectItem (stringsEditor->_list, position);
	/*
	 * Clean up.
	 */
	Melder_free (text);
	stringsEditor->broadcastChange ();
}

static void gui_button_cb_append (I, GuiButtonEvent event) {
	(void) event;
	StringsEditor *stringsEditor = (StringsEditor *)void_me;
	Strings strings = (structStrings*)stringsEditor->_data;
	wchar_t *text = GuiText_getString (stringsEditor->_text);
	/*
	 * Change the data.
	 */
	Strings_insert (strings, 0, text);
	/*
	 * Change the list.
	 */
	GuiList_insertItem (stringsEditor->_list, text, 0);
	GuiList_deselectAllItems (stringsEditor->_list);
	GuiList_selectItem (stringsEditor->_list, strings -> numberOfStrings);
	/*
	 * Clean up.
	 */
	Melder_free (text);
	stringsEditor->broadcastChange ();
}

static void gui_button_cb_remove (I, GuiButtonEvent event) {
	(void) event;
	StringsEditor *stringsEditor = (StringsEditor *)void_me;
	long numberOfSelected, *selected = GuiList_getSelectedPositions (stringsEditor->_list, & numberOfSelected);
	for (long iselected = numberOfSelected; iselected >= 1; iselected --) {
		Strings_remove ((structStrings*)stringsEditor->_data, selected [iselected]);
	}
	NUMlvector_free (selected, 1);
	stringsEditor->updateList ();
	stringsEditor->broadcastChange ();
}

static void gui_button_cb_replace (I, GuiButtonEvent event) {
	(void) event;
	StringsEditor *stringsEditor = (StringsEditor *)void_me;
	Strings strings = (structStrings*)stringsEditor->_data;
	long numberOfSelected, *selected = GuiList_getSelectedPositions (stringsEditor->_list, & numberOfSelected);
	wchar_t *text = GuiText_getString (stringsEditor->_text);
	for (long iselected = 1; iselected <= numberOfSelected; iselected ++) {
		Strings_replace (strings, selected [iselected], text);
		GuiList_replaceItem (stringsEditor->_list, text, selected [iselected]);
	}
	Melder_free (text);
	stringsEditor->broadcastChange ();
}

static void gui_list_cb_doubleClick (GuiObject widget, void *void_me, long item) {
	(void) widget;
	StringsEditor *stringsEditor = (StringsEditor *)void_me;
	Strings strings = (structStrings*)stringsEditor->_data;
	if (item <= strings -> numberOfStrings)
		GuiText_setString (stringsEditor->_text, strings -> strings [item]);
}

void StringsEditor::createChildren () {
	_list = GuiList_create (_dialog, 1, 0, Machine_getMenuBarHeight (), -70, true, NULL);
	//GuiList_setDoubleClickCallback (_list, gui_list_cb_doubleClick, me);
	GuiObject_show (_list);

	_text = GuiText_createShown (_dialog, 0, 0, Gui_AUTOMATIC, -40, 0);
	GuiButton_createShown (_dialog, 10, 100, Gui_AUTOMATIC, -10, L"Insert", gui_button_cb_insert, this, GuiButton_DEFAULT);
	GuiButton_createShown (_dialog, 110, 200, Gui_AUTOMATIC, -10, L"Append", gui_button_cb_append, this, 0);
	GuiButton_createShown (_dialog, 210, 300, Gui_AUTOMATIC, -10, L"Replace", gui_button_cb_replace, this, 0);
	GuiButton_createShown (_dialog, 310, 400, Gui_AUTOMATIC, -10, L"Remove", gui_button_cb_remove, this, 0);	
}

void StringsEditor::dataChanged () {
	updateList ();
}

/* End of file StringsEditor.c */
