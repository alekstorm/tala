/* praat_LPC_init.c
 *
 * Copyright (C) 1994-2010 David Weenink
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 djmw 20030613 Latest modification
 djmw 20040414 Forms texts.
 djmw 20060428 Latest modification
 djmw 20061218 Changed to Melder_information<x> format.
 djmw 20070902 Melder_error<1...>
 djmw 20071011 REQUIRE requires L"".
 djmw 20080313 Cepstrum_formula
 djmw 20100212 Analysis window length is now "Window length"
*/

#include <math.h>
#include "praat.h"
#include "Cepstrumc.h"
#include "DTW.h"
#include "LPC.h"
#include "MFCC.h"
#include "LFCC.h"
#include "LPC_and_Cepstrumc.h"
#include "LPC_and_Formant.h"
#include "LPC_and_LFCC.h"
#include "LPC_and_Polynomial.h"
#include "LPC_and_Tube.h"
#include "LPC_to_Spectrogram.h"
#include "LPC_to_Spectrum.h"
#include "MelFilter_and_MFCC.h"
#include "Sound_and_LPC.h"
#include "Sound_and_LPC_robust.h"
#include "Sound_and_Cepstrum.h"
#include "Sound_to_MFCC.h"
#include "Cepstrum_and_Spectrum.h"

static wchar_t *DRAW_BUTTON    = L"Draw -";
static wchar_t *QUERY_BUTTON   = L"Query -";

extern void praat_CC_init (void *klas);
extern void praat_TimeFrameSampled_query_init (void *klas);
extern int praat_Fon_formula (Any dia);

/********************** Cepstrum  ****************************************/


DIRECT (Cepstrum_help)
	Melder_help (L"Cepstrum");
END

FORM (Cepstrum_draw, L"Cepstrum: Draw", L"Cepstrum: Draw...")
	REAL (L"Minimum quefrency", L"0.0")
	REAL (L"Maximum quefrency", L"0.0")
	REAL (L"Minimum", L"0.0")
	REAL (L"Maximum", L"0.0")
    BOOLEAN (L"Garnish", 0)
	OK
DO
	EVERY_DRAW (Cepstrum_draw (OBJECT, GRAPHICS,
		GET_REAL (L"Minimum quefrency"), GET_REAL (L"Maximum quefrency"),
		GET_REAL (L"Minimum"), GET_REAL (L"Maximum"),
		GET_INTEGER (L"Garnish")))
END

FORM (Cepstrum_formula, L"Cepstrum: Formula...", L"Cepstrum: Formula...")
	LABEL (L"label", L"y := y1; for row := 1 to nrow do { x := x1; "
		"for col := 1 to ncol do { self [row, col] := `formula' ; x := x + dx } y := y + dy }")
	TEXTFIELD (L"formula", L"self")
	OK
DO
	if (! praat_Fon_formula (dia)) return 0;
END


DIRECT (Cepstrum_to_Spectrum)
	EVERY_TO (Cepstrum_to_Spectrum (OBJECT))
END

DIRECT (Cepstrum_to_Matrix)
	EVERY_TO (Cepstrum_to_Matrix (OBJECT))
END

/********************** Cepstrumc  ****************************************/

DIRECT (Cepstrumc_to_LPC)
	EVERY_TO (Cepstrumc_to_LPC (OBJECT))
END

FORM (Cepstrumc_to_DTW, L"Cepstrumc: To DTW", L"Cepstrumc: To DTW...")
	LABEL (L"", L"Distance calculation between Cepstra")
	REAL (L"Cepstral weight", L"1.0")
	REAL (L"Log energy weight", L"0.0")
	REAL (L"Regression weight", L"0.0")
	REAL (L"Regression weight log energy", L"0.0")
	REAL (L"Window for regression coefficients (seconds)", L"0.056")
	LABEL (L"", L"Boundary conditions for time warp")
	BOOLEAN (L"Match begin positions", 0)
	BOOLEAN (L"Match end positions", 0)
	RADIO (L"Slope constraints", 1)
	RADIOBUTTON (L"no restriction")
	RADIOBUTTON (L"1/3 < slope < 3")
	RADIOBUTTON (L"1/2 < slope < 2")
	RADIOBUTTON (L"2/3 < slope < 3/2")
	OK
DO
	Cepstrumc c1 = NULL, c2 = NULL;
	WHERE (SELECTED && CLASS == classCepstrumc) { if (c1) c2 = OBJECT; else c1 = OBJECT; }
	NEW (Cepstrumc_to_DTW (c1, c2, GET_REAL (L"Cepstral weight"),
		GET_REAL (L"Log energy weight"), GET_REAL (L"Regression weight"),
		GET_REAL (L"Regression weight log energy"),
		GET_REAL (L"Window for regression coefficients"),
		GET_INTEGER (L"Match begin positions"), GET_INTEGER(L"Match end positions"),
		GET_INTEGER(L"Slope constraints")))
END

DIRECT (Cepstrumc_to_Matrix)
	EVERY_TO (Cepstrumc_to_Matrix (OBJECT))
END

/******************** Formant ********************************************/

FORM (Formant_to_LPC, L"Formant: To LPC", 0)
	POSITIVE (L"Sampling frequency (Hz)", L"16000.0")
	OK
DO
	EVERY_TO (Formant_to_LPC (OBJECT, 1.0/GET_REAL (L"Sampling frequency")))
END

/********************LFCC ********************************************/

DIRECT (LFCC_help)
	Melder_help (L"LFCC");
END

FORM (LFCC_to_LPC, L"LFCC: To LPC", L"LFCC: To LPC...")
	INTEGER (L"Number of coefficients", L"0")
	OK
DO
	long ncof = GET_INTEGER (L"Number of coefficients");
	REQUIRE (ncof >= 0, L"Number of coefficients must be greater or equal zero.")
	EVERY_TO (LFCC_to_LPC (OBJECT, ncof))
END

/********************LPC ********************************************/

DIRECT (LPC_help) Melder_help (L"LPC"); END

FORM (LPC_drawGain, L"LPC: Draw gain", L"LPC: Draw gain...")
    REAL (L"From time (seconds)", L"0.0")
    REAL (L"To time (seconds)", L"0.0 (=all)")
    REAL (L"Minimum gain", L"0.0")
    REAL (L"Maximum gain", L"0.0")
    BOOLEAN (L"Garnish", 1)
	OK
DO
    EVERY_DRAW (LPC_drawGain (OBJECT, GRAPHICS,
    	GET_REAL (L"From time"), GET_REAL (L"To time"),
    	GET_REAL (L"Minimum gain"), GET_REAL (L"Maximum gain"),
		GET_INTEGER(L"Garnish")))
END

DIRECT (LPC_getSamplingInterval)
	LPC me = ONLY (classLPC);
	Melder_information2 (Melder_double (my samplingPeriod), L" seconds");
END

FORM (LPC_getNumberOfCoefficients, L"LPC: Get number of coefficients", L"LPC: Get number of coefficients...")
	NATURAL (L"Frame number", L"1")
	OK
DO
	LPC me = ONLY (classLPC);
	long iframe = GET_INTEGER (L"Frame number");
	if (iframe > my nx)
	{
		(void) Melder_error3 (L"Frame number is too large.\n\nPlease choose a number between 1 and ", Melder_integer (my nx), L".");
		Melder_information1 (L"-1 coefficients (frame number was not defined)");
		return 0;
	}
	Melder_information2 (Melder_integer ((my frame[iframe]).nCoefficients), L" coefficients");
END

FORM (LPC_drawPoles, L"LPC: Draw poles", L"LPC: Draw poles...")
    REAL (L"Time (seconds)", L"0.0")
    BOOLEAN (L"Garnish", 1)
    OK
DO
	EVERY_DRAW (LPC_drawPoles (OBJECT, GRAPHICS, GET_REAL (L"Time"),
		GET_INTEGER (L"Garnish")))
END

DIRECT (LPC_to_Formant)
	EVERY_TO (LPC_to_Formant (OBJECT, 50))
END

DIRECT (LPC_to_Formant_keep_all)
	EVERY_TO (LPC_to_Formant (OBJECT, 0))
END

FORM (LPC_to_LFCC, L"LPC: To LFCC", L"LPC: To LFCC...")
	INTEGER (L"Number of coefficients", L"0")
	OK
DO
	long ncof = GET_INTEGER (L"Number of coefficients");
	REQUIRE (ncof >= 0, L"Number of coefficients must be greater or equal zero.")
	EVERY_TO (LPC_to_LFCC (OBJECT, ncof))
END

FORM (LPC_to_Polynomial, L"LPC: To Polynomial", L"LPC: To Polynomial (slice)...")
	REAL (L"Time (seconds)", L"0.0")
	OK
DO
	EVERY_TO (LPC_to_Polynomial (OBJECT, GET_REAL (L"Time")))
END

FORM (LPC_to_Spectrum, L"LPC: To Spectrum", L"LPC: To Spectrum (slice)...")
	REAL (L"Time (seconds)", L"0.0")
	REAL (L"Minimum frequency resolution (Hz)", L"20.0")
	REAL (L"Bandwidth reduction (Hz)", L"0.0")
	REAL (L"De-emphasis frequency (Hz)", L"50.0")
	OK
DO
	EVERY_TO (LPC_to_Spectrum (OBJECT, GET_REAL (L"Time"),
		GET_REAL (L"Minimum frequency resolution"),
		GET_REAL (L"Bandwidth reduction"), GET_REAL (L"De-emphasis frequency")))
END

FORM (LPC_to_Spectrogram, L"LPC: To Spectrogram", L"LPC: To Spectrogram...")
	REAL (L"Minimum frequency resolution (Hz)", L"20.0")
	REAL (L"Bandwidth reduction (Hz)", L"0.0")
	REAL (L"De-emphasis frequency (Hz)", L"50.0")
	OK
DO
	EVERY_TO (LPC_to_Spectrogram (OBJECT, GET_REAL (L"Minimum frequency resolution"),
		GET_REAL (L"Bandwidth reduction"), GET_REAL (L"De-emphasis frequency")))
END

FORM (LPC_to_VocalTract, L"LPC: To VocalTract", L"LPC: To VocalTract (slice)...")
	REAL (L"Time (s)", L"0.0")
	POSITIVE (L"Length (m)", L"0.17")
	BOOLEAN (L"Length according to Wakita", 0)
	OK
DO
	EVERY_TO (LPC_to_VocalTract (OBJECT, GET_REAL (L"Time"), GET_REAL (L"Length"),
		GET_INTEGER(L"Length according to Wakita")))
END

DIRECT (LPC_to_Matrix)
	EVERY_TO (LPC_to_Matrix (OBJECT))
END

/********************** Sound *******************************************/

static void Sound_to_LPC_addCommonFields (void *dia) {
	LABEL (L"", L"Warning 1:  for formant analysis, use \"To Formant\" instead.")
	LABEL (L"", L"Warning 2:  if you do use \"To LPC\", you may want to resample first.")
	LABEL (L"", L"Click Help for more details.")
	LABEL (L"", L"")
	NATURAL (L"Prediction order", L"16")
	POSITIVE (L"Window length (s)", L"0.025")
	POSITIVE (L"Time step (s)", L"0.005")
	REAL (L"Pre-emphasis frequency (Hz)", L"50.0")
}
static int Sound_to_LPC_checkCommonFields (void * dia,
	long *predictionOrder,
	double *analysisWindowDuration,
	double *timeStep,
	double *preemphasisFrequency)
{
	*predictionOrder = GET_INTEGER (L"Prediction order");
	*analysisWindowDuration = GET_REAL (L"Window length");
	*timeStep = GET_REAL (L"Time step");
	*preemphasisFrequency = GET_REAL (L"Pre-emphasis frequency");
	if (*preemphasisFrequency < 0.0) {
		(void) Melder_error1 (L"Pre-emphasis frequencies cannot be negative.\n");
		return Melder_error1 (L"Please use a positive or zero pre-emphasis frequency.");
	}
	return 1;
}

FORM (Sound_to_LPC_auto, L"Sound: To LPC (autocorrelation)", L"Sound: To LPC (autocorrelation)...")
	Sound_to_LPC_addCommonFields (dia);
	OK
DO
	long numberOfPoles;
	double analysisWindowDuration, timeStep, preemphasisFrequency;
	if (! Sound_to_LPC_checkCommonFields (dia, & numberOfPoles, & analysisWindowDuration, & timeStep, & preemphasisFrequency))
		return 0;
	EVERY_TO (Sound_to_LPC_auto (OBJECT, numberOfPoles, analysisWindowDuration, timeStep, preemphasisFrequency))
END

FORM (Sound_to_LPC_covar, L"Sound: To LPC (covariance)", L"Sound: To LPC (covariance)...")
	Sound_to_LPC_addCommonFields (dia);
	OK
DO
	long numberOfPoles;
	double analysisWindowDuration, timeStep, preemphasisFrequency;
	if (! Sound_to_LPC_checkCommonFields (dia, & numberOfPoles, & analysisWindowDuration, & timeStep, & preemphasisFrequency))
		return 0;
	EVERY_TO (Sound_to_LPC_covar (OBJECT, numberOfPoles, analysisWindowDuration, timeStep, preemphasisFrequency))
END

FORM (Sound_to_LPC_burg, L"Sound: To LPC (burg)", L"Sound: To LPC (burg)...")
	Sound_to_LPC_addCommonFields (dia);
	OK
DO
	long numberOfPoles;
	double analysisWindowDuration, timeStep, preemphasisFrequency;
	if (! Sound_to_LPC_checkCommonFields (dia, & numberOfPoles, & analysisWindowDuration, & timeStep, & preemphasisFrequency))
		return 0;
	EVERY_TO (Sound_to_LPC_burg (OBJECT, numberOfPoles, analysisWindowDuration, timeStep, preemphasisFrequency))
END

FORM (Sound_to_LPC_marple, L"Sound: To LPC (marple)", L"Sound: To LPC (marple)...")
	Sound_to_LPC_addCommonFields (dia);
	POSITIVE (L"Tolerance 1", L"1e-6")
	POSITIVE (L"Tolerance 2", L"1e-6")
	OK
DO
	long numberOfPoles;
	double analysisWindowDuration, timeStep, preemphasisFrequency;
	if (! Sound_to_LPC_checkCommonFields (dia, & numberOfPoles, & analysisWindowDuration, & timeStep, & preemphasisFrequency))
		return 0;
	EVERY_TO (Sound_to_LPC_marple (OBJECT, numberOfPoles, analysisWindowDuration, timeStep, preemphasisFrequency,
		GET_REAL (L"Tolerance 1"), GET_REAL (L"Tolerance 2")))
END

FORM (Sound_to_MFCC, L"Sound: To MFCC", L"Sound: To MFCC...")
	NATURAL (L"Number of coefficients", L"12")
	POSITIVE (L"Window length (s)", L"0.015")
	POSITIVE (L"Time step (s)", L"0.005")
	LABEL (L"", L"Filter bank parameters")
	POSITIVE (L"Position of first filter (mel)", L"100.0")
	POSITIVE (L"Distance between filters (mel)", L"100.0")
	REAL (L"Maximum frequency (mel)", L"0.0");
	OK
DO
	long p = GET_INTEGER (L"Number of coefficients");
	REQUIRE (p < 25, L"Number of coefficients must be < 25.")
	EVERY_TO (Sound_to_MFCC (OBJECT, p, GET_REAL (L"Window length"),
		GET_REAL (L"Time step"), GET_REAL (L"Position of first filter"),
		GET_REAL (L"Maximum frequency"), GET_REAL (L"Distance between filters")))
END

DIRECT (VocalTract_getLength)
	VocalTract v = ONLY_OBJECT;
    double length = v -> xmax - v -> xmin;
    if (length <= 0.02) length = NUMundefined;
	Melder_information2 (Melder_integer (length), L" m");
END

/******************* LPC & Sound *************************************/

FORM (LPC_and_Sound_filter, L"LPC & Sound: Filter", L"LPC & Sound: Filter...")
	BOOLEAN (L"Use LPC gain", 0)
	OK
DO
	NEW (LPC_and_Sound_filter (ONLY(classLPC) , ONLY(classSound),
		GET_INTEGER (L"Use LPC gain")))
END

FORM (LPC_and_Sound_filterWithFilterAtTime, L"LPC & Sound: Filter with one filter at time",
	  L"LPC & Sound: Filter with filter at time...")
	OPTIONMENU (L"Channel", 2)
		OPTION (L"Both")
		OPTION (L"Left")
		OPTION (L"Right")
	REAL (L"Use filter at time (s)", L"0.0")
	OK
DO
	long channel = GET_INTEGER (L"Channel") - 1;
	NEW (LPC_and_Sound_filterWithFilterAtTime (ONLY(classLPC) , ONLY(classSound), channel,
		GET_REAL (L"Use filter at time")))
END

DIRECT (LPC_and_Sound_filterInverse)
	NEW (LPC_and_Sound_filterInverse (ONLY(classLPC) , ONLY(classSound)))
END

FORM (LPC_and_Sound_filterInverseWithFilterAtTime, L"LPC & Sound: Filter (inverse) with filter at time",
	L"LPC & Sound: Filter (inverse) with filter at time...")
	OPTIONMENU (L"Channel", 2)
		OPTION (L"Both")
		OPTION (L"Left")
		OPTION (L"Right")
	REAL (L"Use filter at time (s)", L"0.0")
	OK
DO
	long channel = GET_INTEGER (L"Channel") - 1;
	NEW (LPC_and_Sound_filterInverseWithFilterAtTime (ONLY(classLPC) , ONLY(classSound), channel,
		GET_REAL (L"Use filter at time")))
END

FORM (LPC_and_Sound_to_LPC_robust, L"Robust LPC analysis", L"LPC & Sound: To LPC (robust)...")
	POSITIVE (L"Window length (s)", L"0.025")
	POSITIVE (L"Pre-emphasis frequency (Hz)", L"50.0")
	POSITIVE (L"Number of std. dev.", L"1.5")
	NATURAL (L"Maximum number of iterations", L"5")
	REAL (L"Tolerance", L"0.000001")
	BOOLEAN (L"Variable location", 0)
	OK
DO
	Sound sound = ONLY (classSound);
	LPC lpc = ONLY (classLPC);
	if (! praat_new2 (LPC_and_Sound_to_LPC_robust (lpc, sound,
		GET_REAL (L"Window length"), GET_REAL (L"Pre-emphasis frequency"),
		GET_REAL (L"Number of std. dev."), GET_INTEGER (L"Maximum number of iterations"),
		GET_REAL (L"Tolerance"), GET_INTEGER (L"Variable location")),
			lpc -> name, L"_r")) return 0;
END

void praat_uvafon_LPC_init (void);
void praat_uvafon_LPC_init (void)
{
	Thing_recognizeClassesByName (classCepstrumc, classLPC, classLFCC, classMFCC, NULL);

	praat_addAction1 (classCepstrum, 0, L"Cepstrum help", 0, 0, DO_Cepstrum_help);
	praat_addAction1 (classCepstrum, 0, L"Draw...", 0, 0, DO_Cepstrum_draw);
	praat_addAction1 (classCepstrum, 0, L"Formula...", 0, 0, DO_Cepstrum_formula);
	praat_addAction1 (classCepstrum, 0, L"To Spectrum", 0, 0, DO_Cepstrum_to_Spectrum);
	praat_addAction1 (classCepstrum, 0, L"To Matrix", 0, 0, DO_Cepstrum_to_Matrix);


	praat_addAction1 (classCepstrumc, 0, L"Analyse", 0, 0, 0);
	praat_addAction1 (classCepstrumc, 0, L"To LPC", 0, 0, DO_Cepstrumc_to_LPC);
	praat_addAction1 (classCepstrumc, 2, L"To DTW...", 0, 0, DO_Cepstrumc_to_DTW);
	praat_addAction1 (classCepstrumc, 0, L"Hack", 0, 0, 0);
	praat_addAction1 (classCepstrumc, 0, L"To Matrix", 0, 0, DO_Cepstrumc_to_Matrix);

	praat_addAction1 (classFormant, 0, L"Analyse", 0, 0, 0);
	praat_addAction1 (classFormant, 0, L"To LPC...", 0, 0, DO_Formant_to_LPC);

	praat_addAction1 (classLFCC, 0, L"LFCC help", 0, 0, DO_LFCC_help);
	praat_CC_init (classLFCC);
	praat_addAction1 (classLFCC, 0, L"To LPC...", 0, 0, DO_LFCC_to_LPC);

	praat_addAction1 (classLPC, 0, L"LPC help", 0, 0, DO_LPC_help);
	praat_addAction1 (classLPC, 0, DRAW_BUTTON, 0, 0, 0);
	praat_addAction1 (classLPC, 0, L"Draw gain...", 0, 1, DO_LPC_drawGain);
	praat_addAction1 (classLPC, 0, L"Draw poles...", 0, 1, DO_LPC_drawPoles);
	praat_addAction1 (classLPC, 0, QUERY_BUTTON, 0, 0, 0);
	praat_TimeFrameSampled_query_init (classLPC);
	praat_addAction1 (classLPC, 1, L"Get sampling interval", 0, 1, DO_LPC_getSamplingInterval);
	praat_addAction1 (classLPC, 1, L"Get number of coefficients...", 0, 1, DO_LPC_getNumberOfCoefficients);
	praat_addAction1 (classLPC, 0, L"Extract", 0, 0, 0);

	praat_addAction1 (classLPC, 0, L"To Spectrum (slice)...", 0, 0, DO_LPC_to_Spectrum);
	praat_addAction1 (classLPC, 0, L"To VocalTract (slice)...", 0, 0, DO_LPC_to_VocalTract);
	praat_addAction1 (classLPC, 0, L"To Polynomial (slice)...", 0, 0, DO_LPC_to_Polynomial);
	praat_addAction1 (classLPC, 0, L"To Matrix", 0, 0, DO_LPC_to_Matrix);
	praat_addAction1 (classLPC, 0, L"Analyse", 0, 0, 0);
	praat_addAction1 (classLPC, 0, L"To Formant", 0, 0, DO_LPC_to_Formant);
	praat_addAction1 (classLPC, 0, L"To Formant (keep all)", 0, 0, DO_LPC_to_Formant_keep_all);
	praat_addAction1 (classLPC, 0, L"To LFCC...", 0, 0, DO_LPC_to_LFCC);
	praat_addAction1 (classLPC, 0, L"To Spectrogram...", 0, 0, DO_LPC_to_Spectrogram);

	praat_addAction2 (classLPC, 1, classSound, 1, L"Analyse", 0, 0, 0);
	praat_addAction2 (classLPC, 1, classSound, 1, L"Filter...", 0, 0, DO_LPC_and_Sound_filter);
	praat_addAction2 (classLPC, 1, classSound, 1, L"Filter (inverse)", 0, 0, DO_LPC_and_Sound_filterInverse);
	praat_addAction2 (classLPC, 1, classSound, 1, L"To LPC (robust)...", 0, praat_HIDDEN + praat_DEPTH_1, DO_LPC_and_Sound_to_LPC_robust);
	praat_addAction2 (classLPC, 1, classSound, 1, L"Filter with filter at time...", 0, 0, DO_LPC_and_Sound_filterWithFilterAtTime);
	praat_addAction2 (classLPC, 1, classSound, 1, L"Filter (inverse) with filter at time...", 0, 0, DO_LPC_and_Sound_filterInverseWithFilterAtTime);


	praat_addAction1 (classSound, 0, L"To LPC (autocorrelation)...", L"To Formant (sl)...", 1, DO_Sound_to_LPC_auto);
	praat_addAction1 (classSound, 0, L"To LPC (covariance)...", L"To LPC (autocorrelation)...", 1, DO_Sound_to_LPC_covar);
	praat_addAction1 (classSound, 0, L"To LPC (burg)...", L"To LPC (covariance)...", 1, DO_Sound_to_LPC_burg);
	praat_addAction1 (classSound, 0, L"To LPC (marple)...", L"To LPC (burg)...", 1, DO_Sound_to_LPC_marple);
	praat_addAction1 (classSound, 0, L"To MFCC...",L"To LPC (marple)...", 1, DO_Sound_to_MFCC);

	praat_addAction1 (classVocalTract, 1, L"Get length", L"Draw", 0, DO_VocalTract_getLength);

	INCLUDE_MANPAGES (manual_LPC_init)

}

/* End of file praat_LPC_init.c */
