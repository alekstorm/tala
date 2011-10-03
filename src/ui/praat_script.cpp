/* praat_script.cpp
 *
 * Copyright (C) 1993-2011 Paul Boersma
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
 * pb 2002/10/02 system -> Melder_system
 * pb 2003/03/09 set UiInterpreter *back to NULL
 * pb 2004/02/22 allow numeric expressions after select/plus/minus
 * pb 2004/10/27 warning off
 * pb 2004/12/04 support for multiple open script dialogs with Apply buttons, both from "Run script..." and from added buttons
 * pb 2005/02/10 corrected bug in nowarn
 * pb 2005/08/22 renamed Control menu to "Praat"
 * pb 2006/01/11 local variables
 * pb 2006/12/28 theCurrentPraat
 * pb 2007/02/17 corrected the messages about trailing spaces
 * pb 2007/06/11 wchar_t
 * pb 2007/10/04 removed swscanf
 * pb 2009/01/04 allow proc(args) syntax
 * pb 2009/01/17 arguments to UiForm *callbacks
 * pb 2009/01/20 pause uses a pause form
 * pb 2011/03/20 C++
 * pb 2011/03/24 command no longer const
 */

#include "editors/DemoEditor.h"
#include "praatP.h"
#include "praat_script.h"
#include "sendpraat/sendpraat.h"
#include "sendpraat/sendsocket.h"
#include "UiFile.h"
#include "UiPause.h"

#include <ctype.h>

static int praat_findObjectFromString (Interpreter *interpreter, const wchar_t *string) {
	int IOBJECT;
	while (*string == ' ') string ++;
	if (*string >= 'A' && *string <= 'Z') {
		static MelderString buffer = { 0 };
		MelderString_copy (& buffer, string);
		wchar_t *space = wcschr (buffer.string, ' ');
		if (space == NULL) goto end;
		*space = '\0';
		wchar_t *className = & buffer.string [0], *givenName = space + 1;
		WHERE_DOWN (1) {
			Data object = (Data) OBJECT;
			if (wcsequ (className, Thing_className (OBJECT)) && wcsequ (givenName, object -> name))
				return IOBJECT;
		}
	} else {
		double value;
		if (! interpreter->numericExpression (string, & value)) goto end;
		long id = (long) value;
		WHERE (ID == id) return IOBJECT;
		goto end;
	}
end:
	return Melder_error3 (L"Object \"", string, L"\" does not exist.");
}

Editor *praat_findEditorFromString (const wchar_t *string) {
	int IOBJECT;
	while (*string == ' ') string ++;
	if (*string >= 'A' && *string <= 'Z') {
		WHERE_DOWN (1) {
			for (int ieditor = 0; ieditor < praat_MAXNUM_EDITORS; ieditor ++) {
				Editor *editor = (Editor *) theCurrentPraatObjects -> list [IOBJECT]. editors [ieditor];
				if (editor != NULL) {
					const wchar_t *name = wcschr (editor -> _name, ' ') + 1;
					if (wcsequ (name, string)) return editor;
				}
			}
		}
	} else {
		WHERE_DOWN (1) {
			for (int ieditor = 0; ieditor < praat_MAXNUM_EDITORS; ieditor ++) {
				Editor *editor = (Editor *) theCurrentPraatObjects -> list [IOBJECT]. editors [ieditor];
				if (editor && wcsequ (editor -> _name, string)) return editor;
			}
		}
	}
	return NULL;
}

int praat_executeCommand (Interpreter *interpreter, wchar_t *command) {
	//Melder_casual ("praat_executeCommand: %ld: %ls", interpreter, command);
	if (command [0] == '\0' || command [0] == '#' || command [0] == '!' || command [0] == ';')
		/* Skip empty lines and comments. */;
	else if ((command [0] == '.' || command [0] == '+' || command [0] == '-') && isupper (command [1])) {   /* Selection? */
		int IOBJECT = praat_findObjectFromString (interpreter, command + 1);
		if (! IOBJECT) return 0;
		if (command [0] == '.') praat_deselectAll ();
		if (command [0] == '-') praat_deselect (IOBJECT); else praat_select (IOBJECT); 
		praat_show ();
	} else if (islower (command [0])) {   /* All directives start with a lower-case letter. */
		if (wcsnequ (command, L"select ", 7)) {
			if (wcsnequ (command + 7, L"all", 3) && (command [10] == '\0' || command [10] == ' ' || command [10] == '\t')) {
				praat_selectAll ();
				praat_show ();
			} else {
				int IOBJECT = praat_findObjectFromString (interpreter, command + 7);
				if (! IOBJECT) return 0;
				praat_deselectAll ();
				praat_select (IOBJECT);
				praat_show ();
			}
		} else if (wcsnequ (command, L"plus ", 5)) {
			int IOBJECT = praat_findObjectFromString (interpreter, command + 5);
			if (! IOBJECT) return 0;
			praat_select (IOBJECT);
			praat_show ();
		} else if (wcsnequ (command, L"minus ", 6)) {
			int IOBJECT = praat_findObjectFromString (interpreter, command + 6);
			if (! IOBJECT) return 0;
			praat_deselect (IOBJECT);
			praat_show ();
		} else if (wcsnequ (command, L"echo ", 5)) {
			MelderInfo_open ();
			MelderInfo_write1 (command + 5);
			MelderInfo_close ();
		} else if (wcsnequ (command, L"clearinfo", 9)) {
			Melder_clearInfo ();
		} else if (wcsnequ (command, L"print ", 6)) {
			Melder_print (command + 6);
		} else if (wcsnequ (command, L"printtab", 8)) {
			Melder_print (L"\t");
		} else if (wcsnequ (command, L"printline", 9)) {
			if (command [9] == ' ') Melder_print (command + 10);
			Melder_print (L"\n");
		} else if (wcsnequ (command, L"fappendinfo ", 12)) {
			if (theCurrentPraatObjects != & theForegroundPraatObjects)
				return Melder_error1 (L"The script command \"fappendinfo\" is not available inside pictures.");
			structMelderFile file = { 0 };
			if (! Melder_relativePathToFile (command + 12, & file)) return 0;
			if (! MelderFile_appendText (& file, Melder_getInfo ())) return 0;
		} else if (wcsnequ (command, L"unix ", 5)) {
			if (theCurrentPraatObjects != & theForegroundPraatObjects)
				return Melder_error1 (L"The script command \"unix\" is not available inside manuals.");
			if (! Melder_system (command + 5))
				return Melder_error3 (L"Unix command \"", command + 5, L"\" returned error status;\n"
					"if you want to ignore this, use `unix_nocheck' instead of `unix'.");
		} else if (wcsnequ (command, L"unix_nocheck ", 13)) {
			if (theCurrentPraatObjects != & theForegroundPraatObjects)
				return Melder_error1 (L"The script command \"unix_nocheck\" is not available inside manuals.");
			(void) Melder_system (command + 13); Melder_clearError ();
		} else if (wcsnequ (command, L"system ", 7)) {
			if (theCurrentPraatObjects != & theForegroundPraatObjects)
				return Melder_error1 (L"The script command \"system\" is not available inside manuals.");
			if (! Melder_system (command + 7))
				return Melder_error3 (L"System command \"", command + 7, L"\" returned error status;\n"
					"if you want to ignore this, use `system_nocheck' instead of `system'.");
		} else if (wcsnequ (command, L"system_nocheck ", 15)) {
			if (theCurrentPraatObjects != & theForegroundPraatObjects)
				return Melder_error1 (L"The script command \"system_nocheck\" is not available inside manuals.");
			(void) Melder_system (command + 15); Melder_clearError ();
		} else if (wcsnequ (command, L"nowarn ", 7)) {
			int result;
			Melder_warningOff ();
			result = praat_executeCommand (interpreter, command + 7);
			Melder_warningOn ();
			return result;
		} else if (wcsnequ (command, L"noprogress ", 11)) {
			int result;
			Melder_progressOff ();
			result = praat_executeCommand (interpreter, command + 11);
			Melder_progressOn ();
			return result;
		} else if (wcsnequ (command, L"nocheck ", 8)) {
			(void) praat_executeCommand (interpreter, command + 8);
			Melder_clearError ();
		} else if (wcsnequ (command, L"demo ", 5)) {
			/*if (! DemoEditor::open ()) return 0; // FIXME
			(void) praat_executeCommand (interpreter, command + 5);
			DemoEditor::close ();*/
		} else if (wcsnequ (command, L"pause ", 6) || wcsequ (command, L"pause")) {
			if (theCurrentPraatApplication -> batch) return 1;
			UiPause::begin (theCurrentPraatApplication -> topShell, L"stop or continue", interpreter); iferror return 0;
			UiPause::comment (wcsequ (command, L"pause") ? L"..." : command + 6); iferror return 0;
			UiPause::end (1, 1, 0, L"Continue", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, interpreter); iferror return 0;
		} else if (wcsnequ (command, L"execute ", 8)) {
			praat_executeScriptFromFileNameWithArguments (command + 8);
		} else if (wcsnequ (command, L"editor", 6)) {
			if (theCurrentPraatObjects != & theForegroundPraatObjects)
				return Melder_error1 (L"The script command \"editor\" is not available inside manuals.");
			if (command [6] == ' ' && isalpha (command [7])) {
				praatP. editor = praat_findEditorFromString (command + 7);
				if (praatP. editor == NULL)
					return Melder_error3 (L"Editor *\"", command + 7, L"\" does not exist.");
			} else if (interpreter && interpreter -> _environmentName) {
				praatP. editor = praat_findEditorFromString (interpreter -> _environmentName);
				if (praatP. editor == NULL)
					return Melder_error3 (L"Editor *\"", interpreter -> _environmentName, L"\" does not exist.");
			} else {
				return Melder_error1 (L"No editor specified.");
			}
		} else if (wcsnequ (command, L"endeditor", 9)) {
			if (theCurrentPraatObjects != & theForegroundPraatObjects)
				return Melder_error1 (L"The script command \"endeditor\" is not available inside manuals.");
			praatP. editor = NULL;
		} else if (wcsnequ (command, L"sendpraat ", 10)) {
			if (theCurrentPraatObjects != & theForegroundPraatObjects)
				return Melder_error1 (L"The script command \"sendpraat\" is not available inside manuals.");
			wchar_t programName [41], *q = & programName [0];
			#ifdef macintosh
				#define SENDPRAAT_TIMEOUT  10
			#else
				#define SENDPRAAT_TIMEOUT  0
			#endif
			const wchar_t *p = command + 10;
			while (*p == ' ' || *p == '\t') p ++;
			while (*p != '\0' && *p != ' ' && *p != '\t' && q < programName + 39) *q ++ = *p ++;
			*q = '\0';
			if (q == programName)
				return Melder_error1 (L"Missing program name after `sendpraat'.");
			while (*p == ' ' || *p == '\t') p ++;
			if (*p == '\0')
				return Melder_error1 (L"Missing command after `sendpraat'.");
		} else if (wcsnequ (command, L"sendsocket ", 11)) {
			/*if (theCurrentPraatObjects != & theForegroundPraatObjects) // FIXME
				return Melder_error1 (L"The script command \"sendsocket\" is not available inside manuals.");
			wchar_t hostName [61], *q = & hostName [0];
			const wchar_t *p = command + 11;
			while (*p == ' ' || *p == '\t') p ++;
			while (*p != '\0' && *p != ' ' && *p != '\t' && q < hostName + 59) *q ++ = *p ++;
			*q = '\0';
			if (q == hostName)
				return Melder_error1 (L"Missing host name after `sendsocket'.");
			while (*p == ' ' || *p == '\t') p ++;
			if (*p == '\0')
				return Melder_error1 (L"Missing command after `sendsocket'.");
			char *result = sendsocket (Melder_peekWcsToUtf8 (hostName), Melder_peekWcsToUtf8 (p));
			if (result)
				return Melder_error4 (Melder_peekUtf8ToWcs (result), L"\nMessage to ", hostName, L" not completed.");*/
		} else if (wcsnequ (command, L"filedelete ", 11)) {
			if (theCurrentPraatObjects != & theForegroundPraatObjects)
				return Melder_error1 (L"The script command \"filedelete\" is not available inside manuals.");
			const wchar_t *p = command + 11;
			structMelderFile file = { 0 };
			while (*p == ' ' || *p == '\t') p ++;
			if (*p == '\0')
				return Melder_error1 (L"Missing file name after `filedelete'.");
			if (! Melder_relativePathToFile (p, & file)) return 0;
			MelderFile_delete (& file);
		} else if (wcsnequ (command, L"fileappend ", 11)) {
			if (theCurrentPraatObjects != & theForegroundPraatObjects)
				return Melder_error1 (L"The script command \"fileappend\" is not available inside manuals.");
			const wchar_t *p = command + 11;
			wchar_t path [256], *q = & path [0];
			while (*p == ' ' || *p == '\t') p ++;
			if (*p == '\0')
				return Melder_error1 (L"Missing file name after `fileappend'.");
			if (*p == '\"') {
				for (;;) {
					int kar = * ++ p;
					if (kar == '\"') if (* ++ p == '\"') *q ++ = '\"'; else break;
					else if (kar == '\0') break;
					else *q ++ = kar;
				}
			} else {
				for (;;) {
					int kar = * p;
					if (kar == '\0' || kar == ' ' || kar == '\t') break;
					*q ++ = kar;
					p ++;
				}
			}
			*q = '\0';
			if (*p == ' ' || *p == '\t') {
				structMelderFile file = { 0 };
				if (! Melder_relativePathToFile (path, & file)) return 0;
				if (! MelderFile_appendText (& file, p + 1)) return 0;
			}
		} else {
			/*
			 * This must be a formula command:
			 *    proc (args)
			 */
			int status = interpreter->voidExpression (command);
			return status;
		}
	} else {   /* Simulate menu choice. */
		wchar_t *arguments;

 		/* Parse command line into command and arguments. */
		/* The separation is formed by the three dots. */

		if ((arguments = wcsstr (command, L"...")) == NULL || wcslen (arguments) < 4) {
			static wchar dummy = { 0 };
			arguments = & dummy;
		} else {
			arguments += 4;
			if (arguments [-1] != ' ' && arguments [-1] != '0') {
				return Melder_error1 (L"There should be a space after the three dots.");
			}
			arguments [-1] = '\0'; /* New end of "command". */
		}

		/* See if command exists and is available; ignore separators. */
		/* First try loose commands, then fixed commands. */

		if (theCurrentPraatObjects == & theForegroundPraatObjects && praatP. editor != NULL) {
			if (! ((Editor *) praatP. editor)->doMenuCommand (command, arguments, interpreter)) {
				return 0;
			}
		} else if (theCurrentPraatObjects != & theForegroundPraatObjects &&
		    (wcsnequ (command, L"Save ", 5) ||
			 wcsnequ (command, L"Write ", 6) ||
			 wcsnequ (command, L"Append ", 7) ||
			 wcsequ (command, L"Quit")))
		{
			return Melder_error1 (L"Commands that write files (including Quit) are not available inside manuals.");
		} else if (! praat_doAction (command, arguments, interpreter)) {
			if (Melder_hasError ()) {
				if (arguments [0] != '\0' && arguments [wcslen (arguments) - 1] == ' ') {
					return Melder_error3 (L"It may be helpful to remove the trailing spaces in \"", arguments, L"\".");
				}
				return 0;
			}
			if (! praat_doMenuCommand (command, arguments, interpreter)) {
				if (Melder_hasError ()) {
					if (arguments [0] != '\0' && arguments [wcslen (arguments) - 1] == ' ') {
						return Melder_error3 (L"It may be helpful to remove the trailing spaces in \"", arguments, L"\".");
					}
					return 0;
				} else if (wcsnequ (command, L"ARGS ", 5)) {
					return Melder_error1 (L"Command \"ARGS\" no longer supported. Instead use \"form\" and \"endform\".");
				} else if (wcschr (command, '=')) {
					return Melder_error3 (L"Command \"", command, L"\" not recognized.\n"
						"Probable cause: you are trying to use a variable name that starts with a capital.");
				} else if (command [0] != '\0' && command [wcslen (command) - 1] == ' ') {
					return Melder_error3 (L"Command \"", command, L"\" not available for current selection. "
						"It may be helpful to remove the trailing spaces.");
				} else {
					return Melder_error3 (L"Command \"", command, L"\" not available for current selection.");
				}
			}
		}
		praat_updateSelection ();
	}
	return 1;
}

int praat_executeCommandFromStandardInput (const char *programName) {
	char command [1000]; /* Can be recursive. */
	for (;;) {
		wchar_t *commandW = NULL;
	//start:
		char *newLine;
		printf ("%s > ", programName);
		if (! fgets (command, 999, stdin)) return 0;
		newLine = strchr (command, '\n');
		if (newLine) *newLine = '\0';
		commandW = Melder_utf8ToWcs_e (command); cherror
		if (! praat_executeCommand (NULL, commandW))
			Melder_flushError ("%s: command \"%s\" not executed.", programName, command);
	end:
		Melder_free (commandW);
	}
	return 1;
}

int praat_executeScriptFromFile (MelderFile file, const wchar_t *arguments) {
	wchar_t *text = NULL;
	Interpreter *interpreter = NULL;
	structMelderDir saveDir = { { 0 } };
	Melder_getDefaultDir (& saveDir);   /* Before the first cherror! */
//start:
	text = MelderFile_readText (file); cherror
	MelderFile_setDefaultDir (file);   /* So that relative file names can be used inside the script. */
	Melder_includeIncludeFiles (& text); cherror
	{
		Editor *editor = (Editor *)praatP.editor;
		interpreter = new Interpreter (editor->_name); cherror
		if (arguments) {
			interpreter->readParameters (text); cherror
			interpreter->getArgumentsFromString (arguments); cherror
		}
		interpreter->run (text);
	}
end:
	Melder_setDefaultDir (& saveDir);
	Melder_free (text);
	forget (interpreter);
	iferror return Melder_error3 (L"Script ", MelderFile_messageName (file), L" not completed.");
	return 1;
}

int praat_executeScriptFromFileNameWithArguments (const wchar_t *nameAndArguments) {
	wchar_t path [256];
	const wchar_t *p, *arguments;
	structMelderFile file = { 0 };
	/*
	 * Split into file name and arguments.
	 */
	p = nameAndArguments;
	while (*p == ' ' || *p == '\t') p ++;
	if (*p == '\"') {
		wchar_t *q = path;
		p ++;   // skip quote
		while (*p != '\"' && *p != '\0') * q ++ = * p ++;
		*q = '\0';
		arguments = p;
		if (*arguments == '\"') arguments ++;
		if (*arguments == ' ') arguments ++;
	} else {
		wchar_t *q = path;
		while (*p != ' ' && *p != '\0') * q ++ = * p ++;
		*q = '\0';
		arguments = p;
		if (*arguments == ' ') arguments ++;
	}
	if (! Melder_relativePathToFile (path, & file)) return 0;
	return praat_executeScriptFromFile (& file, arguments);
}

int praat_executeScriptFromText (wchar_t *text) {
	Interpreter *interpreter = new Interpreter (NULL);
	interpreter->run (text);
	forget (interpreter);
	iferror return Melder_error1 (L"Script not completed.");
	return 1;
}

int praat_executeScriptFromDialog (UiForm *dia) {
	Interpreter *interpreter = NULL;
	wchar_t *text = NULL;
	structMelderDir saveDir = { { 0 } };
	Melder_getDefaultDir (& saveDir);
//start:
	wchar_t *path = dia->getString (L"$file");
	structMelderFile file = { 0 };
	Melder_pathToFile (path, & file); cherror
	text = MelderFile_readText (& file); cherror
	MelderFile_setDefaultDir (& file);
	Melder_includeIncludeFiles (& text); cherror
	{
		Editor *editor = (Editor *)praatP.editor;
		interpreter = new Interpreter (editor->_name); cherror
		interpreter->readParameters (text); cherror
		interpreter->getArgumentsFromDialog (dia); cherror
		praat_background ();
		interpreter->run (text);
		praat_foreground ();
	}
end:
	Melder_setDefaultDir (& saveDir);
	Melder_free (text);
	forget (interpreter);
	iferror return 0;
	return 1;
}

static int secondPassThroughScript (UiForm *sendingForm, const wchar_t *sendingString_dummy, Interpreter *interpreter_dummy, const wchar_t *invokingButtonTitle, bool modified, void *dummy) {
	(void) sendingString_dummy;
	(void) interpreter_dummy;
	(void) invokingButtonTitle;
	(void) modified;
	(void) dummy;
	return praat_executeScriptFromDialog (sendingForm);
}

static int firstPassThroughScript (MelderFile file) {
	wchar_t *text = NULL;
	Interpreter *interpreter = NULL;
	structMelderDir saveDir = { { 0 } };
	Melder_getDefaultDir (& saveDir);
//start:
	text = MelderFile_readText (file); cherror
	MelderFile_setDefaultDir (file);
	Melder_includeIncludeFiles (& text); cherror
	{
		Melder_setDefaultDir (& saveDir);
		Editor *editor = (Editor *)praatP.editor;
		interpreter = new Interpreter (editor->_name); cherror
		if (interpreter->readParameters (text) > 0) {
			UiForm *form = interpreter->createForm (
				praatP.editor ? ((Editor *) praatP.editor) -> _dialog : theCurrentPraatApplication -> topShell,
				Melder_fileToPath (file), secondPassThroughScript, NULL);
			form->do_ (false);
		} else {
			praat_background ();
			praat_executeScriptFromFile (file, NULL);
			praat_foreground ();
		}
	}
end:
	Melder_setDefaultDir (& saveDir);
	Melder_free (text);
	forget (interpreter);
	iferror return Melder_error3 (L"Script ", MelderFile_messageName (file), L" not completed.");
	return 1;
}

static int fileSelectorOkCallback (UiForm *dia, const wchar_t *sendingString_dummy, Interpreter *interpreter_dummy, const wchar_t *invokingButtonTitle, bool modified, void *dummy) {
	(void) sendingString_dummy;
	(void) interpreter_dummy;
	(void) invokingButtonTitle;
	(void) modified;
	(void) dummy;
	return firstPassThroughScript (((UiFile*)dia)->getFile ());
}

/*
 * DO_praat_runScript () is the command callback for "Run script...", which is a bit obsolete command,
 * hidden in the Praat menu, and otherwise replaced by "execute".
 */
int DO_praat_runScript (UiForm *sendingForm, const wchar_t *sendingString, Interpreter *interpreter_dummy, const wchar_t *invokingButtonTitle, bool modified, void *dummy) {
	(void) interpreter_dummy;
	(void) modified;
	(void) dummy;
	if (sendingForm == NULL && sendingString == NULL) {
		/*
		 * User clicked the "Run script..." button in the Praat menu.
		 */
		static UiInfile *file_dialog;
		if (! file_dialog)
			file_dialog = new UiInfile (theCurrentPraatApplication -> topShell, L"Praat: run script", fileSelectorOkCallback, NULL, invokingButtonTitle, NULL, false);
		file_dialog->do_ ();
	} else {
		/*
		 * A script called "Run script..."
		 */
		praat_executeScriptFromFileNameWithArguments (sendingString);
	}
	return 1;
}

int DO_RunTheScriptFromAnyAddedMenuCommand (UiForm *sendingForm_dummy, const wchar_t *scriptPath, Interpreter *interpreter, const wchar_t *invokingButtonTitle, bool modified, void *dummy) {
	structMelderFile file = { 0 };
	(void) sendingForm_dummy;
	(void) interpreter;
	(void) invokingButtonTitle;
	(void) modified;
	(void) dummy;
	if (! Melder_relativePathToFile ((wchar_t *) scriptPath, & file)) return 0;
	return firstPassThroughScript (& file);
}

int DO_RunTheScriptFromAnyAddedEditorCommand (Any editor, const wchar_t *script) {
	praatP.editor = editor;
	DO_RunTheScriptFromAnyAddedMenuCommand (NULL, script, NULL, NULL, false, NULL);
	/*praatP.editor = NULL;*/
	iferror return 0;
	return 1;
}

/* End of file praat_script.c */
