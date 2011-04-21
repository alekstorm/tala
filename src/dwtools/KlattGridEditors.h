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

#ifndef _KlattGrid_h_
	#include "KlattGrid.h"
#endif
#ifndef _PitchTier_h_
	#include "fon/PitchTier.h"
#endif
#ifndef _IntensityTier_h_
	#include "fon/IntensityTier.h"
#endif
#ifndef _RealTierEditor_h_
	#include "fon/RealTierEditor.h"
#endif
#ifndef _FormantGridEditor_h_
	#include "fon/FormantGridEditor.h"
#endif

class KlattGrid_realTierEditor : public RealTierEditor {
  public:
	KlattGrid_realTierEditor (GuiObject parent, const wchar_t *title, KlattGrid klattgrid, RealTier data);

	const wchar_t * type () { return L"KlattGrid_realTierEditor"; }

	void createHelpMenuItems (EditorMenu *menu);
	void play (double tmin, double tmax);

	KlattGrid _klattgrid;
};

class KlattGrid_pitchTierEditor : public KlattGrid_realTierEditor {
  public:
	KlattGrid_pitchTierEditor (GuiObject parent, const wchar_t *title, KlattGrid klattgrid);

	void createHelpMenuItems (EditorMenu *menu);

	const wchar_t * type () { return L"KlattGrid_pitchTierEditor"; }
	const wchar_t * quantityText () { return L"Frequency (Hz)"; }
	const wchar_t * quantityKey () { return L"Frequency (Hz)"; }
	const wchar_t * rightTickUnits () { return L" Hz"; }
	double defaultYmin () { return 50.0; }
	double defaultYmax () { return 600.0; }
	const wchar_t * setRangeTitle () { return L"Set frequency range..."; }
	const wchar_t * defaultYminText () { return L"50.0"; }
	const wchar_t * defaultYmaxText () { return L"600.0"; }
	const wchar_t * yminText () { return L"Minimum frequency (Hz)"; }
	const wchar_t * ymaxText () { return L"Maximum frequency (Hz)"; }
	const wchar_t * yminKey () { return L"Minimum frequency"; }
	const wchar_t * ymaxKey () { return L"Maximum frequency"; }
};

class KlattGrid_intensityTierEditor : public KlattGrid_realTierEditor {
  public:
	KlattGrid_intensityTierEditor (GuiObject parent, const wchar_t *title, KlattGrid klattgrid, RealTier tier);

	const wchar_t * type () { return L"KlattGrid_intensityTierEditor"; }
	const wchar_t * quantityText () { return L"Intensity (dB)"; }
	const wchar_t * quantityKey () { return L"Intensity"; }
	const wchar_t * rightTickUnits () { return L" dB"; }
	double defaultYmin () { return 50.0; }
	double defaultYmax () { return 100.0; }
	const wchar_t * setRangeTitle () { return L"Set intensity range..."; }
	const wchar_t * defaultYminText () { return L"50.0"; }
	const wchar_t * defaultYmaxText () { return L"100.0"; }
	const wchar_t * yminText () { return L"Minimum intensity (dB)"; }
	const wchar_t * ymaxText () { return L"Maximum intensity (dB)"; }
	const wchar_t * yminKey () { return L"Minimum intensity"; }
	const wchar_t * ymaxKey () { return L"Maximum intensity"; }

	void createHelpMenuItems (EditorMenu *menu);
};

class KlattGrid_decibelTierEditor : public KlattGrid_intensityTierEditor {
  public:
	KlattGrid_decibelTierEditor (GuiObject parent, const wchar_t *title, KlattGrid klattgrid, RealTier tier);

	const wchar_t * type () { return L"KlattGrid_decibelTierEditor"; }
	const wchar_t * quantityText () { return L"Amplitude (dB)"; }
	const wchar_t * quantityKey () { return L"Amplitude"; }
	const wchar_t * rightTickUnits () { return L" dB"; }
	double defaultYmin () { return -30.0; }
	double defaultYmax () { return 30.0; }
	const wchar_t * setRangeTitle () { return L"Set amplitude range..."; }
	const wchar_t * defaultYminText () { return L"-30.0"; }
	const wchar_t * defaultYmaxText () { return L"30.0"; }
	const wchar_t * yminText () { return L"Minimum amplitude (dB)"; }
	const wchar_t * ymaxText () { return L"Maximum amplitude (dB)"; }
	const wchar_t * yminKey () { return L"Minimum amplitude"; }
	const wchar_t * ymaxKey () { return L"Maximum amplitude"; }
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

	const wchar_t * type () { return L"KlattGrid_spectralTierEditor"; }
	double defaultYmin () { return -50.0; }
	double defaultYmax () { return 10.0; }
	const wchar_t * defaultYminText () { return L"-50.0"; }
	const wchar_t * defaultYmaxText () { return L"10.0"; }
};

class KlattGrid_fricationBypassTierEditor : public KlattGrid_intensityTierEditor {
  public:
	KlattGrid_fricationBypassTierEditor (GuiObject parent, const wchar_t *title, KlattGrid klattgrid);

	const wchar_t * type () { return L"KlattGrid_fricationBypassTierEditor"; }
	double defaultYmin () { return -50.0; }
	double defaultYmax () { return 10.0; }
	const wchar_t * defaultYminText () { return L"-50.0"; }
	const wchar_t * defaultYmaxText () { return L"10.0"; }
};

class KlattGrid_fricationAmplitudeTierEditor : public KlattGrid_intensityTierEditor {
  public:
	KlattGrid_fricationAmplitudeTierEditor (GuiObject parent, const wchar_t *title, KlattGrid klattgrid);
};

class KlattGrid_openPhaseTierEditor : public KlattGrid_intensityTierEditor {
  public:
	KlattGrid_openPhaseTierEditor (GuiObject parent, const wchar_t *title, KlattGrid klattgrid);

	const wchar_t * type () { return L"KlattGrid_openPhaseTierEditor"; }
	double minimumLegalValue () { return 0; }
	double maximumLegalValue () { return 1; }
	const wchar_t * quantityText () { return L"Open phase (0..1)"; }
	const wchar_t * quantityKey () { return L"Open phase"; }
	const wchar_t * rightTickUnits () { return L""; }
	double defaultYmin () { return 0; }
	double defaultYmax () { return 1; }
	const wchar_t * setRangeTitle () { return L"Set open phase range..."; }
	const wchar_t * defaultYminText () { return L"0.0"; }
	const wchar_t * defaultYmaxText () { return L"1.0"; }
	const wchar_t * yminText () { return L"Minimum (0..1)"; }
	const wchar_t * ymaxText () { return L"Maximum (0..1)"; }
	const wchar_t * yminKey () { return L"Minimum"; }
	const wchar_t * ymaxKey () { return L"Maximum"; }
};

class KlattGrid_collisionPhaseTierEditor : public KlattGrid_intensityTierEditor {
  public:
	KlattGrid_collisionPhaseTierEditor (GuiObject parent, const wchar_t *title, KlattGrid klattgrid);

	const wchar_t * type () { return L"KlattGrid_collisionPhaseTierEditor"; }
	double minimumLegalValue () { return 0; }
	double maximumLegalValue () { return 1; }
	const wchar_t * quantityText () { return L"Collision phase (0..1)"; }
	const wchar_t * quantityKey () { return L"Collision phase"; }
	const wchar_t * rightTickUnits () { return L""; }
	double defaultYmin () { return 0; }
	double defaultYmax () { return 1; }
	const wchar_t * setRangeTitle () { return L"Set collision phase range..."; }
	const wchar_t * defaultYminText () { return L"0.0"; }
	const wchar_t * defaultYmaxText () { return L"1.0"; }
	const wchar_t * yminText () { return L"Minimum (0..1)"; }
	const wchar_t * ymaxText () { return L"Maximum (0..1)"; }
	const wchar_t * yminKey () { return L"Minimum"; }
	const wchar_t * ymaxKey () { return L"Maximum"; }
};

class KlattGrid_power1TierEditor : public KlattGrid_intensityTierEditor {
  public:
	KlattGrid_power1TierEditor (GuiObject parent, const wchar_t *title, KlattGrid klattgrid);

	const wchar_t * type () { return L"KlattGrid_power1TierEditor"; }
	double minimumLegalValue () { return 0; }
	const wchar_t * quantityText () { return L"Power1"; }
	const wchar_t * quantityKey () { return L"Power1"; }
	const wchar_t * rightTickUnits () { return L""; }
	double defaultYmin () { return 0; }
	double defaultYmax () { return 4; }
	const wchar_t * setRangeTitle () { return L"Set power1 range..."; }
	const wchar_t * defaultYminText () { return L"0"; }
	const wchar_t * defaultYmaxText () { return L"4"; }
	const wchar_t * yminText () { return L"Minimum"; }
	const wchar_t * ymaxText () { return L"Maximum"; }
	const wchar_t * yminKey () { return L"Minimum"; }
	const wchar_t * ymaxKey () { return L"Maximum"; }
};

class KlattGrid_power2TierEditor : public KlattGrid_intensityTierEditor {
  public:
	KlattGrid_power2TierEditor (GuiObject parent, const wchar_t *title, KlattGrid klattgrid);

	const wchar_t * type () { return L"KlattGrid_power2TierEditor"; }
	double minimumLegalValue () { return 0; }
	const wchar_t * quantityText () { return L"Power2"; }
	const wchar_t * quantityKey () { return L"Power2"; }
	const wchar_t * rightTickUnits () { return L""; }
	double defaultYmin () { return 0; }
	double defaultYmax () { return 5; }
	const wchar_t * setRangeTitle () { return L"Set power2 range..."; }
	const wchar_t * defaultYminText () { return L"0"; }
	const wchar_t * defaultYmaxText () { return L"5"; }
	const wchar_t * yminText () { return L"Minimum"; }
	const wchar_t * ymaxText () { return L"Maximum"; }
	const wchar_t * yminKey () { return L"Minimum"; }
	const wchar_t * ymaxKey () { return L"Maximum"; }
};

class KlattGrid_flutterTierEditor : public KlattGrid_intensityTierEditor {
  public:
	KlattGrid_flutterTierEditor (GuiObject parent, const wchar_t *title, KlattGrid klattgrid);

	const wchar_t * type () { return L"KlattGrid_flutterTierEditor"; }
	double minimumLegalValue () { return 0; }
	double maximumLegalValue () { return 1; }
	const wchar_t * quantityText () { return L"Flutter (0..1)"; }
	const wchar_t * quantityKey () { return L"Flutter"; }
	const wchar_t * rightTickUnits () { return L""; }
	double defaultYmin () { return 0; }
	double defaultYmax () { return 1; }
	const wchar_t * setRangeTitle () { return L"Set flutter range..."; }
	const wchar_t * defaultYminText () { return L"0.0"; }
	const wchar_t * defaultYmaxText () { return L"1.0"; }
	const wchar_t * yminText () { return L"Minimum (0..1)"; }
	const wchar_t * ymaxText () { return L"Maximum (0..1)"; }
	const wchar_t * yminKey () { return L"Minimum"; }
	const wchar_t * ymaxKey () { return L"Maximum"; }
};

class KlattGrid_doublePulsingTierEditor : public KlattGrid_intensityTierEditor {
  public:
	KlattGrid_doublePulsingTierEditor (GuiObject parent, const wchar_t *title, KlattGrid klattgrid);

	const wchar_t * type () { return L"KlattGrid_doublePulsingTierEditor"; }
	double minimumLegalValue () { return 0; }
	double maximumLegalValue () { return 1; }
	const wchar_t * quantityText () { return L"Double pulsing (0..1)"; }
	const wchar_t * quantityKey () { return L"Double pulsing"; }
	const wchar_t * rightTickUnits () { return L""; }
	double defaultYmin () { return 0; }
	double defaultYmax () { return 1; }
	const wchar_t * setRangeTitle () { return L"Set double pulsing range..."; }
	const wchar_t * defaultYminText () { return L"0.0"; }
	const wchar_t * defaultYmaxText () { return L"1.0"; }
	const wchar_t * yminText () { return L"Minimum (0..1)"; }
	const wchar_t * ymaxText () { return L"Maximum (0..1)"; }
	const wchar_t * yminKey () { return L"Minimum"; }
	const wchar_t * ymaxKey () { return L"Maximum"; }
};

class KlattGrid_formantGridEditor : public FormantGridEditor {
  public:
	KlattGrid_formantGridEditor (GuiObject parent, const wchar_t *title, KlattGrid data, int formantType);

	bool hasSourceMenu () { return false; }
	void play (double tmin, double tmax);

	KlattGrid _klattgrid;
};

#endif /* _KlattGridEditors_h_ */
