#ifndef _Interpreter_h_
#define _Interpreter_h_
/* Interpreter.h
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
 * pb 2011/03/02
 */

#ifndef _Collection_h_
	#include "Collection.h"
#endif
#ifndef _Gui_h_
	#include "Gui.h"
#endif
#ifndef _Formula_h_
	#include "Formula.h"
#endif

#ifdef __cplusplus
class InterpreterVariable {
  public:
	InterpreterVariable (const wchar_t *key);
	~InterpreterVariable();

	wchar_t *_key, *_stringValue;
	double _numericValue;
	struct Formula_NumericArray _numericArrayValue;
};

#define Interpreter_MAXNUM_PARAMETERS  400
#define Interpreter_MAXNUM_LABELS  1000
#define Interpreter_MAX_CALL_DEPTH  50

class UiForm;

class Interpreter { // : protected Thing
  public:
	Interpreter (wchar_t *environmentName);
	~Interpreter();

	int readParameters (wchar_t *text);
	UiForm * createForm (GuiObject parent, const wchar_t *fileName, int (*okCallback) (UiForm *sendingForm, const wchar_t *sendingString, Interpreter *interpreter, const wchar_t *invokingButtonTitle, bool modified, void *closure), void *okClosure);
	int getArgumentsFromDialog (UiForm *dialog);
	int getArgumentsFromString (const wchar_t *arguments);
	int run (wchar_t *text);   /* Destroys 'text'. */
	void stop ();   // Can be called from any procedure called deep-down by the interpreter. Will stop before next line.

	int voidExpression (const wchar_t *expression);
	int numericExpression (const wchar_t *expression, double *value);
	int stringExpression (const wchar_t *expression, wchar_t **value);
	int numericArrayExpression (const wchar_t *expression, struct Formula_NumericArray *value);
	int anyExpression (const wchar_t *expression, struct Formula_Result *result);

	int addNumericVariable (const wchar_t *key, double value);
	InterpreterVariable* addStringVariable (const wchar_t *key, const wchar_t *value);

	InterpreterVariable* hasVariable (const wchar_t *key);
	InterpreterVariable* lookUpVariable (const wchar_t *key);

	wchar_t *_environmentName;
	int _numberOfParameters, _numberOfLabels, _callDepth;
	wchar_t _parameters [1+Interpreter_MAXNUM_PARAMETERS] [100];
	unsigned char _types [1+Interpreter_MAXNUM_PARAMETERS];
	wchar_t *_arguments [1+Interpreter_MAXNUM_PARAMETERS];
	wchar_t _choiceArguments [1+Interpreter_MAXNUM_PARAMETERS] [100];
	wchar_t _labelNames [1+Interpreter_MAXNUM_LABELS] [100];
	long _labelLines [1+Interpreter_MAXNUM_LABELS];
	wchar_t _dialogTitle [1+100], _procedureNames [1+Interpreter_MAX_CALL_DEPTH] [100];
	SortedSetOfString _variables;
	bool _running, _stopped;

  private:
	static bool isCommand (const wchar_t *p);

	long lookupLabel (const wchar_t *labelName);
	void parameterToVariable (int type, const wchar_t *in_parameter, int ipar);
};
#else
typedef struct Interpreter Interpreter;
#endif

#ifdef __cplusplus
	extern "C" {
#endif

int Melder_includeIncludeFiles (wchar_t **text);

int Formula_compile (Interpreter *interpreter, Any data, const wchar_t *expression, int expressionType, int optimize);

int Interpreter_numericExpression_FIXME (const wchar_t *expression, double *value);

#ifdef __cplusplus
	}
#endif

/* End of file Interpreter.h */
#endif
