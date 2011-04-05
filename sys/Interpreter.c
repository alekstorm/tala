/* Interpreter.c
 *
 * Copyright (C) 1993-2010 Paul Boersma
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
 * pb 2002/03/25 option menus
 * pb 2002/06/04 include the script compiler
 * pb 2002/09/26 removed bug: crashed if a line in a form contained only the word "comment"
 * pb 2002/11/25 Melder_double
 * pb 2002/12/10 include files
 * pb 2002/12/14 more informative error messages
 * pb 2003/05/19 Melder_atof
 * pb 2003/07/15 assert
 * pb 2003/07/19 if undefined fails
 * pb 2004/10/16 C++ compatible structs
 * pb 2004/12/06 made Interpreter_getArgumentsFromDialog resistant to changes in the script while the dialog is up
 * pb 2005/01/01 there can be spaces before the "form" statement
 * pb 2005/11/26 allow mixing of "option" and "button", as in Ui.c
 * pb 2006/01/11 local variables
 * pb 2007/02/05 preferencesDirectory$, homeDirectory$, temporaryDirectory$
 * pb 2007/04/02 allow comments (with '#' or ';' or empty lines) in forms
 * pb 2007/04/19 allow comments with '!' in forms
 * pb 2007/05/24 some wchar_t
 * pb 2007/06/09 wchar_t
 * pb 2007/08/12 more wchar_t
 * pb 2007/11/30 removed bug: allowed long arguments to the "call" statement (thanks to Ingmar Steiner)
 * pb 2007/12/10 predefined numeric variables macintosh/windows/unix
 * pb 2008/04/30 new Formula API
 * pb 2008/05/01 arrays
 * pb 2008/05/15 praatVersion, praatVersion$
 * pb 2009/01/04 Interpreter_voidExpression
 * pb 2009/01/17 arguments to UiForm callbacks
 * pb 2009/01/20 pause forms
 * pb 2009/03/17 split up structPraat
 * pb 2009/12/22 invokingButtonTitle
 * pb 2010/04/30 guard against leading nonbreaking spaces
 */

#include <ctype.h>
#include "Interpreter.h"
#include "praatP.h"
extern structMelderDir praatDir;
#include "praat_script.h"
#include "Formula.h"
#include "praat_version.h"
#include "UnicodeData.h"

#define Interpreter_WORD 1
#define Interpreter_REAL 2
#define Interpreter_POSITIVE 3
#define Interpreter_INTEGER 4
#define Interpreter_NATURAL 5
#define Interpreter_BOOLEAN 6
#define Interpreter_SENTENCE 7
#define Interpreter_TEXT 8
#define Interpreter_CHOICE 9
#define Interpreter_OPTIONMENU 10
#define Interpreter_BUTTON 11
#define Interpreter_OPTION 12
#define Interpreter_COMMENT 13

InterpreterVariable::InterpreterVariable (const wchar_t *key) {
	if (key [0] == 'e' && key [1] == '\0')
		throw Exception ("You cannot use 'e' as the name of a variable (e is the constant 2.71...).");
	if (key [0] == 'p' && key [1] == 'i' && key [2] == '\0')
		throw Exception ("You cannot use 'pi' as the name of a variable (pi is the constant 3.14...).");
	if (key [0] == 'u' && key [1] == 'n' && key [2] == 'd' && key [3] == 'e' && key [4] == 'f' && key [5] == 'i' &&
		key [6] == 'n' && key [7] == 'e' && key [8] == 'd' && key [9] == '\0')
		throw new Exception ("You cannot use 'undefined' as the name of a variable.");
	_key = Melder_wcsdup_e (key);
}

InterpreterVariable::~InterpreterVariable () {
	Melder_free (_key);
	Melder_free (_stringValue);
	NUMdmatrix_free (_numericArrayValue. data, 1, 1);
}

bool Interpreter::isCommand (const wchar_t *p) {
	/*
	 * Things that start with "nowarn", "noprogress", or "nocheck" are commands.
	 */
	if (p [0] == 'n' && p [1] == 'o' &&
		(wcsnequ (p + 2, L"warn ", 5) || wcsnequ (p + 2, L"progress ", 9) || wcsnequ (p + 2, L"check ", 6))) return true;
	if (wcsnequ (p, L"demo ", 5)) return true;
	/*
	 * Otherwise, things that start with lower case are formulas.
	 */
	if (! isupper (*p)) return false;
	/*
	 * The remaining possibility is things that start with upper case.
	 * If they contain an underscore, they are object names, hence we must have a formula.
	 * Otherwise, we have a command.
	 */
	while (isalnum (*p)) p ++;
	return *p != '_';
}

int Interpreter::voidExpression (const wchar_t *expression) {
	Formula_compile (this, NULL, expression, kFormula_EXPRESSION_TYPE_NUMERIC, FALSE); cherror
	struct Formula_Result result;
	Formula_run (0, 0, & result); cherror
end:
	iferror return 0;
	return 1;
}

int Interpreter::numericExpression (const wchar_t *expression, double *value) {
	Melder_assert (value != NULL);
	if (wcsstr (expression, L"(=")) {
		*value = Melder_atof (expression);
	} else {
		Formula_compile (this, NULL, expression, kFormula_EXPRESSION_TYPE_NUMERIC, FALSE); cherror
		struct Formula_Result result;
		Formula_run (0, 0, & result); cherror
		*value = result. result.numericResult;
	}
end:
	iferror return 0;
	return 1;
}

int Interpreter_numericExpression_FIXME (const wchar_t *expression, double *value) {
	return Interpreter (NULL, NULL).numericExpression(expression, value);
}

int Interpreter::stringExpression (const wchar_t *expression, wchar_t **value) {
	Formula_compile (this, NULL, expression, kFormula_EXPRESSION_TYPE_STRING, FALSE); cherror
	struct Formula_Result result;
	Formula_run (0, 0, & result); cherror
	*value = result. result.stringResult;
end:
	iferror return 0;
	return 1;
}

int Interpreter::numericArrayExpression (const wchar_t *expression, struct Formula_NumericArray *value) {
	Formula_compile (this, NULL, expression, kFormula_EXPRESSION_TYPE_NUMERIC_ARRAY, FALSE); cherror
	struct Formula_Result result;
	Formula_run (0, 0, & result); cherror
	*value = result. result.numericArrayResult;
end:
	iferror return 0;
	return 1;
}

int Interpreter::anyExpression (const wchar_t *expression, struct Formula_Result *result) {
	Formula_compile (this, NULL, expression, kFormula_EXPRESSION_TYPE_UNKNOWN, FALSE); cherror
	Formula_run (0, 0, result); cherror
end:
	iferror return 0;
	return 1;
}

Interpreter::Interpreter (wchar_t *environmentName, Any editorClass) {
	_variables = SortedSetOfString_create ();
	_environmentName = Melder_wcsdup_f (environmentName);
	_editorClass = editorClass;
}

Interpreter::~Interpreter () {
	Melder_free (_environmentName);
	for (int ipar = 1; ipar <= Interpreter_MAXNUM_PARAMETERS; ipar ++)
		Melder_free (_arguments [ipar]);
	forget (_variables);
}

int Interpreter::readParameters (wchar_t *text) {
	wchar_t *formLocation = NULL;
	int npar = 0;
	_dialogTitle [0] = '\0';
	/*
	 * Look for a "form" line.
	 */
	{
		wchar_t *p = text;
		for (;;) {
			while (*p == ' ' || *p == '\t') p ++;
			if (wcsnequ (p, L"form ", 5)) {
				formLocation = p;
				break;
			}
			while (*p != '\0' && *p != '\n') p ++;
			if (*p == '\0') break;
			p ++;   /* Skip newline symbol. */
		}
	}
	/*
	 * If there is no "form" line, there are no parameters.
	 */
	if (formLocation) {
		wchar_t *dialogTitle = formLocation + 5, *newLine;
		while (*dialogTitle == ' ' || *dialogTitle == '\t') dialogTitle ++;
		newLine = wcschr (dialogTitle, '\n');
		if (newLine) *newLine = '\0';
		wcscpy (_dialogTitle, dialogTitle);
		if (newLine) *newLine = '\n';
		_numberOfParameters = 0;
		while (newLine) {
			wchar_t *line = newLine + 1, *p;
			int type = 0;
			while (*line == ' ' || *line == '\t') line ++;
			while (*line == '#' || *line == ';' || *line == '!' || *line == '\n') {
				newLine = wcschr (line, '\n');
				if (newLine == NULL) return Melder_error1 (L"Unfinished form.");
				line = newLine + 1;
				while (*line == ' ' || *line == '\t') line ++;
			}
			if (wcsnequ (line, L"endform", 7)) break;
			if (wcsnequ (line, L"word ", 5)) { type = Interpreter_WORD; p = line + 5; }
			else if (wcsnequ (line, L"real ", 5)) { type = Interpreter_REAL; p = line + 5; }
			else if (wcsnequ (line, L"positive ", 9)) { type = Interpreter_POSITIVE; p = line + 9; }
			else if (wcsnequ (line, L"integer ", 8)) { type = Interpreter_INTEGER; p = line + 8; }
			else if (wcsnequ (line, L"natural ", 8)) { type = Interpreter_NATURAL; p = line + 8; }
			else if (wcsnequ (line, L"boolean ", 8)) { type = Interpreter_BOOLEAN; p = line + 8; }
			else if (wcsnequ (line, L"sentence ", 9)) { type = Interpreter_SENTENCE; p = line + 9; }
			else if (wcsnequ (line, L"text ", 5)) { type = Interpreter_TEXT; p = line + 5; }
			else if (wcsnequ (line, L"choice ", 7)) { type = Interpreter_CHOICE; p = line + 7; }
			else if (wcsnequ (line, L"optionmenu ", 11)) { type = Interpreter_OPTIONMENU; p = line + 11; }
			else if (wcsnequ (line, L"button ", 7)) { type = Interpreter_BUTTON; p = line + 7; }
			else if (wcsnequ (line, L"option ", 7)) { type = Interpreter_OPTION; p = line + 7; }
			else if (wcsnequ (line, L"comment ", 8)) { type = Interpreter_COMMENT; p = line + 8; }
			else {
				newLine = wcschr (line, '\n');
				if (newLine) *newLine = '\0';
				Melder_error3 (L"Unknown parameter type:\n\"", line, L"\".");
				if (newLine) *newLine = '\n';
				return 0;
			}
			/*
				Example:
					form Something
						real Time_(s) 3.14 (= pi)
						choice Colour 2
							button Red
							button Green
							button Blue
					endform
				_parameters [1] := "Time_(s)"
				_parameters [2] := "Colour"
				_parameters [3] := ""
				_parameters [4] := ""
				_parameters [5] := ""
				_arguments [1] := "3.14 (= pi)"
				_arguments [2] := "2"
				_arguments [3] := "Red"   (funny, but needed in Interpreter_getArgumentsFromString)
				_arguments [4] := "Green"
				_arguments [5] := "Blue"
			*/
			if (type <= Interpreter_OPTIONMENU) {
				while (*p == ' ' || *p == '\t') p ++;
				if (*p == '\n' || *p == '\0')
					return Melder_error3 (L"Missing parameter:\n\"", line, L"\".");
				wchar_t *q = _parameters [++ _numberOfParameters];
				while (*p != ' ' && *p != '\t' && *p != '\n' && *p != '\0') * (q ++) = * (p ++);
				*q = '\0';
				npar ++;
			} else {
				_parameters [++ _numberOfParameters] [0] = '\0';
			}
			while (*p == ' ' || *p == '\t') p ++;
			newLine = wcschr (p, '\n');
			if (newLine) *newLine = '\0';
			Melder_free (_arguments [_numberOfParameters]);
			_arguments [_numberOfParameters] = Melder_wcsdup_f (p);
			if (newLine) *newLine = '\n';
			_types [_numberOfParameters] = type;
		}
	} else {
		npar = _numberOfParameters = 0;
	}
	return npar;
}

Any Interpreter::createForm (GuiObject parent, const wchar_t *path, int (*okCallback) (UiForm, const wchar_t *, Interpreter *, const wchar_t *, bool, void *), void *okClosure) {
	Any form = UiForm_create (parent, _dialogTitle [0] ? _dialogTitle : L"Script arguments", okCallback, okClosure, NULL, NULL);
	Any radio = NULL;
	if (path) UiForm_addText (form, L"$file", path);
	for (int ipar = 1; ipar <= _numberOfParameters; ipar ++) {
		/*
		 * Convert underscores to spaces.
		 */
		wchar_t parameter [100], *p = & parameter [0];
		wcscpy (parameter, _parameters [ipar]);
		while (*p) { if (*p == '_') *p = ' '; p ++; }
		switch (_types [ipar]) {
			case Interpreter_WORD:
				UiForm_addWord (form, parameter, _arguments [ipar]); break;
			case Interpreter_REAL:
				UiForm_addReal (form, parameter, _arguments [ipar]); break;
			case Interpreter_POSITIVE:
				UiForm_addPositive (form, parameter, _arguments [ipar]); break;
			case Interpreter_INTEGER:
				UiForm_addInteger (form, parameter, _arguments [ipar]); break;
			case Interpreter_NATURAL:
				UiForm_addNatural (form, parameter, _arguments [ipar]); break;
			case Interpreter_BOOLEAN:
				UiForm_addBoolean (form, parameter, _arguments [ipar] [0] == '1' ||
					_arguments [ipar] [0] == 'y' || _arguments [ipar] [0] == 'Y' ||
					(_arguments [ipar] [0] == 'o' && _arguments [ipar] [1] == 'n')); break;
			case Interpreter_SENTENCE:
				UiForm_addSentence (form, parameter, _arguments [ipar]); break;
			case Interpreter_TEXT:
				UiForm_addText (form, parameter, _arguments [ipar]); break;
			case Interpreter_CHOICE:
				radio = UiForm_addRadio (form, parameter, wcstol (_arguments [ipar], NULL, 10)); break;
			case Interpreter_OPTIONMENU:
				radio = UiForm_addOptionMenu (form, parameter, wcstol (_arguments [ipar], NULL, 10)); break;
			case Interpreter_BUTTON:
				if (radio) UiRadio_addButton (radio, _arguments [ipar]); break;
			case Interpreter_OPTION:
				if (radio) UiOptionMenu_addButton (radio, _arguments [ipar]); break;
			case Interpreter_COMMENT:
				UiForm_addLabel (form, parameter, _arguments [ipar]); break;
			default:
				UiForm_addWord (form, parameter, _arguments [ipar]); break;
		}
		/*
		 * Strip parentheses and colon off parameter name.
		 */
		if ((p = wcschr (_parameters [ipar], '(')) != NULL) {
			*p = '\0';
			if (p - _parameters [ipar] > 0 && p [-1] == '_') p [-1] = '\0';
		}
		p = _parameters [ipar];
		if (*p != '\0' && p [wcslen (p) - 1] == ':') p [wcslen (p) - 1] = '\0';
	}
	UiForm_finish (form);
	return form;
}

int Interpreter::getArgumentsFromDialog (Any dialog) {
	for (int ipar = 1; ipar <= _numberOfParameters; ipar ++) {
		wchar_t parameter [100], *p;
		/*
		 * Strip parentheses and colon off parameter name.
		 */
		if ((p = wcschr (_parameters [ipar], '(')) != NULL) {
			*p = '\0';
			if (p - _parameters [ipar] > 0 && p [-1] == '_') p [-1] = '\0';
		}
		p = _parameters [ipar];
		if (*p != '\0' && p [wcslen (p) - 1] == ':') p [wcslen (p) - 1] = '\0';
		/*
		 * Convert underscores to spaces.
		 */
		wcscpy (parameter, _parameters [ipar]);
		p = & parameter [0]; while (*p) { if (*p == '_') *p = ' '; p ++; }
		switch (_types [ipar]) {
			case Interpreter_REAL:
			case Interpreter_POSITIVE: {
				double value = UiForm_getReal_check (dialog, parameter); cherror
				Melder_free (_arguments [ipar]);
				_arguments [ipar] = Melder_calloc_f (wchar_t, 40);
				wcscpy (_arguments [ipar], Melder_double (value));
				break;
			}
			case Interpreter_INTEGER:
			case Interpreter_NATURAL:
			case Interpreter_BOOLEAN: {
				long value = UiForm_getInteger (dialog, parameter); cherror
				Melder_free (_arguments [ipar]);
				_arguments [ipar] = Melder_calloc_f (wchar_t, 40);
				swprintf (_arguments [ipar], 40, L"%ld", value);
				break;
			}
			case Interpreter_CHOICE:
			case Interpreter_OPTIONMENU: {
				long integerValue = 0;
				wchar_t *stringValue = NULL;
				integerValue = UiForm_getInteger (dialog, parameter); cherror
				stringValue = UiForm_getString (dialog, parameter); cherror
				Melder_free (_arguments [ipar]);
				_arguments [ipar] = Melder_calloc_f (wchar_t, 40);
				swprintf (_arguments [ipar], 40, L"%ld", integerValue);
				wcscpy (_choiceArguments [ipar], stringValue);
				break;
			}
			case Interpreter_BUTTON:
			case Interpreter_OPTION:
			case Interpreter_COMMENT:
				break;
			default: {
				wchar_t *value = UiForm_getString (dialog, parameter); cherror
				Melder_free (_arguments [ipar]);
				_arguments [ipar] = Melder_wcsdup_f (value);
				break;
			}
		}
	}
end:
	iferror return 0;
	return 1;
}

int Interpreter::getArgumentsFromString (const wchar_t *arguments) {
	int size = _numberOfParameters;
	long length = wcslen (arguments);
	while (size >= 1 && _parameters [size] [0] == '\0')
		size --;   /* Ignore fields without a variable name (button, comment). */
	for (int ipar = 1; ipar <= size; ipar ++) {
		wchar_t *p = _parameters [ipar];
		/*
		 * Ignore buttons and comments again.
		 */
		if (! *p) continue;
		/*
		 * Strip parentheses and colon off parameter name.
		 */
		if ((p = wcschr (p, '(')) != NULL) {
			*p = '\0';
			if (p - _parameters [ipar] > 0 && p [-1] == '_') p [-1] = '\0';
		}
		p = _parameters [ipar];
		if (*p != '\0' && p [wcslen (p) - 1] == ':') p [wcslen (p) - 1] = '\0';
	}
	for (int ipar = 1; ipar < size; ipar ++) {
		int ichar = 0;
		/*
		 * Ignore buttons and comments again. The buttons will keep their labels as "arguments".
		 */
		if (_parameters [ipar] [0] == '\0') continue;
		Melder_free (_arguments [ipar]);   /* Erase the current values, probably the default values. */
		_arguments [ipar] = Melder_calloc_f (wchar_t, length + 1);   /* Replace with the actual arguments. */
		/*
		 * Skip spaces until next argument.
		 */
		while (*arguments == ' ' || *arguments == '\t') arguments ++;
		/*
		 * The argument is everything up to the next space, or, if that starts with a double quote,
		 * everything between this quote and the matching double quote;
		 * in this case, the argument can represent a double quote by a sequence of two double quotes.
		 * Example: the string
		 *     "I said ""hello"""
		 * will be passed to the dialog as a single argument containing the text
		 *     I said "hello"
		 */
		if (*arguments == '\"') {
			arguments ++;   /* Do not include leading double quote. */
			for (;;) {
				if (*arguments == '\0') return Melder_error1 (L"Missing matching quote.");
				if (*arguments == '\"' && * ++ arguments != '\"') break;   /* Remember second quote. */
				_arguments [ipar] [ichar ++] = *arguments ++;
			}
		} else {
			while (*arguments != ' ' && *arguments != '\t' && *arguments != '\0')
				_arguments [ipar] [ichar ++] = *arguments ++;
		}
		_arguments [ipar] [ichar] = '\0';   /* Trailing null byte. */
	}
	/* The last item is handled separately, because it consists of the rest of the line.
	 * Leading spaces are skipped, but trailing spaces are included.
	 */
	if (size > 0) {
		while (*arguments == ' ' || *arguments == '\t') arguments ++;
		Melder_free (_arguments [size]);
		_arguments [size] = Melder_wcsdup_f (arguments);
	}
	/*
	 * Convert booleans and choices to numbers.
	 */
	for (int ipar = 1; ipar <= size; ipar ++) {
		if (_types [ipar] == Interpreter_BOOLEAN) {
			wchar_t *arg = & _arguments [ipar] [0];
			if (wcsequ (arg, L"1") || wcsequ (arg, L"yes") || wcsequ (arg, L"on") ||
			    wcsequ (arg, L"Yes") || wcsequ (arg, L"On") || wcsequ (arg, L"YES") || wcsequ (arg, L"ON"))
			{
				wcscpy (arg, L"1");
			} else if (wcsequ (arg, L"0") || wcsequ (arg, L"no") || wcsequ (arg, L"off") ||
			    wcsequ (arg, L"No") || wcsequ (arg, L"Off") || wcsequ (arg, L"NO") || wcsequ (arg, L"OFF"))
			{
				wcscpy (arg, L"0");
			} else {
				return Melder_error5 (L"Unknown value \"", arg, L"\" for boolean \"", _parameters [ipar], L"\".");
			}
		} else if (_types [ipar] == Interpreter_CHOICE) {
			int jpar;
			wchar_t *arg = & _arguments [ipar] [0];
			for (jpar = ipar + 1; jpar <= _numberOfParameters; jpar ++) {
				if (_types [jpar] != Interpreter_BUTTON && _types [jpar] != Interpreter_OPTION)
					return Melder_error5 (L"Unknown value \"", arg, L"\" for choice \"", _parameters [ipar], L"\".");
				if (wcsequ (_arguments [jpar], arg)) {   /* The button labels are in the arguments, see Interpreter_readParameters */
					swprintf (arg, 40, L"%d", jpar - ipar);
					wcscpy (_choiceArguments [ipar], _arguments [jpar]);
					break;
				}
			}
			if (jpar > _numberOfParameters)
				return Melder_error5 (L"Unknown value \"", arg, L"\" for choice \"", _parameters [ipar], L"\".");
		} else if (_types [ipar] == Interpreter_OPTIONMENU) {
			int jpar;
			wchar_t *arg = & _arguments [ipar] [0];
			for (jpar = ipar + 1; jpar <= _numberOfParameters; jpar ++) {
				if (_types [jpar] != Interpreter_OPTION && _types [jpar] != Interpreter_BUTTON)
					return Melder_error5 (L"Unknown value \"", arg, L"\" for option menu \"", _parameters [ipar], L"\".");
				if (wcsequ (_arguments [jpar], arg)) {
					swprintf (arg, 40, L"%d", jpar - ipar);
					wcscpy (_choiceArguments [ipar], _arguments [jpar]);
					break;
				}
			}
			if (jpar > _numberOfParameters)
				return Melder_error5 (L"Unknown value \"", arg, L"\" for option menu \"", _parameters [ipar], L"\".");
		}
	}
	return 1;
}

int Interpreter::addNumericVariable (const wchar_t *key, double value) {
	InterpreterVariable *variable = new InterpreterVariable (key);
	if (! variable || ! Collection_addItem (_variables, variable)) return 0;
	variable -> _numericValue = value;
	return 1;
}

InterpreterVariable* Interpreter::addStringVariable (const wchar_t *key, const wchar_t *value) {
	InterpreterVariable *variable = new InterpreterVariable (key);
	if (! variable || ! Collection_addItem (_variables, variable)) return NULL;
	variable -> _stringValue = Melder_wcsdup_f (value);
	return variable;
}

InterpreterVariable* Interpreter::hasVariable (const wchar_t *key) {
	long ivar = 0;
	wchar_t variableNameIncludingProcedureName [1+200];
	Melder_assert (key != NULL);
	if (key [0] == '.') {
		wcscpy (variableNameIncludingProcedureName, _procedureNames [_callDepth]);
		wcscat (variableNameIncludingProcedureName, key);
	} else {
		wcscpy (variableNameIncludingProcedureName, key);
	}
	ivar = SortedSetOfString_lookUp (_variables, variableNameIncludingProcedureName);
	return (InterpreterVariable*)(ivar ? _variables -> item [ivar] : NULL);
}

InterpreterVariable* Interpreter::lookUpVariable (const wchar_t *key) {
	InterpreterVariable *var = NULL;
	wchar_t variableNameIncludingProcedureName [1+200];
	Melder_assert (key != NULL);
	if (key [0] == '.') {
		wcscpy (variableNameIncludingProcedureName, _procedureNames [_callDepth]);
		wcscat (variableNameIncludingProcedureName, key);
	} else {
		wcscpy (variableNameIncludingProcedureName, key);
	}
	var = hasVariable (variableNameIncludingProcedureName);
	if (var) return var;
	var = new InterpreterVariable (variableNameIncludingProcedureName);
	if (! var || ! Collection_addItem (_variables, var)) return NULL;
	return hasVariable (variableNameIncludingProcedureName);
}

long Interpreter::lookupLabel (const wchar_t *labelName) {
	int ilabel;
	for (ilabel = 1; ilabel <= _numberOfLabels; ilabel ++)
		if (wcsequ (labelName, _labelNames [ilabel]))
			return ilabel;
	return Melder_error3 (L"Unknown label \"", labelName, L"\".");
}

void Interpreter::parameterToVariable (int type, const wchar_t *in_parameter, int ipar) {
	wchar_t parameter [200];
	Melder_assert (type != 0);
	wcscpy (parameter, in_parameter);
	if (type >= Interpreter_REAL && type <= Interpreter_BOOLEAN) {
		addNumericVariable (parameter, Melder_atof (_arguments [ipar]));
	} else if (type == Interpreter_CHOICE || type == Interpreter_OPTIONMENU) {
		addNumericVariable (parameter, Melder_atof (_arguments [ipar]));
		wcscat (parameter, L"$");
		addStringVariable (parameter, _choiceArguments [ipar]);
	} else if (type == Interpreter_BUTTON || type == Interpreter_OPTION || type == Interpreter_COMMENT) {
		/* Do not add a variable. */
	} else {
		wcscat (parameter, L"$");
		addStringVariable (parameter, _arguments [ipar]);
	}
}

int Interpreter::run (wchar_t *text) {
	static MelderString valueString = { 0 };   /* To divert the info. */
	static MelderString assertErrorString = { 0 };
	wchar_t *command = text;
	MelderString command2 = { 0 }, buffer = { 0 };
	wchar_t **lines = NULL;
	long lineNumber = 0, numberOfLines = 0, assertErrorLineNumber = 0, callStack [1 + Interpreter_MAX_CALL_DEPTH];
	int atLastLine = FALSE, fromif = FALSE, fromendfor = FALSE, callDepth = 0, chopped = 0, ipar, assertionFailed = FALSE;
	_callDepth = 0;
	/*
	 * The "environment" is NULL if we are in the Praat shell, or an editor otherwise.
	 */
	if (_editorClass) {
		praatP. editor = praat_findEditorFromString (_environmentName);
		if (praatP. editor == NULL)
			return Melder_error3 (L"Editor \"", _environmentName, L"\" does not exist.");
	} else {
		praatP. editor = NULL;
	}
	/*
	 * Start.
	 */
	_running = true;
	/*
	 * Count lines and set the newlines to zero.
	 */
	while (! atLastLine) {
		wchar_t *endOfLine = command;
		while (*endOfLine != '\n' && *endOfLine != '\0') endOfLine ++;
		if (*endOfLine == '\0') atLastLine = TRUE;
		*endOfLine = '\0';
		numberOfLines ++;
		command = endOfLine + 1;
	}
	/*
	 * Remember line starts and labels.
	 */
	lines = (wchar_t**)NUMpvector (1, numberOfLines); cherror
	for (lineNumber = 1, command = text; lineNumber <= numberOfLines; lineNumber ++, command += wcslen (command) + 1 + chopped) {
		int length;
		while (*command == ' ' || *command == '\t' || *command == UNICODE_NO_BREAK_SPACE) command ++;   // nbsp can occur for scripts copied from the manual
		length = wcslen (command);
		/*
		 * Chop trailing spaces?
		 */
		/*chopped = 0;
		while (length > 0) { char kar = command [-- length]; if (kar != ' ' && kar != '\t') break; command [length] = '\0'; chopped ++; }*/
		lines [lineNumber] = command;
		if (wcsnequ (command, L"label ", 6)) {
			int ilabel;
			for (ilabel = 1; ilabel <= _numberOfLabels; ilabel ++)
				if (wcsequ (command + 6, _labelNames [ilabel]))
					error3 (L"Duplicate label \"", command + 6, L"\".")
			if (_numberOfLabels >= Interpreter_MAXNUM_LABELS)
				error1 (L"Too many labels.")
			swprintf (_labelNames [++ _numberOfLabels], 50, L"%.47ls", command + 6);
			_labelLines [_numberOfLabels] = lineNumber;
		}
	}
	/*
	 * Connect continuation lines.
	 */
	for (lineNumber = numberOfLines; lineNumber >= 2; lineNumber --) {
		wchar_t *line = lines [lineNumber];
		if (line [0] == '.' && line [1] == '.' && line [2] == '.') {
			wchar_t *previous = lines [lineNumber - 1];
			MelderString_copy (& command2, line + 3);
			MelderString_get (& command2, previous + wcslen (previous));
			lines [lineNumber] = L"";
		}
	}
	/*
	 * Copy the parameter names and argument values into the array of variables.
	 */
	forget (_variables);
	_variables = SortedSetOfString_create ();
	for (ipar = 1; ipar <= _numberOfParameters; ipar ++) {
		wchar_t parameter [200];
		/*
		 * Create variable names as-are and variable names without capitals.
		 */
		wcscpy (parameter, _parameters [ipar]);
		parameterToVariable (_types [ipar], parameter, ipar); cherror
		if (parameter [0] >= 'A' && parameter [0] <= 'Z') {
			parameter [0] = tolower (parameter [0]);
			parameterToVariable (_types [ipar], parameter, ipar); cherror
		}
	}
	{
		/*
		* Initialize some variables.
		*/
		addStringVariable (L"newline$", L"\n");
		addStringVariable (L"tab$", L"\t");
		addStringVariable (L"shellDirectory$", Melder_getShellDirectory ());
		structMelderDir dir = { { 0 } }; Melder_getDefaultDir (& dir);
		addStringVariable (L"defaultDirectory$", Melder_dirToPath (& dir));
		addStringVariable (L"preferencesDirectory$", Melder_dirToPath (& praatDir));
		Melder_getHomeDir (& dir);
		addStringVariable (L"homeDirectory$", Melder_dirToPath (& dir));
		Melder_getTempDir (& dir);
		addStringVariable (L"temporaryDirectory$", Melder_dirToPath (& dir));
		#if defined (macintosh)
			addNumericVariable (L"macintosh", 1);
			addNumericVariable (L"windows", 0);
			addNumericVariable (L"unix", 0);
		#elif defined (_WIN32)
			addNumericVariable (L"macintosh", 0);
			addNumericVariable (L"windows", 1);
			addNumericVariable (L"unix", 0);
		#elif defined (UNIX)
			addNumericVariable (L"macintosh", 0);
			addNumericVariable (L"windows", 0);
			addNumericVariable (L"unix", 1);
		#else
			addNumericVariable (L"macintosh", 0);
			addNumericVariable (L"windows", 0);
			addNumericVariable (L"unix", 0);
		#endif
		addNumericVariable (L"left", 1);   // to accommodate scripts from before Praat 5.2.06
		addNumericVariable (L"right", 2);   // to accommodate scripts from before Praat 5.2.06
		addNumericVariable (L"mono", 1);   // to accommodate scripts from before Praat 5.2.06
		addNumericVariable (L"stereo", 2);   // to accommodate scripts from before Praat 5.2.06
		addNumericVariable (L"all", 0);   // to accommodate scripts from before Praat 5.2.06
		addNumericVariable (L"average", 0);   // to accommodate scripts from before Praat 5.2.06
		#define xstr(s) str(s)
		#define str(s) #s
		addStringVariable (L"praatVersion$", L"" xstr(PRAAT_VERSION_STR));
		addNumericVariable (L"praatVersion", PRAAT_VERSION_NUM);
		/*
		* Execute commands.
		*/
		#define wordEnd(c)  (c == '\0' || c == ' ' || c == '\t')
		for (lineNumber = 1; lineNumber <= numberOfLines; lineNumber ++) {
			if (_stopped) goto end2;
			int c0, fail = FALSE;
			wchar_t *p;
			MelderString_copy (& command2, lines [lineNumber]);
			c0 = command2. string [0];
			if (c0 == '\0') continue;
			/*
			* Substitute variables.
			*/
			for (p = & command2. string [0]; *p !='\0'; p ++) if (*p == '\'') {
				/*
				* Found a left quote. Search for a matching right quote.
				*/
				wchar_t *q = p + 1, varName [300], *r, *s, *colon;
				int precision = -1, percent = FALSE;
				while (*q != '\0' && *q != '\'' && q - p < 299) q ++;
				if (*q == '\0') break;   /* No matching right quote: done with this line. */
				if (q - p == 1 || q - p >= 299) continue;   /* Ignore empty variable names. */
				/*
				* Found a right quote. Get potential variable name.
				*/
				for (r = p + 1, s = varName; q - r > 0; r ++, s ++) *s = *r;
				*s = '\0';   /* Trailing null byte. */
				colon = wcschr (varName, ':');
				if (colon) {
					precision = wcstol (colon + 1, NULL, 10);
					if (wcschr (colon + 1, '%')) percent = TRUE;
					*colon = '\0';
				}
				InterpreterVariable *var = hasVariable (varName);
				if (var) {
					/*
					* Found a variable (p points to the left quote, q to the right quote). Substitute.
					*/
					int headlen = p - command2.string;
					const wchar_t *string = var -> _stringValue ? var -> _stringValue :
						percent ? Melder_percent (var -> _numericValue, precision) :
						precision >= 0 ?  Melder_fixed (var -> _numericValue, precision) :
						Melder_double (var -> _numericValue);
					int arglen = wcslen (string);
					MelderString_ncopy (& buffer, command2.string, headlen);
					MelderString_append2 (& buffer, string, q + 1);
					MelderString_copy (& command2, buffer.string);   // This invalidates p!! (really bad bug 20070203)
					p = command2.string + headlen + arglen - 1;
				} else {
					p = q - 1;   /* Go to before next quote. */
				}
			}
			c0 = command2.string [0];   /* Resume in order to allow things like 'c$' = 5 */
			if ((c0 < 'a' || c0 > 'z') && ! (c0 == '.' && command2.string [1] >= 'a' && command2.string [1] <= 'z')) {
				praat_executeCommand (this, command2.string); cherror
			/*
			* Interpret control flow and variables.
			*/
			} else switch (c0) {
				case '.':
					fail = TRUE;
					break;
				case 'a':
					if (wcsnequ (command2.string, L"assert ", 7)) {
						double value;
						numericExpression (command2.string + 7, & value); cherror
						if (value == 0.0 || value == NUMundefined) {
							assertionFailed = TRUE;
							error6 (L"Script assertion fails in line ", Melder_integer (lineNumber),
								L" (", value ? L"undefined" : L"false", L"):\n   ", command2.string + 7)
						}
					} else if (wcsnequ (command2.string, L"asserterror ", 12)) {
						MelderString_copy (& assertErrorString, command2.string + 12);
						assertErrorLineNumber = lineNumber;
					} else fail = TRUE;
					break;
				case 'b':
					fail = TRUE;
					break;
				case 'c':
					if (wcsnequ (command2.string, L"call ", 5)) {
						wchar_t *p = command2.string + 5, *callName, *procName;
						long iline;
						int hasArguments, callLength;
						while (*p == ' ' || *p == '\t') p ++;
						callName = p;
						while (*p != '\0' && *p != ' ' && *p != '\t') p ++;
						if (p == callName) error1 (L"Missing procedure name after 'call'.")
						hasArguments = *p != '\0';
						*p = '\0';   /* Close procedure name. */
						callLength = wcslen (callName);
						for (iline = 1; iline <= numberOfLines; iline ++) {
							wchar_t *linei = lines [iline], *q;
							int hasParameters;
							if (linei [0] != 'p' || linei [1] != 'r' || linei [2] != 'o' || linei [3] != 'c' ||
							    linei [4] != 'e' || linei [5] != 'd' || linei [6] != 'u' || linei [7] != 'r' ||
							    linei [8] != 'e' || linei [9] != ' ') continue;
							q = lines [iline] + 10;
							while (*q == ' ' || *q == '\t') q ++;
							procName = q;
							while (*q != '\0' && *q != ' ' && *q != '\t') q ++;
							if (q == procName) error1 (L"Missing procedure name after 'procedure'.")
							hasParameters = *q != '\0';
							if (q - procName == callLength && wcsnequ (procName, callName, callLength)) {
								if (hasArguments && ! hasParameters)
									error3 (L"Call to procedure \"", callName, L"\" has too many arguments.")
								if (hasParameters && ! hasArguments)
									error3 (L"Call to procedure \"", callName, L"\" has too few arguments.")
								if (++ _callDepth > Interpreter_MAX_CALL_DEPTH)
									error3 (L"Call depth greater than ", Melder_integer (Interpreter_MAX_CALL_DEPTH), L".")
								wcscpy (_procedureNames [_callDepth], callName);
								if (hasParameters) {
									++ p;   /* First argument. */
									++ q;   /* First parameter. */
									while (*q) {
										wchar_t *par, save;
										static MelderString arg = { 0 };
										MelderString_empty (& arg);
										while (*p == ' ' || *p == '\t') p ++;
										while (*q == ' ' || *q == '\t') q ++;
										par = q;
										while (*q != '\0' && *q != ' ' && *q != '\t') q ++;   /* Collect parameter name. */
										if (*q) {   /* Does anything follow the parameter name? */
											if (*p == '\"') {
												p ++;   /* Skip initial quote. */
												while (*p != '\0') {
													if (*p == '\"') {   /* Quote signals end-of-string or string-internal quote. */
														if (p [1] == '\"') {   /* Double quote signals string-internal quote. */
															MelderString_appendCharacter (& arg, '\"');
															p += 2;   /* Skip second quote. */
														} else {   /* Single quote signals end-of-string. */
															break;
														}
													} else {
														MelderString_appendCharacter (& arg, *p ++);
													}
												}
											} else {
												while (*p != '\0' && *p != ' ' && *p != '\t')
													MelderString_appendCharacter (& arg, *p ++);   /* White space separates. */
											}
											if (*p) { *p = '\0'; p ++; }
										} else {   /* Else rest of line. */
											while (*p != '\0')
												MelderString_appendCharacter (& arg, *p ++);
										}
										if (q [-1] == '$') {
											save = *q; *q = '\0';
											InterpreterVariable *var = lookUpVariable (par); *q = save; cherror
											Melder_free (var -> _stringValue);
											var -> _stringValue = Melder_wcsdup_f (arg.string);
										} else {
											double value;
											_callDepth --;
											numericExpression (arg.string, & value);
											_callDepth ++;
											save = *q; *q = '\0'; 
											InterpreterVariable *var = lookUpVariable (par); *q = save; cherror
											var -> _numericValue = value;
										}
									}
								}
								if (callDepth == Interpreter_MAX_CALL_DEPTH)
									error3 (L"Call depth greater than ", Melder_integer (Interpreter_MAX_CALL_DEPTH), L".")
								callStack [++ callDepth] = lineNumber;
								lineNumber = iline;
								break;
							}
						}
						if (iline > numberOfLines) error3 (L"Procedure \"", callName, L"\" not found.")
					} else fail = TRUE;
					break;
				case 'd':
					if (wcsnequ (command2.string, L"dec ", 4)) {
						InterpreterVariable *var = lookUpVariable (command2.string + 4); cherror
						var -> _numericValue -= 1.0;
					} else fail = TRUE;
					break;
				case 'e':
					if (command2.string [1] == 'n' && command2.string [2] == 'd') {
						if (wcsnequ (command2.string, L"endif", 5) && wordEnd (command2.string [5])) {
							/* Ignore. */
						} else if (wcsnequ (command2.string, L"endfor", 6) && wordEnd (command2.string [6])) {
							int depth = 0;
							long iline;
							for (iline = lineNumber - 1; iline > 0; iline --) {
								wchar_t *line = lines [iline];
								if (line [0] == 'f' && line [1] == 'o' && line [2] == 'r' && line [3] == ' ') {
									if (depth == 0) { lineNumber = iline - 1; fromendfor = TRUE; break; }   /* Go before 'for'. */
									else depth --;
								} else if (wcsnequ (lines [iline], L"endfor", 6) && wordEnd (lines [iline] [6])) {
									depth ++;
								}
							}
							if (iline <= 0) error1 (L"Unmatched 'endfor'.")
						} else if (wcsnequ (command2.string, L"endwhile", 8) && wordEnd (command2.string [8])) {
							int depth = 0;
							long iline;
							for (iline = lineNumber - 1; iline > 0; iline --) {
								if (wcsnequ (lines [iline], L"while ", 6)) {
									if (depth == 0) { lineNumber = iline - 1; break; }   /* Go before 'while'. */
									else depth --;
								} else if (wcsnequ (lines [iline], L"endwhile", 8) && wordEnd (lines [iline] [8])) {
									depth ++;
								}
							}
							if (iline <= 0) error1 (L"Unmatched 'endwhile'.")
						} else if (wcsnequ (command2.string, L"endproc", 7) && wordEnd (command2.string [7])) {
							if (callDepth == 0) error1 (L"Unmatched 'endproc'.")
							lineNumber = callStack [callDepth --];
							-- _callDepth;
						} else fail = TRUE;
					} else if (wcsnequ (command2.string, L"else", 4) && wordEnd (command2.string [4])) {
						int depth = 0;
						long iline;
						for (iline = lineNumber + 1; iline <= numberOfLines; iline ++) {
							if (wcsnequ (lines [iline], L"endif", 5) && wordEnd (lines [iline] [5])) {
								if (depth == 0) { lineNumber = iline; break; }   /* Go after 'endif'. */
								else depth --;
							} else if (wcsnequ (lines [iline], L"if ", 3)) {
								depth ++;
							}
						}
						if (iline > numberOfLines) error1 (L"Unmatched 'else'.")
					} else if (wcsnequ (command2.string, L"elsif ", 6) || wcsnequ (command2.string, L"elif ", 5)) {
						if (fromif) {
							double value;
							fromif = FALSE;
							numericExpression (command2.string + 5, & value); cherror
							if (value == 0.0) {
								int depth = 0;
								long iline;
								for (iline = lineNumber + 1; iline <= numberOfLines; iline ++) {
									if (wcsnequ (lines [iline], L"endif", 5) && wordEnd (lines [iline] [5])) {
										if (depth == 0) { lineNumber = iline; break; }   /* Go after 'endif'. */
										else depth --;
									} else if (wcsnequ (lines [iline], L"else", 4) && wordEnd (lines [iline] [4])) {
										if (depth == 0) { lineNumber = iline; break; }   /* Go after 'else'. */
									} else if ((wcsnequ (lines [iline], L"elsif", 5) && wordEnd (lines [iline] [5]))
										|| (wcsnequ (lines [iline], L"elif", 4) && wordEnd (lines [iline] [4]))) {
										if (depth == 0) { lineNumber = iline - 1; fromif = TRUE; break; }   /* Go at next 'elsif' or 'elif'. */
									} else if (wcsnequ (lines [iline], L"if ", 3)) {
										depth ++;
									}
								}
								if (iline > numberOfLines) error1 (L"Unmatched 'elsif'.")
							}
						} else {
							int depth = 0;
							long iline;
							for (iline = lineNumber + 1; iline <= numberOfLines; iline ++) {
								if (wcsnequ (lines [iline], L"endif", 5) && wordEnd (lines [iline] [5])) {
									if (depth == 0) { lineNumber = iline; break; }   /* Go after 'endif'. */
									else depth --;
								} else if (wcsnequ (lines [iline], L"if ", 3)) {
									depth ++;
								}
							}
							if (iline > numberOfLines) error1 (L"'elsif' not matched with 'endif'.")
						}
					} else if (wcsnequ (command2.string, L"exit", 4)) {
						if (command2.string [4] == '\0') {
							lineNumber = numberOfLines;   /* Go after end. */
						} else {
							error1 (command2.string + 5)
						}
					} else if (wcsnequ (command2.string, L"echo ", 5)) {
						/*
						* Make sure that lines like "echo = 3" will not be regarded as assignments.
						*/
						praat_executeCommand (this, command2.string); cherror
					} else fail = TRUE;
					break;
				case 'f':
					if (command2.string [1] == 'o' && command2.string [2] == 'r' && command2.string [3] == ' ') {   /* for_ */
						double toValue, loopVariable;
						wchar_t *frompos = wcsstr (command2.string, L" from "), *topos = wcsstr (command2.string, L" to ");
						wchar_t *varpos = command2.string + 4, *endvar = frompos;
						if (! topos) error1 (L"Missing \'to\' in \'for\' loop.")
						if (! endvar) endvar = topos;
						while (*endvar == ' ') { *endvar = '\0'; endvar --; }
						while (*varpos == ' ') varpos ++;
						if (endvar - varpos < 0) error1 (L"Missing loop variable after \'for\'.")
						InterpreterVariable *var = lookUpVariable (varpos);
						numericExpression (topos + 4, & toValue); cherror
						if (fromendfor) {
							fromendfor = FALSE;
							loopVariable = var -> _numericValue + 1.0;
						} else if (frompos) {
							*topos = '\0';
							numericExpression (frompos + 6, & loopVariable); cherror
						} else {
							loopVariable = 1.0;
						}
						var -> _numericValue = loopVariable;
						if (loopVariable > toValue) {
							int depth = 0;
							long iline;
							for (iline = lineNumber + 1; iline <= numberOfLines; iline ++) {
								if (wcsnequ (lines [iline], L"endfor", 6)) {
									if (depth == 0) { lineNumber = iline; break; }   /* Go after 'endfor'. */
									else depth --;
								} else if (wcsnequ (lines [iline], L"for ", 4)) {
									depth ++;
								}
							}
							if (iline > numberOfLines) error1 (L"Unmatched 'for'.")
						}
					} else if (wcsnequ (command2.string, L"form ", 5)) {
						long iline;
						for (iline = lineNumber + 1; iline <= numberOfLines; iline ++)
							if (wcsnequ (lines [iline], L"endform", 7))
								{ lineNumber = iline; break; }   /* Go after 'endform'. */
						if (iline > numberOfLines) error1 (L"Unmatched 'form'.")
					} else fail = TRUE;
					break;
				case 'g':
					if (wcsnequ (command2.string, L"goto ", 5)) {
						wchar_t labelName [50], *space;
						int dojump = TRUE, ilabel;
						swprintf (labelName, 50, L"%.47ls", command2.string + 5);
						space = wcschr (labelName, ' ');
						if (space == labelName) error1 (L"Missing label name after 'goto'.")
						if (space) {
							double value;
							*space = '\0';
							numericExpression (command2.string + 6 + wcslen (labelName), & value); cherror
							if (value == 0.0) dojump = FALSE;
						}
						if (dojump) {
							ilabel = lookupLabel (labelName); cherror
							lineNumber = _labelLines [ilabel];   /* Loop will add 1. */
						}
					} else fail = TRUE;
					break;
				case 'h':
					fail = TRUE;
					break;
				case 'i':
					if (command2.string [1] == 'f' && command2.string [2] == ' ') {   /* if_ */
						double value;
						numericExpression (command2.string + 3, & value); cherror
						if (value == 0.0) {
							int depth = 0;
							long iline;
							for (iline = lineNumber + 1; iline <= numberOfLines; iline ++) {
								if (wcsnequ (lines [iline], L"endif", 5)) {
									if (depth == 0) { lineNumber = iline; break; }   /* Go after 'endif'. */
									else depth --;
								} else if (wcsnequ (lines [iline], L"else", 4)) {
									if (depth == 0) { lineNumber = iline; break; }   /* Go after 'else'. */
								} else if (wcsnequ (lines [iline], L"elsif ", 6) || wcsnequ (lines [iline], L"elif ", 5)) {
									if (depth == 0) { lineNumber = iline - 1; fromif = TRUE; break; }   /* Go at 'elsif'. */
								} else if (wcsnequ (lines [iline], L"if ", 3)) {
									depth ++;
								}
							}
							if (iline > numberOfLines) error1 (L"Unmatched 'if'.")
						} else if (value == NUMundefined) {
							error1 (L"The value of the 'if' condition is undefined.")
						}
					} else if (wcsnequ (command2.string, L"inc ", 4)) {
						InterpreterVariable *var = lookUpVariable (command2.string + 4); cherror
						var -> _numericValue += 1.0;
					} else fail = TRUE;
					break;
				case 'j':
					fail = TRUE;
					break;
				case 'k':
					fail = TRUE;
					break;
				case 'l':
					if (wcsnequ (command2.string, L"label ", 6)) {
						;   /* Ignore labels. */
					} else fail = TRUE;
					break;
				case 'm':
					fail = TRUE;
					break;
				case 'n':
					fail = TRUE;
					break;
				case 'o':
					fail = TRUE;
					break;
				case 'p':
					if (wcsnequ (command2.string, L"procedure ", 10)) {
						long iline = lineNumber + 1;
						for (; iline <= numberOfLines; iline ++) {
							if (wcsnequ (lines [iline], L"endproc", 7) && wordEnd (lines [iline] [7])) {
								lineNumber = iline;
								break;
							}   /* Go after 'endproc'. */
						}
						if (iline > numberOfLines) error1 (L"Unmatched 'proc'.")
					} else if (wcsnequ (command2.string, L"print", 5)) {
						/*
						* Make sure that lines like "print = 3" will not be regarded as assingments.
						*/
						if (command2.string [5] == ' ' || (wcsnequ (command2.string + 5, L"line", 4) && (command2.string [9] == ' ' || command2.string [9] == '\0'))) {
							praat_executeCommand (this, command2.string); cherror
						} else fail = TRUE;
					} else fail = TRUE;
					break;
				case 'q':
					fail = TRUE;
					break;
				case 'r':
					if (wcsnequ (command2.string, L"repeat", 6) && wordEnd (command2.string [6])) {
						/* Ignore. */
					} else fail = TRUE;
					break;
				case 's':
					if (wcsnequ (command2.string, L"stopwatch", 9) && wordEnd (command2.string [9])) {
						(void) Melder_stopwatch ();   /* Reset stopwatch. */
					} else fail = TRUE;
					break;
				case 't':
					fail = TRUE;
					break;
				case 'u':
					if (wcsnequ (command2.string, L"until ", 6)) {
						double value;
						numericExpression (command2.string + 6, & value); cherror
						if (value == 0.0) {
							int depth = 0;
							long iline;
							for (iline = lineNumber - 1; iline > 0; iline --) {
								if (wcsnequ (lines [iline], L"repeat", 6) && wordEnd (lines [iline] [6])) {
									if (depth == 0) { lineNumber = iline; break; }   /* Go after 'repeat'. */
									else depth --;
								} else if (wcsnequ (lines [iline], L"until ", 6)) {
									depth ++;
								}
							}
							if (iline <= 0) error1 (L"Unmatched 'until'.")
						}
					} else fail = TRUE;
					break;
				case 'v':
					fail = TRUE;
					break;
				case 'w':
					if (wcsnequ (command2.string, L"while ", 6)) {
						double value;
						numericExpression (command2.string + 6, & value); cherror
						if (value == 0.0) {
							int depth = 0;
							long iline;
							for (iline = lineNumber + 1; iline <= numberOfLines; iline ++) {
								if (wcsnequ (lines [iline], L"endwhile", 8) && wordEnd (lines [iline] [8])) {
									if (depth == 0) { lineNumber = iline; break; }   /* Go after 'endwhile'. */
									else depth --;
								} else if (wcsnequ (lines [iline], L"while ", 6)) {
									depth ++;
								}
							}
							if (iline > numberOfLines) error1 (L"Unmatched 'while'.")
						}
					} else fail = TRUE;
					break;
				case 'x':
					fail = TRUE;
					break;
				case 'y':
					fail = TRUE;
					break;
				case 'z':
					fail = TRUE;
					break;
				default: break;
			}
			if (fail) {
				/*
				* Found an unknown word starting with a lower-case letter, optionally preceded by a period.
				* See whether the word is a variable name.
				*/
				wchar_t *p = & command2.string [0];
				/*
				* Variable names consist of a sequence of letters, digits, and underscores,
				* optionally preceded by a period and optionally followed by a $ and/or #.
				*/
				if (*p == '.') p ++;
				while (isalnum (*p) || *p == '_' || *p == '.')  p ++;
				if (*p == '$') {
					/*
					* Assign to a string variable.
					*/
					wchar_t *endOfVariable = ++ p;
					wchar_t *variableName = command2.string;
					int withFile;
					while (*p == ' ' || *p == '\t') p ++;   /* Go to first token after variable name. */
					if (*p == '[') {
						/*
						* This must be an assignment to an indexed string variable.
						*/
						*endOfVariable = '\0';
						static MelderString indexedVariableName = { 0 };
						MelderString_copy (& indexedVariableName, command2.string);
						MelderString_appendCharacter (& indexedVariableName, '[');
						for (;;) {
							p ++;   // skip opening bracket or comma
							static MelderString index = { 0 };
							MelderString_empty (& index);
							int depth = 0;
							while ((depth > 0 || (*p != ',' && *p != ']')) && *p != '\n' && *p != '\0') {
								MelderString_appendCharacter (& index, *p);
								if (*p == '[') depth ++;
								else if (*p == ']') depth --;
								p ++;
							}
							if (*p == '\n' || *p == '\0')
								error1 (L"Missing closing bracket (]) in indexed variable.")
							double numericIndexValue;
							numericExpression (index.string, & numericIndexValue); cherror
							MelderString_append (& indexedVariableName, Melder_double (numericIndexValue));
							MelderString_appendCharacter (& indexedVariableName, *p);
							if (*p == ']') {
								break;
							}
						}
						variableName = indexedVariableName.string;
						p ++;   // skip closing bracket
					}
					while (*p == ' ' || *p == '\t') p ++;   /* Go to first token after (perhaps indexed) variable name. */
					if (*p == '=') {
						withFile = 0;   /* Assignment. */
					} else if (*p == '<') {
						withFile = 1;   /* Read from file. */
					} else if (*p == '>') {
						if (p [1] == '>')
							withFile = 2, p ++;   /* Append to file. */
						else
							withFile = 3;   /* Save to file. */
					} else error3 (L"Missing '=', '<', or '>' after variable ", variableName, L".")
					*endOfVariable = '\0';
					p ++;
					while (*p == ' ' || *p == '\t') p ++;   /* Go to first token after assignment or I/O symbol. */
					if (*p == '\0') {
						if (withFile != 0)
							error3 (L"Missing file name after variable ", variableName, L".")
						else
							error3 (L"Missing expression after variable ", variableName, L".")
					}
					if (withFile) {
						structMelderFile file = { 0 };
						Melder_relativePathToFile (p, & file); cherror
						if (withFile == 1) {
							wchar_t *stringValue = MelderFile_readText (& file); cherror
							InterpreterVariable *var = lookUpVariable (variableName); cherror
							Melder_free (var -> _stringValue);
							var -> _stringValue = stringValue;   /* var becomes owner */
						} else if (withFile == 2) {
							if (theCurrentPraatObjects != & theForegroundPraatObjects) error1 (L"Commands that write to a file are not available inside pictures.")
							InterpreterVariable *var = hasVariable (variableName); cherror
							if (! var) error3 (L"Variable ", variableName, L" undefined.")
							MelderFile_appendText (& file, var -> _stringValue); cherror
						} else {
							if (theCurrentPraatObjects != & theForegroundPraatObjects) error1 (L"Commands that write to a file are not available inside pictures.")
							InterpreterVariable *var = hasVariable (variableName); cherror
							if (! var) error3 (L"Variable ", variableName, L" undefined.")
							MelderFile_writeText (& file, var -> _stringValue); cherror
						}
					} else if (isCommand (p)) {
						/*
						* Example: name$ = Get name
						*/
						MelderString_empty (& valueString);   // empty because command may print nothing; also makes sure that valueString.string exists
						Melder_divertInfo (& valueString);
						praat_executeCommand (this, p);
						Melder_divertInfo (NULL); cherror
						InterpreterVariable *var = lookUpVariable (variableName); cherror
						Melder_free (var -> _stringValue);
						var -> _stringValue = Melder_wcsdup_e (valueString.string); cherror
					} else {
						/*
						* Evaluate a string expression and assign the result to the variable.
						* Examples:
						*    sentence$ = subject$ + verb$ + object$
						*    extension$ = if index (file$, ".") <> 0
						*       ... then right$ (file$, length (file$) - rindex (file$, "."))
						*       ... else "" fi
						*/
						wchar_t *stringValue;
						stringExpression (p, & stringValue); cherror
						InterpreterVariable *var = lookUpVariable (variableName); cherror
						Melder_free (var -> _stringValue);
						var -> _stringValue = stringValue;   /* var becomes owner */
					}
				} else if (*p == '#') {
					/*
					* Assign to a numeric array variable.
					*/
					wchar_t *endOfVariable = ++ p;
					while (*p == ' ' || *p == '\t') p ++;   // Go to first token after variable name.
					if (*p == '=') {
						;
					} else error3 (L"Missing '=' after variable ", command2.string, L".")
					*endOfVariable = '\0';
					p ++;
					while (*p == ' ' || *p == '\t') p ++;   // Go to first token after assignment or I/O symbol.
					if (*p == '\0') {
						error3 (L"Missing expression after variable ", command2.string, L".")
					}
					struct Formula_NumericArray value;
					numericArrayExpression (p, & value); cherror
					InterpreterVariable *var = lookUpVariable (command2.string); cherror
					NUMdmatrix_free (var -> _numericArrayValue. data, 1, 1);
					var -> _numericArrayValue = value;
				} else {
					/*
					* Try to assign to a numeric variable.
					*/
					double value;
					wchar_t *variableName = command2.string;
					int typeOfAssignment = 0;   /* Plain assignment. */
					if (*p == '\0') {
						/*
						* Command ends here: it may be a PraatShell command.
						*/
						praat_executeCommand (this, command2.string); cherror
						goto end;
					}
					wchar_t *endOfVariable = p;
					while (*p == ' ' || *p == '\t') p ++;
					if (*p == '=' || ((*p == '+' || *p == '-' || *p == '*' || *p == '/') && p [1] == '=')) {
						/*
						* This must be an assignment (though: "echo = ..." ???)
						*/
						typeOfAssignment = *p == '+' ? 1 : *p == '-' ? 2 : *p == '*' ? 3 : *p == '/' ? 4 : 0;
						*endOfVariable = '\0';   // Close variable name. FIXME: this can be any weird character, e.g. hallo&
					} else if (*p == '[') {
						/*
						* This must be an assignment to an indexed numeric variable.
						*/
						*endOfVariable = '\0';
						static MelderString indexedVariableName = { 0 };
						MelderString_copy (& indexedVariableName, command2.string);
						MelderString_appendCharacter (& indexedVariableName, '[');
						for (;;) {
							p ++;   // skip opening bracket or comma
							static MelderString index = { 0 };
							MelderString_empty (& index);
							int depth = 0;
							while ((depth > 0 || (*p != ',' && *p != ']')) && *p != '\n' && *p != '\0') {
								MelderString_appendCharacter (& index, *p);
								if (*p == '[') depth ++;
								else if (*p == ']') depth --;
								p ++;
							}
							if (*p == '\n' || *p == '\0')
								error1 (L"Missing closing bracket (]) in indexed variable.")
							numericExpression (index.string, & value); cherror
							MelderString_append (& indexedVariableName, Melder_double (value));
							MelderString_appendCharacter (& indexedVariableName, *p);
							if (*p == ']') {
								break;
							}
						}
						variableName = indexedVariableName.string;
						p ++;   // skip closing bracket
						while (*p == ' ' || *p == '\t') p ++;
						if (*p == '=' || ((*p == '+' || *p == '-' || *p == '*' || *p == '/') && p [1] == '=')) {
							typeOfAssignment = *p == '+' ? 1 : *p == '-' ? 2 : *p == '*' ? 3 : *p == '/' ? 4 : 0;
						}
					} else {
						/*
						* Not an assignment: perhaps a PraatShell command (select, echo, execute, pause ...).
						*/
						praat_executeCommand (this, variableName); cherror
						goto end;
					}
					p += typeOfAssignment == 0 ? 1 : 2;
					while (*p == ' ' || *p == '\t') p ++;			
					if (*p == '\0') error3 (L"Missing expression after variable ", variableName, L".")
					/*
					* Three classes of assignments:
					*    var = formula
					*    var = Query
					*    var = Object creation
					*/
					if (isCommand (p)) {
						/*
						* Get the value of the query.
						*/
						MelderString_empty (& valueString);
						Melder_divertInfo (& valueString);
						MelderString_appendCharacter (& valueString, 1);
						praat_executeCommand (this, p);
						if (valueString.string [0] == 1) {
							int IOBJECT, result = 0, found = 0;
							WHERE (SELECTED) { result = IOBJECT; found += 1; }
							if (found > 1) {
								Melder_divertInfo (NULL);
								error1 (L"Multiple objects selected. Cannot assign ID to variable.")
							} else if (found == 0) {
								Melder_divertInfo (NULL);
								error1 (L"No objects selected. Cannot assign ID to variable.")
							} else {
								value = theCurrentPraatObjects -> list [result]. id;
							}
						} else {
							value = Melder_atof (valueString.string);   /* Including --undefined-- */
						}
						Melder_divertInfo (NULL); cherror
					} else {
						/*
						* Get the value of the formula.
						*/
						numericExpression (p, & value); cherror
					}
					/*
					* Assign the value to a variable.
					*/
					if (typeOfAssignment == 0) {
						/*
						* Use an existing variable, or create a new one.
						*/
						//Melder_casual ("looking up variable %ls", variableName);
						InterpreterVariable *var = lookUpVariable (variableName); cherror
						var -> _numericValue = value;
					} else {
						/*
						* Modify an existing variable.
						*/
						InterpreterVariable *var = hasVariable (variableName); cherror
						if (var == NULL) error3 (L"Unknown variable ", variableName, L".")
						if (var -> _numericValue == NUMundefined) {
							/* Keep it that way. */
						} else {
							if (typeOfAssignment == 1) {
								var -> _numericValue += value;
							} else if (typeOfAssignment == 2) {
								var -> _numericValue -= value;
							} else if (typeOfAssignment == 3) {
								var -> _numericValue *= value;
							} else if (value == 0) {
								var -> _numericValue = NUMundefined;
							} else {
								var -> _numericValue /= value;
							}
						}
					}
				}
			} // endif fail
		}
	}
end:
	if (assertErrorLineNumber == 0) {
		iferror goto end2;
	} else if (assertErrorLineNumber != lineNumber) {
		if (/*assertErrorLineNumber != lineNumber - 1 ||*/ ! Melder_hasError ()) {
			Melder_error5 (L"Script assertion fails in line ", Melder_integer (assertErrorLineNumber),
				L": error " L_LEFT_GUILLEMET L" ", assertErrorString.string, L" " L_RIGHT_GUILLEMET L" not raised. Instead: no error.");
			goto end2;
		}
		if (wcsstr (Melder_getError (), assertErrorString.string)) {
			Melder_clearError ();
			assertErrorLineNumber = 0;
		} else {
			wchar_t *errorCopy = Melder_wcsdup_f (Melder_getError ());
			Melder_clearError ();
			Melder_error6 (L"Script assertion fails in line ", Melder_integer (assertErrorLineNumber),
				L": error " L_LEFT_GUILLEMET L" ", assertErrorString.string, L" " L_RIGHT_GUILLEMET L" not raised. Instead:\n",
				errorCopy);
			Melder_free (errorCopy);
			goto end2;
		}
	}
end2:
	iferror {
		if (! wcsnequ (lines [lineNumber], L"exit ", 5) && ! assertionFailed) {   /* Don't show the message twice! */
			while (lines [lineNumber] [0] == '\0') {   /* Did this use to be a continuation line? */
				lineNumber --;
				Melder_assert (lineNumber > 0);   /* Originally empty lines that stayed empty should not generate errors. */
			}
			Melder_error5 (L"Script line ", Melder_integer (lineNumber), L" not performed or completed:\n" L_LEFT_GUILLEMET L" ",
				lines [lineNumber], L" " L_RIGHT_GUILLEMET);
		}
	}
	NUMpvector_free (lines, 1);
	MelderString_free (& command2);
	MelderString_free (& buffer);
	_numberOfLabels = 0;
	_running = false;
	_stopped = false;
	iferror return 0;
	return 1;
}

void Interpreter::stop () {
//Melder_casual ("Interpreter_stop in: %ld", me);
	_stopped = true;
//Melder_casual ("Interpreter_stop out: %ld", me);
}

int Melder_includeIncludeFiles (wchar_t **text) {
	int depth;
	for (depth = 0; ; depth ++) {
		wchar_t *head = *text;
		long numberOfIncludes = 0;
		if (depth > 10)
			return Melder_error1 (L"Include files nested too deep. Probably cyclic.");
		for (;;) {
			wchar_t *includeLocation, *includeFileName, *includeText, *tail, *newText;
			long headLength, includeTextLength, newLength;
			/*
				Look for an include statement. If not found, we have finished.
			 */
			includeLocation = wcsnequ (head, L"include ", 8) ? head : wcsstr (head, L"\ninclude ");
			if (includeLocation == NULL) break;
			if (includeLocation != head) includeLocation += 1;
			numberOfIncludes += 1;
			/*
				Separate out the head.
			 */
			*includeLocation = '\0';
			/*
				Separate out the name of the include file.
			 */
			includeFileName = includeLocation + 8;
			while (*includeFileName == ' ' || *includeFileName == '\t') includeFileName ++;
			tail = includeFileName;
			while (*tail != '\n' && *tail != '\0') tail ++;
			if (*tail == '\n') {
				*tail = '\0';
				tail += 1;
			}
			/*
				Get the contents of the include file.
			 */
			structMelderFile includeFile = { 0 };
			if (! Melder_relativePathToFile (includeFileName, & includeFile)) return 0;
			includeText = MelderFile_readText (& includeFile);
			if (! includeText) error3 (L"Include file ", MelderFile_messageName (& includeFile), L" not read.")
			/*
				Construct the new text.
			 */
			headLength = (head - *text) + wcslen (head);
			includeTextLength = wcslen (includeText);
			newLength = headLength + includeTextLength + 1 + wcslen (tail);
			newText = Melder_malloc_e (wchar_t, newLength + 1);
			if (! newText) { Melder_free (includeText); cherror }
			wcscpy (newText, *text);
			wcscpy (newText + headLength, includeText);
			wcscpy (newText + headLength + includeTextLength, L"\n");
			wcscpy (newText + headLength + includeTextLength + 1, tail);
			/*
				Replace the old text with the new.
			 */
			Melder_free (*text);
			*text = newText;
			/*
				Clean up.
			 */
			Melder_free (includeText);
			/*
				Cycle.
			 */
			head = *text + headLength + includeTextLength + 1;
		}
		if (numberOfIncludes == 0) break;
	}
end:
	iferror return 0;
	return 1;
}

/* End of file Interpreter.c */
