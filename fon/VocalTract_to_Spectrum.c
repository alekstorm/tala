/* VocalTract_to_Spectrum.c
 *
 * Copyright (C) 1991-2008 Paul Boersma
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
 * pb 1991/02/28 Pascal and Fortran versions
 * pb 1994/03/23 C version
 * pb 2002/06/04
 * pb 2002/07/16 GPL
 * pb 2008/01/19 double
 */

#include "VocalTract_to_Spectrum.h"

#define cc 353.0
#define rho 1.14
#define mu 1.86e-5
#define lambda 0.0055
#define cp 240.0
#define eta 1.4
#define shapeFactor 2.0

static void TUBE_transfer (double area [], int numberOfSections, double sectionLength, double frequency,
	double *re, double *im,   /* Output. */
	double glottalDamping,   /* 0 or 0.1 */
	int hasRadiationDamping, int hasInternalDamping)
	/* (Re,Im) := (air current at lips) / (air current at glottis) */
{
	int section;
	double omega = 2 * NUMpi * frequency;
	if (hasInternalDamping) {
		double bareResistance = sqrt (mu * rho / 2) * sqrt (omega);
		double bareConductance = (eta - 1) / (rho * cc * cc) *
			sqrt (lambda / (2 * cp * rho)) * sqrt (omega);
		dcomplex c = dcomplex_create (glottalDamping * area [1] / (rho * cc), 0.0);
		dcomplex d = dcomplex_create (1.0, 0.0);
		for (section = 1; section <= numberOfSections; section ++) {
			dcomplex help, help1, sinhg, coshg;
			double a = area [section];
			double perimeter = shapeFactor * 2 * sqrt (NUMpi * a);
			double perimeter_by_a2 = perimeter / (a * a);
			double inertance = rho / a;
			double compliance = a / (rho * cc * cc);
			dcomplex cascade = dcomplex_create (perimeter_by_a2 * bareResistance,
				omega * inertance + perimeter_by_a2 * bareResistance);
			dcomplex parallel = dcomplex_create (perimeter * bareConductance,
				omega * compliance);
			dcomplex gamma = dcomplex_sqrt (dcomplex_mul (cascade, parallel));
			dcomplex impedance = dcomplex_div (gamma, parallel);
			gamma = dcomplex_rmul (sectionLength, gamma);
			help = dcomplex_rmul (0.5, dcomplex_exp (gamma));
			help1 = dcomplex_div (dcomplex_create (0.25, 0), help);
			sinhg = dcomplex_sub (help, help1);
			coshg = dcomplex_add (help, help1);
			help = dcomplex_add (dcomplex_mul (c, coshg), dcomplex_div (dcomplex_mul (d, sinhg), impedance));
			d = dcomplex_add (dcomplex_mul (d, coshg), dcomplex_mul (dcomplex_mul (impedance, c), sinhg));
			c = help;
		}
		if (hasRadiationDamping) {
			double ka = omega * sqrt (area [numberOfSections] / NUMpi) / cc;
			double z = rho * cc / area [numberOfSections];
			double radiationResistance = z * ka * ka / 2;
			double radiationReactance = z * 8 * ka / 3 / NUMpi;
			*re = d.re + c.re * radiationResistance - c.im * radiationReactance;
			*im = d.im + c.im * radiationResistance + c.re * radiationReactance;
		} else { *re = d.re; *im = d.im; };
	} else {
		double c_re, c_im;
		double angle = omega * sectionLength / cc, cosAngle = cos (angle), sinAngle = sin (angle);
		/* The parallel conductance of the glottis is due to air escaping */
		/* through it. It is approximated as  0.1 A / rho C  */
		c_re = glottalDamping * cosAngle;
		c_im = sinAngle;
		*re = cosAngle;
		*im = glottalDamping * sinAngle;
		for (section = 1; section < numberOfSections; section ++) {
			double k = area [section] / area [section + 1];
			double reDummy = c_re * k * cosAngle - *im * sinAngle;
			double imDummy = c_im * k * cosAngle + *re * sinAngle;
			*re = *re * cosAngle - c_im * k * sinAngle;
			*im = *im * cosAngle + c_re * k * sinAngle;
			c_re = reDummy;
			c_im = imDummy;
		}
		/* Radiation impedance at the lips. */
		/* Energy radiates from the mouth. This loses energy to the formants. */
		if (hasRadiationDamping) {
			double ka = omega * sqrt (area [numberOfSections] / NUMpi) / cc;
			double radiationResistance = ka * ka / 2;
			double radiationReactance = 8 * ka / 3 / NUMpi;
			*re += c_re * radiationResistance - c_im * radiationReactance;
			*im += c_re * radiationReactance + c_im * radiationResistance;
		}
	}
	/* Divide into 1. */
	{
		double power = *re * *re + *im * *im;
		if (power != 0) { *re = *re / power; *im = *im / power; }
	}
}

Spectrum VocalTract_to_Spectrum
	(VocalTract me, long numberOfFrequencies, double maximumFrequency,
	 double glottalDamping, int hasRadiationDamping, int hasInternalDamping)
{
	Spectrum thee = Spectrum_create (maximumFrequency, numberOfFrequencies);
	long ifreq;
	for (ifreq = 1; ifreq <= numberOfFrequencies; ifreq ++) {
		TUBE_transfer (my z [1], my nx, my dx,
			(ifreq - 0.9999) * maximumFrequency / (numberOfFrequencies - 1),
			& thy z [1] [ifreq], & thy z [2] [ifreq],
			glottalDamping, hasRadiationDamping, hasInternalDamping);
		thy z [1] [ifreq] *= 0.02;
		thy z [2] [ifreq] *= 0.02;
	}
	return thee;
}

/* End of file VocalTract_to_Spectrum.c */
