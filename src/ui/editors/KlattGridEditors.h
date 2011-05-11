#ifndef _KlattGridEditors_h_
#define _KlattGridEditors_h_
/* KlattGridEditors.h
 *
 * Copyright (C) 2009-2011 David Weenink
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
  djmw 20090123, 20090420, 20090630
  djmw 20110306 Latest modification.
*/

#include "FormantGridEditor.h"
#include "RealTierEditor.h"
#include "dwtools/KlattGrid.h"
#include "fon/IntensityTier.h"
#include "fon/PitchTier.h"

class KlattGrid_realTierEditor : public RealTierEditor {
  public:
	KlattGrid_realTierEditor (GuiObject parent, const wchar_t *title, KlattGrid klattgrid, RealTier data);

	virtual const wchar_t * type () { return L"KlattGrid_realTierEditor"; }

	virtual void createHelpMenuItems (EditorMenu *menu);
	virtual void play (double tmin, double tmax);

	KlattGrid _klattgrid;
};

class KlattGrid_pitchTierEditor : public KlattGrid_realTierEditor {
  public:
	KlattGrid_pitchTierEditor (GuiObject parent, const wchar_t *title, KlattGrid klattgrid);

	virtual void createHelpMenuItems (EditorMenu *menu);

	virtual const wchar_t * type () { return L"KlattGrid_pitchTierEditor"; }
	virtual const wchar_t * quantityText () { return L"Frequency (Hz)"; }
	virtual const wchar_t * quantityKey () { return L"Frequency (Hz)"; }
	virtual const wchar_t * rightTickUnits () { return L" Hz"; }
	virtual double defaultYmin () { return 50.0; }
	virtual double defaultYmax () { return 600.0; }
	virtual const wchar_t * setRangeTitle () { return L"Set frequency range..."; }
	virtual const wchar_t * defaultYminText () { return L"50.0"; }
	virtual const wchar_t * defaultYmaxText () { return L"600.0"; }
	virtual const wchar_t * yminText () { return L"Minimum frequency (Hz)"; }
	virtual const wchar_t * ymaxText () { return L"Maximum frequency (Hz)"; }
	virtual const wchar_t * yminKey () { return L"Minimum frequency"; }
	virtual const wchar_t * ymaxKey () { return L"Maximum frequency"; }
};

class KlattGrid_intensityTierEditor : public KlattGrid_realTierEditor {
  public:
	KlattGrid_intensityTierEditor (GuiObject parent, const wchar_t *title, KlattGrid klattgrid, RealTier tier);

	virtual const wchar_t * type () { return L"KlattGrid_intensityTierEditor"; }
	virtual const wchar_t * quantityText () { return L"Intensity (dB)"; }
	virtual const wchar_t * quantityKey () { return L"Intensity"; }
	virtual const wchar_t * rightTickUnits () { return L" dB"; }
	virtual double defaultYmin () { return 50.0; }
	virtual double defaultYmax () { return 100.0; }
	virtual const wchar_t * setRangeTitle () { return L"Set intensity range..."; }
	virtual const wchar_t * defaultYminText () { return L"50.0"; }
	virtual const wchar_t * defaultYmaxText () { return L"100.0"; }
	virtual const wchar_t * yminText () { return L"Minimum intensity (dB)"; }
	virtual const wchar_t * ymaxText () { return L"Maximum intensity (dB)"; }
	virtual const wchar_t * yminKey () { return L"Minimum intensity"; }
	virtual const wchar_t * ymaxKey () { return L"Maximum intensity"; }

	virtual void createHelpMenuItems (EditorMenu *menu);
};

class KlattGrid_decibelTierEditor : public KlattGrid_intensityTierEditor {
  public:
	KlattGrid_decibelTierEditor (GuiObject parent, const wchar_t *title, KlattGrid klattgrid, RealTier tier);

	virtual const wchar_t * type () { return L"KlattGrid_decibelTierEditor"; }
	virtual const wchar_t * quantityText () { return L"Amplitude (dB)"; }
	virtual const wchar_t * quantityKey () { return L"Amplitude"; }
	virtual const wchar_t * rightTickUnits () { return L" dB"; }
	virtual double defaultYmin () { return -30.0; }
	virtual double defaultYmax () { return 30.0; }
	virtual const wchar_t * setRangeTitle () { return L"Set amplitude range..."; }
	virtual const wchar_t * defaultYminText () { return L"-30.0"; }
	virtual const wchar_t * defaultYmaxText () { return L"30.0"; }
	virtual const wchar_t * yminText () { return L"Minimum amplitude (dB)"; }
	virtual const wchar_t * ymaxText () { return L"Maximum amplitude (dB)"; }
	virtual const wchar_t * yminKey () { return L"Minimum amplitude"; }
	virtual const wchar_t * ymaxKey () { return L"Maximum amplitude"; }
};

class KlattGrid_voicingAmplitudeTierEditor : public KlattGrid_intensityTierEditor {
  public:
	KlattGrid_voicingAmplitudeTierEditor (GuiObject parent, const wchar_t *title, KlattGrid klattgrid);
};

class KlattGrid_aspirationAmplitudeTierEditor : public KlattGrid_intensityTierEditor {
  public:
	KlattGrid_aspirationAmplitudeTierEditor (GuiObject parent, const wchar_t *title, KlattGrid klattgrid);
};

class KlattGrid_breathinessAmplitudeTierEditor : public KlattGrid_intensityTierEditor {
  public:
	KlattGrid_breathinessAmplitudeTierEditor (GuiObject parent, const wchar_t *title, KlattGrid klattgrid);
};

class KlattGrid_spectralTiltTierEditor : public KlattGrid_intensityTierEditor {
  public:
	KlattGrid_spectralTiltTierEditor (GuiObject parent, const wchar_t *title, KlattGrid klattgrid);

	virtual const wchar_t * type () { return L"KlattGrid_spectralTierEditor"; }
	virtual double defaultYmin () { return -50.0; }
	virtual double defaultYmax () { return 10.0; }
	virtual const wchar_t * defaultYminText () { return L"-50.0"; }
	virtual const wchar_t * defaultYmaxText () { return L"10.0"; }
};

class KlattGrid_fricationBypassTierEditor : public KlattGrid_intensityTierEditor {
  public:
	KlattGrid_fricationBypassTierEditor (GuiObject parent, const wchar_t *title, KlattGrid klattgrid);

	virtual const wchar_t * type () { return L"KlattGrid_fricationBypassTierEditor"; }
	virtual double defaultYmin () { return -50.0; }
	virtual double defaultYmax () { return 10.0; }
	virtual const wchar_t * defaultYminText () { return L"-50.0"; }
	virtual const wchar_t * defaultYmaxText () { return L"10.0"; }
};

class KlattGrid_fricationAmplitudeTierEditor : public KlattGrid_intensityTierEditor {
  public:
	KlattGrid_fricationAmplitudeTierEditor (GuiObject parent, const wchar_t *title, KlattGrid klattgrid);
};

class KlattGrid_openPhaseTierEditor : public KlattGrid_intensityTierEditor {
  public:
	KlattGrid_openPhaseTierEditor (GuiObject parent, const wchar_t *title, KlattGrid klattgrid);

	virtual const wchar_t * type () { return L"KlattGrid_openPhaseTierEditor"; }
	virtual double minimumLegalValue () { return 0; }
	virtual double maximumLegalValue () { return 1; }
	virtual const wchar_t * quantityText () { return L"Open phase (0..1)"; }
	virtual const wchar_t * quantityKey () { return L"Open phase"; }
	virtual const wchar_t * rightTickUnits () { return L""; }
	virtual double defaultYmin () { return 0; }
	virtual double defaultYmax () { return 1; }
	virtual const wchar_t * setRangeTitle () { return L"Set open phase range..."; }
	virtual const wchar_t * defaultYminText () { return L"0.0"; }
	virtual const wchar_t * defaultYmaxText () { return L"1.0"; }
	virtual const wchar_t * yminText () { return L"Minimum (0..1)"; }
	virtual const wchar_t * ymaxText () { return L"Maximum (0..1)"; }
	virtual const wchar_t * yminKey () { return L"Minimum"; }
	virtual const wchar_t * ymaxKey () { return L"Maximum"; }
};

class KlattGrid_collisionPhaseTierEditor : public KlattGrid_intensityTierEditor {
  public:
	KlattGrid_collisionPhaseTierEditor (GuiObject parent, const wchar_t *title, KlattGrid klattgrid);

	virtual const wchar_t * type () { return L"KlattGrid_collisionPhaseTierEditor"; }
	virtual double minimumLegalValue () { return 0; }
	virtual double maximumLegalValue () { return 1; }
	virtual const wchar_t * quantityText () { return L"Collision phase (0..1)"; }
	virtual const wchar_t * quantityKey () { return L"Collision phase"; }
	virtual const wchar_t * rightTickUnits () { return L""; }
	virtual double defaultYmin () { return 0; }
	virtual double defaultYmax () { return 1; }
	virtual const wchar_t * setRangeTitle () { return L"Set collision phase range..."; }
	virtual const wchar_t * defaultYminText () { return L"0.0"; }
	virtual const wchar_t * defaultYmaxText () { return L"1.0"; }
	virtual const wchar_t * yminText () { return L"Minimum (0..1)"; }
	virtual const wchar_t * ymaxText () { return L"Maximum (0..1)"; }
	virtual const wchar_t * yminKey () { return L"Minimum"; }
	virtual const wchar_t * ymaxKey () { return L"Maximum"; }
};

class KlattGrid_power1TierEditor : public KlattGrid_intensityTierEditor {
  public:
	KlattGrid_power1TierEditor (GuiObject parent, const wchar_t *title, KlattGrid klattgrid);

	virtual const wchar_t * type () { return L"KlattGrid_power1TierEditor"; }
	virtual double minimumLegalValue () { return 0; }
	virtual const wchar_t * quantityText () { return L"Power1"; }
	virtual const wchar_t * quantityKey () { return L"Power1"; }
	virtual const wchar_t * rightTickUnits () { return L""; }
	virtual double defaultYmin () { return 0; }
	virtual double defaultYmax () { return 4; }
	virtual const wchar_t * setRangeTitle () { return L"Set power1 range..."; }
	virtual const wchar_t * defaultYminText () { return L"0"; }
	virtual const wchar_t * defaultYmaxText () { return L"4"; }
	virtual const wchar_t * yminText () { return L"Minimum"; }
	virtual const wchar_t * ymaxText () { return L"Maximum"; }
	virtual const wchar_t * yminKey () { return L"Minimum"; }
	virtual const wchar_t * ymaxKey () { return L"Maximum"; }
};

class KlattGrid_power2TierEditor : public KlattGrid_intensityTierEditor {
  public:
	KlattGrid_power2TierEditor (GuiObject parent, const wchar_t *title, KlattGrid klattgrid);

	virtual const wchar_t * type () { return L"KlattGrid_power2TierEditor"; }
	virtual double minimumLegalValue () { return 0; }
	virtual const wchar_t * quantityText () { return L"Power2"; }
	virtual const wchar_t * quantityKey () { return L"Power2"; }
	virtual const wchar_t * rightTickUnits () { return L""; }
	virtual double defaultYmin () { return 0; }
	virtual double defaultYmax () { return 5; }
	virtual const wchar_t * setRangeTitle () { return L"Set power2 range..."; }
	virtual const wchar_t * defaultYminText () { return L"0"; }
	virtual const wchar_t * defaultYmaxText () { return L"5"; }
	virtual const wchar_t * yminText () { return L"Minimum"; }
	virtual const wchar_t * ymaxText () { return L"Maximum"; }
	virtual const wchar_t * yminKey () { return L"Minimum"; }
	virtual const wchar_t * ymaxKey () { return L"Maximum"; }
};

class KlattGrid_flutterTierEditor : public KlattGrid_intensityTierEditor {
  public:
	KlattGrid_flutterTierEditor (GuiObject parent, const wchar_t *title, KlattGrid klattgrid);

	virtual const wchar_t * type () { return L"KlattGrid_flutterTierEditor"; }
	virtual double minimumLegalValue () { return 0; }
	virtual double maximumLegalValue () { return 1; }
	virtual const wchar_t * quantityText () { return L"Flutter (0..1)"; }
	virtual const wchar_t * quantityKey () { return L"Flutter"; }
	virtual const wchar_t * rightTickUnits () { return L""; }
	virtual double defaultYmin () { return 0; }
	virtual double defaultYmax () { return 1; }
	virtual const wchar_t * setRangeTitle () { return L"Set flutter range..."; }
	virtual const wchar_t * defaultYminText () { return L"0.0"; }
	virtual const wchar_t * defaultYmaxText () { return L"1.0"; }
	virtual const wchar_t * yminText () { return L"Minimum (0..1)"; }
	virtual const wchar_t * ymaxText () { return L"Maximum (0..1)"; }
	virtual const wchar_t * yminKey () { return L"Minimum"; }
	virtual const wchar_t * ymaxKey () { return L"Maximum"; }
};

class KlattGrid_doublePulsingTierEditor : public KlattGrid_intensityTierEditor {
  public:
	KlattGrid_doublePulsingTierEditor (GuiObject parent, const wchar_t *title, KlattGrid klattgrid);

	virtual const wchar_t * type () { return L"KlattGrid_doublePulsingTierEditor"; }
	virtual double minimumLegalValue () { return 0; }
	virtual double maximumLegalValue () { return 1; }
	virtual const wchar_t * quantityText () { return L"Double pulsing (0..1)"; }
	virtual const wchar_t * quantityKey () { return L"Double pulsing"; }
	virtual const wchar_t * rightTickUnits () { return L""; }
	virtual double defaultYmin () { return 0; }
	virtual double defaultYmax () { return 1; }
	virtual const wchar_t * setRangeTitle () { return L"Set double pulsing range..."; }
	virtual const wchar_t * defaultYminText () { return L"0.0"; }
	virtual const wchar_t * defaultYmaxText () { return L"1.0"; }
	virtual const wchar_t * yminText () { return L"Minimum (0..1)"; }
	virtual const wchar_t * ymaxText () { return L"Maximum (0..1)"; }
	virtual const wchar_t * yminKey () { return L"Minimum"; }
	virtual const wchar_t * ymaxKey () { return L"Maximum"; }
};

class KlattGrid_formantGridEditor : public FormantGridEditor {
  public:
	KlattGrid_formantGridEditor (GuiObject parent, const wchar_t *title, KlattGrid data, int formantType);

	virtual bool hasSourceMenu () { return false; }
	virtual void play (double tmin, double tmax);

	KlattGrid _klattgrid;
};

#endif /* _KlattGridEditors_h_ */
