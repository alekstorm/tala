#ifndef _AmplitudeTier_h_
#define _AmplitudeTier_h_
/* AmplitudeTier.h
 *
 * Copyright (C) 2003-2011 Paul Boersma
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
 * pb 2011/03/03
 */

#ifndef _IntensityTier_h_
	#include "IntensityTier.h"
#endif
#ifndef _TableOfReal_h_
	#include "TableOfReal.h"
#endif
#ifndef _Sound_h_
	#include "Sound.h"
#endif

#ifdef __cplusplus
	extern "C" {
#endif

/********** class AmplitudeTier **********/

#define AmplitudeTier_members RealTier_members
#define AmplitudeTier_methods RealTier_methods
class_create (AmplitudeTier, RealTier);

AmplitudeTier AmplitudeTier_create (double tmin, double tmax);

void AmplitudeTier_draw (AmplitudeTier me, Graphics g, double tmin, double tmax,
	double ymin, double ymax, const wchar_t *method, int garnish);

AmplitudeTier PointProcess_upto_AmplitudeTier (PointProcess me, double soundPressure);
AmplitudeTier IntensityTier_to_AmplitudeTier (IntensityTier me);
IntensityTier AmplitudeTier_to_IntensityTier (AmplitudeTier me, double threshold_dB);
TableOfReal AmplitudeTier_downto_TableOfReal (AmplitudeTier me);
void Sound_AmplitudeTier_multiply_inline (Sound me, AmplitudeTier intensity);
Sound Sound_AmplitudeTier_multiply (Sound me, AmplitudeTier intensity);

AmplitudeTier PointProcess_Sound_to_AmplitudeTier_point (PointProcess me, Sound thee);
AmplitudeTier PointProcess_Sound_to_AmplitudeTier_period (PointProcess me, Sound thee,
	double tmin, double tmax, double shortestPeriod, double longestPeriod, double maximumPeriodFactor);
double AmplitudeTier_getShimmer_local (AmplitudeTier me, double shortestPeriod, double longestPeriod, double maximumAmplitudeFactor);
double AmplitudeTier_getShimmer_local_dB (AmplitudeTier me, double shortestPeriod, double longestPeriod, double maximumAmplitudeFactor);
double AmplitudeTier_getShimmer_apq3 (AmplitudeTier me, double shortestPeriod, double longestPeriod, double maximumAmplitudeFactor);
double AmplitudeTier_getShimmer_apq5 (AmplitudeTier me, double shortestPeriod, double longestPeriod, double maximumAmplitudeFactor);
double AmplitudeTier_getShimmer_apq11 (AmplitudeTier me, double shortestPeriod, double longestPeriod, double maximumAmplitudeFactor);
double AmplitudeTier_getShimmer_dda (AmplitudeTier me, double shortestPeriod, double longestPeriod, double maximumAmplitudeFactor);

Sound AmplitudeTier_to_Sound (AmplitudeTier me, double samplingFrequency, long interpolationDepth);

#ifdef __cplusplus
	}
#endif

/* End of file AmplitudeTier.h */
#endif
