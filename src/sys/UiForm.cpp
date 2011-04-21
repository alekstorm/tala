/* UiForm.cpp
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

#include <wctype.h>
#include "UiForm.h"
#include "machine.h"

UiHistory UiForm::history;
int (*UiForm::theAllowExecutionHookHint) (void *closure);
void *UiForm::theAllowExecutionClosureHint;

UiForm::UiForm (GuiObject parent, const wchar_t *title,
		int (*okCallback) (UiForm *sendingForm, const wchar_t *sendingString, Interpreter *interpreter, const wchar_t *invokingButtonTitle, bool modified, void *closure), void *buttonClosure,
		const wchar_t *invokingButtonTitle, const wchar_t *helpTitle)
	: _command(NULL),
	  _parent(parent),
	  _shell(NULL),
	  _dialog(NULL),
	  _okCallback(okCallback),
	  _applyCallback(NULL),
	  _cancelCallback(NULL),
	  _buttonClosure(buttonClosure),
	  _invokingButtonTitle(Melder_wcsdup_f (invokingButtonTitle)),
	  _helpTitle(Melder_wcsdup_f (helpTitle)),
	  _numberOfContinueButtons(0),
	  _defaultContinueButton(0),
	  _cancelContinueButton(0),
	  _clickedContinueButton(0),
	  _numberOfFields(0),
	  _okButton(NULL),
	  _cancelButton(NULL),
	  _revertButton(NULL),
	  _helpButton(NULL),
	  _applyButton(NULL),
	  _destroyWhenUnmanaged(false),
	  _isPauseForm(false),
	  _allowExecutionHook(NULL),
	  _allowExecutionClosure(NULL),
	  _name(Melder_wcsdup_f (title)) {}

UiForm::~UiForm () {
	Melder_free (_name);
	for (int ifield = 1; ifield <= _numberOfFields; ifield ++)
		forget (_field [ifield]);
	if (_dialog) {
		GuiObject_destroy (GuiObject_parent (_dialog));
	}
	Melder_free (_invokingButtonTitle);
	Melder_free (_helpTitle);
}

static void gui_button_cb_revert (void *data, GuiButtonEvent event) {
	(void) event;
	UiForm *form = (UiForm *)data;
	for (int ifield = 1; ifield <= form->_numberOfFields; ifield ++)
		form->_field[ifield]->setDefault ();
}

static void gui_dialog_cb_close (void *data) {
	UiForm *form = (UiForm *)data;
	if (form->_cancelCallback) form->_cancelCallback (form, form->_buttonClosure);
	GuiObject_hide (form->_dialog);
}

static void gui_button_cb_cancel (void *data, GuiButtonEvent event) {
	(void) event;
	UiForm *form = (UiForm *)data;
	if (form->_cancelCallback) form->_cancelCallback (form, form->_buttonClosure);
	GuiObject_hide (form->_dialog);
}

int UiForm::widgetsToValues () {
	for (int ifield = 1; ifield <= _numberOfFields; ifield ++) {
		if (! _field [ifield]->widgetToValue ()) {
			return Melder_error3 (L"Please correct command window " L_LEFT_DOUBLE_QUOTE, _name, L_RIGHT_DOUBLE_QUOTE L" or cancel.");
		}
	}
	return 1;
}

void UiForm::okOrApply (GuiObject button, int hide) {
	if (_allowExecutionHook && ! _allowExecutionHook (_allowExecutionClosure)) {
		Melder_error3 (L"Cannot execute command window " L_LEFT_DOUBLE_QUOTE, _name, L_RIGHT_DOUBLE_QUOTE L".");
		Melder_flushError (NULL);
		return;
	}
	if (! widgetsToValues ()) {
		Melder_flushError (NULL);
		return;
	}
	/* In the next, w must become _okButton? */
	/*XtRemoveCallback (w, XmNactivateCallback, ok, void_me);   /* FIX */
	if (_okButton) GuiObject_setSensitive (_okButton, false);
	for (int i = 1; i <= _numberOfContinueButtons; i ++) if (_continueButtons [i]) GuiObject_setSensitive (_continueButtons [i], false);
	if (_applyButton) GuiObject_setSensitive (_applyButton, false);
	if (_cancelButton) GuiObject_setSensitive (_cancelButton, false);
	if (_revertButton) GuiObject_setSensitive (_revertButton, false);
	if (_helpButton) GuiObject_setSensitive (_helpButton, false);
	#if motif
	XmUpdateDisplay (_dialog);
	#endif
	if (_isPauseForm) {
		for (int i = 1; i <= _numberOfContinueButtons; i ++) {
			if (button == _continueButtons [i]) {
				_clickedContinueButton = i;
			}
		}
	}
	/*
	 * Keep the gate for both C and C++ error handling.
	 */
	int status = 1;
	try {
		status = _okCallback (this, NULL, NULL, NULL, false, _buttonClosure);   // C: errors are put into a zero return value
	} catch (...) {
		status = 0;   // C++: errors are thrown as exceptions
	}
	if (status) {
		/*
		 * Write everything to history. Before destruction!
		 */
		if (! _isPauseForm) {
			history.write (L"\n");
			history.write (_invokingButtonTitle);
			int size = _numberOfFields;
			while (size >= 1 && _field [size] -> _type == UI_LABEL)
				size --;   /* Ignore trailing fields without a value. */
			for (int ifield = 1; ifield <= size; ifield ++) {
				UiField *field = _field [ifield];
				switch (field -> _type) {
					case UI_REAL: case UI_REAL_OR_UNDEFINED: case UI_POSITIVE: {
						history.write (L" ");
						history.write (Melder_double (field -> _realValue));
					} break; case UI_INTEGER: case UI_NATURAL: case UI_CHANNEL: {
						history.write (L" ");
						history.write (Melder_integer (field -> _integerValue));
					} break; case UI_WORD: case UI_SENTENCE: case UI_TEXT: {
						if (ifield < size && (field -> _stringValue [0] == '\0' || wcschr (field -> _stringValue, ' '))) {
							history.write (L" \"");
							history.write (field -> _stringValue);   // BUG: should double any double quotes
							history.write (L"\"");
						} else {
							history.write (L" ");
							history.write (field -> _stringValue);
						}
					} break; case UI_BOOLEAN: {
						history.write (field -> _integerValue ? L" yes" : L" no");
					} break; case UI_RADIO: case UI_OPTIONMENU: {
						UiOption *b = (UiOption*)field -> _options -> item [field -> _integerValue];
						if (ifield < size && (b -> _name [0] == '\0' || wcschr (b -> _name, ' '))) {
							history.write (L" \"");
							history.write (b -> _name);
							history.write (L"\"");
						} else {
							history.write (L" ");
							history.write (b -> _name);
						}
					} break; case UI_LIST: {
						if (ifield < size && (field -> _strings [field -> _integerValue] [0] == '\0' || wcschr (field -> _strings [field -> _integerValue], ' '))) {
							history.write (L" \"");
							history.write (field -> _strings [field -> _integerValue]);
							history.write (L"\"");
						} else {
							history.write (L" ");
							history.write (field -> _strings [field -> _integerValue]);
						}
					} break; case UI_COLOUR: {
						history.write (L" ");
						history.write (Graphics_Colour_name (field -> _colourValue));
					}
				}
			}
		}
		if (hide) {
			GuiObject_hide (_dialog);
			if (_destroyWhenUnmanaged)
				return;
		}
	} else {
		/*
		 * If a solution has already been suggested, or the "error" was actually a conscious user action, do not add anything more.
		 */
		if (! wcsstr (Melder_getError (), L"Please ") && ! wcsstr (Melder_getError (), L"You could ") &&
			! wcsstr (Melder_getError (), L"You interrupted "))
		{
			/*
			 * Otherwise, show a generic message.
			 */
			if (wcsstr (Melder_getError (), L"Selection changed!")) {
				Melder_error3 (L"Please change the selection in the object list, or click Cancel in the command window " L_LEFT_DOUBLE_QUOTE,
					_name, L_RIGHT_DOUBLE_QUOTE L".");
			} else {
				Melder_error3 (L"Please change something in the command window " L_LEFT_DOUBLE_QUOTE,
					_name, L_RIGHT_DOUBLE_QUOTE L", or click Cancel in that window.");
			}
		}
		/*XtAddCallback (w, XmNactivateCallback, ok, void_me);   /* FIX */
		Melder_flushError (NULL);
	}
	if (_okButton) GuiObject_setSensitive (_okButton, true);
	for (int i = 1; i <= _numberOfContinueButtons; i ++) if (_continueButtons [i]) GuiObject_setSensitive (_continueButtons [i], true);
	if (_applyButton) GuiObject_setSensitive (_applyButton, true);
	if (_cancelButton) GuiObject_setSensitive (_cancelButton, true);
	if (_revertButton) GuiObject_setSensitive (_revertButton, true);
	if (_helpButton) GuiObject_setSensitive (_helpButton, true);
}

static void gui_button_cb_ok (void *form, GuiButtonEvent event) {
	(void) event;
	((UiForm *)form) -> okOrApply (event -> button, true);
}

static void gui_button_cb_apply (void *form, GuiButtonEvent event) {
	(void) event;
	((UiForm *)form) -> okOrApply (event -> button, false);
}

static void gui_button_cb_help (void *form, GuiButtonEvent event) {
	(void) event;
	Melder_help (((UiForm *)form) -> _helpTitle);
}

void UiForm::setPauseForm (
	int numberOfContinueButtons, int defaultContinueButton, int cancelContinueButton,
	const wchar_t *continue1, const wchar_t *continue2, const wchar_t *continue3,
	const wchar_t *continue4, const wchar_t *continue5, const wchar_t *continue6,
	const wchar_t *continue7, const wchar_t *continue8, const wchar_t *continue9,
	const wchar_t *continue10,
	int (*cancelCallback) (Any dia, void *closure))
{
	_isPauseForm = true;
	_numberOfContinueButtons = numberOfContinueButtons;
	_defaultContinueButton = defaultContinueButton;
	_cancelContinueButton = cancelContinueButton;
	_continueTexts [1] = continue1;
	_continueTexts [2] = continue2;
	_continueTexts [3] = continue3;
	_continueTexts [4] = continue4;
	_continueTexts [5] = continue5;
	_continueTexts [6] = continue6;
	_continueTexts [7] = continue7;
	_continueTexts [8] = continue8;
	_continueTexts [9] = continue9;
	_continueTexts [10] = continue10;
	_cancelCallback = cancelCallback;
}

static int commonOkCallback (UiForm *dia, const wchar_t *dummy, Interpreter *interpreter, const wchar_t *invokingButtonTitle, bool modified, void *closure) {
	EditorCommand *cmd = (EditorCommand *) closure;
	(void) dia;
	(void) dummy;
	(void) invokingButtonTitle;
	(void) modified;
	return cmd -> _commandCallback (cmd -> _editor, cmd, (UiForm *)cmd -> _dialog, NULL, interpreter);
}

UiForm * UiForm::createE (EditorCommand *cmd, const wchar_t *title, const wchar_t *invokingButtonTitle, const wchar_t *helpTitle) {
	Editor *editor = cmd -> _editor;
	UiForm *dia = new UiForm (editor -> _dialog, title, commonOkCallback, cmd, invokingButtonTitle, helpTitle);
	dia -> _command = cmd;
	return dia;
}

UiForm::UiField * UiForm::addField (int type, const wchar_t *label) {
	if (_numberOfFields == MAXIMUM_NUMBER_OF_FIELDS) return NULL;
	return _field [++ _numberOfFields] = new UiField (type, label);
}

UiForm::UiField * UiForm::addReal (const wchar_t *label, const wchar_t *defaultValue) {
	UiField *field = addField (UI_REAL, label);
	if (field == NULL) return NULL;
	field -> _stringDefaultValue = Melder_wcsdup_f (defaultValue);
	return field;
}

UiForm::UiField * UiForm::addRealOrUndefined (const wchar_t *label, const wchar_t *defaultValue) {
	UiField *field = addField (UI_REAL_OR_UNDEFINED, label);
	if (field == NULL) return NULL;
	field -> _stringDefaultValue = Melder_wcsdup_f (defaultValue);
	return field;
}

UiForm::UiField * UiForm::addPositive (const wchar_t *label, const wchar_t *defaultValue) {
	UiField *field = addField (UI_POSITIVE, label);
	if (field == NULL) return NULL;
	field -> _stringDefaultValue = Melder_wcsdup_f (defaultValue);
	return field;
}

UiForm::UiField * UiForm::addInteger (const wchar_t *label, const wchar_t *defaultValue) {
	UiField *field = addField (UI_INTEGER, label);
	if (field == NULL) return NULL;
	field -> _stringDefaultValue = Melder_wcsdup_f (defaultValue);
	return field;
}

UiForm::UiField * UiForm::addNatural (const wchar_t *label, const wchar_t *defaultValue) {
	UiField *field = addField (UI_NATURAL, label);
	if (field == NULL) return NULL;
	field -> _stringDefaultValue = Melder_wcsdup_f (defaultValue);
	return field;
}

UiForm::UiField * UiForm::addWord (const wchar_t *label, const wchar_t *defaultValue) {
	UiField *field = addField (UI_WORD, label);
	if (field == NULL) return NULL;
	field -> _stringDefaultValue = Melder_wcsdup_f (defaultValue);
	return field;
}

UiForm::UiField * UiForm::addSentence (const wchar_t *label, const wchar_t *defaultValue) {
	UiField *field = addField (UI_SENTENCE, label);
	if (field == NULL) return NULL;
	field -> _stringDefaultValue = Melder_wcsdup_f (defaultValue);
	return field;
}

UiForm::UiField * UiForm::addLabel (const wchar_t *name, const wchar_t *label) {
	UiField *field = addField (UI_LABEL, name);
	if (field == NULL) return NULL;
	field -> _stringValue = Melder_wcsdup_f (label);
	return field;
}

UiForm::UiField * UiForm::addBoolean (const wchar_t *label, int defaultValue) {
	UiField *field = addField (UI_BOOLEAN, label);
	if (field == NULL) return NULL;
	field -> _integerDefaultValue = defaultValue;
	return field;
}

UiForm::UiField * UiForm::addText (const wchar_t *name, const wchar_t *defaultValue) {
	UiField *field = addField (UI_TEXT, name);
	if (field == NULL) return NULL;
	field -> _stringDefaultValue = Melder_wcsdup_f (defaultValue);
	return field;
}

UiForm::UiField * UiForm::addRadio (const wchar_t *label, int defaultValue) {
	UiField *field = addField (UI_RADIO, label);
	if (field == NULL) return NULL;
	field -> _integerDefaultValue = defaultValue;
	field -> _options = Ordered_create ();
	return field;
}

UiForm::UiField * UiForm::addOptionMenu (const wchar_t *label, int defaultValue) {
	UiField *field = addField (UI_OPTIONMENU, label);
	if (field == NULL) return NULL;
	field -> _integerDefaultValue = defaultValue;
	field -> _options = Ordered_create ();
	return field;
}

UiForm::UiField * UiForm::addList (const wchar_t *label, long numberOfStrings, const wchar_t **strings, long defaultValue) {
	UiField *field = addField (UI_LIST, label);
	if (field == NULL) return NULL;
	field -> _numberOfStrings = numberOfStrings;
	field -> _strings = strings;
	field -> _integerDefaultValue = defaultValue;
	return field;
}

UiForm::UiField * UiForm::addColour (const wchar_t *label, const wchar_t *defaultValue) {
	UiField *field = addField (UI_COLOUR, label);
	if (field == NULL) return NULL;
	field -> _stringDefaultValue = Melder_wcsdup_f (defaultValue);
	return field;
}

UiForm::UiField * UiForm::addChannel (const wchar_t *label, const wchar_t *defaultValue) {
	UiField *field = addField (UI_CHANNEL, label);
	if (field == NULL) return NULL;
	field -> _stringDefaultValue = Melder_wcsdup_f (defaultValue);
	return field;
}

#define DIALOG_X  150
#define DIALOG_Y  70
#define HELP_BUTTON_WIDTH  60
#define STANDARDS_BUTTON_WIDTH  100
#define REVERT_BUTTON_WIDTH  60
#define STOP_BUTTON_WIDTH  50
#define HELP_BUTTON_X  20
#define LIST_HEIGHT  192

static MelderString theFinishBuffer = { 0 };

static void appendColon (void) {
	long length = theFinishBuffer.length;
	if (length < 1) return;
	wchar_t lastCharacter = theFinishBuffer.string [length - 1];
	if (lastCharacter == ':' || lastCharacter == '?' || lastCharacter == '.') return;
	MelderString_appendCharacter (& theFinishBuffer, ':');
}

// FIXME wrong number of parameters
static void gui_radiobutton_cb_toggled (void *field, GuiRadioButtonEvent event) {
	#if !gtk
	for (int i = 1; i <= _options -> size; i ++) {
		UiOption *b = (UiOption *)_options -> item [i];
		GuiRadioButton_setValue (b -> _toggle, b -> _toggle == event -> _toggle);
	}
	#endif
}

void UiForm::finish () {
	if (! _parent && ! _isPauseForm) return;

	int size = _numberOfFields;
	int dialogHeight = 0, x = Gui_LEFT_DIALOG_SPACING, y;
	int textFieldHeight = Gui_TEXTFIELD_HEIGHT;
	int dialogWidth = 520, dialogCentre = dialogWidth / 2, fieldX = dialogCentre + Gui_LABEL_SPACING / 2;
	int labelWidth = fieldX - Gui_LABEL_SPACING - x, fieldWidth = labelWidth, halfFieldWidth = fieldWidth / 2 - 6;

	#if gtk
		GuiObject form, buttons;
		int numberOfRows = 0, row = 0;
	#else
		GuiObject form, buttons; // Define?
	#endif

	/*
		Compute height. Cannot leave this to the default geometry management system.
	*/
	for (long ifield = 1; ifield <= _numberOfFields; ifield ++ ) {
		UiField *field = _field [ifield], *previous = _field [ifield - 1];
		dialogHeight +=
			ifield == 1 ? Gui_TOP_DIALOG_SPACING :
			field -> _type == UI_RADIO || previous -> _type == UI_RADIO ? Gui_VERTICAL_DIALOG_SPACING_DIFFERENT :
			field -> _type >= UI_LABELLEDTEXT_MIN && field -> _type <= UI_LABELLEDTEXT_MAX && wcsnequ (field -> _name, L"right ", 6) &&
			previous -> _type >= UI_LABELLEDTEXT_MIN && previous -> _type <= UI_LABELLEDTEXT_MAX &&
			wcsnequ (previous -> _name, L"left ", 5) ? - textFieldHeight : Gui_VERTICAL_DIALOG_SPACING_SAME;
		field -> _y = dialogHeight;
		dialogHeight +=
			field -> _type == UI_BOOLEAN ? Gui_CHECKBUTTON_HEIGHT :
			field -> _type == UI_RADIO ? field -> _options -> size * Gui_RADIOBUTTON_HEIGHT +
				(field -> _options -> size - 1) * Gui_RADIOBUTTON_SPACING :
			field -> _type == UI_OPTIONMENU ? Gui_OPTIONMENU_HEIGHT :
			field -> _type == UI_LIST ? LIST_HEIGHT :
			field -> _type == UI_LABEL && field -> _stringValue [0] != '\0' && field -> _stringValue [wcslen (field -> _stringValue) - 1] != '.' &&
				ifield != _numberOfFields ? textFieldHeight
				#ifdef _WIN32
					- 6 :
				#else
					- 10 :
				#endif
			textFieldHeight;
		#if gtk
			numberOfRows += wcsnequ (field -> _name, L"left ", 5);
		#endif
	}
	dialogHeight += 2 * Gui_BOTTOM_DIALOG_SPACING + Gui_PUSHBUTTON_HEIGHT;
	_dialog = GuiDialog_create (_parent, DIALOG_X, DIALOG_Y, dialogWidth, dialogHeight, _name, gui_dialog_cb_close, this, 0);

	#if gtk
		form = gtk_table_new (numberOfRows, 3, false);
		gtk_table_set_col_spacing (GTK_TABLE (form), 0, 5);
		gtk_container_add (GTK_CONTAINER (_dialog), form);
		gtk_widget_show (form);
		buttons = GTK_DIALOG (GuiObject_parent (_dialog)) -> action_area;
	#else
		form = _dialog;
		buttons = _dialog;
	#endif

	for (long ifield = 1; ifield <= size; ifield ++) {
		UiField *field = _field [ifield];
		y = field -> _y;
		switch (field -> _type) {
			case UI_REAL:
			case UI_REAL_OR_UNDEFINED:
			case UI_POSITIVE:
			case UI_INTEGER:
			case UI_NATURAL:
			case UI_WORD:
			case UI_SENTENCE:
			case UI_COLOUR:
			case UI_CHANNEL:
			{
				int ylabel = y;
				#if defined (macintosh)
					ylabel += 3;
				#endif
				if (wcsnequ (field -> _name, L"left ", 5)) {
					MelderString_copy (& theFinishBuffer, field -> _formLabel + 5);
					appendColon ();
					GuiObject label = GuiLabel_createShown (form, x, x + labelWidth, ylabel, ylabel + textFieldHeight,
						theFinishBuffer.string, GuiLabel_RIGHT);
					#if gtk
						gtk_table_attach_defaults (GTK_TABLE (form), label, 0, 1, row, row + 1);
					#endif
					field -> _text = GuiText_createShown (form, fieldX, fieldX + halfFieldWidth, y, Gui_AUTOMATIC, 0);
					#if gtk
						gtk_table_attach_defaults (GTK_TABLE (form), field -> _text, 1, 2, row, row + 1);
					#endif
				} else if (wcsnequ (field -> _name, L"right ", 6)) {
					field -> _text = GuiText_createShown (form, fieldX + halfFieldWidth + 12, fieldX + fieldWidth,
						y, Gui_AUTOMATIC, 0);
					#if gtk
						gtk_table_attach_defaults (GTK_TABLE (form), field -> _text, 2, 3, row, row + 1);
						row += 1;
					#endif
				} else {
					MelderString_copy (& theFinishBuffer, field -> _formLabel);
					appendColon ();
					GuiObject label = GuiLabel_createShown (form, x, x + labelWidth,
						ylabel, ylabel + textFieldHeight,
						theFinishBuffer.string, GuiLabel_RIGHT);
					#if gtk
						gtk_table_attach_defaults (GTK_TABLE (form), label, 0, 1, row, row + 1);
					#endif
					field -> _text = GuiText_createShown (form, fieldX, fieldX + fieldWidth, // or once the dialog is a Form: - Gui_RIGHT_DIALOG_SPACING,
						y, Gui_AUTOMATIC, 0);
					#if gtk
						gtk_table_attach_defaults (GTK_TABLE (form), field -> _text, 1, 3, row, row + 1);
						row += 1;
					#endif
				}
			} break;
			case UI_TEXT:
			{
				field -> _text = GuiText_createShown (form, x, x + dialogWidth - Gui_LEFT_DIALOG_SPACING - Gui_RIGHT_DIALOG_SPACING,
					y, Gui_AUTOMATIC, 0);
				#if gtk
					gtk_table_attach_defaults (GTK_TABLE (form), field -> _text, 0, 3, row, row + 1);
					row += 1;
				#endif
			} break;
			case UI_LABEL:
			{
				MelderString_copy (& theFinishBuffer, field -> _stringValue);
				field -> _text = GuiLabel_createShown (form,
					x, dialogWidth /* allow to extend into the margin */, y + 5, y + 5 + textFieldHeight,
					theFinishBuffer.string, 0);
				#if gtk
					gtk_table_attach_defaults (GTK_TABLE (form), field -> _text, 0, 3, row, row + 1);
					row += 1;
				#endif
			} break;
			case UI_RADIO:
			{
				int ylabel = y;
				#if gtk
					 void *group = NULL;
				#endif
				#if defined (macintosh)
					ylabel += 1;
				#endif
				MelderString_copy (& theFinishBuffer, field -> _formLabel);
				appendColon ();
				GuiObject label = GuiLabel_createShown (form, x, x + labelWidth, ylabel, ylabel + Gui_RADIOBUTTON_HEIGHT,
					theFinishBuffer.string, GuiLabel_RIGHT);
				#if gtk
					gtk_table_attach_defaults (GTK_TABLE (form), label, 0, 1, row, row + 1);
				#endif
				for (long ibutton = 1; ibutton <= field -> _options -> size; ibutton ++) {
					UiOption *button = (UiOption *)field -> _options -> item [ibutton];
					MelderString_copy (& theFinishBuffer, button -> _name);
					button -> _toggle = GuiRadioButton_createShown (form,
						fieldX, dialogWidth /* allow to extend into the margin */,
						y + (ibutton - 1) * (Gui_RADIOBUTTON_HEIGHT + Gui_RADIOBUTTON_SPACING), Gui_AUTOMATIC,
						theFinishBuffer.string, gui_radiobutton_cb_toggled, field, 0);
					#if gtk
						if (group != NULL) {
							GuiRadioButton_setGroup (button -> _toggle, group);
						} 
						group = GuiRadioButton_getGroup (button -> _toggle);
						gtk_table_attach_defaults (GTK_TABLE (form), button -> _toggle, 1, 3, row, row + 1);
						row += 1;
					#endif
				}
			} break; 
			case UI_OPTIONMENU:
			{
				int ylabel = y;
				#if defined (macintosh)
					ylabel += 2;
				#endif
				GuiObject bar, box;
				MelderString_copy (& theFinishBuffer, field -> _formLabel);
				appendColon ();
				GuiObject label = GuiLabel_createShown (form, x, x + labelWidth, ylabel, ylabel + Gui_OPTIONMENU_HEIGHT,
					theFinishBuffer.string, GuiLabel_RIGHT);
				#if gtk
					gtk_table_attach_defaults (GTK_TABLE (form), label, 0, 1, row, row + 1);
				#endif

				#if motif
					bar = XmCreateMenuBar (form, "UiOptionMenu", NULL, 0);
					XtVaSetValues (bar, XmNx, fieldX - 4, XmNy, y - 4
						#if defined (macintosh)
							- 1
						#endif
						, XmNwidth, fieldWidth + 8, XmNheight, Gui_OPTIONMENU_HEIGHT + 8, NULL);
				#endif
				// TODO: dit wil natuurlijk heel graag in GuiComboBox.c ;)
				#if gtk
					field -> _cascadeButton = gtk_combo_box_new_text ();
					gtk_combo_box_set_focus_on_click (GTK_COMBO_BOX (field -> _cascadeButton), false);
					GTK_WIDGET_UNSET_FLAGS (field -> _cascadeButton, GTK_CAN_DEFAULT);
					gtk_table_attach_defaults (GTK_TABLE (form), field -> _cascadeButton, 1, 3, row, row + 1);
					row += 1;
				#elif motif
					box = GuiMenuBar_addMenu2 (bar, L"choice", 0, & field -> _cascadeButton);
					XtVaSetValues (bar, XmNwidth, fieldWidth + 8, NULL);
					XtVaSetValues (field -> _cascadeButton, XmNx, 4, XmNy, 4, XmNwidth, fieldWidth, XmNheight, Gui_OPTIONMENU_HEIGHT, NULL);
				#endif
				for (long ibutton = 1; ibutton <= field -> _options -> size; ibutton ++) {
					UiOption *button = (UiOption *)field -> _options -> item [ibutton];
					MelderString_copy (& theFinishBuffer, button -> _name);
					#if gtk
						gtk_combo_box_append_text (GTK_COMBO_BOX (field -> _cascadeButton), Melder_peekWcsToUtf8 (theFinishBuffer.string));
					#elif motif
						button -> _toggle = XtVaCreateManagedWidget (Melder_peekWcsToUtf8 (theFinishBuffer.string), xmToggleButtonWidgetClass, box, NULL);
						XtAddCallback (button -> _toggle, XmNvalueChangedCallback, cb_optionChanged, (XtPointer) field);
					#endif
				}
				#if gtk
					GuiObject_show (field -> _cascadeButton);
				#elif motif
					GuiObject_show (bar);
				#endif
			} break;
			case UI_BOOLEAN:
			{
				MelderString_copy (& theFinishBuffer, field -> _formLabel);
				/*GuiLabel_createShown (form, x, x + labelWidth, y, y + Gui_CHECKBUTTON_HEIGHT,
					theFinishBuffer.string, GuiLabel_RIGHT); */
				field -> _toggle = GuiCheckButton_createShown (form,
					fieldX, dialogWidth /* allow to extend into the margin */, y, Gui_AUTOMATIC,
					theFinishBuffer.string, NULL, NULL, 0);
				#if gtk
					gtk_table_attach_defaults (GTK_TABLE (form), field -> _toggle, 1, 3, row, row + 1);
					row += 1;
				#endif
			} break;
			case UI_LIST:
			{
				int listWidth = _numberOfFields == 1 ? dialogWidth - fieldX : fieldWidth;
				MelderString_copy (& theFinishBuffer, field -> _formLabel);
				appendColon ();
				GuiObject label = GuiLabel_createShown (form, x, x + labelWidth, y + 1, y + 21,
					theFinishBuffer.string, GuiLabel_RIGHT);
				#if gtk
					gtk_table_attach_defaults (GTK_TABLE (form), label, 0, 1, row, row + 1);
				#endif
				field -> _list = GuiList_create (form, fieldX, fieldX + listWidth, y, y + LIST_HEIGHT, false, theFinishBuffer.string);
				for (long i = 1; i <= field -> _numberOfStrings; i ++) {
					GuiList_insertItem (field -> _list, field -> _strings [i], 0);
				}
				GuiObject_show (field -> _list);
				#if gtk
					gtk_table_attach_defaults (GTK_TABLE (form), gtk_widget_get_parent (field -> _list), 1, 3, row, row + 1);
					row += 1;
				#endif
			} break;
		}
	}
	for (long ifield = 1; ifield <= _numberOfFields; ifield ++)
		_field [ifield] -> setDefault ();
	/*separator = XmCreateSeparatorGadget (column, "separator", NULL, 0);*/
	y = dialogHeight - Gui_BOTTOM_DIALOG_SPACING - Gui_PUSHBUTTON_HEIGHT;
	if (_helpTitle) {
		_helpButton = GuiButton_createShown (buttons, HELP_BUTTON_X, HELP_BUTTON_X + HELP_BUTTON_WIDTH, y, Gui_AUTOMATIC,
			L"Help", gui_button_cb_help, this, 0);
		#if gtk
			gtk_button_box_set_child_secondary(GTK_BUTTON_BOX(buttons), _helpButton, TRUE);
		#endif
	}
	if (_numberOfFields > 1 || (_numberOfFields > 0 && _field [1] -> _type != UI_LABEL)) {
		if (_isPauseForm) {
			_revertButton = GuiButton_createShown (buttons,
				HELP_BUTTON_X, HELP_BUTTON_X + REVERT_BUTTON_WIDTH,
				y, Gui_AUTOMATIC, L"Revert", gui_button_cb_revert, this, 0);
		} else {
			_revertButton = GuiButton_createShown (buttons,
				HELP_BUTTON_X + HELP_BUTTON_WIDTH + Gui_HORIZONTAL_DIALOG_SPACING,
				HELP_BUTTON_X + HELP_BUTTON_WIDTH + Gui_HORIZONTAL_DIALOG_SPACING + STANDARDS_BUTTON_WIDTH,
				y, Gui_AUTOMATIC, L"Standards", gui_button_cb_revert, this, 0);
		}
		#if gtk
			gtk_button_box_set_child_secondary (GTK_BUTTON_BOX (buttons), _revertButton, TRUE);
		#endif
	}
	if (_isPauseForm) {
		x = HELP_BUTTON_X + REVERT_BUTTON_WIDTH + Gui_HORIZONTAL_DIALOG_SPACING;
		if (_cancelContinueButton == 0) {
			_cancelButton = GuiButton_createShown (buttons, x, x + STOP_BUTTON_WIDTH, y, Gui_AUTOMATIC,
				L"Stop", gui_button_cb_cancel, this, GuiButton_CANCEL);
			x += STOP_BUTTON_WIDTH + 7;
		} else {
			x += 30;
		}
		int room = dialogWidth - Gui_RIGHT_DIALOG_SPACING - x;
		int roomPerContinueButton = room / _numberOfContinueButtons;
		int horizontalSpacing = _numberOfContinueButtons > 7 ? Gui_HORIZONTAL_DIALOG_SPACING - 2 * (_numberOfContinueButtons - 7) : Gui_HORIZONTAL_DIALOG_SPACING;
		int continueButtonWidth = roomPerContinueButton - horizontalSpacing;
		for (int i = 1; i <= _numberOfContinueButtons; i ++) {
			x = dialogWidth - Gui_RIGHT_DIALOG_SPACING - roomPerContinueButton * (_numberOfContinueButtons - i + 1) + horizontalSpacing;
			_continueButtons [i] = GuiButton_createShown (buttons, x, x + continueButtonWidth, y, Gui_AUTOMATIC,
				_continueTexts [i], gui_button_cb_ok, this, i == _defaultContinueButton ? GuiButton_DEFAULT : 0);
		}
	} else {
		x = dialogWidth - Gui_RIGHT_DIALOG_SPACING - Gui_OK_BUTTON_WIDTH - 2 * Gui_HORIZONTAL_DIALOG_SPACING
			 - Gui_APPLY_BUTTON_WIDTH - Gui_CANCEL_BUTTON_WIDTH;
		_cancelButton = GuiButton_createShown (buttons, x, x + Gui_CANCEL_BUTTON_WIDTH, y, Gui_AUTOMATIC,
			L"Cancel", gui_button_cb_cancel, this, GuiButton_CANCEL);
		x = dialogWidth - Gui_RIGHT_DIALOG_SPACING - Gui_OK_BUTTON_WIDTH - Gui_HORIZONTAL_DIALOG_SPACING - Gui_APPLY_BUTTON_WIDTH;
		if (_numberOfFields > 1 || _field [1] -> _type != UI_LABEL) {
			_applyButton = GuiButton_createShown (buttons, x, x + Gui_APPLY_BUTTON_WIDTH, y, Gui_AUTOMATIC,
				L"Apply", gui_button_cb_apply, this, 0);
		}
		x = dialogWidth - Gui_RIGHT_DIALOG_SPACING - Gui_OK_BUTTON_WIDTH;
		_okButton = GuiButton_createShown (buttons, x, x + Gui_OK_BUTTON_WIDTH, y, Gui_AUTOMATIC,
			_isPauseForm ? L"Continue" : L"OK", gui_button_cb_ok, this, GuiButton_DEFAULT);
	}
	/*GuiObject_show (separator);*/
}

void UiForm::do_ (bool modified) {
	_allowExecutionHook = theAllowExecutionHookHint;
	_allowExecutionClosure = theAllowExecutionClosureHint;
	/* Prevent double callbacks: */
	/*XtRemoveCallback (_okButton, XmNactivateCallback, ok, (XtPointer) me);*/
	/* This is the only place where this callback is installed. Moved from UiForm::close ppgb950613. */
	/*XtAddCallback (_okButton, XmNactivateCallback, ok, (XtPointer) me);*/
	GuiObject_show (_dialog);
	if (modified)
		okOrApply (NULL, true);
}

int UiForm::parseString (const wchar_t *arguments, Interpreter *interpreter) {
	int size = _numberOfFields;
	while (size >= 1 && _field [size] -> _type == UI_LABEL)
		size --;   /* Ignore trailing fields without a value. */
	for (int i = 1; i < size; i ++) {
		static wchar_t _stringValue [3000];
		int ichar = 0;
		if (_field [i] -> _type == UI_LABEL)
			continue;   /* Ignore non-trailing fields without a value. */
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
				_stringValue [ichar ++] = *arguments ++;
			}
		} else {
			while (*arguments != ' ' && *arguments != '\t' && *arguments != '\0')
				_stringValue [ichar ++] = *arguments ++;
		}
		_stringValue [ichar] = '\0';   /* Trailing null byte. */
		if (! _field [i] -> stringToValue (_stringValue, interpreter))
			return Melder_error3 (L"Don't understand contents of field \"", _field [i] -> _name, L"\".");
	}
	/* The last item is handled separately, because it consists of the rest of the line.
	 * Leading spaces are skipped, but trailing spaces are included.
	 */
	if (size > 0) {
		while (*arguments == ' ' || *arguments == '\t') arguments ++;
		if (! _field [size] -> stringToValue (arguments, interpreter))
			return Melder_error3 (L"Don't understand contents of field \"", _field [size] -> _name, L"\".");
	}
	return _okCallback (this, NULL, interpreter, NULL, false, _buttonClosure);
}

int UiForm::parseStringE (EditorCommand *cmd, const wchar_t *arguments, Interpreter *interpreter) {
	return ((UiForm *)cmd->_dialog)->parseString (arguments, interpreter);
}

UiForm::UiField *UiForm::findField (const wchar_t *fieldName) {
	for (int ifield = 1; ifield <= _numberOfFields; ifield ++)
		if (wcsequ (fieldName, _field [ifield] -> _name)) return _field [ifield];
	return NULL;
}

static void fatalField (UiForm *dia) {
	Melder_fatal ("Wrong field in dialog \"%s\".", Melder_peekWcsToUtf8 (dia -> _name));
}

void UiForm::setReal (const wchar_t *fieldName, double value) {
	UiField *field = findField (fieldName);
	if (field == NULL) Melder_fatal ("(UiForm::setReal:) No field \"%s\" in dialog \"%s\".",
		Melder_peekWcsToUtf8 (fieldName), Melder_peekWcsToUtf8 (_name));
	switch (field -> _type) {
		case UI_REAL: case UI_REAL_OR_UNDEFINED: case UI_POSITIVE: {
			if (value == Melder_atof (field -> _stringDefaultValue)) {
				GuiText_setString (field -> _text, field -> _stringDefaultValue);
			} else {
				wchar_t s [40];
				wcscpy (s, Melder_double (value));
				/*
				 * If the default is overtly real, the shown value must be as well.
				 */
				if ((wcschr (field -> _stringDefaultValue, '.') || wcschr (field -> _stringDefaultValue, 'e')) &&
					! (wcschr (s, '.') || wcschr (s, 'e')))
				{
					wcscat (s, L".0");
				}
				GuiText_setString (field -> _text, s);
			}
		} break; case UI_COLOUR: {
			GuiText_setString (field -> _text, Melder_double (value));   // some grey value
		} break; default: {
			Melder_fatal ("Wrong field in dialog \"%s\".", Melder_peekWcsToUtf8 (_name));
		}
	}
}

void UiForm::setInteger (const wchar_t *fieldName, long value) {
	UiField *field = findField (fieldName);
	if (field == NULL) Melder_fatal ("(UiForm::setInteger:) No field \"%s\" in dialog \"%s\".",
		Melder_peekWcsToUtf8 (fieldName), Melder_peekWcsToUtf8 (_name));
	switch (field -> _type) {
		case UI_INTEGER: case UI_NATURAL: case UI_CHANNEL: {
			if (value == wcstol (field -> _stringDefaultValue, NULL, 10)) {
				GuiText_setString (field -> _text, field -> _stringDefaultValue);
			} else {
				GuiText_setString (field -> _text, Melder_integer (value));
			}
		} break; case UI_BOOLEAN: {
			GuiCheckButton_setValue (field -> _toggle, value);
		} break; case UI_RADIO: {
			if (value < 1 || value > field -> _options -> size) value = 1;   /* Guard against incorrect prefs file. */
			for (int i = 1; i <= field -> _options -> size; i ++) {
				UiOption *b = (UiOption *) field -> _options -> item [i];
				GuiRadioButton_setValue (b -> _toggle, i == value);
			}
		} break; case UI_OPTIONMENU: {
			if (value < 1 || value > field -> _options -> size) value = 1;   /* Guard against incorrect prefs file. */
			for (int i = 1; i <= field -> _options -> size; i ++) {
				UiOption *b = (UiOption *) field -> _options -> item [i];
				#if motif
				XmToggleButtonSetState (b -> _toggle, i == value, False);
				if (i == value) {
					XtVaSetValues (field -> _cascadeButton, motif_argXmString (XmNlabelString, Melder_peekWcsToUtf8 (b -> _name)), NULL);
				}
				#endif
			}
		} break; case UI_LIST: {
			if (value < 1 || value > field -> _numberOfStrings) value = 1;   /* Guard against incorrect prefs file. */
			GuiList_selectItem (field -> _list, value);
		} break; default: {
			fatalField (this);
		}
	}
}

void UiForm::setString (const wchar_t *fieldName, const wchar_t *value) {
	UiField *field = findField (fieldName);
	if (field == NULL) Melder_fatal ("(UiForm::setString:) No field \"%s\" in dialog \"%s\".",
		Melder_peekWcsToUtf8 (fieldName), Melder_peekWcsToUtf8 (_name));
	if (value == NULL) value = L"";   /* Accept NULL strings. */
	switch (field -> _type) {
		case UI_REAL: case UI_REAL_OR_UNDEFINED: case UI_POSITIVE: case UI_INTEGER: case UI_NATURAL:
			case UI_WORD: case UI_SENTENCE: case UI_COLOUR: case UI_CHANNEL: case UI_TEXT:
		{
			GuiText_setString (field -> _text, value);
		} break; case UI_LABEL: {
			GuiLabel_setString (field -> _text, value);
		} break; case UI_RADIO: {
			bool found = false;
			for (int i = 1; i <= field -> _options -> size; i ++) {
				UiOption *b = (UiOption *) field -> _options -> item [i];
				if (wcsequ (value, b -> _name)) {
					GuiRadioButton_setValue (b -> _toggle, true);
					found = true;
				} else {
					GuiRadioButton_setValue (b -> _toggle, false);
				}
			}
			/* If not found: do nothing (guard against incorrect prefs file). */
		} break; case UI_OPTIONMENU: {
			bool found = false;
			for (int i = 1; i <= field -> _options -> size; i ++) {
				UiOption *b = (UiOption *) field -> _options -> item [i];
				if (wcsequ (value, b -> _name)) {
					#if motif
					XmToggleButtonSetState (b -> _toggle, True, False);
					#endif
					found = true;
					if (field -> _type == UI_OPTIONMENU) {
						#if motif
						XtVaSetValues (field -> _cascadeButton, motif_argXmString (XmNlabelString, Melder_peekWcsToUtf8 (value)), NULL);
						#endif
					}
				} else {
					#if motif
						XmToggleButtonSetState (b -> _toggle, False, False);
					#endif
				}
			}
			/* If not found: do nothing (guard against incorrect prefs file). */
		} break; case UI_LIST: {
			long i;
			for (i = 1; i <= field -> _numberOfStrings; i ++)
				if (wcsequ (value, field -> _strings [i])) break;
			if (i > field -> _numberOfStrings) i = 1;   /* Guard against incorrect prefs file. */
			GuiList_selectItem (field -> _list, i);
		} break; default: {
			fatalField (this);
		}
	}
}

UiForm::UiField* UiForm::findField_check (const wchar_t *fieldName) {
	UiField *result = findField (fieldName);
	if (result == NULL) {
		Melder_error3 (L"Cannot find field \"", fieldName, L"\" in form.\n"
			"The script may have changed while the form was open.\n"
			"Please click Cancel in the form and try again.");
	}
	return result;
}

double UiForm::getReal (const wchar_t *fieldName) {
	UiField *field = findField (fieldName);
	if (field == NULL) Melder_fatal ("(UiForm::getReal:) No field \"%s\" in dialog \"%s\".",
		Melder_peekWcsToUtf8 (fieldName), Melder_peekWcsToUtf8 (_name));
	switch (field -> _type) {
		case UI_REAL: case UI_REAL_OR_UNDEFINED: case UI_POSITIVE: {
			return field -> _realValue;
		} break; default: {
			fatalField (this);
		}
	}
	return 0.0;
}

double UiForm::getReal_check (const wchar_t *fieldName) {
	UiField *field = findField_check (fieldName); cherror
	switch (field -> _type) {
		case UI_REAL: case UI_REAL_OR_UNDEFINED: case UI_POSITIVE: {
			return field -> _realValue;
		} break; default: {
			Melder_error3 (L"Cannot find a real value in field \"", fieldName, L"\" in the form.\n"
				"The script may have changed while the form was open.\n"
				"Please click Cancel in the form and try again.");
		}
	}
end:
	return 0.0;
}

long UiForm::getInteger (const wchar_t *fieldName) {
	UiField *field = findField (fieldName);
	if (field == NULL) Melder_fatal ("(UiForm::getInteger:) No field \"%s\" in dialog \"%s\".",
		Melder_peekWcsToUtf8 (fieldName), Melder_peekWcsToUtf8 (_name));
	switch (field -> _type) {
		case UI_INTEGER: case UI_NATURAL: case UI_CHANNEL: case UI_BOOLEAN: case UI_RADIO:
			case UI_OPTIONMENU: case UI_LIST:
		{
			return field -> _integerValue;
		} break; default: {
			fatalField (this);
		}
	}
	return 0L;
}

long UiForm::getInteger_check (const wchar_t *fieldName) {
	UiField *field = findField_check (fieldName); cherror
	switch (field -> _type) {
		case UI_INTEGER: case UI_NATURAL: case UI_CHANNEL: case UI_BOOLEAN: case UI_RADIO:
			case UI_OPTIONMENU: case UI_LIST:
		{
			return field -> _integerValue;
		} break; default: {
			Melder_error3 (L"Cannot find an integer value in field \"", fieldName, L"\" in the form.\n"
				"The script may have changed while the form was open.\n"
				"Please click Cancel in the form and try again.");
		}
	}
end:
	return 0L;
}

wchar_t * UiForm::getString (const wchar_t *fieldName) {
	UiField *field = findField (fieldName);
	if (field == NULL) Melder_fatal ("(UiForm::getString:) No field \"%s\" in dialog \"%s\".",
		Melder_peekWcsToUtf8 (fieldName), Melder_peekWcsToUtf8 (_name));
	switch (field -> _type) {
		case UI_WORD: case UI_SENTENCE: case UI_TEXT: {
			return field -> _stringValue;
		} break; case UI_RADIO: case UI_OPTIONMENU: {
			UiOption *b = (UiOption *)field -> _options -> item [field -> _integerValue];
			return b -> _name;
		} break; case UI_LIST: {
			return (wchar_t *) field -> _strings [field -> _integerValue];
		} break; default: {
			fatalField (this);
		}
	}
	return NULL;
}

wchar_t * UiForm::getString_check (const wchar_t *fieldName) {
	UiField *field = findField_check (fieldName); cherror
	switch (field -> _type) {
		case UI_WORD: case UI_SENTENCE: case UI_TEXT: {
			return field -> _stringValue;
		} break; case UI_RADIO: case UI_OPTIONMENU: {
			UiOption *b = (UiOption *)field -> _options -> item [field -> _integerValue];
			return b -> _name;
		} break; case UI_LIST: {
			return (wchar_t *) field -> _strings [field -> _integerValue];
		} break; default: {
			Melder_error3 (L"Cannot find a string in field \"", fieldName, L"\" in the form.\n"
				"The script may have changed while the form was open.\n"
				"Please click Cancel in the form and try again.");
		}
	}
end:
	return NULL;
}

Graphics_Colour UiForm::getColour (const wchar_t *fieldName) {
	UiField *field = findField (fieldName);
	if (field == NULL) Melder_fatal ("(UiForm::getColour:) No field \"%s\" in dialog \"%s\".",
		Melder_peekWcsToUtf8 (fieldName), Melder_peekWcsToUtf8 (_name));
	switch (field -> _type) {
		case UI_COLOUR: {
			return field -> _colourValue;
		} break; default: {
			fatalField (this);
		}
	}
	return Graphics_BLACK;
}

Graphics_Colour UiForm::getColour_check (const wchar_t *fieldName) {
	UiField *field = findField_check (fieldName); cherror
	switch (field -> _type) {
		case UI_COLOUR: {
			return field -> _colourValue;
		} break; default: {
			Melder_error3 (L"Cannot find a real value in field \"", fieldName, L"\" in the form.\n"
				"The script may have changed while the form was open.\n"
				"Please click Cancel in the form and try again.");
		}
	}
end:
	return Graphics_BLACK;
}

int UiForm::Interpreter_addVariables (Interpreter *interpreter) {
	static MelderString lowerCaseFieldName = { 0 };
	for (int ifield = 1; ifield <= _numberOfFields; ifield ++) {
		UiField *field = _field [ifield];
		MelderString_copy (& lowerCaseFieldName, field -> _name);
		/*
		 * Change e.g. "Number of people" to "number_of_people".
		 */
		lowerCaseFieldName.string [0] = towlower (lowerCaseFieldName.string [0]);
		for (wchar_t *p = & lowerCaseFieldName.string [0]; *p != '\0'; p ++) {
			if (*p == ' ') *p = '_';
		}
		switch (field -> _type) {
			case UI_INTEGER: case UI_NATURAL: case UI_CHANNEL: case UI_BOOLEAN: {
				InterpreterVariable *var = interpreter->lookUpVariable (lowerCaseFieldName.string); cherror
				var -> _numericValue = field -> _integerValue;
			} break; case UI_REAL: case UI_REAL_OR_UNDEFINED: case UI_POSITIVE: {
				InterpreterVariable *var = interpreter->lookUpVariable (lowerCaseFieldName.string); cherror
				var -> _numericValue = field -> _realValue;
			} break; case UI_RADIO: case UI_OPTIONMENU: {
				InterpreterVariable *var = interpreter->lookUpVariable (lowerCaseFieldName.string); cherror
				var -> _numericValue = field -> _integerValue;
				MelderString_appendCharacter (& lowerCaseFieldName, '$');
				var = interpreter->lookUpVariable (lowerCaseFieldName.string); cherror
				Melder_free (var -> _stringValue);
				UiOption *b = (UiOption *) field -> _options -> item [field -> _integerValue];
				var -> _stringValue = Melder_wcsdup_f (b -> _name);
			} break; case UI_LIST: {
				InterpreterVariable *var = interpreter->lookUpVariable (lowerCaseFieldName.string); cherror
				var -> _numericValue = field -> _integerValue;
				MelderString_appendCharacter (& lowerCaseFieldName, '$');
				var = interpreter->lookUpVariable (lowerCaseFieldName.string); cherror
				Melder_free (var -> _stringValue);
				var -> _stringValue = Melder_wcsdup_f ((wchar_t *) field -> _strings [field -> _integerValue]);
			} break; case UI_WORD: case UI_SENTENCE: case UI_TEXT: {
				MelderString_appendCharacter (& lowerCaseFieldName, '$');
				InterpreterVariable *var = interpreter->lookUpVariable (lowerCaseFieldName.string); cherror
				Melder_free (var -> _stringValue);
				var -> _stringValue = Melder_wcsdup_f (field -> _stringValue);
			} break; case UI_COLOUR: {
				// to be implemented
			} break; default: {
			}
		}
	}
end:
	iferror return 0;
	return 1;
}

int UiForm::getClickedContinueButton () {
	return _clickedContinueButton;
}




/***** class UiOption: radio buttons and menu options *****/

#if motif
// TODO: Ik denk dat dit Native GTK gedrag is (als dit alleen het label update)
static void cb_optionChanged (GuiObject w, XtPointer void_me, XtPointer call) {
	int i;
	(void) call;
	for (i = 1; i <= _options -> size; i ++) {
		UiOption *b = (UiOption *)_options -> item [i];
		#if motif
		if (b -> _toggle == w) {
			XtVaSetValues (_cascadeButton, motif_argXmString (XmNlabelString, Melder_peekWcsToUtf8 (b -> _name)), NULL);
			XmToggleButtonSetState (b -> _toggle, TRUE, FALSE);
			if (Melder_debug == 11) {
				Melder_warning4 (Melder_integer (i), L" \"", b -> _name, L"\"");
			}
		} else {
			XmToggleButtonSetState (b -> _toggle, FALSE, FALSE);
		}
		#endif
	}
}
#endif
