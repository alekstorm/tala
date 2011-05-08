#ifndef _TimeSoundEditor_h_
#define _TimeSoundEditor_h_
/* TimeSoundEditor.h
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

#include "FunctionEditor.h"
#include "fon/LongSound.h"
#include "fon/Sound.h"

struct TimeSoundEditor_sound {
	/* KEEP IN SYNC WITH PREFS. */
	Sound data;
	bool autoscaling;
	double minimum, maximum;
};

class TimeSoundEditor : public FunctionEditor {
  public:
	static void prefs (void);

	TimeSoundEditor (GuiObject parent, const wchar_t *title, Any data, Any sound, bool ownSound);
	virtual ~TimeSoundEditor ();

	bool _ownSound;
	struct TimeSoundEditor_sound _sound;
	struct { LongSound data; } _longSound;
	GuiObject _drawButton, _publishButton, _publishPreserveButton, _publishWindowButton;
	GuiObject _writeAiffButton, _writeAifcButton, _writeWavButton, _writeNextSunButton, _writeNistButton, _writeFlacButton;

  protected:
	virtual void info ();
	virtual void draw_sound (double globalMinimum, double globalMaximum);

	virtual void updateMenuItems_file (); // FIXME

  private:
	static int menu_cb_DrawVisibleSound (EDITOR_ARGS);
	static int menu_cb_DrawSelectedSound (EDITOR_ARGS);
	static int menu_cb_ExtractSelectedSound_timeFromZero (EDITOR_ARGS);
	static int menu_cb_ExtractSelectedSound_preserveTimes (EDITOR_ARGS);
	static int menu_cb_ExtractSelectedSound_windowed (EDITOR_ARGS);
	static int menu_cb_WriteWav (EDITOR_ARGS);
	static int menu_cb_WriteAiff (EDITOR_ARGS);
	static int menu_cb_WriteAifc (EDITOR_ARGS);
	static int menu_cb_WriteNextSun (EDITOR_ARGS);
	static int menu_cb_WriteNist (EDITOR_ARGS);
	static int menu_cb_WriteFlac (EDITOR_ARGS);
	static int menu_cb_SoundInfo (EDITOR_ARGS);
	static int menu_cb_LongSoundInfo (EDITOR_ARGS);
	static int menu_cb_autoscaling (EDITOR_ARGS);

	virtual const wchar_t * type () { return L"TimeSoundEditor"; }

	int do_ExtractSelectedSound (bool preserveTimes);
	int do_write (MelderFile file, int format);

	void createMenus ();
};

/* End of file TimeSoundEditor.h */
#endif
