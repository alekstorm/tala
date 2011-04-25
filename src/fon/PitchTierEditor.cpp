/* PitchTierEditor.cpp
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
 * pb 2002/07/16 GPL
 * pb 2007/06/10 wchar_t
 * pb 2007/08/12 wchar_t
 * pb 2008/03/20 split off Help menu
 * pb 2008/03/21 new Editor API
 * pb 2009/01/23 minimum and maximum legal values
 * pb 2011/03/22 C++
 */

#include "PitchTierEditor.h"
#include "PitchTier_to_Sound.h"
#include "sys/EditorM.h"

PitchTierEditor::PitchTierEditor (GuiObject parent, const wchar_t *title, PitchTier pitch, Sound sound, int ownSound)
	: RealTierEditor (parent, title, (RealTier) pitch, sound, ownSound) {
	createMenus ();
}

static int menu_cb_PitchTierEditorHelp (EDITOR_ARGS) { Melder_help (L"PitchTierEditor"); return 1; }
static int menu_cb_PitchTierHelp (EDITOR_ARGS) { Melder_help (L"PitchTier"); return 1; }

void PitchTierEditor::createMenus () {
	EditorMenu *menu = getMenu (L"Help");
	menu->addCommand (L"PitchTierEditor help", 0, menu_cb_PitchTierEditorHelp);
	menu->addCommand (L"PitchTier help", 0, menu_cb_PitchTierHelp);
}

void PitchTierEditor::play (double tmin, double tmax) {
	if (_sound.data) Sound_playPart (_sound.data, tmin, tmax, playCallback, this);
	else if (! PitchTier_playPart ((PitchTier) _data, tmin, tmax, FALSE)) Melder_flushError (NULL);
}

/* End of file PitchTierEditor.cpp */
