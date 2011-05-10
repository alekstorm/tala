/* PitchTier.c
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
 * pb 2002/07/16 GPL
 * pb 2003/05/31 PointProcess_upto_RealTier
 * pb 2004/10/03 moved audio to PitchTier_to_Sound.c
 * pb 2005/06/16 units
 * pb 2006/12/08 info
 * pb 2007/03/17 domain quantity
 * pb 2007/08/12 wchar_t
 * pb 2010/10/19 allow drawing without speckles
 */

#include "PitchTier.h"
#include "Pitch.h"

static void info (I) {
	iam (RealTier);
	classData -> info (me);
	MelderInfo_writeLine1 (L"Time domain:");
	MelderInfo_writeLine3 (L"   Start time: ", Melder_double (my xmin), L" seconds");
	MelderInfo_writeLine3 (L"   End time: ", Melder_double (my xmax), L" seconds");
	MelderInfo_writeLine3 (L"   Total duration: ", Melder_double (my xmax - my xmin), L" seconds");
	MelderInfo_writeLine2 (L"Number of points: ", Melder_integer (my points -> size));
	MelderInfo_writeLine3 (L"Minimum pitch value: ", Melder_double (RealTier_getMinimumValue (me)), L" Hertz");
	MelderInfo_writeLine3 (L"Maximum pitch value: ", Melder_double (RealTier_getMaximumValue (me)), L" Hertz");
}

class_methods (PitchTier, RealTier)
	class_method (info)
	us -> domainQuantity = MelderQuantity_TIME_SECONDS;
class_methods_end

PitchTier PitchTier_create (double tmin, double tmax) {
	PitchTier me = Thing_new (PitchTier); cherror
	RealTier_init_e (me, tmin, tmax); cherror
end:
	iferror forget (me);
	return me;
}

PitchTier PointProcess_upto_PitchTier (PointProcess me, double frequency) {
	PitchTier thee = (PitchTier) PointProcess_upto_RealTier (me, frequency);
	if (! thee) return NULL;
	Thing_overrideClass (thee, classPitchTier);
	return thee;
}

void PitchTier_stylize (PitchTier me, double frequencyResolution, int useSemitones) {
	double dfmin;
	for (;;) {
		long i, imin = 0;
		dfmin = 1e300;
		for (i = 2; i <= my points -> size - 1; i ++) {
			RealPoint pm = (structRealPoint *)my points -> item [i];
			RealPoint pl = (structRealPoint *)my points -> item [i - 1];
			RealPoint pr = (structRealPoint *)my points -> item [i + 1];
			double expectedFrequency = pl -> value + (pr -> value - pl -> value) /
				 (pr -> time - pl -> time) * (pm -> time - pl -> time);
			double df = useSemitones ?
				12 * fabs (log (pm -> value / expectedFrequency)) / NUMln2:
				fabs (pm -> value - expectedFrequency);
			if (df < dfmin) {
				imin = i;
				dfmin = df;
			}
		}
		if (imin == 0 || dfmin > frequencyResolution) break;
		Collection_removeItem (my points, imin);
	}
}

static int PitchTier_writeToSpreadsheetFile (PitchTier me, MelderFile fs, int hasHeader) {
	FILE *f = Melder_fopen (fs, "w");
	long i;
	if (! f) return 0;
	if (hasHeader)
		fprintf (f, "\"ooTextFile\"\n\"PitchTier\"\n%.17g %.17g %ld\n", my xmin, my xmax, my points -> size);
	for (i = 1; i <= my points -> size; i ++) {
		RealPoint point = (structRealPoint *)my points -> item [i];
		fprintf (f, "%.17g\t%.17g\n", point -> time, point -> value);
	}
	if (! Melder_fclose (fs, f)) return 0;
	MelderFile_setMacTypeAndCreator (fs, 'TEXT', 0);
	return 1;
}

int PitchTier_writeToPitchTierSpreadsheetFile (PitchTier me, MelderFile fs) {
	return PitchTier_writeToSpreadsheetFile (me, fs, TRUE);
}

int PitchTier_writeToHeaderlessSpreadsheetFile (PitchTier me, MelderFile fs) {
	return PitchTier_writeToSpreadsheetFile (me, fs, FALSE);
}

int PitchTier_shiftFrequencies (PitchTier me, double tmin, double tmax, double shift, int unit) {
	long i;
	for (i = 1; i <= my points -> size; i ++) {
		RealPoint point = (structRealPoint *)my points -> item [i];
		double frequency = point -> value;
		if (point -> time < tmin || point -> time > tmax) continue;
		switch (unit) {
			case kPitch_unit_HERTZ: {	
				frequency += shift;
				if (frequency <= 0.0)
					error1 (L"Resulting frequency has to be greater than 0 Hz.")
			} break; case kPitch_unit_MEL: {
				frequency = NUMhertzToMel (frequency) + shift;
				if (frequency <= 0.0)
					error1 (L"Resulting frequency has to be greater than 0 mel.")
				frequency = NUMmelToHertz (frequency);
			} break; case kPitch_unit_LOG_HERTZ: {
				frequency = pow (10.0, log10 (frequency) + shift);
			} break; case kPitch_unit_SEMITONES_1: {
				frequency = NUMsemitonesToHertz (NUMhertzToSemitones (frequency) + shift);
			} break; case kPitch_unit_ERB: {
				frequency = NUMhertzToErb (frequency) + shift;
				if (frequency <= 0.0)
					error1 (L"Resulting frequency has to be greater than 0 ERB.")
				frequency = NUMerbToHertz (frequency);
			}
		}
		point -> value = frequency;
	}
end:
	iferror return Melder_error1 (L"(PitchTier_shiftFrequency:) Not completed.");
	return 1;
}

void PitchTier_multiplyFrequencies (PitchTier me, double tmin, double tmax, double factor) {
	long i;
	Melder_assert (factor > 0.0);
	for (i = 1; i <= my points -> size; i ++) {
		RealPoint point = (structRealPoint *)my points -> item [i];
		if (point -> time < tmin || point -> time > tmax) continue;
		point -> value *= factor;
	}
}

/* End of file PitchTier.c */
