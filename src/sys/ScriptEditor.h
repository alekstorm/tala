#ifndef _ScriptEditor_h_
#define _ScriptEditor_h_
/* ScriptEditor.h
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

#include "Script.h"
#include "TextEditor.h"

class Interpreter;

int ScriptEditors_dirty (void);   /* Are there any modified and unsaved scripts? Ask before quitting the program. */

class ScriptEditor : public TextEditor {
  public:
	static void addToEditors (ScriptEditor *editor);
	static ScriptEditor * createFromText (GuiObject parent, Editor *other, const wchar_t *initialText);
	static ScriptEditor * createFromScript (GuiObject parent, Editor *other, Script script);

	ScriptEditor (GuiObject parent, Editor *other, const wchar_t *initialText);
	virtual ~ScriptEditor ();

	virtual const wchar_t * type () { return L"ScriptEditor"; }
	virtual bool isScriptable() { return false; }

	virtual void nameChanged ();
	virtual void run (wchar_t **text);
	virtual void createMenus ();

	wchar_t *_environmentName;
	Interpreter *_interpreter;
	UiForm *_argsDialog;

  protected:
	virtual void goAway ();

  private:
	static int args_ok (UiForm *sendingForm, const wchar_t *sendingString_dummy, Interpreter *interpreter_dummy, const wchar_t *invokingButtonTitle, bool modified_dummy, I);
	static int menu_cb_run (EDITOR_ARGS);
	static int menu_cb_runSelection (EDITOR_ARGS);
	static int menu_cb_addToMenu (EDITOR_ARGS);
	static int menu_cb_addToFixedMenu (EDITOR_ARGS);
	static int menu_cb_addToDynamicMenu (EDITOR_ARGS);
	static int menu_cb_clearHistory (EDITOR_ARGS);
	static int menu_cb_pasteHistory (EDITOR_ARGS);
	static int menu_cb_expandIncludeFiles (EDITOR_ARGS);
	static int menu_cb_AboutScriptEditor (EDITOR_ARGS);
	static int menu_cb_ScriptingTutorial (EDITOR_ARGS);
	static int menu_cb_ScriptingExamples (EDITOR_ARGS);
	static int menu_cb_PraatScript (EDITOR_ARGS);
	static int menu_cb_FormulasTutorial (EDITOR_ARGS);
	static int menu_cb_DemoWindow (EDITOR_ARGS);
	static int menu_cb_TheHistoryMechanism (EDITOR_ARGS);
	static int menu_cb_InitializationScripts (EDITOR_ARGS);
	static int menu_cb_AddingToAFixedMenu (EDITOR_ARGS);
	static int menu_cb_AddingToADynamicMenu (EDITOR_ARGS);

	void init (GuiObject parent, Editor *editor, const wchar_t *initialText);
};

/* End of file ScriptEditor.h */
#endif
