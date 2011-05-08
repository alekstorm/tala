#ifndef _VowelEditor_h_
#define _VowelEditor_h_
/* VowelEditor.h
 *
 * Copyright (C) 2008-2011 David Weenink
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
 djmw 20070130 First
 djmw 20110306 Latest modification.
*/

#include "fon/FormantTier.h"
#include "fon/PitchTier.h"
#include "stat/TableOfReal.h"
#include "Editor.h"
#include "ui/Graphics.h"

#define Vowel_members Function_members \
	PitchTier pt; \
	FormantTier ft;
#define Vowel_methods Function_methods
class_create (Vowel, Function);

struct structF0 
{
	double start;
	double slopeOctPerSec;
	double minimum, maximum;
	double samplingFrequency, adaptFactor, adaptTime;
	long interpolationDepth;
};

struct structF1F2Grid
{
	double df1, df2;
	int text_left, text_right, text_bottom, text_top;
	double grey;
};

class VowelEditor : public Editor {
  public:
	static void prefs (void);

	VowelEditor (GuiObject parent, const wchar_t *title, Any data);
	virtual ~VowelEditor ();

	int _soundFollowsMouse, _shiftKeyPressed;
	double _f1min, _f1max, _f2min, _f2max; /* Domain of graphics F1-F2 area */
	Matrix _f3, _b3, _f4, _b4;
	int _frequencyScale; /* 0: lin, 1: log, 2: bark, 3: mel */
	int _axisOrientation; /* 0: origin topright + f1 down + f2 to left, 0: origin lb + f1 right +f2 up */
	int _speakerType; /* 1 male, 2 female, 3 child */
	Graphics _g; /* the drawing */
	short _width, _height; /* Size of drawing area in pixels. */
	Table _marks; /* Vowel, F1, F2, Colour... */
	Vowel _vowel;
	double _markTraceEvery;
	struct structF0 _f0;
	double _maximumDuration, _extendDuration;
	Sound _source, _target;
	GuiObject _drawingArea, _playButton, _reverseButton, _publishButton;
	GuiObject _f0Label, _f0TextField, _f0SlopeLabel, _f0SlopeTextField;
	GuiObject _durationLabel, _durationTextField, _extendLabel, _extendTextField;
	GuiObject _startInfo, _endInfo;
	struct structF1F2Grid _grid;

  protected:
	virtual void dataChanged ();

  private:
	static void cb_publish (Editor *editor, void *closure, Any publish);
	// menu callbacks
	static int menu_cb_help (EDITOR_ARGS);
	static int menu_cb_prefs (EDITOR_ARGS);
	static int menu_cb_publishSound (EDITOR_ARGS);
	static int menu_cb_extract_FormantGrid (EDITOR_ARGS);
	static int menu_cb_extract_PitchTier (EDITOR_ARGS);
	static int menu_cb_extract_KlattGrid (EDITOR_ARGS);
	static int menu_cb_drawTrajectory (EDITOR_ARGS);
	static int menu_cb_showOneVowelMark (EDITOR_ARGS);
	static int menu_cb_showVowelMarks (EDITOR_ARGS);
	static int menu_cb_setF0 (EDITOR_ARGS);
	static int menu_cb_setF3F4 (EDITOR_ARGS);
	static int menu_cb_reverseTrajectory (EDITOR_ARGS);
	static int menu_cb_newTrajectory (EDITOR_ARGS);
	static int menu_cb_extendTrajectory (EDITOR_ARGS);
	static int menu_cb_modifyTrajectoryDuration (EDITOR_ARGS);
	static int menu_cb_shiftTrajectory (EDITOR_ARGS);
	static int menu_cb_showTrajectoryTimeMarkersEvery (EDITOR_ARGS);
	// button callbacks
	static void gui_button_cb_publish (I, GuiButtonEvent event);
	static void gui_button_cb_play (I, GuiButtonEvent event);
	static void gui_drawingarea_cb_resize (I, GuiDrawingAreaResizeEvent event);
	static void gui_drawingarea_cb_click (I, GuiDrawingAreaClickEvent event);
	static void gui_drawingarea_cb_key (I, GuiDrawingAreaKeyEvent event);
	static void gui_button_cb_reverse (I, GuiButtonEvent event);
	static void gui_drawingarea_cb_expose (I, GuiDrawingAreaExposeEvent event);

	virtual const wchar_t * type () { return L"VowelEditor"; }

	PitchTier to_PitchTier (double duration);
	void updateF0Info ();
	void updateExtendDuration ();
	double updateDurationInfo ();
	int Vowel_updateTiers (Vowel thee, double time, double x, double y);
	int Vowel_addData (Vowel thee, double time, double f1, double f2, double f0);
	void getXYFromF1F2 (double f1, double f2, double *x, double *y);
	void getF1F2FromXY (double x, double y, double *f1, double *f2);
	void updateVowel ();
	Sound createTarget ();
	void Vowel_reverseFormantTier ();
	void checkF1F2 (double *f1, double *f2);
	void shiftF1F2 (double f1_st, double f2_st);
	int setSource ();
	int setMarks (int dataset, int speakerType, int fontSize);
	int setF3F4 (double f3, double b3, double f4, double b4);
	void getF3F4 (double f1, double f2, double *f3, double *b3, double *f4, double *b4);
	void drawBackground (Graphics g);
	void updateWidgets ();

	void createMenus ();
	void createChildren ();
};

#endif /* _VowelEditor_h_ */
