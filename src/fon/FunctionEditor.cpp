/* FunctionEditor.c
 *
 * Copyright (C) 1992-2010 Paul Boersma
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * yoption) any later version.
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
 * pb 2002/07/16 GPL
 * pb 2004/10/16 C++ compatible struct tags
 * pb 2006/12/21 thicker moving cursor
 * pb 2007/06/10 wchar_t
 * pb 2007/08/12 wchar_t
 * pb 2007/09/19 info
 * pb 2007/09/21 query menu hierarchical
 * pb 2007/11/30 erased Graphics_printf
 * pb 2007/12/27 Gui
 * pb 2008/03/20 split off Help menu
 * pb 2009/09/21 Zoom Back
 * pb 2009/10/26 repaired the synchronizedZoomAndScroll preference
 * fb 2010/02/24 GTK
 * pb 2010/05/14 abolished resolution independence
 */

#include "FunctionEditor.h"
#include "sys/machine.h"
#include "sys/Preferences.h"
#include "sys/EditorM.h"

#define maximumScrollBarValue  2000000000
#define RELATIVE_PAGE_INCREMENT  0.8
#define SCROLL_INCREMENT_FRACTION  20
#define space 30
#define MARGIN 107
#define BOTTOM_MARGIN  2
#define TOP_MARGIN  3
#ifdef macintosh
	#define BUTTON_X  3
	#define BUTTON_WIDTH  40
	#define BUTTON_SPACING  8
#else
	#define BUTTON_X  1
	#define BUTTON_WIDTH  30
	#define BUTTON_SPACING  4
#endif

static struct {
	int shellWidth, shellHeight;
	bool synchronizedZoomAndScroll;
	double arrowScrollStep;
	struct { bool drawSelectionTimes, drawSelectionHairs; } picture;
} preferences;

void FunctionEditor::prefs (void) {
	Preferences_addInt (L"FunctionEditor.shellWidth", & preferences.shellWidth, 700);
	Preferences_addInt (L"FunctionEditor.shellHeight", & preferences.shellHeight, 440);
	Preferences_addBool (L"FunctionEditor.synchronizedZoomAndScroll", & preferences.synchronizedZoomAndScroll, true);
	Preferences_addDouble (L"FunctionEditor.arrowScrollStep", & preferences.arrowScrollStep, 0.05);   // BUG: seconds?
	Preferences_addBool (L"FunctionEditor.picture.drawSelectionTimes", & preferences.picture.drawSelectionTimes, true);
	Preferences_addBool (L"FunctionEditor.picture.drawSelectionHairs", & preferences.picture.drawSelectionHairs, true);
}

#define maxGroup 100
static int nGroup = 0;
static FunctionEditor *group [1 + maxGroup];

static int group_equalDomain (double tmin, double tmax) {
	if (nGroup == 0) return 1;
	for (int i = 1; i <= maxGroup; i ++)
		if (group [i])
			return tmin == group [i] -> _tmin && tmax == group [i] -> _tmax;
	return 0;   /* Should not occur. */
}

static void gui_drawingarea_cb_resize (I, GuiDrawingAreaResizeEvent event) {
	FunctionEditor *editor = (FunctionEditor *)void_me;
	if (editor->_graphics == NULL) return;   // Could be the case in the very beginning.
	Graphics_setWsViewport (editor->_graphics, 0, event -> width, 0, event -> height);
	editor->_width = event -> width + 21;
	editor->_height = event -> height + 111;
	Graphics_setWsWindow (editor->_graphics, 0, editor->_width, 0, editor->_height);
	Graphics_setViewport (editor->_graphics, 0, editor->_width, 0, editor->_height);
	#if gtk
		// updateWs() also resizes the cairo clipping context to the new window size
	#endif
	Graphics_updateWs (editor->_graphics);

	/* Save the current shell size as the user's preference for a new FunctionEditor. */

	preferences.shellWidth = GuiObject_getWidth (editor->_shell);
	preferences.shellHeight = GuiObject_getHeight (editor->_shell);
}

static void gui_checkbutton_cb_group (I, GuiCheckButtonEvent event) {
	FunctionEditor *editor = (FunctionEditor *)void_me;
	(void) event;
	int i;
	editor->_group = ! editor->_group;
	if (editor->_group) {
		FunctionEditor *thee;
		i = 1; while (group [i]) i ++; group [i] = editor;
		if (++ nGroup == 1) { Graphics_updateWs (editor->_graphics); return; }
		i = 1; while (group [i] == NULL || group [i] == editor) i ++; thee = group [i];
		if (preferences.synchronizedZoomAndScroll) {
			editor->_startWindow = thee->_startWindow;
			editor->_endWindow = thee->_endWindow;
		}
		editor->_startSelection = thee->_startSelection;
		editor->_endSelection = thee->_endSelection;
		if (editor->_tmin > thee->_tmin || editor->_tmax < thee->_tmax) {
			if (editor->_tmin > thee->_tmin) editor->_tmin = thee->_tmin;
			if (editor->_tmax < thee->_tmax) editor->_tmax = thee->_tmax;
			editor->updateText ();
			editor->updateScrollBar ();
			Graphics_updateWs (editor->_graphics);
		} else {
			editor->updateText ();
			editor->updateScrollBar ();
			Graphics_updateWs (editor->_graphics);
			if (editor->_tmin < thee->_tmin || editor->_tmax > thee->_tmax)
				for (i = 1; i <= maxGroup; i ++) if (group [i] && group [i] != editor) {
					if (editor->_tmin < thee->_tmin)
						group [i] -> _tmin = editor->_tmin;
					if (editor->_tmax > thee->_tmax)
						group [i] -> _tmax = editor->_tmax;
					group [i]->updateText ();
					group [i]->updateScrollBar ();
					Graphics_updateWs (group [i] -> _graphics);
				}
		}
	} else {
		i = 1; while (group [i] != editor) i ++; group [i] = NULL;
		nGroup --;
		editor->updateText ();
		Graphics_updateWs (editor->_graphics);   /* For setting buttons in draw method. */
	}
	if (editor->_group) editor->updateGroup ();
}

FunctionEditor::FunctionEditor (GuiObject parent, const wchar_t *title, Any data)
	: Editor (parent, 0, 0, preferences.shellWidth, preferences.shellHeight, title, data) {
	createMenus ();
	createChildren ();
	_tmin = ((Function) data) -> xmin;   /* Set before adding children (see group button). */
	_tmax = ((Function) data) -> xmax;

	_startWindow = _tmin;
	_endWindow = _tmax;
	_startSelection = _endSelection = 0.5 * (_tmin + _tmax);
	#if motif
		Melder_assert (XtWindow (_drawingArea));
	#endif
	_graphics = Graphics_create_xmdrawingarea (_drawingArea);
	Graphics_setFontSize (_graphics, 12);

{
// This exdents because it's a hack:
struct structGuiDrawingAreaResizeEvent event = { _drawingArea, 0 };
event. width = GuiObject_getWidth (_drawingArea);
event. height = GuiObject_getHeight (_drawingArea);
gui_drawingarea_cb_resize (this, & event);
}

	updateText ();
	if (group_equalDomain (_tmin, _tmax))
		gui_checkbutton_cb_group (this, NULL);   // BUG: NULL
	_enableUpdates = TRUE;
	_arrowScrollStep = preferences.arrowScrollStep;
}

FunctionEditor::~FunctionEditor () {
	MelderAudio_stopPlaying (MelderAudio_IMPLICIT);
	if (_group) {   /* Undangle. */
		int i = 1; while (group [i] != this) { Melder_assert (i < maxGroup); i ++; } group [i] = NULL;
		nGroup --;
	}
	forget (_graphics);
}

void FunctionEditor::updateScrollBar () {
/* We cannot call this immediately after creation. */
	int slider_size = (_endWindow - _startWindow) / (_tmax - _tmin) * maximumScrollBarValue - 1;
	int increment, page_increment;
	int value = (_startWindow - _tmin) / (_tmax - _tmin) * maximumScrollBarValue + 1;
	if (slider_size < 1) slider_size = 1;
	if (value > maximumScrollBarValue - slider_size)
		value = maximumScrollBarValue - slider_size;
	if (value < 1) value = 1;
	#if motif
		XtVaSetValues (_scrollBar, XmNmaximum, maximumScrollBarValue, NULL);
	#endif
	increment = slider_size / SCROLL_INCREMENT_FRACTION + 1;
	page_increment = RELATIVE_PAGE_INCREMENT * slider_size + 1;
	#if gtk
		GtkAdjustment *adj = gtk_range_get_adjustment (GTK_RANGE (_scrollBar));
		adj -> page_size = slider_size;
		gtk_adjustment_set_value (adj, value);
		gtk_adjustment_changed (adj);
		gtk_range_set_increments (GTK_RANGE (_scrollBar), increment, page_increment);
	#elif motif
		XmScrollBarSetValues (_scrollBar, value, slider_size, increment, page_increment, False);
	#endif
}

void FunctionEditor::updateGroup () {
	if (! _group) return;
	for (int i = 1; i <= maxGroup; i ++) if (group [i] && group [i] != this) {
		FunctionEditor *thee = group [i];
		if (preferences.synchronizedZoomAndScroll) {
			thee->_startWindow = _startWindow;
			thee->_endWindow = _endWindow;
		}
		thee->_startSelection = _startSelection;
		thee->_endSelection = _endSelection;
		thee->updateText ();
		thee->updateScrollBar ();
		Graphics_updateWs (thee->_graphics);
	}
}

void FunctionEditor::drawNow () {
	int i;
	int leftFromWindow = _startWindow > _tmin;
	int rightFromWindow = _endWindow < _tmax;
	int cursorVisible = _startSelection == _endSelection && _startSelection >= _startWindow && _startSelection <= _endWindow;
	int selection = _endSelection > _startSelection;
	int beginVisible, endVisible;
	double verticalCorrection, bottom;
	wchar_t text [100];

	/* Update selection. */

	beginVisible = _startSelection > _startWindow && _startSelection < _endWindow;
	endVisible = _endSelection > _startWindow && _endSelection < _endWindow;

	/* Update markers. */

	_numberOfMarkers = 0;
	if (beginVisible)
		_marker [++ _numberOfMarkers] = _startSelection;
	if (endVisible && _endSelection != _startSelection)
		_marker [++ _numberOfMarkers] = _endSelection;
	_marker [++ _numberOfMarkers] = _endWindow;
	NUMsort_d (_numberOfMarkers, _marker);

	/* Update rectangles. */

	for (i = 0; i < 8; i++) _rect [i]. left = _rect [i]. right = 0;

	/* 0: rectangle for total. */

	_rect [0]. left = leftFromWindow ? 0 : MARGIN;
	_rect [0]. right = _width - (rightFromWindow ? 0 : MARGIN);
	_rect [0]. bottom = BOTTOM_MARGIN;
	_rect [0]. top = BOTTOM_MARGIN + space;

	/* 1: rectangle for visible part. */

	_rect [1]. left = MARGIN;
	_rect [1]. right = _width - MARGIN;
	_rect [1]. bottom = BOTTOM_MARGIN + space;
	_rect [1]. top = BOTTOM_MARGIN + space * (_numberOfMarkers > 1 ? 2 : 3);

	/* 2: rectangle for left from visible part. */

	if (leftFromWindow) {
		_rect [2]. left = 0.0;
		_rect [2]. right = MARGIN;
		_rect [2]. bottom = BOTTOM_MARGIN + space;
		_rect [2]. top = BOTTOM_MARGIN + space * 2;
	}

	/* 3: rectangle for right from visible part. */

	if (rightFromWindow) {
		_rect [3]. left = _width - MARGIN;
		_rect [3]. right = _width;
		_rect [3]. bottom = BOTTOM_MARGIN + space;
		_rect [3]. top = BOTTOM_MARGIN + space * 2;
	}

	/* 4, 5, 6: rectangles between markers visible in visible part. */

	if (_numberOfMarkers > 1) {
		double window = _endWindow - _startWindow;
		for (i = 1; i <= _numberOfMarkers; i ++) {
			_rect [3 + i]. left = i == 1 ? MARGIN : MARGIN + (_width - MARGIN * 2) *
				(_marker [i - 1] - _startWindow) / window;
			_rect [3 + i]. right = MARGIN + (_width - MARGIN * 2) *
				(_marker [i] - _startWindow) / window;
			_rect [3 + i]. bottom = BOTTOM_MARGIN + space * 2;
			_rect [3 + i]. top = BOTTOM_MARGIN + space * 3;
		}
	}
	
	if (selection) {
		double window = _endWindow - _startWindow;
		double left =
			_startSelection == _startWindow ? MARGIN :
			_startSelection == _tmin ? 0.0 :
			_startSelection < _startWindow ? MARGIN * 0.3 :
			_startSelection < _endWindow ? MARGIN + (_width - MARGIN * 2) * (_startSelection - _startWindow) / window :
			_startSelection == _endWindow ? _width - MARGIN : _width - MARGIN * 0.7;
		double right =
			_endSelection < _startWindow ? MARGIN * 0.7 :
			_endSelection == _startWindow ? MARGIN :
			_endSelection < _endWindow ? MARGIN + (_width - MARGIN * 2) * (_endSelection - _startWindow) / window :
			_endSelection == _endWindow ? _width - MARGIN :
			_endSelection < _tmax ? _width - MARGIN * 0.3 : _width;
		_rect [7]. left = left;
		_rect [7]. right = right;
		_rect [7]. bottom = _height - space - TOP_MARGIN;
		_rect [7]. top = _height - TOP_MARGIN;
	}

	/*
	 * Be responsive: update the markers now.
	 */
	Graphics_setViewport (_graphics, 0, _width, 0, _height);
	Graphics_setWindow (_graphics, 0, _width, 0, _height);
	Graphics_setGrey (_graphics, 0.85);
	Graphics_fillRectangle (_graphics, MARGIN, _width - MARGIN, _height - TOP_MARGIN - space, _height);
	Graphics_fillRectangle (_graphics, 0, MARGIN, BOTTOM_MARGIN + ( leftFromWindow ? space * 2 : 0 ), _height);
	Graphics_fillRectangle (_graphics, _width - MARGIN, _width, BOTTOM_MARGIN + ( rightFromWindow ? space * 2 : 0 ), _height);
	Graphics_setGrey (_graphics, 0.0);
	#if defined (macintosh)
		Graphics_line (_graphics, 0, 2, _width, 2);
		Graphics_line (_graphics, 0, _height - 2, _width, _height - 2);
	#endif

	Graphics_setTextAlignment (_graphics, Graphics_CENTRE, Graphics_HALF);
	for (i = 0; i < 8; i ++) {
		double left = _rect [i]. left, right = _rect [i]. right;
		if (left < right)
			Graphics_button (_graphics, left, right, _rect [i]. bottom, _rect [i]. top);
	}
	verticalCorrection = _height / (_height - 111 + 11.0);
	#ifdef _WIN32
		verticalCorrection *= 1.5;
	#endif
	for (i = 0; i < 8; i ++) {
		double left = _rect [i]. left, right = _rect [i]. right;
		double bottom = _rect [i]. bottom, top = _rect [i]. top;
		if (left < right) {
			const wchar_t *format = format_long ();
			double value = NUMundefined, inverseValue = 0.0;
			switch (i) {
				case 0: format = format_totalDuration (), value = _tmax - _tmin; break;
				case 1: format = format_window (), value = _endWindow - _startWindow;
					/*
					 * Window domain text.
					 */	
					Graphics_setColour (_graphics, Graphics_BLUE);
					Graphics_setTextAlignment (_graphics, Graphics_LEFT, Graphics_HALF);
					Graphics_text1 (_graphics, left, 0.5 * (bottom + top) - verticalCorrection, Melder_fixed (_startWindow, fixedPrecision_long ()));
					Graphics_setTextAlignment (_graphics, Graphics_RIGHT, Graphics_HALF);
					Graphics_text1 (_graphics, right, 0.5 * (bottom + top) - verticalCorrection, Melder_fixed (_endWindow, fixedPrecision_long ()));
					Graphics_setColour (_graphics, Graphics_BLACK);
					Graphics_setTextAlignment (_graphics, Graphics_CENTRE, Graphics_HALF);
				break;
				case 2: value = _startWindow - _tmin; break;
				case 3: value = _tmax - _endWindow; break;
				case 4: value = _marker [1] - _startWindow; break;
				case 5: value = _marker [2] - _marker [1]; break;
				case 6: value = _marker [3] - _marker [2]; break;
				case 7: format = format_selection (), value = _endSelection - _startSelection, inverseValue = 1 / value; break;
			}
			swprintf (text, 100, format, value, inverseValue);
			if (Graphics_textWidth (_graphics, text) < right - left) {
				Graphics_text (_graphics, 0.5 * (left + right), 0.5 * (bottom + top) - verticalCorrection, text);
			} else if (format == format_long ()) {
				swprintf (text, 100, format_short (), value);
				if (Graphics_textWidth (_graphics, text) < right - left)
					Graphics_text (_graphics, 0.5 * (left + right), 0.5 * (bottom + top) - verticalCorrection, text);
			} else {
				swprintf (text, 100, format_long (), value);
				if (Graphics_textWidth (_graphics, text) < right - left) {
						Graphics_text (_graphics, 0.5 * (left + right), 0.5 * (bottom + top) - verticalCorrection, text);
				} else {
					swprintf (text, 100, format_short (), _endSelection - _startSelection);
					if (Graphics_textWidth (_graphics, text) < right - left)
						Graphics_text (_graphics, 0.5 * (left + right), 0.5 * (bottom + top) - verticalCorrection, text);
				}
			}
		}
	}

	Graphics_setViewport (_graphics, MARGIN, _width - MARGIN, 0, _height);
	Graphics_setWindow (_graphics, _startWindow, _endWindow, 0, _height);
	/*Graphics_setColour (_graphics, Graphics_WHITE);
	Graphics_fillRectangle (_graphics, _startWindow, _endWindow, BOTTOM_MARGIN + space * 3, _height - (TOP_MARGIN + space));*/
	Graphics_setColour (_graphics, Graphics_BLACK);
	Graphics_rectangle (_graphics, _startWindow, _endWindow, BOTTOM_MARGIN + space * 3, _height - (TOP_MARGIN + space));

	/*
	 * Red marker text.
	 */
	Graphics_setColour (_graphics, Graphics_RED);
	if (cursorVisible) {
		Graphics_setTextAlignment (_graphics, Graphics_CENTRE, Graphics_BOTTOM);
		Graphics_text1 (_graphics, _startSelection, _height - (TOP_MARGIN + space) - verticalCorrection, Melder_fixed (_startSelection, fixedPrecision_long ()));
	}
	if (beginVisible && selection) {
		Graphics_setTextAlignment (_graphics, Graphics_RIGHT, Graphics_HALF);
		Graphics_text1 (_graphics, _startSelection, _height - (TOP_MARGIN + space/2) - verticalCorrection, Melder_fixed (_startSelection, fixedPrecision_long ()));
	}
	if (endVisible && selection) {
		Graphics_setTextAlignment (_graphics, Graphics_LEFT, Graphics_HALF);
		Graphics_text1 (_graphics, _endSelection, _height - (TOP_MARGIN + space/2) - verticalCorrection, Melder_fixed (_endSelection, fixedPrecision_long ()));
	}
	Graphics_setColour (_graphics, Graphics_BLACK);

	/*
	 * To reduce flashing, give descendants the opportunity to prepare their data.
	 */
	prepareDraw ();

	/*
	 * Start of inner drawing.
	 */
	Graphics_setViewport (_graphics, MARGIN, _width - MARGIN, BOTTOM_MARGIN + space * 3, _height - (TOP_MARGIN + space));

	draw ();
	Graphics_setViewport (_graphics, MARGIN, _width - MARGIN, BOTTOM_MARGIN + space * 3, _height - (TOP_MARGIN + space));

	/*
	 * Red dotted marker lines.
	 */
	Graphics_setWindow (_graphics, _startWindow, _endWindow, 0.0, 1.0);
	Graphics_setColour (_graphics, Graphics_RED);
	Graphics_setLineType (_graphics, Graphics_DOTTED);
	bottom = getBottomOfSoundAndAnalysisArea ();
	if (cursorVisible)
		Graphics_line (_graphics, _startSelection, bottom, _startSelection, 1.0);
	if (beginVisible)
		Graphics_line (_graphics, _startSelection, bottom, _startSelection, 1.0);
	if (endVisible)
		Graphics_line (_graphics, _endSelection, bottom, _endSelection, 1.0);
	Graphics_setColour (_graphics, Graphics_BLACK);
	Graphics_setLineType (_graphics, Graphics_DRAWN);

	/*
	 * Highlight selection.
	 */
	if (selection && _startSelection < _endWindow && _endSelection > _startWindow) {
		double left = _startSelection, right = _endSelection;
		if (left < _startWindow) left = _startWindow;
		if (right > _endWindow) right = _endWindow;
		highlightSelection (left, right, 0.0, 1.0);
	}

	/*
	 * End of inner drawing.
	 */
	Graphics_setViewport (_graphics, 0, _width, 0, _height);
}

/********** METHODS **********/

void FunctionEditor::info () {
	Editor::info ();
	MelderInfo_writeLine4 (L"Editor start: ", Melder_double (_tmin), L" ", format_units ());
	MelderInfo_writeLine4 (L"Editor end: ", Melder_double (_tmax), L" ", format_units ());
	MelderInfo_writeLine4 (L"Window start: ", Melder_double (_startWindow), L" ", format_units ());
	MelderInfo_writeLine4 (L"Window end: ", Melder_double (_endWindow), L" ", format_units ());
	MelderInfo_writeLine4 (L"Selection start: ", Melder_double (_startSelection), L" ", format_units ());
	MelderInfo_writeLine4 (L"Selection end: ", Melder_double (_endSelection), L" ", format_units ());
	MelderInfo_writeLine4 (L"Arrow scroll step: ", Melder_double (_arrowScrollStep), L" ", format_units ());
	MelderInfo_writeLine2 (L"Group: ", _group ? L"yes" : L"no");
}

/********** FILE MENU **********/

static int menu_cb_preferences (EDITOR_ARGS) {
	FunctionEditor *editor = (FunctionEditor *)editor_me;
	EDITOR_FORM (L"Preferences", 0)
		BOOLEAN (L"Synchronize zoom and scroll", 1)
		POSITIVE (L"Arrow scroll step (s)", L"0.05")
		editor->prefs_addFields (cmd);
	EDITOR_OK
		SET_INTEGER (L"Synchronize zoom and scroll", preferences.synchronizedZoomAndScroll)
		SET_REAL (L"Arrow scroll step", editor->_arrowScrollStep)
		editor->prefs_setValues (cmd);
	EDITOR_DO
		bool oldSynchronizedZoomAndScroll = preferences.synchronizedZoomAndScroll;
		preferences.synchronizedZoomAndScroll = GET_INTEGER (L"Synchronize zoom and scroll");
		preferences.arrowScrollStep = editor->_arrowScrollStep = GET_REAL (L"Arrow scroll step");
		if (! oldSynchronizedZoomAndScroll && preferences.synchronizedZoomAndScroll) {
			editor->updateGroup ();
		}
		editor->prefs_getValues (cmd);
	EDITOR_END
}

void FunctionEditor::form_pictureSelection (EditorCommand *cmd) {
	BOOLEAN (L"Draw selection times", 1);
	BOOLEAN (L"Draw selection hairs", 1);
}
void FunctionEditor::ok_pictureSelection (EditorCommand *cmd) {
	SET_INTEGER (L"Draw selection times", preferences.picture.drawSelectionTimes);
	SET_INTEGER (L"Draw selection hairs", preferences.picture.drawSelectionHairs);
}
void FunctionEditor::do_pictureSelection (EditorCommand *cmd) {
	preferences.picture.drawSelectionTimes = GET_INTEGER (L"Draw selection times");
	preferences.picture.drawSelectionHairs = GET_INTEGER (L"Draw selection hairs");
}

/********** QUERY MENU **********/

static int menu_cb_getB (EDITOR_ARGS) {
	FunctionEditor *editor = (FunctionEditor *)editor_me;
	Melder_informationReal (editor->_startSelection, editor->format_units ());
	return 1;
}
static int menu_cb_getCursor (EDITOR_ARGS) {
	FunctionEditor *editor = (FunctionEditor *)editor_me;
	Melder_informationReal (0.5 * (editor->_startSelection + editor->_endSelection), editor->format_units ());
	return 1;
}
static int menu_cb_getE (EDITOR_ARGS) {
	FunctionEditor *editor = (FunctionEditor *)editor_me;
	Melder_informationReal (editor->_endSelection, editor->format_units ());
	return 1;
}
static int menu_cb_getSelectionDuration (EDITOR_ARGS) {
	FunctionEditor *editor = (FunctionEditor *)editor_me;
	Melder_informationReal (editor->_endSelection - editor->_startSelection, editor->format_units ());
	return 1;
}

/********** VIEW MENU **********/

static int menu_cb_zoom (EDITOR_ARGS) {
	FunctionEditor *editor = (FunctionEditor *)editor_me;
	EDITOR_FORM (L"Zoom", 0)
		REAL (L"From", L"0.0")
		REAL (L"To", L"1.0")
	EDITOR_OK
		SET_REAL (L"From", editor->_startWindow)
		SET_REAL (L"To", editor->_endWindow)
	EDITOR_DO
		editor->_startWindow = GET_REAL (L"From");
		if (editor->_startWindow < editor->_tmin + 1e-12)
			editor->_startWindow = editor->_tmin;
		editor->_endWindow = GET_REAL (L"To");
		if (editor->_endWindow > editor->_tmax - 1e-12)
			editor->_endWindow = editor->_tmax;
		editor->updateText ();
		editor->updateScrollBar ();
		/*Graphics_updateWs (_graphics);*/ editor->drawNow ();
		editor->updateGroup ();
	EDITOR_END
}

void FunctionEditor::do_showAll () {
	_startWindow = _tmin;
	_endWindow = _tmax;
	updateText ();
	updateScrollBar ();
	/*Graphics_updateWs (_graphics);*/ drawNow ();
	if (preferences.synchronizedZoomAndScroll) {
		updateGroup ();
	}
}

static void gui_button_cb_showAll (I, GuiButtonEvent event) {
	(void) event;
	FunctionEditor *editor = (FunctionEditor *)void_me;
	editor->do_showAll ();
}

void FunctionEditor::do_zoomIn () {
	double shift = (_endWindow - _startWindow) / 4;
	_startWindow += shift;
	_endWindow -= shift;
	updateText ();
	updateScrollBar ();
	/*Graphics_updateWs (_graphics);*/ drawNow ();
	if (preferences.synchronizedZoomAndScroll) {
		updateGroup ();
	}
}

static void gui_button_cb_zoomIn (I, GuiButtonEvent event) {
	(void) event;
	FunctionEditor *editor = (FunctionEditor *)void_me;
	editor->do_zoomIn ();
}

void FunctionEditor::do_zoomOut () {
	double shift = (_endWindow - _startWindow) / 2;
	MelderAudio_stopPlaying (MelderAudio_IMPLICIT);   /* Quickly, before window changes. */
	_startWindow -= shift;
	if (_startWindow < _tmin + 1e-12)
		_startWindow = _tmin;
	_endWindow += shift;
	if (_endWindow > _tmax - 1e-12)
		_endWindow = _tmax;
	updateText ();
	updateScrollBar ();
	/*Graphics_updateWs (_graphics);*/ drawNow ();
	if (preferences.synchronizedZoomAndScroll) {
		updateGroup ();
	}
}

static void gui_button_cb_zoomOut (I, GuiButtonEvent event) {
	(void) event;
	FunctionEditor *editor = (FunctionEditor *)void_me;
	editor->do_zoomOut ();
}

void FunctionEditor::do_zoomToSelection () {
	if (_endSelection > _startSelection) {
		_startZoomHistory = _startWindow;   // remember for Zoom Back
		_endZoomHistory = _endWindow;   // remember for Zoom Back
		//Melder_casual ("Zoomed in to %f ~ %f seconds.", _startSelection, _endSelection);
		_startWindow = _startSelection;
		_endWindow = _endSelection;
		updateText ();
		updateScrollBar ();
		/*Graphics_updateWs (_graphics);*/ drawNow ();
		if (preferences.synchronizedZoomAndScroll) {
			updateGroup ();
		}
	}
}

static void gui_button_cb_zoomToSelection (I, GuiButtonEvent event) {
	(void) event;
	FunctionEditor *editor = (FunctionEditor *)void_me;
	editor->do_zoomToSelection ();
}

void FunctionEditor::do_zoomBack () {
	if (_endZoomHistory > _startZoomHistory) {
		_startWindow = _startZoomHistory;
		_endWindow = _endZoomHistory;
		updateText ();
		updateScrollBar ();
		/*Graphics_updateWs (_graphics);*/ drawNow ();
		if (preferences.synchronizedZoomAndScroll) {
			updateGroup ();
		}
	}
}

static void gui_button_cb_zoomBack (I, GuiButtonEvent event) {
	(void) event;
	FunctionEditor *editor = (FunctionEditor *)void_me;
	editor->do_zoomBack ();
}

static int menu_cb_showAll (EDITOR_ARGS) {
	FunctionEditor *editor = (FunctionEditor *)editor_me;
	editor->do_showAll ();
	return 1;
}

static int menu_cb_zoomIn (EDITOR_ARGS) {
	FunctionEditor *editor = (FunctionEditor *)editor_me;
	editor->do_zoomIn ();
	return 1;
}

static int menu_cb_zoomOut (EDITOR_ARGS) {
	FunctionEditor *editor = (FunctionEditor *)editor_me;
	editor->do_zoomOut ();
	return 1;
}

static int menu_cb_zoomToSelection (EDITOR_ARGS) {
	FunctionEditor *editor = (FunctionEditor *)editor_me;
	editor->do_zoomToSelection ();
	return 1;
}

static int menu_cb_zoomBack (EDITOR_ARGS) {
	FunctionEditor *editor = (FunctionEditor *)editor_me;
	editor->do_zoomBack ();
	return 1;
}

static int menu_cb_play (EDITOR_ARGS) {
	FunctionEditor *editor = (FunctionEditor *)editor_me;
	EDITOR_FORM (L"Play", 0)
		REAL (L"From", L"0.0")
		REAL (L"To", L"1.0")
	EDITOR_OK
		SET_REAL (L"From", editor->_startWindow)
		SET_REAL (L"To", editor->_endWindow)
	EDITOR_DO
		MelderAudio_stopPlaying (MelderAudio_IMPLICIT);
		editor->play (GET_REAL (L"From"), GET_REAL (L"To"));
	EDITOR_END
}

static int menu_cb_playOrStop (EDITOR_ARGS) {
	FunctionEditor *editor = (FunctionEditor *)editor_me;
	if (MelderAudio_isPlaying) {
		MelderAudio_stopPlaying (MelderAudio_EXPLICIT);
	} else if (editor->_startSelection < editor->_endSelection) {
		editor->_playingSelection = TRUE;
		editor->play (editor->_startSelection, editor->_endSelection);
	} else {
		editor->_playingCursor = TRUE;
		if (editor->_startSelection == editor->_endSelection && editor->_startSelection > editor->_startWindow && editor->_startSelection < editor->_endWindow)
			editor->play (editor->_startSelection, editor->_endWindow);
		else
			editor->play (editor->_startWindow, editor->_endWindow);
	}
	return 1;
}

static int menu_cb_playWindow (EDITOR_ARGS) {
	FunctionEditor *editor = (FunctionEditor *)editor_me;
	MelderAudio_stopPlaying (MelderAudio_IMPLICIT);
	editor->_playingCursor = TRUE;
	editor->play (editor->_startWindow, editor->_endWindow);
	return 1;
}

static int menu_cb_interruptPlaying (EDITOR_ARGS) {
	MelderAudio_stopPlaying (MelderAudio_IMPLICIT);
	return 1;
}

/********** SELECT MENU **********/

static int menu_cb_select (EDITOR_ARGS) {
	FunctionEditor *editor = (FunctionEditor *)editor_me;
	EDITOR_FORM (L"Select", 0)
		REAL (L"Start of selection", L"0.0")
		REAL (L"End of selection", L"1.0")
	EDITOR_OK
		SET_REAL (L"Start of selection", editor->_startSelection)
		SET_REAL (L"End of selection", editor->_endSelection)
	EDITOR_DO
		editor->_startSelection = GET_REAL (L"Start of selection");
		if (editor->_startSelection < editor->_tmin + 1e-12)
			editor->_startSelection = editor->_tmin;
		editor->_endSelection = GET_REAL (L"End of selection");
		if (editor->_endSelection > editor->_tmax - 1e-12)
			editor->_endSelection = editor->_tmax;
		if (editor->_startSelection > editor->_endSelection) {
			double dummy = editor->_startSelection;
			editor->_startSelection = editor->_endSelection;
			editor->_endSelection = dummy;
		}
		editor->updateText ();
		/*Graphics_updateWs (_graphics);*/ editor->drawNow ();
		editor->updateGroup ();
	EDITOR_END
}

static int menu_cb_moveCursorToB (EDITOR_ARGS) {
	FunctionEditor *editor = (FunctionEditor *)editor_me;
	editor->_endSelection = editor->_startSelection;
	editor->updateText ();
	/*Graphics_updateWs (_graphics);*/ editor->drawNow ();
	editor->updateGroup ();
	return 1;
}

static int menu_cb_moveCursorToE (EDITOR_ARGS) {
	FunctionEditor *editor = (FunctionEditor *)editor_me;
	editor->_startSelection = editor->_endSelection;
	editor->updateText ();
	/*Graphics_updateWs (_graphics);*/ editor->drawNow ();
	editor->updateGroup ();
	return 1;
}

static int menu_cb_moveCursorTo (EDITOR_ARGS) {
	FunctionEditor *editor = (FunctionEditor *)editor_me;
	EDITOR_FORM (L"Move cursor to", 0)
		REAL (L"Position", L"0.0")
	EDITOR_OK
		SET_REAL (L"Position", 0.5 * (editor->_startSelection + editor->_endSelection))
	EDITOR_DO
		double position = GET_REAL (L"Position");
		if (position < editor->_tmin + 1e-12) position = editor->_tmin;
		if (position > editor->_tmax - 1e-12) position = editor->_tmax;
		editor->_startSelection = editor->_endSelection = position;
		editor->updateText ();
		/*Graphics_updateWs (_graphics);*/ editor->drawNow ();
		editor->updateGroup ();
	EDITOR_END
}

static int menu_cb_moveCursorBy (EDITOR_ARGS) {
	FunctionEditor *editor = (FunctionEditor *)editor_me;
	EDITOR_FORM (L"Move cursor by", 0)
		REAL (L"Distance", L"0.05")
	EDITOR_OK
	EDITOR_DO
		double position = 0.5 * (editor->_startSelection + editor->_endSelection) + GET_REAL (L"Distance");
		if (position < editor->_tmin) position = editor->_tmin;
		if (position > editor->_tmax) position = editor->_tmax;
		editor->_startSelection = editor->_endSelection = position;
		editor->updateText ();
		/*Graphics_updateWs (_graphics);*/ editor->drawNow ();
		editor->updateGroup ();
	EDITOR_END
}

static int menu_cb_moveBby (EDITOR_ARGS) {
	FunctionEditor *editor = (FunctionEditor *)editor_me;
	EDITOR_FORM (L"Move start of selection by", 0)
		REAL (L"Distance", L"0.05")
	EDITOR_OK
	EDITOR_DO
		double position = editor->_startSelection + GET_REAL (L"Distance");
		if (position < editor->_tmin) position = editor->_tmin;
		if (position > editor->_tmax) position = editor->_tmax;
		editor->_startSelection = position;
		if (editor->_startSelection > editor->_endSelection) {
			double dummy = editor->_startSelection;
			editor->_startSelection = editor->_endSelection;
			editor->_endSelection = dummy;
		}
		editor->updateText ();
		/*Graphics_updateWs (_graphics);*/ editor->drawNow ();
		editor->updateGroup ();
	EDITOR_END
}

static int menu_cb_moveEby (EDITOR_ARGS) {
	FunctionEditor *editor = (FunctionEditor *)editor_me;
	EDITOR_FORM (L"Move end of selection by", 0)
		REAL (L"Distance", L"0.05")
	EDITOR_OK
	EDITOR_DO
		double position = editor->_endSelection + GET_REAL (L"Distance");
		if (position < editor->_tmin) position = editor->_tmin;
		if (position > editor->_tmax) position = editor->_tmax;
		editor->_endSelection = position;
		if (editor->_startSelection > editor->_endSelection) {
			double dummy = editor->_startSelection;
			editor->_startSelection = editor->_endSelection;
			editor->_endSelection = dummy;
		}
		editor->updateText ();
		/*Graphics_updateWs (editor->_graphics);*/ editor->drawNow ();
		editor->updateGroup ();
	EDITOR_END
}

void FunctionEditor::shift (double shift) {
	double windowLength = _endWindow - _startWindow;
	MelderAudio_stopPlaying (MelderAudio_IMPLICIT);   /* Quickly, before window changes. */
	if (shift < 0.0) {
		_startWindow += shift;
		if (_startWindow < _tmin + 1e-12)
			_startWindow = _tmin;
		_endWindow = _startWindow + windowLength;
		if (_endWindow > _tmax - 1e-12)
			_endWindow = _tmax;
	} else {
		_endWindow += shift;
		if (_endWindow > _tmax - 1e-12)
			_endWindow = _tmax;
		_startWindow = _endWindow - windowLength;
		if (_startWindow < _tmin + 1e-12)
			_startWindow = _tmin;
	}
	marksChanged ();
}

static int menu_cb_pageUp (EDITOR_ARGS) {
	FunctionEditor *editor = (FunctionEditor *)editor_me;
	editor->shift (-RELATIVE_PAGE_INCREMENT * (editor->_endWindow - editor->_startWindow));
	return 1;
}

static int menu_cb_pageDown (EDITOR_ARGS) {
	FunctionEditor *editor = (FunctionEditor *)editor_me;
	editor->shift (+RELATIVE_PAGE_INCREMENT * (editor->_endWindow - editor->_startWindow));
	return 1;
}

void FunctionEditor::scrollToView (double t) {
	if (t <= _startWindow) {
		shift (t - _startWindow - 0.618 * (_endWindow - _startWindow));
	} else if (t >= _endWindow) {
		shift (t - _endWindow + 0.618 * (_endWindow - _startWindow));
	} else {
		marksChanged ();
	}
}

static int menu_cb_selectEarlier (EDITOR_ARGS) {
	FunctionEditor *editor = (FunctionEditor *)editor_me;
	editor->_startSelection -= editor->_arrowScrollStep;
	if (editor->_startSelection < editor->_tmin + 1e-12)
		editor->_startSelection = editor->_tmin;
	editor->_endSelection -= editor->_arrowScrollStep;
	if (editor->_endSelection < editor->_tmin + 1e-12)
		editor->_endSelection = editor->_tmin;
	editor->scrollToView (0.5 * (editor->_startSelection + editor->_endSelection));
	return 1;
}

static int menu_cb_selectLater (EDITOR_ARGS) {
	FunctionEditor *editor = (FunctionEditor *)editor_me;
	editor->_startSelection += editor->_arrowScrollStep;
	if (editor->_startSelection > editor->_tmax - 1e-12)
		editor->_startSelection = editor->_tmax;
	editor->_endSelection += editor->_arrowScrollStep;
	if (editor->_endSelection > editor->_tmax - 1e-12)
		editor->_endSelection = editor->_tmax;
	editor->scrollToView (0.5 * (editor->_startSelection + editor->_endSelection));
	return 1;
}

static int menu_cb_moveBleft (EDITOR_ARGS) {
	FunctionEditor *editor = (FunctionEditor *)editor_me;
	editor->_startSelection -= editor->_arrowScrollStep;
	if (editor->_startSelection < editor->_tmin + 1e-12)
		editor->_startSelection = editor->_tmin;
	editor->scrollToView (0.5 * (editor->_startSelection + editor->_endSelection));
	return 1;
}

static int menu_cb_moveBright (EDITOR_ARGS) {
	FunctionEditor *editor = (FunctionEditor *)editor_me;
	editor->_startSelection += editor->_arrowScrollStep;
	if (editor->_startSelection > editor->_tmax - 1e-12)
		editor->_startSelection = editor->_tmax;
	if (editor->_startSelection > editor->_endSelection) {
		double dummy = editor->_startSelection;
		editor->_startSelection = editor->_endSelection;
		editor->_endSelection = dummy;
	}
	editor->scrollToView (0.5 * (editor->_startSelection + editor->_endSelection));
	return 1;
}

static int menu_cb_moveEleft (EDITOR_ARGS) {
	FunctionEditor *editor = (FunctionEditor *)editor_me;
	editor->_endSelection -= editor->_arrowScrollStep;
	if (editor->_endSelection < editor->_tmin + 1e-12)
		editor->_endSelection = editor->_tmin;
	if (editor->_startSelection > editor->_endSelection) {
		double dummy = editor->_startSelection;
		editor->_startSelection = editor->_endSelection;
		editor->_endSelection = dummy;
	}
	editor->scrollToView (0.5 * (editor->_startSelection + editor->_endSelection));
	return 1;
}

static int menu_cb_moveEright (EDITOR_ARGS) {
	FunctionEditor *editor = (FunctionEditor *)editor_me;
	editor->_endSelection += editor->_arrowScrollStep;
	if (editor->_endSelection > editor->_tmax - 1e-12)
		editor->_endSelection = editor->_tmax;
	editor->scrollToView (0.5 * (editor->_startSelection + editor->_endSelection));
	return 1;
}

/********** GUI CALLBACKS **********/

#if gtk
static void gui_cb_scroll (GtkRange *rng, gpointer void_me) {
	FunctionEditor *editor = (FunctionEditor *)void_me;
	if (editor->_graphics == NULL) return;   // ignore events during creation
	double value = gtk_range_get_value (GTK_RANGE (rng));
	double shift = editor->_tmin + (value - 1) * (editor->_tmax - editor->_tmin) / maximumScrollBarValue - editor->_startWindow;
	if (shift != 0.0) {
		int i;
		editor->_startWindow += shift;
		if (editor->_startWindow < editor->_tmin + 1e-12) editor->_startWindow = editor->_tmin;
		editor->_endWindow += shift;
		if (editor->_endWindow > editor->_tmax - 1e-12) editor->_endWindow = editor->_tmax;
		editor->updateText ();
		/*Graphics_clearWs (_graphics);*/
		editor->drawNow ();   /* Do not wait for expose event. */
		if (! editor->_group || ! preferences.synchronizedZoomAndScroll) return;
		for (i = 1; i <= maxGroup; i ++) if (group [i] && group [i] != editor) {
			group [i] -> _startWindow = editor->_startWindow;
			group [i] -> _endWindow = editor->_endWindow;
			group [i]->updateText ();
			group [i]->updateScrollBar ();
			Graphics_clearWs (group [i] -> _graphics);
			group [i]->drawNow ();
		}
	}
}
#else
static void gui_cb_scroll (GUI_ARGS) {
	GUI_IAM (FunctionEditor);
	if (_graphics == NULL) return;   // ignore events during creation
	int value, slider, incr, pincr;
	XmScrollBarGetValues (w, & value, & slider, & incr, & pincr);
	double shift = editor->_tmin + (value - 1) * (editor->_tmax - editor->_tmin) / maximumScrollBarValue - _startWindow;
	if (shift != 0.0) {
		int i;
		_startWindow += shift;
		if (_startWindow < editor->_tmin + 1e-12) _startWindow = editor->_tmin;
		_endWindow += shift;
		if (_endWindow > editor->_tmax - 1e-12) _endWindow = editor->_tmax;
		updateText ();
		/*Graphics_clearWs (_graphics);*/
		drawNow ();   /* Do not wait for expose event. */
		if (! _group || ! preferences.synchronizedZoomAndScroll) return;
		for (i = 1; i <= maxGroup; i ++) if (group [i] && group [i] != me) {
			group [i] -> startWindow = _startWindow;
			group [i] -> endWindow = _endWindow;
			FunctionEditor_updateText (group [i]);
			updateScrollBar (group [i]);
			Graphics_clearWs (group [i] -> graphics);
			drawNow (group [i]);
		}
	}
}
#endif

static int menu_cb_intro (EDITOR_ARGS) {
	FunctionEditor *editor = (FunctionEditor *)editor_me;
	Melder_help (L"Intro");
	return 1;
}

void FunctionEditor::createMenuItems_file (EditorMenu *menu) {
	menu->addCommand (L"Preferences...", 0, menu_cb_preferences);
	menu->addCommand (L"-- after preferences --", 0, 0);
}

void FunctionEditor::createMenuItems_view_timeDomain (EditorMenu *menu) {
	menu->addCommand (format_domain (), GuiMenu_INSENSITIVE, menu_cb_zoom /* dum_*/);
	menu->addCommand (L"Zoom...", 0, menu_cb_zoom);
	menu->addCommand (L"Show all", 'A', menu_cb_showAll);
	menu->addCommand (L"Zoom in", 'I', menu_cb_zoomIn);
	menu->addCommand (L"Zoom out", 'O', menu_cb_zoomOut);
	menu->addCommand (L"Zoom to selection", 'N', menu_cb_zoomToSelection);
	menu->addCommand (L"Zoom back", 'B', menu_cb_zoomBack);
	menu->addCommand (L"Scroll page back", GuiMenu_PAGE_UP, menu_cb_pageUp);
	menu->addCommand (L"Scroll page forward", GuiMenu_PAGE_DOWN, menu_cb_pageDown);
}

void FunctionEditor::createMenuItems_view_audio (EditorMenu *menu) {
	menu->addCommand (L"-- play --", 0, 0);
	menu->addCommand (L"Audio:", GuiMenu_INSENSITIVE, menu_cb_play /* dum_*/);
	menu->addCommand (L"Play...", 0, menu_cb_play);
	menu->addCommand (L"Play or stop", gtk ? 0 : GuiMenu_TAB, menu_cb_playOrStop);
	menu->addCommand (L"Play window", gtk ? 0 : GuiMenu_SHIFT + GuiMenu_TAB, menu_cb_playWindow);
	menu->addCommand (L"Interrupt playing", GuiMenu_ESCAPE, menu_cb_interruptPlaying);
}

void FunctionEditor::createMenuItems_view (EditorMenu *menu) {
	createMenuItems_view_timeDomain (menu);
	createMenuItems_view_audio (menu);
}

void FunctionEditor::createMenuItems_query (EditorMenu *menu) {
	menu->addCommand (L"-- query selection --", 0, 0);
	menu->addCommand (L"Get start of selection", 0, menu_cb_getB);
	menu->addCommand (L"Get begin of selection", Editor_HIDDEN, menu_cb_getB);
	menu->addCommand (L"Get cursor", GuiMenu_F6, menu_cb_getCursor);
	menu->addCommand (L"Get end of selection", 0, menu_cb_getE);
	menu->addCommand (L"Get selection length", 0, menu_cb_getSelectionDuration);
}

void FunctionEditor::createMenus () {
	EditorMenu *menu;

	menu = addMenu (L"View", 0);
	createMenuItems_view (menu);

	addMenu (L"Select", 0);
	addCommand (L"Select", L"Select...", 0, menu_cb_select);
	addCommand (L"Select", L"Move cursor to start of selection", 0, menu_cb_moveCursorToB);
	addCommand (L"Select", L"Move cursor to begin of selection", Editor_HIDDEN, menu_cb_moveCursorToB);
	addCommand (L"Select", L"Move cursor to end of selection", 0, menu_cb_moveCursorToE);
	addCommand (L"Select", L"Move cursor to...", 0, menu_cb_moveCursorTo);
	addCommand (L"Select", L"Move cursor by...", 0, menu_cb_moveCursorBy);
	addCommand (L"Select", L"Move start of selection by...", 0, menu_cb_moveBby);
	addCommand (L"Select", L"Move begin of selection by...", Editor_HIDDEN, menu_cb_moveBby);
	addCommand (L"Select", L"Move end of selection by...", 0, menu_cb_moveEby);
	/*addCommand (L"Select", L"Move cursor back by half a second", motif_, menu_cb_moveCursorBy);*/
	addCommand (L"Select", L"Select earlier", GuiMenu_UP_ARROW, menu_cb_selectEarlier);
	addCommand (L"Select", L"Select later", GuiMenu_DOWN_ARROW, menu_cb_selectLater);
	addCommand (L"Select", L"Move start of selection left", GuiMenu_SHIFT + GuiMenu_UP_ARROW, menu_cb_moveBleft);
	addCommand (L"Select", L"Move begin of selection left", Editor_HIDDEN, menu_cb_moveBleft);
	addCommand (L"Select", L"Move start of selection right", GuiMenu_SHIFT + GuiMenu_DOWN_ARROW, menu_cb_moveBright);
	addCommand (L"Select", L"Move begin of selection right", Editor_HIDDEN, menu_cb_moveBright);
	addCommand (L"Select", L"Move end of selection left", GuiMenu_COMMAND + GuiMenu_UP_ARROW, menu_cb_moveEleft);
	addCommand (L"Select", L"Move end of selection right", GuiMenu_COMMAND + GuiMenu_DOWN_ARROW, menu_cb_moveEright);
}

void FunctionEditor::createHelpMenuItems (EditorMenu *menu) {
	menu->addCommand (L"Intro", 0, menu_cb_intro);
}

static void gui_drawingarea_cb_expose (I, GuiDrawingAreaExposeEvent event) {
	FunctionEditor *editor = (FunctionEditor *)void_me;
	(void) event;
	if (editor->_graphics == NULL) return;   // Could be the case in the very beginning.
	if (editor->_enableUpdates)
		editor->drawNow ();
}

static void gui_drawingarea_cb_click (I, GuiDrawingAreaClickEvent event) {
	FunctionEditor *editor = (FunctionEditor *)void_me;
	if (editor->_graphics == NULL) return;   // Could be the case in the very beginning.
	if (gtk && event -> type != BUTTON_PRESS) return;
	double xWC, yWC;
	editor->_shiftKeyPressed = event -> shiftKeyPressed;
	Graphics_setWindow (editor->_graphics, 0, editor->_width, 0, editor->_height);
	Graphics_DCtoWC (editor->_graphics, event -> x, event -> y, & xWC, & yWC);

	if (yWC > BOTTOM_MARGIN + space * 3 && yWC < editor->_height - (TOP_MARGIN + space)) {   /* In signal region? */
		int needsUpdate;
		Graphics_setViewport (editor->_graphics, MARGIN, editor->_width - MARGIN,
			BOTTOM_MARGIN + space * 3, editor->_height - (TOP_MARGIN + space));
		Graphics_setWindow (editor->_graphics, editor->_startWindow, editor->_endWindow, 0.0, 1.0);
		Graphics_DCtoWC (editor->_graphics, event -> x, event -> y, & xWC, & yWC);
		if (xWC < editor->_startWindow) xWC = editor->_startWindow;
		if (xWC > editor->_endWindow) xWC = editor->_endWindow;
		if (Melder_debug == 24) {
			Melder_casual ("FunctionEditor::gui_drawingarea_cb_click: button %d shift %d option %d command %d control %d",
				event -> button, editor->_shiftKeyPressed, event -> optionKeyPressed, event -> commandKeyPressed, event -> extraControlKeyPressed);
		}
#if defined (macintosh)
		needsUpdate =
			event -> optionKeyPressed || event -> extraControlKeyPressed ? editor->clickB (xWC, yWC) :
			event -> commandKeyPressed ? editor->clickE (xWC, yWC) :
			editor->click (xWC, yWC, editor->_shiftKeyPressed);
#elif defined (_WIN32)
		needsUpdate =
			event -> commandKeyPressed ? editor->clickB (xWC, yWC) :
			event -> optionKeyPressed ? editor->clickE (xWC, yWC) :
			editor->click (xWC, yWC, editor->_shiftKeyPressed);
#else
		needsUpdate =
			event -> commandKeyPressed ? editor->clickB (xWC, yWC) :
			event -> optionKeyPressed ? editor->clickE (xWC, yWC) :
			event -> button == 1 ? editor->click (xWC, yWC, editor->_shiftKeyPressed) :
			event -> button == 2 ? editor->clickB (xWC, yWC) : editor->clickE (xWC, yWC);
#endif
		if (needsUpdate) editor->updateText ();
		Graphics_setViewport (editor->_graphics, 0, editor->_width, 0, editor->_height);
		if (needsUpdate) /*Graphics_updateWs (_graphics);*/ editor->drawNow ();
		if (needsUpdate) editor->updateGroup ();
	}
	else   /* Clicked outside signal region? Let us hear it. */
	{
		int i;
		for (i = 0; i < 8; i ++)
			if (xWC > editor->_rect [i]. left && xWC < editor->_rect [i]. right &&
				 yWC > editor->_rect [i]. bottom && yWC < editor->_rect [i]. top)
				switch (i) {
					case 0: editor->play (editor->_tmin, editor->_tmax); break;
					case 1: editor->play (editor->_startWindow, editor->_endWindow); break;
					case 2: editor->play (editor->_tmin, editor->_startWindow); break;
					case 3: editor->play (editor->_endWindow, editor->_tmax); break;
					case 4: editor->play (editor->_startWindow, editor->_marker [1]); break;
					case 5: editor->play (editor->_marker [1], editor->_marker [2]); break;
					case 6: editor->play (editor->_marker [2], editor->_marker [3]); break;
					case 7: editor->play (editor->_startSelection, editor->_endSelection); break;
				}
	}
}

static void gui_drawingarea_cb_key (I, GuiDrawingAreaKeyEvent event) {
	FunctionEditor *editor = (FunctionEditor *)void_me;
	if (editor->_graphics == NULL) return;   // Could be the case in the very beginning.
	editor->key (event -> key);
}

void FunctionEditor::createChildren () {
	GuiObject form;
	int x = BUTTON_X;

	#if gtk
		form = _dialog;
		GuiObject hctl_box = gtk_hbox_new (FALSE, BUTTON_SPACING);
		gtk_box_pack_end (GTK_BOX (form), hctl_box, FALSE, FALSE, 0);
		GuiObject leftbtn_box = gtk_hbox_new (TRUE, 3);
		gtk_box_pack_start (GTK_BOX (hctl_box), leftbtn_box, FALSE, FALSE, 0);

		/***** Create zoom buttons. *****/

		gtk_box_pack_start (GTK_BOX (leftbtn_box),
			GuiButton_create (NULL, 0, 0, 0, 0, L"all", gui_button_cb_showAll, this, 0), TRUE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (leftbtn_box),
			GuiButton_create (NULL, 0, 0, 0, 0, L"in", gui_button_cb_zoomIn, this, 0), TRUE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (leftbtn_box),
			GuiButton_create (NULL, 0, 0, 0, 0, L"out", gui_button_cb_zoomOut, this, 0), TRUE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (leftbtn_box),
			GuiButton_create (NULL, 0, 0, 0, 0, L"sel", gui_button_cb_zoomToSelection, this, 0), TRUE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (leftbtn_box),
			GuiButton_create (NULL, 0, 0, 0, 0, L"bak", gui_button_cb_zoomBack, this, 0), TRUE, TRUE, 0);

		GuiObject_show (leftbtn_box);
	#elif motif
		form = XmCreateForm (_dialog, "buttons", NULL, 0);
		XtVaSetValues (form,
			XmNleftAttachment, XmATTACH_FORM, XmNrightAttachment, XmATTACH_FORM,
			XmNtopAttachment, XmATTACH_FORM, XmNtopOffset, Machine_getMenuBarHeight (),
			XmNbottomAttachment, XmATTACH_FORM,
			XmNtraversalOn, False,   /* Needed in order to redirect all keyboard input to the text widget. */
			NULL);

		/***** Create zoom buttons. *****/

		GuiButton_createShown (form, x, x + BUTTON_WIDTH, -6 - Machine_getScrollBarWidth (), -4,
			L"all", gui_button_cb_showAll, this, 0);
		x += BUTTON_WIDTH + BUTTON_SPACING;
		GuiButton_createShown (form, x, x + BUTTON_WIDTH, -6 - Machine_getScrollBarWidth (), -4,
			L"in", gui_button_cb_zoomIn, this, 0);
		x += BUTTON_WIDTH + BUTTON_SPACING;
		GuiButton_createShown (form, x, x + BUTTON_WIDTH, -6 - Machine_getScrollBarWidth (), -4,
			L"out", gui_button_cb_zoomOut, this, 0);
		x += BUTTON_WIDTH + BUTTON_SPACING;
		GuiButton_createShown (form, x, x + BUTTON_WIDTH, -6 - Machine_getScrollBarWidth (), -4,
			L"sel", gui_button_cb_zoomToSelection, this, 0);
		x += BUTTON_WIDTH + BUTTON_SPACING;
		GuiButton_createShown (form, x, x + BUTTON_WIDTH, -6 - Machine_getScrollBarWidth (), -4,
			L"bak", gui_button_cb_zoomBack, this, 0);
	#endif

	/***** Create scroll bar. *****/

	#if gtk
		GtkObject *adj = gtk_adjustment_new (1, 1, maximumScrollBarValue, 1, 1, maximumScrollBarValue - 1);
		_scrollBar = gtk_hscrollbar_new (GTK_ADJUSTMENT (adj));
		g_signal_connect (G_OBJECT (_scrollBar), "value-changed", G_CALLBACK (gui_cb_scroll), this);
		GuiObject_show (_scrollBar);
		gtk_box_pack_start (GTK_BOX (hctl_box), _scrollBar, TRUE, TRUE, 3);
	#elif motif
		_scrollBar = XtVaCreateManagedWidget ("scrollBar",
			xmScrollBarWidgetClass, form,
			XmNorientation, XmHORIZONTAL,
			XmNleftAttachment, XmATTACH_FORM, XmNleftOffset, x += BUTTON_WIDTH + BUTTON_SPACING,
			XmNrightAttachment, XmATTACH_FORM, XmNrightOffset, 80 + BUTTON_SPACING,
			XmNbottomAttachment, XmATTACH_FORM,
			XmNheight, Machine_getScrollBarWidth (),
			XmNminimum, 1,
			XmNmaximum, maximumScrollBarValue,
			XmNvalue, 1,
			XmNsliderSize, maximumScrollBarValue - 1,
			NULL);
		XtAddCallback (_scrollBar, XmNvalueChangedCallback, gui_cb_scroll, (XtPointer) this);
		XtAddCallback (_scrollBar, XmNdragCallback, gui_cb_scroll, (XtPointer) this);
	#endif

	/***** Create Group button. *****/

	#if gtk
		_groupButton = GuiCheckButton_create (NULL, 0, 0, 0, 0, L"Group",
			gui_checkbutton_cb_group, this, group_equalDomain (_tmin, _tmax) ? GuiCheckButton_SET : 0);
		gtk_box_pack_start (GTK_BOX (hctl_box), _groupButton, FALSE, FALSE, 0);
		gtk_widget_show_all (hctl_box);
	#else
		_groupButton = GuiCheckButton_createShown (form, -80, 0, - Machine_getScrollBarWidth () - 5, -4,
			L"Group", gui_checkbutton_cb_group, this, group_equalDomain (_tmin, _tmax) ? GuiCheckButton_SET : 0);
	#endif

	/***** Create drawing area. *****/

	#if gtk
		_drawingArea = GuiDrawingArea_create (NULL, 0, 0, 0, - Machine_getScrollBarWidth () - 9,
			gui_drawingarea_cb_expose, gui_drawingarea_cb_click, gui_drawingarea_cb_key, gui_drawingarea_cb_resize, this, 0);
		
		// turn off double-buffering, otherwise the reaction to the expose-events gets
		// delayed by one event (TODO: figure out, why)
		gtk_widget_set_double_buffered (_drawingArea, FALSE);
		
		// turn off clearing window to background colbefore an expose event
		// gtk_widget_set_app_paintable (_drawingArea, FALSE);
		
		gtk_box_pack_start (GTK_BOX (form), _drawingArea, TRUE, TRUE, 0);
		GuiObject_show (_drawingArea);
	#else
		_drawingArea = GuiDrawingArea_createShown (form, 0, 0, 0, - Machine_getScrollBarWidth () - 9,
			gui_drawingarea_cb_expose, gui_drawingarea_cb_click, gui_drawingarea_cb_key, gui_drawingarea_cb_resize, this, 0);
	#endif

	GuiObject_show (form);
}

void FunctionEditor::dataChanged () {
	Function function = (Function) _data;
	Melder_assert (Thing_member (function, classFunction));
	_tmin = function -> xmin;
 	_tmax = function -> xmax;
 	if (_startWindow < _tmin || _startWindow > _tmax) _startWindow = _tmin;
 	if (_endWindow < _tmin || _endWindow > _tmax) _endWindow = _tmax;
 	if (_startWindow >= _endWindow) { _startWindow = _tmin; _endWindow = _tmax; }
 	if (_startSelection < _tmin) _startSelection = _tmin;
 	if (_startSelection > _tmax) _startSelection = _tmax;
 	if (_endSelection < _tmin) _endSelection = _tmin;
 	if (_endSelection > _tmax) _endSelection = _tmax;
	marksChanged ();
}

void FunctionEditor::draw () {}

void FunctionEditor::prepareDraw () {}

void FunctionEditor::play (double tmin, double tmax) {}

void FunctionEditor::drawWhileDragging (double x1, double x2) {
	/*
	 * We must draw this within the window, because the window tends to have a white background.
	 * We cannot draw this in the margins, because these tend to be grey, so that Graphics_xorOn does not work properly.
	 * We draw the text twice, because we expect that not ALL of the window is white...
	 */
	double xleft, xright;
	if (x1 > x2) xleft = x2, xright = x1; else xleft = x1, xright = x2;
	Graphics_xorOn (_graphics, Graphics_MAROON);
	Graphics_setTextAlignment (_graphics, Graphics_RIGHT, Graphics_TOP);
	Graphics_text1 (_graphics, xleft, 1.0, Melder_fixed (xleft, 6));
	Graphics_setTextAlignment (_graphics, Graphics_LEFT, Graphics_TOP);
	Graphics_text1 (_graphics, xright, 1.0, Melder_fixed (xright, 6));
	Graphics_setTextAlignment (_graphics, Graphics_RIGHT, Graphics_BOTTOM);
	Graphics_text1 (_graphics, xleft, 0.0, Melder_fixed (xleft, 6));
	Graphics_setTextAlignment (_graphics, Graphics_LEFT, Graphics_BOTTOM);
	Graphics_text1 (_graphics, xright, 0.0, Melder_fixed (xright, 6));
	Graphics_xorOff (_graphics);
}

int FunctionEditor::click (double xbegin, double ybegin, int shiftKeyPressed) {
	int drag = FALSE;
	double x = xbegin, y = ybegin, x1, x2;
	/*
	 * The 'anchor' is the point that will stay fixed during dragging.
	 * For instance, if she clicks and drags to the right,
	 * the location at which she originally clicked will be the anchor,
         * even if she later chooses to drag the mouse to the left of it.
	 * Another example: if she shift-clicks near E, B will become (and stay) the anchor.
	 */
	double anchorForDragging;
	Graphics_setWindow (_graphics, _startWindow, _endWindow, 0, 1);
	if (shiftKeyPressed) {
		/*
		 * Extend the selection.
		 * We should always end up with a real selection (B < E),
		 * even if we start with the reversed temporal order (E < B).
		 */
		int reversed = _startSelection > _endSelection;
		double firstMark = reversed ? _endSelection : _startSelection;
		double secondMark = reversed ? _startSelection : _endSelection;
		/*
		 * Undraw the old selection.
		 */
		if (_endSelection > _startSelection) {
			/*
			 * Determine the visible part of the old selection.
			 */
			double startVisible = _startSelection > _startWindow ? _startSelection : _startWindow;
			double endVisible = _endSelection < _endWindow ? _endSelection : _endWindow;
			/*
			 * Undraw the visible part of the old selection.
			 */
			if (endVisible > startVisible)
				unhighlightSelection (startVisible, endVisible, 0, 1);
		}
		if (xbegin >= secondMark) {
		 	/*
			 * She clicked right from the second mark (usually E). We move E.
			 */
			_endSelection = xbegin;
			anchorForDragging = _startSelection;
		} else if (xbegin <= firstMark) {
		 	/*
			 * She clicked left from the first mark (usually B). We move B.
			 */
			_startSelection = xbegin;
			anchorForDragging = _endSelection;
		} else {
			/*
			 * She clicked in between the two marks. We move the nearest mark.
			 */
			double distanceOfClickToFirstMark = fabs (xbegin - firstMark);
			double distanceOfClickToSecondMark = fabs (xbegin - secondMark);
			/*
			 * We make sure that the marks are in the unmarked B - E order.
			 */
			if (reversed) {
				/*
				 * Swap B and E.
				 */
				_startSelection = firstMark;
				_endSelection = secondMark;
			}
			/*
			 * Move the nearest mark.
			 */
			if (distanceOfClickToFirstMark < distanceOfClickToSecondMark) {
				_startSelection = xbegin;
				anchorForDragging = _endSelection;
			} else {
				_endSelection = xbegin;
				anchorForDragging = _startSelection;
			}
		}
		/*
		 * Draw the new selection.
		 */
		if (_endSelection > _startSelection) {
			/*
			 * Determine the visible part of the new selection.
			 */
			double startVisible = _startSelection > _startWindow ? _startSelection : _startWindow;
			double endVisible = _endSelection < _endWindow ? _endSelection : _endWindow;
			/*
			 * Draw the visible part of the new selection.
			 */
			if (endVisible > startVisible)
				highlightSelection (startVisible, endVisible, 0, 1);
		}
	}
	/*
	 * Find out whether this is a click or a drag.
	 */
	while (Graphics_mouseStillDown (_graphics)) {
		Graphics_getMouseLocation (_graphics, & x, & y);
		if (x < _startWindow) x = _startWindow;
		if (x > _endWindow) x = _endWindow;
		if (fabs (Graphics_dxWCtoMM (_graphics, x - xbegin)) > 1.5) {
			drag = TRUE;
			break;
		}
	}
	if (drag) {
		if (! shiftKeyPressed) {
			anchorForDragging = xbegin;
			/*
			 * We will drag and create a new selection.
			 */
		}
		/*
		 * First undraw the old selection.
		 */
		if (_endSelection > _startSelection) {
			/*
			 * Determine the visible part of the old selection.
			 */
			double startVisible = _startSelection > _startWindow ? _startSelection : _startWindow;
			double endVisible = _endSelection < _endWindow ? _endSelection : _endWindow;
			/*
			 * Undraw the visible part of the old selection.
			 */
			if (endVisible > startVisible)
				unhighlightSelection (startVisible, endVisible, 0, 1);
		}
		/*
		 * Draw the text at least once.
		 */
		/*if (x < _startWindow) x = _startWindow; else if (x > _endWindow) x = _endWindow;*/
		drawWhileDragging (anchorForDragging, x);
		/*
		 * Draw the dragged selection at least once.
		 */
		if (x > anchorForDragging) x1 = anchorForDragging, x2 = x; else x1 = x, x2 = anchorForDragging;
		highlightSelection (x1, x2, 0, 1);
		/*
		 * Drag for the new selection.
		 */
		while (Graphics_mouseStillDown (_graphics)) {
			double xold = x, x1, x2;
			Graphics_getMouseLocation (_graphics, & x, & y);
			/*
			 * Clip to the visible window. Ideally, we should perform autoscrolling instead, though...
			 */
			if (x < _startWindow) x = _startWindow; else if (x > _endWindow) x = _endWindow;
			if (x == xold)
				continue;
			/*
			 * Undraw and redraw the text at the top.
			 */
			drawWhileDragging (anchorForDragging, xold);
			/*
			 * Remove previous dragged selection.
			 */
			if (xold > anchorForDragging) x1 = anchorForDragging, x2 = xold; else x1 = xold, x2 = anchorForDragging;
			if (x1 != x2) unhighlightSelection (x1, x2, 0, 1);
			/*
			 * Draw new dragged selection.
			 */
			if (x > anchorForDragging) x1 = anchorForDragging, x2 = x; else x1 = x, x2 = anchorForDragging;
			if (x1 != x2) highlightSelection (x1, x2, 0, 1);
			/*
			 * Redraw the text at the top.
			 */
			drawWhileDragging (anchorForDragging, x);
		} ;
		/*
		 * Set the new selection.
		 */
		if (x > anchorForDragging) _startSelection = anchorForDragging, _endSelection = x;
		else _startSelection = x, _endSelection = anchorForDragging;
	} else if (! shiftKeyPressed) {
		/*
		 * Move the cursor to the clicked position.
		 */
		_startSelection = _endSelection = xbegin;
	}
	return FunctionEditor_UPDATE_NEEDED;
}

int FunctionEditor::clickB (double xWC, double yWC) {
	(void) yWC;
	_startSelection = xWC;
	if (_startSelection > _endSelection) {
		double dummy = _startSelection;
		_startSelection = _endSelection;
		_endSelection = dummy;
	}
	return 1;
}

int FunctionEditor::clickE (double xWC, double yWC) {
	_endSelection = xWC;
	(void) yWC;
	if (_startSelection > _endSelection) {
		double dummy = _startSelection;
		_startSelection = _endSelection;
		_endSelection = dummy;
	}
	return 1;
}

void FunctionEditor::key (unsigned char key) {}

void FunctionEditor::insetViewport () {
	Graphics_setViewport (_graphics, MARGIN, _width - MARGIN,
		BOTTOM_MARGIN + space * 3, _height - (TOP_MARGIN + space));
	Graphics_setWindow (_graphics, _startWindow, _endWindow, 0, 1);
}

int FunctionEditor::playCallback (Any void_me, int phase, double tmin, double tmax, double t) {
	FunctionEditor *editor = (FunctionEditor *)void_me;
	/*
	 * This callback will often be called by the Melder workproc during playback.
	 * However, it will sometimes be called by Melder_stopPlaying with phase=3.
	 * This will occur at unpredictable times, perhaps when the LongSound is updated.
	 * So we had better make no assumptions about the current viewport.
	 */
	double x1NDC, x2NDC, y1NDC, y2NDC;
	(void) tmin;
	Graphics_inqViewport (editor->_graphics, & x1NDC, & x2NDC, & y1NDC, & y2NDC);
	editor->insetViewport ();
	Graphics_xorOn (editor->_graphics, Graphics_MAROON);
	/*
	 * Undraw the play cursor at its old location.
	 * BUG: during scrolling, zooming, and exposure, an ugly line may remain.
	 */
	if (phase != 1 && editor->_playCursor >= editor->_startWindow && editor->_playCursor <= editor->_endWindow) {
		Graphics_setLineWidth (editor->_graphics, 3.0);
		Graphics_line (editor->_graphics, editor->_playCursor, 0, editor->_playCursor, 1);
		Graphics_setLineWidth (editor->_graphics, 1.0);
	}
	/*
	 * Draw the play cursor at its new location.
	 */
	if (phase != 3 && t >= editor->_startWindow && t <= editor->_endWindow) {
		Graphics_setLineWidth (editor->_graphics, 3.0);
		Graphics_line (editor->_graphics, t, 0, t, 1);
		Graphics_setLineWidth (editor->_graphics, 1.0);
	}
	Graphics_xorOff (editor->_graphics);
	/*
	 * Usually, there will be an event test after each invocation of this callback,
	 * because the asynchronicity is kMelder_asynchronicityLevel_INTERRUPTABLE or kMelder_asynchronicityLevel_ASYNCHRONOUS.
	 * However, if the asynchronicity is just kMelder_asynchronicityLevel_CALLING_BACK,
	 * there is no event test. Which means: no server round trip.
	 * Which means: no automatic flushing of graphics output.
	 * So: we force the flushing ourselves, lest we see too few moving cursors.
	 */
	Graphics_flushWs (editor->_graphics);
	Graphics_setViewport (editor->_graphics, x1NDC, x2NDC, y1NDC, y2NDC);
	editor->_playCursor = t;
	if (phase == 3) {
		if (t < tmax && MelderAudio_stopWasExplicit ()) {
			if (t > editor->_startSelection && t < editor->_endSelection)
				editor->_startSelection = t;
			else
				editor->_startSelection = editor->_endSelection = t;
			editor->updateText ();
			/*Graphics_updateWs (_graphics);*/
			editor->drawNow ();
			editor->updateGroup ();
		}
		editor->_playingCursor = FALSE;
		editor->_playingSelection = FALSE;
	}
	return 1;
}

void FunctionEditor::prefs_addFields (EditorCommand *cmd) {}
void FunctionEditor::prefs_setValues (EditorCommand *cmd) {}
void FunctionEditor::prefs_getValues (EditorCommand *cmd) {}

void FunctionEditor::highlightSelection (double left, double right, double bottom, double top) {
	Graphics_highlight (_graphics, left, right, bottom, top);
}

void FunctionEditor::unhighlightSelection (double left, double right, double bottom, double top) {
	Graphics_unhighlight (_graphics, left, right, bottom, top);
}

double FunctionEditor::getBottomOfSoundAndAnalysisArea () {
	return 0.0;
}

void FunctionEditor::marksChanged () {
	updateText ();
	updateScrollBar ();
	/*Graphics_updateWs (_graphics);*/ drawNow ();
	updateGroup ();
}

void FunctionEditor::updateText () {}

void FunctionEditor::redraw () {
	/*Graphics_updateWs (_graphics);*/ drawNow ();
}

void FunctionEditor::enableUpdates (bool enable) {
	_enableUpdates = enable;
}

void FunctionEditor::ungroup () {
	if (! _group) return;
	_group = false;
	GuiCheckButton_setValue (_groupButton, false);
	int i = 1;
	while (group [i] != this) i ++;
	group [i] = NULL;
	nGroup --;
	updateText ();
	Graphics_updateWs (_graphics);   /* For setting buttons in draw method. */
}

void FunctionEditor::drawRangeMark (double yWC, const wchar_t *yWC_string, const wchar_t *units, int verticalAlignment) {
	static MelderString text = { 0 };
	MelderString_empty (& text);
	MelderString_append2 (& text, yWC_string, units);
	double textWidth = Graphics_textWidth (_graphics, text.string) + Graphics_dxMMtoWC (_graphics, 0.5);
	Graphics_setColour (_graphics, Graphics_BLUE);
	Graphics_line (_graphics, _endWindow, yWC, _endWindow + textWidth, yWC);
	Graphics_setTextAlignment (_graphics, Graphics_LEFT, verticalAlignment);
	if (verticalAlignment == Graphics_BOTTOM) yWC -= Graphics_dyMMtoWC (_graphics, 0.5);
	Graphics_text (_graphics, _endWindow, yWC, text.string);
}

void FunctionEditor::drawCursorFunctionValue (double yWC, const wchar_t *yWC_string, const wchar_t *units) {
	Graphics_setColour (_graphics, Graphics_CYAN);
	Graphics_line (_graphics, _startWindow, yWC, 0.99 * _startWindow + 0.01 * _endWindow, yWC);
	Graphics_fillCircle_mm (_graphics, 0.5 * (_startSelection + _endSelection), yWC, 1.5);
	Graphics_setColour (_graphics, Graphics_BLUE);
	Graphics_setTextAlignment (_graphics, Graphics_RIGHT, Graphics_HALF);
	Graphics_text2 (_graphics, _startWindow, yWC, yWC_string, units);
}

void FunctionEditor::insertCursorFunctionValue (double yWC, const wchar_t *yWC_string, const wchar_t *units, double minimum, double maximum) {
	static MelderString text = { 0 };
	MelderString_empty (& text);
	MelderString_append2 (& text, yWC_string, units);
	double textX = _endWindow, textY = yWC, textWidth;
	int tooHigh = Graphics_dyWCtoMM (_graphics, maximum - textY) < 5.0;
	int tooLow = Graphics_dyWCtoMM (_graphics, textY - minimum) < 5.0;
	if (yWC < minimum || yWC > maximum) return;
	Graphics_setColour (_graphics, Graphics_CYAN);
	Graphics_line (_graphics, 0.99 * _endWindow + 0.01 * _startWindow, yWC, _endWindow, yWC);
	Graphics_fillCircle_mm (_graphics, 0.5 * (_startSelection + _endSelection), yWC, 1.5);
	if (tooHigh) {
		if (tooLow) textY = 0.5 * (minimum + maximum);
		else textY = maximum - Graphics_dyMMtoWC (_graphics, 5.0);
	} else if (tooLow) {
		textY = minimum + Graphics_dyMMtoWC (_graphics, 5.0);
	}
	textWidth = Graphics_textWidth (_graphics, text.string);
	Graphics_fillCircle_mm (_graphics, _endWindow + textWidth + Graphics_dxMMtoWC (_graphics, 1.5), textY, 1.5);
	Graphics_setColour (_graphics, Graphics_RED);
	Graphics_setTextAlignment (_graphics, Graphics_LEFT, Graphics_HALF);
	Graphics_text (_graphics, textX, textY, text.string);
}

void FunctionEditor::drawHorizontalHair (double yWC, const wchar_t *yWC_string, const wchar_t *units) {
	Graphics_setColour (_graphics, Graphics_RED);
	Graphics_line (_graphics, _startWindow, yWC, _endWindow, yWC);
	Graphics_setTextAlignment (_graphics, Graphics_RIGHT, Graphics_HALF);
	Graphics_text2 (_graphics, _startWindow, yWC, yWC_string, units);
}

void FunctionEditor::drawGridLine (double yWC) {
	Graphics_setColour (_graphics, Graphics_CYAN);
	Graphics_setLineType (_graphics, Graphics_DOTTED);
	Graphics_line (_graphics, _startWindow, yWC, _endWindow, yWC);
	Graphics_setLineType (_graphics, Graphics_DRAWN);
}

void FunctionEditor::garnish () {
	if (preferences.picture.drawSelectionTimes) {
		if (_startSelection >= _startWindow && _startSelection <= _endWindow)
			Graphics_markTop (_pictureGraphics, _startSelection, true, true, false, NULL);
		if (_endSelection != _startSelection && _endSelection >= _startWindow && _endSelection <= _endWindow)
			Graphics_markTop (_pictureGraphics, _endSelection, true, true, false, NULL);
	}
	if (preferences.picture.drawSelectionHairs) {
		if (_startSelection >= _startWindow && _startSelection <= _endWindow)
			Graphics_markTop (_pictureGraphics, _startSelection, false, false, true, NULL);
		if (_endSelection != _startSelection && _endSelection >= _startWindow && _endSelection <= _endWindow)
			Graphics_markTop (_pictureGraphics, _endSelection, false, false, true, NULL);
	}
}

/* End of file FunctionEditor.c */
