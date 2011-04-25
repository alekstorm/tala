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

#ifndef _Editor_h_
	#include "Editor.h"
#endif

#include "Data.h"

#define MAXNUM_ROWS  12

#ifdef __cplusplus
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
	~DataSubEditor ();

	wchar_t * type () { return L"DataSubEditor"; }
	bool isScriptable() { return false; }
	void createMenus ();
	void update ();
	long countFields ();
	Data_Description findNumberUse (const wchar_t *number);
	void createChildren ();

	DataEditor *_root;
	void *_address;
	Data_Description _description;
	GuiObject _scrollBar;
	int _irow, _topField, _numberOfFields;
	FieldData _fieldData [1 + MAXNUM_ROWS];

  protected:
	static wchar_t * singleTypeToText (void *address, int type, void *tagType, MelderString *buffer);

	Data_Description getDescription () { return _description; }
	void showStructMember (void *structAddress, Data_Description structDescription, Data_Description memberDescription, FieldData *fieldData, wchar_t *history);
	void showStructMembers (void *structAddress, Data_Description structDescription, int fromMember, wchar_t *history);
	void showMembers ();
};

class VectorEditor : public DataSubEditor {
  public:
	VectorEditor (DataEditor *root, const wchar_t *title, void *address,
		Data_Description description, long minimum, long maximum);

	wchar_t * type () { return L"VectorEditor"; }

	long _minimum, _maximum;

  protected:
	/* No structs inside. */
	Data_Description getDescription () { return _description -> type != structwa ? NULL : (Data_Description) _description -> tagType; }

	long countFields ();
	void showMembers ();
};

class MatrixEditor : public DataSubEditor {
  public:
	MatrixEditor (DataEditor *root, const wchar_t *title, void *address,
		Data_Description description, long min1, long max1, long min2, long max2);

	wchar_t * type () { return L"MatrixEditor"; }

	long _minimum, _maximum, _min2, _max2;

  protected:
	/* No structs inside. */
	Data_Description getDescription () { return NULL; }
	long countFields ();
	void showMembers ();
};

class StructEditor : public DataSubEditor {
  public:
	StructEditor (DataEditor *root, const wchar_t *title, void *address, Data_Description description);

	wchar_t * type () { return L"StructEditor"; }

  protected:
	long countFields ();
	void showMembers ();
};

class ClassEditor : public StructEditor {
  public:
	ClassEditor (DataEditor *root, const wchar_t *title, void *address, Data_Description description);

	wchar_t * type () { return L"ClassEditor"; }

  protected:
	long countFields ();
	void showMembers ();
	void showMembers_recursive (void *klas);
};

class DataEditor : public ClassEditor {
  public:
	DataEditor (GuiObject parent, const wchar_t *title, Any data);
	~DataEditor ();

	void dataChanged ();

	Collection _children;
};
#else
typedef struct DataSubEditor DataSubEditor;
typedef struct VectorEditor VectorEditor;
typedef struct MatrixEditor MatrixEditor;
typedef struct StructEditor StructEditor;
typedef struct ClassEditor ClassEditor;
typedef struct DataEditor *DataEditor;
#endif

/* End of file DataEditor.h */
#endif
