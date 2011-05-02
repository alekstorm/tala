#ifndef _TableEditor_h_
#define _TableEditor_h_
/* TableEditor.h
 *
 * Copyright (C) 2006-2011 Paul Boersma
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
 * pb 2011/03/03
 */

#include "Table.h"
#include "sys/Editor.h"

#define MAXNUM_VISIBLE_COLUMNS  100
#define SIZE_INCHES  40

class TableEditor : public Editor {
  public:
	TableEditor (GuiObject parent, const wchar_t *title, Table table);
	~TableEditor ();

	virtual wchar_t * type () { return L"TableEditor"; }

	virtual void draw ();
	virtual int click (double xWC, double yWC, int shiftKeyPressed);
	virtual void updateVerticalScrollBar ();
	virtual void updateHorizontalScrollBar ();
	virtual void dataChanged ();

	long _topRow, _leftColumn, _selectedRow, _selectedColumn;
	GuiObject _text, _drawingArea, _horizontalScrollBar, _verticalScrollBar;
	double _columnLeft [MAXNUM_VISIBLE_COLUMNS], _columnRight [MAXNUM_VISIBLE_COLUMNS];
	Graphics _graphics;

  private:
	#if gtk
	static void gui_cb_horizontalScroll(GtkRange *rng, gpointer void_me);
	static void gui_cb_verticalScroll(GtkRange *rng, gpointer void_me);
	static gboolean gui_cb_drawing_area_scroll(GuiObject w, GdkEventScroll *event, gpointer void_me);
	#elif motif
	static void gui_cb_horizontalScroll(GUI_ARGS);
	static void gui_cb_verticalScroll(GUI_ARGS);
	#endif

	#ifndef macintosh
	static int menu_cb_Cut (EDITOR_ARGS);
	static int menu_cb_Copy (EDITOR_ARGS);
	static int menu_cb_Paste (EDITOR_ARGS);
	static int menu_cb_Erase (EDITOR_ARGS);
	#endif

	static int menu_cb_TableEditorHelp (EDITOR_ARGS);
	static void gui_text_cb_change (I, GuiTextEvent event);
	static void gui_drawingarea_cb_expose (I, GuiDrawingAreaExposeEvent event);
	static void gui_drawingarea_cb_click (I, GuiDrawingAreaClickEvent event);
	static void gui_drawingarea_cb_resize (I, GuiDrawingAreaResizeEvent event);

	void createChildren ();
	void createMenus ();
};

/* End of file TableEditor.h */
#endif
