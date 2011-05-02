/* RealTierEditor.cpp
 *
 * Copyright (C) 1992-2011 Paul Boersma
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
 * pb 2002/10/06 improved visilibity of dragging
 * pb 2004/04/13 less flashing
 * pb 2006/12/08 keyboard shortcuts
 * pb 2007/06/10 wchar_t
 * pb 2007/09/04 new FunctionEditor API
 * pb 2007/09/08 inherit from TimeSoundEditor
 * pb 2007/11/30 erased Graphics_printf
 * pb 2008/03/21 new Editor API
 * pb 2009/01/23 minimum and maximum legal values
 * pb 2011/03/23 C++
 */

#include "RealTierEditor.h"

#include "sys/Preferences.h"

#define SOUND_HEIGHT  0.382

RealTierEditor::RealTierEditor (GuiObject parent, const wchar_t *title, RealTier data, Sound sound, int ownSound)
	: TimeSoundEditor (parent, title, data, sound, ownSound),
	  _ymin(-1.0) {
	createMenus ();
	Melder_assert (data != NULL);
	Melder_assert (Thing_member (data, classRealTier));
	updateScaling ();
	_ycursor = 0.382 * _ymin + 0.618 * _ymax;
}

/********** MENU COMMANDS **********/

int RealTierEditor::menu_cb_removePoints (EDITOR_ARGS) {
	RealTierEditor *editor = (RealTierEditor *)editor_me;
	editor->save (L"Remove point(s)");
	if (editor->_startSelection == editor->_endSelection)
		AnyTier_removePointNear (editor->_data, editor->_startSelection);
	else
		AnyTier_removePointsBetween (editor->_data, editor->_startSelection, editor->_endSelection);
	editor->updateScaling ();
	editor->redraw ();
	editor->broadcastChange ();
	return 1;
}

int RealTierEditor::menu_cb_addPointAtCursor (EDITOR_ARGS) {
	RealTierEditor *editor = (RealTierEditor *)editor_me;
	if (NUMdefined (editor->minimumLegalValue ()) && editor->_ycursor < editor->minimumLegalValue ())
		return Melder_error4 (L"Cannot add a point below ", Melder_double (editor->minimumLegalValue ()), editor->rightTickUnits (), L".");
	if (NUMdefined (editor->maximumLegalValue ()) && editor->_ycursor > editor->maximumLegalValue ())
		return Melder_error4 (L"Cannot add a point above ", Melder_double (editor->maximumLegalValue ()), editor->rightTickUnits (), L".");
	editor->save (L"Add point");
	RealTier_addPoint (editor->_data, 0.5 * (editor->_startSelection + editor->_endSelection), editor->_ycursor);
	editor->updateScaling ();
	editor->redraw ();
	editor->broadcastChange ();
	return 1;
}

int RealTierEditor::menu_cb_addPointAt (EDITOR_ARGS) {
	RealTierEditor *editor = (RealTierEditor *)editor_me;
	EDITOR_FORM (L"Add point", 0)
		REAL (L"Time (s)", L"0.0")
		REAL (editor->quantityText (), L"0.0")
	EDITOR_OK
		SET_REAL (L"Time", 0.5 * (editor->_startSelection + editor->_endSelection))
		SET_REAL (editor->quantityKey (), editor->_ycursor)
	EDITOR_DO
		double desiredValue = GET_REAL (editor->quantityKey ());
		if (NUMdefined (editor->minimumLegalValue ()) && desiredValue < editor->minimumLegalValue ())
			return Melder_error4 (L"Cannot add a point below ", Melder_double (editor->minimumLegalValue ()), editor->rightTickUnits (), L".");
		if (NUMdefined (editor->maximumLegalValue ()) && desiredValue > editor->maximumLegalValue ())
			return Melder_error4 (L"Cannot add a point above ", Melder_double (editor->maximumLegalValue ()), editor->rightTickUnits (), L".");
		editor->save (L"Add point");
		RealTier_addPoint (editor->_data, GET_REAL (L"Time"), desiredValue);
		editor->updateScaling ();
		editor->redraw ();
		editor->broadcastChange ();
	EDITOR_END
}

int RealTierEditor::menu_cb_setRange (EDITOR_ARGS) {
	RealTierEditor *editor = (RealTierEditor *)editor_me;
	EDITOR_FORM (editor->setRangeTitle (), 0)
		REAL (editor->yminText (), editor->defaultYminText ())
		REAL (editor->ymaxText (), editor->defaultYmaxText ())
	EDITOR_OK
		SET_REAL (editor->yminKey (), editor->_ymin)
		SET_REAL (editor->ymaxKey (), editor->_ymax)
	EDITOR_DO
		editor->_ymin = GET_REAL (editor->yminKey ());
		editor->_ymax = GET_REAL (editor->ymaxKey ());
		if (editor->_ymax <= editor->_ymin) editor->updateScaling ();
		editor->redraw ();
	EDITOR_END
}

void RealTierEditor::createMenus () {
	EditorMenu *menu = addMenu (L"Point", 0);
	menu->addCommand (L"Add point at cursor", 'T', menu_cb_addPointAtCursor);
	menu->addCommand (L"Add point at...", 0, menu_cb_addPointAt);
	menu->addCommand (L"-- remove point --", 0, NULL);
	menu->addCommand (L"Remove point(s)", GuiMenu_OPTION + 'T', menu_cb_removePoints);

	menu = getMenu (L"View");
	menu->addCommand (L"-- view/realtier --", 0, 0);
	menu->addCommand (setRangeTitle (), 0, menu_cb_setRange);
}

void RealTierEditor::updateScaling () {
	RealTier data = (RealTier) _data;
	if (data -> points -> size == 0) {
		_ymin = defaultYmin ();
		_ymax = defaultYmax ();
	} else {
		double ymin = RealTier_getMinimumValue (_data);
		double ymax = RealTier_getMaximumValue (_data);
		double range = ymax - ymin;
		if (range == 0.0) ymin -= 1.0, ymax += 1.0;
		else ymin -= 0.2 * range, ymax += 0.2 * range;
		if (NUMdefined (minimumLegalValue ()) && ymin < minimumLegalValue ())
			ymin = minimumLegalValue ();
		if (NUMdefined (maximumLegalValue ()) && ymin > maximumLegalValue ())
			ymin = maximumLegalValue ();
		if (NUMdefined (minimumLegalValue ()) && ymax < minimumLegalValue ())
			ymax = minimumLegalValue ();
		if (NUMdefined (maximumLegalValue ()) && ymax > maximumLegalValue ())
			ymax = maximumLegalValue ();
		if (ymin >= ymax) {
			if (NUMdefined (minimumLegalValue ()) && NUMdefined (maximumLegalValue ())) {
				ymin = minimumLegalValue ();
				ymax = maximumLegalValue ();
			} else if (NUMdefined (minimumLegalValue ())) {
				ymin = minimumLegalValue ();
				ymax = ymin + 1.0;
			} else {
				Melder_assert (NUMdefined (maximumLegalValue ()));
				ymax = maximumLegalValue ();
				ymin = ymax - 1.0;
			}
		}
		if (ymin < _ymin || _ymin < 0.0) _ymin = ymin;
		if (ymax > _ymax) _ymax = ymax;
		if (_ycursor <= _ymin || _ycursor >= _ymax)
			_ycursor = 0.382 * _ymin + 0.618 * _ymax;
	}
}

void RealTierEditor::dataChanged () {
	updateScaling ();
	TimeSoundEditor::dataChanged ();
}

/********** DRAWING AREA **********/

void RealTierEditor::draw () {
	RealTier data = (RealTier) _data;
	long ifirstSelected, ilastSelected, n = data -> points -> size, imin, imax, i;
	Graphics_Viewport viewport;
	if (_sound.data) {
		viewport = Graphics_insetViewport (_graphics, 0, 1, 1 - SOUND_HEIGHT, 1.0);
		Graphics_setColour (_graphics, Graphics_WHITE);
		Graphics_setWindow (_graphics, 0, 1, 0, 1);
		Graphics_fillRectangle (_graphics, 0, 1, 0, 1);
		draw_sound (-1.0, 1.0);
		Graphics_resetViewport (_graphics, viewport);
		Graphics_insetViewport (_graphics, 0, 1, 0.0, 1 - SOUND_HEIGHT);
	}
	Graphics_setColour (_graphics, Graphics_WHITE);
	Graphics_setWindow (_graphics, 0, 1, 0, 1);
	Graphics_fillRectangle (_graphics, 0, 1, 0, 1);
	Graphics_setWindow (_graphics, _startWindow, _endWindow, _ymin, _ymax);
	Graphics_setColour (_graphics, Graphics_RED);
	Graphics_line (_graphics, _startWindow, _ycursor, _endWindow, _ycursor);
	Graphics_setTextAlignment (_graphics, Graphics_RIGHT, Graphics_HALF);
	Graphics_text1 (_graphics, _startWindow, _ycursor, Melder_float (Melder_half (_ycursor)));
	Graphics_setColour (_graphics, Graphics_BLUE);
	Graphics_setTextAlignment (_graphics, Graphics_LEFT, Graphics_TOP);
	Graphics_text2 (_graphics, _endWindow, _ymax, Melder_float (Melder_half (_ymax)), rightTickUnits ());
	Graphics_setTextAlignment (_graphics, Graphics_LEFT, Graphics_HALF);
	Graphics_text2 (_graphics, _endWindow, _ymin, Melder_float (Melder_half (_ymin)), rightTickUnits ());
	ifirstSelected = AnyTier_timeToHighIndex (data, _startSelection);
	ilastSelected = AnyTier_timeToLowIndex (data, _endSelection);
	imin = AnyTier_timeToHighIndex (data, _startWindow);
	imax = AnyTier_timeToLowIndex (data, _endWindow);
	Graphics_setLineWidth (_graphics, 2);
	if (n == 0) {
		Graphics_setTextAlignment (_graphics, Graphics_CENTRE, Graphics_HALF);
		Graphics_text (_graphics, 0.5 * (_startWindow + _endWindow),
			0.5 * (_ymin + _ymax), L"(no points)");
	} else if (imax < imin) {
		double yleft = RealTier_getValueAtTime (data, _startWindow);
		double yright = RealTier_getValueAtTime (data, _endWindow);
		Graphics_line (_graphics, _startWindow, yleft, _endWindow, yright);
	} else for (i = imin; i <= imax; i ++) {
		RealPoint point = (RealPoint) data -> points -> item [i];
		double t = point -> time, y = point -> value;
		if (i >= ifirstSelected && i <= ilastSelected)
			Graphics_setColour (_graphics, Graphics_RED);	
		Graphics_fillCircle_mm (_graphics, t, y, 3);
		Graphics_setColour (_graphics, Graphics_BLUE);
		if (i == 1)
			Graphics_line (_graphics, _startWindow, y, t, y);
		else if (i == imin)
			Graphics_line (_graphics, t, y, _startWindow, RealTier_getValueAtTime (data, _startWindow));
		if (i == n)
			Graphics_line (_graphics, t, y, _endWindow, y);
		else if (i == imax)
			Graphics_line (_graphics, t, y, _endWindow, RealTier_getValueAtTime (data, _endWindow));
		else {
			RealPoint pointRight = (RealPoint) data -> points -> item [i + 1];
			Graphics_line (_graphics, t, y, pointRight -> time, pointRight -> value);
		}
	}
	Graphics_setLineWidth (_graphics, 1);
	Graphics_setColour (_graphics, Graphics_BLACK);
	updateMenuItems_file ();
}

void RealTierEditor::drawWhileDragging (double xWC, double yWC, long first, long last, double dt, double dy) {
	RealTier data = (RealTier) _data;
	(void) xWC;
	(void) yWC;

	/*
	 * Draw all selected points as magenta empty circles, if inside the window.
	 */
	for (long i = first; i <= last; i ++) {
		RealPoint point = (RealPoint) data -> points -> item [i];
		double t = point -> time + dt, y = point -> value + dy;
		if (t >= _startWindow && t <= _endWindow)
			Graphics_circle_mm (_graphics, t, y, 3);
	}

	if (last == first) {
		/*
		 * Draw a crosshair with time and y.
		 */
		RealPoint point = (RealPoint) data -> points -> item [first];
		double t = point -> time + dt, y = point -> value + dy;
		Graphics_line (_graphics, t, _ymin, t, _ymax - Graphics_dyMMtoWC (_graphics, 4.0));
		Graphics_setTextAlignment (_graphics, kGraphics_horizontalAlignment_CENTRE, Graphics_TOP);
		Graphics_text1 (_graphics, t, _ymax, Melder_fixed (t, 6));
		Graphics_line (_graphics, _startWindow, y, _endWindow, y);
		Graphics_setTextAlignment (_graphics, Graphics_LEFT, Graphics_BOTTOM);
		Graphics_text1 (_graphics, _startWindow, y, Melder_fixed (y, 6));
	}
}

int RealTierEditor::click (double xWC, double yWC, int shiftKeyPressed) {
	RealTier pitch = (RealTier) _data;
	long inearestPoint, ifirstSelected, ilastSelected, i;
	RealPoint nearestPoint;
	double dt = 0, df = 0;
	int draggingSelection;
	Graphics_Viewport viewport;

	/*
	 * Perform the default action: move cursor.
	 */
	//_startSelection = _endSelection = xWC;
	if (_sound.data) {
		if (yWC < 1 - SOUND_HEIGHT) {   /* Clicked in tier area? */
			yWC /= 1 - SOUND_HEIGHT;
			_ycursor = (1.0 - yWC) * _ymin + yWC * _ymax;
			viewport = Graphics_insetViewport (_graphics, 0, 1, 0, 1 - SOUND_HEIGHT);
		} else {
			return TimeSoundEditor::click (xWC, yWC, shiftKeyPressed);
		}
	} else {
		_ycursor = (1.0 - yWC) * _ymin + yWC * _ymax;
	}
	Graphics_setWindow (_graphics, _startWindow, _endWindow, _ymin, _ymax);
	yWC = _ycursor;

	/*
	 * Clicked on a point?
	 */
	inearestPoint = AnyTier_timeToNearestIndex (pitch, xWC);
	if (inearestPoint == 0) return TimeSoundEditor::click (xWC, yWC, shiftKeyPressed);
	nearestPoint = (RealPoint) pitch -> points -> item [inearestPoint];
	if (Graphics_distanceWCtoMM (_graphics, xWC, yWC, nearestPoint -> time, nearestPoint -> value) > 1.5) {
		if (_sound.data) Graphics_resetViewport (_graphics, viewport);
		return TimeSoundEditor::click (xWC, yWC, shiftKeyPressed);
	}

	/*
	 * Clicked on a selected point?
	 */
	draggingSelection = shiftKeyPressed &&
		nearestPoint -> time > _startSelection && nearestPoint -> time < _endSelection;
	if (draggingSelection) {
		ifirstSelected = AnyTier_timeToHighIndex (pitch, _startSelection);
		ilastSelected = AnyTier_timeToLowIndex (pitch, _endSelection);
		save (L"Drag points");
	} else {
		ifirstSelected = ilastSelected = inearestPoint;
		save (L"Drag point");
	}

	/*
	 * Drag.
	 */
	Graphics_xorOn (_graphics, Graphics_MAROON);
	drawWhileDragging (xWC, yWC, ifirstSelected, ilastSelected, dt, df);   // draw at old position
	while (Graphics_mouseStillDown (_graphics)) {
		double xWC_new, yWC_new;
		Graphics_getMouseLocation (_graphics, & xWC_new, & yWC_new);
		if (xWC_new != xWC || yWC_new != yWC) {
			drawWhileDragging (xWC, yWC, ifirstSelected, ilastSelected, dt, df);   // undraw at old position
			dt += xWC_new - xWC, df += yWC_new - yWC;
			xWC = xWC_new, yWC = yWC_new;
			drawWhileDragging (xWC, yWC, ifirstSelected, ilastSelected, dt, df);   // draw at new position
		}
	}
	Graphics_xorOff (_graphics);

	/*
	 * Dragged inside window?
	 */
	if (xWC < _startWindow || xWC > _endWindow) return 1;

	/*
	 * Points not dragged past neighbours?
	 */
	{
		RealPoint *points = (RealPoint *) pitch -> points -> item;
		double newTime = points [ifirstSelected] -> time + dt;
		if (newTime < _tmin) return 1;   /* Outside domain. */
		if (ifirstSelected > 1 && newTime <= points [ifirstSelected - 1] -> time)
			return 1;   /* Past left neighbour. */
		newTime = points [ilastSelected] -> time + dt;
		if (newTime > _tmax) return 1;   /* Outside domain. */
		if (ilastSelected < pitch -> points -> size && newTime >= points [ilastSelected + 1] -> time)
			return 1;   /* Past right neighbour. */
	}

	/*
	 * Drop.
	 */
	for (i = ifirstSelected; i <= ilastSelected; i ++) {
		RealPoint point = (RealPoint) pitch -> points -> item [i];
		point -> time += dt;
		point -> value += df;
		if (NUMdefined (minimumLegalValue ()) && point -> value < minimumLegalValue ())
			point -> value = minimumLegalValue ();
		if (NUMdefined (maximumLegalValue ()) && point -> value > maximumLegalValue ())
			point -> value = maximumLegalValue ();
	}

	/*
	 * Make sure that the same points are still selected (a problem with Undo...).
	 */

	if (draggingSelection) _startSelection += dt, _endSelection += dt;
	if (ifirstSelected == ilastSelected) {
		/*
		 * Move crosshair to only selected pitch point.
		 */
		RealPoint point = (RealPoint) pitch -> points -> item [ifirstSelected];
		_startSelection = _endSelection = point -> time;
		_ycursor = point -> value;
	} else {
		/*
		 * Move crosshair to mouse location.
		 */
		/*_cursor += dt;*/
		_ycursor += df;
		if (NUMdefined (minimumLegalValue ()) && _ycursor < minimumLegalValue ())
			_ycursor = minimumLegalValue ();
		if (NUMdefined (maximumLegalValue ()) && _ycursor > maximumLegalValue ())
			_ycursor = maximumLegalValue ();
	}

	broadcastChange ();
	updateScaling ();
	return 1;   /* Update needed. */
}

void RealTierEditor::play (double tmin, double tmax) {
	if (_sound.data)
		Sound_playPart (_sound.data, tmin, tmax, playCallback, this);
}

/* End of file RealTierEditor.cpp */
