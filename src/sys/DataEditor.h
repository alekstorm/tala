#ifndef _DataEditor_h_
#define _DataEditor_h
/* DataEditor.h
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
 * pb 2011/03/02
 */

#include "Editor.h"

#include "Data.h"

#define MAXNUM_ROWS  12

class DataEditor;

class DataSubEditor : public Editor {
  public:
	struct FieldData {
		GuiObject _label, _button, _text;
		void *_address;
		Data_Description _description;
		long _minimum, _maximum, _min2, _max2;
		wchar_t *_history;   /* The full prefix of the members. */
		int _rank;   /* Should the button open a StructEditor (0) or VectorEditor (1) or MatrixEditor (2) ? */
	};

	DataSubEditor (DataEditor *root, const wchar_t *title, void *address, Data_Description description);
	virtual ~DataSubEditor ();

	DataEditor *_root;
	void *_address;
	Data_Description _description;
	GuiObject _scrollBar;
	int _irow, _topField, _numberOfFields;
	FieldData _fieldData [1 + MAXNUM_ROWS];

  protected:
	static wchar_t * singleTypeToText (void *address, int type, void *tagType, MelderString *buffer);

	virtual wchar_t * type () { return L"DataSubEditor"; }
	virtual bool isScriptable() { return false; }
	virtual void createMenus ();
	virtual void update ();
	virtual long countFields ();
	virtual Data_Description findNumberUse (const wchar_t *number);
	virtual void createChildren ();

	virtual Data_Description getDescription () { return _description; }
	virtual void showStructMember (void *structAddress, Data_Description structDescription, Data_Description memberDescription, FieldData *fieldData, wchar_t *history);
	virtual void showStructMembers (void *structAddress, Data_Description structDescription, int fromMember, wchar_t *history);
	virtual void showMembers ();

  private:
	static void gui_button_cb_change (I, GuiButtonEvent event);
	static void gui_button_cb_cancel (I, GuiButtonEvent event);
	static void gui_cb_scroll (GUI_ARGS);
	static void gui_button_cb_open (I, GuiButtonEvent event);
	static int menu_cb_help (EDITOR_ARGS);
};

class VectorEditor : public DataSubEditor {
  public:
	VectorEditor (DataEditor *root, const wchar_t *title, void *address,
		Data_Description description, long minimum, long maximum);

	wchar_t * type () { return L"VectorEditor"; }

	long _minimum, _maximum;

  protected:
	/* No structs inside. */
	virtual Data_Description getDescription () { return _description -> type != structwa ? NULL : (Data_Description) _description -> tagType; }

	virtual long countFields ();
	virtual void showMembers ();
};

class MatrixEditor : public DataSubEditor {
  public:
	MatrixEditor (DataEditor *root, const wchar_t *title, void *address,
		Data_Description description, long min1, long max1, long min2, long max2);

	virtual wchar_t * type () { return L"MatrixEditor"; }

	long _minimum, _maximum, _min2, _max2;

  protected:
	/* No structs inside. */
	virtual Data_Description getDescription () { return NULL; }
	virtual long countFields ();
	virtual void showMembers ();
};

class StructEditor : public DataSubEditor {
  public:
	StructEditor (DataEditor *root, const wchar_t *title, void *address, Data_Description description);

	virtual wchar_t * type () { return L"StructEditor"; }

  protected:
	virtual long countFields ();
	virtual void showMembers ();
};

class ClassEditor : public StructEditor {
  public:
	ClassEditor (DataEditor *root, const wchar_t *title, void *address, Data_Description description);

	virtual wchar_t * type () { return L"ClassEditor"; }

  protected:
	virtual void showMembers ();
	virtual void showMembers_recursive (void *klas);
};

class DataEditor : public ClassEditor {
  public:
	DataEditor (GuiObject parent, const wchar_t *title, Any data);
	virtual ~DataEditor ();

	virtual void dataChanged ();

	Collection _children;
};

/* End of file DataEditor.h */
#endif
