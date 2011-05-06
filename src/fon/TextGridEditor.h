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

#include "SpellingChecker.h"
#include "TextGrid.h"
#include "TextGridEditor_enums.h"
#include "TimeSoundAnalysisEditor.h"
#include "sys/Preferences.h"

#define TEXT_HEIGHT  50

class TextGridEditor : public TimeSoundAnalysisEditor {
  public:
	static void prefs (void);

	TextGridEditor (GuiObject parent, const wchar_t *title, TextGrid grid,
		Any sound,   /* 'sound' could be a Sound or a LongSound */
		Any spellingChecker);
	virtual ~TextGridEditor ();

	SpellingChecker _spellingChecker;
	long _selectedTier;
	bool _useTextStyles, _shiftDragMultiple, _suppressRedraw;
	int _fontSize;
	enum kGraphics_horizontalAlignment _alignment;
	wchar_t *_findString, _greenString [Preferences_STRING_BUFFER_SIZE];
	enum kTextGridEditor_showNumberOf _showNumberOf;
	enum kMelder_string _greenMethod;
	GuiObject _extractSelectedTextGridPreserveTimesButton, _extractSelectedTextGridTimeFromZeroButton, _writeSelectedTextGridButton;

  protected:
	virtual void info ();

	virtual void draw ();
	virtual int click (double xclick, double yWC, int shiftKeyPressed);
	virtual void play (double tmin, double tmax);
	virtual void dataChanged ();
	virtual void prepareDraw ();

	virtual void highlightSelection (double left, double right, double bottom, double top);
	virtual void unhighlightSelection (double left, double right, double bottom, double top);

  private:
	static int menu_cb_ExtractSelectedTextGrid_preserveTimes (EDITOR_ARGS);
	static int menu_cb_ExtractSelectedTextGrid_timeFromZero (EDITOR_ARGS);
	static int menu_cb_WriteSelectionToTextFile (EDITOR_ARGS);
	static int menu_cb_WriteToTextFile (EDITOR_ARGS);
	static int menu_cb_DrawVisibleTextGrid (EDITOR_ARGS);
	static int menu_cb_DrawVisibleSoundAndTextGrid (EDITOR_ARGS);
#ifndef macintosh
	static int menu_cb_Cut (EDITOR_ARGS);
	static int menu_cb_Copy (EDITOR_ARGS);
	static int menu_cb_Paste (EDITOR_ARGS);
	static int menu_cb_Erase (EDITOR_ARGS);
#endif
	static int menu_cb_Genericize (EDITOR_ARGS);
	static int menu_cb_Nativize (EDITOR_ARGS);
	static int menu_cb_GetStartingPointOfInterval (EDITOR_ARGS);
	static int menu_cb_GetEndPointOfInterval (EDITOR_ARGS);
	static int menu_cb_GetLabelOfInterval (EDITOR_ARGS);
	static int menu_cb_SelectPreviousTier (EDITOR_ARGS);
	static int menu_cb_SelectNextTier (EDITOR_ARGS);
	static int menu_cb_SelectPreviousInterval (EDITOR_ARGS);
	static int menu_cb_SelectNextInterval (EDITOR_ARGS);
	static int menu_cb_ExtendSelectPreviousInterval (EDITOR_ARGS);
	static int menu_cb_ExtendSelectNextInterval (EDITOR_ARGS);
	static int menu_cb_MoveBtoZero (EDITOR_ARGS);
	static int menu_cb_MoveCursorToZero (EDITOR_ARGS);
	static int menu_cb_MoveEtoZero (EDITOR_ARGS);
	static int menu_cb_DrawTextGridAndPitch (EDITOR_ARGS);
	static int menu_cb_InsertIntervalOnTier1 (EDITOR_ARGS);
	static int menu_cb_InsertIntervalOnTier2 (EDITOR_ARGS);
	static int menu_cb_InsertIntervalOnTier3 (EDITOR_ARGS);
	static int menu_cb_InsertIntervalOnTier4 (EDITOR_ARGS);
	static int menu_cb_InsertIntervalOnTier5 (EDITOR_ARGS);
	static int menu_cb_InsertIntervalOnTier6 (EDITOR_ARGS);
	static int menu_cb_InsertIntervalOnTier7 (EDITOR_ARGS);
	static int menu_cb_InsertIntervalOnTier8 (EDITOR_ARGS);
	static int menu_cb_RemovePointOrBoundary (EDITOR_ARGS);
	static int menu_cb_MoveToB (EDITOR_ARGS);
	static int menu_cb_MoveToE (EDITOR_ARGS);
	static int menu_cb_MoveToZero (EDITOR_ARGS);
	static int menu_cb_InsertOnSelectedTier (EDITOR_ARGS);
	static int menu_cb_InsertOnTier1 (EDITOR_ARGS);
	static int menu_cb_InsertOnTier2 (EDITOR_ARGS);
	static int menu_cb_InsertOnTier3 (EDITOR_ARGS);
	static int menu_cb_InsertOnTier4 (EDITOR_ARGS);
	static int menu_cb_InsertOnTier5 (EDITOR_ARGS);
	static int menu_cb_InsertOnTier6 (EDITOR_ARGS);
	static int menu_cb_InsertOnTier7 (EDITOR_ARGS);
	static int menu_cb_InsertOnTier8 (EDITOR_ARGS);
	static int menu_cb_InsertOnAllTiers (EDITOR_ARGS);
	static int menu_cb_Find (EDITOR_ARGS);
	static int menu_cb_FindAgain (EDITOR_ARGS);
	static int menu_cb_CheckSpelling (EDITOR_ARGS);
	static int menu_cb_CheckSpellingInInterval (EDITOR_ARGS);
	static int menu_cb_AddToUserDictionary (EDITOR_ARGS);
	static int menu_cb_RenameTier (EDITOR_ARGS);
	static int menu_cb_PublishTier (EDITOR_ARGS);
	static int menu_cb_RemoveAllTextFromTier (EDITOR_ARGS);
	static int menu_cb_RemoveTier (EDITOR_ARGS);
	static int menu_cb_AddIntervalTier (EDITOR_ARGS);
	static int menu_cb_AddPointTier (EDITOR_ARGS);
	static int menu_cb_DuplicateTier (EDITOR_ARGS);
	static int menu_cb_TextGridEditorHelp (EDITOR_ARGS);
	static int menu_cb_AboutSpecialSymbols (EDITOR_ARGS);
	static int menu_cb_PhoneticSymbols (EDITOR_ARGS);
	static int menu_cb_AboutTextStyles (EDITOR_ARGS);
	static void gui_text_cb_change (I, GuiTextEvent event);

	virtual const wchar_t * type () { return L"TextGridEditor"; }

	double _computeSoundY ();
	int _yWCtoTier (double yWC);
	void _timeToInterval (double t, int itier, double *tmin, double *tmax);

	int checkTierSelection (const wchar_t *verbPhrase);
	long getSelectedInterval ();
	long getSelectedLeftBoundary ();
	long getSelectedPoint ();
	void scrollToView (double t);
	void do_selectAdjacentTier (int previous);
	void do_selectAdjacentInterval (int previous, int shift);
	int insertBoundaryOrPoint (int itier, double t1, double t2, bool insertSecond);
	void do_insertIntervalOnTier (int itier);
	int do_movePointOrBoundary (int where);
	void do_insertOnTier (int itier);
	int findInTier ();
	void do_find ();
	int checkSpellingInTier ();
	void do_drawIntervalTier (IntervalTier tier, int itier);
	void do_drawTextTier (TextTier tier, int itier);
	void do_drawWhileDragging (double numberOfTiers, int *selectedTier, double x, double soundY);
	void do_dragBoundary (double xbegin, int iClickedTier, int shiftKeyPressed);
	int clickB (double t, double yWC);
	int clickE (double t, double yWC);
	void updateText ();
	void prefs_addFields (EditorCommand *cmd);
	void prefs_setValues (EditorCommand *cmd);
	void prefs_getValues (EditorCommand *cmd);
	double getBottomOfSoundAndAnalysisArea ();
	void updateMenuItems_file ();

	// FIXME
	void createMenuItems_view_timeDomain (EditorMenu *menu);
	void createMenuItems_pitch_picture (EditorMenu *menu);

	void createMenus ();
	void createChildren ();
};

/* End of file TextGridEditor.h */
#endif
