/* KlattGridEditors.c
 *
 * Copyright (C) 2009-2011 david Weenink
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
 * djmw 20090123
 * djmw 20090128 Remove source menu from formant grid editor.
 * djmw 20090420 dbEditor
 * djmw 20090527 Protect FormantGridEditor against empty FormantGrids.
 * djmw 20110304 Thing_new
 */

#include "ui/Preferences.h"
#include "EditorM.h"

#include "KlattGridEditors.h"

static int KlattGrid_Editor_defaultPlay (KlattGrid klattGrid, double tmin, double tmax)
{
	klattGrid -> options -> xmin = tmin; klattGrid -> options-> xmax = tmax;
	return KlattGrid_playSpecial (klattGrid);
}

/************************** KlattGrid_realTierEditor *********************************/

static int menu_cb_KlattGridHelp (EDITOR_ARGS) { Melder_help (L"KlattGrid"); return 1; }

KlattGrid_realTierEditor::KlattGrid_realTierEditor (GuiObject parent, const wchar_t *title, KlattGrid klattgrid, RealTier data)
	: RealTierEditor (parent, title, data, NULL, 0) {
	_klattgrid = klattgrid;
}

void KlattGrid_realTierEditor::createHelpMenuItems (EditorMenu *menu)
{
	menu->addCommand (L"KlattGrid help", 0, menu_cb_KlattGridHelp);
}

void KlattGrid_realTierEditor::play (double tmin, double tmax)
{
	KlattGrid_Editor_defaultPlay (_klattgrid, tmin, tmax);
}

/************************** KlattGrid_pitchTierEditor *********************************/

static int menu_cb_KlattGrid_pitchTierEditorHelp (EDITOR_ARGS)
{
	Melder_help (L"PitchTierEditor"); return 1;
}

static int menu_cb_PitchTierHelp (EDITOR_ARGS)
{
	Melder_help (L"PitchTier"); return 1;
}

KlattGrid_pitchTierEditor::KlattGrid_pitchTierEditor (GuiObject parent, const wchar_t *title, KlattGrid klattgrid)
	: KlattGrid_realTierEditor (parent, title, klattgrid, (RealTier) klattgrid -> phonation -> pitch) {}

void KlattGrid_pitchTierEditor::createHelpMenuItems (EditorMenu *menu)
{
	KlattGrid_realTierEditor::createHelpMenuItems (menu);
	menu->addCommand (L"PitchTierEditor help", 0, menu_cb_KlattGrid_pitchTierEditorHelp);
	menu->addCommand (L"PitchTier help", 0, menu_cb_PitchTierHelp);
}

/************************** KlattGrid_intensityTierEditor *********************************/

static int menu_cb_IntensityTierHelp (EDITOR_ARGS) { Melder_help (L"IntensityTier"); return 1; }

KlattGrid_intensityTierEditor::KlattGrid_intensityTierEditor (GuiObject parent, const wchar_t *title, KlattGrid klattgrid, RealTier tier)
	: KlattGrid_realTierEditor (parent, title, klattgrid, tier) {}

void KlattGrid_intensityTierEditor::createHelpMenuItems (EditorMenu *menu)
{
	KlattGrid_realTierEditor::createHelpMenuItems (menu);
	menu->addCommand (L"IntensityTier help", 0, menu_cb_IntensityTierHelp);
}

/************************** KlattGrid_DecibelTierEditor *********************************/

KlattGrid_decibelTierEditor::KlattGrid_decibelTierEditor (GuiObject parent, const wchar_t *title, KlattGrid klattgrid, RealTier tier)
	: KlattGrid_intensityTierEditor (parent, title, klattgrid, tier) {}

/************************** KlattGrid_voicingAmplitudeTierEditor *********************************/

KlattGrid_voicingAmplitudeTierEditor::KlattGrid_voicingAmplitudeTierEditor (GuiObject parent, const wchar_t *title, KlattGrid klattgrid)
	: KlattGrid_intensityTierEditor	(parent, title, klattgrid, (RealTier) klattgrid -> phonation -> voicingAmplitude) {}

/************************** KlattGrid_aspirationAmplitudeTierEditor *********************************/

KlattGrid_aspirationAmplitudeTierEditor::KlattGrid_aspirationAmplitudeTierEditor (GuiObject parent, const wchar_t *title, KlattGrid klattgrid)
	: KlattGrid_intensityTierEditor (parent, title, klattgrid, (RealTier) klattgrid -> phonation -> aspirationAmplitude) {}

/************************** KlattGrid_breathinessAmplitudeTierEditor *********************************/

KlattGrid_breathinessAmplitudeTierEditor::KlattGrid_breathinessAmplitudeTierEditor (GuiObject parent, const wchar_t *title, KlattGrid klattgrid)
	: KlattGrid_intensityTierEditor (parent, title, klattgrid, (RealTier) klattgrid -> phonation -> breathinessAmplitude) {}

/************************** KlattGrid_spectralTiltTierEditor *********************************/

KlattGrid_spectralTiltTierEditor::KlattGrid_spectralTiltTierEditor (GuiObject parent, const wchar_t *title, KlattGrid klattgrid)
	: KlattGrid_intensityTierEditor (parent, title, klattgrid, (RealTier) klattgrid -> phonation -> spectralTilt) {}

/************************** KlattGrid_fricationBypassTierEditor *********************************/

KlattGrid_fricationBypassTierEditor::KlattGrid_fricationBypassTierEditor (GuiObject parent, const wchar_t *title, KlattGrid klattgrid)
	: KlattGrid_intensityTierEditor (parent, title, klattgrid, (RealTier) klattgrid -> frication -> bypass) {}

/************************** KlattGrid_fricationAmplitudeTierEditor *********************************/

KlattGrid_fricationAmplitudeTierEditor::KlattGrid_fricationAmplitudeTierEditor (GuiObject parent, const wchar_t *title, KlattGrid klattgrid)
	: KlattGrid_intensityTierEditor (parent, title, klattgrid, (RealTier) klattgrid -> frication -> fricationAmplitude) {}

/************************** KlattGrid_openPhaseTierEditor *********************************/

KlattGrid_openPhaseTierEditor::KlattGrid_openPhaseTierEditor (GuiObject parent, const wchar_t *title, KlattGrid klattgrid)
	: KlattGrid_intensityTierEditor (parent, title, klattgrid, (RealTier) klattgrid -> phonation -> openPhase) {}

/************************** KlattGrid_collisionPhaseTierEditor *********************************/

KlattGrid_collisionPhaseTierEditor::KlattGrid_collisionPhaseTierEditor (GuiObject parent, const wchar_t *title, KlattGrid klattgrid)
	: KlattGrid_intensityTierEditor (parent, title, klattgrid, (RealTier) klattgrid -> phonation -> collisionPhase) {}

/************************** KlattGrid_power1TierEditor *********************************/

KlattGrid_power1TierEditor::KlattGrid_power1TierEditor (GuiObject parent, const wchar_t *title, KlattGrid klattgrid)
	: KlattGrid_intensityTierEditor (parent, title, klattgrid, (RealTier) klattgrid -> phonation -> power1) {}

/************************** KlattGrid_power2TierEditor *********************************/

KlattGrid_power2TierEditor::KlattGrid_power2TierEditor (GuiObject parent, const wchar_t *title, KlattGrid klattgrid)
	: KlattGrid_intensityTierEditor (parent, title, klattgrid, (RealTier) klattgrid -> phonation -> power2) {}

/************************** KlattGrid_flutterTierEditor *********************************/

KlattGrid_flutterTierEditor::KlattGrid_flutterTierEditor (GuiObject parent, const wchar_t *title, KlattGrid klattgrid)
	: KlattGrid_intensityTierEditor (parent, title, klattgrid, (RealTier) klattgrid -> phonation -> flutter) {}

/************************** KlattGrid_doublePulsingTierEditor *********************************/

KlattGrid_doublePulsingTierEditor::KlattGrid_doublePulsingTierEditor (GuiObject parent, const wchar_t *title, KlattGrid klattgrid)
	: KlattGrid_intensityTierEditor (parent, title, klattgrid, (RealTier) klattgrid -> phonation -> doublePulsing) {}

/************************** KlattGrid_formantGridEditor *********************************/

static int FormantGrid_isEmpty (FormantGrid formantGrid)
{
	return formantGrid -> formants -> size == 0 || formantGrid -> bandwidths -> size == 0;
}

KlattGrid_formantGridEditor::KlattGrid_formantGridEditor (GuiObject parent, const wchar_t *title, KlattGrid data, int formantType)
	: FormantGridEditor (parent, title, (structFormantGrid *)KlattGrid_getAddressOfFormantGrid (data, formantType)) {
	Melder_assert (data != NULL);
	//if (fg == NULL) return Melder_errorp1 (L"Formant type unknown."); // FIXME exception
	//if (FormantGrid_isEmpty (*fg)) return Melder_errorp1 (L"Cannot edit an empty formant grid.");
	_klattgrid = data;
}

void KlattGrid_formantGridEditor::play (double tmin, double tmax)
{
	KlattGrid_Editor_defaultPlay (_klattgrid, tmin, tmax);
}

/* End of file KlattGridEditors.c */
