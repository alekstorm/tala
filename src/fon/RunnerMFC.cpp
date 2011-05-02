/* RunnerMFC.cpp
 *
 * Copyright (C) 2001-2011 Paul Boersma
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
 * pb 2001/07/18
 * pb 2002/07/08 goodness
 * pb 2002/07/16 GPL
 * pb 2005/11/21 play again
 * pb 2005/12/02 response sounds are played
 * pb 2005/12/04 oops button
 * pb 2005/12/08 multiple experiments
 * pb 2006/01/19 fixed a bug that caused an assertion violation when the oops button was pressed after the experiment finished
 * pb 2006/02/23 repaired small memory leak in destroy()
 * pb 2007/08/12 wchar_t
 * pb 2008/03/31 correct error message when the second of multiple experiments cannot start
 * pb 2008/04/08 disable the OK key if no response has been given yet
 * pb 2011/03/23 C++
 */

#include "RunnerMFC.h"
#include "sys/EditorM.h"
#include "sys/machine.h"

void RunnerMFC::gui_drawingarea_cb_resize (I, GuiDrawingAreaResizeEvent event) {
	RunnerMFC *editor = (RunnerMFC *)void_me;
	if (editor->_graphics == NULL) return;
	Graphics_setWsViewport (editor->_graphics, 0, event -> width, 0, event -> height);
	Graphics_setWsWindow (editor->_graphics, 0, event -> width, 0, event -> height);
	Graphics_setViewport (editor->_graphics, 0, event -> width, 0, event -> height);
	Graphics_updateWs (editor->_graphics);
}

RunnerMFC::RunnerMFC (GuiObject parent, const wchar_t *title, Ordered experiments)
	: Editor (parent, 0, 0, 2000, 2000, title, NULL),
	  _experiments(experiments),
	  _iexperiment(1),
	  _numberOfReplays(0) {
	createChildren ();
	//try { // FIXME exception
		_graphics = Graphics_create_xmdrawingarea (_drawingArea);
		#if gtk
			gtk_widget_set_double_buffered (_drawingArea, FALSE);
		#endif

struct structGuiDrawingAreaResizeEvent event = { _drawingArea, 0 };
event. width = GuiObject_getWidth (_drawingArea);
event. height = GuiObject_getHeight (_drawingArea);
gui_drawingarea_cb_resize (this, & event);

		_iexperiment = 1;
		startExperiment (); therror
	/*} catch (...) {
		rethrowmzero ("Experiment window not created.");
	}*/
}

RunnerMFC::~RunnerMFC () {
	if (_experiments) {
		_experiments -> size = 0;   /* Give ownership back to whoever thinks they own the experiments. */
		forget (_experiments);
	}
	forget (_graphics);
}

void RunnerMFC::dataChanged () {
	Graphics_updateWs (_graphics);
}

int RunnerMFC::startExperiment () {
	_data = _experiments -> item [_iexperiment];
	if (! ExperimentMFC_start ((ExperimentMFC) _data)) return Melder_error1 (L"Cannot start experiment.");
	_name = Melder_wcsdup_f (((ExperimentMFC) _data) -> name);
	broadcastChange ();
	Graphics_updateWs (_graphics);
	return 1;
}

void RunnerMFC::drawControlButton (double left, double right, double bottom, double top, const wchar_t *visibleText) {
	Graphics_setColour (_graphics, Graphics_MAROON);
	Graphics_setLineWidth (_graphics, 3.0);
	Graphics_fillRectangle (_graphics, left, right, bottom, top);
	Graphics_setColour (_graphics, Graphics_YELLOW);
	Graphics_rectangle (_graphics, left, right, bottom, top);
	Graphics_text (_graphics, 0.5 * (left + right), 0.5 * (bottom + top), visibleText);
}

void RunnerMFC::gui_drawingarea_cb_expose (I, GuiDrawingAreaExposeEvent event) {
	RunnerMFC *editor = (RunnerMFC *)void_me;
	Melder_assert (event -> widget == editor->_drawingArea);
	if (editor->_graphics == NULL) return;   // Could be the case in the very beginning.
	ExperimentMFC experiment = (ExperimentMFC) editor->_data;
	long iresponse;
	if (editor->_data == NULL) return;
	Graphics_setGrey (editor->_graphics, 0.8);
	Graphics_fillRectangle (editor->_graphics, 0, 1, 0, 1);
	Graphics_setGrey (editor->_graphics, 0.0);
	if (experiment -> trial == 0) {
		Graphics_setTextAlignment (editor->_graphics, Graphics_CENTRE, Graphics_HALF);
		Graphics_setFontSize (editor->_graphics, 24);
		Graphics_text (editor->_graphics, 0.5, 0.5, experiment -> startText);
	} else if (experiment -> pausing) {
		Graphics_setTextAlignment (editor->_graphics, Graphics_CENTRE, Graphics_HALF);
		Graphics_setFontSize (editor->_graphics, 24);
		Graphics_text (editor->_graphics, 0.5, 0.5, experiment -> pauseText);
		if (experiment -> oops_right > experiment -> oops_left && experiment -> trial > 1) {
			editor->drawControlButton (
				experiment -> oops_left, experiment -> oops_right, experiment -> oops_bottom, experiment -> oops_top,
				experiment -> oops_label);
		}
	} else if (experiment -> trial <= experiment -> numberOfTrials) {
		const wchar_t *visibleText = experiment -> stimulus [experiment -> stimuli [experiment -> trial]]. visibleText;
		wchar_t *visibleText_dup = Melder_wcsdup_f (visibleText ? visibleText : L""), *visibleText_p = visibleText_dup;
		Graphics_setFont (editor->_graphics, kGraphics_font_TIMES);
		Graphics_setFontSize (editor->_graphics, 10);
		Graphics_setColour (editor->_graphics, Graphics_BLACK);
		Graphics_setTextAlignment (editor->_graphics, Graphics_LEFT, Graphics_TOP);
		Graphics_text3 (editor->_graphics, 0, 1, Melder_integer (experiment -> trial), L" / ", Melder_integer (experiment -> numberOfTrials));
		Graphics_setTextAlignment (editor->_graphics, Graphics_CENTRE, Graphics_TOP);
		Graphics_setFontSize (editor->_graphics, 24);
		/*
		 * The run text.
		 */
		if (visibleText_p [0] != '\0') {
			wchar_t *visibleText_q = wcschr (visibleText_p, '|');
			if (visibleText_q) *visibleText_q = '\0';
			Graphics_text (editor->_graphics, 0.5, 1.0, visibleText_p [0] != '\0' ? visibleText_p : experiment -> runText);
			if (visibleText_q) visibleText_p = visibleText_q + 1; else visibleText_p += wcslen (visibleText_p);
		} else {
			Graphics_text (editor->_graphics, 0.5, 1.0, experiment -> runText);
		}
		Graphics_setTextAlignment (editor->_graphics, Graphics_CENTRE, Graphics_HALF);
		for (iresponse = 1; iresponse <= experiment -> numberOfDifferentResponses; iresponse ++) {
			ResponseMFC response = & experiment -> response [iresponse];
			wchar_t *textToDraw = response -> label;   // can be overridden
			if (visibleText_p [0] != '\0') {
				wchar_t *visibleText_q = wcschr (visibleText_p, '|');
				if (visibleText_q) *visibleText_q = '\0';
				textToDraw = visibleText_p;   // override
				if (visibleText_q) visibleText_p = visibleText_q + 1; else visibleText_p += wcslen (visibleText_p);
			}
			if (wcsnequ (textToDraw, L"\\FI", 3)) {
				structMelderFile file;
				MelderDir_relativePathToFile (& experiment -> rootDirectory, textToDraw + 3, & file);
				Graphics_imageFromFile (editor->_graphics, Melder_fileToPath (& file), response -> left, response -> right, response -> bottom, response -> top);
			} else {
				Graphics_setColour (editor->_graphics,
					response -> name [0] == '\0' ? Graphics_SILVER :
					experiment -> responses [experiment -> trial] == iresponse ? Graphics_RED :
					experiment -> ok_right > experiment -> ok_left || experiment -> responses [experiment -> trial] == 0 ?
					Graphics_YELLOW : Graphics_SILVER);
				Graphics_setLineWidth (editor->_graphics, 3.0);
				Graphics_fillRectangle (editor->_graphics, response -> left, response -> right, response -> bottom, response -> top);
				Graphics_setColour (editor->_graphics, Graphics_MAROON);
				Graphics_rectangle (editor->_graphics, response -> left, response -> right, response -> bottom, response -> top);
				Graphics_setFontSize (editor->_graphics, response -> fontSize ? response -> fontSize : 24);
				Graphics_text (editor->_graphics, 0.5 * (response -> left + response -> right),
					0.5 * (response -> bottom + response -> top), textToDraw);
			}
			Graphics_setFontSize (editor->_graphics, 24);
		}
		for (iresponse = 1; iresponse <= experiment -> numberOfGoodnessCategories; iresponse ++) {
			GoodnessMFC goodness = & experiment -> goodness [iresponse];
			Graphics_setColour (editor->_graphics, experiment -> responses [experiment -> trial] == 0 ? Graphics_SILVER :
				experiment -> goodnesses [experiment -> trial] == iresponse ? Graphics_RED : Graphics_YELLOW);
			Graphics_setLineWidth (editor->_graphics, 3.0);
			Graphics_fillRectangle (editor->_graphics, goodness -> left, goodness -> right, goodness -> bottom, goodness -> top);
			Graphics_setColour (editor->_graphics, Graphics_MAROON);
			Graphics_rectangle (editor->_graphics, goodness -> left, goodness -> right, goodness -> bottom, goodness -> top);
			Graphics_text (editor->_graphics, 0.5 * (goodness -> left + goodness -> right), 0.5 * (goodness -> bottom + goodness -> top), goodness -> label);
		}
		if (experiment -> replay_right > experiment -> replay_left && editor->_numberOfReplays < experiment -> maximumNumberOfReplays) {
			editor->drawControlButton (
				experiment -> replay_left, experiment -> replay_right, experiment -> replay_bottom, experiment -> replay_top,
				experiment -> replay_label);
		}
		if (experiment -> ok_right > experiment -> ok_left &&
		    experiment -> responses [experiment -> trial] != 0 &&
		    (experiment -> numberOfGoodnessCategories == 0 || experiment -> goodnesses [experiment -> trial] != 0))
		{
			editor->drawControlButton (
				experiment -> ok_left, experiment -> ok_right, experiment -> ok_bottom, experiment -> ok_top,
				experiment -> ok_label);
		}
		if (experiment -> oops_right > experiment -> oops_left && experiment -> trial > 1) {
			editor->drawControlButton (
				experiment -> oops_left, experiment -> oops_right, experiment -> oops_bottom, experiment -> oops_top,
				experiment -> oops_label);
		}
		Melder_free (visibleText_dup);
	} else {
		Graphics_setTextAlignment (editor->_graphics, Graphics_CENTRE, Graphics_HALF);
		Graphics_setFontSize (editor->_graphics, 24);
		Graphics_text (editor->_graphics, 0.5, 0.5, experiment -> endText);
		if (experiment -> oops_right > experiment -> oops_left && experiment -> trial > 1) {
			editor->drawControlButton (
				experiment -> oops_left, experiment -> oops_right, experiment -> oops_bottom, experiment -> oops_top,
				experiment -> oops_label);
		}
	}
}

void RunnerMFC::do_ok () {
	ExperimentMFC experiment = (ExperimentMFC) _data;
	Melder_assert (experiment -> trial >= 1 && experiment -> trial <= experiment -> numberOfTrials);
	_numberOfReplays = 0;
	if (experiment -> trial == experiment -> numberOfTrials) {
		experiment -> trial ++;
		broadcastChange ();
		Graphics_updateWs (_graphics);
	} else if (experiment -> breakAfterEvery != 0 && experiment -> trial % experiment -> breakAfterEvery == 0) {
		experiment -> pausing = TRUE;
		broadcastChange ();
		Graphics_updateWs (_graphics);
	} else {
		experiment -> trial ++;
		broadcastChange ();
		Graphics_updateWs (_graphics);
		if (experiment -> stimuliAreSounds) {
			ExperimentMFC_playStimulus (experiment, experiment -> stimuli [experiment -> trial]);
		}
	}
}

void RunnerMFC::do_oops () {
	ExperimentMFC experiment = (ExperimentMFC) _data;
	Melder_assert (experiment -> trial >= 2 && experiment -> trial <= experiment -> numberOfTrials + 1);
	if (experiment -> trial <= experiment -> numberOfTrials) {
		experiment -> responses [experiment -> trial] = 0;
		experiment -> goodnesses [experiment -> trial] = 0;
	}
	experiment -> trial --;
	experiment -> responses [experiment -> trial] = 0;
	experiment -> goodnesses [experiment -> trial] = 0;
	experiment -> pausing = FALSE;
	_numberOfReplays = 0;
	broadcastChange ();
	Graphics_updateWs (_graphics);
	if (experiment -> stimuliAreSounds) {
		ExperimentMFC_playStimulus (experiment, experiment -> stimuli [experiment -> trial]);
	}
}

void RunnerMFC::do_replay () {
	ExperimentMFC experiment = (ExperimentMFC) _data;
	Melder_assert (experiment -> trial >= 1 && experiment -> trial <= experiment -> numberOfTrials);
	_numberOfReplays ++;
	broadcastChange ();
	Graphics_updateWs (_graphics);
	if (experiment -> stimuliAreSounds) {
		ExperimentMFC_playStimulus (experiment, experiment -> stimuli [experiment -> trial]);
	}
}

void RunnerMFC::gui_drawingarea_cb_click (I, GuiDrawingAreaClickEvent event) {
	RunnerMFC *editor = (RunnerMFC *)void_me;
	if (editor->_graphics == NULL) return;   // Could be the case in the very beginning.
if (gtk && event -> type != BUTTON_PRESS) return;
	ExperimentMFC experiment = (ExperimentMFC) editor->_data;
	if (editor->_data == NULL) return;
	double reactionTime = Melder_clock () - experiment -> startingTime - experiment -> stimulusInitialSilenceDuration;
	double x, y;
	Graphics_DCtoWC (editor->_graphics, event -> x, event -> y, & x, & y);
	if (experiment -> trial == 0) {   /* The first click of the experiment. */
		experiment -> trial ++;
		editor->broadcastChange ();
		Graphics_updateWs (editor->_graphics);
		if (experiment -> stimuliAreSounds) {
			ExperimentMFC_playStimulus (experiment, experiment -> stimuli [1]);
		}
	} else if (experiment -> pausing) {   /* A click to leave the break. */
		if (x > experiment -> oops_left && x < experiment -> oops_right &&
			y > experiment -> oops_bottom && y < experiment -> oops_top && experiment -> trial > 1)
		{
			editor->do_oops ();
		} else {
			experiment -> pausing = FALSE;
			experiment -> trial ++;
			editor->broadcastChange ();
			Graphics_updateWs (editor->_graphics);
			if (experiment -> stimuliAreSounds) {
				ExperimentMFC_playStimulus (experiment, experiment -> stimuli [experiment -> trial]);
			}
		}
	} else if (experiment -> trial <= experiment -> numberOfTrials) {
		long iresponse;
		if (x > experiment -> ok_left && x < experiment -> ok_right &&
			y > experiment -> ok_bottom && y < experiment -> ok_top &&
			experiment -> responses [experiment -> trial] != 0 &&
			(experiment -> numberOfGoodnessCategories == 0 || experiment -> goodnesses [experiment -> trial] != 0))
		{
			editor->do_ok ();
		} else if (x > experiment -> replay_left && x < experiment -> replay_right &&
			y > experiment -> replay_bottom && y < experiment -> replay_top && editor->_numberOfReplays < experiment -> maximumNumberOfReplays)
		{
			editor->do_replay ();
		} else if (x > experiment -> oops_left && x < experiment -> oops_right &&
			y > experiment -> oops_bottom && y < experiment -> oops_top && experiment -> trial > 1)
		{
			editor->do_oops ();
		} else if (experiment -> responses [experiment -> trial] == 0 || experiment -> ok_right > experiment -> ok_left) {
			for (iresponse = 1; iresponse <= experiment -> numberOfDifferentResponses; iresponse ++) {
				ResponseMFC response = & experiment -> response [iresponse];
				if (x > response -> left && x < response -> right && y > response -> bottom && y < response -> top && response -> name [0] != '\0') {
					experiment -> responses [experiment -> trial] = iresponse;
					experiment -> reactionTimes [experiment -> trial] = reactionTime;
					if (experiment -> responsesAreSounds) {
						ExperimentMFC_playResponse (experiment, iresponse);
					}
					if (experiment -> ok_right <= experiment -> ok_left && experiment -> numberOfGoodnessCategories == 0) {
						editor->do_ok ();
					} else {
						editor->broadcastChange ();
						Graphics_updateWs (editor->_graphics);
					}
				}
			}
			if (experiment -> responses [experiment -> trial] != 0 && experiment -> ok_right > experiment -> ok_left) {
				for (iresponse = 1; iresponse <= experiment -> numberOfGoodnessCategories; iresponse ++) {
					GoodnessMFC cat = & experiment -> goodness [iresponse];
					if (x > cat -> left && x < cat -> right && y > cat -> bottom && y < cat -> top) {
						experiment -> goodnesses [experiment -> trial] = iresponse;
						editor->broadcastChange ();
						Graphics_updateWs (editor->_graphics);
					}
				}
			}
		} else if (experiment -> responses [experiment -> trial] != 0) {
			Melder_assert (experiment -> ok_right <= experiment -> ok_left);
			for (iresponse = 1; iresponse <= experiment -> numberOfGoodnessCategories; iresponse ++) {
				GoodnessMFC cat = & experiment -> goodness [iresponse];
				if (x > cat -> left && x < cat -> right && y > cat -> bottom && y < cat -> top) {
					experiment -> goodnesses [experiment -> trial] = iresponse;
					editor->do_ok ();
				}
			}
		}
	} else {
		if (x > experiment -> oops_left && x < experiment -> oops_right &&
			y > experiment -> oops_bottom && y < experiment -> oops_top)
		{
			editor->do_oops ();
			return;
		}
		if (editor->_iexperiment < editor->_experiments -> size) {
			editor->_iexperiment ++;
			if (! editor->startExperiment ()) {
				Melder_flushError (NULL);
				forget ();
				return;
			}
		}
	}
}

void RunnerMFC::gui_drawingarea_cb_key (I, GuiDrawingAreaKeyEvent event) {
	RunnerMFC *editor = (RunnerMFC *)void_me;
	if (editor->_graphics == NULL) return;   // Could be the case in the very beginning.
	ExperimentMFC experiment = (ExperimentMFC) editor->_data;
	if (editor->_data == NULL) return;
	if (experiment -> trial == 0) {
	} else if (experiment -> pausing) {
	} else if (experiment -> trial <= experiment -> numberOfTrials) {
		long iresponse;
		if (experiment -> ok_key != NULL && experiment -> ok_key [0] == event -> key &&
			experiment -> responses [experiment -> trial] != 0 &&
			(experiment -> numberOfGoodnessCategories == 0 || experiment -> goodnesses [experiment -> trial] != 0))
		{
			editor->do_ok ();
		} else if (experiment -> replay_key != NULL && experiment -> replay_key [0] == event -> key &&
			editor->_numberOfReplays < experiment -> maximumNumberOfReplays)
		{
			editor->do_replay ();
		} else if (experiment -> oops_key != NULL && experiment -> oops_key [0] == event -> key) {
			editor->do_oops ();
		} else if (experiment -> responses [experiment -> trial] == 0) {
			for (iresponse = 1; iresponse <= experiment -> numberOfDifferentResponses; iresponse ++) {
				ResponseMFC response = & experiment -> response [iresponse];
				if (response -> key != NULL && response -> key [0] == event -> key) {
					experiment -> responses [experiment -> trial] = iresponse;
					if (experiment -> responsesAreSounds) {
						ExperimentMFC_playResponse (experiment, iresponse);
					}
					if (experiment -> ok_right <= experiment -> ok_left && experiment -> numberOfGoodnessCategories == 0) {
						editor->do_ok ();
					} else {
						editor->broadcastChange ();
						Graphics_updateWs (editor->_graphics);
					}
				}
			}
		}
	}
}

void RunnerMFC::createChildren () {
	_drawingArea = GuiDrawingArea_createShown (_dialog, 0, 0, Machine_getMenuBarHeight (), 0,
		gui_drawingarea_cb_expose, gui_drawingarea_cb_click, gui_drawingarea_cb_key, gui_drawingarea_cb_resize, this, 0);
}

/* End of file RunnerMFC.cpp */
