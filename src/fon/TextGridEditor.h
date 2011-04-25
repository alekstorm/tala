#ifndef _TextGridEditor_h_
#define _TextGridEditor_h_
/* TextGridEditor.h
 *
 * Copyright (C) 1992-2011 Paul Boersma
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
 * pb 2011/03/23
 */

#include "TimeSoundAnalysisEditor.h"
#include "SpellingChecker.h"
#include "sys/Preferences.h"
#include "TextGrid.h"
#include "TextGridEditor_enums.h"

#define TEXT_HEIGHT  50

class TextGridEditor : public TimeSoundAnalysisEditor {
  public:
	static void prefs (void);

	TextGridEditor (GuiObject parent, const wchar_t *title, TextGrid grid,
		Any sound,   /* 'sound' could be a Sound or a LongSound */
		Any spellingChecker);
	~TextGridEditor ();

	const wchar_t * type () { return L"TextGridEditor"; }
	void info ();

	double _computeSoundY ();
	int _yWCtoTier (double yWC);
	void _timeToInterval (double t, int itier, double *tmin, double *tmax);

	int checkTierSelection (const wchar_t *verbPhrase);
	long getSelectedInterval ();
	long getSelectedLeftBoundary ();
	long getSelectedPoint ();
	void scrollToView (double t);
	void createMenuItems_file_extract (EditorMenu *menu);
	void createMenuItems_file_write (EditorMenu *menu);
	void createMenuItems_file_draw (EditorMenu *menu);
	void do_selectAdjacentTier (int previous);
	void do_selectAdjacentInterval (int previous, int shift);
	int insertBoundaryOrPoint (int itier, double t1, double t2, bool insertSecond);
	void do_insertIntervalOnTier (int itier);
	int do_movePointOrBoundary (int where);
	void do_insertOnTier (int itier);
	int findInTier ();
	void do_find ();
	int checkSpellingInTier ();
	void createMenus ();
	void createChildren ();
	void dataChanged ();
	void prepareDraw ();
	void do_drawIntervalTier (IntervalTier tier, int itier);
	void do_drawTextTier (TextTier tier, int itier);
	void draw ();
	void do_drawWhileDragging (double numberOfTiers, int *selectedTier, double x, double soundY);
	void do_dragBoundary (double xbegin, int iClickedTier, int shiftKeyPressed);
	int click (double xclick, double yWC, int shiftKeyPressed);
	int clickB (double t, double yWC);
	int clickE (double t, double yWC);
	void play (double tmin, double tmax);
	void updateText ();
	void prefs_addFields (EditorCommand *cmd);
	void prefs_setValues (EditorCommand *cmd);
	void prefs_getValues (EditorCommand *cmd);
	void createMenuItems_view_timeDomain (EditorMenu *menu);
	void highlightSelection (double left, double right, double bottom, double top);
	void unhighlightSelection (double left, double right, double bottom, double top);
	double getBottomOfSoundAndAnalysisArea ();
	void createMenuItems_pitch_picture (EditorMenu *menu);
	void updateMenuItems_file ();

	SpellingChecker _spellingChecker;
	long _selectedTier;
	bool _useTextStyles, _shiftDragMultiple, _suppressRedraw;
	int _fontSize;
	enum kGraphics_horizontalAlignment _alignment;
	wchar_t *_findString, _greenString [Preferences_STRING_BUFFER_SIZE];
	enum kTextGridEditor_showNumberOf _showNumberOf;
	enum kMelder_string _greenMethod;
	GuiObject _extractSelectedTextGridPreserveTimesButton, _extractSelectedTextGridTimeFromZeroButton, _writeSelectedTextGridButton;
};

/* End of file TextGridEditor.h */
#endif
