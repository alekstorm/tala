/* TextGrid_Sound.c
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
 * pb 2010/12/06 BDF/EDF files (for EEG)
 * pb 2010/12/08 split off from TextGrid.c and Sound.c
 */

#include "TextGrid_Sound.h"
#include "Pitch_to_PitchTier.h"

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
		Data anyTier = my tiers -> item [itier];
		double ymin = -1.0 - 0.5 * itier, ymax = ymin + 0.5;
		Graphics_rectangle (g, tmin, tmax, ymin, ymax);
		if (anyTier -> methods == (Data_Table) classIntervalTier) {
			IntervalTier tier = (IntervalTier) anyTier;
			long iinterval, ninterval = tier -> intervals -> size;
			for (iinterval = 1; iinterval <= ninterval; iinterval ++) {
				TextInterval interval = tier -> intervals -> item [iinterval];
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
				TextPoint point = tier -> points -> item [i];
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

Collection TextGrid_Sound_extractAllIntervals (TextGrid me, Sound sound, long itier, int preserveTimes) {
	IntervalTier tier;
	long iseg;
	Collection collection;
	if (itier < 1 || itier > my tiers -> size)
		return Melder_errorp ("Tier number %ld out of range 1..%ld.", itier, my tiers -> size);
	tier = my tiers -> item [itier];
	if (tier -> methods != classIntervalTier)
		return Melder_errorp ("Tier %ld is not an interval tier.", itier);
	collection = Collection_create (NULL, tier -> intervals -> size); cherror
	for (iseg = 1; iseg <= tier -> intervals -> size; iseg ++) {
		TextInterval segment = tier -> intervals -> item [iseg];
		Sound interval = Sound_extractPart (sound, segment -> xmin, segment -> xmax, 0, 1.0, preserveTimes); cherror
		Thing_setName (interval, segment -> text ? segment -> text : L"untitled");
		Collection_addItem (collection, interval); cherror
	}
end:
	iferror forget (collection);
	return collection;
}

Collection TextGrid_Sound_extractNonemptyIntervals (TextGrid me, Sound sound, long itier, int preserveTimes) {
	IntervalTier tier;
	long iseg;
	Collection collection;
	if (itier < 1 || itier > my tiers -> size)
		return Melder_errorp ("Tier number %ld out of range 1..%ld.", itier, my tiers -> size);
	tier = my tiers -> item [itier];
	if (tier -> methods != classIntervalTier)
		return Melder_errorp ("Tier %ld is not an interval tier.", itier);
	collection = Collection_create (NULL, tier -> intervals -> size); cherror
	for (iseg = 1; iseg <= tier -> intervals -> size; iseg ++) {
		TextInterval segment = tier -> intervals -> item [iseg];
		if (segment -> text != NULL && segment -> text [0] != '\0') {
			Sound interval = Sound_extractPart (sound, segment -> xmin, segment -> xmax, 0, 1.0, preserveTimes); cherror
			Thing_setName (interval, segment -> text ? segment -> text : L"untitled");
			Collection_addItem (collection, interval); cherror
		}
	}
	if (collection -> size == 0) Melder_warning1 (L"No non-empty intervals were found.");
end:
	iferror forget (collection);
	return collection;
}

Collection TextGrid_Sound_extractIntervalsWhere (TextGrid me, Sound sound, long itier,
	int comparison_Melder_STRING, const wchar_t *text, int preserveTimes)
{
	IntervalTier tier;
	long count = 0;
	Collection collection;
	if (itier < 1 || itier > my tiers -> size)
		return Melder_errorp ("Tier number %ld out of range 1..%ld.", itier, my tiers -> size);
	tier = my tiers -> item [itier];
	if (tier -> methods != classIntervalTier)
		return Melder_errorp ("Tier %ld is not an interval tier.", itier);
	collection = Collection_create (NULL, 10);
	if (! collection) goto error;
	for (long iseg = 1; iseg <= tier -> intervals -> size; iseg ++) {
		TextInterval segment = tier -> intervals -> item [iseg];
		if (Melder_stringMatchesCriterion (segment -> text, comparison_Melder_STRING, text)) {
			Sound interval = Sound_extractPart (sound, segment -> xmin, segment -> xmax,
				0, 1.0, preserveTimes);
			wchar_t name [1000];
			if (! interval) goto error;
			swprintf (name, 1000, L"%ls_%ls_%ld", sound -> name ? sound -> name : L"", text, ++ count);
			Thing_setName (interval, name);
			if (! Collection_addItem (collection, interval)) goto error;
		}
	}
	if (collection -> size == 0)
		Melder_warning5 (L"No label that ", kMelder_string_getText (comparison_Melder_STRING), L" the text \"", text, L"\" was found.");
	return collection;
error:
	forget (collection);
	return NULL;
}

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
	anyTier = grid -> tiers -> item [itier];
	if (anyTier -> methods == (Data_Table) classIntervalTier) {
		IntervalTier tier = (IntervalTier) anyTier;
		for (i = 1; i <= tier -> intervals -> size; i ++) {
			TextInterval interval = tier -> intervals -> item [i];
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
			TextPoint point = tier -> points -> item [i];
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

int TextGrid_Sound_readFromBdfFile (MelderFile file, TextGrid *out_textGrid, Sound *out_sound) {
	*out_textGrid = NULL;
	*out_sound = NULL;
	Sound me = NULL;
	TextGrid thee = NULL;
	double *physicalMinimum = NULL, *physicalMaximum = NULL, *digitalMinimum = NULL, *digitalMaximum = NULL;
//start:
	FILE *f = Melder_fopen (file, "rb");
	char buffer [81];
	fread (buffer, 1, 8, f); buffer [8] = '\0';
	fread (buffer, 1, 80, f); buffer [80] = '\0';
	//Melder_casual ("Local subject identification: \"%s\"", buffer);
	fread (buffer, 1, 80, f); buffer [80] = '\0';
	//Melder_casual ("Local recording identification: \"%s\"", buffer);
	fread (buffer, 1, 8, f); buffer [8] = '\0';
	//Melder_casual ("Start date of recording: \"%s\"", buffer);
	fread (buffer, 1, 8, f); buffer [8] = '\0';
	//Melder_casual ("Start time of recording: \"%s\"", buffer);
	fread (buffer, 1, 8, f); buffer [8] = '\0';
	long numberOfBytesInHeaderRecord = atol (buffer);
	//Melder_casual ("Number of bytes in header record: %ld", numberOfBytesInHeaderRecord);
	fread (buffer, 1, 44, f); buffer [44] = '\0';
	//Melder_casual ("Version of data format: \"%s\"", buffer);
	fread (buffer, 1, 8, f); buffer [8] = '\0';
	long numberOfDataRecords = strtol (buffer, NULL, 10);
	//Melder_casual ("Number of data records: %ld", numberOfDataRecords);
	fread (buffer, 1, 8, f); buffer [8] = '\0';
	double durationOfDataRecord = atof (buffer);
	//Melder_casual ("Duration of a data record: \"%f\"", durationOfDataRecord);
	fread (buffer, 1, 4, f); buffer [4] = '\0';
	long numberOfChannels = atol (buffer);
	//Melder_casual ("Number of channels in data record: %ld", numberOfChannels);
	if (numberOfBytesInHeaderRecord != (numberOfChannels + 1) * 256)
		error5 (L"(Read from BDF file:) Number of bytes in header record (", Melder_integer (numberOfBytesInHeaderRecord),
			L") doesn't match number of channels (", Melder_integer (numberOfChannels), L").")
	for (long ichannel = 1; ichannel <= numberOfChannels; ichannel ++) {
		fread (buffer, 1, 16, f); buffer [16] = '\0';   // labels of the channels
	}
	double samplingFrequency = NUMundefined;
	for (long channel = 1; channel <= numberOfChannels; channel ++) {
		fread (buffer, 1, 80, f); buffer [80] = '\0';   // transducer type
	}
	for (long channel = 1; channel <= numberOfChannels; channel ++) {
		fread (buffer, 1, 8, f); buffer [8] = '\0';   // physical dimension of channels
	}
	physicalMinimum = NUMdvector (1, numberOfChannels); cherror
	for (long ichannel = 1; ichannel <= numberOfChannels; ichannel ++) {
		fread (buffer, 1, 8, f); buffer [8] = '\0';
		physicalMinimum [ichannel] = atof (buffer);
	}
	physicalMaximum = NUMdvector (1, numberOfChannels); cherror
	for (long ichannel = 1; ichannel <= numberOfChannels; ichannel ++) {
		fread (buffer, 1, 8, f); buffer [8] = '\0';
		physicalMaximum [ichannel] = atof (buffer);
	}
	digitalMinimum = NUMdvector (1, numberOfChannels); cherror
	for (long ichannel = 1; ichannel <= numberOfChannels; ichannel ++) {
		fread (buffer, 1, 8, f); buffer [8] = '\0';
		digitalMinimum [ichannel] = atof (buffer);
	}
	digitalMaximum = NUMdvector (1, numberOfChannels); cherror
	for (long ichannel = 1; ichannel <= numberOfChannels; ichannel ++) {
		fread (buffer, 1, 8, f); buffer [8] = '\0';
		digitalMaximum [ichannel] = atof (buffer);
	}
	for (long channel = 1; channel <= numberOfChannels; channel ++) {
		fread (buffer, 1, 80, f); buffer [80] = '\0';   // prefiltering
	}
	long numberOfSamplesPerDataRecord = 0;
	for (long channel = 1; channel <= numberOfChannels; channel ++) {
		fread (buffer, 1, 8, f); buffer [8] = '\0';   // number of samples in each data record
		long numberOfSamplesInThisDataRecord = atol (buffer);
		if (samplingFrequency == NUMundefined) {
			numberOfSamplesPerDataRecord = numberOfSamplesInThisDataRecord;
			samplingFrequency = numberOfSamplesInThisDataRecord / durationOfDataRecord;
		}
		if (numberOfSamplesInThisDataRecord / durationOfDataRecord != samplingFrequency)
			error7 (L"(Read from BDF file:) Number of samples per data record in channel ", Melder_integer (channel),
				L" (", Melder_integer (numberOfSamplesInThisDataRecord),
				L") doesn't match sampling frequency of channel 1 (", Melder_integer (numberOfChannels), L").")
	}
	for (long channel = 1; channel <= numberOfChannels; channel ++) {
		fread (buffer, 1, 32, f); buffer [32] = '\0';   // reserved
	}
	double duration = numberOfDataRecords * durationOfDataRecord;
	me = Sound_createSimple (numberOfChannels, duration, samplingFrequency); cherror
	for (long record = 1; record <= numberOfDataRecords; record ++) {
		for (long channel = 1; channel <= numberOfChannels; channel ++) {
			double factor = channel == numberOfChannels ? 1.0 : physicalMinimum [channel] / digitalMinimum [channel];
			for (long i = 1; i <= numberOfSamplesPerDataRecord; i ++) {
				long sample = i + (record - 1) * numberOfSamplesPerDataRecord;
				Melder_assert (sample <= my nx);
				my z [channel] [sample] = bingeti3LE (f) * factor;
			}
		}
	}
	thee = TextGrid_create (0, duration, L"S1 S2 S3 S4 S5 S6 S7 S8", L""); cherror
	for (int bit = 1; bit <= 8; bit ++) {
		unsigned long bitValue = 1 << (bit - 1);
		IntervalTier tier = thy tiers -> item [bit];
		for (long i = 1; i <= my nx; i ++) {
			unsigned long previousValue = i == 1 ? 0 : (long) my z [numberOfChannels] [i - 1];
			unsigned long thisValue = (long) my z [numberOfChannels] [i];
			if ((thisValue & bitValue) != (previousValue & bitValue)) {
				double time = i == 1 ? 0.0 : my x1 + (i - 1.5) * my dx;
				if (time != 0.0) TextGrid_insertBoundary (thee, bit, time);
				if ((thisValue & bitValue) != 0) {
					TextGrid_setIntervalText (thee, bit, tier -> intervals -> size, L"1");
				}
			}
		}
	}
end:
	NUMdvector_free (physicalMinimum, 1);
	NUMdvector_free (physicalMaximum, 1);
	NUMdvector_free (digitalMinimum, 1);
	NUMdvector_free (digitalMaximum, 1);
	iferror return 0;
	*out_sound = me;
	*out_textGrid = thee;
	return 1;
}

/* End of file TextGrid_Sound.c */
