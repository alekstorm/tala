/* DurationTierEditor.c
 *
 * Copyright (C) 1992-2009 Paul Boersma
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
 * pb 2008/03/21
 * pb 2009/01/23 minimum and maximum legal values
 */

#include "DurationTierEditor.h"
#include "sys/EditorM.h"

DurationTierEditor::DurationTierEditor (GuiObject parent, const wchar_t *title, DurationTier duration, Sound sound, int ownSound)
	: RealTierEditor (parent, title, (RealTier) duration, sound, ownSound) {}

static int menu_cb_DurationTierHelp (EDITOR_ARGS) { Melder_help (L"DurationTier"); return 1; }

void DurationTierEditor::createHelpMenuItems (EditorMenu *menu) {
	RealTierEditor::createHelpMenuItems (menu);
	menu->addCommand (L"DurationTier help", 0, menu_cb_DurationTierHelp);
}

void DurationTierEditor::play (double tmin, double tmax) {
	if (_sound.data) {
		Sound_playPart (_sound.data, tmin, tmax, NULL, NULL);
	} else {
		/*if (! DurationTier_playPart (_data, tmin, tmax, FALSE)) Melder_flushError (NULL);*/
	}
}

/* End of file DurationTierEditor.c */
