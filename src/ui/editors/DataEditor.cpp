/* DataEditor.c
 *
 * Copyright (C) 1995-2011 Paul Boersma
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
 * pb 1998/05/17 mac 64-bit floats only
 * pb 1998/05/18 "verticalScrollBar"
 * pb 1998/10/22 removed now duplicate "parent" attribute
 * pb 1998/11/04 removed BUG: added assignment to "_parent"
 * pb 1999/06/21 removed BUG: dcomplex read %lf
 * pb 2003/05/19 Melder_atof
 * pb 2005/12/04 wider names
 * pb 2007/06/10 wchar_t
 * pb 2007/08/12 wchar_t
 * pb 2007/12/23 Gui
 * pb 2007/12/31 Gui
 * pb 2008/03/20 split off Help menu
 * pb 2008/03/21 new Editor API
 * pb 2008/07/20 wchar_t
 * pb 2011/03/03 removed stringwa
 */

#include "DataEditor.h"

#include "sys/Collection.h"
#include "ui/machine.h"
#include "ui/praatP.h"

#define NAME_X  30
#define TEXT_X  250
#define BUTTON_X  250
#define LIST_Y  (2 * Gui_TOP_DIALOG_SPACING + Gui_PUSHBUTTON_HEIGHT)
#define EDITOR_WIDTH  820
#define EDITOR_HEIGHT  (LIST_Y + MAXNUM_ROWS * ROW_HEIGHT + 29 + Machine_getMenuBarHeight ())
#define ROW_HEIGHT  31

#define SCROLL_BAR_WIDTH  Machine_getScrollBarWidth ()

/*static const char * typeStrings [] = { "none",
	"byte", "short", "int", "long", "ubyte", "ushort", "uint", "ulong", "bool",
	"float", "double", "fcomplex", "dcomplex", "char", "wchar",
	"enum", "lenum", "boolean", "question", "stringw", "lstringw",
	"struct", "widget", "object", "collection" };*/
static int stringLengths [] = { 0,
	4, 6, 6, 11, 3, 5, 5, 10, 1,
	15, 27, 35, 59, 4, 6,
	33, 33, 8, 6, 60, 60 };

/********** DataSubEditor ***********/

wchar_t * DataSubEditor::singleTypeToText (void *address, int type, void *tagType, MelderString *buffer) {
	switch (type) {
		case bytewa: MelderString_append1 (buffer, Melder_integer (* (signed char *) address)); break;
		case shortwa: MelderString_append1 (buffer, Melder_integer (* (short *) address)); break;
		case intwa: MelderString_append1 (buffer, Melder_integer (* (int *) address)); break;
		case longwa: MelderString_append1 (buffer, Melder_integer (* (long *) address)); break;
		case ubytewa: MelderString_append1 (buffer, Melder_integer (* (unsigned char *) address)); break;
		case ushortwa: MelderString_append1 (buffer, Melder_integer (* (unsigned short *) address)); break;
		case uintwa: MelderString_append1 (buffer, Melder_integer (* (unsigned int *) address)); break;
		case ulongwa: MelderString_append1 (buffer, Melder_integer (* (unsigned long *) address)); break;
		case boolwa: MelderString_append1 (buffer, Melder_integer (* (bool *) address)); break;
		case floatwa: MelderString_append1 (buffer, Melder_single (* (double *) address)); break;
		case doublewa: MelderString_append1 (buffer, Melder_double (* (double *) address)); break;
		case fcomplexwa: { fcomplex value = * (fcomplex *) address;
			MelderString_append4 (buffer, Melder_single (value. re), L" + ", Melder_single (value. im), L" i"); } break;
		case dcomplexwa: { dcomplex value = * (dcomplex *) address;
			MelderString_append4 (buffer, Melder_double (value. re), L" + ", Melder_double (value. im), L" i"); } break;
		case charwa: MelderString_append1 (buffer, Melder_integer (* (wchar_t *) address)); break;
		case enumwa: MelderString_append3 (buffer, L"<", ((const wchar_t * (*) (int)) tagType) (* (signed char *) address), L">"); break;
		case lenumwa: MelderString_append3 (buffer, L"<", ((const wchar_t * (*) (int)) tagType) (* (signed short *) address), L">"); break;
		case booleanwa: MelderString_append1 (buffer, * (signed char *) address ? L"<true>" : L"<false>"); break;
		case questionwa: MelderString_append1 (buffer, * (signed char *) address ? L"<yes>" : L"<no>"); break;
		case stringwa:
		case lstringwa: {
			wchar_t *string = * (wchar_t **) address;
			if (string == NULL) { MelderString_empty (buffer); return buffer -> string; }   // Convert NULL string to empty string.
			return string;   // May be much longer than the usual size of 'buffer'.
		} break;
		default: return L"(unknown)";
	}
	return buffer -> string;   // Mind the special return for strings above.
}

DataSubEditor::DataSubEditor (DataEditor *root, const wchar_t *title, void *address, Data_Description description)
	: Editor (root -> _parent, 0, 0, EDITOR_WIDTH, EDITOR_HEIGHT, title, NULL),
	  _root(root),
	  _address(address),
	  _description(description),
	  _topField(1),
	  _numberOfFields(countFields ()) {
	createMenus ();
	createChildren ();
	if (this != root) Collection_addItem (root -> _children, this);
	update ();
}

DataSubEditor::~DataSubEditor () {
	for (int i = 1; i <= MAXNUM_ROWS; i ++)
		Melder_free (_fieldData [i]. _history);
	if (_root && _root -> _children) for (int i = _root -> _children -> size; i > 0; i --)
		if (_root -> _children -> item [i] == this)
			Collection_subtractItem (_root -> _children, i);
}

void DataSubEditor::update () {
	/* Hide all the existing widgets. */

	for (int i = 1; i <= MAXNUM_ROWS; i ++) {
		_fieldData [i]. _address = NULL;
		_fieldData [i]. _description = NULL;
		GuiObject_hide (_fieldData [i]. _label);
		GuiObject_hide (_fieldData [i]. _button);
		GuiObject_hide (_fieldData [i]. _text);
	}

	_irow = 0;
	showMembers ();
}

Data_Description DataSubEditor::findNumberUse (const wchar_t *number) {
	Data_Description structDescription = getDescription (), result;
	wchar_t string [100];
	swprintf (string, 100, L"_%ls", number);
	if ((result = Data_Description_findNumberUse (structDescription, string)) != NULL) return result;
	swprintf (string, 100, L"_%ls - 1", number);
	if ((result = Data_Description_findNumberUse (structDescription, string)) != NULL) return result;
	return NULL;
}

void DataSubEditor::gui_button_cb_change (I, GuiButtonEvent event) {
	(void) event;
	DataSubEditor *editor = (DataSubEditor *)void_me;
	int i;   // has to be declared here
	for (i = 1; i <= MAXNUM_ROWS; i ++) {
	
	#if motif
	if (XtIsManaged (editor->_fieldData [i]. _text)) {
	#elif gtk
	gboolean visible;
	g_object_get(G_OBJECT(editor->_fieldData[i]. _text), "visible", &visible, NULL);
	if (visible) {
	#endif
		int type = editor->_fieldData [i]. _description -> type;
		wchar_t *text;
		if (type > maxsingletypewa) continue;
		text = GuiText_getString (editor->_fieldData [i]. _text);
		switch (type) {
			case bytewa: {
				signed char oldValue = * (signed char *) editor->_fieldData [i]. _address, newValue = wcstol (text, NULL, 10);
				if (newValue != oldValue) {
					Data_Description numberUse = editor->findNumberUse (editor->_fieldData [i]. _description -> name);
					if (numberUse) {
						Melder_error5 (L"Changing field \"", editor->_fieldData [i]. _description -> name,
							L"\" would damage the array \"", numberUse -> name, L"\".");
						Melder_flushError (NULL);
					} else {
						* (signed char *) editor->_fieldData [i]. _address = newValue;
					}
				}
			} break;
			case shortwa: {
				short oldValue = * (short *) editor->_fieldData [i]. _address, newValue = wcstol (text, NULL, 10);
				if (newValue != oldValue) {
					Data_Description numberUse = editor->findNumberUse (editor->_fieldData [i]. _description -> name);
					if (numberUse) {
						Melder_error5 (L"Changing field \"", editor->_fieldData [i]. _description -> name,
							L"\" would damage the array \"", numberUse -> name, L"\".");
						Melder_flushError (NULL);
					} else {
						* (short *) editor->_fieldData [i]. _address = newValue;
					}
				}
			} break;
			case intwa: {
				int oldValue = * (int *) editor->_fieldData [i]. _address, newValue = wcstol (text, NULL, 10);
				if (newValue != oldValue) {
					Data_Description numberUse = editor->findNumberUse (editor->_fieldData [i]. _description -> name);
					if (numberUse) {
						Melder_error5 (L"Changing field \"", editor->_fieldData [i]. _description -> name,
							L"\" would damage the array \"", numberUse -> name, L"\".");
						Melder_flushError (NULL);
					} else {
						* (int *) editor->_fieldData [i]. _address = newValue;
					}
				}
			} break;
			case longwa: {
				long oldValue = * (long *) editor->_fieldData [i]. _address, newValue = wcstol (text, NULL, 10);
				if (newValue != oldValue) {
					Data_Description numberUse = editor->findNumberUse (editor->_fieldData [i]. _description -> name);
					if (numberUse) {
						Melder_error5 (L"Changing field \"", editor->_fieldData [i]. _description -> name,
							L"\" would damage the array \"", numberUse -> name, L"\".");
						Melder_flushError (NULL);
					} else {
						* (long *) editor->_fieldData [i]. _address = newValue;
					}
				}
			} break;
			case ubytewa: { * (unsigned char *) editor->_fieldData [i]. _address = wcstoul (text, NULL, 10); } break;
			case ushortwa: { * (unsigned short *) editor->_fieldData [i]. _address = wcstoul (text, NULL, 10); } break;
			case uintwa: { * (unsigned int *) editor->_fieldData [i]. _address = wcstoul (text, NULL, 10); } break;
			case ulongwa: { * (unsigned long *) editor->_fieldData [i]. _address = wcstoul (text, NULL, 10); } break;
			case boolwa: { * (bool *) editor->_fieldData [i]. _address = wcstol (text, NULL, 10); } break;
			case floatwa: { * (double *) editor->_fieldData [i]. _address = Melder_atof (text); } break;
			case doublewa: { * (double *) editor->_fieldData [i]. _address = Melder_atof (text); } break;
			case fcomplexwa: { fcomplex *x = (fcomplex *) editor->_fieldData [i]. _address;
				swscanf (text, L"%f + %f i", & x -> re, & x -> im); } break;
			case dcomplexwa: { dcomplex *x = (dcomplex *) editor->_fieldData [i]. _address;
				swscanf (text, L"%lf + %lf i", & x -> re, & x -> im); } break;
			case charwa: { * (char *) editor->_fieldData [i]. _address = wcstol (text, NULL, 10); } break;
			case enumwa: {
				if (wcslen (text) < 3) goto error;
				text [wcslen (text) - 1] = '\0';   /* Remove trailing ">". */
				int value = ((int (*) (const wchar_t *)) (editor->_fieldData [i]. _description -> tagType)) (text + 1);   /* Skip leading "<". */
				if (value < 0) goto error;
				* (signed char *) editor->_fieldData [i]. _address = value;
			} break;
			case lenumwa: {
				if (wcslen (text) < 3) goto error;
				text [wcslen (text) - 1] = '\0';   /* Remove trailing ">". */
				int value = ((int (*) (const wchar_t *)) (editor->_fieldData [i]. _description -> tagType)) (text + 1);   /* Skip leading "<". */
				if (value < 0) goto error;
				* (signed short *) editor->_fieldData [i]. _address = value;
			} break;
			case booleanwa: {
				int value = wcsnequ (text, L"<true>", 6) ? 1 : wcsnequ (text, L"<false>", 7) ? 0 : -1;
				if (value < 0) goto error;
				* (signed char *) editor->_fieldData [i]. _address = value;
			} break;
			case questionwa: {
				int value = wcsnequ (text, L"<yes>", 5) ? 1 : wcsnequ (text, L"<no>", 4) ? 0 : -1;
				if (value < 0) goto error;
				* (signed char *) editor->_fieldData [i]. _address = value;
			} break;
			case stringwa:
			case lstringwa: {
				wchar_t *old = * (wchar_t **) editor->_fieldData [i]. _address;
				Melder_free (old);
				* (wchar_t **) editor->_fieldData [i]. _address = Melder_wcsdup_f (text);
			} break;
			default: break;
		}
		Melder_free (text);
	#if motif || gtk
	}
	#endif
	}
	/* Several collaborators have to be notified of this change:
	 * 1. The owner (creator) of our root DataEditor: so that she can notify other editors, if any.
	 * 2. All our sibling DataSubEditors.
	 */
	if (editor->_root -> _dataChangedCallback)
		editor->_root -> _dataChangedCallback (editor->_root, editor->_root -> _dataChangedClosure, NULL);   /* Notify owner. */
	editor->update ();
	for (i = 1; i <= editor->_root -> _children -> size; i ++) {
		DataSubEditor *subeditor = (DataSubEditor*)editor->_root -> _children -> item [i];
		if (subeditor != editor) subeditor->update ();
	}
	return;
error:
	Melder_error3 (L"Edit field \"", editor->_fieldData [i]. _description -> name, L"\" or click \"Cancel\".");
	Melder_flushError (NULL);
}

void DataSubEditor::gui_button_cb_cancel (I, GuiButtonEvent event) {
	(void) event;
	((DataSubEditor *)void_me)->update ();
}

void DataSubEditor::gui_cb_scroll (GUI_ARGS) {
	DataSubEditor *editor = (DataSubEditor *)void_me;
	int value, slider, incr, pincr;
	#if motif
		XmScrollBarGetValues (w, & value, & slider, & incr, & pincr);
	#endif
	editor->_topField = value + 1;
	editor->update ();
}

void DataSubEditor::gui_button_cb_open (I, GuiButtonEvent event) {
	DataSubEditor *editor = (DataSubEditor *)void_me;
	int ifield = 0;
	static MelderString name = { 0 };
	MelderString_empty (& name);
	DataSubEditor::FieldData *fieldData;

	/* Identify the pressed button; it must be one of those created in the list. */

	for (int i = 1; i <= MAXNUM_ROWS; i ++) if (editor->_fieldData [i]. _button == event -> button) { ifield = i; break; }
	Melder_assert (ifield != 0);

	/* Launch the appropriate subeditor. */

	fieldData = & editor->_fieldData [ifield];
	if (! fieldData -> _description) {
		Melder_casual ("Not yet implemented.");
		return;   /* Not yet implemented. */
	}

	if (fieldData -> _description -> rank == 1 || fieldData -> _description -> rank == 3 || fieldData -> _description -> rank < 0) {
		MelderString_append8 (& name, fieldData -> _history, L". ", fieldData -> _description -> name,
			L" [", Melder_integer (fieldData -> _minimum), L"..", Melder_integer (fieldData -> _maximum), L"]");
		if (! new VectorEditor (editor->_root, name.string, fieldData -> _address,
			fieldData -> _description, fieldData -> _minimum, fieldData -> _maximum)) Melder_flushError (NULL);
	} else if (fieldData -> _description -> rank == 2) {
		MelderString_append8 (& name, fieldData -> _history, L". ", fieldData -> _description -> name,
			L" [", Melder_integer (fieldData -> _minimum), L"..", Melder_integer (fieldData -> _maximum), L"]");
		MelderString_append5 (& name, L" [", Melder_integer (fieldData -> _min2), L"..", Melder_integer (fieldData -> _max2), L"]");
		if (! new MatrixEditor (editor->_root, name.string, fieldData -> _address, fieldData -> _description,
			fieldData -> _minimum, fieldData -> _maximum, fieldData -> _min2, fieldData -> _max2)) Melder_flushError (NULL);
	} else if (fieldData -> _description -> type == structwa) {
		MelderString_append3 (& name, fieldData -> _history, L". ", fieldData -> _description -> name);
		if (! new StructEditor (editor->_root, name.string, fieldData -> _address,
			(Data_Description) fieldData -> _description -> tagType)) Melder_flushError (NULL);
	} else if (fieldData -> _description -> type == objectwa || fieldData -> _description -> type == collectionwa) {
		MelderString_append3 (& name, fieldData -> _history, L". ", fieldData -> _description -> name);
		if (! new ClassEditor (editor->_root, name.string, fieldData -> _address,
			((Data_Table) fieldData -> _description -> tagType) -> description)) Melder_flushError (NULL);
	} else /*if (fieldData -> _description -> type == inheritwa)*/ {
		if (! new ClassEditor (editor->_root, fieldData -> _history, fieldData -> _address,
			fieldData -> _description)) Melder_flushError (NULL);
/*	} else {
		Melder_casual ("Strange editor \"%s\" required (type %d, rank %d).",
			fieldData -> _description -> name,
			fieldData -> _description -> type,
			fieldData -> _description -> rank);*/
	}
}

void DataSubEditor::createChildren () {
	int x = Gui_LEFT_DIALOG_SPACING, y = Gui_TOP_DIALOG_SPACING + Machine_getMenuBarHeight (), buttonWidth = 120;

	#if motif
	GuiButton_createShown (_dialog, x, x + buttonWidth, y, Gui_AUTOMATIC,
		L"Change", gui_button_cb_change, me, 0);
	x += buttonWidth + Gui_HORIZONTAL_DIALOG_SPACING;
	GuiButton_createShown (_dialog, x, x + buttonWidth, y, Gui_AUTOMATIC,
		L"Cancel", gui_button_cb_cancel, me, 0);
	
	GuiObject scrolledWindow = XmCreateScrolledWindow (_dialog, "list", NULL, 0);
	XtVaSetValues (scrolledWindow, 
		XmNrightAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_FORM, XmNtopOffset, LIST_Y + Machine_getMenuBarHeight (),
		XmNbottomAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM, NULL);

	_scrollBar = XtVaCreateManagedWidget ("verticalScrollBar",
		xmScrollBarWidgetClass, scrolledWindow,
		XmNheight, SCROLL_BAR_WIDTH,
		XmNminimum, 0,
		XmNmaximum, _numberOfFields,
		XmNvalue, 0,
		XmNsliderSize, _numberOfFields < MAXNUM_ROWS ? _numberOfFields : MAXNUM_ROWS,
		XmNincrement, 1, XmNpageIncrement, MAXNUM_ROWS - 1,
		NULL);
	XtVaSetValues (scrolledWindow, XmNverticalScrollBar, _scrollBar, NULL);
	GuiObject_show (scrolledWindow);
	XtAddCallback (_scrollBar, XmNvalueChangedCallback, gui_cb_scroll, (XtPointer) me);
	XtAddCallback (_scrollBar, XmNdragCallback, gui_cb_scroll, (XtPointer) me);
	GuiObject form = XmCreateForm (scrolledWindow, "list", NULL, 0);
	
	#elif gtk
	GuiObject outerBox = gtk_vbox_new(0, 0);
	GuiObject buttonBox = gtk_hbutton_box_new();
	gtk_button_box_set_layout(GTK_BUTTON_BOX(buttonBox), GTK_BUTTONBOX_START);
	gtk_box_pack_start(GTK_BOX(outerBox), buttonBox, 0, 0, 3);
	
	GuiButton_createShown (buttonBox, x, x + buttonWidth, y, Gui_AUTOMATIC,
		L"Change", gui_button_cb_change, this, 0);
	x += buttonWidth + Gui_HORIZONTAL_DIALOG_SPACING;
	GuiButton_createShown (buttonBox, x, x + buttonWidth, y, Gui_AUTOMATIC,
		L"Cancel", gui_button_cb_cancel, this, 0);
	
	GuiObject scrolledWindow = gtk_scrolled_window_new(NULL, NULL);
	
	GuiObject form = gtk_vbox_new(0, 3);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolledWindow), form);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledWindow), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_box_pack_start(GTK_BOX(outerBox), scrolledWindow, 1, 1, 3);
	gtk_container_add(GTK_CONTAINER(_dialog), outerBox);
	
	GuiObject_show(outerBox);
	GuiObject_show(buttonBox);
	GuiObject_show(scrolledWindow);
	#endif
	
	for (int i = 1; i <= MAXNUM_ROWS; i ++) {
		y = Gui_TOP_DIALOG_SPACING + (i - 1) * ROW_HEIGHT;
		_fieldData[i]._label = GuiLabel_create(form, 0, 200, y, Gui_AUTOMATIC, L"label", 0);   /* No fixed x value: sometimes indent. */
		_fieldData[i]._button = GuiButton_create(form, BUTTON_X, BUTTON_X + buttonWidth, y, Gui_AUTOMATIC,
			L"Open", gui_button_cb_open, this, 0);
		_fieldData[i]._text = GuiText_create(form, TEXT_X, 0, y, Gui_AUTOMATIC, 0);
	}
	
	GuiObject_show (form);
}

int DataSubEditor::menu_cb_help (EDITOR_ARGS) { Melder_help (L"Inspect"); return 1; }

void DataSubEditor::createMenus () {
	EditorMenu *menu = getMenu (L"Help");
	menu->addCommand (L"DataEditor *help", '?', menu_cb_help);
}

long DataSubEditor::countFields () { return 0; }

void DataSubEditor::showStructMember (
	void *structAddress,   /* The address of (the first member of) the struct. */
	Data_Description structDescription,   /* The description of (the first member of) the struct. */
	Data_Description memberDescription,   /* The description of the current member. */
	DataSubEditor::FieldData *fieldData,   /* The widgets in which to show the info about the current member. */
	wchar_t *history)
{
	int type = memberDescription -> type, rank = memberDescription -> rank, isSingleType = type <= maxsingletypewa && rank == 0;
	unsigned char *memberAddress = (unsigned char *) structAddress + memberDescription -> offset;
	static MelderString buffer = { 0 };
	MelderString_empty (& buffer);
	if (type == inheritwa) {
		MelderString_append3 (& buffer, L"Class part \"", memberDescription -> name, L"\":");
	} else {
		MelderString_append2 (& buffer, memberDescription -> name,
			rank == 0 ? L"" : rank == 1 || rank == 3 || rank < 0 ? L" [ ]" : L" [ ] [ ]");
	}
	GuiLabel_setString (fieldData -> _label, buffer.string);
	GuiObject_move (fieldData -> _label, type == inheritwa ? 0 : NAME_X, Gui_AUTOMATIC);
	GuiObject_show (fieldData -> _label);

	/* Show the value (for a single type) or a button (for a composite type). */

	if (isSingleType) {
		#if motif
			XtVaSetValues (fieldData -> _text, XmNcolumns, stringLengths [type], NULL);   // TODO: change to GuiObject_size
		#endif
		MelderString_empty (& buffer);
		wchar_t *text = singleTypeToText (memberAddress, type, memberDescription -> tagType, & buffer);
		GuiText_setString (fieldData -> _text, text);
		GuiObject_show (fieldData -> _text);
		fieldData -> _address = memberAddress;
		fieldData -> _description = memberDescription;
		fieldData -> _rank = 0;
	} else if (rank == 1) {
		void *arrayAddress = * (void **) memberAddress;
		long minimum, maximum;
		if (arrayAddress == NULL) return;   /* No button for empty fields. */
		Data_Description_evaluateInteger (structAddress, structDescription,
			memberDescription -> min1, & minimum);
		Data_Description_evaluateInteger (structAddress, structDescription,
			memberDescription -> max1, & maximum);
		if (maximum < minimum) return;   /* No button if no elements. */
		fieldData -> _address = arrayAddress;   /* Indirect. */
		fieldData -> _description = memberDescription;
		fieldData -> _minimum = minimum;   /* Normally 1. */
		fieldData -> _maximum = maximum;
		fieldData -> _rank = 1;
		Melder_free (fieldData -> _history); fieldData -> _history = Melder_wcsdup_f (history);
		GuiObject_show (fieldData -> _button);
	} else if (rank < 0) {
		/*
		 * This represents an in-line array.
		 */
		long maximum;   /* But: capacity = - rank */
		Data_Description_evaluateInteger (structAddress, structDescription,
			memberDescription -> max1, & maximum);
		if (-- maximum < 0) return;   /* Subtract one for zero-based array; no button if no elements. */
		fieldData -> _address = memberAddress;   /* Direct. */
		fieldData -> _description = memberDescription;
		fieldData -> _minimum = 0;   /* In-line arrays start with index 0. */
		fieldData -> _maximum = maximum;   /* Probably between -1 and capacity - 1. */
		fieldData -> _rank = rank;
		Melder_free (fieldData -> _history); fieldData -> _history = Melder_wcsdup_f (history);
		GuiObject_show (fieldData -> _button);
	} else if (rank == 3) {
		/*
		 * This represents an in-line set.
		 */
		fieldData -> _address = memberAddress;   /* Direct. */
		fieldData -> _description = memberDescription;
		fieldData -> _minimum = wcsequ (((const wchar_t * (*) (int)) memberDescription -> min1) (0), L"_") ? 1 : 0;
		fieldData -> _maximum = ((int (*) (const wchar_t *)) memberDescription -> max1) (L"\n");
		fieldData -> _rank = rank;
		Melder_free (fieldData -> _history); fieldData -> _history = Melder_wcsdup_f (history);
		GuiObject_show (fieldData -> _button);
	} else if (rank == 2) {
		void *arrayAddress = * (void **) memberAddress;
		long min1, max1, min2, max2;
		if (arrayAddress == NULL) return;   /* No button for empty fields. */
		Data_Description_evaluateInteger (structAddress, structDescription,
			memberDescription -> min1,  & min1);
		Data_Description_evaluateInteger (structAddress, structDescription,
			memberDescription -> max1, & max1);
		Data_Description_evaluateInteger (structAddress, structDescription,
			memberDescription -> min2,  & min2);
		Data_Description_evaluateInteger (structAddress, structDescription,
			memberDescription -> max2, & max2);
		if (max1 < min1 || max2 < min2) return;   /* No button if no elements. */
		fieldData -> _address = arrayAddress;   /* Indirect. */
		fieldData -> _description = memberDescription;
		fieldData -> _minimum = min1;   /* Normally 1. */
		fieldData -> _maximum = max1;
		fieldData -> _min2 = min2;
		fieldData -> _max2 = max2;
		fieldData -> _rank = 2;
		Melder_free (fieldData -> _history); fieldData -> _history = Melder_wcsdup_f (history);
		GuiObject_show (fieldData -> _button);
	} else if (type == structwa) {   /* In-line struct. */
		fieldData -> _address = memberAddress;   /* Direct. */
		fieldData -> _description = memberDescription;
		fieldData -> _rank = 0;
		Melder_free (fieldData -> _history); fieldData -> _history = Melder_wcsdup_f (history);
		GuiObject_show (fieldData -> _button);
	} else if (type == objectwa || type == collectionwa) {
		fieldData -> _address = * (Data *) memberAddress;   /* Indirect. */
		if (! fieldData -> _address) return;   /* No button if no object. */
		fieldData -> _description = memberDescription;
		fieldData -> _rank = 0;
		Melder_free (fieldData -> _history); fieldData -> _history = Melder_wcsdup_f (history);
		GuiObject_show (fieldData -> _button);
	}
}

void DataSubEditor::showStructMembers (void *structAddress, Data_Description structDescription, int fromMember, wchar_t *history) {
	int i;
	Data_Description memberDescription;
	for (i = 1, memberDescription = structDescription;
	     i < fromMember && memberDescription -> name != NULL;
	     i ++, memberDescription ++)
		(void) 0;
	for (; memberDescription -> name != NULL; memberDescription ++) {
		if (++ _irow > MAXNUM_ROWS) return;
		showStructMember (structAddress, structDescription, memberDescription, & _fieldData [_irow], history);
	}
}

void DataSubEditor::showMembers () {}

/********** StructEditor **********/

StructEditor::StructEditor (DataEditor *root, const wchar_t *title, void *address, Data_Description description)
	: DataSubEditor (root, title, address, description) {}

long StructEditor::countFields () {
	return Data_Description_countMembers (_description);
}

static wchar_t * singleTypeToText (void *address, int type, void *tagType, MelderString *buffer) {
	switch (type) {
		case bytewa: MelderString_append1 (buffer, Melder_integer (* (signed char *) address)); break;
		case shortwa: MelderString_append1 (buffer, Melder_integer (* (short *) address)); break;
		case intwa: MelderString_append1 (buffer, Melder_integer (* (int *) address)); break;
		case longwa: MelderString_append1 (buffer, Melder_integer (* (long *) address)); break;
		case ubytewa: MelderString_append1 (buffer, Melder_integer (* (unsigned char *) address)); break;
		case ushortwa: MelderString_append1 (buffer, Melder_integer (* (unsigned short *) address)); break;
		case uintwa: MelderString_append1 (buffer, Melder_integer (* (unsigned int *) address)); break;
		case ulongwa: MelderString_append1 (buffer, Melder_integer (* (unsigned long *) address)); break;
		case boolwa: MelderString_append1 (buffer, Melder_integer (* (bool *) address)); break;
		case floatwa: MelderString_append1 (buffer, Melder_single (* (double *) address)); break;
		case doublewa: MelderString_append1 (buffer, Melder_double (* (double *) address)); break;
		case fcomplexwa: { fcomplex value = * (fcomplex *) address;
			MelderString_append4 (buffer, Melder_single (value. re), L" + ", Melder_single (value. im), L" i"); } break;
		case dcomplexwa: { dcomplex value = * (dcomplex *) address;
			MelderString_append4 (buffer, Melder_double (value. re), L" + ", Melder_double (value. im), L" i"); } break;
		case charwa: MelderString_append1 (buffer, Melder_integer (* (wchar_t *) address)); break;
		case enumwa: MelderString_append3 (buffer, L"<", ((const wchar_t * (*) (int)) tagType) (* (signed char *) address), L">"); break;
		case lenumwa: MelderString_append3 (buffer, L"<", ((const wchar_t * (*) (int)) tagType) (* (signed short *) address), L">"); break;
		case booleanwa: MelderString_append1 (buffer, * (signed char *) address ? L"<true>" : L"<false>"); break;
		case questionwa: MelderString_append1 (buffer, * (signed char *) address ? L"<yes>" : L"<no>"); break;
		case stringwa:
		case lstringwa: {
			wchar_t *string = * (wchar_t **) address;
			if (string == NULL) { MelderString_empty (buffer); return buffer -> string; }   // Convert NULL string to empty string.
			return string;   // May be much longer than the usual size of 'buffer'.
		} break;
		default: return (wchar_t*)L"(unknown)";
	}
	return buffer -> string;   // Mind the special return for strings above.
}

void StructEditor::showMembers () {
	showStructMembers (_address, _description, _topField, _name);
}

/********** VectorEditor **********/

VectorEditor::VectorEditor (DataEditor *root, const wchar_t *title, void *address,
	Data_Description description, long minimum, long maximum)
	: DataSubEditor (root, title, address, description),
	_minimum(minimum),
	_maximum(maximum) {}

long VectorEditor::countFields () {
	long numberOfElements = _maximum - _minimum + 1;
	if (_description -> type == structwa)
		return numberOfElements * (Data_Description_countMembers ((structData_Description*)_description -> tagType) + 1);
	else
		return numberOfElements;
}

void VectorEditor::showMembers () {
	long firstElement, ielement;
	int type = _description -> type, isSingleType = type <= maxsingletypewa;
	int elementSize = type == structwa ?
		Data_Description_countMembers ((structData_Description*)_description -> tagType) + 1 : 1;
	firstElement = _minimum + (_topField - 1) / elementSize;

	for (ielement = firstElement; ielement <= _maximum; ielement ++) {
		unsigned char *elementAddress = (unsigned char *) _address + ielement * _description -> size;
		static MelderString buffer = { 0 };
		MelderString_empty (& buffer);
		DataSubEditor::FieldData *fieldData;
		int skip = ielement == firstElement ? (_topField - 1) % elementSize : 0;

		if (++ _irow > MAXNUM_ROWS) return;
		fieldData = & _fieldData [_irow];

		if (isSingleType) {
			MelderString_append4 (& buffer, _description -> name, L" [",
				_description -> rank == 3 ? ((const wchar_t * (*) (int)) _description -> min1) (ielement) : Melder_integer (ielement), L"]");
			GuiObject_move (fieldData -> _label, 0, Gui_AUTOMATIC);
			GuiLabel_setString (fieldData -> _label, buffer.string);
			GuiObject_show (fieldData -> _label);

			MelderString_empty (& buffer);
			wchar_t *text = singleTypeToText (elementAddress, type, _description -> tagType, & buffer);
			#if motif
				XtVaSetValues (fieldData -> _text, XmNcolumns, stringLengths [type], NULL);   // TODO: change to GuiObject_size
			#endif
			GuiText_setString (fieldData -> _text, text);
			GuiObject_show (fieldData -> _text);
			fieldData -> _address = elementAddress;
			fieldData -> _description = _description;
		} else if (type == structwa) {
			static MelderString history = { 0 };
			MelderString_copy (& history, _name);

			/* Replace things like [1..100] by things like [19]. */

			if (history.string [history.length - 1] == ']') {
				wchar_t *openingBracket = wcsrchr (history.string, '[');
				Melder_assert (openingBracket != NULL);
				* openingBracket = '\0';
				history.length = openingBracket - history.string;
			}
			MelderString_append3 (& history, L"[", Melder_integer (ielement), L"]");

			if (skip) {
				_irow --;
			} else {
				MelderString_append4 (& buffer, _description -> name, L" [", Melder_integer (ielement), L"]: ---------------------------");
				GuiObject_move (fieldData -> _label, 0, Gui_AUTOMATIC);
				GuiLabel_setString (fieldData -> _label, buffer.string);
				GuiObject_show (fieldData -> _label);
			}
			showStructMembers (elementAddress, (structData_Description*)_description -> tagType, skip, history.string);
		} else if (type == objectwa) {
			static MelderString history = { 0 };
			MelderString_copy (& history, _name);
			if (history.string [history.length - 1] == ']') {
				wchar_t *openingBracket = wcsrchr (history.string, '[');
				Melder_assert (openingBracket != NULL);
				* openingBracket = '\0';
				history.length = openingBracket - history.string;
			}
			MelderString_append3 (& history, L"[", Melder_integer (ielement), L"]");

			MelderString_append4 (& buffer, _description -> name, L" [", Melder_integer (ielement), L"]");
			GuiObject_move (fieldData -> _label, 0, Gui_AUTOMATIC);
			GuiLabel_setString (fieldData -> _label, buffer.string);
			GuiObject_show (fieldData -> _label);

			Data object = * (Data *) elementAddress;
			if (object == NULL) return;   /* No button if no object. */
			if (! object -> methods -> description) return;   /* No button if no description for this class. */
			fieldData -> _address = object;
			fieldData -> _description = object -> methods -> description;
			fieldData -> _rank = 0;
			if (fieldData -> _history) Melder_free (fieldData -> _history);
			fieldData -> _history = Melder_wcsdup_f (history.string);
			GuiObject_show (fieldData -> _button);			
		}
	}
}

/********** MatrixEditor **********/

MatrixEditor::MatrixEditor (DataEditor *root, const wchar_t *title, void *address,
	Data_Description description, long min1, long max1, long min2, long max2)
	: DataSubEditor (root, title, address, description),
	_minimum(min1),
	_maximum(max1),
	_min2(min2),
	_max2(max2) {}

long MatrixEditor::countFields () {
	long numberOfElements = (_maximum - _minimum + 1) * (_max2 - _min2 + 1);
	if (_description -> type == structwa)
		return numberOfElements * (Data_Description_countMembers ((structData_Description*)_description -> tagType) + 1);
	else
		return numberOfElements;
}

void MatrixEditor::showMembers () {
	long firstRow, firstColumn;
	int type = _description -> type, isSingleType = type <= maxsingletypewa;
	int elementSize = type == structwa ?
		Data_Description_countMembers ((structData_Description*)_description -> tagType) + 1 : 1;
	int rowSize = elementSize * (_max2 - _min2 + 1);
	firstRow = _minimum + (_topField - 1) / rowSize;
	firstColumn = _min2 + (_topField - 1 - (firstRow - _minimum) * rowSize) / elementSize;

	for (long irow = firstRow; irow <= _maximum; irow ++)
	for (long icolumn = irow == firstRow ? firstColumn : _min2; icolumn <= _max2; icolumn ++) {
		unsigned char *elementAddress = * ((unsigned char **) _address + irow) + icolumn * _description -> size;
		DataSubEditor::FieldData *fieldData;

		if (++ _irow > MAXNUM_ROWS) return;
		fieldData = & _fieldData [_irow];
		
		if (isSingleType) {
			static MelderString buffer = { 0 };
			MelderString_empty (& buffer);
			MelderString_append6 (& buffer, _description -> name, L" [", Melder_integer (irow), L"] [", Melder_integer (icolumn), L"]");
			GuiObject_move (fieldData -> _label, 0, Gui_AUTOMATIC);
			GuiLabel_setString (fieldData -> _label, buffer.string);
			GuiObject_show (fieldData -> _label);

			MelderString_empty (& buffer);
			wchar_t *text = singleTypeToText (elementAddress, type, _description -> tagType, & buffer);
			#if motif
				XtVaSetValues (fieldData -> _text, XmNcolumns, stringLengths [type], NULL);   // TODO: change to GuiObject_size
			#endif
			GuiText_setString (fieldData -> _text, text);
			GuiObject_show (fieldData -> _text);
			fieldData -> _address = elementAddress;
			fieldData -> _description = _description;
		}
	}
}

/********** ClassEditor **********/

ClassEditor::ClassEditor (DataEditor *root, const wchar_t *title, void *address, Data_Description description)
	: StructEditor (root, title, address, description) {
	/*if (description == NULL) error3 // FIXME exception
		(L"(ClassEditor_init:) Class ", Thing_className (address), L" cannot be inspected.");*/
}

void ClassEditor::showMembers_recursive (void *klas) {
	Data_Table parentClass = (Data_Table) ((Data_Table) klas) -> _parent;
	Data_Description description = ((Data_Table) klas) -> description;
	int classFieldsTraversed = 0;
	while (parentClass -> description == description)
		parentClass = (Data_Table) parentClass -> _parent;
	if (parentClass != classData) {
		showMembers_recursive (parentClass);
		classFieldsTraversed = Data_Description_countMembers (parentClass -> description);
	}
	showStructMembers (_address, description, _irow ? 1 : _topField - classFieldsTraversed, _name);
}

void ClassEditor::showMembers () {
	showMembers_recursive (((Data) _address) -> methods);
}

/********** DataEditor ***********/

DataEditor::DataEditor (GuiObject parent, const wchar_t *title, Any data)
	: ClassEditor (this, title, data, ((Data) data) -> methods -> description),
	_children(Collection_create (NULL, 10)) {
	Data_Table klas = ((Data) data) -> methods;
	_parent = parent; // FIXME may have to be set before call to superclass constructor
	/*if (klas -> description == NULL) // FIXME exception
		(L"(DataEditor_create:) Class ", klas -> _className, L" cannot be inspected.");*/
}

DataEditor::~DataEditor () {
	int i;

	/* Tell _children not to notify me when they die. */

	for (i = 1; i <= _children -> size; i ++) {
		DataSubEditor *child = (DataSubEditor*)_children -> item [i];
		child -> _root = NULL;
	}

	forget (_children);
}

void DataEditor::dataChanged () {
	/*
	 * Someone else changed our data.
	 * We know that the top-level data is still accessible.
	 */
	update ();
	/*
	 * But all structure may have changed,
	 * so that we do not know if any of the subeditors contain valid data.
	 */
	for (int i = _children -> size; i >= 1; i --) {
		DataSubEditor *subeditor = (DataSubEditor*)_children -> item [i];
		Collection_subtractItem (_children, i);
		forget (subeditor);
	}
}

/* End of file DataEditor.c */
