/* Excitation.c
 *
 * Copyright (C) 1992-2008 Paul Boersma
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
 * pb 2004/02/11 Excitation_soundPressureToPhon: handle zero sound pressure correctly (thanks to James Keidel)
 * pb 2006/12/10 MelderInfo
 * pb 2007/03/17 domain quantity
 * pb 2008/01/19 double
 */

#include "Excitation.h"

double Excitation_hertzToBark (double hertz) {
	double h650 = hertz / 650;
	return 7.0 * log (h650 + sqrt (1 + h650 * h650));
}

double Excitation_barkToHertz (double bark) {
	return 650.0 * sinh (bark / 7.0);
}

double Excitation_phonToDifferenceLimens (double phon) {
	return 30 * (pow (61.0 / 60, phon) - 1);
}

double Excitation_differenceLimensToPhon (double ndli) {
	return log (1 + ndli / 30) / log (61.0 / 60);
}

double Excitation_soundPressureToPhon (double soundPressure, double bark) {
	double result, dum;

	if (soundPressure <= 0.0) return 0.0;

	/*  dB = 20 * log10 (soundPressure / threshold)  */
	result = 20 * log10 (soundPressure / 2.0e-5);   /* First approximation: phon = dB */

	/*  Phones from dB  */
	if (result < 90 && bark < 8.0)
	{
		dum = (90 - result) * (8.0 - bark);
		result -= dum * dum / 2500;
	}
	dum = bark / 3.6 - 5;
	result += 5 * exp (- dum * dum);
	if (bark > 20.0) { dum = bark - 20; result -= 0.5 * dum * dum; }
	if (result < 0) result = 0;
	return result;
}

static void info (I) {
	iam (Excitation);
	double *y = my z [1];
	long i, numberOfMaxima = 0;
	classData -> info (me);
	MelderInfo_writeLine3 (L"Loudness: ", Melder_half (Excitation_getLoudness (me)), L" sones");
	for (i = 2; i < my nx; i ++) if (y [i] > y [i - 1] && y [i] >= y [i + 1]) {
		double i_real, formant_bark, strength;
		if (++ numberOfMaxima > 15) break;
		strength = NUMimproveMaximum (my z [1], my nx, i, NUM_PEAK_INTERPOLATE_SINC70, & i_real);
		formant_bark = my x1 + (i_real - 1) * my dx;
		MelderInfo_write3 (L"Peak at ", Melder_single (formant_bark), L" Bark");
		MelderInfo_write3 (L", ", Melder_integer ((long) NUMbarkToHertz (formant_bark)), L" Hertz");
		MelderInfo_writeLine3 (L", ", Melder_half (strength), L" phon.");
	}
}

class_methods (Excitation, Vector)
	class_method (info)
	us -> domainQuantity = MelderQuantity_FREQUENCY_BARK;
class_methods_end

Excitation Excitation_create (double df, long nf) {
	Excitation me = Thing_new (Excitation);
	if (! me || ! Matrix_init (me, 0.0, nf * df, nf, df, 0.5 * df, 1, 1, 1, 1, 1))
		forget (me);
	return me;
}

double Excitation_getDistance (Excitation me, Excitation thee) {
	double distance = 0.0, mean = 0.0;
	int i;
	Melder_assert (my nx == thy nx);
	for (i = 1; i <= my nx; i ++) {
		double dper = my z [1] [i] - thy z [1] [i];
		mean += dper;
		distance += dper * dper;
	}
	mean /= my nx;
	distance /= my nx;
	/* distance -= mean * mean; */
	return sqrt (distance);
}

double Excitation_getLoudness (Excitation me) {
	int i;
	double loudness = 0.0;
	for (i = 1; i <= my nx; i ++)
		/*  Sones = 2 ** ((Phones - 40) / 10)  */
		loudness += pow (2, (my z [1] [i] - 40) / 10);
	return my dx * loudness;
}

Matrix Excitation_to_Matrix (Excitation me) {
	Matrix thee = (structMatrix *)Data_copy (me);
	if (! thee) return NULL;
	Thing_overrideClass (thee, classMatrix);
	return thee;
}

Excitation Matrix_to_Excitation (Matrix me) {
	Excitation thee = (structExcitation *)Data_copy (me);
	if (! thee) return NULL;
	Thing_overrideClass (thee, classExcitation);
	return thee;
}

/* End of file Excitation.c */
