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
#include "machine.h"
#include "kar/longchar.h"
#include "EditorM.h"
#include "Preferences.h"
#include "kar/UnicodeData.h"

static int theTextEditorFontSize;

void TextEditor::prefs (void) {
	Preferences_addInt (L"TextEditor.fontSize", & theTextEditorFontSize, 12);
}

static Collection theOpenTextEditors = NULL;

/***** TextEditor methods *****/

/* 'initialText' may be NULL. */
TextEditor::TextEditor (GuiObject parent, const wchar_t *initialText)
	: Editor (parent, 0, 0, 600, 400, NULL, NULL),
	  _fontSizeButton_10(NULL),
	  _fontSizeButton_12(NULL),
	  _fontSizeButton_14(NULL),
	  _fontSizeButton_18(NULL),
	  _fontSizeButton_24(NULL) {
	createMenus ();
	createChildren ();
	setFontSize (theTextEditorFontSize);
	if (initialText) {
		GuiText_setString (_textWidget, initialText);
		_dirty = FALSE;   /* Was set to TRUE in valueChanged callback. */
	}
	if (theOpenTextEditors == NULL) {
		theOpenTextEditors = Collection_create (NULL, 100);
	}
	if (theOpenTextEditors != NULL) {
		Collection_addItem (theOpenTextEditors, this);
	}
}

TextEditor::~TextEditor () {
	forget (_openDialog);
	forget (_saveDialog);
	forget (_printDialog);
	forget (_findDialog);
	if (theOpenTextEditors) {
		Collection_undangleItem (theOpenTextEditors, this);
	}
}

void TextEditor::nameChanged () {
	if (isFileBased ()) {
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
		#if motif	
			XtVaSetValues (_shell, XmNiconName, Melder_peekWcsToUtf8 (windowTitle.string), NULL);
		#endif
	} else {
		Editor::nameChanged ();
	}
}

int TextEditor::openDocument (MelderFile file) {
	if (theOpenTextEditors) {
		for (long ieditor = 1; ieditor <= theOpenTextEditors -> size; ieditor ++) {
			TextEditor *editor = (TextEditor*)theOpenTextEditors -> item [ieditor];
			if (editor != this && MelderFile_equal (file, & editor -> _file)) {
				editor->raise ();
				Melder_error3 (L"Text file ", MelderFile_messageName (file), L" is already open.");
				//forget (me);   // don't forget me before Melder_error, because "file" is owned by one of _dialogs // FIXME
				return 0;
			}
		}
	}
	wchar_t *text = MelderFile_readText (file);
	if (! text) return 0;
	GuiText_setString (_textWidget, text);
	Melder_free (text);
	/*
	 * GuiText_setString has invoked the changeCallback,
	 * which has set '_dirty' to TRUE. Fix this.
	 */
	_dirty = FALSE;
	MelderFile_copy (file, & _file);
	_name = Melder_wcsdup_f (Melder_fileToPath (file));
	return 1;
}

void TextEditor::newDocument () {
	GuiText_setString (_textWidget, L"");   /* Implicitly sets _dirty to TRUE. */
	_dirty = FALSE;
	if (isFileBased ()) _name = NULL;
}

int TextEditor::saveDocument (MelderFile file) {
	wchar_t *text = GuiText_getString (_textWidget);
	if (! MelderFile_writeText (file, text)) { Melder_free (text); return 0; }
	Melder_free (text);
	_dirty = FALSE;
	MelderFile_copy (file, & _file);
	if (isFileBased ()) _name = Melder_wcsdup_f (Melder_fileToPath (file));
	return 1;
}

void TextEditor::closeDocument () {}

static int cb_open_ok (UiForm *sendingForm, const wchar_t *sendingString, Interpreter *interpreter, const wchar_t *invokingButtonTitle, bool modified, void *void_me) {
	(void) sendingString;
	(void) interpreter;
	(void) invokingButtonTitle;
	(void) modified;
	TextEditor *editor = (TextEditor *)void_me;
	MelderFile file = ((UiFile *)sendingForm)->getFile ();
	if (! editor->openDocument (file)) return 0;
	return 1;
}

static void cb_showOpen (EditorCommand *cmd, UiForm *sendingForm, const wchar_t *sendingString, Interpreter *interpreter) {
	TextEditor *editor = (TextEditor *) cmd -> _editor;
	(void) sendingForm;
	(void) sendingString;
	(void) interpreter;
	if (! editor->_openDialog)
		editor->_openDialog = new UiInfile (editor->_dialog, L"Open", cb_open_ok, editor, NULL, NULL, false);
	editor->_openDialog -> do_ ();
}

static int cb_saveAs_ok (UiForm *sendingForm, const wchar_t *sendingString, Interpreter *interpreter, const wchar_t *invokingButtonTitle, bool modified, I) {
	TextEditor *editor = (TextEditor *)void_me;
	(void) sendingString;
	(void) interpreter;
	(void) invokingButtonTitle;
	(void) modified;
	MelderFile file = ((UiFile *)sendingForm)->getFile ();
	if (! editor->saveDocument (file)) return 0;
	return 1;
}

static int menu_cb_saveAs (EDITOR_ARGS) {
	TextEditor *editor = (TextEditor *)editor_me;
	wchar_t defaultName [300];
	if (! editor->_saveDialog)
		editor->_saveDialog = new UiOutfile (editor->_dialog, L"Save", cb_saveAs_ok, editor, NULL, NULL);
	swprintf (defaultName, 300, ! editor->isFileBased () ? L"info.txt" : editor->_name ? MelderFile_name (& editor->_file) : L"");
	editor->_saveDialog -> do_ (defaultName);
	return 1;
}

static void gui_button_cb_saveAndOpen (void *void_me, GuiButtonEvent event) {
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

static void gui_button_cb_cancelOpen (void *void_me, GuiButtonEvent event) {
	(void) event;
	EditorCommand *cmd = (EditorCommand *) void_me;
	TextEditor *editor = (TextEditor *) cmd -> _editor;
	GuiObject_hide (editor->_dirtyOpenDialog);
}

static void gui_button_cb_discardAndOpen (void *void_me, GuiButtonEvent event) {
	(void) event;
	EditorCommand *cmd = (EditorCommand *) void_me;
	TextEditor *editor = (TextEditor *) cmd -> _editor;
	GuiObject_hide (editor->_dirtyOpenDialog);
	cb_showOpen (cmd, NULL, NULL, NULL);
}

static int menu_cb_open (EDITOR_ARGS) {
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

static void gui_button_cb_saveAndNew (void *void_me, GuiButtonEvent event) {
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

static void gui_button_cb_cancelNew (void *void_me, GuiButtonEvent event) {
	(void) event;
	EditorCommand *cmd = (EditorCommand *) void_me;
	TextEditor *editor = (TextEditor *) cmd -> _editor;
	GuiObject_hide (editor->_dirtyNewDialog);
}

static void gui_button_cb_discardAndNew (void *void_me, GuiButtonEvent event) {
	(void) event;
	EditorCommand *cmd = (EditorCommand *) void_me;
	TextEditor *editor = (TextEditor *) cmd -> _editor;
	GuiObject_hide (editor->_dirtyNewDialog);
	editor->newDocument ();
}

static int menu_cb_new (EDITOR_ARGS) {
	TextEditor *editor = (TextEditor *)editor_me;
	if (editor->isFileBased () && editor->_dirty) {
		if (! editor->_dirtyNewDialog) {
			int buttonWidth = 120, buttonSpacing = 20;
			editor->_dirtyNewDialog = GuiDialog_create (editor->_shell,
				150, 70, Gui_LEFT_DIALOG_SPACING + 3 * buttonWidth + 2 * buttonSpacing + Gui_RIGHT_DIALOG_SPACING,
					Gui_TOP_DIALOG_SPACING + Gui_TEXTFIELD_HEIGHT + Gui_VERTICAL_DIALOG_SPACING_SAME + 2 * Gui_BOTTOM_DIALOG_SPACING + Gui_PUSHBUTTON_HEIGHT,
				L"Text changed", NULL, NULL, GuiDialog_MODAL);
			GuiLabel_createShown (editor->_dirtyNewDialog,
				Gui_LEFT_DIALOG_SPACING, Gui_AUTOMATIC, Gui_TOP_DIALOG_SPACING, Gui_AUTOMATIC,
				L"The text has changed! Save changes?", 0);
			GuiObject buttonArea = GuiDialog_getButtonArea (editor->_dirtyNewDialog);
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
		GuiObject_show (editor->_dirtyNewDialog);
	} else {
		editor->newDocument ();
	}
	return 1;
}

static int menu_cb_clear (EDITOR_ARGS) {
	((TextEditor *)editor_me)->clear ();
	return 1;
}

static int menu_cb_save (EDITOR_ARGS) {
	TextEditor *editor = (TextEditor *)editor_me;
	if (editor->_name) {
		if (! editor->saveDocument (& editor->_file)) return 0;
	} else {
		menu_cb_saveAs (editor, cmd, NULL, NULL, NULL);
	}
	return 1;
}

static int menu_cb_reopen (EDITOR_ARGS) {
	TextEditor *editor = (TextEditor *)editor_me;
	if (editor->_name) {
		if (! editor->openDocument (& editor->_file)) return 0;
	} else {
		return Melder_error1 (L"Cannot reopen from disk, because the text has never been saved yet.");
	}
	return 1;
}

static void gui_button_cb_saveAndClose (void *void_me, GuiButtonEvent event) {
	(void) event;
	TextEditor *editor = (TextEditor *)void_me;
	GuiObject_hide (editor->_dirtyCloseDialog);
	if (editor->_name) {
		if (! editor->saveDocument (& editor->_file)) { Melder_flushError (NULL); return; }
		editor->closeDocument ();
	} else {
		menu_cb_saveAs (editor, editor->Editor::getMenuCommand (L"File", L"Save as..."), NULL, NULL, NULL);
	}
}

static void gui_button_cb_cancelClose (void *void_me, GuiButtonEvent event) {
	(void) event;
	TextEditor *editor = (TextEditor *)void_me;
	GuiObject_hide (editor->_dirtyCloseDialog);
}

static void gui_button_cb_discardAndClose (void *void_me, GuiButtonEvent event) {
	(void) event;
	TextEditor *editor = (TextEditor *)void_me;
	GuiObject_hide (editor->_dirtyCloseDialog);
	editor->closeDocument ();
}

void TextEditor::goAway () {
	if (isFileBased () && _dirty) {
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
	} else {
		closeDocument ();
	}
}

static int menu_cb_undo (EDITOR_ARGS) {
	TextEditor *editor = (TextEditor *)editor_me;
	GuiText_undo (editor->_textWidget);
	return 1;
}

static int menu_cb_redo (EDITOR_ARGS) {
	TextEditor *editor = (TextEditor *)editor_me;
	GuiText_redo (editor->_textWidget);
	return 1;
}

static int menu_cb_cut (EDITOR_ARGS) {
	TextEditor *editor = (TextEditor *)editor_me;
	GuiText_cut (editor->_textWidget);  // use ((XmAnyCallbackStruct *) call) -> event -> xbutton. time
	return 1;
}

static int menu_cb_copy (EDITOR_ARGS) {
	TextEditor *editor = (TextEditor *)editor_me;
	GuiText_copy (editor->_textWidget);
	return 1;
}

static int menu_cb_paste (EDITOR_ARGS) {
	TextEditor *editor = (TextEditor *)editor_me;
	GuiText_paste (editor->_textWidget);
	return 1;
}

static int menu_cb_erase (EDITOR_ARGS) {
	TextEditor *editor = (TextEditor *)editor_me;
	GuiText_remove (editor->_textWidget);
	return 1;
}

bool TextEditor::getSelectedLines (long *firstLine, long *lastLine) {
	long left, right;
	wchar_t *text = GuiText_getStringAndSelectionPosition (_textWidget, & left, & right);
	long textLength = wcslen (text);
	Melder_assert (left >= 0);
	Melder_assert (left <= right);
	Melder_assert (right <= textLength);
	long i = 0;
	*firstLine = 1;
	/*
	 * Cycle through the text in order to see how many linefeeds we pass.
	 */
	for (; i < left; i ++) {
		if (text [i] == '\n') {
			(*firstLine) ++;
		}
	}
	if (left == right) return false;
	*lastLine = *firstLine;
	for (; i < right; i ++) {
		if (text [i] == '\n') {
			(*lastLine) ++;
		}
	}
	Melder_free (text);
	return true;
}

static wchar_t *theFindString = NULL, *theReplaceString = NULL;
void TextEditor::do_find () {
	if (theFindString == NULL) return;   // e.g. when the user does "Find again" before having done any "Find"
	long left, right;
	wchar_t *text = GuiText_getStringAndSelectionPosition (_textWidget, & left, & right);
	wchar_t *location = wcsstr (text + right, theFindString);
	if (location != NULL) {
		long index = location - text;
		GuiText_setSelection (_textWidget, index, index + wcslen (theFindString));
		GuiText_scrollToSelection (_textWidget);
		#ifdef _WIN32
			GuiObject_show (_dialog);
		#endif
	} else {
		/* Try from the start of the document. */
		location = wcsstr (text, theFindString);
		if (location != NULL) {
			long index = location - text;
			GuiText_setSelection (_textWidget, index, index + wcslen (theFindString));
			GuiText_scrollToSelection (_textWidget);
			#ifdef _WIN32
				GuiObject_show (_dialog);
			#endif
		} else {
			Melder_beep ();
		}
	}
	Melder_free (text);
}

void TextEditor::do_replace () {
	if (theReplaceString == NULL) return;   // e.g. when the user does "Replace again" before having done any "Replace"
	wchar_t *selection = GuiText_getSelection (_textWidget);
	if (! Melder_wcsequ (selection, theFindString)) {
		do_find ();
		return;
	}
	long left, right;
	wchar_t *text = GuiText_getStringAndSelectionPosition (_textWidget, & left, & right);
	Melder_free (text);
	GuiText_replace (_textWidget, left, right, theReplaceString);
	GuiText_setSelection (_textWidget, left, left + wcslen (theReplaceString));
	GuiText_scrollToSelection (_textWidget);
	#ifdef _WIN32
		GuiObject_show (_dialog);
	#endif
}

static int menu_cb_find (EDITOR_ARGS) {
	TextEditor *editor = (TextEditor *)editor_me;
	EDITOR_FORM (L"Find", 0)
		LABEL (L"", L"Find:")
		TEXTFIELD (L"findString", L"")
	EDITOR_OK
		if (theFindString != NULL) SET_STRING (L"findString", theFindString);
	EDITOR_DO
		Melder_free (theFindString);
		theFindString = Melder_wcsdup_f (GET_STRING (L"findString"));
		editor->do_find ();
	EDITOR_END
}

static int menu_cb_findAgain (EDITOR_ARGS) {
	TextEditor *editor = (TextEditor *)editor_me;
	editor->do_find ();
	return 1;
}

static int menu_cb_replace (EDITOR_ARGS) {
	TextEditor *editor = (TextEditor *)editor_me;
	EDITOR_FORM (L"Find", 0)
		LABEL (L"", L"This is a \"slow\" find-and-replace method;")
		LABEL (L"", L"if the selected text is identical to the Find string,")
		LABEL (L"", L"the selected text will be replaced by the Replace string;")
		LABEL (L"", L"otherwise, the next occurrence of the Find string will be selected.")
		LABEL (L"", L"So you typically need two clicks on Apply to get a text replaced.")
		LABEL (L"", L"Find:")
		TEXTFIELD (L"findString", L"")
		LABEL (L"", L"Replace with:")
		TEXTFIELD (L"replaceString", L"")
	EDITOR_OK
		if (theFindString != NULL) SET_STRING (L"findString", theFindString);
		if (theReplaceString != NULL) SET_STRING (L"replaceString", theReplaceString);
	EDITOR_DO
		Melder_free (theFindString);
		theFindString = Melder_wcsdup_f (GET_STRING (L"findString"));
		Melder_free (theReplaceString);
		theReplaceString = Melder_wcsdup_f (GET_STRING (L"replaceString"));
		editor->do_replace ();
	EDITOR_END
}

static int menu_cb_replaceAgain (EDITOR_ARGS) {
	TextEditor *editor = (TextEditor *)editor_me;
	editor->do_replace ();
	return 1;
}

static int menu_cb_whereAmI (EDITOR_ARGS) {
	TextEditor *editor = (TextEditor *)editor_me;
	long numberOfLinesLeft, numberOfLinesRight;
	if (! editor->getSelectedLines (& numberOfLinesLeft, & numberOfLinesRight)) {
		Melder_information3 (L"The cursor is on line ", Melder_integer (numberOfLinesLeft), L".");
	} else if (numberOfLinesLeft == numberOfLinesRight) {
		Melder_information3 (L"The selection is on line ", Melder_integer (numberOfLinesLeft), L".");
	} else {
		Melder_information5 (L"The selection runs from line ", Melder_integer (numberOfLinesLeft),
			L" to line ", Melder_integer (numberOfLinesRight), L".");
	}
	return 1;
}

static int menu_cb_goToLine (EDITOR_ARGS) {
	TextEditor *editor = (TextEditor *)editor_me;
	EDITOR_FORM (L"Go to line", 0)
		NATURAL (L"Line", L"1")
	EDITOR_OK
		long firstLine, lastLine;
		editor->getSelectedLines (& firstLine, & lastLine);
		SET_INTEGER (L"Line", firstLine);
	EDITOR_DO
		wchar_t *text = GuiText_getString (editor->_textWidget);
		long lineToGo = GET_INTEGER (L"Line"), currentLine = 1;
		unsigned long left = 0, right = 0;
		if (lineToGo == 1) {
			for (; text [right] != '\n' && text [right] != '\0'; right ++) { }
		} else {
			for (; text [left] != '\0'; left ++) {
				if (text [left] == '\n') {
					currentLine ++;
					if (currentLine == lineToGo) {
						left ++;
						for (right = left; text [right] != '\n' && text [right] != '\0'; right ++) { }
						break;
					}
				}
			}
		}
		if (left == wcslen (text)) {
			right = left;
		} else if (text [right] == '\n') {
			right ++;
		}
		Melder_free (text);
		GuiText_setSelection (editor->_textWidget, left, right);
		GuiText_scrollToSelection (editor->_textWidget);
	EDITOR_END
}

static int menu_cb_convertToCString (EDITOR_ARGS) {
	TextEditor *editor = (TextEditor *)editor_me;
	wchar_t *text = GuiText_getString (editor->_textWidget);
	wchar_t buffer [2] = L" ";
	wchar_t *hex [16] = { L"0", L"1", L"2", L"3", L"4", L"5", L"6", L"7", L"8", L"9", L"A", L"B", L"C", L"D", L"E", L"F" };
	MelderInfo_open ();
	MelderInfo_write1 (L"\"");
	for (wchar_t *p = & text [0]; *p != '\0'; p ++) {
		if (*p == '\n') {
			MelderInfo_write1 (L"\\n\"\n\"");
		} else if (*p == '\t') {
			MelderInfo_write1 (L"   ");
		} else if (*p == '\"') {
			MelderInfo_write1 (L"\\\"");
		} else if (*p == '\\') {
			MelderInfo_write1 (L"\\\\");
		} else if (*p < 0 || *p > 127) {
			uint32_t kar = *p;
			if (kar <= 0xFFFF) {
				MelderInfo_write5 (L"\\u", hex [kar >> 12], hex [(kar >> 8) & 0x0000000F], hex [(kar >> 4) & 0x0000000F], hex [kar & 0x0000000F]);
			} else {
				MelderInfo_write9 (L"\\U", hex [kar >> 28], hex [(kar >> 24) & 0x0000000F], hex [(kar >> 20) & 0x0000000F], hex [(kar >> 16) & 0x0000000F],
					hex [(kar >> 12) & 0x0000000F], hex [(kar >> 8) & 0x0000000F], hex [(kar >> 4) & 0x0000000F], hex [kar & 0x0000000F]);
			}
		} else {
			buffer [0] = *p;
			MelderInfo_write1 (& buffer [0]);
		}
	}
	MelderInfo_write1 (L"\"");
	MelderInfo_close ();
	Melder_free (text);
	return 1;
}

/***** 'Font' menu *****/

void TextEditor::updateSizeMenu () {
	if (_fontSizeButton_10) GuiMenuItem_check (_fontSizeButton_10, _fontSize == 10);
	if (_fontSizeButton_12) GuiMenuItem_check (_fontSizeButton_12, _fontSize == 12);
	if (_fontSizeButton_14) GuiMenuItem_check (_fontSizeButton_14, _fontSize == 14);
	if (_fontSizeButton_18) GuiMenuItem_check (_fontSizeButton_18, _fontSize == 18);
	if (_fontSizeButton_24) GuiMenuItem_check (_fontSizeButton_24, _fontSize == 24);
}
void TextEditor::setFontSize (int fontSize) {
	GuiText_setFontSize (_textWidget, fontSize);
	theTextEditorFontSize = _fontSize = fontSize;
	updateSizeMenu ();
}

static int menu_cb_10 (EDITOR_ARGS) { ((TextEditor *)editor_me)->setFontSize (10); return 1; }
static int menu_cb_12 (EDITOR_ARGS) { ((TextEditor *)editor_me)->setFontSize (12); return 1; }
static int menu_cb_14 (EDITOR_ARGS) { ((TextEditor *)editor_me)->setFontSize (14); return 1; }
static int menu_cb_18 (EDITOR_ARGS) { ((TextEditor *)editor_me)->setFontSize (18); return 1; }
static int menu_cb_24 (EDITOR_ARGS) { ((TextEditor *)editor_me)->setFontSize (24); return 1; }
static int menu_cb_fontSize (EDITOR_ARGS) {
	TextEditor *editor = (TextEditor *)editor_me;
	EDITOR_FORM (L"Text window: Font size", 0)
		NATURAL (L"Font size (points)", L"12")
	EDITOR_OK
		SET_INTEGER (L"Font size", (long) editor->_fontSize);
	EDITOR_DO
		editor->setFontSize (GET_INTEGER (L"Font size"));
	EDITOR_END
}

void TextEditor::createMenus () {
	if (isFileBased ()) {
		Editor::addCommand (L"File", L"New", 'N', menu_cb_new);
		Editor::addCommand (L"File", L"Open...", 'O', menu_cb_open);
		Editor::addCommand (L"File", L"Reopen from disk", 0, menu_cb_reopen);
	} else {
		Editor::addCommand (L"File", L"Clear", 'N', menu_cb_clear);
	}
	Editor::addCommand (L"File", L"-- save --", 0, NULL);
	if (isFileBased ()) {
		Editor::addCommand (L"File", L"Save", 'S', menu_cb_save);
		Editor::addCommand (L"File", L"Save as...", 0, menu_cb_saveAs);
	} else {
		Editor::addCommand (L"File", L"Save as...", 'S', menu_cb_saveAs);
	}
	Editor::addCommand (L"File", L"-- close --", 0, NULL);
	Editor::addCommand (L"Edit", L"Undo", 'Z', menu_cb_undo);
	Editor::addCommand (L"Edit", L"Redo", 'Y', menu_cb_redo);
	Editor::addCommand (L"Edit", L"-- cut copy paste --", 0, NULL);
	Editor::addCommand (L"Edit", L"Cut", 'X', menu_cb_cut);
	Editor::addCommand (L"Edit", L"Copy", 'C', menu_cb_copy);
	Editor::addCommand (L"Edit", L"Paste", 'V', menu_cb_paste);
	Editor::addCommand (L"Edit", L"Erase", 0, menu_cb_erase);
	Editor::addMenu (L"Search", 0);
	Editor::addCommand (L"Search", L"Find...", 'F', menu_cb_find);
	Editor::addCommand (L"Search", L"Find again", 'G', menu_cb_findAgain);
	Editor::addCommand (L"Search", L"Replace...", GuiMenu_SHIFT + 'F', menu_cb_replace);
	Editor::addCommand (L"Search", L"Replace again", GuiMenu_SHIFT + 'G', menu_cb_replaceAgain);
	Editor::addCommand (L"Search", L"-- line --", 0, NULL);
	Editor::addCommand (L"Search", L"Where am I?", 0, menu_cb_whereAmI);
	Editor::addCommand (L"Search", L"Go to line...", 'L', menu_cb_goToLine);
	Editor::addMenu (L"Convert", 0);
	Editor::addCommand (L"Convert", L"Convert to C string", 0, menu_cb_convertToCString);
	#ifdef macintosh
		Editor::addMenu (L"Font", 0);
		Editor::addCommand (L"Font", L"Font size...", 0, menu_cb_fontSize);
		_fontSizeButton_10 = Editor::addCommand (L"Font", L"10", GuiMenu_CHECKBUTTON, menu_cb_10);
		_fontSizeButton_12 = Editor::addCommand (L"Font", L"12", GuiMenu_CHECKBUTTON, menu_cb_12);
		_fontSizeButton_14 = Editor::addCommand (L"Font", L"14", GuiMenu_CHECKBUTTON, menu_cb_14);
		_fontSizeButton_18 = Editor::addCommand (L"Font", L"18", GuiMenu_CHECKBUTTON, menu_cb_18);
		_fontSizeButton_24 = Editor::addCommand (L"Font", L"24", GuiMenu_CHECKBUTTON, menu_cb_24);
	#endif
}

static void gui_text_cb_change (void *void_me, GuiTextEvent event) {
	(void) event;
	TextEditor *editor = (TextEditor *)void_me;
	if (! editor->_dirty) {
		editor->_dirty = TRUE;
		editor->nameChanged ();
	}
}

void TextEditor::createChildren () {
	_textWidget = GuiText_createShown (_dialog, 0, 0, Machine_getMenuBarHeight (), 0, GuiText_SCROLLED);
	GuiText_setChangeCallback (_textWidget, gui_text_cb_change, this);
	GuiText_setUndoItem (_textWidget, Editor::getMenuCommand (L"Edit", L"Undo") -> _itemWidget);
	GuiText_setRedoItem (_textWidget, Editor::getMenuCommand (L"Edit", L"Redo") -> _itemWidget);
}

void TextEditor::clear () {}

void TextEditor::createMenuItems_query (EditorMenu *menu) {}

void TextEditor::showOpen () {
	cb_showOpen (Editor::getMenuCommand (L"File", L"Open..."), NULL, NULL, NULL);
}

/* End of file TextEditor.c */
