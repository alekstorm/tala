/* UiField.cpp
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

#include <ctype.h>
#include "UiForm.h"

UiForm::UiField::UiField (int type, const wchar_t *name)
	: _type(type),
	  _formLabel(Melder_wcsdup_f (name)),
	  _realValue(0.0),
	  _realDefaultValue(0.0),
	  _integerValue(0L),
	  _integerDefaultValue(0L),
	  _stringValue(NULL),
	  _stringDefaultValue(NULL),
	  _colourValue(Graphics_BLACK),
	  _stringValueA(NULL),
	  _numberOfStrings(0L),
	  _strings(NULL),
	  _text(NULL),
	  _toggle(NULL),
	  _list(NULL),
	  _cascadeButton(NULL),
	  _y(0),
	  _name(NULL) {
	wchar_t shortName [101], *p;
	wcscpy (shortName, name);
	/*
	 * Strip parentheses and colon off parameter name.
	 */
	//p = wcschr (shortName, ':');   /* ppgb 20101015: no idea why this used to be here */
	//if (p) *p = '\0';
	if ((p = (wchar *) wcschr (shortName, '(')) != NULL) {
		*p = '\0';
		if (p - shortName > 0 && p [-1] == ' ') p [-1] = '\0';
	}
	p = shortName;
	if (*p != '\0' && p [wcslen (p) - 1] == ':') p [wcslen (p) - 1] = '\0';
	_name = Melder_wcsdup_f (shortName);
}

UiForm::UiField::~UiField () {
	Melder_free (_formLabel);
	Melder_free (_stringValue);
	Melder_free (_stringValueA);
	Melder_free (_stringDefaultValue);
	Melder_free (_name);
	forget (_options);
}

void UiForm::UiField::setDefault () {
	switch (_type) {
		case UI_REAL: case UI_REAL_OR_UNDEFINED: case UI_POSITIVE: case UI_INTEGER: case UI_NATURAL:
			case UI_WORD: case UI_SENTENCE: case UI_COLOUR: case UI_CHANNEL: case UI_TEXT:
		{
			GuiText_setString (_text, _stringDefaultValue);
		} break; case UI_BOOLEAN: {
			GuiCheckButton_setValue (_toggle, _integerDefaultValue);
		} break; case UI_RADIO: {
			for (int i = 1; i <= _options -> size; i ++) {
				UiOption *b = (UiOption *) _options -> item [i];
				GuiRadioButton_setValue (b -> _toggle, i == _integerDefaultValue);
			}
		} break; case UI_OPTIONMENU: {
			#if gtk
				gtk_combo_box_set_active (GTK_COMBO_BOX (_cascadeButton), (_integerDefaultValue - 1));
			#elif motif
				for (int i = 1; i <= _options -> size; i ++) {
					UiOption *b = (UiOption *) _options -> item [i];
					XmToggleButtonSetState (b -> _toggle, i == _integerDefaultValue, False);
					if (i == _integerDefaultValue) {
						XtVaSetValues (_cascadeButton, motif_argXmString (XmNlabelString, Melder_peekWcsToUtf8 (b - > _name)), NULL);
					}
				}
			#endif
		} break; case UI_LIST: {
			GuiList_selectItem (_list, _integerDefaultValue);
		}
	}
}

int UiForm::UiField::colourToValue (wchar_t *string) {
	wchar_t *p = string;
	while (*p == ' ' || *p == '\t') p ++;
	*p = tolower (*p);
	int first = *p;
	if (first == '{') {
		_colourValue. red = Melder_atof (++ p);
		p = (wchar *) wcschr (p, ',');
		if (p == NULL) return 0;
		_colourValue. green = Melder_atof (++ p);
		p = (wchar *) wcschr (p, ',');
		if (p == NULL) return 0;
		_colourValue. blue = Melder_atof (++ p);
	} else {
		*p = tolower (*p);
		if (wcsequ (p, L"black")) _colourValue = Graphics_BLACK;
		else if (wcsequ (p, L"white")) _colourValue = Graphics_WHITE;
		else if (wcsequ (p, L"red")) _colourValue = Graphics_RED;
		else if (wcsequ (p, L"green")) _colourValue = Graphics_GREEN;
		else if (wcsequ (p, L"blue")) _colourValue = Graphics_BLUE;
		else if (wcsequ (p, L"yellow")) _colourValue = Graphics_YELLOW;
		else if (wcsequ (p, L"cyan")) _colourValue = Graphics_CYAN;
		else if (wcsequ (p, L"magenta")) _colourValue = Graphics_MAGENTA;
		else if (wcsequ (p, L"maroon")) _colourValue = Graphics_MAROON;
		else if (wcsequ (p, L"lime")) _colourValue = Graphics_LIME;
		else if (wcsequ (p, L"navy")) _colourValue = Graphics_NAVY;
		else if (wcsequ (p, L"teal")) _colourValue = Graphics_TEAL;
		else if (wcsequ (p, L"purple")) _colourValue = Graphics_PURPLE;
		else if (wcsequ (p, L"olive")) _colourValue = Graphics_OLIVE;
		else if (wcsequ (p, L"pink")) _colourValue = Graphics_PINK;
		else if (wcsequ (p, L"silver")) _colourValue = Graphics_SILVER;
		else if (wcsequ (p, L"grey")) _colourValue = Graphics_GREY;
		else { *p = first; return 0; }
		*p = first;
	}
	return 1;
}

int UiForm::UiField::widgetToValue () {
	switch (_type) {
		case UI_REAL: case UI_REAL_OR_UNDEFINED: case UI_POSITIVE: {
			wchar_t *dirty = GuiText_getString (_text);   /* The text as typed by the user. */
			if (! Interpreter(NULL).numericExpression (dirty, & _realValue)) { Melder_free (dirty); return 0; }
			Melder_free (dirty);
			/*
			 * Put a clean version of the new value in the form.
			 * If the value is equal to the default, make sure that any default comments are included.
			 */
			if (_realValue == Melder_atof (_stringDefaultValue)) {
				GuiText_setString (_text, _stringDefaultValue);
			} else {
				wchar_t clean [40];
				wcscpy (clean, Melder_double (_realValue));
				/*
				 * If the default is overtly real, the shown value must be as well.
				 */
				if ((wcschr (_stringDefaultValue, '.') || wcschr (_stringDefaultValue, 'e')) &&
					! (wcschr (clean, '.') || wcschr (clean, 'e')))
				{
					wcscat (clean, L".0");
				}
				GuiText_setString (_text, clean);
			}
			if (_realValue == NUMundefined && _type != UI_REAL_OR_UNDEFINED)
				return Melder_error3 (L"`", _name, L"' has the value \"undefined\".");
			if (_type == UI_POSITIVE && _realValue <= 0.0)
				return Melder_error3 (L"`", _name, L"' must be greater than 0.0.");
		} break; case UI_INTEGER: case UI_NATURAL: case UI_CHANNEL: {
			wchar_t *dirty = GuiText_getString (_text);
			if (_type == UI_CHANNEL && (wcsequ (dirty, L"Left") || wcsequ (dirty, L"Mono"))) {
				_integerValue = 1;
			} else if (_type == UI_CHANNEL && (wcsequ (dirty, L"Right") || wcsequ (dirty, L"Stereo"))) {
				_integerValue = 2;
			} else {
				double realValue;
				if (! Interpreter(NULL).numericExpression (dirty, & realValue)) { Melder_free (dirty); return 0; }
				_integerValue = floor (realValue + 0.5);
			}
			Melder_free (dirty);
			if (_integerValue == wcstol (_stringDefaultValue, NULL, 10)) {
				GuiText_setString (_text, _stringDefaultValue);
			} else {
				GuiText_setString (_text, Melder_integer (_integerValue));
			}
			if (_type == UI_NATURAL && _integerValue < 1)
				return Melder_error3 (L"`", _name, L"' must be a positive whole number.");
		} break; case UI_WORD: {
			Melder_free (_stringValue);
			_stringValue = GuiText_getString (_text);
			wchar_t *p = _stringValue;
			while (*p != '\0') { if (*p == ' ' || *p == '\t') *p = '\0'; p ++; }
			GuiText_setString (_text, _stringValue);
		} break; case UI_SENTENCE: case UI_TEXT: {
			Melder_free (_stringValue);
			_stringValue = GuiText_getString (_text);
		} break; case UI_BOOLEAN: {
			_integerValue = GuiCheckButton_getValue (_toggle);
		} break; case UI_RADIO: {
			_integerValue = 0;
			for (int i = 1; i <= _options -> size; i ++) {
				UiOption *b = (UiOption *)_options -> item [i];
				if (GuiRadioButton_getValue (b -> _toggle))
					_integerValue = i;
			}
			if (_integerValue == 0)
				return Melder_error3 (L"No option chosen for `", _name, L"'.");
		} break; case UI_OPTIONMENU: {
			_integerValue = 0;
			#if gtk
				// TODO: Graag even een check :)
				_integerValue = gtk_combo_box_get_active (GTK_COMBO_BOX (_cascadeButton)) + 1;
			#elif motif
			for (int i = 1; i <= _options -> size; i ++) {
				UiOption *b = (UiOption *)_options -> item [i];
				if (XmToggleButtonGetState (b -> _toggle))
					_integerValue = i;
			}
			#endif
			if (_integerValue == 0)
				return Melder_error3 (L"No option chosen for `", _name, L"'.");
		} break; case UI_LIST: {
			long numberOfSelected, *selected = GuiList_getSelectedPositions (_list, & numberOfSelected);
			if (selected == NULL) {
				Melder_warning1 (L"No items selected.");
				_integerValue = 1;
			} else {
				if (numberOfSelected > 1) Melder_warning1 (L"More than one item selected.");
				_integerValue = selected [1];
				NUMlvector_free (selected, 1);
			}
		} break; case UI_COLOUR: {
			wchar_t *string = GuiText_getString (_text);
			if (colourToValue (string)) {
				Melder_free (string);
			} else if (Interpreter(NULL).numericExpression (string, & _colourValue. red)) {
				_colourValue. green = _colourValue. blue = _colourValue. red;
				Melder_free (string);
			} else {
				Melder_free (string);
				return 0;
			}
		}
	}
	return 1;
}

int UiForm::UiField::stringToValue (const wchar_t *string, Interpreter *interpreter) {
	switch (_type) {
		case UI_REAL: case UI_REAL_OR_UNDEFINED: case UI_POSITIVE: {
			if (wcsspn (string, L" \t") == wcslen (string))
				return Melder_error3 (L"Argument `", _name, L"' empty.");
			if (! interpreter->numericExpression (string, & _realValue)) return 0;
			if (_realValue == NUMundefined && _type != UI_REAL_OR_UNDEFINED)
				return Melder_error3 (L"`", _name, L"' has the value \"undefined\".");
			if (_type == UI_POSITIVE && _realValue <= 0.0)
				return Melder_error3 (L"`", _name, L"' must be greater than 0.");
		} break; case UI_INTEGER: case UI_NATURAL: case UI_CHANNEL: {
			if (wcsspn (string, L" \t") == wcslen (string))
				return Melder_error3 (L"Argument `", _name, L"' empty.");
			if (_type == UI_CHANNEL && (wcsequ (string, L"All") || wcsequ (string, L"Average"))) {
				_integerValue = 0;
			} else if (_type == UI_CHANNEL && (wcsequ (string, L"Left") || wcsequ (string, L"Mono"))) {
				_integerValue = 1;
			} else if (_type == UI_CHANNEL && (wcsequ (string, L"Right") || wcsequ (string, L"Stereo"))) {
				_integerValue = 2;
			} else {
				double realValue;
				if (! interpreter->numericExpression (string, & realValue)) return 0;
				_integerValue = floor (realValue + 0.5);
			}
			if (_type == UI_NATURAL && _integerValue < 1)
				return Melder_error3 (L"`", _name, L"' must be a positive whole number.");
		} break; case UI_WORD: case UI_SENTENCE: case UI_TEXT: {
			Melder_free (_stringValue);
			_stringValue = Melder_wcsdup_f (string);
		} break; case UI_BOOLEAN: {
			if (! string [0]) return Melder_error1 (L"Empty argument for toggle button.");
			_integerValue = string [0] == '1' || string [0] == 'y' || string [0] == 'Y' ||
				string [0] == 't' || string [0] == 'T';
		} break; case UI_RADIO: case UI_OPTIONMENU: {
			_integerValue = 0;
			for (int i = 1; i <= _options -> size; i ++) {
				UiOption *b = (UiOption *)_options -> item [i];
				if (wcsequ (string, b -> _name))
					_integerValue = i;
			}
			if (_integerValue == 0) {
				/*
				 * Retry with different case.
				 */
				for (int i = 1; i <= _options -> size; i ++) {
					UiOption *b = (UiOption *)_options -> item [i];
					wchar_t name2 [100];
					wcscpy (name2, b -> _name);
					if (islower (name2 [0])) name2 [0] = toupper (name2 [0]);
					else if (isupper (name2 [0])) name2 [0] = tolower (name2 [0]);
					if (wcsequ (string, name2))
						_integerValue = i;
				}
			}
			if (_integerValue == 0) {
				return Melder_error5
					(L"Field `", _name, L"' cannot have the value \"", string, L"\".");
			}
		} break; case UI_LIST: {
			long i;
			for (i = 1; i <= _numberOfStrings; i ++)
				if (wcsequ (string, _strings [i])) break;
			if (i > _numberOfStrings) return Melder_error5
				(L"Field `", _name, L"' cannot have the value \"", string, L"\".");
			_integerValue = i;
		} break; case UI_COLOUR: {
			wchar_t *string2 = Melder_wcsdup_f (string);
			if (colourToValue (string2)) {
				Melder_free (string2);
			} else if (interpreter->numericExpression (string2, & _colourValue. red)) {
				_colourValue. green = _colourValue. blue = _colourValue. red;
				Melder_free (string2);
			} else {
				Melder_free (string2);
				return 0;
			}
		} break; default: {
			return 0;
		}
	}
	return 1;
}

UiForm::UiOption * UiForm::UiField::addRadio (const wchar_t *label) {
	Melder_assert (_type == UI_RADIO || _type == UI_OPTIONMENU);
	UiOption *thee = new UiOption (label);
	Collection_addItem (_options, thee);
	return thee;
}

UiForm::UiOption * UiForm::UiField::addOptionMenu (const wchar_t *label) {
	Melder_assert (_type == UI_RADIO || _type == UI_OPTIONMENU);
	UiOption *thee = new UiOption (label);
	Collection_addItem (_options, thee);
	return thee;
}

