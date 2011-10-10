/* TextEditor.c
 *
 * Copyright (C) 1997-2010 Paul Boersma
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
 * pb 2002/03/07 GPL
 * pb 2004/01/07 use GuiWindow_setDirty
 * pb 2004/01/28 MacOS X: use trick for ensuring dirtiness callback
 * pb 2005/06/28 font size
 * pb 2005/09/01 Undo and Redo buttons
 * pb 2006/08/09 guarded against closing when a file selector is open
 * pb 2006/10/28 erased MacOS 9 stuff
 * pb 2006/12/18 improved info
 * pb 2007/02/15 GuiText_updateChangeCountAfterSave
 * pb 2007/03/23 Go to line: guarded against uninitialized 'right'
 * pb 2007/05/30 save Unicode
 * pb 2007/06/12 more wchar_t
 * pb 2007/08/12 more wchar_t
 * pb 2007/10/05 less char
 * pb 2007/12/05 prefs
 * pb 2007/12/23 Gui
 * pb 2008/01/04 guard against multiple opening of same file
 * pb 2008/03/21 new Editor API
 * pb 2009/01/18 arguments to UiForm *callbacks
 * pb 2010/01/20 Reopen from disk
 * pb 2010/01/20 guard against Find Again before Find
 * fb 2010/02/26 tell Undo/Redo buttons to GuiText for (de)sensitivization
 * pb 2010/05/16 correct order and types in Font menu
 * pb 2010/07/29 removed GuiDialog_show
 * pb 2010/12/03 command "Convert to C string"
 */

#include "TextEditor.h"

#include "ui/machine.h"
#include "kar/longchar.h"
#include "kar/UnicodeData.h"

/***** TextEditor methods *****/

/* 'initialText' may be NULL. */

TextEditor::TextEditor (GuiObject parent, const wchar_t *initialText)
	: InfoEditor(parent, initialText) {
	createMenus ();
}

void TextEditor::gui_button_cb_saveAndOpen (void *void_me, GuiButtonEvent event) {
	(void) event;
	EditorCommand *cmd = (EditorCommand *) void_me;
	TextEditor *editor = (TextEditor *) cmd -> _editor;
	GuiObject_hide (editor->_dirtyOpenDialog);
	if (editor->_name) {
		if (! editor->saveDocument (& editor->_file)) { Melder_flushError (NULL); return; }
		cb_showOpen (cmd, NULL, NULL, NULL);
	} else {
		menu_cb_saveAs (editor, cmd, NULL, NULL, NULL);
	}
}

void TextEditor::gui_button_cb_cancelOpen (void *void_me, GuiButtonEvent event) {
	(void) event;
	EditorCommand *cmd = (EditorCommand *) void_me;
	TextEditor *editor = (TextEditor *) cmd -> _editor;
	GuiObject_hide (editor->_dirtyOpenDialog);
}

void TextEditor::gui_button_cb_discardAndOpen (void *void_me, GuiButtonEvent event) {
	(void) event;
	EditorCommand *cmd = (EditorCommand *) void_me;
	TextEditor *editor = (TextEditor *) cmd -> _editor;
	GuiObject_hide (editor->_dirtyOpenDialog);
	cb_showOpen (cmd, NULL, NULL, NULL);
}

int TextEditor::menu_cb_open (EDITOR_ARGS) {
	TextEditor *editor = (TextEditor *)editor_me;
	if (editor->_dirty) {
		if (editor->_dirtyOpenDialog == NULL) {
			int buttonWidth = 120, buttonSpacing = 20;
			editor->_dirtyOpenDialog = GuiDialog_create (editor->_shell,
				150, 70, Gui_LEFT_DIALOG_SPACING + 3 * buttonWidth + 2 * buttonSpacing + Gui_RIGHT_DIALOG_SPACING,
					Gui_TOP_DIALOG_SPACING + Gui_TEXTFIELD_HEIGHT + Gui_VERTICAL_DIALOG_SPACING_SAME + 2 * Gui_BOTTOM_DIALOG_SPACING + Gui_PUSHBUTTON_HEIGHT,
				L"Text changed", NULL, NULL, GuiDialog_MODAL);
			GuiLabel_createShown (editor->_dirtyOpenDialog,
				Gui_LEFT_DIALOG_SPACING, Gui_AUTOMATIC, Gui_TOP_DIALOG_SPACING, Gui_AUTOMATIC,
				L"The text has changed! Save changes?", 0);
			GuiObject buttonArea = GuiDialog_getButtonArea (editor->_dirtyOpenDialog);
			int x = Gui_LEFT_DIALOG_SPACING, y = - Gui_BOTTOM_DIALOG_SPACING;
			GuiButton_createShown (buttonArea,
				x, x + buttonWidth, y - Gui_PUSHBUTTON_HEIGHT, y,
				L"Discard & Open", gui_button_cb_discardAndOpen, cmd, 0);
			x += buttonWidth + buttonSpacing;
			GuiButton_createShown (buttonArea,
				x, x + buttonWidth, y - Gui_PUSHBUTTON_HEIGHT, y,
				L"Cancel", gui_button_cb_cancelOpen, cmd, 0);
			x += buttonWidth + buttonSpacing;
			GuiButton_createShown (buttonArea,
				x, x + buttonWidth, y - Gui_PUSHBUTTON_HEIGHT, y,
				L"Save & Open", gui_button_cb_saveAndOpen, cmd, 0);
		}
		GuiObject_show (editor->_dirtyOpenDialog);
	} else {
		cb_showOpen (cmd, sendingForm, sendingString, interpreter);
	}
	return 1;
}

int TextEditor::menu_cb_save (EDITOR_ARGS) {
	TextEditor *editor = (TextEditor *)editor_me;
	if (editor->_name) {
		if (! editor->saveDocument (& editor->_file)) return 0;
	} else {
		menu_cb_saveAs (editor, cmd, NULL, NULL, NULL);
	}
	return 1;
}

int TextEditor::menu_cb_reopen (EDITOR_ARGS) {
	TextEditor *editor = (TextEditor *)editor_me;
	if (editor->_name) {
		if (! editor->openDocument (& editor->_file)) return 0;
	} else {
		return Melder_error1 (L"Cannot reopen from disk, because the text has never been saved yet.");
	}
	return 1;
}

void TextEditor::gui_button_cb_saveAndNew (void *void_me, GuiButtonEvent event) {
	(void) event;
	EditorCommand *cmd = (EditorCommand *) void_me;
	TextEditor *editor = (TextEditor *) cmd -> _editor;
	GuiObject_hide (editor->_dirtyNewDialog);
	if (editor->_name) {
		if (! editor->saveDocument (& editor->_file)) { Melder_flushError (NULL); return; }
		editor->newDocument ();
	} else {
		menu_cb_saveAs (editor, cmd, NULL, NULL, NULL);
	}
}

void TextEditor::gui_button_cb_cancelNew (void *void_me, GuiButtonEvent event) {
	(void) event;
	EditorCommand *cmd = (EditorCommand *) void_me;
	TextEditor *editor = (TextEditor *) cmd -> _editor;
	GuiObject_hide (editor->_dirtyNewDialog);
}

void TextEditor::gui_button_cb_discardAndNew (void *void_me, GuiButtonEvent event) {
	(void) event;
	EditorCommand *cmd = (EditorCommand *) void_me;
	TextEditor *editor = (TextEditor *) cmd -> _editor;
	GuiObject_hide (editor->_dirtyNewDialog);
	editor->newDocument ();
}

void TextEditor::gui_button_cb_saveAndClose (void *void_me, GuiButtonEvent event) {
	(void) event;
	TextEditor *editor = (TextEditor *)void_me;
	GuiObject_hide (editor->_dirtyCloseDialog);
	if (editor->_name) {
		if (! editor->saveDocument (& editor->_file)) { Melder_flushError (NULL); return; }
		editor->goAway ();
	} else {
		menu_cb_saveAs (editor, editor->getMenuCommand (L"File", L"Save as..."), NULL, NULL, NULL);
	}
}

void TextEditor::gui_button_cb_cancelClose (void *void_me, GuiButtonEvent event) {
	(void) event;
	TextEditor *editor = (TextEditor *)void_me;
	GuiObject_hide (editor->_dirtyCloseDialog);
}

void TextEditor::gui_button_cb_discardAndClose (void *void_me, GuiButtonEvent event) {
	(void) event;
	TextEditor *editor = (TextEditor *)void_me;
	GuiObject_hide (editor->_dirtyCloseDialog);
	editor->_dirty = FALSE;
	editor->goAway ();
}

void TextEditor::menu_new (EditorCommand *cmd) {
	if (_dirty) {
		if (! _dirtyNewDialog) {
			int buttonWidth = 120, buttonSpacing = 20;
			_dirtyNewDialog = GuiDialog_create (_shell,
				150, 70, Gui_LEFT_DIALOG_SPACING + 3 * buttonWidth + 2 * buttonSpacing + Gui_RIGHT_DIALOG_SPACING,
					Gui_TOP_DIALOG_SPACING + Gui_TEXTFIELD_HEIGHT + Gui_VERTICAL_DIALOG_SPACING_SAME + 2 * Gui_BOTTOM_DIALOG_SPACING + Gui_PUSHBUTTON_HEIGHT,
				L"Text changed", NULL, NULL, GuiDialog_MODAL);
			GuiLabel_createShown (_dirtyNewDialog,
				Gui_LEFT_DIALOG_SPACING, Gui_AUTOMATIC, Gui_TOP_DIALOG_SPACING, Gui_AUTOMATIC,
				L"The text has changed! Save changes?", 0);
			GuiObject buttonArea = GuiDialog_getButtonArea (_dirtyNewDialog);
			int x = Gui_LEFT_DIALOG_SPACING, y = - Gui_BOTTOM_DIALOG_SPACING;
			GuiButton_createShown (buttonArea,
				x, x + buttonWidth, y - Gui_PUSHBUTTON_HEIGHT, y,
				L"Discard & New", gui_button_cb_discardAndNew, cmd, 0);
			x += buttonWidth + buttonSpacing;
			GuiButton_createShown (buttonArea,
				x, x + buttonWidth, y - Gui_PUSHBUTTON_HEIGHT, y,
				L"Cancel", gui_button_cb_cancelNew, cmd, 0);
			x += buttonWidth + buttonSpacing;
			GuiButton_createShown (buttonArea,
				x, x + buttonWidth, y - Gui_PUSHBUTTON_HEIGHT, y,
				L"Save & New", gui_button_cb_saveAndNew, cmd, 0);
		}
		GuiObject_show (_dirtyNewDialog);
	}
	else
		InfoEditor::menu_new (cmd);
}

int TextEditor::menu_cb_new (EDITOR_ARGS) {
	((TextEditor *)editor_me)->menu_new (cmd);
	return 1;
}

void TextEditor::createMenus () {
	EditorMenu *menu = getMenu (L"File");
	menu->addCommand (L"New", 'N', menu_cb_new);
	menu->addCommand (L"Open...", 'O', menu_cb_open);
	menu->addCommand (L"Reopen from disk", 0, menu_cb_reopen);
	menu->addCommand (L"-- save --", 0, NULL);
	menu->addCommand (L"Save", 'S', menu_cb_save);
	menu->addCommand (L"-- close --", 0, NULL);
}

void TextEditor::nameChanged () {
	int dirtinessAlreadyShown = GuiWindow_setDirty (_shell, _dirty);
	static MelderString windowTitle = { 0 };
	MelderString_empty (& windowTitle);
	if (_name == NULL) {
		MelderString_append (& windowTitle, L"(untitled");
		if (_dirty && ! dirtinessAlreadyShown) MelderString_append (& windowTitle, L", modified");
		MelderString_append (& windowTitle, L")");
	} else {
		MelderString_append3 (& windowTitle, L"File " UNITEXT_LEFT_DOUBLE_QUOTATION_MARK, MelderFile_messageName (& _file), UNITEXT_RIGHT_DOUBLE_QUOTATION_MARK);
		if (_dirty && ! dirtinessAlreadyShown) MelderString_append (& windowTitle, L" (modified)");
	}
	GuiWindow_setTitle (_shell, windowTitle.string);
	MelderString_empty (& windowTitle);
	MelderString_append2 (& windowTitle, _dirty && ! dirtinessAlreadyShown ? L"*" : L"", _name == NULL ? L"(untitled)" : MelderFile_name (& _file));
}

void TextEditor::newDocument () {
	InfoEditor::newDocument ();
	_name = NULL;
}

int TextEditor::saveDocument (MelderFile file) {
	int ret = InfoEditor::saveDocument (file);
	_name = Melder_wcsdup_f (Melder_fileToPath (file));
	return ret;
}

void TextEditor::goAway () {
	if (_dirty) {
		if (! _dirtyCloseDialog) {
			int buttonWidth = 120, buttonSpacing = 20;
			_dirtyCloseDialog = GuiDialog_create (_shell,
				150, 70, Gui_LEFT_DIALOG_SPACING + 3 * buttonWidth + 2 * buttonSpacing + Gui_RIGHT_DIALOG_SPACING,
					Gui_TOP_DIALOG_SPACING + Gui_TEXTFIELD_HEIGHT + Gui_VERTICAL_DIALOG_SPACING_SAME + 2 * Gui_BOTTOM_DIALOG_SPACING + Gui_PUSHBUTTON_HEIGHT,
				L"Text changed", NULL, NULL, GuiDialog_MODAL);
			GuiLabel_createShown (_dirtyCloseDialog,
				Gui_LEFT_DIALOG_SPACING, Gui_AUTOMATIC, Gui_TOP_DIALOG_SPACING, Gui_AUTOMATIC,
				L"The text has changed! Save changes?", 0);
			GuiObject buttonArea = GuiDialog_getButtonArea (_dirtyCloseDialog);
			int x = Gui_LEFT_DIALOG_SPACING, y = - Gui_BOTTOM_DIALOG_SPACING;
			GuiButton_createShown (buttonArea,
				x, x + buttonWidth, y - Gui_PUSHBUTTON_HEIGHT, y,
				L"Discard & Close", gui_button_cb_discardAndClose, this, 0);
			x += buttonWidth + buttonSpacing;
			GuiButton_createShown (buttonArea,
				x, x + buttonWidth, y - Gui_PUSHBUTTON_HEIGHT, y,
				L"Cancel", gui_button_cb_cancelClose, this, 0);
			x += buttonWidth + buttonSpacing;
			GuiButton_createShown (buttonArea,
				x, x + buttonWidth, y - Gui_PUSHBUTTON_HEIGHT, y,
				L"Save & Close", gui_button_cb_saveAndClose, this, 0);
		}
		GuiObject_show (_dirtyCloseDialog);
	}
	else
		Editor::goAway ();
}

const wchar_t * TextEditor::getName () {
	return _name ? MelderFile_name (& _file) : L"";
}

/* End of file TextEditor.c */
