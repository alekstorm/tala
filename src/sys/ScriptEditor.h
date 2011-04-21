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

#ifndef _Script_h_
	#include "Script.h"
#endif
#ifndef _TextEditor_h_
	#include "TextEditor.h"
#endif
#ifndef _Interpreter_h_
	#include "Interpreter.h"
#endif

int ScriptEditors_dirty (void);   /* Are there any modified and unsaved scripts? Ask before quitting the program. */

class ScriptEditor : public TextEditor {
  public:
	static void addToEditors (ScriptEditor *editor);
	static ScriptEditor * createFromText (GuiObject parent, Editor *other, const wchar_t *initialText);
	static ScriptEditor * createFromScript (GuiObject parent, Editor *other, Script script);

	ScriptEditor (GuiObject parent, Editor *other, const wchar_t *initialText);
	~ScriptEditor ();

	const wchar_t * type () { return L"ScriptEditor"; }
	bool isScriptable() { return false; }

	void nameChanged ();
	void run (wchar_t **text);
	void createMenus ();
	void createHelpMenuItems (EditorMenu *menu);

	wchar_t *_environmentName;
	Interpreter *_interpreter;
	UiForm *_argsDialog;

  protected:
	void goAway ();

  private:
	void init (GuiObject parent, Editor *editor, const wchar_t *initialText);
};

/* End of file ScriptEditor.h */
#endif
