/* ScriptEditor.c
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
 * pb 2002/10/14 added scripting examples to Help menu
 * pb 2002/12/05 include
 * pb 2004/01/07 use GuiWindow_setDirty
 * pb 2007/06/12 wchar_t
 * pb 2007/08/12 wchar_t
 * pb 2008/03/20 split off Help menu
 * pb 2008/03/21 new Editor API
 * pb 2009/01/18 arguments to UiForm *callbacks
 * pb 2009/01/20 pause forms
 * pb 2009/05/07 demo window
 * pb 2010/04/30 command "Expand include files"
 */

#include "ScriptEditor.h"

#include "ui/Interpreter.h"
#include "ui/praatP.h"
#include "ui/UiHistory.h"
#include "kar/longchar.h"
#include "kar/UnicodeData.h"

#include "EditorM.h"

static Collection theScriptEditors; // FIXME

int ScriptEditors_dirty (void) {
	if (! theScriptEditors) return FALSE;
	for (long i = 1; i <= theScriptEditors -> size; i ++) {
		ScriptEditor *editor = (ScriptEditor*)theScriptEditors -> item [i];
		if (editor->_dirty) return TRUE;
	}
	return FALSE;
}

ScriptEditor::ScriptEditor (GuiObject parent, Editor *other, const wchar_t *initialText)
	: TextEditor (parent, initialText),
	  _environmentName(other != NULL ? Melder_wcsdup_e (other -> _name) : NULL),
	  _interpreter(new Interpreter (other != NULL ? other->_name : NULL)) {
	createMenus ();
}

ScriptEditor::~ScriptEditor () {
	Melder_free (_environmentName);
	forget (_interpreter);
	forget (_argsDialog);
	if (theScriptEditors) Collection_undangleItem (theScriptEditors, this);
}

void ScriptEditor::nameChanged () {
	int dirtinessAlreadyShown = GuiWindow_setDirty (_shell, _dirty);
	static MelderString buffer = { 0 };
	MelderString_copy (& buffer, _name ? L"Script" : L"untitled script");
	if (_environmentName) {
		MelderString_append3 (& buffer, L" [", _environmentName, L"]");
	}
	if (_name) {
		MelderString_append3 (& buffer, L" " UNITEXT_LEFT_DOUBLE_QUOTATION_MARK, MelderFile_messageName (& _file), UNITEXT_RIGHT_DOUBLE_QUOTATION_MARK);
	}
	if (_dirty && ! dirtinessAlreadyShown)
		MelderString_append (& buffer, L" (modified)");
	GuiWindow_setTitle (_shell, buffer.string);
}

void ScriptEditor::goAway () {
	if (_interpreter -> _running) {
		Melder_error1 (L"Cannot close the script window while the script is running or paused. Please close or continue the pause or demo window.");
		Melder_flushError (NULL);
		return;
	}
	TextEditor::goAway ();
}

int ScriptEditor::args_ok (UiForm *sendingForm, const wchar_t *sendingString_dummy, Interpreter *interpreter_dummy, const wchar_t *invokingButtonTitle, bool modified_dummy, I) {
	(void) sendingString_dummy;
	(void) interpreter_dummy;
	(void) invokingButtonTitle;
	(void) modified_dummy;
	ScriptEditor *editor = (ScriptEditor *)void_me;
	structMelderFile file = { 0 };
	wchar_t *text = GuiText_getString (editor->_textWidget);
	if (editor->_name) {
		Melder_pathToFile (editor->_name, & file);
		MelderFile_setDefaultDir (& file);
	}
	Melder_includeIncludeFiles (& text);

	editor->_interpreter->getArgumentsFromDialog (sendingForm);

	praat_background ();
	if (editor->_name) MelderFile_setDefaultDir (& file);   /* BUG if two disks have the same name (on Mac). */
	editor->_interpreter->run (text);
	praat_foreground ();
	Melder_free (text);
	iferror return 0;
	return 1;
}

void ScriptEditor::run (wchar_t **text) {
	structMelderFile file = { 0 };
	if (_name) {
		Melder_pathToFile (_name, & file);
		MelderFile_setDefaultDir (& file);
	}
	Melder_includeIncludeFiles (text);
	iferror { Melder_flushError (NULL); return; }
	int npar = _interpreter->readParameters (*text);
	iferror { Melder_flushError (NULL); return; }
	if (npar) {
		/*
		 * Pop up a dialog box for querying the arguments.
		 */
		forget (_argsDialog);
		_argsDialog = _interpreter->createForm (_shell, NULL, args_ok, this);
		_argsDialog -> do_ (false);
	} else {
		praat_background ();
		if (_name) MelderFile_setDefaultDir (& file);   /* BUG if two disks have the same name (on Mac). */
		_interpreter->run (*text);
		praat_foreground ();
		iferror Melder_flushError (NULL);
	}
}

int ScriptEditor::menu_cb_run (EDITOR_ARGS) {
	ScriptEditor *editor = (ScriptEditor *)editor;
	if (editor->_interpreter -> _running)
		return Melder_error1 (L"The script is already running (paused). Please close or continue the pause or demo window.");
	wchar_t *text = GuiText_getString (editor->_textWidget);
	editor->run (& text);
	Melder_free (text);
	return 1;
}

int ScriptEditor::menu_cb_runSelection (EDITOR_ARGS) {
	ScriptEditor *editor = (ScriptEditor *)editor;
	if (editor->_interpreter -> _running)
		return Melder_error1 (L"The script is already running (paused). Please close or continue the pause or demo window.");
	wchar_t *text = GuiText_getSelection (editor->_textWidget);
	if (text == NULL) {
		return Melder_error1 (L"No text selected.");
	}
	editor->run (& text);
	Melder_free (text);
	return 1;
}

int ScriptEditor::menu_cb_addToMenu (EDITOR_ARGS) {
	ScriptEditor *editor = (ScriptEditor *)editor;
	EDITOR_FORM (L"Add to menu", L"Add to fixed menu...")
		WORD (L"Window", L"?")
		SENTENCE (L"Menu", L"File")
		SENTENCE (L"Command", L"Do it...")
		SENTENCE (L"After command", L"")
		INTEGER (L"Depth", L"0")
		LABEL (L"", L"Script file:")
		TEXTFIELD (L"Script", L"")
	EDITOR_OK
		if (editor->_environmentName) SET_STRING (L"Window", editor->_environmentName)
		if (editor->_name)
			SET_STRING (L"Script", editor->_name)
		else
			SET_STRING (L"Script", L"(please save your script first)")
	EDITOR_DO
		if (! praat_addMenuCommandScript (GET_STRING (L"Window"),
			GET_STRING (L"Menu"), GET_STRING (L"Command"), GET_STRING (L"After command"),
			GET_INTEGER (L"Depth"), GET_STRING (L"Script"))) return 0;
		praat_show ();
	EDITOR_END
}

int ScriptEditor::menu_cb_addToFixedMenu (EDITOR_ARGS) {
	ScriptEditor *editor = (ScriptEditor *)editor;
	EDITOR_FORM (L"Add to fixed menu", L"Add to fixed menu...");
		RADIO (L"Window", 1)
			RADIOBUTTON (L"Objects")
			RADIOBUTTON (L"Picture")
		SENTENCE (L"Menu", L"New")
		SENTENCE (L"Command", L"Do it...")
		SENTENCE (L"After command", L"")
		INTEGER (L"Depth", L"0")
		LABEL (L"", L"Script file:")
		TEXTFIELD (L"Script", L"")
	EDITOR_OK
		if (editor->_name)
			SET_STRING (L"Script", editor->_name)
		else
			SET_STRING (L"Script", L"(please save your script first)")
	EDITOR_DO
		if (! praat_addMenuCommandScript (GET_STRING (L"Window"),
			GET_STRING (L"Menu"), GET_STRING (L"Command"), GET_STRING (L"After command"),
			GET_INTEGER (L"Depth"), GET_STRING (L"Script"))) return 0;
		praat_show ();
	EDITOR_END
}

int ScriptEditor::menu_cb_addToDynamicMenu (EDITOR_ARGS) {
	ScriptEditor *editor = (ScriptEditor *)editor;
	EDITOR_FORM (L"Add to dynamic menu", L"Add to dynamic menu...")
		WORD (L"Class 1", L"Sound")
		INTEGER (L"Number 1", L"0")
		WORD (L"Class 2", L"")
		INTEGER (L"Number 2", L"0")
		WORD (L"Class 3", L"")
		INTEGER (L"Number 3", L"0")
		SENTENCE (L"Command", L"Do it...")
		SENTENCE (L"After command", L"")
		INTEGER (L"Depth", L"0")
		LABEL (L"", L"Script file:")
		TEXTFIELD (L"Script", L"")
	EDITOR_OK
		if (editor->_name)
			SET_STRING (L"Script", editor->_name)
		else
			SET_STRING (L"Script", L"(please save your script first)")
	EDITOR_DO
		if (! praat_addActionScript (GET_STRING (L"Class 1"), GET_INTEGER (L"Number 1"),
			GET_STRING (L"Class 2"), GET_INTEGER (L"Number 2"), GET_STRING (L"Class 3"),
			GET_INTEGER (L"Number 3"), GET_STRING (L"Command"), GET_STRING (L"After command"),
			GET_INTEGER (L"Depth"), GET_STRING (L"Script"))) return 0;
		praat_show ();
	EDITOR_END
}

int ScriptEditor::menu_cb_clearHistory (EDITOR_ARGS) {
	ScriptEditor *editor = (ScriptEditor *)editor;
	UiForm::history.clear ();
	return 1;
}

int ScriptEditor::menu_cb_pasteHistory (EDITOR_ARGS) {
	ScriptEditor *editor = (ScriptEditor *)editor;
	wchar_t *history = UiForm::history.get ();
	if (history == NULL || history [0] == '\0')
		return Melder_error1 (L"No history.");
	long length = wcslen (history);
	if (history [length - 1] != '\n') {
		UiForm::history.write (L"\n");
		history = UiForm::history.get ();
		length = wcslen (history);
	}
	if (history [0] == '\n') {
		history ++;
		length --;
	}
	long first = 0, last = 0;
	wchar_t *text = GuiText_getStringAndSelectionPosition (editor->_textWidget, & first, & last);
	Melder_free (text);
	GuiText_replace (editor->_textWidget, first, last, history);
	GuiText_setSelection (editor->_textWidget, first, first + length);
	GuiText_scrollToSelection (editor->_textWidget);
	return 1;
}

int ScriptEditor::menu_cb_expandIncludeFiles (EDITOR_ARGS) {
	ScriptEditor *editor = (ScriptEditor *)editor;
	structMelderFile file = { 0 };
	wchar_t *text = GuiText_getString (editor->_textWidget);
	if (editor->_name) {
		Melder_pathToFile (editor->_name, & file);
		MelderFile_setDefaultDir (& file);
	}
	Melder_includeIncludeFiles (& text); cherror
	GuiText_setString (editor->_textWidget, text);
end:
	Melder_free (text);
	iferror return 0;
	return 1;
}

int ScriptEditor::menu_cb_AboutScriptEditor (EDITOR_ARGS) { Melder_help (L"ScriptEditor"); return 1; }
int ScriptEditor::menu_cb_ScriptingTutorial (EDITOR_ARGS) { Melder_help (L"Scripting"); return 1; }
int ScriptEditor::menu_cb_ScriptingExamples (EDITOR_ARGS) { Melder_help (L"Scripting examples"); return 1; }
int ScriptEditor::menu_cb_PraatScript (EDITOR_ARGS) { Melder_help (L"Praat script"); return 1; }
int ScriptEditor::menu_cb_FormulasTutorial (EDITOR_ARGS) { Melder_help (L"Formulas"); return 1; }
int ScriptEditor::menu_cb_DemoWindow (EDITOR_ARGS) { Melder_help (L"Demo window"); return 1; }
int ScriptEditor::menu_cb_TheHistoryMechanism (EDITOR_ARGS) { Melder_help (L"History mechanism"); return 1; }
int ScriptEditor::menu_cb_InitializationScripts (EDITOR_ARGS) { Melder_help (L"Initialization script"); return 1; }
int ScriptEditor::menu_cb_AddingToAFixedMenu (EDITOR_ARGS) { Melder_help (L"Add to fixed menu..."); return 1; }
int ScriptEditor::menu_cb_AddingToADynamicMenu (EDITOR_ARGS) { Melder_help (L"Add to dynamic menu..."); return 1; }

void ScriptEditor::createMenus () {
	EditorMenu *menu = getMenu (L"File");
	if (_environmentName) {
		menu->addCommand (L"Add to menu...", 0, menu_cb_addToMenu);
	} else {
		menu->addCommand (L"Add to fixed menu...", 0, menu_cb_addToFixedMenu);
		menu->addCommand (L"Add to dynamic menu...", 0, menu_cb_addToDynamicMenu);
	}
	menu->addCommand (L"-- close --", 0, NULL);
	menu = getMenu (L"Edit");
	menu->addCommand (L"-- history --", 0, 0);
	menu->addCommand (L"Clear history", 0, menu_cb_clearHistory);
	menu->addCommand (L"Paste history", 'H', menu_cb_pasteHistory);
	menu->addCommand (L"-- expand --", 0, 0);
	menu->addCommand (L"Expand include files", 0, menu_cb_expandIncludeFiles);
	menu = addMenu (L"Run", 0);
	menu->addCommand (L"Run", 'R', menu_cb_run);
	menu->addCommand (L"Run selection", 'T', menu_cb_runSelection);

	menu = getMenu (L"Help");
	menu->addCommand (L"About ScriptEditor", '?', menu_cb_AboutScriptEditor);
	menu->addCommand (L"Scripting tutorial", 0, menu_cb_ScriptingTutorial);
	menu->addCommand (L"Scripting examples", 0, menu_cb_ScriptingExamples);
	menu->addCommand (L"Praat script", 0, menu_cb_PraatScript);
	menu->addCommand (L"Formulas tutorial", 0, menu_cb_FormulasTutorial);
	menu->addCommand (L"Demo window", 0, menu_cb_DemoWindow);
	menu->addCommand (L"-- help history --", 0, NULL);
	menu->addCommand (L"The History mechanism", 0, menu_cb_TheHistoryMechanism);
	menu->addCommand (L"Initialization scripts", 0, menu_cb_InitializationScripts);
	menu->addCommand (L"-- help add --", 0, NULL);
	menu->addCommand (L"Adding to a fixed menu", 0, menu_cb_AddingToAFixedMenu);
	menu->addCommand (L"Adding to a dynamic menu", 0, menu_cb_AddingToADynamicMenu);
}

void ScriptEditor::addToEditors (ScriptEditor *editor) {
	if (theScriptEditors == NULL) {
		theScriptEditors = Collection_create (NULL, 10);
	}
	Collection_addItem (theScriptEditors, editor);
}

ScriptEditor * ScriptEditor::createFromText (GuiObject parent, Editor *other, const wchar_t *initialText) {
	ScriptEditor *editor = new ScriptEditor (parent, other, initialText);
	addToEditors (editor);
	return editor;
}

ScriptEditor * ScriptEditor::createFromScript (GuiObject parent, Editor *other, Script script) {
	if (theScriptEditors) {
		for (long ieditor = 1; ieditor <= theScriptEditors -> size; ieditor ++) {
			ScriptEditor *editor = (ScriptEditor*)theScriptEditors -> item [ieditor];
			if (MelderFile_equal (& script -> file, & editor -> _file)) {
				editor->raise ();
				Melder_error3 (L"Script ", MelderFile_messageName (& script -> file), L" is already open.");
				return NULL;
			}
		}
	}
	wchar_t *text = MelderFile_readText (& script -> file);
	ScriptEditor *editor = new ScriptEditor (parent, other, text);
	addToEditors (editor);
	MelderFile_copy (& script -> file, & editor->_file);
	editor->_name = Melder_wcsdup_f (Melder_fileToPath (& script -> file));
	Melder_free (text);
}

/* End of file ScriptEditor.c */
