/* GuiList.c
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
 * pb 2007/12/26 abstraction from Motif
 * pb 2009/01/31 NUMlvector_free has to be followed by assigning a NULL
 * fb 2010/02/23 GTK
 * pb 2010/06/14 HandleControlClick
 * pb 2010/07/05 blockSelectionChangedCallback
 * pb 2010/11/28 removed Motif
 */

#include "GuiP.h"
#include "num/NUM.h"
#undef iam
#define iam(x)  x me = (x) void_me
#if win || mac
	#define iam_list \
		Melder_assert (widget -> widgetClass == xmListWidgetClass); \
		GuiList me = (structGuiList*)widget -> userData
#else
	#define iam_list \
		GuiList me = (structGuiList*)_GuiObject_getUserData (widget)
#endif

#if win
	#define CELL_HEIGHT  15
#elif mac
	#define CELL_HEIGHT  18
	#define USE_MAC_LISTBOX_CONTROL  0
#endif

typedef struct structGuiList {
	GuiObject widget;
	bool allowMultipleSelection, blockSelectionChangedCallback;
	void (*selectionChangedCallback) (void *boss, GuiListEvent event);
	void *selectionChangedBoss;
	void (*doubleClickCallback) (void *boss, GuiListEvent event);
	void *doubleClickBoss;
	GtkListStore *liststore;
} *GuiList;

static void _GuiGtkList_destroyCallback (gpointer void_me) {
	iam (GuiList);
	Melder_free (me);
}
static void _GuiGtkList_selectionChangedCallback (GtkTreeSelection *sel, gpointer void_me) {
	iam (GuiList);
	if (my selectionChangedCallback != NULL && ! my blockSelectionChangedCallback) {
		//Melder_casual ("Selection changed.");
		struct structGuiListEvent event = { GTK_WIDGET (gtk_tree_selection_get_tree_view (sel)) };
		my selectionChangedCallback (my selectionChangedBoss, & event);
	}
}

enum {
  COLUMN_STRING,
  N_COLUMNS
};

GuiObject GuiList_create (GuiObject parent, int left, int right, int top, int bottom, bool allowMultipleSelection, const wchar_t *header) {
	GuiList me = Melder_calloc_f (struct structGuiList, 1);
	my allowMultipleSelection = allowMultipleSelection;
	GtkCellRenderer *renderer = NULL;
	GtkTreeViewColumn *col = NULL;
	GtkTreeSelection *sel = NULL;
	GtkListStore *liststore = NULL;

	liststore = gtk_list_store_new (1, G_TYPE_STRING);   // 1 column, of type String (this is a vararg list)
	GuiObject scrolled = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	my widget = gtk_tree_view_new_with_model (GTK_TREE_MODEL (liststore));
	gtk_container_add (GTK_CONTAINER (scrolled), my widget);
	gtk_widget_show (scrolled);
	gtk_tree_view_set_rubber_banding (GTK_TREE_VIEW (my widget), allowMultipleSelection ? GTK_SELECTION_MULTIPLE : GTK_SELECTION_SINGLE);
	g_object_unref (liststore);   // Destroys the widget after the list is destroyed

	_GuiObject_setUserData (my widget, me); /* nog een functie die je niet moet vergeten */

	renderer = gtk_cell_renderer_text_new ();
	col = gtk_tree_view_column_new ();
	gtk_tree_view_column_pack_start (col, renderer, TRUE);
	gtk_tree_view_column_add_attribute (col, renderer, "text", 0);   // zeroeth column
	if (header != NULL)
		gtk_tree_view_column_set_title (col, Melder_peekWcsToUtf8 (header));
	gtk_tree_view_append_column (GTK_TREE_VIEW (my widget), col);

	g_object_set_data_full (G_OBJECT (my widget), "guiList", me, (GDestroyNotify) _GuiGtkList_destroyCallback); 

/*	GtkCellRenderer *renderer;
	GtkTreeViewColumn *col;
		
	my widget = gtk_tree_view_new_with_model (GTK_TREE_MODEL (liststore));

	renderer = gtk_cell_renderer_text_new ();
	col = gtk_tree_view_column_new ();
	gtk_tree_view_column_pack_start (col, renderer, TRUE);
	gtk_tree_view_column_add_attribute (col, renderer, "text", COL_ID);
	gtk_tree_view_column_set_title (col, " ID ");
	gtk_tree_view_append_column (GTK_TREE_VIEW (view), col);
		
	renderer = gtk_cell_renderer_text_new ();
	col = gtk_tree_view_column_new ();
	gtk_tree_view_column_pack_start (col, renderer, TRUE);
	gtk_tree_view_column_add_attribute (col, renderer, "text", COL_TYPE);
	gtk_tree_view_column_set_title (col, " Type ");
	gtk_tree_view_append_column (GTK_TREE_VIEW (view), col);

	renderer = gtk_cell_renderer_text_new ();
	col = gtk_tree_view_column_new ();
	gtk_tree_view_column_pack_start (col, renderer, TRUE);
	gtk_tree_view_column_add_attribute (col, renderer, "text", COL_NAME);
	gtk_tree_view_column_set_title (col, " Name ");
	gtk_tree_view_append_column (GTK_TREE_VIEW (view), col);
*/

	sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (my widget));
	if (allowMultipleSelection) {
		gtk_tree_selection_set_mode (sel, GTK_SELECTION_MULTIPLE);
	} else {
		gtk_tree_selection_set_mode (sel, GTK_SELECTION_SINGLE);
	}
	if (GTK_IS_BOX (parent)) {
		gtk_box_pack_start (GTK_BOX (parent), scrolled, TRUE, TRUE, 0);
	}
	g_signal_connect (sel, "changed", G_CALLBACK (_GuiGtkList_selectionChangedCallback), me);
	return my widget;
}

GuiObject GuiList_createShown (GuiObject parent, int left, int right, int top, int bottom, bool allowMultipleSelection, const wchar_t *header) {
	GuiObject widget = GuiList_create (parent, left, right, top, bottom, allowMultipleSelection, header);
	GuiObject_show (widget);
	return widget;
}

void GuiList_deleteAllItems (GuiObject widget) {
	iam_list;
	my blockSelectionChangedCallback = true;
	GtkListStore *list_store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (widget)));
	gtk_list_store_clear (list_store);
	my blockSelectionChangedCallback = false;
}

void GuiList_deleteItem (GuiObject widget, long position) {
	iam_list;
	my blockSelectionChangedCallback = true;
	GtkTreeIter iter;
	GtkTreeModel *tree_model = gtk_tree_view_get_model (GTK_TREE_VIEW (widget));
	if (gtk_tree_model_iter_nth_child (tree_model, &iter, NULL, (gint) (position - 1))) {
		gtk_list_store_remove (GTK_LIST_STORE (tree_model), & iter);
	}
	my blockSelectionChangedCallback = false;
}

void GuiList_deselectAllItems (GuiObject widget) {
	iam_list;
	my blockSelectionChangedCallback = true;
	GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (widget));
	gtk_tree_selection_unselect_all (selection);
	my blockSelectionChangedCallback = false;
}

void GuiList_deselectItem (GuiObject widget, long position) {
	iam_list;
	my blockSelectionChangedCallback = true;
	GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (widget));
/*	GtkListStore *list_store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (widget)));
	GtkTreePath *path = gtk_tree_path_new_from_indices ((gint) position);*/
	GtkTreeIter iter;
//	gtk_tree_model_get_iter (GTK_TREE_MODEL (list_store), & iter, path);
//	gtk_tree_path_free (path);
	GtkTreeModel *tree_model = gtk_tree_view_get_model (GTK_TREE_VIEW(widget));
	if (gtk_tree_model_iter_nth_child (tree_model, &iter, NULL, (gint) (position - 1))) {
		gtk_tree_selection_unselect_iter (selection, & iter);
	}
	my blockSelectionChangedCallback = false;
}

long * GuiList_getSelectedPositions (GuiObject widget, long *numberOfSelectedPositions) {
	*numberOfSelectedPositions = 0;
	long *selectedPositions = NULL;
	GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (widget));
	GtkListStore *list_store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (widget)));
	int n = gtk_tree_selection_count_selected_rows (selection);
	if (n > 0) {
		GList *list = gtk_tree_selection_get_selected_rows (selection, (GtkTreeModel **) & list_store);
		long ipos = 1;
		*numberOfSelectedPositions = n;
		selectedPositions = NUMlvector (1, *numberOfSelectedPositions);
		Melder_assert (selectedPositions != NULL);
		for (GList *l = g_list_first (list); l != NULL; l = g_list_next (l)) {
			gint *index = gtk_tree_path_get_indices ((GtkTreePath*)l -> data);
			selectedPositions [ipos] = index [0] + 1;
			ipos ++;
		}
		g_list_foreach (list, (GFunc) gtk_tree_path_free, NULL);
		g_list_free (list);

		// TODO: probably one big bug
		// Much nicer is: gtk_tree_selection_selected_foreach ()
		// But requires a structure + function
		// Structure must contain the iterator (ipos) and
		// selectedPositions
		// fb: don't think that using the above function would be nicer,
		//     the code is not that confusing  -- 20100223
	}
	return selectedPositions;
}

long GuiList_getBottomPosition (GuiObject widget) {
	// TODO
	return 1;
}

long GuiList_getNumberOfItems (GuiObject widget) {
	long numberOfItems = 0;
	GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (widget));
	return gtk_tree_model_iter_n_children (model, NULL); 
}

long GuiList_getTopPosition (GuiObject widget) {
	// TODO
	return 1;
}

void GuiList_insertItem (GuiObject widget, const wchar_t *itemText, long position) {
	/*
	 * 'position' is the position of the new item in the list after insertion:
	 * a value of 1 therefore puts the new item at the top of the list;
	 * a value of 0 is special: the item is put at the bottom of the list.
	 */
	iam_list;
	my blockSelectionChangedCallback = true;
	GtkListStore *list_store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (widget)));
	gtk_list_store_insert_with_values (list_store, NULL, (gint) position - 1, COLUMN_STRING, Melder_peekWcsToUtf8 (itemText), -1);
	my blockSelectionChangedCallback = false;
	// TODO: Tekst opsplitsen
	// does GTK know the '0' trick?
	// it does know about NULL, to append in another function
}

void GuiList_replaceItem (GuiObject widget, const wchar_t *itemText, long position) {
	iam_list;
	my blockSelectionChangedCallback = true;
	GtkTreeIter iter;
	GtkTreeModel *tree_model = gtk_tree_view_get_model (GTK_TREE_VIEW (widget));
	if (gtk_tree_model_iter_nth_child (tree_model, &iter, NULL, (gint) (position - 1))) {
		gtk_list_store_set (GTK_LIST_STORE (tree_model), & iter, COLUMN_STRING, Melder_peekWcsToUtf8 (itemText), -1);
	}
	my blockSelectionChangedCallback = false;
/*
	GtkTreePath *path = gtk_tree_path_new_from_indices ((gint) position);
	GtkTreeIter iter;
	GtkListStore *list_store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (widget)));
	gtk_tree_model_get_iter (GTK_TREE_MODEL (list_store), & iter, path);
	gtk_tree_path_free (path);*/
	// gtk_list_store_set (list_store, & iter, 0, Melder_peekWcsToUtf8 (itemText), -1);
	// TODO: Tekst opsplitsen
}

void GuiList_selectItem (GuiObject widget, long position) {
	iam_list;
	my blockSelectionChangedCallback = true;
	GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (widget));
	GtkTreePath *path = gtk_tree_path_new_from_indices ((gint) position - 1, -1);
	gtk_tree_selection_select_path(selection, path);
	gtk_tree_path_free (path);
	my blockSelectionChangedCallback = false;

// TODO: check of het bovenstaande werkt, dan kan dit weg
//		GtkListStore *list_store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (widget)));
//		GtkTreePath *path = gtk_tree_path_new_from_indices ((gint) position);
//		GtkTreeIter iter;
//		gtk_tree_model_get_iter (GTK_TREE_MODEL (list_store), & iter, path);
//		gtk_tree_selection_select_iter (selection, & iter);
}

void GuiList_setDoubleClickCallback (GuiObject widget, void (*callback) (void *boss, GuiListEvent event), void *boss) {
	GuiList me = (structGuiList*)_GuiObject_getUserData (widget);
	if (me != NULL) {
		my doubleClickCallback = callback;
		my doubleClickBoss = boss;
	}
}

void GuiList_setSelectionChangedCallback (GuiObject widget, void (*callback) (void *boss, GuiListEvent event), void *boss) {
	GuiList me = (structGuiList*)_GuiObject_getUserData (widget);
	if (me != NULL) {
		my selectionChangedCallback = callback;
		my selectionChangedBoss = boss;
	}
}

void GuiList_setTopPosition (GuiObject widget, long topPosition) {
	//Melder_casual ("Set top position %ld", topPosition);
//	GtkListStore *list_store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (widget)));
	GtkTreePath *path = gtk_tree_path_new_from_indices ((gint) topPosition);
	gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (widget), path, NULL, FALSE, 0.0, 0.0);
	gtk_tree_path_free (path);
}

/* End of file GuiList.c */
