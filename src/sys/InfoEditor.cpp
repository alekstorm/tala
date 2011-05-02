/* InfoEditor.c
 *
 * Copyright (C) 2004-2010 Paul Boersma
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
 * pb 2004/09/13
 * pb 2004/10/21 clear method also clears the info buffer, not just the visible text
 * pb 2007/05/24 wchar_t
 * pb 2007/06/09 more wchar_t
 * pb 2007/12/31 Gui
 * sdk 2008/03/24 Gui
 * pb 2010/07/29 removed GuiDialog_show
 */

#include "InfoEditor.h"
#include "EditorM.h"
#include "Preferences.h"
#include "machine.h"

// FIXME
static InfoEditor *theInfoEditor;
static Collection theOpenTextEditors = NULL;

static int theTextEditorFontSize;

void InfoEditor::prefs (void) {
	Preferences_addInt (L"InfoEditor.fontSize", & theTextEditorFontSize, 12);
}

InfoEditor::InfoEditor (GuiObject parent, const wchar_t *initialText)
	: Editor (parent, 0, 0, 600, 400, NULL, NULL),
	  _textWidget(NULL),
	  _openDialog(NULL),
	  _saveDialog(NULL),
	  _printDialog(NULL),
	  _findDialog(NULL),
	  _dirty(FALSE),
	  _dirtyNewDialog(NULL),
	  _dirtyOpenDialog(NULL),
	  _dirtyCloseDialog(NULL),
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
	}
	if (theOpenTextEditors == NULL) {
		theOpenTextEditors = Collection_create (NULL, 100);
	}
	if (theOpenTextEditors != NULL) {
		Collection_addItem (theOpenTextEditors, this);
	}
}

InfoEditor::~InfoEditor () {
	forget (_openDialog);
	forget (_saveDialog);
	forget (_printDialog);
	forget (_findDialog);
	if (theOpenTextEditors) {
		Collection_undangleItem (theOpenTextEditors, this);
	}
}

void InfoEditor::clear () {
	Melder_clearInfo ();
}

int InfoEditor::openDocument (MelderFile file) {
	if (theOpenTextEditors) {
		for (long ieditor = 1; ieditor <= theOpenTextEditors -> size; ieditor ++) {
			InfoEditor *editor = (InfoEditor*)theOpenTextEditors -> item [ieditor];
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

void InfoEditor::newDocument () {
	GuiText_setString (_textWidget, L"");   /* Implicitly sets _dirty to TRUE. */
	_dirty = FALSE;
}

int InfoEditor::saveDocument (MelderFile file) {
	wchar_t *text = GuiText_getString (_textWidget);
	if (! MelderFile_writeText (file, text)) { Melder_free (text); return 0; }
	Melder_free (text);
	_dirty = FALSE;
	MelderFile_copy (file, & _file);
	return 1;
}

void InfoEditor::closeDocument () {}

const wchar_t * InfoEditor::getName () {
	return L"info.txt";
}

void InfoEditor::menu_new (EditorCommand *cmd) {
	newDocument ();
}

int InfoEditor::menu_cb_clear (EDITOR_ARGS) {
	((InfoEditor *)editor_me)->clear ();
	return 1;
}

int InfoEditor::cb_saveAs_ok (UiForm *sendingForm, const wchar_t *sendingString, Interpreter *interpreter, const wchar_t *invokingButtonTitle, bool modified, I) {
	InfoEditor *editor = (InfoEditor *)void_me;
	(void) sendingString;
	(void) interpreter;
	(void) invokingButtonTitle;
	(void) modified;
	MelderFile file = ((UiFile *)sendingForm)->getFile ();
	if (! editor->saveDocument (file)) return 0;
	return 1;
}

int InfoEditor::menu_cb_saveAs (EDITOR_ARGS) {
	InfoEditor *editor = (InfoEditor *)editor_me;
	wchar_t defaultName [300];
	if (! editor->_saveDialog)
		editor->_saveDialog = new UiOutfile (editor->_dialog, L"Save", cb_saveAs_ok, editor, NULL, NULL);
	swprintf (defaultName, 300, editor->getName ());
	editor->_saveDialog -> do_ (defaultName);
	return 1;
}

void InfoEditor::goAway () {
	closeDocument ();
}

int InfoEditor::menu_cb_undo (EDITOR_ARGS) {
	InfoEditor *editor = (InfoEditor *)editor_me;
	GuiText_undo (editor->_textWidget);
	return 1;
}

int InfoEditor::menu_cb_redo (EDITOR_ARGS) {
	InfoEditor *editor = (InfoEditor *)editor_me;
	GuiText_redo (editor->_textWidget);
	return 1;
}

int InfoEditor::menu_cb_cut (EDITOR_ARGS) {
	InfoEditor *editor = (InfoEditor *)editor_me;
	GuiText_cut (editor->_textWidget);  // use ((XmAnyCallbackStruct *) call) -> event -> xbutton. time
	return 1;
}

int InfoEditor::menu_cb_copy (EDITOR_ARGS) {
	InfoEditor *editor = (InfoEditor *)editor_me;
	GuiText_copy (editor->_textWidget);
	return 1;
}

int InfoEditor::menu_cb_paste (EDITOR_ARGS) {
	InfoEditor *editor = (InfoEditor *)editor_me;
	GuiText_paste (editor->_textWidget);
	return 1;
}

int InfoEditor::menu_cb_erase (EDITOR_ARGS) {
	InfoEditor *editor = (InfoEditor *)editor_me;
	GuiText_remove (editor->_textWidget);
	return 1;
}

bool InfoEditor::getSelectedLines (long *firstLine, long *lastLine) {
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
void InfoEditor::do_find () {
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

void InfoEditor::do_replace () {
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

int InfoEditor::menu_cb_find (EDITOR_ARGS) {
	InfoEditor *editor = (InfoEditor *)editor_me;
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

int InfoEditor::menu_cb_findAgain (EDITOR_ARGS) {
	InfoEditor *editor = (InfoEditor *)editor_me;
	editor->do_find ();
	return 1;
}

int InfoEditor::menu_cb_replace (EDITOR_ARGS) {
	InfoEditor *editor = (InfoEditor *)editor_me;
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

int InfoEditor::menu_cb_replaceAgain (EDITOR_ARGS) {
	InfoEditor *editor = (InfoEditor *)editor_me;
	editor->do_replace ();
	return 1;
}

int InfoEditor::menu_cb_whereAmI (EDITOR_ARGS) {
	InfoEditor *editor = (InfoEditor *)editor_me;
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

int InfoEditor::menu_cb_goToLine (EDITOR_ARGS) {
	InfoEditor *editor = (InfoEditor *)editor_me;
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

int InfoEditor::menu_cb_convertToCString (EDITOR_ARGS) {
	InfoEditor *editor = (InfoEditor *)editor_me;
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

void InfoEditor::updateSizeMenu () {
	if (_fontSizeButton_10) GuiMenuItem_check (_fontSizeButton_10, _fontSize == 10);
	if (_fontSizeButton_12) GuiMenuItem_check (_fontSizeButton_12, _fontSize == 12);
	if (_fontSizeButton_14) GuiMenuItem_check (_fontSizeButton_14, _fontSize == 14);
	if (_fontSizeButton_18) GuiMenuItem_check (_fontSizeButton_18, _fontSize == 18);
	if (_fontSizeButton_24) GuiMenuItem_check (_fontSizeButton_24, _fontSize == 24);
}
void InfoEditor::setFontSize (int fontSize) {
	GuiText_setFontSize (_textWidget, fontSize);
	theTextEditorFontSize = _fontSize = fontSize;
	updateSizeMenu ();
}

int InfoEditor::menu_cb_10 (EDITOR_ARGS) { ((InfoEditor *)editor_me)->setFontSize (10); return 1; }
int InfoEditor::menu_cb_12 (EDITOR_ARGS) { ((InfoEditor *)editor_me)->setFontSize (12); return 1; }
int InfoEditor::menu_cb_14 (EDITOR_ARGS) { ((InfoEditor *)editor_me)->setFontSize (14); return 1; }
int InfoEditor::menu_cb_18 (EDITOR_ARGS) { ((InfoEditor *)editor_me)->setFontSize (18); return 1; }
int InfoEditor::menu_cb_24 (EDITOR_ARGS) { ((InfoEditor *)editor_me)->setFontSize (24); return 1; }
int InfoEditor::menu_cb_fontSize (EDITOR_ARGS) {
	InfoEditor *editor = (InfoEditor *)editor_me;
	EDITOR_FORM (L"Text window: Font size", 0)
		NATURAL (L"Font size (points)", L"12")
	EDITOR_OK
		SET_INTEGER (L"Font size", (long) editor->_fontSize);
	EDITOR_DO
		editor->setFontSize (GET_INTEGER (L"Font size"));
	EDITOR_END
}

void InfoEditor::createMenus () {
	EditorMenu *menu = getMenu (L"File");
	menu->addCommand (L"Clear", 0, menu_cb_clear); // FIXME ordering
	menu->addCommand (L"Save as...", 0, menu_cb_saveAs);

	menu = getMenu (L"Edit");
	menu->addCommand (L"Undo", 'Z', menu_cb_undo);
	menu->addCommand (L"Redo", 'Y', menu_cb_redo);
	menu->addCommand (L"-- cut copy paste --", 0, NULL);
	menu->addCommand (L"Cut", 'X', menu_cb_cut);
	menu->addCommand (L"Copy", 'C', menu_cb_copy);
	menu->addCommand (L"Paste", 'V', menu_cb_paste);
	menu->addCommand (L"Erase", 0, menu_cb_erase);
	menu = addMenu (L"Search", 0);
	menu->addCommand (L"Find...", 'F', menu_cb_find);
	menu->addCommand (L"Find again", 'G', menu_cb_findAgain);
	menu->addCommand (L"Replace...", GuiMenu_SHIFT + 'F', menu_cb_replace);
	menu->addCommand (L"Replace again", GuiMenu_SHIFT + 'G', menu_cb_replaceAgain);
	menu->addCommand (L"-- line --", 0, NULL);
	menu->addCommand (L"Where am I?", 0, menu_cb_whereAmI);
	menu->addCommand (L"Go to line...", 'L', menu_cb_goToLine);
	menu = addMenu (L"Convert", 0);
	menu->addCommand (L"Convert to C string", 0, menu_cb_convertToCString);
	#ifdef macintosh
		menu = addMenu (L"Font", 0);
		menu->addCommand (L"Font size...", 0, menu_cb_fontSize);
		_fontSizeButton_10 = menu->addCommand (L"10", GuiMenu_CHECKBUTTON, menu_cb_10);
		_fontSizeButton_12 = menu->addCommand (L"12", GuiMenu_CHECKBUTTON, menu_cb_12);
		_fontSizeButton_14 = menu->addCommand (L"14", GuiMenu_CHECKBUTTON, menu_cb_14);
		_fontSizeButton_18 = menu->addCommand (L"18", GuiMenu_CHECKBUTTON, menu_cb_18);
		_fontSizeButton_24 = menu->addCommand (L"24", GuiMenu_CHECKBUTTON, menu_cb_24);
	#endif
}

static void gui_text_cb_change (void *void_me, GuiTextEvent event) {
	(void) event;
	InfoEditor *editor = (InfoEditor *)void_me;
	if (! editor->_dirty) {
		editor->_dirty = TRUE;
		editor->nameChanged ();
	}
}

int InfoEditor::cb_open_ok (UiForm *sendingForm, const wchar_t *sendingString, Interpreter *interpreter, const wchar_t *invokingButtonTitle, bool modified, void *void_me) {
	(void) sendingString;
	(void) interpreter;
	(void) invokingButtonTitle;
	(void) modified;
	InfoEditor *editor = (InfoEditor *)void_me;
	MelderFile file = ((UiFile *)sendingForm)->getFile ();
	if (! editor->openDocument (file)) return 0;
	return 1;
}

void InfoEditor::cb_showOpen (EditorCommand *cmd, UiForm *sendingForm, const wchar_t *sendingString, Interpreter *interpreter) {
	InfoEditor *editor = (InfoEditor *) cmd -> _editor;
	(void) sendingForm;
	(void) sendingString;
	(void) interpreter;
	if (! editor->_openDialog)
		editor->_openDialog = new UiInfile (editor->_dialog, L"Open", cb_open_ok, editor, NULL, NULL, false);
	editor->_openDialog -> do_ ();
}

void InfoEditor::createChildren () {
	_textWidget = GuiText_createShown (_dialog, 0, 0, Machine_getMenuBarHeight (), 0, GuiText_SCROLLED);
	GuiText_setChangeCallback (_textWidget, gui_text_cb_change, this);
	GuiText_setUndoItem (_textWidget, getMenuCommand (L"Edit", L"Undo") -> _itemWidget);
	GuiText_setRedoItem (_textWidget, getMenuCommand (L"Edit", L"Redo") -> _itemWidget);
}

void InfoEditor::showOpen () {
	cb_showOpen (getMenuCommand (L"File", L"Open..."), NULL, NULL, NULL);
}

// FIXME
void gui_information (wchar_t *message) {
	if (! theInfoEditor) {
		theInfoEditor = new InfoEditor ((GtkWidget*)Melder_topShell, L"");
		Thing_setName (theInfoEditor, L"Praat Info");
	}
	GuiText_setString (theInfoEditor -> _textWidget, message);
	GuiObject_show (theInfoEditor -> _dialog);
	GuiWindow_drain (theInfoEditor -> _shell);
}

/* End of file InfoEditor.c */
