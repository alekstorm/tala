/* UiFile.c
 *
 * Copyright (C) 1992-2010 Paul Boersma
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
 * pb 2006/08/10 Windows: turned file selector into a modal dialog box
 * pb 2006/10/28 erased MacOS 9 stuff
 * pb 2007/02/12 worked around a bug in Windows XP that caused Praat to crash
                 when the user moved the mouse pointer over a file in the Desktop of the second file selector
                 that was raised in Praat. The workaround is to temporarily disable file info tips.
 * pb 2007/03/23 new Editor API
 * pb 2007/05/30 wchar_t
 * pb 2009/01/18 arguments to UiForm *callbacks
 * pb 2009/12/22 invokingButtonTitle
 * pb 2010/07/21 erased Motif stuff
 * bp 2010/07/26 split off GuiFileSelect.c
 */

#include "UiP.h"
#include "Editor.h"
#include "UiFile.h"

UiFile::UiFile (GuiObject parent, const wchar_t *title,
	int (*okCallback) (UiForm *sendingForm, const wchar_t *sendingString, Interpreter *interpreter, const wchar_t *invokingButtonTitle, bool modified, void *closure), void *okClosure,
	const wchar_t *invokingButtonTitle, const wchar_t *helpTitle)
	: UiForm (parent, title, okCallback, okClosure, invokingButtonTitle, helpTitle),
	  _okClosure(NULL),
	  _shiftKeyPressed(0) {
	_parent = parent;
	_name = Melder_wcsdup_f (title);
}

MelderFile UiFile::getFile () {
	return & _file;
}

/********** READING A FILE **********/

UiInfile::UiInfile (GuiObject parent, const wchar_t *title,
	int (*okCallback) (UiForm *, const wchar_t *, Interpreter *, const wchar_t *, bool, void *), void *okClosure,
	const wchar_t *invokingButtonTitle, const wchar_t *helpTitle, bool allowMultipleFiles)
	: UiFile (parent, title, okCallback, okClosure, invokingButtonTitle, helpTitle),
	  _allowMultipleFiles(allowMultipleFiles) {}

void UiInfile::do_ () {
	SortedSetOfString infileNames = GuiFileSelect_getInfileNames (_parent, _name, _allowMultipleFiles);
	if (infileNames == NULL) {
		Melder_flushError (NULL);
		return;
	}
	for (long ifile = 1; ifile <= infileNames -> size; ifile ++) {
		SimpleString infileName = (structSimpleString*)infileNames -> item [ifile];
		Melder_pathToFile (infileName -> string, & _file);
		UiForm::history.write (L"\n");
		UiForm::history.write (_invokingButtonTitle);
		UiForm::history.write (L" ");
		UiForm::history.write (infileName -> string);
		structMelderFile file;
		MelderFile_copy (& _file, & file);
		if (! _okCallback (this, NULL, NULL, _invokingButtonTitle, false, _okClosure)) {
			Melder_error3 (L"File ", MelderFile_messageName (& file), L" not finished.");
			Melder_flushError (NULL);
		}
	}
	forget (infileNames);
}

/********** WRITING A FILE **********/

UiOutfile::UiOutfile (GuiObject parent, const wchar_t *title,
	int (*okCallback) (UiForm *, const wchar_t *, Interpreter *, const wchar_t *, bool, void *), void *okClosure,
	const wchar_t *invokingButtonTitle, const wchar_t *helpTitle)
	: UiFile (parent, title, okCallback, okClosure, invokingButtonTitle, helpTitle),
	  _allowExecutionHook(UiForm::theAllowExecutionHookHint),
	  _allowExecutionClosure(UiForm::theAllowExecutionClosureHint) {}

static int commonOutfileCallback (UiForm *sendingForm, const wchar_t *sendingString, Interpreter *interpreter, const wchar_t *invokingButtonTitle, bool modified, void *closure) {
	EditorCommand *command = (EditorCommand *) closure;
	(void) invokingButtonTitle;
	(void) modified;
	return command -> _commandCallback (command -> _editor, command, sendingForm, sendingString, interpreter);
}

void UiOutfile::do_ (const wchar_t *defaultName) {
	wchar_t *outfileName = GuiFileSelect_getOutfileName (NULL, _name, defaultName);
	if (outfileName == NULL) return;   // cancelled
	if (_allowExecutionHook && ! _allowExecutionHook (_allowExecutionClosure)) {
		Melder_flushError ("Dialog `%s' cancelled.", _name);
		return;
	}
	Melder_pathToFile (outfileName, & _file);
	structMelderFile file;
	MelderFile_copy (& _file, & file);   // save, because okCallback could destroy me
	UiForm::history.write (L"\n");
	UiForm::history.write (_invokingButtonTitle);
	if (! _okCallback (this, NULL, NULL, _invokingButtonTitle, false, _okClosure)) {
		Melder_error3 (L"File ", MelderFile_messageName (& file), L" not finished.");
		Melder_flushError (NULL);
	}
	UiForm::history.write (L" ");
	UiForm::history.write (outfileName);
	Melder_free (outfileName);
}

UiOutfile *UiOutfile::createE (EditorCommand *cmd, const wchar_t *title, const wchar_t *invokingButtonTitle, const wchar_t *helpTitle) {
	Editor *editor = cmd -> _editor;
	UiOutfile *dia = new UiOutfile (editor -> _dialog, title, commonOutfileCallback, cmd, invokingButtonTitle, helpTitle);
	dia -> _command = cmd;
	return dia;   // BUG
}

/* End of file UiFile.c */
