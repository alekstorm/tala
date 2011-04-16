#ifndef _UiForm_h_
#define _UiForm_h_
/* UiForm.h
 *
 * Copyright (C) 1992-2011 Alek Storm
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

#include "Editor.h"
#include "Interpreter.h"
#include "UiHistory.h"

#define MAXIMUM_NUMBER_OF_FIELDS  50
#define MAXIMUM_NUMBER_OF_CONTINUE_BUTTONS  10

/* Values for 'type'. */
#define UI_REAL  1
#define UI_REAL_OR_UNDEFINED  2
#define UI_POSITIVE  3
#define UI_INTEGER  4
#define UI_NATURAL  5
#define UI_WORD  6
#define UI_SENTENCE  7
#define UI_COLOUR  8
#define UI_CHANNEL  9
	#define UI_LABELLEDTEXT_MIN  UI_REAL
	#define UI_LABELLEDTEXT_MAX  UI_CHANNEL
#define UI_LABEL  10
#define UI_TEXT  11
#define UI_BOOLEAN  12
#define UI_RADIO  13
#define UI_OPTIONMENU  14
#define UI_LIST  15

/* Forms for getting arguments from the user. */

/* Example of usage:
{
	static Any dia = NULL;
	if (dia == NULL) {
		Any radio;
		dia = create
		  (topShell,   // The parent GuiObject of the dialog window.
			L"Create a new person",   // The window title.
			DO_Person_create,   // The routine to call when the user clicks OK.
			NULL,   // The last argument to the OK routine (also for the other buttons). Could be a ScriptEditor, or an EditorCommand, or an Interpreter, or NULL.
			L"Create person...",   // The invoking button title.
			L"Create person...");   // The help string; may be NULL.
		dia->addNatural (L"Age (years)", L"18");
		dia->addPositive (L"Length (metres)", L"1.68 (average)");
		dia->addBoolean (L"Beard", FALSE);
		radio = addRadio (L"Sex", 1);
			radio->addRadio (L"Female");
			radio->addRadio (L"Male");
		dia->addWord (L"Colour", L"black");
		dia->addLabel (L"features", L"Some less conspicuous features:");
		dia->addNatural (L"Number of birth marks", L"28");
		dia->addSentence (L"Favourite greeting", L"Good morning");
		dia->finish (dia);
	}
	dia->setReal (L"Length", myLength);
	dia->setInteger (L"Number of birth marks", 30);
	dia->do_ (0);   // Show dialog box.
}
	Real, Positive, Integer, Natural, Word, and Sentence
		show a label (name) and an editable text field (value).
	Radio shows a label (name) and has Button children.
	OptionMenu shows a label (name) and has Button children in a menu.
	Label only shows its value.
	Text only shows an editable text field (value).
	Boolean shows a labeled toggle button which is on (1) or off (0).
	Button does the same inside a radio box or option menu.
	List shows a scrollable list.
	Colour shows a label (name) and an editable text field for a grey value between 0 and 1, a colour name, ar {r,g,b}.
	Channel shows a label (name) and an editable text field for a natural number or the text Left or Right.
	As shown in the example, Real, Positive, Integer, Natural, and Word may contain extra text;
	this text is considered as comments and is erased as soon as you click OK.
	When you click "Standards", the standard values (including comments)
	are restored to all items in the form.
*/

#ifdef __cplusplus
class Interpreter;
class EditorCommand;

class UiForm {
  public:
	class UiOption {
	  public:
		UiOption (const wchar_t *label) {
			_name = Melder_wcsdup_f (label);
		}

		GuiObject _toggle;
		wchar_t *_name;
	};

	class UiField {
	  public:
		UiField(int type, const wchar_t *name);
		~UiField();

		UiOption * addRadio (const wchar_t *label);
		UiOption * addOptionMenu (const wchar_t *label);

		void setDefault();
		int widgetToValue();
		int stringToValue (const wchar_t *string, Interpreter *interpreter);

		int _type;
		const wchar_t *_formLabel;
		double _realValue, _realDefaultValue;
		long _integerValue, _integerDefaultValue;
		wchar_t *_stringValue; const wchar_t *_stringDefaultValue;
		Graphics_Colour _colourValue;
		char *_stringValueA;
		Ordered _options;
		long _numberOfStrings;
		const wchar_t **_strings;
		GuiObject _text, _toggle, _list, _cascadeButton;
		int _y;
		wchar_t *_name;

	  private:
		int colourToValue (wchar_t *string);
	};

	static UiForm * createE (EditorCommand *cmd, const wchar_t *title, const wchar_t *invokingButtonTitle, const wchar_t *helpTitle);
	static int parseStringE (EditorCommand *cmd, const wchar_t *arguments, Interpreter *interpreter);

	static UiHistory history;
	static int (*theAllowExecutionHookHint) (void *closure);
	static void *theAllowExecutionClosureHint;

/* The following routines work on the screen and from batch. */
	UiForm (GuiObject parent, const wchar_t *title,
		int (*okCallback) (UiForm *sendingForm, const wchar_t *sendingString, Interpreter *interpreter, const wchar_t *invokingButtonTitle, bool modified, void *closure), void *buttonClosure,
		const wchar_t *invokingButtonTitle, const wchar_t *helpTitle);
	~UiForm();

	UiField * addReal (const wchar_t *label, const wchar_t *defaultValue);
	UiField * addRealOrUndefined (const wchar_t *label, const wchar_t *defaultValue);
	UiField * addPositive (const wchar_t *label, const wchar_t *defaultValue);
	UiField * addInteger (const wchar_t *label, const wchar_t *defaultValue);
	UiField * addNatural (const wchar_t *label, const wchar_t *defaultValue);
	UiField * addWord (const wchar_t *label, const wchar_t *defaultValue);
	UiField * addSentence (const wchar_t *label, const wchar_t *defaultValue);
	UiField * addLabel (const wchar_t *name, const wchar_t *label);
	UiField * addBoolean (const wchar_t *label, int defaultValue);
	UiField * addText (const wchar_t *name, const wchar_t *defaultValue);
	UiField * addRadio (const wchar_t *label, int defaultValue);
	UiField * addOptionMenu (const wchar_t *label, int defaultValue);
	UiField * addList (const wchar_t *label, long numberOfStrings, const wchar_t **strings, long defaultValue);
	UiField * addColour (const wchar_t *label, const wchar_t *defaultValue);
	UiField * addChannel (const wchar_t *label, const wchar_t *defaultValue);
	void finish ();
	void setPauseForm (
		int numberOfContinueButtons, int defaultContinueButton, int cancelContinueButton,
		const wchar_t *continue1, const wchar_t *continue2, const wchar_t *continue3,
		const wchar_t *continue4, const wchar_t *continue5, const wchar_t *continue6,
		const wchar_t *continue7, const wchar_t *continue8, const wchar_t *continue9,
		const wchar_t *continue10,
		int (*cancelCallback) (Any dia, void *closure));

	/* The following three routines set values in widgets. */
	/* Do not call from batch. */
	/* 'fieldName' is name from addXXXXXX (), */
	/* without anything from and including the first " (" or ":". */
	void setString (const wchar_t *fieldName, const wchar_t *text);
		/* Real, Positive, Integer, Natural, Word, Sentence, Label, Text, Radio, List. */
	void setReal (const wchar_t *fieldName, double value);
		/* Real, Positive. */
	void setInteger (const wchar_t *fieldName, long value);
		/* Integer, Natural, Boolean, Radio, List. */

	void do_ (bool modified);
	/*
		Function:
			put the form on the screen.
		Behaviour:
			If the user clicks "OK",
			the form will call the okCallback that was registered with create ().
			   If the okCallback then returns 1, the form will disappear from the screen;
			if it returns 0, the form will stay on the screen; this can be used
			for enabling the user to repair mistakes in the form.

			If the user clicks "Apply",
			the form will call the okCallback that was registered with create (),
			and the form disappears from the screen.

			If the user clicks "Cancel", the form disappears from the screen.

			If the user clicks "Help", the form calls "help" with the "helpTitle"
			and stays on the screen.

			When the form disappears from the screen, the values in the fields
			will remain until the next invocation of do () for the same form.
		Arguments:
			the above behaviour describes the action when 'modified' is 0.
			If 'modified' is set, the user does not have to click OK.
			The form will still appear on the screen,
			but the okCallback will be called immediately.
	*/

	/* The 'okCallback' can use the following four routines to ask arguments. */
	/* The field names are the 'label' or 'name' arguments to addXXXXXX (), */
	/* without anything from parentheses or from a colon. */
	/* These routines work from the screen and from batch. */
	double getReal (const wchar_t *fieldName);	/* Real, Positive */
	long getInteger (const wchar_t *fieldName);	/* Integer, Natural, Boolean, Radio, List */
	wchar_t * getString (const wchar_t *fieldName);	/* Word, Sentence, Text, Radio, List */
	Graphics_Colour getColour (const wchar_t *fieldName);   /* Colour */
	MelderFile getFile (const wchar_t *fieldName); /* FileIn, FileOut */

	double getReal_check (const wchar_t *fieldName);
	long getInteger_check (const wchar_t *fieldName);
	wchar_t * getString_check (const wchar_t *fieldName);
	Graphics_Colour getColour_check (const wchar_t *fieldName);

	int parseString (const wchar_t *arguments, Interpreter *interpreter);

	int getClickedContinueButton ();
	int widgetsToValues ();
	void okOrApply (GuiObject button, int hide);
	int Interpreter_addVariables (Interpreter *interpreter);

	EditorCommand *_command;
	GuiObject _parent, _shell, _dialog;
	int (*_okCallback) (UiForm *sendingForm, const wchar_t *sendingString, Interpreter *interpreter, const wchar_t *invokingButtonTitle, bool modified, void *closure);
	int (*_applyCallback) (Any dia, void *closure);
	int (*_cancelCallback) (Any dia, void *closure);
	void *_buttonClosure;
	const wchar_t *_invokingButtonTitle, *_helpTitle;
	int _numberOfContinueButtons, _defaultContinueButton, _cancelContinueButton, _clickedContinueButton;
	const wchar_t *_continueTexts [1 + MAXIMUM_NUMBER_OF_CONTINUE_BUTTONS];
	int _numberOfFields;
	UiField *_field [1 + MAXIMUM_NUMBER_OF_FIELDS];
	GuiObject _okButton, _cancelButton, _revertButton, _helpButton, _applyButton, _continueButtons [1 + MAXIMUM_NUMBER_OF_CONTINUE_BUTTONS];
	bool _destroyWhenUnmanaged, _isPauseForm;
	int (*_allowExecutionHook) (void *closure);
	void *_allowExecutionClosure;
	wchar_t *_name;

  private:
	UiField * addField (int type, const wchar_t *label);
	UiField * findField (const wchar_t *fieldName);
	UiField * findField_check (const wchar_t *fieldName);
};
#else
typedef struct UiForm UiForm;
#endif
#endif
