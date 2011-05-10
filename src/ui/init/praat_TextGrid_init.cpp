/* praat_TextGrid_init.c
 *
 * Copyright (C) 1992-2010 Paul Boersma
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
 * pb 2010/12/08
 */

#include "fon/Pitch_AnyTier_to_PitchTier.h"
#include "ui/editors/SpectrumEditor.h"
#include "fon/SpellingChecker.h"
#include "ui/editors/TextGridEditor.h"
#include "fon/TextGrid_Sound.h"
#include "fon/WordList.h"
#include "dwtools/TextGrid_extensions.h"
#include "ui/UiFile.h"
#include "fon/Pitch_to_PitchTier.h"

#include "ui/praat.h"

void praat_dia_timeRange (UiForm *dia);
void praat_get_timeRange (UiForm *dia, double *tmin, double *tmax);
int praat_get_frequencyRange (UiForm *dia, double *fmin, double *fmax);

static const wchar_t *STRING_FROM_FREQUENCY_HZ = L"left Frequency range (Hz)";
static const wchar_t *STRING_TO_FREQUENCY_HZ = L"right Frequency range (Hz)";
static const wchar_t *STRING_TIER_NUMBER = L"Tier number";
static const wchar_t *STRING_INTERVAL_NUMBER = L"Interval number";
static const wchar_t *STRING_POINT_NUMBER = L"Point number";

void praat_TimeFunction_modify_init (void *klas);   // Modify buttons for time-based subclasses of Function.

/***** ANYTIER (generic) *****/

DIRECT (AnyTier_into_TextGrid)
	TextGrid grid = TextGrid_createWithoutTiers (1e30, -1e30);
	if (grid == NULL) return 0;
	WHERE (SELECTED) if (! TextGrid_add (grid, OBJECT)) { forget (grid); return 0; }
	if (! praat_new1 (grid, L"grid")) return 0;
END

/***** INTERVALTIER *****/

FORM (IntervalTier_downto_TableOfReal, L"IntervalTier: Down to TableOfReal", 0)
	SENTENCE (L"Label", L"ui/editors/AmplitudeTierEditor.h")
	OK
DO
	EVERY_TO (IntervalTier_downto_TableOfReal ((structIntervalTier *)OBJECT, GET_STRING (L"Label")))
END

DIRECT (IntervalTier_downto_TableOfReal_any)
	EVERY_TO (IntervalTier_downto_TableOfReal_any ((structIntervalTier *)OBJECT))
END

FORM (IntervalTier_getCentrePoints, L"IntervalTier: Get centre points", 0)
	SENTENCE (L"Text", L"ui/editors/AmplitudeTierEditor.h")
	OK
DO
	WHERE (SELECTED)
		if (! praat_new1 (IntervalTier_getCentrePoints ((structIntervalTier *)OBJECT, GET_STRING (L"Text")), GET_STRING (L"Text"))) return 0;
END

FORM (IntervalTier_getEndPoints, L"IntervalTier: Get end points", 0)
	SENTENCE (L"Text", L"ui/editors/AmplitudeTierEditor.h")
	OK
DO
	WHERE (SELECTED)
		if (! praat_new1 (IntervalTier_getEndPoints ((structIntervalTier *)OBJECT, GET_STRING (L"Text")), GET_STRING (L"Text"))) return 0;
END

FORM (IntervalTier_getStartingPoints, L"IntervalTier: Get starting points", 0)
	SENTENCE (L"Text", L"ui/editors/AmplitudeTierEditor.h")
	OK
DO
	WHERE (SELECTED)
		if (! praat_new1 (IntervalTier_getStartingPoints ((structIntervalTier *)OBJECT, GET_STRING (L"Text")), GET_STRING (L"Text"))) return 0;
END

DIRECT (IntervalTier_help) Melder_help (L"IntervalTier"); END

FORM_WRITE (IntervalTier_writeToXwaves, L"Xwaves label file", 0, 0)
	if (! IntervalTier_writeToXwaves ((structIntervalTier *)ONLY_OBJECT, file)) return 0;
END

/***** INTERVALTIER & POINTPROCESS *****/

FORM (IntervalTier_PointProcess_endToCentre, L"From end to centre", L"IntervalTier & PointProcess: End to centre...")
	REAL (L"Phase (0-1)", L"0.5")
	OK
DO
	IntervalTier tier = (structIntervalTier *)ONLY (classIntervalTier);
	PointProcess point = (structPointProcess *)ONLY (classPointProcess);
	double phase = GET_REAL (L"Phase");
	if (! praat_new5 (IntervalTier_PointProcess_endToCentre (tier, point, phase),
		tier -> name, L"_", point -> name, L"_", Melder_integer ((long) (100 * phase)))) return 0;
END

FORM (IntervalTier_PointProcess_startToCentre, L"From start to centre", L"IntervalTier & PointProcess: Start to centre...")
	REAL (L"Phase (0-1)", L"0.5")
	OK
DO
	IntervalTier tier = (structIntervalTier *)ONLY (classIntervalTier);
	PointProcess point = (structPointProcess *)ONLY (classPointProcess);
	double phase = GET_REAL (L"Phase");
	if (! praat_new5 (IntervalTier_PointProcess_startToCentre (tier, point, phase),
		tier -> name, L"_", point -> name, L"_", Melder_integer ((long) (100 * phase)))) return 0;
END

/***** LABEL (obsolete) *****/

DIRECT (Label_Sound_to_TextGrid)
	Label label = (structLabel *)ONLY (classLabel);
	Sound sound = (structSound *)ONLY (classSound);
	if (! praat_new1 (Label_Function_to_TextGrid (label, sound), sound -> name)) return 0;
END

DIRECT (info_Label_Sound_to_TextGrid)
	Melder_information1 (L"This is an old-style Label object. To turn it into a TextGrid, L"
		"select it together with a Sound of the appropriate duration, and click \"To TextGrid\".");
END

/***** SOUND & TEXTGRID *****/

void TextGrid_Sound_draw (TextGrid me, Sound sound, Graphics g, double tmin, double tmax,
	int showBoundaries, int useTextStyles, int garnish)   // STEREO BUG
{
	long first, last;
	int itier, ntier = my tiers -> size;

	/*
	 * Automatic windowing:
	 */
	if (tmax <= tmin) tmin = my xmin, tmax = my xmax;

	Graphics_setInner (g);
	Graphics_setWindow (g, tmin, tmax, -1.0 - 0.5 * ntier, 1.0);

	/*
	 * Draw sound in upper part.
	 */
	if (sound && Sampled_getWindowSamples (sound, tmin, tmax, & first, & last) > 1) {
		Graphics_setLineType (g, Graphics_DOTTED);
		Graphics_line (g, tmin, 0.0, tmax, 0.0);
		Graphics_setLineType (g, Graphics_DRAWN);      
		Graphics_function (g, sound -> z [1], first, last,
			Sampled_indexToX (sound, first), Sampled_indexToX (sound, last));
	}

	/*
	 * Draw labels in lower part.
	 */
	Graphics_setTextAlignment (g, Graphics_CENTRE, Graphics_HALF);
	Graphics_setPercentSignIsItalic (g, useTextStyles);
	Graphics_setNumberSignIsBold (g, useTextStyles);
	Graphics_setCircumflexIsSuperscript (g, useTextStyles);
	Graphics_setUnderscoreIsSubscript (g, useTextStyles);
	for (itier = 1; itier <= ntier; itier ++) {
		Data anyTier = (structData *)my tiers -> item [itier];
		double ymin = -1.0 - 0.5 * itier, ymax = ymin + 0.5;
		Graphics_rectangle (g, tmin, tmax, ymin, ymax);
		if (anyTier -> methods == (Data_Table) classIntervalTier) {
			IntervalTier tier = (IntervalTier) anyTier;
			long iinterval, ninterval = tier -> intervals -> size;
			for (iinterval = 1; iinterval <= ninterval; iinterval ++) {
				TextInterval interval = (structTextInterval *)tier -> intervals -> item [iinterval];
				double intmin = interval -> xmin, intmax = interval -> xmax;
				if (intmin < tmin) intmin = tmin;
				if (intmax > tmax) intmax = tmax;
				if (intmin >= intmax) continue;
				if (showBoundaries && intmin > tmin && intmin < tmax) {
					Graphics_setLineType (g, Graphics_DOTTED);
					Graphics_line (g, intmin, -1.0, intmin, 1.0);   /* In sound part. */
					Graphics_setLineType (g, Graphics_DRAWN);
				}      
				/* Draw left boundary. */
				if (intmin > tmin && intmin < tmax) Graphics_line (g, intmin, ymin, intmin, ymax);
				/* Draw label text. */
				if (interval -> text && intmax >= tmin && intmin <= tmax) {
					double t1 = tmin > intmin ? tmin : intmin;
					double t2 = tmax < intmax ? tmax : intmax;
					Graphics_text (g, 0.5 * (t1 + t2), 0.5 * (ymin + ymax), interval -> text);
				}
			}
		} else {
			TextTier tier = (TextTier) anyTier;
			long i, n = tier -> points -> size;
			for (i = 1; i <= n; i ++) {
				TextPoint point = (structTextPoint *)tier -> points -> item [i];
				double t = point -> time;
				if (t > tmin && t < tmax) {
					if (showBoundaries) {
						Graphics_setLineType (g, Graphics_DOTTED);
						Graphics_line (g, t, -1.0, t, 1.0);   /* In sound part. */
						Graphics_setLineType (g, Graphics_DRAWN);
					}
					Graphics_line (g, t, ymin, t, 0.8 * ymin + 0.2 * ymax);
					Graphics_line (g, t, 0.2 * ymin + 0.8 * ymax, t, ymax);
					if (point -> mark)
						Graphics_text (g, t, 0.5 * (ymin + ymax), point -> mark);
				}
			}
		}
	}
	Graphics_setPercentSignIsItalic (g, TRUE);
	Graphics_setNumberSignIsBold (g, TRUE);
	Graphics_setCircumflexIsSuperscript (g, TRUE);
	Graphics_setUnderscoreIsSubscript (g, TRUE);
	Graphics_unsetInner (g);
	if (garnish) {
		Graphics_drawInnerBox (g);
		Graphics_textBottom (g, 1, L"Time (s)");
		Graphics_marksBottom (g, 2, 1, 1, 1);
	}
}

FORM (TextGrid_Sound_draw, L"TextGrid & Sound: Draw...", 0)
	praat_dia_timeRange (dia);
	BOOLEAN (L"Show boundaries", 1)
	BOOLEAN (L"Use text styles", 1)
	BOOLEAN (L"Garnish", 1)
	OK
DO
	praat_picture_open ();
	TextGrid_Sound_draw ((structTextGrid *)ONLY (classTextGrid), (structSound *)ONLY (classSound), GRAPHICS,
		GET_REAL (L"left Time range"), GET_REAL (L"right Time range"), GET_INTEGER (L"Show boundaries"),
		GET_INTEGER (L"Use text styles"), GET_INTEGER (L"Garnish"));
	praat_picture_close ();
END

FORM (TextGrid_Sound_extractAllIntervals, L"TextGrid & Sound: Extract all intervals", 0)
	INTEGER (STRING_TIER_NUMBER, L"1")
	BOOLEAN (L"Preserve times", 0)
	OK
DO
	if (! praat_new1 (TextGrid_Sound_extractAllIntervals ((structTextGrid *)ONLY (classTextGrid), (structSound *)ONLY (classSound),
		GET_INTEGER (STRING_TIER_NUMBER), GET_INTEGER (L"Preserve times")), L"dummy")) return 0;
END

FORM (TextGrid_Sound_extractNonemptyIntervals, L"TextGrid & Sound: Extract non-empty intervals", 0)
	INTEGER (STRING_TIER_NUMBER, L"1")
	BOOLEAN (L"Preserve times", 0)
	OK
DO
	if (! praat_new1 (TextGrid_Sound_extractNonemptyIntervals ((structTextGrid *)(classTextGrid), (structSound *)ONLY (classSound),
		GET_INTEGER (STRING_TIER_NUMBER), GET_INTEGER (L"Preserve times")), L"dummy")) return 0;
END

FORM (TextGrid_Sound_extractIntervals, L"TextGrid & Sound: Extract intervals", 0)
	INTEGER (STRING_TIER_NUMBER, L"1")
	BOOLEAN (L"Preserve times", 0)
	SENTENCE (L"Label text", L"ui/editors/AmplitudeTierEditor.h")
	OK
DO
	if (! praat_new1 (TextGrid_Sound_extractIntervalsWhere ((structTextGrid *)ONLY (classTextGrid), (structSound *)ONLY (classSound),
		GET_INTEGER (STRING_TIER_NUMBER), kMelder_string_EQUAL_TO, GET_STRING (L"Label text"),
		GET_INTEGER (L"Preserve times")), GET_STRING (L"Label text"))) return 0;
END

FORM (TextGrid_Sound_extractIntervalsWhere, L"TextGrid & Sound: Extract intervals", 0)
	INTEGER (STRING_TIER_NUMBER, L"1")
	BOOLEAN (L"Preserve times", 0)
	OPTIONMENU_ENUM (L"Extract all intervals whose label...", kMelder_string, DEFAULT)
	SENTENCE (L"...the text", L"ui/editors/AmplitudeTierEditor.h")
	OK
DO
	if (! praat_new1 (TextGrid_Sound_extractIntervalsWhere ((structTextGrid *)ONLY (classTextGrid), (structSound *)ONLY (classSound),
		GET_INTEGER (STRING_TIER_NUMBER),
		GET_ENUM (kMelder_string, L"Extract all intervals whose label..."),
		GET_STRING (L"...the text"),
		GET_INTEGER (L"Preserve times")), GET_STRING (L"...the text"))) return 0;
END

DIRECT (TextGrid_Sound_scaleTimes)
	TextGrid grid = (structTextGrid *)ONLY (classTextGrid);
	Sound sound = (structSound *)ONLY (classSound);
	Function_scaleXTo (grid, sound -> xmin, sound -> xmax);
	praat_dataChanged (grid);
END

DIRECT (TextGrid_Sound_cloneTimeDomain)
	TextGrid grid = (structTextGrid *)ONLY (classTextGrid);
	Sound sound = (structSound *)ONLY (classSound);
	sound -> x1 += grid -> xmin - sound -> xmin;
	sound -> xmin = grid -> xmin;
	sound -> xmax = grid -> xmax;
	praat_dataChanged (sound);
END

/***** PITCH & TEXTGRID *****/

static void autoMarks (Graphics g, double ymin, double ymax, int haveDottedLines) {
	double dy = ymax - ymin;
	if (dy < 26) {
		long imin = ceil ((ymin + 2.0) / 5.0), imax = floor ((ymax - 2.0) / 5.0), i;
		for (i = imin; i <= imax; i ++)
			Graphics_markLeft (g, i * 5, TRUE, TRUE, haveDottedLines, NULL);
	} else if (dy < 110) {
		long imin = ceil ((ymin + 8.0) / 20.0), imax = floor ((ymax - 8.0) / 20.0), i;
		for (i = imin; i <= imax; i ++)
			Graphics_markLeft (g, i * 20, TRUE, TRUE, haveDottedLines, NULL);
	} else if (dy < 260) {
		long imin = ceil ((ymin + 20.0) / 50.0), imax = floor ((ymax - 20.0) / 50.0), i;
		for (i = imin; i <= imax; i ++)
			Graphics_markLeft (g, i * 50, TRUE, TRUE, haveDottedLines, NULL);
	} else if (dy < 510) {
		long imin = ceil ((ymin + 40.0) / 100.0), imax = floor ((ymax - 40.0) / 100.0), i;
		for (i = imin; i <= imax; i ++)
			Graphics_markLeft (g, i * 100, TRUE, TRUE, haveDottedLines, NULL);
	}
}

static void autoMarks_logarithmic (Graphics g, double ymin, double ymax, int haveDottedLines) {
	double fy = ymax / ymin;
	int i;
	for (i = -12; i <= 12; i ++) {
		double power = pow (10, i), y = power;
		if (y > ymin * 1.2 && y < ymax / 1.2)
			Graphics_markLeftLogarithmic (g, y, TRUE, TRUE, haveDottedLines, NULL);
		if (fy > 2100) {
			;   /* Enough. */
		} else if (fy > 210) {
			y = 3.0 * power;
			if (y > ymin * 1.2 && y < ymax / 1.2)
				Graphics_markLeftLogarithmic (g, y, TRUE, TRUE, haveDottedLines, NULL);
		} else {
			y = 2.0 * power;
			if (y > ymin * 1.2 && y < ymax / 1.2)
				Graphics_markLeftLogarithmic (g, y, TRUE, TRUE, haveDottedLines, NULL);
			y = 5.0 * power;
			if (y > ymin * 1.2 && y < ymax / 1.2)
				Graphics_markLeftLogarithmic (g, y, TRUE, TRUE, haveDottedLines, NULL);
			if (fy < 21) {
				y = 3.0 * power;
				if (y > ymin * 1.2 && y < ymax / 1.2)
					Graphics_markLeftLogarithmic (g, y, TRUE, TRUE, haveDottedLines, NULL);
				y = 7.0 * power;
				if (y > ymin * 1.2 && y < ymax / 1.2)
					Graphics_markLeftLogarithmic (g, y, TRUE, TRUE, haveDottedLines, NULL);
			}
			if (fy < 4.1) {
				y = 1.5 * power;
				if (y > ymin * 1.2 && y < ymax / 1.2)
					Graphics_markLeftLogarithmic (g, y, TRUE, TRUE, haveDottedLines, NULL);
				y = 4.0 * power;
				if (y > ymin * 1.2 && y < ymax / 1.2)
					Graphics_markLeftLogarithmic (g, y, TRUE, TRUE, haveDottedLines, NULL);
			}
		}
	}
}

static void autoMarks_semitones (Graphics g, double ymin, double ymax, int haveDottedLines) {
	double dy = ymax - ymin;
	if (dy < 16) {
		long imin = ceil ((ymin + 1.2) / 3.0), imax = floor ((ymax - 1.2) / 3.0), i;
		for (i = imin; i <= imax; i ++)
			Graphics_markLeft (g, i * 3, TRUE, TRUE, haveDottedLines, NULL);
	} else if (dy < 32) {
		long imin = ceil ((ymin + 2.4) / 6.0), imax = floor ((ymax - 2.4) / 6.0), i;
		for (i = imin; i <= imax; i ++)
			Graphics_markLeft (g, i * 6, TRUE, TRUE, haveDottedLines, NULL);
	} else if (dy < 64) {
		long imin = ceil ((ymin + 4.8) / 12.0), imax = floor ((ymax - 4.8) / 12.0), i;
		for (i = imin; i <= imax; i ++)
			Graphics_markLeft (g, i * 12, TRUE, TRUE, haveDottedLines, NULL);
	} else if (dy < 128) {
		long imin = ceil ((ymin + 9.6) / 24.0), imax = floor ((ymax - 9.6) / 24.0), i;
		for (i = imin; i <= imax; i ++)
			Graphics_markLeft (g, i * 24, TRUE, TRUE, haveDottedLines, NULL);
	}
}

void Pitch_draw (Pitch me, Graphics g, double tmin, double tmax, double fmin, double fmax, int garnish, int speckle, int unit);

void TextGrid_Pitch_drawSeparately (TextGrid grid, Pitch pitch, Graphics g, double tmin, double tmax,
	double fmin, double fmax, int showBoundaries, int useTextStyles, int garnish, int speckle, int unit)
{
	int ntier = grid -> tiers -> size;
	if (tmax <= tmin) tmin = grid -> xmin, tmax = grid -> xmax;
	if (ClassFunction_isUnitLogarithmic (classPitch, Pitch_LEVEL_FREQUENCY, unit)) {
		fmin = ClassFunction_convertStandardToSpecialUnit (classPitch, fmin, Pitch_LEVEL_FREQUENCY, unit);
		fmax = ClassFunction_convertStandardToSpecialUnit (classPitch, fmax, Pitch_LEVEL_FREQUENCY, unit);
	}
	if (unit == kPitch_unit_HERTZ_LOGARITHMIC)
		Pitch_draw (pitch, g, tmin, tmax, pow (10, fmin - 0.25 * (fmax - fmin) * ntier), pow (10, fmax), FALSE, speckle, unit);
	else
		Pitch_draw (pitch, g, tmin, tmax, fmin - 0.25 * (fmax - fmin) * ntier, fmax, FALSE, speckle, unit);
	TextGrid_Sound_draw (grid, NULL, g, tmin, tmax, showBoundaries, useTextStyles, FALSE);
	/*
	 * Restore window for the sake of margin drawing.
	 */
	Graphics_setWindow (g, tmin, tmax, fmin - 0.25 * (fmax - fmin) * ntier, fmax);
	if (unit == kPitch_unit_HERTZ_LOGARITHMIC)
		fmin = pow (10, fmin), fmax = pow (10, fmax);
	if (garnish) {
		Graphics_drawInnerBox (g);
		if (unit == kPitch_unit_HERTZ_LOGARITHMIC) {
			Graphics_markLeftLogarithmic (g, fmin, TRUE, TRUE, FALSE, NULL);
			Graphics_markLeftLogarithmic (g, fmax, TRUE, TRUE, FALSE, NULL);
			autoMarks_logarithmic (g, fmin, fmax, FALSE);
		} else if (unit == kPitch_unit_SEMITONES_100) {
			Graphics_markLeft (g, fmin, TRUE, TRUE, FALSE, NULL);
			Graphics_markLeft (g, fmax, TRUE, TRUE, FALSE, NULL);
			autoMarks_semitones (g, fmin, fmax, FALSE);
		} else {
			Graphics_markLeft (g, fmin, TRUE, TRUE, FALSE, NULL);
			Graphics_markLeft (g, fmax, TRUE, TRUE, FALSE, NULL);
			autoMarks (g, fmin, fmax, FALSE);
		}
		static MelderString buffer = { 0 };
		MelderString_empty (& buffer);
		MelderString_append3 (& buffer, L"Pitch (", ClassFunction_getUnitText (classPitch, Pitch_LEVEL_FREQUENCY, unit, Function_UNIT_TEXT_GRAPHICAL), L")");
		Graphics_textLeft (g, true, buffer.string);
		Graphics_textBottom (g, true, L"Time (s)");
		Graphics_marksBottom (g, 2, true, true, true);
	}
}

void TextGrid_Pitch_draw (TextGrid grid, Pitch pitch, Graphics g,
	long itier, double tmin, double tmax, double fmin, double fmax,
	double fontSize, int useTextStyles, int horizontalAlignment, int garnish, int speckle, int unit)
{
	Data anyTier;
	long i;
	PitchTier pitchTier = NULL;
	double oldFontSize = Graphics_inqFontSize (g);
	Pitch_draw (pitch, g, tmin, tmax, fmin, fmax, garnish, speckle, unit);
	if (tmax <= tmin) tmin = grid -> xmin, tmax = grid -> xmax;
	if (itier < 1 || itier > grid -> tiers -> size) goto end;
	pitchTier = Pitch_to_PitchTier (pitch);
	if (! pitchTier) goto end;
	if (ClassFunction_isUnitLogarithmic (classPitch, Pitch_LEVEL_FREQUENCY, unit)) {
		fmin = ClassFunction_convertStandardToSpecialUnit (classPitch, fmin, Pitch_LEVEL_FREQUENCY, unit);
		fmax = ClassFunction_convertStandardToSpecialUnit (classPitch, fmax, Pitch_LEVEL_FREQUENCY, unit);
	}
	Graphics_setTextAlignment (g, horizontalAlignment, Graphics_BOTTOM);
	Graphics_setInner (g);
	Graphics_setFontSize (g, fontSize);
	Graphics_setPercentSignIsItalic (g, useTextStyles);
	Graphics_setNumberSignIsBold (g, useTextStyles);
	Graphics_setCircumflexIsSuperscript (g, useTextStyles);
	Graphics_setUnderscoreIsSubscript (g, useTextStyles);
	anyTier = (structData *)grid -> tiers -> item [itier];
	if (anyTier -> methods == (Data_Table) classIntervalTier) {
		IntervalTier tier = (IntervalTier) anyTier;
		for (i = 1; i <= tier -> intervals -> size; i ++) {
			TextInterval interval = (structTextInterval *)tier -> intervals -> item [i];
			double tleft = interval -> xmin, tright = interval -> xmax, tmid, f0;
			if (! interval -> text || ! interval -> text [0]) continue;
			if (tleft < pitch -> xmin) tleft = pitch -> xmin;
			if (tright > pitch -> xmax) tright = pitch -> xmax;
			tmid = (tleft + tright) / 2;
			if (tmid < tmin || tmid > tmax) continue;
			f0 = ClassFunction_convertStandardToSpecialUnit (classPitch, RealTier_getValueAtTime (pitchTier, tmid), Pitch_LEVEL_FREQUENCY, unit);
			if (f0 < fmin || f0 > fmax) continue;
			Graphics_text (g,
				horizontalAlignment == Graphics_LEFT ? tleft : horizontalAlignment == Graphics_RIGHT ? tright : tmid,
				f0, interval -> text);
		}
	} else {
		TextTier tier = (TextTier) anyTier;
		for (i = 1; i <= tier -> points -> size; i ++) {
			TextPoint point = (structTextPoint *)tier -> points -> item [i];
			double t = point -> time, f0;
			if (! point -> mark || ! point -> mark [0]) continue;
			if (t < tmin || t > tmax) continue;
			f0 = ClassFunction_convertStandardToSpecialUnit (classPitch, RealTier_getValueAtTime (pitchTier, t), Pitch_LEVEL_FREQUENCY, unit);
			if (f0 < fmin || f0 > fmax) continue;
			Graphics_text (g, t, f0, point -> mark);
		}
	}
	Graphics_setPercentSignIsItalic (g, TRUE);
	Graphics_setNumberSignIsBold (g, TRUE);
	Graphics_setCircumflexIsSuperscript (g, TRUE);
	Graphics_setUnderscoreIsSubscript (g, TRUE);
	Graphics_setFontSize (g, oldFontSize);
	Graphics_unsetInner (g);
end:
	forget (pitchTier);
}

static int pr_TextGrid_Pitch_draw (UiForm *dia, int speckle, int unit) {
	double tmin, tmax, fmin, fmax;
	praat_get_timeRange (dia, & tmin, & tmax);
	if (! praat_get_frequencyRange (dia, & fmin, & fmax)) return 0;
	praat_picture_open ();
	TextGrid_Pitch_draw ((structTextGrid *)ONLY (classTextGrid), (structPitch *)ONLY (classPitch), GRAPHICS,
		GET_INTEGER (STRING_TIER_NUMBER), tmin, tmax, fmin, fmax, GET_INTEGER (L"Font size"),
		GET_INTEGER (L"Use text styles"), GET_INTEGER (L"Text alignment") - 1, GET_INTEGER (L"Garnish"), speckle, unit);
	praat_picture_close ();
	return 1;
}

FORM (TextGrid_Pitch_draw, L"TextGrid & Pitch: Draw", 0)
	INTEGER (STRING_TIER_NUMBER, L"1")
	praat_dia_timeRange (dia);
	REAL (STRING_FROM_FREQUENCY_HZ, L"0.0")
	POSITIVE (STRING_TO_FREQUENCY_HZ, L"500.0")
	INTEGER (L"Font size (points)", L"18")
	BOOLEAN (L"Use text styles", 1)
	OPTIONMENU (L"Text alignment", 2) OPTION (L"Left") OPTION (L"Centre") OPTION (L"Right")
	BOOLEAN (L"Garnish", 1)
	OK
DO
	if (! pr_TextGrid_Pitch_draw (dia, Pitch_speckle_NO, kPitch_unit_HERTZ)) return 0;
END

FORM (TextGrid_Pitch_drawErb, L"TextGrid & Pitch: Draw erb", 0)
	INTEGER (STRING_TIER_NUMBER, L"1")
	praat_dia_timeRange (dia);
	REAL (L"left Frequency range (ERB)", L"0")
	REAL (L"right Frequency range (ERB)", L"10.0")
	INTEGER (L"Font size (points)", L"18")
	BOOLEAN (L"Use text styles", 1)
	OPTIONMENU (L"Text alignment", 2) OPTION (L"Left") OPTION (L"Centre") OPTION (L"Right")
	BOOLEAN (L"Garnish", 1)
	OK
DO
	if (! pr_TextGrid_Pitch_draw (dia, Pitch_speckle_NO, kPitch_unit_ERB)) return 0;
END

FORM (TextGrid_Pitch_drawLogarithmic, L"TextGrid & Pitch: Draw logarithmic", 0)
	INTEGER (STRING_TIER_NUMBER, L"1")
	praat_dia_timeRange (dia);
	POSITIVE (STRING_FROM_FREQUENCY_HZ, L"50.0")
	POSITIVE (STRING_TO_FREQUENCY_HZ, L"500.0")
	INTEGER (L"Font size (points)", L"18")
	BOOLEAN (L"Use text styles", 1)
	OPTIONMENU (L"Text alignment", 2) OPTION (L"Left") OPTION (L"Centre") OPTION (L"Right")
	BOOLEAN (L"Garnish", 1)
	OK
DO
	if (! pr_TextGrid_Pitch_draw (dia, Pitch_speckle_NO, kPitch_unit_HERTZ_LOGARITHMIC)) return 0;
END

FORM (TextGrid_Pitch_drawMel, L"TextGrid & Pitch: Draw mel", 0)
	INTEGER (STRING_TIER_NUMBER, L"1")
	praat_dia_timeRange (dia);
	REAL (L"left Frequency range (mel)", L"0")
	REAL (L"right Frequency range (mel)", L"500")
	INTEGER (L"Font size (points)", L"18")
	BOOLEAN (L"Use text styles", 1)
	OPTIONMENU (L"Text alignment", 2) OPTION (L"Left") OPTION (L"Centre") OPTION (L"Right")
	BOOLEAN (L"Garnish", 1)
	OK
DO
	if (! pr_TextGrid_Pitch_draw (dia, Pitch_speckle_NO, kPitch_unit_MEL)) return 0;
END

FORM (TextGrid_Pitch_drawSemitones, L"TextGrid & Pitch: Draw semitones", 0)
	INTEGER (STRING_TIER_NUMBER, L"1")
	praat_dia_timeRange (dia);
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Range in semitones re 100 Hertz:")
	REAL (L"left Frequency range (st)", L"-12.0")
	REAL (L"right Frequency range (st)", L"30.0")
	INTEGER (L"Font size (points)", L"18")
	BOOLEAN (L"Use text styles", 1)
	OPTIONMENU (L"Text alignment", 2) OPTION (L"Left") OPTION (L"Centre") OPTION (L"Right")
	BOOLEAN (L"Garnish", 1)
	OK
DO
	if (! pr_TextGrid_Pitch_draw (dia, Pitch_speckle_NO, kPitch_unit_SEMITONES_100)) return 0;
END

static int pr_TextGrid_Pitch_drawSeparately (UiForm *dia, int speckle, int unit) {
	double tmin, tmax, fmin, fmax;
	praat_get_timeRange (dia, & tmin, & tmax);
	if (! praat_get_frequencyRange (dia, & fmin, & fmax)) return 0;
	praat_picture_open ();
	TextGrid_Pitch_drawSeparately ((structTextGrid *)ONLY (classTextGrid), (structPitch *)ONLY (classPitch), GRAPHICS,
		tmin, tmax, fmin, fmax, GET_INTEGER (L"Show boundaries"),
		GET_INTEGER (L"Use text styles"), GET_INTEGER (L"Garnish"), speckle, unit);
	praat_picture_close ();
	return 1;
}

FORM (TextGrid_Pitch_drawSeparately, L"TextGrid & Pitch: Draw separately", 0)
	praat_dia_timeRange (dia);
	REAL (STRING_FROM_FREQUENCY_HZ, L"0.0")
	REAL (STRING_TO_FREQUENCY_HZ, L"500.0")
	BOOLEAN (L"Show boundaries", 1)
	BOOLEAN (L"Use text styles", 1)
	BOOLEAN (L"Garnish", 1)
	OK
DO
	if (! pr_TextGrid_Pitch_drawSeparately (dia, Pitch_speckle_NO, kPitch_unit_HERTZ)) return 0;
END

FORM (TextGrid_Pitch_drawSeparatelyErb, L"TextGrid & Pitch: Draw separately erb", 0)
	praat_dia_timeRange (dia);
	REAL (L"left Frequency range (ERB)", L"0")
	REAL (L"right Frequency range (ERB)", L"10.0")
	BOOLEAN (L"Show boundaries", 1)
	BOOLEAN (L"Use text styles", 1)
	BOOLEAN (L"Garnish", 1)
	OK
DO
	if (! pr_TextGrid_Pitch_drawSeparately (dia, Pitch_speckle_NO, kPitch_unit_ERB)) return 0;
END

FORM (TextGrid_Pitch_drawSeparatelyLogarithmic, L"TextGrid & Pitch: Draw separately logarithmic", 0)
	praat_dia_timeRange (dia);
	POSITIVE (STRING_FROM_FREQUENCY_HZ, L"50.0")
	POSITIVE (STRING_TO_FREQUENCY_HZ, L"500.0")
	BOOLEAN (L"Show boundaries", 1)
	BOOLEAN (L"Use text styles", 1)
	BOOLEAN (L"Garnish", 1)
	OK
DO
	if (! pr_TextGrid_Pitch_drawSeparately (dia, Pitch_speckle_NO, kPitch_unit_HERTZ_LOGARITHMIC)) return 0;
END

FORM (TextGrid_Pitch_drawSeparatelyMel, L"TextGrid & Pitch: Draw separately mel", 0)
	praat_dia_timeRange (dia);
	REAL (L"left Frequency range (mel)", L"0")
	REAL (L"right Frequency range (mel)", L"500")
	BOOLEAN (L"Show boundaries", 1)
	BOOLEAN (L"Use text styles", 1)
	BOOLEAN (L"Garnish", 1)
	OK
DO
	if (! pr_TextGrid_Pitch_drawSeparately (dia, Pitch_speckle_NO, kPitch_unit_MEL)) return 0;
END

FORM (TextGrid_Pitch_drawSeparatelySemitones, L"TextGrid & Pitch: Draw separately semitones", 0)
	praat_dia_timeRange (dia);
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Range in semitones re 100 Hertz:")
	REAL (L"left Frequency range (st)", L"-12.0")
	REAL (L"right Frequency range (st)", L"30.0")
	BOOLEAN (L"Show boundaries", 1)
	BOOLEAN (L"Use text styles", 1)
	BOOLEAN (L"Garnish", 1)
	OK
DO
	if (! pr_TextGrid_Pitch_drawSeparately (dia, Pitch_speckle_NO, kPitch_unit_SEMITONES_100)) return 0;
END

FORM (TextGrid_Pitch_speckle, L"TextGrid & Pitch: Speckle", 0)
	INTEGER (STRING_TIER_NUMBER, L"1")
	praat_dia_timeRange (dia);
	REAL (STRING_FROM_FREQUENCY_HZ, L"0.0")
	POSITIVE (STRING_TO_FREQUENCY_HZ, L"500.0")
	INTEGER (L"Font size (points)", L"18")
	BOOLEAN (L"Use text styles", 1)
	OPTIONMENU (L"Text alignment", 2) OPTION (L"Left") OPTION (L"Centre") OPTION (L"Right")
	BOOLEAN (L"Garnish", 1)
	OK
DO
	if (! pr_TextGrid_Pitch_draw (dia, Pitch_speckle_YES, kPitch_unit_HERTZ)) return 0;
END

FORM (TextGrid_Pitch_speckleErb, L"TextGrid & Pitch: Speckle erb", 0)
	INTEGER (STRING_TIER_NUMBER, L"1")
	praat_dia_timeRange (dia);
	REAL (L"left Frequency range (ERB)", L"0")
	REAL (L"right Frequency range (ERB)", L"10.0")
	INTEGER (L"Font size (points)", L"18")
	BOOLEAN (L"Use text styles", 1)
	OPTIONMENU (L"Text alignment", 2) OPTION (L"Left") OPTION (L"Centre") OPTION (L"Right")
	BOOLEAN (L"Garnish", 1)
	OK
DO
	if (! pr_TextGrid_Pitch_draw (dia, Pitch_speckle_YES, kPitch_unit_ERB)) return 0;
END

FORM (TextGrid_Pitch_speckleLogarithmic, L"TextGrid & Pitch: Speckle logarithmic", 0)
	INTEGER (STRING_TIER_NUMBER, L"1")
	praat_dia_timeRange (dia);
	POSITIVE (STRING_FROM_FREQUENCY_HZ, L"50.0")
	POSITIVE (STRING_TO_FREQUENCY_HZ, L"500.0")
	INTEGER (L"Font size (points)", L"18")
	BOOLEAN (L"Use text styles", 1)
	OPTIONMENU (L"Text alignment", 2) OPTION (L"Left") OPTION (L"Centre") OPTION (L"Right")
	BOOLEAN (L"Garnish", 1)
	OK
DO
	if (! pr_TextGrid_Pitch_draw (dia, Pitch_speckle_YES, kPitch_unit_HERTZ_LOGARITHMIC)) return 0;
END

FORM (TextGrid_Pitch_speckleMel, L"TextGrid & Pitch: Speckle mel", 0)
	INTEGER (STRING_TIER_NUMBER, L"1")
	praat_dia_timeRange (dia);
	REAL (L"left Frequency range (mel)", L"0")
	REAL (L"right Frequency range (mel)", L"500")
	INTEGER (L"Font size (points)", L"18")
	BOOLEAN (L"Use text styles", 1)
	OPTIONMENU (L"Text alignment", 2) OPTION (L"Left") OPTION (L"Centre") OPTION (L"Right")
	BOOLEAN (L"Garnish", 1)
	OK
DO
	if (! pr_TextGrid_Pitch_draw (dia, Pitch_speckle_YES, kPitch_unit_MEL)) return 0;
END

FORM (TextGrid_Pitch_speckleSemitones, L"TextGrid & Pitch: Speckle semitones", 0)
	INTEGER (STRING_TIER_NUMBER, L"1")
	praat_dia_timeRange (dia);
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Range in semitones re 100 Hertz:")
	REAL (L"left Frequency range (st)", L"-12.0")
	REAL (L"right Frequency range (st)", L"30.0")
	INTEGER (L"Font size (points)", L"18")
	BOOLEAN (L"Use text styles", 1)
	OPTIONMENU (L"Text alignment", 2) OPTION (L"Left") OPTION (L"Centre") OPTION (L"Right")
	BOOLEAN (L"Garnish", 1)
	OK
DO
	if (! pr_TextGrid_Pitch_draw (dia, Pitch_speckle_YES, kPitch_unit_SEMITONES_100)) return 0;
END

FORM (TextGrid_Pitch_speckleSeparately, L"TextGrid & Pitch: Speckle separately", 0)
	praat_dia_timeRange (dia);
	REAL (STRING_FROM_FREQUENCY_HZ, L"0.0")
	REAL (STRING_TO_FREQUENCY_HZ, L"500.0")
	BOOLEAN (L"Show boundaries", 1)
	BOOLEAN (L"Use text styles", 1)
	BOOLEAN (L"Garnish", 1)
	OK
DO
	if (! pr_TextGrid_Pitch_drawSeparately (dia, Pitch_speckle_YES, kPitch_unit_HERTZ)) return 0;
END

FORM (TextGrid_Pitch_speckleSeparatelyErb, L"TextGrid & Pitch: Speckle separately erb", 0)
	praat_dia_timeRange (dia);
	REAL (L"left Frequency range (ERB)", L"0")
	REAL (L"right Frequency range (ERB)", L"10.0")
	BOOLEAN (L"Show boundaries", 1)
	BOOLEAN (L"Use text styles", 1)
	BOOLEAN (L"Garnish", 1)
	OK
DO
	if (! pr_TextGrid_Pitch_drawSeparately (dia, Pitch_speckle_YES, kPitch_unit_ERB)) return 0;
END

FORM (TextGrid_Pitch_speckleSeparatelyLogarithmic, L"TextGrid & Pitch: Speckle separately logarithmic", 0)
	praat_dia_timeRange (dia);
	POSITIVE (STRING_FROM_FREQUENCY_HZ, L"50.0")
	POSITIVE (STRING_TO_FREQUENCY_HZ, L"500.0")
	BOOLEAN (L"Show boundaries", 1)
	BOOLEAN (L"Use text styles", 1)
	BOOLEAN (L"Garnish", 1)
	OK
DO
	if (! pr_TextGrid_Pitch_drawSeparately (dia, Pitch_speckle_YES, kPitch_unit_HERTZ_LOGARITHMIC)) return 0;
END

FORM (TextGrid_Pitch_speckleSeparatelyMel, L"TextGrid & Pitch: Speckle separately mel", 0)
	praat_dia_timeRange (dia);
	REAL (L"left Frequency range (mel)", L"0")
	REAL (L"right Frequency range (mel)", L"500")
	BOOLEAN (L"Show boundaries", 1)
	BOOLEAN (L"Use text styles", 1)
	BOOLEAN (L"Garnish", 1)
	OK
DO
	if (! pr_TextGrid_Pitch_drawSeparately (dia, Pitch_speckle_YES, kPitch_unit_MEL)) return 0;
END

FORM (TextGrid_Pitch_speckleSeparatelySemitones, L"TextGrid & Pitch: Speckle separately semitones", 0)
	praat_dia_timeRange (dia);
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Range in semitones re 100 Hertz:")
	REAL (L"left Frequency range (st)", L"-12.0")
	REAL (L"right Frequency range (st)", L"30.0")
	BOOLEAN (L"Show boundaries", 1)
	BOOLEAN (L"Use text styles", 1)
	BOOLEAN (L"Garnish", 1)
	OK
DO
	if (! pr_TextGrid_Pitch_drawSeparately (dia, Pitch_speckle_YES, kPitch_unit_SEMITONES_100)) return 0;
END

/***** PITCH & TEXTTIER *****/

FORM (Pitch_TextTier_to_PitchTier, L"Pitch & TextTier to PitchTier", L"Pitch & TextTier: To PitchTier...")
	RADIO (L"Unvoiced strategy", 3)
		RADIOBUTTON (L"Zero")
		RADIOBUTTON (L"Error")
		RADIOBUTTON (L"Interpolate")
	OK
DO
	if (! praat_new1 (Pitch_AnyTier_to_PitchTier ((structPitch *)ONLY (classPitch), (structAnyTier *)ONLY (classTextTier),
		GET_INTEGER (L"Unvoiced strategy") - 1), ((Pitch) (ONLY (classPitch))) -> name)) return 0;
END

/***** SPELLINGCHECKER *****/

FORM (SpellingChecker_addNewWord, L"Add word to user dictionary", L"SpellingChecker")
	SENTENCE (L"New word", L"ui/editors/AmplitudeTierEditor.h")
	OK
DO
	WHERE (SELECTED) {
		if (! SpellingChecker_addNewWord ((structSpellingChecker *)OBJECT, GET_STRING (L"New word"))) return 0;
		praat_dataChanged (OBJECT);
	}
END

FORM (SpellingChecker_edit, L"Edit spelling checker", L"SpellingChecker")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"-- Syntax --")
	SENTENCE (L"Forbidden strings", L"ui/editors/AmplitudeTierEditor.h")
	BOOLEAN (L"Check matching parentheses", 0)
	SENTENCE (L"Separating characters", L"ui/editors/AmplitudeTierEditor.h")
	BOOLEAN (L"Allow all parenthesized", 0)
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"-- Capitals --")
	BOOLEAN (L"Allow all names", 0)
	SENTENCE (L"Name prefixes", L"ui/editors/AmplitudeTierEditor.h")
	BOOLEAN (L"Allow all abbreviations", 0)
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"-- Capitalization --")
	BOOLEAN (L"Allow caps sentence-initially", 0)
	BOOLEAN (L"Allow caps after colon", 0)
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"-- Word parts --")
	SENTENCE (L"Allow all words containing", L"ui/editors/AmplitudeTierEditor.h")
	SENTENCE (L"Allow all words starting with", L"ui/editors/AmplitudeTierEditor.h")
	SENTENCE (L"Allow all words ending in", L"ui/editors/AmplitudeTierEditor.h")
	OK
SpellingChecker me = (structSpellingChecker *)ONLY_OBJECT;
SET_STRING (L"Forbidden strings", my forbiddenStrings)
SET_INTEGER (L"Check matching parentheses", my checkMatchingParentheses)
SET_STRING (L"Separating characters", my separatingCharacters)
SET_INTEGER (L"Allow all parenthesized", my allowAllParenthesized)
SET_INTEGER (L"Allow all names", my allowAllNames)
SET_STRING (L"Name prefixes", my namePrefixes)
SET_INTEGER (L"Allow all abbreviations", my allowAllAbbreviations)
SET_INTEGER (L"Allow caps sentence-initially", my allowCapsSentenceInitially)
SET_INTEGER (L"Allow caps after colon", my allowCapsAfterColon)
SET_STRING (L"Allow all words containing", my allowAllWordsContaining)
SET_STRING (L"Allow all words starting with", my allowAllWordsStartingWith)
SET_STRING (L"Allow all words ending in", my allowAllWordsEndingIn)
DO
	SpellingChecker me = (structSpellingChecker *)ONLY_OBJECT;
	Melder_free (my forbiddenStrings); my forbiddenStrings = Melder_wcsdup_f (GET_STRING (L"Forbidden strings"));
	my checkMatchingParentheses = GET_INTEGER (L"Check matching parentheses");
	Melder_free (my separatingCharacters); my separatingCharacters = Melder_wcsdup_f (GET_STRING (L"Separating characters"));
	my allowAllParenthesized = GET_INTEGER (L"Allow all parenthesized");
	my allowAllNames = GET_INTEGER (L"Allow all names");
	Melder_free (my namePrefixes); my namePrefixes = Melder_wcsdup_f (GET_STRING (L"Name prefixes"));
	my allowAllAbbreviations = GET_INTEGER (L"Allow all abbreviations");
	my allowCapsSentenceInitially = GET_INTEGER (L"Allow caps sentence-initially");
	my allowCapsAfterColon = GET_INTEGER (L"Allow caps after colon");
	Melder_free (my allowAllWordsContaining); my allowAllWordsContaining = Melder_wcsdup_f (GET_STRING (L"Allow all words containing"));
	Melder_free (my allowAllWordsStartingWith); my allowAllWordsStartingWith = Melder_wcsdup_f (GET_STRING (L"Allow all words starting with"));
	Melder_free (my allowAllWordsEndingIn); my allowAllWordsEndingIn = Melder_wcsdup_f (GET_STRING (L"Allow all words ending in"));
	praat_dataChanged (me);
END

DIRECT (SpellingChecker_extractWordList)
	EVERY_TO (SpellingChecker_extractWordList ((structSpellingChecker *)OBJECT))
END

DIRECT (SpellingChecker_extractUserDictionary)
	EVERY_TO (SpellingChecker_extractUserDictionary ((structSpellingChecker *)OBJECT))
END

FORM (SpellingChecker_isWordAllowed, L"Is word allowed?", L"SpellingChecker")
	SENTENCE (L"Word", L"ui/editors/AmplitudeTierEditor.h")
	OK
DO
	Melder_information1 (SpellingChecker_isWordAllowed ((structSpellingChecker *)ONLY_OBJECT, GET_STRING (L"Word")) ?
		L"1 (allowed)" : L"0 (not allowed)");
END

FORM (SpellingChecker_nextNotAllowedWord, L"Next not allowed word?", L"SpellingChecker")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Sentence:")
	TEXTFIELD (L"sentence", L"ui/editors/AmplitudeTierEditor.h")
	INTEGER (L"Starting character", L"0")
	OK
DO
	wchar_t *sentence = GET_STRING (L"sentence");
	long startingCharacter = GET_INTEGER (L"Starting character");
	REQUIRE (startingCharacter >= 0, L"Starting character should be 0 or positive.")
	REQUIRE (startingCharacter <= (int) wcslen (sentence), L"Starting character should not exceed end of sentence.")
	Melder_information1 (SpellingChecker_nextNotAllowedWord ((structSpellingChecker *)ONLY_OBJECT, sentence, & startingCharacter));
END

DIRECT (SpellingChecker_replaceWordList)
	if (! SpellingChecker_replaceWordList ((structSpellingChecker *)ONLY (classSpellingChecker), (structWordList *)ONLY (classWordList))) return 0;
END

DIRECT (SpellingChecker_replaceWordList_help)
	Melder_information1 (L"To replace the checker's word list\nby the contents of a Strings object:\n"
		"1. select the Strings;\n2. convert to a WordList object;\n3. select the SpellingChecker and the WordList;\n"
		"4. choose Replace.");
END

DIRECT (SpellingChecker_replaceUserDictionary)
	if (! SpellingChecker_replaceUserDictionary ((structSpellingChecker *)ONLY (classSpellingChecker), (structSortedSetOfString *)ONLY (classSortedSetOfString))) return 0;
END

/***** TEXTGRID *****/

FORM (TextGrid_countLabels, L"Count labels", L"TextGrid: Count labels...")
	INTEGER (STRING_TIER_NUMBER, L"1")
	SENTENCE (L"Label text", L"a")
	OK
DO
	Melder_information2 (Melder_integer (TextGrid_countLabels ((structTextGrid *)ONLY (classTextGrid),
		GET_INTEGER (STRING_TIER_NUMBER), GET_STRING (L"Label text"))), L" labels");
END

FORM (TextGrid_downto_Table, L"TextGrid: Down to Table", 0)
	BOOLEAN (L"Include line number", false)
	NATURAL (L"Time decimals", L"6")
	BOOLEAN (L"Include tier names", true)
	BOOLEAN (L"Include empty intervals", false)
	OK
DO
	EVERY_TO (TextGrid_downto_Table ((structTextGrid *)OBJECT, GET_INTEGER (L"Include line number"), GET_INTEGER (L"Time decimals"),
		GET_INTEGER (L"Include tier names"), GET_INTEGER (L"Include empty intervals")))
END

FORM (TextGrid_draw, L"TextGrid: Draw", 0)
	praat_dia_timeRange (dia);
	BOOLEAN (L"Show boundaries", 1)
	BOOLEAN (L"Use text styles", 1)
	BOOLEAN (L"Garnish", 1)
	OK
DO
	EVERY_DRAW (TextGrid_Sound_draw ((structTextGrid *)OBJECT, NULL, GRAPHICS,
		GET_REAL (L"left Time range"), GET_REAL (L"right Time range"), GET_INTEGER (L"Show boundaries"),
		GET_INTEGER (L"Use text styles"), GET_INTEGER (L"Garnish")))
END

FORM (TextGrid_duplicateTier, L"TextGrid: Duplicate tier", 0)
	NATURAL (STRING_TIER_NUMBER, L"1")
	NATURAL (L"Position", L"1 (= at top)")
	WORD (L"Name", L"ui/editors/AmplitudeTierEditor.h")
	OK
DO
	WHERE (SELECTED) {
		TextGrid grid = (structTextGrid *)OBJECT;
		int itier = GET_INTEGER (STRING_TIER_NUMBER);
		int position = GET_INTEGER (L"Position");
		const wchar_t *name = GET_STRING (L"Name");
		AnyTier newTier;
		if (itier > grid -> tiers -> size) itier = grid -> tiers -> size;
		newTier = (structAnyTier *)Data_copy (grid -> tiers -> item [itier]);
		if (! newTier) return 0;
		Thing_setName (newTier, name);
		if (! Ordered_addItemPos (grid -> tiers, newTier, position)) return 0;
		praat_dataChanged (OBJECT);
	}
END

static void cb_TextGridEditor_publish (Editor *editor, void *closure, Any publish) {
	(void) editor;
	(void) closure;
	if (! praat_new1 (publish, NULL)) { Melder_flushError (NULL); return; }
	praat_updateSelection ();
	if (Thing_member (publish, classSpectrum) && wcsequ (Thing_getName (publish), L"slice")) {
		int IOBJECT;
		WHERE (SELECTED) {
			SpectrumEditor *editor2 = new SpectrumEditor (theCurrentPraatApplication -> topShell, ID_AND_FULL_NAME, OBJECT);
			if (! editor2) return;
			if (! praat_installEditor (editor2, IOBJECT)) Melder_flushError (NULL);
		}
	}
}
DIRECT (TextGrid_edit)
	if (theCurrentPraatApplication -> batch) {
		return Melder_error1 (L"Cannot edit a TextGrid from batch.");
	} else {
		WHERE (SELECTED && CLASS == classTextGrid) {
			TextGridEditor *editor = new TextGridEditor (theCurrentPraatApplication -> topShell, ID_AND_FULL_NAME,
				(structTextGrid *)OBJECT, ONLY (classSound), NULL);
			if (! praat_installEditor (editor, IOBJECT)) return 0;
			editor->setPublishCallback (cb_TextGridEditor_publish, NULL);
		}
	}
END

DIRECT (TextGrid_LongSound_edit)
	if (theCurrentPraatApplication -> batch) {
		return Melder_error1 (L"Cannot edit a TextGrid from batch.");
	} else {
		LongSound longSound = NULL;
		int ilongSound = 0;
		WHERE (SELECTED)
			if (CLASS == classLongSound) longSound = (structLongSound *)OBJECT, ilongSound = IOBJECT;
		Melder_assert (ilongSound != 0);
		WHERE (SELECTED && CLASS == classTextGrid)
			if (! praat_installEditor2 (new TextGridEditor (theCurrentPraatApplication -> topShell, ID_AND_FULL_NAME,
				(structTextGrid *)OBJECT, longSound, NULL), IOBJECT, ilongSound)) return 0;
	}
END

DIRECT (TextGrid_SpellingChecker_edit)
	if (theCurrentPraatApplication -> batch) {
		return Melder_error1 (L"Cannot edit a TextGrid from batch.");
	} else {
		SpellingChecker spellingChecker = NULL;
		int ispellingChecker = 0;
		WHERE (SELECTED)
			if (CLASS == classSpellingChecker) spellingChecker = (structSpellingChecker *)OBJECT, ispellingChecker = IOBJECT;
		Melder_assert (ispellingChecker != 0);
		WHERE (SELECTED && CLASS == classTextGrid)
			if (! praat_installEditor2 (new TextGridEditor (theCurrentPraatApplication -> topShell, ID_AND_FULL_NAME,
				(structTextGrid *)OBJECT, ONLY (classSound), spellingChecker), IOBJECT, ispellingChecker)) return 0;
	}
END

DIRECT (TextGrid_LongSound_SpellingChecker_edit)
	if (theCurrentPraatApplication -> batch) {
		return Melder_error1 (L"Cannot edit a TextGrid from batch.");
	} else {
		LongSound longSound = NULL;
		SpellingChecker spellingChecker = NULL;
		int ilongSound = 0, ispellingChecker = 0;
		WHERE (SELECTED) {
			if (CLASS == classLongSound) longSound = (structLongSound *)OBJECT, ilongSound = IOBJECT;
			if (CLASS == classSpellingChecker) spellingChecker = (structSpellingChecker *)OBJECT, ispellingChecker = IOBJECT;
		}
		Melder_assert (ilongSound != 0 && ispellingChecker != 0);
		WHERE (SELECTED && CLASS == classTextGrid)
			if (! praat_installEditor3 (new TextGridEditor (theCurrentPraatApplication -> topShell, ID_AND_FULL_NAME,
				(structTextGrid *)OBJECT, longSound, spellingChecker), IOBJECT, ilongSound, ispellingChecker)) return 0;
	}
END

FORM (TextGrid_extractPart, L"TextGrid: Extract part", 0)
	REAL (L"left Time range (s)", L"0.0")
	REAL (L"right Time range (s)", L"1.0")
	BOOLEAN (L"Preserve times", 0)
	OK
DO
	TextGrid grid = (structTextGrid *)ONLY (classTextGrid);
	if (! praat_new2 (TextGrid_extractPart (grid, GET_REAL (L"left Time range"), GET_REAL (L"right Time range"), GET_INTEGER (L"Preserve times")), grid -> name, L"_part")) return 0;
END

static Data pr_TextGrid_getTier_e (UiForm *dia) {
	TextGrid grid = (structTextGrid *)ONLY_OBJECT;
	long tierNumber = GET_INTEGER (STRING_TIER_NUMBER);
	if (tierNumber > grid -> tiers -> size) return (structData *)Melder_errorp
		("Tier number (%ld) should not be larger than number of tiers (%ld).", tierNumber, grid -> tiers -> size);
	return (structData *)grid -> tiers -> item [tierNumber];
}

static IntervalTier pr_TextGrid_getIntervalTier_e (UiForm *dia) {
	Data tier = pr_TextGrid_getTier_e (dia);
	if (! tier) return NULL;
	if (tier -> methods != (Data_Table) classIntervalTier) return (structIntervalTier *)Melder_errorp1 (L"Tier should be interval tier.");
	return (IntervalTier) tier;
}

static TextTier pr_TextGrid_getTextTier_e (UiForm *dia) {
	Data tier = pr_TextGrid_getTier_e (dia);
	if (! tier) return NULL;
	if (tier -> methods != (Data_Table) classTextTier) return (structTextTier *)Melder_errorp1 (L"Tier should be point tier (TextTier).");
	return (TextTier) tier;
}

static TextInterval pr_TextGrid_getInterval_e (UiForm *dia) {
	int intervalNumber = GET_INTEGER (STRING_INTERVAL_NUMBER);
	IntervalTier intervalTier = pr_TextGrid_getIntervalTier_e (dia);
	if (! intervalTier) return NULL;
	if (intervalNumber > intervalTier -> intervals -> size) return (structTextInterval *)Melder_errorp1 (L"Interval number too large.");
	return (structTextInterval *)intervalTier -> intervals -> item [intervalNumber];
}

static TextPoint pr_TextGrid_getPoint_e (UiForm *dia) {	
	long pointNumber = GET_INTEGER (STRING_POINT_NUMBER);
	TextTier textTier = pr_TextGrid_getTextTier_e (dia);
	if (! textTier) return NULL;
	if (pointNumber > textTier -> points -> size) return (structTextPoint *)Melder_errorp1 (L"Point number too large.");
	return (structTextPoint *)textTier -> points -> item [pointNumber];
}

FORM (TextGrid_extractOneTier, L"TextGrid: Extract one tier", 0)
	NATURAL (STRING_TIER_NUMBER, L"1")
	OK
DO
	TextGrid grid = NULL;
	Data tier = NULL;
//start:
	tier = pr_TextGrid_getTier_e (dia); cherror
	grid = TextGrid_createWithoutTiers (1e30, -1e30); cherror
	TextGrid_add (grid, tier); cherror
	if (! praat_new1 (grid, tier -> name)) return 0;
end:
	iferror { forget (grid); return 0; }
END

FORM (TextGrid_extractTier, L"TextGrid: Extract tier", 0)
	NATURAL (STRING_TIER_NUMBER, L"1")
	OK
DO
	Data tier = pr_TextGrid_getTier_e (dia);
	if (! tier) return 0;
	if (! praat_new1 (Data_copy (tier), tier -> name)) return 0;
END

DIRECT (TextGrid_genericize)
	WHERE (SELECTED) {
		if (! TextGrid_genericize ((structTextGrid *)OBJECT)) return 0;
		praat_dataChanged (OBJECT);
	}
END

DIRECT (TextGrid_nativize)
	WHERE (SELECTED) {
		if (! TextGrid_nativize ((structTextGrid *)OBJECT)) return 0;
		praat_dataChanged (OBJECT);
	}
END

FORM (TextGrid_getHighIndexFromTime, L"Get high index", L"AnyTier: Get high index from time...")
	NATURAL (STRING_TIER_NUMBER, L"1")
	REAL (L"Time (s)", L"0.5")
	OK
DO
	TextTier textTier = pr_TextGrid_getTextTier_e (dia);
	if (! textTier) return 0;
	Melder_information1 (Melder_integer (AnyTier_timeToHighIndex (textTier, GET_REAL (L"Time"))));
END

FORM (TextGrid_getLowIndexFromTime, L"Get low index", L"AnyTier: Get low index from time...")
	NATURAL (STRING_TIER_NUMBER, L"1")
	REAL (L"Time (s)", L"0.5")
	OK
DO
	TextTier textTier = pr_TextGrid_getTextTier_e (dia);
	if (! textTier) return 0;
	Melder_information1 (Melder_integer (AnyTier_timeToLowIndex (textTier, GET_REAL (L"Time"))));
END

FORM (TextGrid_getNearestIndexFromTime, L"Get nearest index", L"AnyTier: Get nearest index from time...")
	NATURAL (STRING_TIER_NUMBER, L"1")
	REAL (L"Time (s)", L"0.5")
	OK
DO
	TextTier textTier = pr_TextGrid_getTextTier_e (dia);
	if (! textTier) return 0;
	Melder_information1 (Melder_integer (AnyTier_timeToNearestIndex (textTier, GET_REAL (L"Time"))));
END

FORM (TextGrid_getIntervalAtTime, L"TextGrid: Get interval at time", 0)
	NATURAL (STRING_TIER_NUMBER, L"1")
	REAL (L"Time (s)", L"0.5")
	OK
DO
	IntervalTier intervalTier = pr_TextGrid_getIntervalTier_e (dia);
	if (! intervalTier) return 0;
	Melder_information1 (Melder_integer (IntervalTier_timeToIndex (intervalTier, GET_REAL (L"Time"))));
END

FORM (TextGrid_getNumberOfIntervals, L"TextGrid: Get number of intervals", 0)
	NATURAL (STRING_TIER_NUMBER, L"1")
	OK
DO
	IntervalTier intervalTier = pr_TextGrid_getIntervalTier_e (dia);
	if (! intervalTier) return 0;
	Melder_information1 (Melder_integer (intervalTier -> intervals -> size));
END

DIRECT (TextGrid_getNumberOfTiers)
	TextGrid grid = (structTextGrid *)ONLY_OBJECT;
	Melder_information1 (Melder_integer (grid -> tiers -> size));
END

FORM (TextGrid_getStartingPoint, L"TextGrid: Get starting point", 0)
	NATURAL (STRING_TIER_NUMBER, L"1")
	NATURAL (STRING_INTERVAL_NUMBER, L"1")
	OK
DO
	TextInterval interval = pr_TextGrid_getInterval_e (dia);
	if (! interval) return 0;
	Melder_informationReal (interval -> xmin, L"seconds");
END

FORM (TextGrid_getEndPoint, L"TextGrid: Get end point", 0)
	NATURAL (STRING_TIER_NUMBER, L"1")
	NATURAL (STRING_INTERVAL_NUMBER, L"1")
	OK
DO
	TextInterval interval = pr_TextGrid_getInterval_e (dia);
	if (! interval) return 0;
	Melder_informationReal (interval -> xmax, L"seconds");
END
	
FORM (TextGrid_getLabelOfInterval, L"TextGrid: Get label of interval", 0)
	NATURAL (STRING_TIER_NUMBER, L"1")
	NATURAL (STRING_INTERVAL_NUMBER, L"1")
	OK
DO
	TextInterval interval = pr_TextGrid_getInterval_e (dia);
	if (! interval) return 0;
	MelderInfo_open ();
	MelderInfo_write1 (interval -> text);
	MelderInfo_close ();
END
	
FORM (TextGrid_getNumberOfPoints, L"TextGrid: Get number of points", 0)
	NATURAL (STRING_TIER_NUMBER, L"1")
	OK
DO
	TextTier textTier = pr_TextGrid_getTextTier_e (dia);
	if (! textTier) return 0;
	Melder_information1 (Melder_integer (textTier -> points -> size));
END
	
FORM (TextGrid_getTierName, L"TextGrid: Get tier name", 0)
	NATURAL (STRING_TIER_NUMBER, L"1")
	OK
DO
	Data tier = pr_TextGrid_getTier_e (dia);
	if (! tier) return 0;
	Melder_information1 (tier -> name);
END

FORM (TextGrid_getTimeOfPoint, L"TextGrid: Get time of point", 0)
	NATURAL (STRING_TIER_NUMBER, L"1")
	NATURAL (STRING_POINT_NUMBER, L"1")
	OK
DO
	TextPoint point = pr_TextGrid_getPoint_e (dia);
	if (! point) return 0;
	Melder_informationReal (point -> time, L"seconds");
END
	
FORM (TextGrid_getLabelOfPoint, L"TextGrid: Get label of point", 0)
	NATURAL (STRING_TIER_NUMBER, L"1")
	NATURAL (STRING_POINT_NUMBER, L"1")
	OK
DO
	TextPoint point = pr_TextGrid_getPoint_e (dia);
	if (! point) return 0;
	Melder_information1 (point -> mark);
END
	
DIRECT (TextGrid_help) Melder_help (L"TextGrid"); END

FORM (TextGrid_insertBoundary, L"TextGrid: Insert boundary", 0)
	NATURAL (STRING_TIER_NUMBER, L"1")
	REAL (L"Time (s)", L"0.5")
	OK
DO
	WHERE (SELECTED) {
		if (! TextGrid_insertBoundary ((structTextGrid *)OBJECT, GET_INTEGER (STRING_TIER_NUMBER), GET_REAL (L"Time"))) return 0;
		praat_dataChanged (OBJECT);
	}
END

FORM (TextGrid_insertIntervalTier, L"TextGrid: Insert interval tier", 0)
	NATURAL (L"Position", L"1 (= at top)")
	WORD (L"Name", L"ui/editors/AmplitudeTierEditor.h")
	OK
DO
	WHERE (SELECTED) {
		TextGrid grid = (structTextGrid *)OBJECT;
		int position = GET_INTEGER (L"Position");
		wchar_t *name = GET_STRING (L"Name");
		IntervalTier tier = IntervalTier_create (grid -> xmin, grid -> xmax);
		if (! tier) return 0;
		if (position > grid -> tiers -> size) position = grid -> tiers -> size + 1;
		Thing_setName (tier, name);
		if (! Ordered_addItemPos (grid -> tiers, tier, position)) return 0;
		praat_dataChanged (OBJECT);
	}
END

FORM (TextGrid_insertPoint, L"TextGrid: Insert point", 0)
	NATURAL (STRING_TIER_NUMBER, L"1")
	REAL (L"Time (s)", L"0.5")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Text:")
	TEXTFIELD (L"text", L"ui/editors/AmplitudeTierEditor.h")
	OK
DO
	WHERE (SELECTED) {
		if (! TextGrid_insertPoint ((structTextGrid *)OBJECT, GET_INTEGER (STRING_TIER_NUMBER), GET_REAL (L"Time"), GET_STRING (L"text"))) return 0;
		praat_dataChanged (OBJECT);
	}
END

FORM (TextGrid_insertPointTier, L"TextGrid: Insert point tier", 0)
	NATURAL (L"Position", L"1 (= at top)")
	WORD (L"Name", L"ui/editors/AmplitudeTierEditor.h")
	OK
DO
	WHERE (SELECTED) {
		TextGrid grid = (structTextGrid *)OBJECT;
		int position = GET_INTEGER (L"Position");
		wchar_t *name = GET_STRING (L"Name");
		TextTier tier = TextTier_create (grid -> xmin, grid -> xmax);
		if (! tier) return 0;
		if (position > grid -> tiers -> size) position = grid -> tiers -> size + 1;
		Thing_setName (tier, name);
		if (! Ordered_addItemPos (grid -> tiers, tier, position)) return 0;
		praat_dataChanged (OBJECT);
	}
END

FORM (TextGrid_isIntervalTier, L"TextGrid: Is interval tier?", 0)
	NATURAL (STRING_TIER_NUMBER, L"1")
	OK
DO
	Data tier = pr_TextGrid_getTier_e (dia);
	if (! tier) return 0;
	if (tier -> methods == (Data_Table) classIntervalTier) {
		Melder_information3 (L"1 (yes, tier ", Melder_integer (GET_INTEGER (STRING_TIER_NUMBER)), L" is an interval tier)");
	} else {
		Melder_information3 (L"0 (no, tier ", Melder_integer (GET_INTEGER (STRING_TIER_NUMBER)), L" is a point tier)");
	}
END

FORM (TextGrid_list, L"TextGrid: List", 0)
	BOOLEAN (L"Include line number", false)
	NATURAL (L"Time decimals", L"6")
	BOOLEAN (L"Include tier names", true)
	BOOLEAN (L"Include empty intervals", false)
	OK
DO
	EVERY (TextGrid_list ((structTextGrid *)OBJECT, GET_INTEGER (L"Include line number"), GET_INTEGER (L"Time decimals"),
		GET_INTEGER (L"Include tier names"), GET_INTEGER (L"Include empty intervals")))
END

DIRECT (TextGrids_merge)
	Collection textGrids;
	int n = 0;
	WHERE (SELECTED) n ++;
	textGrids = Collection_create (classTextGrid, n);
	WHERE (SELECTED) Collection_addItem (textGrids, OBJECT);
	if (! praat_new1 (TextGrid_merge (textGrids), L"merged")) {
		textGrids -> size = 0;   /* Undangle. */
		forget (textGrids);
		return 0;
	}
	textGrids -> size = 0;   /* Undangle. */
	forget (textGrids);
END

DIRECT (info_TextGrid_Pitch_draw)
	Melder_information1 (L"You can draw a TextGrid together with a Pitch after selecting them both.");
END

FORM (TextGrid_removeBoundaryAtTime, L"TextGrid: Remove boundary at time", 0)
	NATURAL (STRING_TIER_NUMBER, L"1")
	REAL (L"Time (s)", L"0.5")
	OK
DO
	WHERE (SELECTED) {
		if (! TextGrid_removeBoundaryAtTime ((structTextGrid *)OBJECT, GET_INTEGER (STRING_TIER_NUMBER), GET_REAL (L"Time"))) return 0;
		praat_dataChanged (OBJECT);
	}
END

FORM (TextGrid_getCentrePoints, L"TextGrid: Get centre points", 0)
	NATURAL (STRING_TIER_NUMBER, L"1")
	OPTIONMENU_ENUM (L"Get centre points where label", kMelder_string, DEFAULT)
	SENTENCE (L"...the text", L"hi")
	OK
DO
	wchar_t *text = GET_STRING (L"...the text");
	WHERE (SELECTED) {
		if (! praat_new3 (TextGrid_getCentrePoints ((structTextGrid *)OBJECT, GET_INTEGER (STRING_TIER_NUMBER),
			GET_ENUM (kMelder_string, L"Get centre points where label"), text), NAME, L"_", text)) return 0;
	}
END

FORM (TextGrid_getEndPoints, L"TextGrid: Get end points", 0)
	NATURAL (STRING_TIER_NUMBER, L"1")
	OPTIONMENU_ENUM (L"Get end points where label", kMelder_string, DEFAULT)
	SENTENCE (L"...the text", L"hi")
	OK
DO
	wchar_t *text = GET_STRING (L"...the text");
	WHERE (SELECTED) {
		if (! praat_new3 (TextGrid_getEndPoints ((structTextGrid *)OBJECT, GET_INTEGER (STRING_TIER_NUMBER),
			GET_ENUM (kMelder_string, L"Get end points where label"), text), NAME, L"_", text)) return 0;
	}
END

FORM (TextGrid_getStartingPoints, L"TextGrid: Get starting points", 0)
	NATURAL (STRING_TIER_NUMBER, L"1")
	OPTIONMENU_ENUM (L"Get starting points where label", kMelder_string, DEFAULT)
	SENTENCE (L"...the text", L"hi")
	OK
DO
	wchar_t *text = GET_STRING (L"...the text");
	WHERE (SELECTED) {
		if (! praat_new3 (TextGrid_getStartingPoints ((structTextGrid *)OBJECT, GET_INTEGER (STRING_TIER_NUMBER),
			GET_ENUM (kMelder_string, L"Get starting points where label"), text), NAME, L"_", text)) return 0;
	}
END

FORM (TextGrid_getPoints, L"Get points", 0)
	NATURAL (STRING_TIER_NUMBER, L"1")
	OPTIONMENU_ENUM (L"Get points where label", kMelder_string, DEFAULT)
	SENTENCE (L"...the text", L"hi")
	OK
DO
	wchar_t *text = GET_STRING (L"...the text");
	WHERE (SELECTED)
		if (! praat_new3 (TextGrid_getPoints ((structTextGrid *)OBJECT, GET_INTEGER (STRING_TIER_NUMBER),
			GET_ENUM (kMelder_string, L"Get points where label"), text), NAME, L"_", text)) return 0;
END

FORM (TextGrid_removeLeftBoundary, L"TextGrid: Remove left boundary", 0)
	NATURAL (STRING_TIER_NUMBER, L"1")
	NATURAL (STRING_INTERVAL_NUMBER, L"2")
	OK
DO
	long itier = GET_INTEGER (STRING_TIER_NUMBER);
	long iinterval = GET_INTEGER (STRING_INTERVAL_NUMBER);
	WHERE (SELECTED) {
		TextGrid grid = (structTextGrid *)OBJECT;
		IntervalTier intervalTier;
		if (itier > grid -> tiers -> size)
			return Melder_error7 (L"You cannot remove a boundary from tier ", Melder_integer (itier), L" of ", Thing_messageName (grid),
				L", because that TextGrid has only ", Melder_integer (grid -> tiers -> size), L" tiers.");
		intervalTier = (structIntervalTier *)grid -> tiers -> item [itier];
		if (intervalTier -> methods != classIntervalTier)
			return Melder_error5 (L"You cannot remove a boundary from tier ", Melder_integer (itier), L" of ", Thing_messageName (grid),
				L", because that tier is a point tier instead of an interval tier.");
		if (iinterval > intervalTier -> intervals -> size)
			return Melder_error9 (L"You cannot remove a boundary from interval ", Melder_integer (iinterval), L" of tier ", Melder_integer (itier), L" of ",
				Thing_messageName (grid), L", because that tier has only ", Melder_integer (intervalTier -> intervals -> size), L" intervals.");
		if (iinterval == 1)
			return Melder_error5 (L"You cannot remove the left boundary from interval 1 of tier ", Melder_integer (itier), L" of ", Thing_messageName (grid),
				L", because this is at the left edge of the tier.");
		if (! IntervalTier_removeLeftBoundary (intervalTier, iinterval)) return 0;
		praat_dataChanged (grid);
	}
END

FORM (TextGrid_removePoint, L"TextGrid: Remove point", 0)
	NATURAL (STRING_TIER_NUMBER, L"1")
	NATURAL (STRING_POINT_NUMBER, L"2")
	OK
DO
	long itier = GET_INTEGER (STRING_TIER_NUMBER);
	long ipoint = GET_INTEGER (STRING_POINT_NUMBER);
	WHERE (SELECTED) {
		TextGrid grid = (structTextGrid *)OBJECT;
		TextTier pointTier;
		if (itier > grid -> tiers -> size)
			return Melder_error7 (L"You cannot remove a point from tier ", Melder_integer (itier), L" of ", Thing_messageName (grid),
				L", because that TextGrid has only ", Melder_integer (grid -> tiers -> size), L" tiers.");
		pointTier = (structTextTier *)grid -> tiers -> item [itier];
		if (pointTier -> methods != classTextTier)
			return Melder_error5 (L"You cannot remove a point from tier ", Melder_integer (itier), L" of ", Thing_messageName (grid),
				L", because that tier is an interval tier instead of a point tier.");
		if (ipoint > pointTier -> points -> size)
			return Melder_error9 (L"You cannot remove point ", Melder_integer (ipoint), L" from tier ", Melder_integer (itier), L" of ", Thing_messageName (grid),
				L", because that tier has only ", Melder_integer (pointTier -> points -> size), L" points.");
		TextTier_removePoint (pointTier, ipoint);
		praat_dataChanged (grid);
	}
END

FORM (TextGrid_removeRightBoundary, L"TextGrid: Remove right boundary", 0)
	NATURAL (STRING_TIER_NUMBER, L"1")
	NATURAL (STRING_INTERVAL_NUMBER, L"1")
	OK
DO
	long itier = GET_INTEGER (STRING_TIER_NUMBER);
	long iinterval = GET_INTEGER (STRING_INTERVAL_NUMBER);
	WHERE (SELECTED) {
		TextGrid grid = (structTextGrid *)OBJECT;
		IntervalTier intervalTier;
		if (itier > grid -> tiers -> size)
			return Melder_error7 (L"You cannot remove a boundary from tier ", Melder_integer (itier), L" of ", Thing_messageName (grid),
				L", because that TextGrid has only ", Melder_integer (grid -> tiers -> size), L" tiers.");
		intervalTier = (structIntervalTier *)grid -> tiers -> item [itier];
		if (intervalTier -> methods != classIntervalTier)
			return Melder_error5 (L"You cannot remove a boundary from tier ", Melder_integer (itier), L" of ", Thing_messageName (grid),
				L", because that tier is a point tier instead of an interval tier.");
		if (iinterval > intervalTier -> intervals -> size)
			return Melder_error9 (L"You cannot remove a boundary from interval ", Melder_integer (iinterval), L" of tier ", Melder_integer (itier), L" of ", Thing_messageName (grid),
				L", because that tier has only ", Melder_integer (intervalTier -> intervals -> size), L" intervals.");
		if (iinterval == intervalTier -> intervals -> size)
			return Melder_error7 (L"You cannot remove the right boundary from interval ", Melder_integer (iinterval), L" of tier ", Melder_integer (itier), L" of ", Thing_messageName (grid),
				L", because this is at the right edge of the tier.");
		if (! IntervalTier_removeLeftBoundary (intervalTier, iinterval + 1)) return 0;
		praat_dataChanged (grid);
	}
END

FORM (TextGrid_removeTier, L"TextGrid: Remove tier", 0)
	NATURAL (STRING_TIER_NUMBER, L"1")
	OK
DO
	WHERE (SELECTED) {
		TextGrid grid = (structTextGrid *)OBJECT;
		int itier = GET_INTEGER (STRING_TIER_NUMBER);
		if (grid -> tiers -> size <= 1) {
			return Melder_error1 (L"Sorry, I refuse to remove the last tier.");
		}
		if (itier > grid -> tiers -> size) itier = grid -> tiers -> size;
		Collection_removeItem (grid -> tiers, itier);
		praat_dataChanged (grid);
	}
END

DIRECT (info_TextGrid_Sound_edit)
	Melder_information1 (L"To include a copy of a Sound in your TextGrid editor:\n"
		"   select a TextGrid and a Sound, and click \"View & Edit\".");
END

DIRECT (info_TextGrid_Sound_draw)
	Melder_information1 (L"You can draw a TextGrid together with a Sound after selecting them both.");
END

FORM (TextGrid_setIntervalText, L"TextGrid: Set interval text", 0)
	NATURAL (STRING_TIER_NUMBER, L"1")
	NATURAL (STRING_INTERVAL_NUMBER, L"1")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Text:")
	TEXTFIELD (L"text", L"ui/editors/AmplitudeTierEditor.h")
	OK
DO
	WHERE (SELECTED) {
		if (! TextGrid_setIntervalText ((structTextGrid *)OBJECT, GET_INTEGER (STRING_TIER_NUMBER), GET_INTEGER (STRING_INTERVAL_NUMBER), GET_STRING (L"text"))) return 0;
		praat_dataChanged (OBJECT);
	}
END

FORM (TextGrid_setPointText, L"TextGrid: Set point text", 0)
	NATURAL (STRING_TIER_NUMBER, L"1")
	NATURAL (STRING_POINT_NUMBER, L"1")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Text:")
	TEXTFIELD (L"text", L"ui/editors/AmplitudeTierEditor.h")
	OK
DO
	WHERE (SELECTED) {
		if (! TextGrid_setPointText ((structTextGrid *)OBJECT, GET_INTEGER (STRING_TIER_NUMBER), GET_INTEGER (STRING_POINT_NUMBER), GET_STRING (L"text"))) return 0;
		praat_dataChanged (OBJECT);
	}
END

FORM_WRITE (TextGrid_writeToChronologicalTextFile, L"Text file", 0, 0)
	if (! TextGrid_writeToChronologicalTextFile ((structTextGrid *)ONLY_OBJECT, file)) return 0;
END

/***** TEXTGRID & ANYTIER *****/

DIRECT (TextGrid_AnyTier_append)
	TextGrid oldGrid = (structTextGrid *)ONLY (classTextGrid), newGrid = (structTextGrid *)Data_copy (oldGrid);
	if (! newGrid) return 0;
	WHERE (SELECTED && OBJECT != oldGrid) if (! TextGrid_add (newGrid, OBJECT)) { forget (newGrid); return 0; }
	if (! praat_new1 (newGrid, oldGrid -> name)) return 0;
END

/***** TEXTGRID & LONGSOUND *****/

DIRECT (TextGrid_LongSound_scaleTimes)
	TextGrid grid = (structTextGrid *)ONLY (classTextGrid);
	LongSound longSound = (structLongSound *)ONLY (classLongSound);
	Function_scaleXTo (grid, longSound -> xmin, longSound -> xmax);
	praat_dataChanged (grid);
END

/***** TEXTTIER *****/

FORM (TextTier_addPoint, L"TextTier: Add point", L"TextTier: Add point...")
	REAL (L"Time (s)", L"0.5")
	SENTENCE (L"Text", L"ui/editors/AmplitudeTierEditor.h")
	OK
DO
	WHERE (SELECTED) {
		if (! TextTier_addPoint ((structTextTier *)OBJECT, GET_REAL (L"Time"), GET_STRING (L"Text"))) return 0;
		praat_dataChanged (OBJECT);
	}
END

DIRECT (TextTier_downto_PointProcess)
	EVERY_TO (AnyTier_downto_PointProcess (OBJECT))
END

FORM (TextTier_downto_TableOfReal, L"TextTier: Down to TableOfReal", 0)
	SENTENCE (L"Label", L"ui/editors/AmplitudeTierEditor.h")
	OK
DO
	EVERY_TO (TextTier_downto_TableOfReal ((structTextTier *)OBJECT, GET_STRING (L"Label")))
END

DIRECT (TextTier_downto_TableOfReal_any)
	EVERY_TO (TextTier_downto_TableOfReal_any ((structTextTier *)OBJECT))
END

FORM (TextTier_getLabelOfPoint, L"Get label of point", 0)
	NATURAL (L"Point number", L"1")
	OK
DO
	TextTier me = (structTextTier *)ONLY_OBJECT;
	long ipoint = GET_INTEGER (L"Point number");
	REQUIRE (ipoint <= my points -> size, L"No such point.")
	TextPoint point = (structTextPoint *)my points -> item [ipoint];
	Melder_information1 (point -> mark);
END

FORM (TextTier_getPoints, L"Get points", 0)
	SENTENCE (L"Text", L"ui/editors/AmplitudeTierEditor.h")
	OK
DO
	WHERE (SELECTED)
		if (! praat_new1 (TextTier_getPoints ((structTextTier *)OBJECT, GET_STRING (L"Text")), GET_STRING (L"Text"))) return 0;
END

DIRECT (TextTier_help) Melder_help (L"TextTier"); END

/***** WORDLIST *****/

FORM (WordList_hasWord, L"Does word occur in list?", L"WordList")
	SENTENCE (L"Word", L"ui/editors/AmplitudeTierEditor.h")
	OK
DO
	Melder_information1 (WordList_hasWord ((structWordList *)ONLY_OBJECT, GET_STRING (L"Word")) ? L"1" : L"0");
END

DIRECT (WordList_to_Strings)
	EVERY_TO (WordList_to_Strings ((structWordList *)OBJECT))
END

DIRECT (WordList_upto_SpellingChecker)
	EVERY_TO (WordList_upto_SpellingChecker ((structWordList *)OBJECT))
END

/***** buttons *****/

void praat_TimeFunction_query_init (void *klas);
void praat_TimeTier_query_init (void *klas);
void praat_TimeTier_modify_init (void *klas);

extern "C" void praat_uvafon_TextGrid_init (void);
void praat_uvafon_TextGrid_init (void) {
	Thing_recognizeClassByOtherName (classTextTier, L"MarkTier");

	TextGridEditor::prefs ();

	praat_addAction1 (classIntervalTier, 0, L"IntervalTier help", 0, 0, DO_IntervalTier_help);
	praat_addAction1 (classIntervalTier, 1, L"Save as Xwaves label file...", 0, 0, DO_IntervalTier_writeToXwaves);
	praat_addAction1 (classIntervalTier, 1, L"Write to Xwaves label file...", 0, praat_HIDDEN, DO_IntervalTier_writeToXwaves);
	praat_addAction1 (classIntervalTier, 0, L"Collect", 0, 0, 0);
	praat_addAction1 (classIntervalTier, 0, L"Into TextGrid", 0, 0, DO_AnyTier_into_TextGrid);
	praat_addAction1 (classIntervalTier, 0, L"Analyse", 0, 0, 0);
	praat_addAction1 (classIntervalTier, 0, L"Get starting points...", 0, 0, DO_IntervalTier_getStartingPoints);
	praat_addAction1 (classIntervalTier, 0, L"Get centre points...", 0, 0, DO_IntervalTier_getCentrePoints);
	praat_addAction1 (classIntervalTier, 0, L"Get end points...", 0, 0, DO_IntervalTier_getEndPoints);
	praat_addAction1 (classIntervalTier, 0, L"Convert", 0, 0, 0);
	praat_addAction1 (classIntervalTier, 0, L"Down to TableOfReal (any)", 0, 0, DO_IntervalTier_downto_TableOfReal_any);
	praat_addAction1 (classIntervalTier, 0, L"Down to TableOfReal...", 0, 0, DO_IntervalTier_downto_TableOfReal);

	praat_addAction1 (classLabel, 0, L"& Sound: To TextGrid?", 0, 0, DO_info_Label_Sound_to_TextGrid);

	praat_addAction1 (classSpellingChecker, 1, L"View & Edit...", 0, praat_ATTRACTIVE, DO_SpellingChecker_edit);
	praat_addAction1 (classSpellingChecker, 1, L"Edit...", 0, praat_HIDDEN, DO_SpellingChecker_edit);
	praat_addAction1 (classSpellingChecker, 0, L"Query", 0, 0, 0);
	praat_addAction1 (classSpellingChecker, 1, L"Is word allowed...", 0, 0, DO_SpellingChecker_isWordAllowed);
	praat_addAction1 (classSpellingChecker, 1, L"Next not allowed word...", 0, 0, DO_SpellingChecker_nextNotAllowedWord);
	praat_addAction1 (classSpellingChecker, 0, L"Modify", 0, 0, 0);
	praat_addAction1 (classSpellingChecker, 0, L"Add new word...", 0, 0, DO_SpellingChecker_addNewWord);
	praat_addAction1 (classSpellingChecker, 0, L"Analyze", 0, 0, 0);
	praat_addAction1 (classSpellingChecker, 0, L"Extract WordList", 0, 0, DO_SpellingChecker_extractWordList);
	praat_addAction1 (classSpellingChecker, 0, L"Extract user dictionary", 0, 0, DO_SpellingChecker_extractUserDictionary);

	praat_addAction1 (classTextGrid, 0, L"TextGrid help", 0, 0, DO_TextGrid_help);
	praat_addAction1 (classTextGrid, 1, L"Save as chronological text file...", 0, 0, DO_TextGrid_writeToChronologicalTextFile);
	praat_addAction1 (classTextGrid, 1, L"Write to chronological text file...", 0, praat_HIDDEN, DO_TextGrid_writeToChronologicalTextFile);
	praat_addAction1 (classTextGrid, 1, L"View & Edit alone", 0, 0, DO_TextGrid_edit);
	praat_addAction1 (classTextGrid, 1, L"View & Edit", 0, praat_HIDDEN, DO_TextGrid_edit);
	praat_addAction1 (classTextGrid, 1, L"Edit", 0, praat_HIDDEN, DO_TextGrid_edit);
	praat_addAction1 (classTextGrid, 1, L"View & Edit with Sound?", 0, praat_ATTRACTIVE, DO_info_TextGrid_Sound_edit);
	praat_addAction1 (classTextGrid, 0, L"Draw -", 0, 0, 0);
	praat_addAction1 (classTextGrid, 0, L"Draw...", 0, 1, DO_TextGrid_draw);
	praat_addAction1 (classTextGrid, 1, L"Draw with Sound?", 0, 1, DO_info_TextGrid_Sound_draw);
	praat_addAction1 (classTextGrid, 1, L"Draw with Pitch?", 0, 1, DO_info_TextGrid_Pitch_draw);
	praat_addAction1 (classTextGrid, 0, L"List...", 0, 0, DO_TextGrid_list);
	praat_addAction1 (classTextGrid, 0, L"Down to Table...", 0, 0, DO_TextGrid_downto_Table);
	praat_addAction1 (classTextGrid, 0, L"Query -", 0, 0, 0);
		praat_TimeFunction_query_init (classTextGrid);
		praat_addAction1 (classTextGrid, 1, L"-- query textgrid --", 0, 1, 0);
		praat_addAction1 (classTextGrid, 1, L"Get number of tiers", 0, 1, DO_TextGrid_getNumberOfTiers);
		praat_addAction1 (classTextGrid, 1, L"Get tier name...", 0, 1, DO_TextGrid_getTierName);
		praat_addAction1 (classTextGrid, 1, L"Is interval tier...", 0, 1, DO_TextGrid_isIntervalTier);
		praat_addAction1 (classTextGrid, 1, L"-- query tier --", 0, 1, 0);
		praat_addAction1 (classTextGrid, 1, L"Query interval tier", 0, 1, 0);
			praat_addAction1 (classTextGrid, 1, L"Get number of intervals...", 0, 2, DO_TextGrid_getNumberOfIntervals);
			praat_addAction1 (classTextGrid, 1, L"Get start point...", 0, 2, DO_TextGrid_getStartingPoint);
			praat_addAction1 (classTextGrid, 1, L"Get starting point...", 0, praat_HIDDEN + praat_DEPTH_2, DO_TextGrid_getStartingPoint);   // hidden 2008
			praat_addAction1 (classTextGrid, 1, L"Get end point...", 0, 2, DO_TextGrid_getEndPoint);
			praat_addAction1 (classTextGrid, 1, L"Get label of interval...", 0, 2, DO_TextGrid_getLabelOfInterval);
			praat_addAction1 (classTextGrid, 1, L"Get interval at time...", 0, 2, DO_TextGrid_getIntervalAtTime);
		praat_addAction1 (classTextGrid, 1, L"Query point tier", 0, 1, 0);
			praat_addAction1 (classTextGrid, 1, L"Get number of points...", 0, 2, DO_TextGrid_getNumberOfPoints);
			praat_addAction1 (classTextGrid, 1, L"Get time of point...", 0, 2, DO_TextGrid_getTimeOfPoint);
			praat_addAction1 (classTextGrid, 1, L"Get label of point...", 0, 2, DO_TextGrid_getLabelOfPoint);
			praat_addAction1 (classTextGrid, 1, L"Get low index from time...", 0, 2, DO_TextGrid_getLowIndexFromTime);
			praat_addAction1 (classTextGrid, 1, L"Get high index from time...", 0, 2, DO_TextGrid_getHighIndexFromTime);
			praat_addAction1 (classTextGrid, 1, L"Get nearest index from time...", 0, 2, DO_TextGrid_getNearestIndexFromTime);
		praat_addAction1 (classTextGrid, 1, L"-- query labels --", 0, 1, 0);
		praat_addAction1 (classTextGrid, 1, L"Count labels...", 0, 1, DO_TextGrid_countLabels);
	praat_addAction1 (classTextGrid, 0, L"Modify -", 0, 0, 0);
		praat_addAction1 (classTextGrid, 0, L"Convert to backslash trigraphs", 0, 1, DO_TextGrid_genericize);
		praat_addAction1 (classTextGrid, 0, L"Genericize", 0, praat_HIDDEN + praat_DEPTH_1, DO_TextGrid_genericize);   // hidden 2007
		praat_addAction1 (classTextGrid, 0, L"Convert to Unicode", 0, 1, DO_TextGrid_nativize);
		praat_addAction1 (classTextGrid, 0, L"Nativize", 0, praat_HIDDEN + praat_DEPTH_1, DO_TextGrid_nativize);   // hidden 2007
		praat_TimeFunction_modify_init (classTextGrid);
		praat_addAction1 (classTextGrid, 0, L"-- modify tiers --", 0, 1, 0);
		praat_addAction1 (classTextGrid, 0, L"Insert interval tier...", 0, 1, DO_TextGrid_insertIntervalTier);
		praat_addAction1 (classTextGrid, 0, L"Insert point tier...", 0, 1, DO_TextGrid_insertPointTier);
		praat_addAction1 (classTextGrid, 0, L"Duplicate tier...", 0, 1, DO_TextGrid_duplicateTier);
		praat_addAction1 (classTextGrid, 0, L"Remove tier...", 0, 1, DO_TextGrid_removeTier);
		praat_addAction1 (classTextGrid, 1, L"-- modify tier --", 0, 1, 0);
		praat_addAction1 (classTextGrid, 0, L"Modify interval tier", 0, 1, 0);
			praat_addAction1 (classTextGrid, 0, L"Insert boundary...", 0, 2, DO_TextGrid_insertBoundary);
			praat_addAction1 (classTextGrid, 0, L"Remove left boundary...", 0, 2, DO_TextGrid_removeLeftBoundary);
			praat_addAction1 (classTextGrid, 0, L"Remove right boundary...", 0, 2, DO_TextGrid_removeRightBoundary);
			praat_addAction1 (classTextGrid, 0, L"Remove boundary at time...", 0, 2, DO_TextGrid_removeBoundaryAtTime);
			praat_addAction1 (classTextGrid, 0, L"Set interval text...", 0, 2, DO_TextGrid_setIntervalText);
		praat_addAction1 (classTextGrid, 0, L"Modify point tier", 0, 1, 0);
			praat_addAction1 (classTextGrid, 0, L"Insert point...", 0, 2, DO_TextGrid_insertPoint);
			praat_addAction1 (classTextGrid, 0, L"Remove point...", 0, 2, DO_TextGrid_removePoint);
			praat_addAction1 (classTextGrid, 0, L"Set point text...", 0, 2, DO_TextGrid_setPointText);
praat_addAction1 (classTextGrid, 0, L"Analyse", 0, 0, 0);
	praat_addAction1 (classTextGrid, 1, L"Extract one tier...", 0, 0, DO_TextGrid_extractOneTier);
	praat_addAction1 (classTextGrid, 1, L"Extract tier...", 0, praat_HIDDEN, DO_TextGrid_extractTier);   // hidden 2010
	praat_addAction1 (classTextGrid, 1, L"Extract part...", 0, 0, DO_TextGrid_extractPart);
	praat_addAction1 (classTextGrid, 1, L"Analyse interval tier -", 0, 0, 0);
		praat_addAction1 (classTextGrid, 1, L"Get starting points...", 0, 1, DO_TextGrid_getStartingPoints);
		praat_addAction1 (classTextGrid, 1, L"Get end points...", 0, 1, DO_TextGrid_getEndPoints);
		praat_addAction1 (classTextGrid, 1, L"Get centre points...", 0, 1, DO_TextGrid_getCentrePoints);
	praat_addAction1 (classTextGrid, 1, L"Analyse point tier -", 0, 0, 0);
		praat_addAction1 (classTextGrid, 1, L"Get points...", 0, 1, DO_TextGrid_getPoints);
praat_addAction1 (classTextGrid, 0, L"Synthesize", 0, 0, 0);
	praat_addAction1 (classTextGrid, 0, L"Merge", 0, 0, DO_TextGrids_merge);

	praat_addAction1 (classTextTier, 0, L"TextTier help", 0, 0, DO_TextTier_help);
	praat_addAction1 (classTextTier, 0, L"Query -", 0, 0, 0);
		praat_TimeTier_query_init (classTextTier);
		praat_addAction1 (classTextTier, 0, L"Get label of point...", 0, 1, DO_TextTier_getLabelOfPoint);
	praat_addAction1 (classTextTier, 0, L"Modify -", 0, 0, 0);
		praat_TimeTier_modify_init (classTextTier);
		praat_addAction1 (classTextTier, 0, L"Add point...", 0, 1, DO_TextTier_addPoint);
	praat_addAction1 (classTextTier, 0, L"Analyse", 0, 0, 0);
	praat_addAction1 (classTextTier, 0, L"Get points...", 0, 0, DO_TextTier_getPoints);
	praat_addAction1 (classTextTier, 0, L"Collect", 0, 0, 0);
	praat_addAction1 (classTextTier, 0, L"Into TextGrid", 0, 0, DO_AnyTier_into_TextGrid);
	praat_addAction1 (classTextTier, 0, L"Convert", 0, 0, 0);
	praat_addAction1 (classTextTier, 0, L"Down to PointProcess", 0, 0, DO_TextTier_downto_PointProcess);
	praat_addAction1 (classTextTier, 0, L"Down to TableOfReal (any)", 0, 0, DO_TextTier_downto_TableOfReal_any);
	praat_addAction1 (classTextTier, 0, L"Down to TableOfReal...", 0, 0, DO_TextTier_downto_TableOfReal);

	praat_addAction1 (classWordList, 0, L"Query", 0, 0, 0);
		praat_addAction1 (classWordList, 1, L"Has word...", 0, 0, DO_WordList_hasWord);
	praat_addAction1 (classWordList, 0, L"Analyze", 0, 0, 0);
		praat_addAction1 (classWordList, 0, L"To Strings", 0, 0, DO_WordList_to_Strings);
	praat_addAction1 (classWordList, 0, L"Synthesize", 0, 0, 0);
		praat_addAction1 (classWordList, 0, L"Up to SpellingChecker", 0, 0, DO_WordList_upto_SpellingChecker);

	praat_addAction2 (classIntervalTier, 1, classPointProcess, 1, L"Start to centre...", 0, 0, DO_IntervalTier_PointProcess_startToCentre);
	praat_addAction2 (classIntervalTier, 1, classPointProcess, 1, L"End to centre...", 0, 0, DO_IntervalTier_PointProcess_endToCentre);
	praat_addAction2 (classIntervalTier, 0, classTextTier, 0, L"Collect", 0, 0, 0);
	praat_addAction2 (classIntervalTier, 0, classTextTier, 0, L"Into TextGrid", 0, 0, DO_AnyTier_into_TextGrid);
	praat_addAction2 (classLabel, 1, classSound, 1, L"To TextGrid", 0, 0, DO_Label_Sound_to_TextGrid);
	praat_addAction2 (classLongSound, 1, classTextGrid, 1, L"View & Edit", 0, praat_ATTRACTIVE, DO_TextGrid_LongSound_edit);
	praat_addAction2 (classLongSound, 1, classTextGrid, 1, L"Edit", 0, praat_HIDDEN, DO_TextGrid_LongSound_edit);   // hidden 2011
	praat_addAction2 (classLongSound, 1, classTextGrid, 1, L"Scale times", 0, 0, DO_TextGrid_LongSound_scaleTimes);
	praat_addAction2 (classPitch, 1, classTextGrid, 1, L"Draw -", 0, 0, 0);
	praat_addAction2 (classPitch, 1, classTextGrid, 1, L"Draw...", 0, 1, DO_TextGrid_Pitch_draw);
	praat_addAction2 (classPitch, 1, classTextGrid, 1, L"Draw logarithmic...", 0, 1, DO_TextGrid_Pitch_drawLogarithmic);
	praat_addAction2 (classPitch, 1, classTextGrid, 1, L"Draw semitones...", 0, 1, DO_TextGrid_Pitch_drawSemitones);
	praat_addAction2 (classPitch, 1, classTextGrid, 1, L"Draw mel...", 0, 1, DO_TextGrid_Pitch_drawMel);
	praat_addAction2 (classPitch, 1, classTextGrid, 1, L"Draw erb...", 0, 1, DO_TextGrid_Pitch_drawErb);
	praat_addAction2 (classPitch, 1, classTextGrid, 1, L"Speckle...", 0, 1, DO_TextGrid_Pitch_speckle);
	praat_addAction2 (classPitch, 1, classTextGrid, 1, L"Speckle logarithmic...", 0, 1, DO_TextGrid_Pitch_speckleLogarithmic);
	praat_addAction2 (classPitch, 1, classTextGrid, 1, L"Speckle semitones...", 0, 1, DO_TextGrid_Pitch_speckleSemitones);
	praat_addAction2 (classPitch, 1, classTextGrid, 1, L"Speckle mel...", 0, 1, DO_TextGrid_Pitch_speckleMel);
	praat_addAction2 (classPitch, 1, classTextGrid, 1, L"Speckle erb...", 0, 1, DO_TextGrid_Pitch_speckleErb);
	praat_addAction2 (classPitch, 1, classTextGrid, 1, L"-- draw separately --", 0, 1, 0);
	praat_addAction2 (classPitch, 1, classTextGrid, 1, L"Draw separately...", 0, 1, DO_TextGrid_Pitch_drawSeparately);
	praat_addAction2 (classPitch, 1, classTextGrid, 1, L"Draw separately (logarithmic)...", 0, 1, DO_TextGrid_Pitch_drawSeparatelyLogarithmic);
	praat_addAction2 (classPitch, 1, classTextGrid, 1, L"Draw separately (semitones)...", 0, 1, DO_TextGrid_Pitch_drawSeparatelySemitones);
	praat_addAction2 (classPitch, 1, classTextGrid, 1, L"Draw separately (mel)...", 0, 1, DO_TextGrid_Pitch_drawSeparatelyMel);
	praat_addAction2 (classPitch, 1, classTextGrid, 1, L"Draw separately (erb)...", 0, 1, DO_TextGrid_Pitch_drawSeparatelyErb);
	praat_addAction2 (classPitch, 1, classTextGrid, 1, L"Speckle separately...", 0, 1, DO_TextGrid_Pitch_speckleSeparately);
	praat_addAction2 (classPitch, 1, classTextGrid, 1, L"Speckle separately (logarithmic)...", 0, 1, DO_TextGrid_Pitch_speckleSeparatelyLogarithmic);
	praat_addAction2 (classPitch, 1, classTextGrid, 1, L"Speckle separately (semitones)...", 0, 1, DO_TextGrid_Pitch_speckleSeparatelySemitones);
	praat_addAction2 (classPitch, 1, classTextGrid, 1, L"Speckle separately (mel)...", 0, 1, DO_TextGrid_Pitch_speckleSeparatelyMel);
	praat_addAction2 (classPitch, 1, classTextGrid, 1, L"Speckle separately (erb)...", 0, 1, DO_TextGrid_Pitch_speckleSeparatelyErb);
	praat_addAction2 (classPitch, 1, classTextTier, 1, L"To PitchTier...", 0, 0, DO_Pitch_TextTier_to_PitchTier);
	praat_addAction2 (classSound, 1, classTextGrid, 1, L"View & Edit", 0, praat_ATTRACTIVE, DO_TextGrid_edit);
	praat_addAction2 (classSound, 1, classTextGrid, 1, L"Edit", 0, praat_HIDDEN, DO_TextGrid_edit);
	praat_addAction2 (classSound, 1, classTextGrid, 1, L"Draw...", 0, 0, DO_TextGrid_Sound_draw);
	praat_addAction2 (classSound, 1, classTextGrid, 1, L"Extract -", 0, 0, 0);
	praat_addAction2 (classSound, 1, classTextGrid, 1, L"Extract all intervals...", 0, praat_DEPTH_1, DO_TextGrid_Sound_extractAllIntervals);
	praat_addAction2 (classSound, 1, classTextGrid, 1, L"Extract non-empty intervals...", 0, praat_DEPTH_1, DO_TextGrid_Sound_extractNonemptyIntervals);
	praat_addAction2 (classSound, 1, classTextGrid, 1, L"Extract intervals...", 0, praat_DEPTH_1 + praat_HIDDEN, DO_TextGrid_Sound_extractIntervals);
	praat_addAction2 (classSound, 1, classTextGrid, 1, L"Extract intervals where...", 0, praat_DEPTH_1, DO_TextGrid_Sound_extractIntervalsWhere);
	praat_addAction2 (classSound, 1, classTextGrid, 1, L"Modify TextGrid", 0, 0, 0);
	praat_addAction2 (classSound, 1, classTextGrid, 1, L"Scale times", 0, 0, DO_TextGrid_Sound_scaleTimes);
	praat_addAction2 (classSound, 1, classTextGrid, 1, L"Modify Sound", 0, 0, 0);
	praat_addAction2 (classSound, 1, classTextGrid, 1, L"Clone time domain", 0, 0, DO_TextGrid_Sound_cloneTimeDomain);
	praat_addAction2 (classSpellingChecker, 1, classWordList, 1, L"Replace WordList", 0, 0, DO_SpellingChecker_replaceWordList);
	praat_addAction2 (classSpellingChecker, 1, classSortedSetOfString, 1, L"Replace user dictionary", 0, 0, DO_SpellingChecker_replaceUserDictionary);
	praat_addAction2 (classSpellingChecker, 1, classStrings, 1, L"Replace word list?", 0, 0, DO_SpellingChecker_replaceWordList_help);
	praat_addAction2 (classSpellingChecker, 1, classTextGrid, 1, L"View & Edit", 0, praat_ATTRACTIVE, DO_TextGrid_SpellingChecker_edit);
	praat_addAction2 (classSpellingChecker, 1, classTextGrid, 1, L"Edit", 0, praat_HIDDEN, DO_TextGrid_SpellingChecker_edit);   // hidden 2011
	praat_addAction2 (classTextGrid, 1, classTextTier, 1, L"Append", 0, 0, DO_TextGrid_AnyTier_append);
	praat_addAction2 (classTextGrid, 1, classIntervalTier, 1, L"Append", 0, 0, DO_TextGrid_AnyTier_append);

	praat_addAction3 (classLongSound, 1, classSpellingChecker, 1, classTextGrid, 1, L"View & Edit", 0, praat_ATTRACTIVE, DO_TextGrid_LongSound_SpellingChecker_edit);
	praat_addAction3 (classLongSound, 1, classSpellingChecker, 1, classTextGrid, 1, L"Edit", 0, praat_HIDDEN, DO_TextGrid_LongSound_SpellingChecker_edit);
	praat_addAction3 (classSound, 1, classSpellingChecker, 1, classTextGrid, 1, L"View & Edit", 0, praat_ATTRACTIVE, DO_TextGrid_SpellingChecker_edit);
	praat_addAction3 (classSound, 1, classSpellingChecker, 1, classTextGrid, 1, L"Edit", 0, praat_HIDDEN, DO_TextGrid_SpellingChecker_edit);
}

/* End of file praat_TextGrid_init.c */
