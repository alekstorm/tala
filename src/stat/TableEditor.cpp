/* TableEditor.cpp
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
 * pb 2006/02/11 first version
 * pb 2006/04/22 all cells visible
 * pb 2006/05/11 raised maximum number of visible columns
 * pb 2006/05/11 underscores are not subscripts
 * pb 2006/08/02 correct vertical scroll bar on Windows (has to be called "verticalScrollBar")
 * pb 2007/06/10 wchar_t
 * pb 2007/08/12 wchar_t
 * pb 2007/11/30 erased Graphics_printf
 * pb 2008/03/20 split off Help menu
 * fb 2010/02/24 GTK
 * pb 2011/03/20 C++
 */

#include "TableEditor.h"

#include "sys/machine.h"

/********** EDITOR METHODS **********/

#if gtk
	#define gui_cb_scroll(name, var) \
		void TableEditor::gui_cb_ ## name ## Scroll(GtkRange *rng, gpointer void_me) { \
			TableEditor *editor = (TableEditor *)void_me; \
			double var = gtk_range_get_value(rng); \
			do
#elif motif
	#define gui_cb_scroll(name, var) \
		void TableEditor::gui_cb_ ## name ## Scroll(GUI_ARGS) { \
			TableEditor *editor = (TableEditor *)void_me; \
			int var; \
			{ int slider, incr, pincr; \
			  XmScrollBarGetValues(w, &var, &slider, &incr, &pincr); } \
			do
#endif
#define gui_cb_scroll_end while (0); }

gui_cb_scroll(horizontal, value) {
	if ((int)value != editor->_leftColumn) {
		editor->_leftColumn = value;
		editor->draw ();
	}
} gui_cb_scroll_end

gui_cb_scroll(vertical, value) {
	if ((int)value != editor->_topRow) {
		editor->_topRow = value;
		editor->draw ();
	}
} gui_cb_scroll_end

#if gtk
gboolean TableEditor::gui_cb_drawing_area_scroll(GuiObject w, GdkEventScroll *event, gpointer void_me) {
	TableEditor *editor = (TableEditor *)void_me;
	double hv = gtk_range_get_value(GTK_RANGE(editor->_horizontalScrollBar));
	double hi = gtk_range_get_adjustment(GTK_RANGE(editor->_horizontalScrollBar))->step_increment;
	double vv = gtk_range_get_value(GTK_RANGE(editor->_verticalScrollBar));
	double vi = gtk_range_get_adjustment(GTK_RANGE(editor->_verticalScrollBar))->step_increment;
	switch (event->direction) {
		case GDK_SCROLL_UP:
			gtk_range_set_value(GTK_RANGE(editor->_verticalScrollBar), vv - vi);
			break;
		case GDK_SCROLL_DOWN:
			gtk_range_set_value(GTK_RANGE(editor->_verticalScrollBar), vv + vi);
			break;
		case GDK_SCROLL_LEFT:
			gtk_range_set_value(GTK_RANGE(editor->_horizontalScrollBar), hv - hi);
			break;
		case GDK_SCROLL_RIGHT:
			gtk_range_set_value(GTK_RANGE(editor->_horizontalScrollBar), hv + hi);
			break;
	}
	return TRUE;
}
#endif

TableEditor::TableEditor (GuiObject parent, const wchar_t *title, Table table)
	: Editor (parent, 0, 0, 700, 500, title, table),
	  _topRow(1), _leftColumn(1), _selectedColumn(1), _selectedRow(1) {
	createMenus ();
	createChildren ();
	//try { // FIXME exception
		#if motif
		Melder_assert (XtWindow (_drawingArea));
		#endif
		_graphics = Graphics_create_xmdrawingarea (_drawingArea);
		double size_pixels = SIZE_INCHES * Graphics_getResolution (_graphics);
		Graphics_setWsViewport (_graphics, 0, size_pixels, 0, size_pixels);
		Graphics_setWsWindow (_graphics, 0, size_pixels, 0, size_pixels);
		Graphics_setViewport (_graphics, 0, size_pixels, 0, size_pixels);
		Graphics_setFont (_graphics, kGraphics_font_COURIER);
		Graphics_setFontSize (_graphics, 12);
		Graphics_setUnderscoreIsSubscript (_graphics, FALSE);
		Graphics_setAtSignIsLink (_graphics, TRUE);

		#if gtk
		g_signal_connect(G_OBJECT(_drawingArea), "scroll-event", G_CALLBACK(gui_cb_drawing_area_scroll), this);
		g_signal_connect(G_OBJECT(_horizontalScrollBar), "value-changed", G_CALLBACK(gui_cb_horizontalScroll), this);
		g_signal_connect(G_OBJECT(_verticalScrollBar), "value-changed", G_CALLBACK(gui_cb_verticalScroll), this);
		#elif motif
		XtAddCallback (_horizontalScrollBar, XmNvalueChangedCallback, gui_cb_horizontalScroll, (XtPointer) this);
		XtAddCallback (_horizontalScrollBar, XmNdragCallback, gui_cb_horizontalScroll, (XtPointer) this);
		XtAddCallback (_verticalScrollBar, XmNvalueChangedCallback, gui_cb_verticalScroll, (XtPointer) this);
		XtAddCallback (_verticalScrollBar, XmNdragCallback, gui_cb_verticalScroll, (XtPointer) this);
		#endif
	/*} catch (...) {
		rethrowmzero ("TableEditor not created.");
	}*/
}

TableEditor::~TableEditor () {
	forget (_graphics);
}

void TableEditor::updateVerticalScrollBar () {
	Table table = static_cast<Table> (_data);
	#if motif
	/*int value, slider, incr, pincr;
	XmScrollBarGetValues (_verticalScrollBar, & value, & slider, & incr, & pincr);
	XmScrollBarSetValues (_verticalScrollBar, _topRow, slider, incr, pincr, False);*/
	XtVaSetValues (_verticalScrollBar,
		XmNvalue, _topRow, XmNmaximum, table -> rows -> size + 1, NULL);
	#endif
}

void TableEditor::updateHorizontalScrollBar () {
	Table table = static_cast<Table> (_data);
	#if motif
	/*int value, slider, incr, pincr;
	XmScrollBarGetValues (_horizontalScrollBar, & value, & slider, & incr, & pincr);
	XmScrollBarSetValues (_horizontalScrollBar, _topRow, slider, incr, pincr, False);*/
	XtVaSetValues (_horizontalScrollBar,
		XmNvalue, _leftColumn, XmNmaximum, table -> numberOfColumns + 1, NULL);
	#endif
}

void TableEditor::dataChanged () {
	Table table = static_cast<Table> (_data);
	if (_topRow > table -> rows -> size) _topRow = table -> rows -> size;
	if (_leftColumn > table -> numberOfColumns) _leftColumn = table -> numberOfColumns;
	updateVerticalScrollBar ();
	updateHorizontalScrollBar ();
	Graphics_updateWs (_graphics);
}

/********** FILE MENU **********/


/********** EDIT MENU **********/

#ifndef macintosh
int TableEditor::menu_cb_Cut (EDITOR_ARGS) {
	TableEditor *editor = (TableEditor *)editor;
	GuiText_cut (editor->_text);
	return 1;
}
int TableEditor::menu_cb_Copy (EDITOR_ARGS) {
	TableEditor *editor = (TableEditor *)editor;
	GuiText_copy (editor->_text);
	return 1;
}
int TableEditor::menu_cb_Paste (EDITOR_ARGS) {
	TableEditor *editor = (TableEditor *)editor;
	GuiText_paste (editor->_text);
	return 1;
}
int TableEditor::menu_cb_Erase (EDITOR_ARGS) {
	TableEditor *editor = (TableEditor *)editor;
	GuiText_remove (editor->_text);
	return 1;
}
#endif

/********** VIEW MENU **********/

/********** HELP MENU **********/

int TableEditor::menu_cb_TableEditorHelp (EDITOR_ARGS) {
	Melder_help (L"TableEditor");
	return 1;
}

/********** DRAWING AREA **********/

void TableEditor::draw () {
	Table table = static_cast<Table> (_data);
	double spacing = 2.0;   /* millimetres at both edges */
	double columnWidth, cellWidth;
	/*
	 * We fit 200 rows in 40 inches, which is 14.4 points per row.
	 */
	long rowmin = _topRow, rowmax = rowmin + 197;
	long colmin = _leftColumn, colmax = colmin + (MAXNUM_VISIBLE_COLUMNS - 1);
	if (rowmax > table -> rows -> size) rowmax = table -> rows -> size;
	if (colmax > table -> numberOfColumns) colmax = table -> numberOfColumns;
	Graphics_clearWs (_graphics);
	Graphics_setTextAlignment (_graphics, Graphics_CENTRE, Graphics_HALF);
	Graphics_setWindow (_graphics, 0.0, 1.0, rowmin + 197.5, rowmin - 2.5);
	Graphics_setColour (_graphics, Graphics_SILVER);
	Graphics_fillRectangle (_graphics, 0.0, 1.0, rowmin - 2.5, rowmin - 0.5);
	Graphics_setColour (_graphics, Graphics_BLACK);
	Graphics_line (_graphics, 0.0, rowmin - 0.5, 1.0, rowmin - 0.5);
	Graphics_setWindow (_graphics, 0.0, Graphics_dxWCtoMM (_graphics, 1.0), rowmin + 197.5, rowmin - 2.5);
	/*
	 * Determine the width of the column with the row numbers.
	 */
	columnWidth = Graphics_textWidth (_graphics, L"row");
	for (long irow = rowmin; irow <= rowmax; irow ++) {
		cellWidth = Graphics_textWidth (_graphics, Melder_integer (irow));
		if (cellWidth > columnWidth) columnWidth = cellWidth;
	}
	_columnLeft [0] = columnWidth + 2 * spacing;
	Graphics_setColour (_graphics, Graphics_SILVER);
	Graphics_fillRectangle (_graphics, 0.0, _columnLeft [0], rowmin - 0.5, rowmin + 197.5);
	Graphics_setColour (_graphics, Graphics_BLACK);
	Graphics_line (_graphics, _columnLeft [0], rowmin - 0.5, _columnLeft [0], rowmin + 197.5);
	/*
	 * Determine the width of the columns.
	 */
	for (long icol = colmin; icol <= colmax; icol ++) {
		const wchar_t *columnLabel = table -> columnHeaders [icol]. label;
		columnWidth = Graphics_textWidth (_graphics, Melder_integer (icol));
		if (columnLabel == NULL) columnLabel = L"";
		cellWidth = Graphics_textWidth (_graphics, columnLabel);
		if (cellWidth > columnWidth) columnWidth = cellWidth;
		for (long irow = rowmin; irow <= rowmax; irow ++) {
			const wchar_t *cell = Table_getStringValue_Assert (table, irow, icol);
			Melder_assert (cell != NULL);
			if (cell [0] == '\0') cell = L"?";
			cellWidth = Graphics_textWidth (_graphics, cell);
			if (cellWidth > columnWidth) columnWidth = cellWidth;
		}
		_columnRight [icol - colmin] = _columnLeft [icol - colmin] + columnWidth + 2 * spacing;
		if (icol < colmax) _columnLeft [icol - colmin + 1] = _columnRight [icol - colmin];
	}
	/*
	 * Show the row numbers.
	 */
	Graphics_text (_graphics, _columnLeft [0] / 2, rowmin - 1, L"row");
	for (long irow = rowmin; irow <= rowmax; irow ++) {
		Graphics_text1 (_graphics, _columnLeft [0] / 2, irow, Melder_integer (irow));
	}
	/*
	 * Show the column labels.
	 */
	for (long icol = colmin; icol <= colmax; icol ++) {
		double mid = (_columnLeft [icol - colmin] + _columnRight [icol - colmin]) / 2;
		const wchar_t *columnLabel = table -> columnHeaders [icol]. label;
		if (columnLabel == NULL || columnLabel [0] == '\0') columnLabel = L"?";
		Graphics_text1 (_graphics, mid, rowmin - 2, Melder_integer (icol));
		Graphics_text (_graphics, mid, rowmin - 1, columnLabel);
	}
	/*
	 * Show the cell contents.
	 */
	for (long irow = rowmin; irow <= rowmax; irow ++) {
		for (long icol = colmin; icol <= colmax; icol ++) {
			double mid = (_columnLeft [icol - colmin] + _columnRight [icol - colmin]) / 2;
			const wchar_t *cell = Table_getStringValue_Assert (table, irow, icol);
			Melder_assert (cell != NULL);
			if (cell [0] == '\0') cell = L"?";
			Graphics_text (_graphics, mid, irow, cell);
		}
	}
}

int TableEditor::click (double xclick, double yWC, int shiftKeyPressed) {
	Table table = static_cast<Table> (_data);
	return 1;
}

void TableEditor::gui_text_cb_change (I, GuiTextEvent event) {
	TableEditor *editor = (TableEditor *)void_me;
	(void) event;
	Table table = static_cast<Table> (editor->_data);
	editor->broadcastChange ();
}

void TableEditor::gui_drawingarea_cb_expose (I, GuiDrawingAreaExposeEvent event) {
	TableEditor *editor = (TableEditor *)void_me;
	(void) event;
	if (editor->_graphics == NULL) return;
	editor->draw ();
}

void TableEditor::gui_drawingarea_cb_click (I, GuiDrawingAreaClickEvent event) {
	TableEditor *editor = (TableEditor *)void_me;
	if (editor->_graphics == NULL) return;
	if (gtk && event -> type != BUTTON_PRESS) return;
	double xWC, yWC;
	Graphics_DCtoWC (editor->_graphics, event -> x, event -> y, & xWC, & yWC);
	// TODO: implement selection
}

void TableEditor::gui_drawingarea_cb_resize (I, GuiDrawingAreaResizeEvent event) {
	TableEditor *editor = (TableEditor *)void_me;
	if (editor->_graphics == NULL) return;
	Graphics_updateWs (editor->_graphics);
}

void TableEditor::createChildren () {
	Table table = static_cast<Table> (_data);
	GuiObject form;   /* A form inside a form; needed to keep key presses away from the drawing area. */

	#if gtk
		form = _dialog;
	#elif motif
		form = XmCreateForm (_dialog, "buttons", NULL, 0);
		XtVaSetValues (form,
			XmNleftAttachment, XmATTACH_FORM, XmNrightAttachment, XmATTACH_FORM,
			XmNtopAttachment, XmATTACH_FORM, XmNtopOffset, Machine_getMenuBarHeight (),
			XmNbottomAttachment, XmATTACH_FORM,
			XmNtraversalOn, False,   /* Needed in order to redirect all keyboard input to the text widget. */
			NULL);
	#endif

	/***** Create text field. *****/

	#if gtk
		_text = GuiText_create(NULL, 0, 0, 0, Machine_getTextHeight(), 0);
		gtk_box_pack_start(GTK_BOX(form), _text, FALSE, FALSE, 3);
		GuiObject_show(_text);
	#else
		_text = GuiText_createShown (form, 0, 0, 0, Machine_getTextHeight (), 0);
	#endif
	GuiText_setChangeCallback (_text, gui_text_cb_change, this);

	/***** Create drawing area. *****/
	
	#if gtk
		GuiObject table_container = gtk_table_new (2, 2, FALSE);
		gtk_box_pack_start (GTK_BOX (form), table_container, TRUE, TRUE, 3);
		GuiObject_show (table_container);
		
		_drawingArea = GuiDrawingArea_create (NULL, 0, 0, 0, 0,
			gui_drawingarea_cb_expose, gui_drawingarea_cb_click, NULL, gui_drawingarea_cb_resize, this, 0);
		
		// need to turn off double buffering, otherwise we receive the expose events
		// delayed by one event, see also FunctionEditor.c
		gtk_widget_set_double_buffered (_drawingArea, FALSE);
		
		gtk_table_attach (GTK_TABLE (table_container), _drawingArea, 0, 1, 0, 1,
			(GtkAttachOptions) (GTK_EXPAND | GTK_FILL), (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
		GuiObject_show (_drawingArea);
	#else
		_drawingArea = GuiDrawingArea_createShown (form, 0, - Machine_getScrollBarWidth (),
			Machine_getTextHeight (), - Machine_getScrollBarWidth (),
			gui_drawingarea_cb_expose, gui_drawingarea_cb_click, NULL, NULL, me, 0);
	#endif

	/***** Create horizontal scroll bar. *****/

	#if gtk
		GtkAdjustment *hadj = GTK_ADJUSTMENT (gtk_adjustment_new (1, 1, table->numberOfColumns + 1, 1, 3, 1));
		_horizontalScrollBar = gtk_hscrollbar_new (hadj);
		gtk_table_attach (GTK_TABLE(table_container), _horizontalScrollBar, 0, 1, 1, 2,
			(GtkAttachOptions) (GTK_EXPAND | GTK_FILL), (GtkAttachOptions) 0, 0, 0);
		GuiObject_show (_horizontalScrollBar);
	#elif motif
	_horizontalScrollBar = XtVaCreateManagedWidget ("horizontalScrollBar",
		xmScrollBarWidgetClass, form,
		XmNorientation, XmHORIZONTAL,
		XmNleftAttachment, XmATTACH_FORM, XmNleftOffset, 0,
		XmNrightAttachment, XmATTACH_FORM, XmNrightOffset, Machine_getScrollBarWidth (),
		XmNbottomAttachment, XmATTACH_FORM,
		XmNheight, Machine_getScrollBarWidth (),
		XmNminimum, 1,
		XmNmaximum, table -> numberOfColumns + 1,
		XmNvalue, 1,
		XmNsliderSize, 1,
		XmNincrement, 1,
		XmNpageIncrement, 3,
		NULL);
	#endif

	/***** Create vertical scroll bar. *****/

	#if gtk
		GtkAdjustment *vadj = GTK_ADJUSTMENT (gtk_adjustment_new (1, 1, table->rows->size + 1, 1, 10, 1));
		_verticalScrollBar = gtk_vscrollbar_new (vadj);
		gtk_table_attach (GTK_TABLE (table_container), _verticalScrollBar, 1, 2, 0, 1,
			(GtkAttachOptions) 0, (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
		GuiObject_show (_verticalScrollBar);
	#elif motif
	_verticalScrollBar = XtVaCreateManagedWidget ("verticalScrollBar",
		xmScrollBarWidgetClass, form,
		XmNorientation, XmVERTICAL,
		XmNtopAttachment, XmATTACH_FORM, XmNtopOffset, Machine_getTextHeight (),
		XmNbottomAttachment, XmATTACH_FORM, XmNbottomOffset, Machine_getScrollBarWidth (),
		XmNrightAttachment, XmATTACH_FORM, XmNrightOffset, 0,
		XmNwidth, Machine_getScrollBarWidth (),
		XmNminimum, 1,
		XmNmaximum, table -> rows -> size + 1,
		XmNvalue, 1,
		XmNsliderSize, 1,
		XmNincrement, 1,
		XmNpageIncrement, 10,
		NULL);
	#endif

	GuiObject_show (form);
}

void TableEditor::createMenus () {
	EditorMenu *menu = getMenu (L"Edit");
	#ifndef macintosh
	menu->addCommand (L"-- cut copy paste --", 0, NULL);
	menu->addCommand (L"Cut text", 'X', menu_cb_Cut);
	menu->addCommand (L"Cut", Editor_HIDDEN, menu_cb_Cut);
	menu->addCommand (L"Copy text", 'C', menu_cb_Copy);
	menu->addCommand (L"Copy", Editor_HIDDEN, menu_cb_Copy);
	menu->addCommand (L"Paste text", 'V', menu_cb_Paste);
	menu->addCommand (L"Paste", Editor_HIDDEN, menu_cb_Paste);
	menu->addCommand (L"Erase text", 0, menu_cb_Erase);
	menu->addCommand (L"Erase", Editor_HIDDEN, menu_cb_Erase);
	#endif

	menu = getMenu (L"Help");
	menu->addCommand (L"TableEditor help", '?', menu_cb_TableEditorHelp);
}

/* End of file TableEditor.cpp */
