/* Sound_to_Pitch.h
 *
 * Copyright (C) 1992-2011 Paul Boersma
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

#include "Sound.h"
#include "Pitch.h"

#ifdef __cplusplus
	extern "C" {
#endif

Pitch Sound_to_Pitch (Sound me, double timeStep,
	double minimumPitch, double maximumPitch);
/* Calls Sound_to_Pitch_ac with default arguments. */

Pitch Sound_to_Pitch_ac (Sound me, double timeStep, double minimumPitch,
	double periodsPerWindow, int maxnCandidates, int accurate,
	double silenceThreshold, double voicingThreshold, double octaveCost,
	double octaveJumpCost, double voicedUnvoicedCost, double maximumPitch);
/* Calls Sound_to_Pitch_any with AC method. */

Pitch Sound_to_Pitch_cc (Sound me, double timeStep, double minimumPitch,
	double periodsPerWindow, int maxnCandidates, int accurate,
	double silenceThreshold, double voicingThreshold, double octaveCost,
	double octaveJumpCost, double voicedUnvoicedCost, double maximumPitch);
/* Calls Sound_to_Pitch_any with FCC method. */

Pitch Sound_to_Pitch_any (Sound me,

	double dt,                 /* time step (seconds); 0.0 = automatic = periodsPerWindow / minimumPitch / 4 */
	double minimumPitch,       /* (Hertz) */
	double periodsPerWindow,   /* ac3 for pitch analysis, 6 or 4.5 for HNR, 1 for FCC */
	int maxnCandidates,        /* maximum number of candidates per frame */
	int method,                /* 0 or 1 = AC, 2 or 3 = FCC, 0 or 2 = fast, 1 or 3 = accurate */

	double silenceThreshold,   /* relative to purely periodic; default 0.03 */
	double voicingThreshold,   /* relative to purely periodic; default 0.45 */
	double octaveCost,         /* favours higher pitches; default 0.01 */
	double octaveJumpCost,     /* default 0.35 */
	double voicedUnvoicedCost, /* default 0.14 */
	double maximumPitch);      /* (Hertz) */
/*
	Function:
		acoustic periodicity analysis.
	Preconditions:
		minimumPitch > 0.0;
		maxnCandidates >= 2;
	Return value:
		the resulting pitch contour, or NULL in case of failure.
	Failures:
		Out of memory.
		Minimum frequency too low.
		Maximum frequency should not be greater than the Sound's Nyquist frequency.
	Description for method 0 or 1:
		There is a Hanning window (method == 0) or Gaussian window (method == 1)
		over the analysis window, in order to avoid phase effects.
		Zeroes are appended to the analysis window to avoid edge effects in the FFT.
		An FFT is done on the window, giving a complex spectrum.
		This complex spectrum is squared, thus giving the power spectrum.
		The power spectrum is FFTed back, thus giving the autocorrelation of
		the windowed frame. This autocorrelation is expressed relative to the power,
		which is the autocorrelation for lag 0. The autocorrelation is divided by
		the normalized autocorrelation of the window, in order to bring
		all maxima of the autocorrelation of a periodic signal to the same height.
	General description:
		The maxima are found by sinc interpolation.
		The pitch values (frequencies) of the highest 'maxnCandidates' maxima
		are saved in 'result', together with their strengths (relative correlations).
		A Viterbi algorithm is used to find a smooth path through the candidates,
		using the last six arguments of this function.

		The 'maximumPitch' argument has no influence on the search for candidates.
		It is directly copied into the Pitch object as a hint for considering
		pitches above a certain value "voiceless".
*/

#ifdef __cplusplus
	}
#endif

/* End of file Sound_to_Pitch.h */
