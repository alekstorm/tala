/* CategoriesEditor.c
 *
 * Copyright (C) 1993-2011 David Weenink
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 djmw 1995
 djmw 19980225 repaired a memory leak, caused by wrong inheritance for
 	 CategoriesEditorInsert command.
 djmw 20020408 GPL
 djmw 20020408 Modified 'createMenus'
 djmw 20060111 Replaced Resources.h with Preferences.h
 djmw 20060328 Changed last argument to NULL in XtVaSetValues, XtVaGetValues and XtVaCreateManagedWidget
 	for 64-bit compatibility.
 djmw 20070620 Latest modification.
 pb 20080320 split off Help menu
 pb 20080321 new Editor API
 djmw 20090107 Removed a bug in update that caused editor to crash on replace
 djmw 20090203 Removed potential crashes in CategoriesEditor<command>_create.
 djmw 20110304 Thing_new
*/

#include "CategoriesEditor.h"

#define CategoriesEditor_TEXTMAXLENGTH 100

wchar_t *CategoriesEditor_EMPTYLABEL = L"(empty)";

int CategoriesEditor::menu_cb_help (EDITOR_ARGS) { Melder_help (L"CategoriesEditor"); return 1; }

/**************** Some methods for Collection  ****************/

/* Preconditions: */
/*	1 <= (position[i], newpos) <= size; */
/*	newpos <= position[1] || newpos >= position[npos] */
static void Ordered_moveItems (I, long position[], long npos, long newpos)
{
	iam (Ordered);
	Any *tmp = NULL;
	long i, pos, min = position[1], max = position[1];

	for (i=2; i <= npos; i++)
	{
		if (position[i] > max) max = position[i];
		else if (position[i] < min) min = position[i];
	}

	Melder_assert (min >= 1 && max <= my size && (newpos <= min ||
		newpos >= max));

	if (! (tmp = (void **)NUMpvector (1, npos))) return;

	/*
		'remove'
	*/

	for (i=1; i <= npos; i++)
	{
		tmp[i] = my item[position[i]];
		my item[position[i]] = NULL;
	}

	/*
		create a contiguous 'hole'
	*/

	if (newpos <= min)
	{
		for (pos=max, i=max; i >= newpos; i--)
		{
			if (my item[i]) my item[pos--]= my item[i];
		}
		pos = newpos;
	}
	else
	{
		for (pos=min, i=min; i <= newpos; i++)
		{
			if (my item[i]) my item[pos++]= my item[i];
		}
		pos = newpos - npos + 1;
	}

	/*
		fill the 'hole'
	*/

	for (i=1; i <= npos; i++)
	{
		my item[pos++] = tmp[i];
	}

	NUMpvector_free (tmp, 1);
}

static void Collection_replaceItemPos (I, Any item, long pos)
{
	iam (Collection);
	if (pos < 1 || pos > my size) return;
	forget (my item[pos]);
	my item[pos] = item;
}

/* Remove the item at position 'from' and insert it at position 'to'. */
static void Ordered_moveItem (I, long from, long to)
{
	iam (Ordered); Data tmp; long i;
	if (from < 1 || from > my size) from = my size;
	if (to < 1 || to > my size) to = my size;
	if (from == to) return;
	tmp = (structData *)my item[from];
	if (from > to)
	{
		for (i=from; i > to; i--) my item[i] = my item[i-1];
	}
	else
	{
		for (i=from; i < to; i++) my item[i] = my item[i+1];
	}
	my item[to] = tmp;
}

/********************** General Command **********************/

#define CategoriesEditorCommand_members Command_members \
	Categories categories;  							\
	long *selection; long nSelected, newPos;
#define CategoriesEditorCommand_methods Command_methods
class_create (CategoriesEditorCommand, Command);

static void classCategoriesEditorCommand_destroy (I)
{
	iam (CategoriesEditorCommand);
	NUMlvector_free (my selection, 1);
	forget (my categories);
	inherited (CategoriesEditorCommand) destroy (me);
}

static int CategoriesEditorCommand_init (I, wchar_t *name,  Any data,
	int (*execute) (Any), int (*undo) (Any), int nCategories, int nSelected)
{
	iam (CategoriesEditorCommand);
	(void) nCategories;

	my nSelected = nSelected;
	return Command_init (me, name, data, execute, undo) &&
			(my categories = Categories_create()) &&
			(my selection = NUMlvector (1, nSelected));
}

class_methods (CategoriesEditorCommand, Command)
    class_method_local (CategoriesEditorCommand, destroy)
class_methods_end

/*********************** Insert Command ***********************/

#define CategoriesEditorInsert_members CategoriesEditorCommand_members
#define CategoriesEditorInsert_methods CategoriesEditorCommand_methods
class_create (CategoriesEditorInsert, CategoriesEditorCommand);

static int CategoriesEditorInsert_execute (I)
{
	iam (CategoriesEditorInsert);
	CategoriesEditor *editor = (CategoriesEditor *)my data;
	SimpleString str;
	if (! (str = (structSimpleString *)Data_copy (((Categories) my categories)->item[1])) ||
		! Ordered_addItemPos (editor->_data, str, my selection[1]))
	{
		forget (str); return 0;
	}
	editor->update (my selection[1], 0, my selection, 1);
	return 1;
}

static int CategoriesEditorInsert_undo (I)
{
	iam (CategoriesEditorInsert);
	CategoriesEditor *editor = (CategoriesEditor *)my data;
	Collection_removeItem (editor->_data, my selection[1]);
	editor->update (my selection[1], 0, my selection, 1);
	return 1;
}

static Any CategoriesEditorInsert_create (Any data, Any str, int position)
{
	CategoriesEditorInsert me = Thing_new (CategoriesEditorInsert);
	if (me == NULL || ! CategoriesEditorCommand_init (me, L"Insert", data,
		CategoriesEditorInsert_execute, CategoriesEditorInsert_undo, 1, 1)) goto end;
	my selection[1] = position;
	Collection_addItem (my categories, str);
end:
	if (Melder_hasError ()) forget (me);
	return me;
}

class_methods (CategoriesEditorInsert, CategoriesEditorCommand)
class_methods_end

/*********************** Remove Command ***********************/

#define CategoriesEditorRemove_members CategoriesEditorCommand_members
#define CategoriesEditorRemove_methods CategoriesEditorCommand_methods
class_create (CategoriesEditorRemove, CategoriesEditorCommand);

static int CategoriesEditorRemove_execute (I)
{
	iam (CategoriesEditorRemove);
	long i;
	CategoriesEditor *editor = (CategoriesEditor *)my data;
	Categories categories = (structCategories *)editor->_data;

	for (i = my nSelected; i >= 1; i--)
	{
		Ordered_addItemPos (my categories, categories->item[my selection[i]], 1);
		categories->item[my selection[i]] = NULL;
		Collection_removeItem (categories, my selection[i]);
	}
	editor->update (my selection[1], 0, NULL, 0);
	return 1;
}

static int CategoriesEditorRemove_undo (I)
{
	iam (CategoriesEditorRemove);
	int i;
	CategoriesEditor *editor = (CategoriesEditor *)my data;
	Categories categories = (structCategories *)editor->_data;

	for (i = 1; i <= my nSelected; i++)
	{
		Data item = (structData *)Data_copy (my categories->item[i]);
		Ordered_addItemPos (categories, item, my selection[i]);
	}
	editor->update (my selection[1], 0, my selection, my nSelected);
	return 1;
}

static Any CategoriesEditorRemove_create (Any data, long *posList, long posCount)
{
	CategoriesEditorRemove me = Thing_new (CategoriesEditorRemove);

	if (me == NULL || ! CategoriesEditorCommand_init (me, L"Remove", data,
		CategoriesEditorRemove_execute, CategoriesEditorRemove_undo,
		posCount, posCount)) goto end;
	for (long i = 1; i <= posCount; i++) my selection[i] = posList[i];
end:
	if (Melder_hasError ()) forget (me);
	return me;
}

class_methods (CategoriesEditorRemove, CategoriesEditorCommand)
class_methods_end
//update (me);
/*********************** Replace Command ***********************/

#define CategoriesEditorReplace_members CategoriesEditorCommand_members
#define CategoriesEditorReplace_methods CategoriesEditorCommand_methods
class_create (CategoriesEditorReplace, CategoriesEditorCommand);

static int CategoriesEditorReplace_execute (I)
{
	iam (CategoriesEditorReplace);
	long i;
	CategoriesEditor *editor = (CategoriesEditor *)my data;
	Categories categories = (structCategories *)editor -> _data;

	for (i = my nSelected; i >= 1; i--)
	{
		Data str = (structData *)Data_copy (my categories -> item[1]);
		Ordered_addItemPos (my categories,
			categories -> item[my selection[i]], 2);
		categories -> item[my selection[i]] =  str;
	}
	editor->update (my selection[1], my selection[my nSelected],
		my selection, my nSelected);
	return 1;
}

static int CategoriesEditorReplace_undo (I)
{
	iam (CategoriesEditorReplace); long i;
	CategoriesEditor *editor = (CategoriesEditor *)my data;
	Categories categories = (structCategories *)editor -> _data;

	for (i = 1; i <= my nSelected; i++)
	{
		Data str = (structData *)Data_copy (my categories -> item[i+1]);
		Collection_replaceItemPos (categories, str, my selection[i]);
	}
	editor->update (my selection[1], my selection[my nSelected],
		my selection, my nSelected);
	return 1;
}

static Any CategoriesEditorReplace_create (Any data, Any str, long *posList, long posCount)
{
	CategoriesEditorReplace me = Thing_new (CategoriesEditorReplace);

	if (me == NULL || ! CategoriesEditorCommand_init (me, L"Replace", data,
		CategoriesEditorReplace_execute, CategoriesEditorReplace_undo,
			posCount + 1, posCount)) goto end;
	for (long i = 1; i <= posCount; i++)
	{
		my selection[i] = posList[i];
	}
	Collection_addItem (my categories, str);
end:
	if (Melder_hasError ()) forget (me);
	return me;
}

class_methods (CategoriesEditorReplace, CategoriesEditorCommand)
class_methods_end

/*********************** MoveUp Command ***********************/

#define CategoriesEditorMoveUp_members CategoriesEditorCommand_members
#define CategoriesEditorMoveUp_methods CategoriesEditorCommand_methods
class_create (CategoriesEditorMoveUp, CategoriesEditorCommand);

static int CategoriesEditorMoveUp_execute (I)
{
	iam (CategoriesEditorMoveUp);
	CategoriesEditor *editor = (CategoriesEditor *)my data;
	long i, *selection;

	Ordered_moveItems (editor->_data, my selection, my nSelected, my newPos);
	if (! (selection = NUMlvector (1, my nSelected))) return 0;
	for (i = 1; i <= my nSelected; i++)
	{
		selection[i] = my newPos + i - 1;
	}
	editor->update (my newPos, my selection[my nSelected], selection,
		my nSelected);
	NUMlvector_free (selection, 1);
	return 1;
}

static int CategoriesEditorMoveUp_undo (I)
{
	iam (CategoriesEditorMoveUp); long i;
	CategoriesEditor *editor = (CategoriesEditor *)my data;

	for (i = 1; i <= my nSelected; i++)
	{
		Ordered_moveItem (editor->_data, my newPos, my selection[my nSelected]);
	}
	editor->update (my newPos, my selection[my nSelected], my selection,
		my nSelected);
	return 1;
}

static Any CategoriesEditorMoveUp_create (Any data, long *posList,
	long posCount, long newPos)
{
	CategoriesEditorMoveUp me = Thing_new (CategoriesEditorMoveUp);

	if (me == NULL || ! CategoriesEditorCommand_init (me, L"Move up", data,
		CategoriesEditorMoveUp_execute, CategoriesEditorMoveUp_undo, 0, posCount)) goto end;
	for (long i = 1; i <= posCount; i++)
	{
		my selection[i] = posList[i];
	}
	my newPos = newPos;
end:
	if (Melder_hasError ()) forget (me);
	return me;
}

class_methods (CategoriesEditorMoveUp, CategoriesEditorCommand)
class_methods_end

/*********************** MoveDown Command ***********************/

#define CategoriesEditorMoveDown_members CategoriesEditorCommand_members
#define CategoriesEditorMoveDown_methods CategoriesEditorCommand_methods
class_create (CategoriesEditorMoveDown, CategoriesEditorCommand);

static int CategoriesEditorMoveDown_execute (I)
{
	iam (CategoriesEditorMoveDown);
	CategoriesEditor *editor = (CategoriesEditor *)my data;
	long i, *selection;

	Ordered_moveItems (editor->_data, my selection, my nSelected, my newPos);
	if (! (selection = NUMlvector (1, my nSelected))) return 0;
	for (i = 1; i <= my nSelected; i++)
	{
		selection[i] = my newPos - my nSelected + i;
	}
	editor->update (my selection[1], my newPos, selection, my nSelected);
	NUMlvector_free (selection, 1);
	return 1;
}

static int CategoriesEditorMoveDown_undo (I)
{
	iam (CategoriesEditorMoveDown); long i, from = my selection[1];
	CategoriesEditor *editor = (CategoriesEditor *)my data;
	for (i=1; i <= my nSelected; i++)
	{
		Ordered_moveItem (editor->_data, my newPos, my selection[1]);
	}
	editor->update ((from > 1 ? from-- : from), my newPos, my selection,
		my nSelected);
	return 1;
}

static Any CategoriesEditorMoveDown_create (Any data, long *posList,
	long posCount, long newPos)
{
	CategoriesEditorMoveDown me = Thing_new (CategoriesEditorMoveDown);

	if (me == NULL || ! CategoriesEditorCommand_init (me, L"Move down", data,
		CategoriesEditorMoveDown_execute, CategoriesEditorMoveDown_undo,
			0, posCount)) goto end;
	for (long i = 1; i <= posCount; i++)
	{
		my selection[i] = posList[i];
	}
	my newPos = newPos;
end:
	if (Melder_hasError ()) forget (me);
	return me;
}

class_methods (CategoriesEditorMoveDown, CategoriesEditorCommand)
class_methods_end

/********************* Commands (End)  *************************************/

CategoriesEditor::CategoriesEditor (GuiObject parent, const wchar_t *title, Any data)
	: Editor (parent, 20, 40, 600, 600, title, data),
	_history((structCommandHistory *)CommandHistory_create (100)),
	_position(0) {
	createMenus ();
	createChildren ();
	update (0, 0, NULL, 0);
	updateWidgets ();
}

CategoriesEditor::~CategoriesEditor () {
	forget (_history); /* !! Editor */
}

void CategoriesEditor::notifyOutOfView ()
{
	MelderString tmp = { 0 };
	long posCount, *posList = GuiList_getSelectedPositions (_list, & posCount);
	MelderString_append1 (&tmp, L"");
	if (posList != NULL)
	{
		long outOfView = 0, top = GuiList_getTopPosition (_list), bottom = GuiList_getBottomPosition (_list);

		for (long i = posCount; i > 0; i--)
		{
			if (posList[i] < top || posList[i] > bottom) outOfView++;
		}
		NUMlvector_free (posList, 1);
		if (outOfView > 0)
		{
			MelderString_append2 (&tmp, Melder_integer (outOfView), L" selection(s) out of view");
		}
	}
	GuiLabel_setString (_outOfView, tmp.string);
	MelderString_free (&tmp);
}

void CategoriesEditor::update_dos ()
{
	wchar_t *name;
	MelderString tmp = { 0 };
	Boolean undoSense = True, redoSense = True;
	/*
		undo
	*/

	if (! (name = CommandHistory_commandName (_history, 0)))
	{
			name = L"nothing"; undoSense = False;
	}

	MelderString_append4 (&tmp, L"Undo ", L"\"", name, L"\"");
	GuiButton_setString (_undo, tmp.string);
	GuiObject_setSensitive (_undo, undoSense);

	/*
		redo
	*/

	if (! (name = CommandHistory_commandName (_history, 1)))
	{
		name = L"nothing"; redoSense = False;
	}
	MelderString_empty (&tmp);
	MelderString_append4 (&tmp, L"Redo ", L"\"", name, L"\"");
	GuiButton_setString (_redo, tmp.string);
	GuiObject_setSensitive (_redo, redoSense);
	MelderString_free (&tmp);
}

void CategoriesEditor::updateWidgets () /*all buttons except undo & redo */
{
	long size = ((Categories) _data)->size;
	Boolean insert = False, insertAtEnd = True, replace = False, remove = False;
	Boolean moveUp = False, moveDown = False;
	long posCount, *posList = GuiList_getSelectedPositions (_list, & posCount);
	if (posList != NULL)
	{
		int firstPos = posList[1], lastPos = posList[posCount];
		int contiguous = lastPos - firstPos + 1 == posCount;
		moveUp = contiguous && firstPos > 1;
		moveDown = contiguous && lastPos < size;
		_position = firstPos;
		remove = True; replace = True; //insertAtEnd = False;
		if (posCount == 1)
		{
			insert = True;
			//if (posList[1] == size) insertAtEnd = True;
			if (size == 1 && ! wcscmp (CategoriesEditor_EMPTYLABEL,
				OrderedOfString_itemAtIndex_c (_data, 1))) remove = False;
		}
		NUMlvector_free (posList, 1);
	}
	GuiObject_setSensitive (_insert, insert); GuiObject_setSensitive (_insertAtEnd, insertAtEnd);
	GuiObject_setSensitive (_replace, replace); GuiObject_setSensitive (_remove, remove);
	GuiObject_setSensitive (_moveUp, moveUp); GuiObject_setSensitive (_moveDown, moveDown);
	if (_history) update_dos ();
	notifyOutOfView ();
}

void CategoriesEditor::update (long from, long to, const long *select, long nSelect)
{
	int i, itemCount, size = ((Categories) _data)->size;

	if (size == 0)
	{
		SimpleString str = SimpleString_create (CategoriesEditor_EMPTYLABEL);
		if (! str || ! Collection_addItem (_data, str)) return;
		update (0, 0, NULL, 0);
		return;
	}
	if (from == 0 && from == to)
	{
		from = 1; to = size;
	}
	if (from < 1 || from > size) from = size;
	if (to < 1 || to > size) to = size;
	if (from > to)
	{
		i = from; from = to; to = i;
	}

	/*
		Begin optimization: add the items from a table instead of separately.
	*/

	{
		const wchar_t **table = NULL;
		MelderString itemText  = { 0 };
		int k;

		if (! (table = Melder_malloc_e (const wchar_t *, to - from + 1))) return;
		itemCount = GuiList_getNumberOfItems (_list);
		/*for (k = 0, i = from; i <= to; i++)
		{
			wchar_t itemText[CategoriesEditor_TEXTMAXLENGTH+10];
			swprintf (itemText, CategoriesEditor_TEXTMAXLENGTH+10, L"%6d     %.*ls", i, CategoriesEditor_TEXTMAXLENGTH,
				OrderedOfString_itemAtIndex_c (_data, i));
			table[k++] = XmStringCreateSimple (Melder_peekWcsToUtf8 (itemText));
		}*/
		for (k = 0, i = from; i <= to; i++)
		{
			wchar_t wcindex[20];
			MelderString_empty (&itemText);
			swprintf (wcindex, 19, L"%5ld ", i);
			MelderString_append2 (&itemText, wcindex, OrderedOfString_itemAtIndex_c (_data, i));
			table[k++] = Melder_wcsdup_f (itemText.string);
		}
		if (itemCount > size) /* some items have been removed from Categories? */
		{
			for (long j = itemCount; j > size; j --) {
				GuiList_deleteItem (_list, j);
			}
			itemCount = size;
		}
		if (to > itemCount)
		{
			for (long j = 1; j <= to - itemCount; j ++) {
				GuiList_insertItem (_list, table [itemCount - from + j], 0);
			}
		}
		if (from <= itemCount)
		{
			long n = (to < itemCount ? to : itemCount);
			for (long j = 0; j < n - from + 1; j++) {
				GuiList_replaceItem (_list, table[j], from + j);
			}
		}
		for (k = 0, i = from; i <= to; i++)
		{
			Melder_free (table[k++]);
		}
		Melder_free (table);
		MelderString_free (&itemText);
	}

	/*
		End of optimization
	*/

	/*
		HIGHLIGHT
	*/

	GuiList_deselectAllItems (_list);
	if (size == 1) /* the only item is always selected */
	{
		const wchar_t *catg = OrderedOfString_itemAtIndex_c (_data, 1);
		GuiList_selectItem (_list, 1);
		updateWidgets ();   // instead of "notify". BUG?
		GuiText_setString (_text, catg);
	}
	else if (nSelect > 0)
	{
		/*
			Select but postpone highlighting
		*/
		for (i = 1; i <= nSelect; i++)
		{
			GuiList_selectItem (_list, select[i] > size ? size : select[i]);
		}
	}

	/*
		VIEWPORT
	*/

	{
		long top = GuiList_getTopPosition (_list), bottom = GuiList_getBottomPosition (_list);
		long visible = bottom - top + 1;
		if (nSelect == 0)
		{
			top = _position - visible / 2;
		}
		else if (select[nSelect] < top)
		{
			/* selection above visible area*/
			top = select[1];
		}
		else if (select[1] > bottom)
		{
			/* selection below visible area */
			top = select[nSelect] - visible + 1;
		}
		else
		{
			int deltaTopPos = -1, nUpdate = to - from + 1;
			if ((from == select[1] && to == select[nSelect]) /* Replace */ ||
				(nUpdate > 2 && nSelect == 1) /* Inserts */) deltaTopPos = 0;
			else if (nUpdate == nSelect + 1 && select[1] == from + 1) /* down */
				deltaTopPos = 1;
			top += deltaTopPos;
		}
		if (top + visible > size) top = size - visible + 1;
		if (top < 1) top = 1;
		GuiList_setTopPosition (_list, top);
	}
}

void CategoriesEditor::gui_button_cb_remove (I, GuiButtonEvent event) {
	(void) event;
	CategoriesEditor *editor = (CategoriesEditor *)void_me;
	long posCount, *posList = GuiList_getSelectedPositions (editor->_list, & posCount);
	if (posList != NULL)
	{
		CategoriesEditorRemove command = (structCategoriesEditorRemove *)CategoriesEditorRemove_create
			(editor, posList, posCount);
		if (! command || ! Command_do (command))
		{
			forget (command); NUMlvector_free (posList, 1); return;
		}
		if (editor->_history) CommandHistory_insertItem (editor->_history, command);
		NUMlvector_free (posList, 1);
		editor->updateWidgets ();
	}
}

void CategoriesEditor::insert (int position)
{
	SimpleString str = NULL;
	CategoriesEditorInsert command = NULL;
	wchar_t *text = GuiText_getString (_text);

	if (wcslen (text) == 0 || ! (str = SimpleString_create (text)) ||
		! (command = (structCategoriesEditorInsert *)CategoriesEditorInsert_create (this, str, position)) ||
		! Command_do (command)) goto end;
	if (_history) CommandHistory_insertItem (_history, command);
	Melder_free (text);
	updateWidgets ();
	return;
end:
	Melder_free (text);
	forget (str);
	forget (command);
}

void CategoriesEditor::gui_button_cb_insert (I, GuiButtonEvent event) {
	(void) event;
	CategoriesEditor *editor = (CategoriesEditor *)void_me;
	editor->insert (editor->_position);
}

void CategoriesEditor::gui_button_cb_insertAtEnd (I, GuiButtonEvent event) {
	(void) event;
	CategoriesEditor *editor = (CategoriesEditor *)void_me;
	Categories categories = (structCategories *)editor->_data;
	editor->insert (categories->size + 1);
	editor->_position = categories->size;
}

void CategoriesEditor::gui_button_cb_replace (I, GuiButtonEvent event) {
	(void) event;
	CategoriesEditor *editor = (CategoriesEditor *)void_me;
	long posCount, *posList = GuiList_getSelectedPositions (editor->_list, & posCount);
	if (posList != NULL)
	{
		CategoriesEditorReplace command = NULL;
		wchar_t *text = GuiText_getString (editor->_text);
		SimpleString str = NULL;

		if (wcslen (text) == 0 || ! (str = SimpleString_create (text)) ||
			! (command = (structCategoriesEditorReplace *)CategoriesEditorReplace_create (editor, str, posList, posCount)) ||
			! Command_do (command)) goto end;
		if (editor->_history) CommandHistory_insertItem (editor->_history, command);
		NUMlvector_free (posList, 1);
		Melder_free (text);
		editor->updateWidgets ();
		return;
end:
		Melder_free (text);
		forget (str);
		forget (command);
	}
}

/* Precondition: contiguous selection */
void CategoriesEditor::gui_button_cb_moveUp (I, GuiButtonEvent event) {
	(void) event;
	CategoriesEditor *editor = (CategoriesEditor *)void_me;
	long posCount, *posList = GuiList_getSelectedPositions (editor->_list, & posCount);
	if (posList != NULL)
	{
		CategoriesEditorMoveUp command = (structCategoriesEditorMoveUp *)CategoriesEditorMoveUp_create
			(editor, posList, posCount, posList[1]-1);
		if (! command || ! Command_do (command)) goto end;
		if (editor->_history) CommandHistory_insertItem (editor->_history, command);
		NUMlvector_free (posList, 1);
		editor->updateWidgets ();
		return;
end:
		NUMlvector_free (posList, 1);
		forget (command);
	}
}

/* Precondition: contiguous selection */
void CategoriesEditor::gui_button_cb_moveDown (I, GuiButtonEvent event) {
	(void) event;
	CategoriesEditor *editor = (CategoriesEditor *)void_me;
	long posCount, *posList = GuiList_getSelectedPositions (editor->_list, & posCount);
	if (posList != NULL)
	{
		CategoriesEditorMoveDown command = (structCategoriesEditorMoveDown *)CategoriesEditorMoveDown_create
			(editor, posList, posCount, posList[posCount] + 1);
		if (! command || ! Command_do (command)) goto end;
		if (editor->_history) CommandHistory_insertItem (editor->_history, command);
		NUMlvector_free (posList, 1);
		editor->updateWidgets ();
		return;
end:
		NUMlvector_free (posList, 1);
		forget (command);
	}
}


void CategoriesEditor::gui_cb_scroll (GUI_ARGS) {
	CategoriesEditor *editor = (CategoriesEditor *)void_me;
	editor->notifyOutOfView ();
}

void CategoriesEditor::gui_list_cb_double_click (void *void_me, GuiListEvent event) {
	(void) event;
	CategoriesEditor *editor = (CategoriesEditor *)void_me;
	const wchar_t *catg = OrderedOfString_itemAtIndex_c (editor->_data, editor->_position);
	GuiText_setString (editor->_text, catg);
}

void CategoriesEditor::gui_list_cb_extended (void *void_me, GuiListEvent event) {
	(void) event;
	CategoriesEditor *editor = (CategoriesEditor *)void_me;
	editor->updateWidgets ();
}

void CategoriesEditor::gui_button_cb_undo (I, GuiButtonEvent event) {
	(void) event;
	CategoriesEditor *editor = (CategoriesEditor *)void_me;
	if (CommandHistory_offleft (editor->_history)) return;
	Command_undo (CommandHistory_getItem (editor->_history));
	CommandHistory_back (editor->_history);
	editor->updateWidgets ();
}

void CategoriesEditor::gui_button_cb_redo (I, GuiButtonEvent event) {
	(void) event;
	CategoriesEditor *editor = (CategoriesEditor *)void_me;
	CommandHistory_forth (editor->_history);
	if (CommandHistory_offright (editor->_history)) return;
	Command_do (CommandHistory_getItem (editor->_history));
	editor->updateWidgets ();
}

void CategoriesEditor::createMenus () {
	EditorMenu *menu = getMenu(L"Help");
	menu->addCommand (L"CategoriesEditor help", '?', menu_cb_help);
}

// origin is at top left.
void CategoriesEditor::createChildren ()
{
	GuiObject vertScrollBar;
	double menuBarOffset = 40;
	double button_width = 90, button_height = menuBarOffset, list_width = 260, list_height = 200, list_bottom;
	double delta_x = 15, delta_y = menuBarOffset / 2, text_button_height = button_height / 2;
	double left, right, top, bottom, buttons_left, buttons_top;

	left = 5; right = left + button_width; top = 3 + menuBarOffset; bottom = top + text_button_height;
	GuiLabel_createShown (_dialog, left, right, top, bottom, L"Positions:", 0);
	left = right + delta_x ; right = left + button_width;
	GuiLabel_createShown (_dialog, left, right, top, bottom, L"Values:", 0);

	left = 0; right = left + list_width; buttons_top = (top = bottom + delta_y); list_bottom = bottom = top + list_height;
	_list = GuiList_create (_dialog, left, right, top, bottom, true, NULL);
	GuiList_setSelectionChangedCallback (_list, gui_list_cb_extended, this);
	GuiList_setDoubleClickCallback (_list, gui_list_cb_double_click, this);
	GuiObject_show (_list);

	/*
		The valueChangedCallback does not get any notification in case of:
			drag, decrement, increment, pageIncrement & pageDecrement
	*/

	buttons_left = left = right + 2*delta_x; right = left + button_width; bottom = top + button_height;
	GuiLabel_createShown (_dialog, left, right, top, bottom, L"Value:", 0);
	left = right + delta_x; right = left + button_width;
	_text = GuiText_createShown (_dialog, left, right, top, bottom, 0);
	GuiText_setString (_text, CategoriesEditor_EMPTYLABEL);

	left = buttons_left; right = left + button_width; top = bottom + delta_y; bottom = top + button_height;
	_insert = GuiButton_createShown (_dialog, left, right, top, bottom,	L"Insert", gui_button_cb_insert, this, GuiButton_DEFAULT);
	left = right + delta_x; right = left + button_width;
	_replace = GuiButton_createShown (_dialog, left, right, top, bottom, L"Replace", gui_button_cb_replace, this, 0);
	left = buttons_left; right = left + 1.5 * button_width; top = bottom + delta_y; bottom = top + button_height;
	_insertAtEnd = GuiButton_createShown (_dialog, left, right, top, bottom, L"Insert at end", gui_button_cb_insertAtEnd, this, 0);
	top = bottom + delta_y; bottom = top + button_height;
	_undo = GuiButton_createShown (_dialog, left, right, top, bottom, L"Undo", gui_button_cb_undo, this, 0);
	top = bottom + delta_y; bottom = top + button_height;
	_redo = GuiButton_createShown (_dialog, left, right, top, bottom, L"Redo", gui_button_cb_redo, this, 0);
	top = bottom + delta_y; bottom = top + button_height;
	_remove = GuiButton_createShown (_dialog, left, right, top, bottom, L"Remove", gui_button_cb_remove, this, 0);
	top = bottom + delta_y; bottom = top + button_height;
	_moveUp = GuiButton_createShown (_dialog, left, right, top, bottom, L"Move selection up", gui_button_cb_moveUp, this, 0);
	top = bottom + delta_y; bottom = top + button_height;
	_moveDown = GuiButton_createShown (_dialog, left, right, top, bottom, L"Move selection down", gui_button_cb_moveDown, this, 0);

	top = list_bottom + delta_y; bottom = top + button_height; left = 5; right = left + 200;
	_outOfView = GuiLabel_createShown (_dialog, left, right, top, bottom, L"", 0);
}

void CategoriesEditor::dataChanged ()
{
	update (0, 0, NULL, 0);
	updateWidgets ();
}

/* End of file CategoriesEditor.c */
