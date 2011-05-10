/* praat_David_init.c
 *
 * Copyright (C) 1993-2011 David Weenink
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
 djmw 20030701 Added Strings_setString.
 djmw 20031020 Changed Matrix_solveEquation.
 djmw 20031023 Added Spectra_multiply, Spectrum_conjugate and modified interface for CCA_and_TableOfReal_scores.
 djmw 20031030 Added TableOfReal_appendColumns.
 djmw 20031107 Added TablesOfReal_to_GSVD.
 djmw 20040303 Latest modification
 djmw 20040305 Added hints for PCA.
 djmw 20040323 Added hint for Discriminant.
 djmw 20040324 Added PCA_and_TableOfReal_getFractionVariance.
 djmw 20040331 Modified Eigen_drawEigenvalues interface.
 djmw 20040406 Extensive checks for creation of Sounds.
 djmw 20040414 Forms texts.
 djmw 20040523 Discriminant_and_TableOfReal_to_ClassificationTable: give new object a name.
 djmw 20040623 Added ClassificationTable_to_Strings_maximumProbability.
 djmw 20040704 BarkFilter... in Thing_recognizeClassesByName.
 djmw 20041020 MelderFile -> structMelderFile.
 djmw 20041105 TableOfReal_createFromVanNieropData_25females.
 djmw 20041108 FormantFilter_drawSpectrum bug correted (wrong field name).
 djmw 20050308 Find path (slopes), Find path (band)... and others.
 djmw 20050404 TableOfReal_appendColumns -> TableOfReal_appendColumnsMany
 djmw 20050406 Procrustus -> Prorustes
 djmw 20050407 MelFilter_drawFilterFunctions error in field names crashed praat
 djmw 20050706 Eigen_getSumOfEigenvalues
 djmw 20051012 Robust LPC analysis test
 djmw 20051116 TableOfReal_drawScatterPlot horizontal and vertical axes indices must be positive numbers
 djmw SVD extract lef/right singular vectors
 djmw 20060111 TextGrid: Extend time moved from depth 1 to depth 2.
 djmw 20060308 Thing_recognizeClassesByName: StringsIndex, CCA
 djmw 20070206 Sound_changeGender: pitch range factor must be >= 0
 djmw 20070304 Latest modification.
 djmw 20070903 Melder_new<1...>
 djmw 20071011 REQUIRE requires L"ui/editors/AmplitudeTierEditor.h".
 djmw 20071202 Melder_warning<n>
 djmw 20080521 Confusion_drawAsnumbers
 djmw 20090109 KlattGrid formulas for formant
 djmw 20090708 KlattTable <-> Table
 djmw 20090822 Thing_recognizeClassesByName: added classCepstrum, classIndex, classKlattTable
 djmw 20090914 Excitation to Excitations crashed because of NULL reference
 djmw 20090927 TableOfReal_drawRow(s)asHistogram
 djmw 20091023 Sound_draw_selectedIntervals
 djmw 20091230 Covariance_and_TableOfReal_mahalanobis
 djmw 20100212 Standardize on Window length
 djmw 20100511 Categories_getNumberOfCategories
*/

#include "num/NUM2.h"
#include "num/NUMlapack.h"
#include "num/NUMmachar.h"

#include "dwtools/Activation.h"
#include "dwtools/Categories.h"
#include "ui/editors/CategoriesEditor.h"
#include "dwtools/ClassificationTable.h"
#include "dwtools/Confusion.h"
#include "dwtools/Discriminant.h"
#include "dwtools/Eigen_and_Matrix.h"
#include "dwtools/Eigen_and_Procrustes.h"
#include "dwtools/Eigen_and_SSCP.h"
#include "dwtools/Eigen_and_TableOfReal.h"
#include "dwtools/Excitations.h"
#include "dwtools/FormantGrid_extensions.h"
#include "dwtools/Intensity_extensions.h"
#include "dwtools/Matrix_Categories.h"
#include "dwtools/Matrix_extensions.h"
#include "dwtools/LongSound_extensions.h"
#include "ui/editors/KlattGridEditors.h"
#include "dwtools/KlattTable.h"
#include "dwtools/Pattern.h"
#include "dwtools/PCA.h"
#include "dwtools/Polygon_extensions.h"
#include "dwtools/Polynomial.h"
#include "dwtools/Sound_extensions.h"
#include "dwtools/Spectrum_extensions.h"
#include "dwtools/SSCP.h"
#include "dwtools/Strings_extensions.h"
#include "dwtools/Table_extensions.h"
#include "dwtools/TableOfReal_and_Permutation.h"
#include "dwtools/TextGrid_extensions.h"
#include "dwsys/SVD.h"
#include "dwsys/Collection_extensions.h"
#include "dwtools/Minimizers.h"
#include "ui/editors/FormantGridEditor.h"
#include "ui/editors/IntensityTierEditor.h"
#include "ui/editors/PitchTierEditor.h"
#include "fon/Spectrogram.h"
#include "ui/editors/Editor.h"
#include "ui/Formula.h"
#include "ui/GraphicsP.h"
#include "ui/UiFile.h"

#include "dwtools/Categories_and_Strings.h"
#include "dwtools/CCA_and_Correlation.h"
#include "dwtools/CCs_to_DTW.h"
#include "dwtools/Discriminant_Pattern_Categories.h"
#include "dwtools/DTW_and_TextGrid.h"
#include "dwtools/MelFilter_and_MFCC.h"
#include "dwtools/Pitch_extensions.h"
#include "dwtools/Sound_and_FilterBank.h"
#include "dwtools/Sound_to_Pitch2.h"
#include "dwtools/Sound_to_SPINET.h"
#include "dwtools/TableOfReal_and_SVD.h"
#include "ui/editors/VowelEditor.h"
#include "dwsys/Permutation_and_Index.h"
#include "LPC/Cepstrum_and_Spectrum.h"

extern machar_Table NUMfpp;

#include "ui/praat.h"

static wchar_t *QUERY_BUTTON   = L"Query -";
static wchar_t *DRAW_BUTTON    = L"Draw -";
static wchar_t *MODIFY_BUTTON  = L"Modify -";
static wchar_t *EXTRACT_BUTTON = L"Extract -";

extern "C" void praat_TimeFunction_query_init (void *klas);
extern "C" void praat_TimeFrameSampled_query_init (void *klas);
extern "C" void praat_TableOfReal_init (void *klas);
extern "C" void praat_TableOfReal_init2  (void *klas);
extern "C" void praat_SSCP_as_TableOfReal_init (void *klas);

extern "C" void praat_CC_init (void *klas);
void DTW_constraints_addCommonFields (UiForm *dia);
void DTW_constraints_getCommonFields (UiForm *dia, int *begin, int *end, int *slope);
extern "C" void praat_Matrixft_query_init (void *klas);
extern "C" int praat_Fon_formula (UiForm *dia, Interpreter *interpreter);

#undef INCLUDE_DTW_SLOPES

// FIXME
void Matrix_drawRows (I, Graphics g, double xmin, double xmax, double ymin, double ymax,
	double minimum, double maximum);
void Matrix_drawOneContour (I, Graphics g, double xmin, double xmax, double ymin, double ymax,
	double height);
void Matrix_drawContours (I, Graphics g, double xmin, double xmax, double ymin, double ymax,
	double minimum, double maximum);
void Matrix_paintContours (I, Graphics g, double xmin, double xmax, double ymin, double ymax,
	double minimum, double maximum);
void Matrix_paintImage (I, Graphics g, double xmin, double xmax, double ymin, double ymax,
	double minimum, double maximum);
void Matrix_paintCells (I, Graphics g, double xmin, double xmax, double ymin, double ymax,
	double minimum, double maximum);
void Matrix_paintSurface (I, Graphics g, double xmin, double xmax, double ymin, double ymax,
	double minimum, double maximum, double elevation, double azimuth);
void Matrix_movie (I, Graphics g);

void TableOfReal_drawAsNumbers (I, Graphics graphics, long rowmin, long rowmax, int iformat, int precision);

void Sound_drawWhere (Sound me, Graphics g, double tmin, double tmax, double minimum, double maximum,
	bool garnish, const wchar_t *method, long numberOfBisections, const wchar_t *formula, Interpreter *interpreter);
void Sound_paintWhere (Sound me, Graphics g, Graphics_Colour colour, double tmin, double tmax,
	double minimum, double maximum, double level, bool garnish, long numberOfBisections, const wchar_t *formula, Interpreter *interpreter);
void Sounds_paintEnclosed (Sound me, Sound thee, Graphics g, Graphics_Colour colour, double tmin, double tmax,
	double minimum, double maximum, bool garnish);

static int pr_LongSounds_appendToExistingSoundFile (MelderFile file)
{
	int IOBJECT, status = 0;
	Ordered me = Ordered_create ();
	if (me == NULL) return 0;
	WHERE (SELECTED)
		if (! Collection_addItem (me, OBJECT)) goto end;

	status = LongSounds_appendToExistingSoundFile (me, file);
end:
	my size = 0; forget (me);
	return status;
}

static int pr_LongSounds_writeToStereoAudioFile (MelderFile file, int audioFileType)
{
	int IOBJECT;
	LongSound me = NULL, thee = NULL;
	WHERE (SELECTED)
	{
		if (me) thee = (structLongSound *)OBJECT;
		else me = (structLongSound *)OBJECT;
	}
	return LongSounds_writeToStereoAudioFile16 (me, thee, audioFileType, file);
}

/********************** Activation *******************************************/

FORM (Activation_formula, L"Activation: Formula", 0)
	LABEL (L"label", L"for col := 1 to ncol do { self [row, col] := `formula' ; x := x + dx } y := y + dy }}")
	TEXTFIELD (L"formula", L"self")
	OK
DO
	if (! praat_Fon_formula (dia, interpreter)) return 0;
END

DIRECT (Activation_to_Matrix)
	EVERY_TO (Activation_to_Matrix (OBJECT))
END

/********************** BarkFilter *******************************************/

static double scaleFrequency (double f, int scale_from, int scale_to)
{
	double fhz = NUMundefined;
	
	if (scale_from == scale_to) return f;
	if (scale_from == FilterBank_HERTZ)
	{
		fhz = f;
	}
	else if (scale_from == FilterBank_BARK)
	{
		fhz = BARKTOHZ (f);
	}
	else if (scale_from == FilterBank_MEL)
	{
		fhz = MELTOHZ (f);
	}
	
	if (scale_to == FilterBank_HERTZ || fhz == NUMundefined) return fhz;
 
	if (scale_to == FilterBank_BARK)	
	{
		f = HZTOBARK (fhz);
	}
	else if (scale_to == FilterBank_MEL)
	{
		f = HZTOMEL (fhz);
	}
	else
	{
		return NUMundefined;
	}
	return f;
}

static wchar_t *GetFreqScaleText (int scale)
{
	wchar_t *hertz = L"Frequency (Hz)";
	wchar_t *bark = L"Frequency (Bark)";
	wchar_t *mel = L"Frequency (mel)";
	wchar_t *error = L"Frequency (undefined)";
	if (scale == FilterBank_HERTZ)
	{
		return hertz;
	}
	else if (scale == FilterBank_BARK)
	{
		return bark;
	}
	else if (scale == FilterBank_MEL)
	{
		return mel;
	}
	return error;
	
}

static int checkLimits (I, int fromFreqScale, int toFreqScale, int *fromFilter,
	int *toFilter, double *zmin, double *zmax, int dbScale, 
	double *ymin, double *ymax)
{
	iam (Matrix);
	
	if (*fromFilter == 0)  *fromFilter = 1;
	if (*toFilter == 0)  *toFilter = my ny;
	if (*toFilter < *fromFilter)
	{
		*fromFilter = 1;
		*toFilter = my ny;
	}
	if (*fromFilter < 1) *fromFilter = 1;
	if (*toFilter > my ny) *toFilter = my ny;
	if (*fromFilter > *toFilter)
	{
		Melder_warning3 (L"Filter numbers must be in range [1, ", Melder_integer (my ny), L"]");
		return 0;
	}
		
	if (*zmin < 0 || *zmax < 0)
	{
		Melder_warning1 (L"Frequencies must be positive.");
		return 0;
	}
	if (*zmax <= *zmin)
	{
		*zmin = scaleFrequency (my ymin, fromFreqScale, toFreqScale);
		*zmax = scaleFrequency (my ymax, fromFreqScale, toFreqScale); 
	}

	if (*ymax <= *ymin)
	{
		*ymax = 1; *ymin = 0;
		if (dbScale)
		{
			*ymax = 0; *ymin = -60;
		}
	}
	return 1;
}

static double to_dB (double a, double factor, double ref_dB)
{
	if (a <= 0) return ref_dB;
	a = factor * log10 (a);
	if (a < ref_dB) a = ref_dB;
	return a;
}

static void setDrawingLimits (double *a, long n, double amin, double amax,
	long *ibegin, long *iend)
{
	long i, lower = 1;
	
	*ibegin = 0;
	*iend = n + 1;

	for (i = 1; i <= n; i++)
	{
		if (a[i] == NUMundefined)
		{
			if (lower == 0)
			{
				/* high frequency part */
				*iend = i;
				break;
			}
			*ibegin = i;
			continue; 
		}
		lower = 0;
		if (a[i] < amin)
		{
			a[i] = amin; 
		}
		else if (a[i] > amax)
		{
			a[i] = amax;
		}
	}
		
	(*ibegin)++;
	(*iend)--;
}

void FilterBank_drawFrequencyScales (I, Graphics g, int horizontalScale, double xmin, 
	double xmax, int verticalScale, double ymin, double ymax, int garnish)
{
	iam (FilterBank);
	long i, ibegin, iend, n = 2000;
	double *a, df;
	int myFreqScale = FilterBank_getFrequencyScale (me);

	if (xmin < 0 || xmax < 0 ||ymin < 0 || ymax < 0)
	{
		Melder_warning1 (L"Frequencies must be >= 0.");
		return;
	}
	
	if (xmin >= xmax)
	{
		double xmint = my ymin;
		double xmaxt = my ymax;
		if (ymin < ymax)
		{
			xmint = scaleFrequency (ymin, verticalScale, myFreqScale);
			xmaxt = scaleFrequency (ymax, verticalScale, myFreqScale); 
		}
		xmin = scaleFrequency (xmint, myFreqScale, horizontalScale);
		xmax = scaleFrequency (xmaxt, myFreqScale, horizontalScale);
	}
	
	if (ymin >= ymax)
	{
		ymin = scaleFrequency (xmin, horizontalScale, verticalScale); 
		ymax = scaleFrequency (xmax, horizontalScale, verticalScale);
	}
	
	a = NUMdvector (1, n);
	if (a == NULL) return;
	
	Graphics_setInner (g);
	Graphics_setWindow (g, xmin, xmax, ymin, ymax);

	df = (xmax - xmin) / (n - 1);
	
	for (i = 1; i <= n; i++)
	{
		double f = xmin + (i - 1) * df;
		a[i] = scaleFrequency (f, horizontalScale, verticalScale);
	}
	
	setDrawingLimits (a, n, ymin, ymax,	& ibegin, & iend);
	if (ibegin <= iend)
	{
		double fmin = xmin + (ibegin - 1) * df;
		double fmax = xmax - (n - iend) * df;
		Graphics_function (g, a, ibegin, iend, fmin, fmax);
	}
	Graphics_unsetInner (g);
	
	if (garnish)
	{
		Graphics_drawInnerBox (g);
    	Graphics_marksLeft (g, 2, 1, 1, 0);
    	Graphics_textLeft (g, 1, GetFreqScaleText (verticalScale));
    	Graphics_marksBottom (g, 2, 1, 1, 0);
    	Graphics_textBottom (g, 1, GetFreqScaleText (horizontalScale));
	}
	
	NUMdvector_free (a, 1);
}

void BarkFilter_drawSekeyHansonFilterFunctions (BarkFilter me, Graphics g,
	int toFreqScale, int fromFilter, int toFilter, double zmin, double zmax, 
	int dbScale, double ymin, double ymax, int garnish)
{
	long i, j, n = 1000; 
	double *a = NULL;
		
	if (! checkLimits (me, FilterBank_BARK, toFreqScale, & fromFilter, & toFilter, 
		& zmin, & zmax, dbScale, & ymin, & ymax)) return;
	
	a = NUMdvector (1, n);
	if (a == NULL) return;
	
	Graphics_setInner (g);
	Graphics_setWindow (g, zmin, zmax, ymin, ymax);
	
	for (j = fromFilter; j <= toFilter; j++)
	{
		double df = (zmax - zmin) / (n - 1);
		double zMid = Matrix_rowToY (me, j);
		long ibegin, iend;
		
		for (i = 1; i <= n; i++)
		{
			double z, f = zmin + (i - 1) * df;
			
			z = scaleFrequency (f, toFreqScale, FilterBank_BARK);
			if (z == NUMundefined)
			{
				a[i] = NUMundefined;
			}
			else
			{
				z -= zMid + 0.215;
				a[i] = 7 - 7.5 * z - 17.5 * sqrt (0.196 + z * z);
				if (! dbScale) a[i] = pow (10, a[i]);
			}			
		}
		
		setDrawingLimits (a, n, ymin, ymax,	&ibegin, &iend);
		
		if (ibegin <= iend)
		{
			double fmin = zmin + (ibegin - 1) * df;
			double fmax = zmax - (n - iend) * df;
			Graphics_function (g, a, ibegin, iend, fmin, fmax);
		}
	}
		
			
	Graphics_unsetInner (g);
	
	if (garnish)
	{
		double distance = dbScale ? 10 : 1;
		const wchar_t *ytext = dbScale ? L"Amplitude (dB)" : L"Amplitude";
		Graphics_drawInnerBox (g);
    	Graphics_marksBottom (g, 2, 1, 1, 0);
    	Graphics_marksLeftEvery (g, 1, distance, 1, 1, 0);
    	Graphics_textLeft (g, 1, ytext);
    	Graphics_textBottom (g, 1, GetFreqScaleText (toFreqScale));
	}
	
	NUMdvector_free (a, 1);
}


/*
void FilterBank_drawFilters (I, Graphics g, long fromf, long tof,
	double xmin, double xmax, int xlinear, double ymin, double ymax, int ydb,
	double (*tolinf)(double f), double (*tononlin) (double f), 
	double (*filteramp)(double f0, double b, double f))
{
	iam (Matrix);
	

}*/

void Matrix_drawSliceY (I, Graphics g, double x, double ymin, double ymax,
	double min, double max);

void FilterBank_drawTimeSlice (I, Graphics g, double t, double fmin, 
	double fmax, double min, double max, wchar_t *xlabel, int garnish)
{
	iam (Matrix);
	Matrix_drawSliceY (me, g, t, fmin, fmax, min, max);
	if (garnish)
	{
		Graphics_drawInnerBox (g);
		Graphics_marksBottom (g, 2, 1, 1, 0);
		Graphics_marksLeft (g, 2, 1, 1, 0);
		if (xlabel) Graphics_textBottom (g, 0, xlabel);
	}
}

DIRECT (BarkFilter_help)
	Melder_help (L"BarkFilter");
END

FORM (BarkFilter_drawSpectrum, L"BarkFilter: Draw spectrum (slice)", L"FilterBank: Draw spectrum (slice)...")
	POSITIVE (L"Time (s)", L"0.1")
	REAL (L"left Frequency range (Bark)", L"0.0")
	REAL (L"right Frequency range (Bark)", L"0.0")
	REAL (L"left Amplitude range (dB)", L"0.0")
	REAL (L"right Amplitude range (dB)", L"0.0")
	BOOLEAN (L"Garnish", 1)
	OK
DO
	EVERY_DRAW (FilterBank_drawTimeSlice (OBJECT, GRAPHICS,
		GET_REAL (L"Time"), GET_REAL (L"left Frequency range"),
		GET_REAL (L"right Frequency range"), GET_REAL (L"left Amplitude range"),
		GET_REAL (L"right Amplitude range"), L"Barks", GET_INTEGER (L"Garnish")))
END

FORM (BarkFilter_drawSekeyHansonFilterFunctions, L"BarkFilter: Draw filter functions", L"FilterBank: Draw filter functions...")
	INTEGER (L"left Filter range", L"0")
	INTEGER (L"right Filter range", L"0")
	RADIO (L"Frequency scale", 1)
	RADIOBUTTON (L"Hertz")
	RADIOBUTTON (L"Bark")
	RADIOBUTTON (L"mel")
	REAL (L"left Frequency range", L"0.0")
	REAL (L"right Frequency range", L"0.0")
	BOOLEAN (L"Amplitude scale in dB", 1)
	REAL (L"left Amplitude range", L"0.0")
	REAL (L"right Amplitude range", L"0.0")
	BOOLEAN (L"Garnish", 1)
	OK
DO
	EVERY_DRAW (BarkFilter_drawSekeyHansonFilterFunctions ((structBarkFilter *)OBJECT, GRAPHICS,
		GET_INTEGER (L"Frequency scale"),
		GET_INTEGER (L"left Filter range"), GET_INTEGER (L"right Filter range"),
		GET_REAL (L"left Frequency range"), GET_REAL (L"right Frequency range"),
		GET_INTEGER (L"Amplitude scale in dB"),
		GET_REAL (L"left Amplitude range"), GET_REAL (L"right Amplitude range"), GET_INTEGER (L"Garnish")))
END
/********************** Categories  ****************************************/

FORM (Categories_append, L"Categories: Append 1 category", L"Categories: Append 1 category...")
	SENTENCE (L"Category", L"ui/editors/AmplitudeTierEditor.h")
	OK
DO
	if (! OrderedOfString_append ((structOrderedOfString *)ONLY_OBJECT, GET_STRING (L"Category")))
		return 0;
END

DIRECT (Categories_edit)
	if (theCurrentPraatApplication -> batch)
		return Melder_error1 (L"Cannot edit a Categories from batch.");
	else
		WHERE (SELECTED) if (! praat_installEditor (new CategoriesEditor (theCurrentPraatApplication -> topShell,
			FULL_NAME, OBJECT), IOBJECT)) return 0;
END

DIRECT (Categories_getNumberOfCategories)
	Categories c1 = (structCategories *)ONLY_OBJECT;
	Melder_information2 (Melder_integer (c1 -> size), L" categories");
END

DIRECT (Categories_getNumberOfDifferences)
	Categories c1 = NULL, c2 = NULL;
	long NumberOfDifferences;
	WHERE (SELECTED && CLASS == classCategories)
	{
   		if (c1) c2 = (structCategories *)OBJECT; else c1 = (structCategories *)OBJECT;
	}
	NumberOfDifferences = OrderedOfString_getNumberOfDifferences (c1, c2);
	if (NumberOfDifferences< 0) Melder_information1 (L"-1 (undefined: number of elements differ!)");
	else
		Melder_information2 (Melder_integer (NumberOfDifferences), L" differences");
END

DIRECT (Categories_getFractionDifferent)
	Categories c1 = NULL, c2 = NULL;
	WHERE (SELECTED && CLASS == classCategories)
	{
   		if (c1) c2 = (structCategories *)OBJECT; else c1 = (structCategories *)OBJECT;
	}
	Melder_information1 (Melder_double (OrderedOfString_getFractionDifferent (c1,c2)));
END

DIRECT (Categories_difference)
	Categories l1 = NULL, l2 = NULL;
	double fraction;
	long n;
	WHERE (SELECTED && CLASS == classCategories)
	{
   		if (l1) l2 = (structCategories *)OBJECT; else l1 = (structCategories *)OBJECT;
	}
	if (! OrderedOfString_difference (l1, l2, &n, &fraction)) return 0;
	Melder_information2 (Melder_integer (n), L" differences");
END

DIRECT (Categories_selectUniqueItems)
	EVERY_TO ((Categories) Categories_selectUniqueItems ((structCategories *)OBJECT, 1))
END

DIRECT (Categories_to_Confusion)
	Categories c1 = NULL, c2 = NULL;
	int i1 = 0, i2 = 0;
	WHERE (SELECTED)
	{
		if (c1)
		{
			c2 = (structCategories *)OBJECT; i2 = IOBJECT;
		}
		else
		{
			c1 = (structCategories *)OBJECT; i1 = IOBJECT;
		}
	}
	Melder_assert (c1 && c2);
	if (! praat_new3 (Categories_to_Confusion (c1, c2), Thing_getName(c1), L"_", Thing_getName(c2))) return 0;
END

DIRECT (Categories_to_Strings)
	EVERY_TO (Categories_to_Strings ((structCategories *)OBJECT))
END

DIRECT (Categories_join)
	Data l1 = NULL, l2 = NULL;
	WHERE (SELECTED && CLASS == classCategories)
	{
		if (l1) l2 = (structData *)OBJECT; else l1 = (structData *)OBJECT;
	}
	NEW (OrderedOfString_joinItems (l1, l2))
END

DIRECT (Categories_permuteItems)
	Collection c = NULL;
	WHERE (SELECTED && CLASS == classCategories) c = (structCollection *)OBJECT;
	NEW (Collection_permuteItems (c));
END

/***************** CC ****************************************/

void CC_drawC0 (I, Graphics g, double xmin, double xmax, double ymin,
	double ymax, int garnish)
{
	iam (CC);
	double *c;
	long i, bframe, eframe, nframes;
	(void) garnish;
	
	if (xmin >= xmax)
	{
		xmin = my xmin; xmax = my xmax;
	}
	
	nframes = Sampled_getWindowSamples (me, xmin, xmax, &bframe, &eframe);
	
	if ((c = NUMdvector (bframe, eframe)) == NULL) return;
	
	for (i = bframe; i <= eframe; i++)
	{
		CC_Frame cf = & my frame[i];
		c[i] = cf -> c0;
	}
	if (ymin >= ymax)
	{
		NUMdvector_extrema (c, bframe, eframe, &ymin, &ymax);
		if (ymax <= ymin) { ymin -= 1.0; ymax += 1.0; }
	}
	else
	{
		NUMdvector_clip (c, bframe, eframe, ymin, ymax);
	}
	Graphics_setInner (g);
	Graphics_setWindow (g, xmin, xmax, ymin, ymax);
		
	Graphics_function (g, c, bframe, eframe, xmin, xmax);
	Graphics_unsetInner (g);
	
	NUMdvector_free (c, bframe);
}

void CC_paint (I, Graphics g, double xmin, double xmax, long cmin,
	long cmax, double minimum, double maximum, int garnish)
{
	iam (CC);
	Matrix thee = CC_to_Matrix (me);
	
	if (thee == NULL) return;
	
	Matrix_paintCells (thee, g, xmin, xmax, cmin, cmax, minimum, maximum);
	
	if (garnish)
	{
		Graphics_marksBottom (g, 2, 1, 1, 0);
		Graphics_textBottom (g, 1, L"Time (s)");
		Graphics_marksLeft (g, 2, 1, 1, 0);
		Graphics_textLeft (g, 1, L"Coefficients");
	}
	
	forget (thee);
}

FORM (CC_getValue, L"CC: Get value", L"CC: Get value...")
	REAL (L"Time (s)", L"0.1")
	NATURAL (L"Index", L"1")
	OK
DO
	Melder_informationReal (CC_getValue ((structCC *)ONLY_OBJECT, GET_REAL (L"Time"), GET_INTEGER (L"Index")), NULL);
END

FORM (CC_paint, L"CC: Paint", L"CC: Paint...")
	REAL (L"left Time range (s)", L"0.0")
	REAL (L"right Time range (s)", L"0.0")
	INTEGER (L"From coefficient", L"0")
	INTEGER (L"To coefficient", L"0")
	REAL (L"Minimum", L"0.0")
	REAL (L"Maximum", L"0.0")
	BOOLEAN (L"Garnish", 1)
	OK
DO
	EVERY_DRAW (CC_paint ((structCC *)OBJECT, GRAPHICS, GET_REAL (L"left Time range"),
		GET_REAL (L"right Time range"), GET_INTEGER (L"From coefficient"),
		GET_INTEGER (L"To coefficient"), GET_REAL (L"Minimum"),
		GET_REAL (L"Maximum"), GET_INTEGER (L"Garnish")))
END

FORM (CC_drawC0, L"CC: Draw c0", L"CC: Draw c0...")
	REAL (L"left Time range (s)", L"0.0")
	REAL (L"right Time range (s)", L"0.0")
	REAL (L"left Amplitude range", L"0.0")
	REAL (L"right Amplitude range", L"0.0")
	BOOLEAN (L"Garnish", 1)
	OK
DO
	EVERY_DRAW (CC_drawC0 ((structCC *)OBJECT, GRAPHICS, GET_REAL (L"left Time range"),
		GET_REAL (L"right Time range"), GET_REAL (L"left Amplitude range"),
		GET_REAL (L"right Amplitude range"), GET_INTEGER (L"Garnish")))
END

FORM (CCs_to_DTW, L"CC: To DTW", L"CC: To DTW...")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Distance  between cepstral coefficients")
	REAL (L"Cepstral weight", L"1.0")
	REAL (L"Log energy weight", L"0.0")
	REAL (L"Regression weight", L"0.0")
	REAL (L"Regression weight log energy", L"0.0")
	REAL (L"Regression coefficients window (s)", L"0.056")
	DTW_constraints_addCommonFields (dia);
	OK
DO
	CC c1 = NULL, c2 = NULL;
	int begin, end, slope;
	DTW_constraints_getCommonFields (dia, &begin, &end, &slope);

	WHERE (SELECTED && Thing_member (OBJECT, classCC))
	{
		if (c1) c2 = (structCC *)OBJECT; else c1 = (structCC *)OBJECT;
	}
	NEW (CCs_to_DTW (c1, c2, GET_REAL (L"Cepstral weight"),
		GET_REAL (L"Log energy weight"), GET_REAL (L"Regression weight"),
		GET_REAL (L"Regression weight log energy"),
		GET_REAL (L"Regression coefficients window"),begin, end, slope))

END

DIRECT (CC_to_Matrix)
	EVERY_TO (CC_to_Matrix (OBJECT))
END

/******************* class CCA ********************************/

void Eigen_drawEigenvector (I, Graphics g, long ivec, long first, long last,
	double ymin, double ymax, int weigh, double size_mm, const wchar_t *mark,
	int connect, wchar_t **rowLabels, int garnish);

void CCA_drawEigenvector (CCA me, Graphics g, int x_or_y, long ivec, long first, long last,
	double ymin, double ymax, int weigh, double size_mm, const wchar_t *mark, int connect, int garnish)
{
	Eigen e = my x; 
	Strings labels = my xLabels;
	if (x_or_y == 1)
	{
		e = my y; labels = my yLabels;
	}	
	Eigen_drawEigenvector (e, g, ivec, first, last, ymin, ymax, weigh, size_mm, mark,
		connect, labels -> strings, garnish);
}

FORM (CCA_drawEigenvector, L"CCA: Draw eigenvector", L"Eigen: Draw eigenvector...")
	OPTIONMENU (L"X or Y", 1)
	OPTION (L"y")
	OPTION (L"x")
	INTEGER (L"Eigenvector number", L"1")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Multiply by eigenvalue?")
	BOOLEAN (L"Component loadings", 0)
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Select part of the eigenvector:")
	INTEGER (L"left Element range", L"0")
	INTEGER (L"right Element range", L"0")
	REAL (L"left Amplitude range", L"-1.0")
	REAL (L"right Amplitude range", L"1.0")
	POSITIVE (L"Mark size (mm)", L"1.0")
	SENTENCE (L"Mark string (+xo.)", L"+")
	BOOLEAN (L"Connect points", 1)
	BOOLEAN (L"Garnish", 1)
	OK
DO
	EVERY_DRAW (CCA_drawEigenvector ((structCCA *)OBJECT, GRAPHICS, GET_INTEGER (L"X or Y"),
		GET_INTEGER (L"Eigenvector number"),
		GET_INTEGER (L"left Element range"), GET_INTEGER (L"right Element range"),
		GET_REAL (L"left Amplitude range"), GET_REAL (L"right Amplitude range"),
		GET_INTEGER (L"Component loadings"), GET_REAL (L"Mark size"),
		GET_STRING (L"Mark string"), GET_INTEGER (L"Connect points"),
		GET_INTEGER (L"Garnish")))
END

DIRECT (CCA_getNumberOfCorrelations)
	CCA cca = (structCCA *)ONLY(classCCA);
	Melder_information1 (Melder_double (cca->numberOfCoefficients));
END

FORM (CCA_getCorrelationCoefficient, L"CCA: Get canonical correlation coefficient", L"CCA: Get canonical correlation coefficient")
	NATURAL (L"Coefficient number", L"1")
	OK
DO
	Melder_information1 (Melder_double (CCA_getCorrelationCoefficient ((structCCA *)ONLY (classCCA),
		GET_INTEGER (L"Coefficient number"))));
END

FORM (CCA_getEigenvectorElement, L"CCA: Get eigenvector element", L"Eigen: Get eigenvector element...")
	OPTIONMENU (L"X or Y", 1)
	OPTION (L"y")
	OPTION (L"x")
	NATURAL (L"Eigenvector number", L"1")
	NATURAL (L"Element number", L"1")
	OK
DO
	Melder_information1 (Melder_double (CCA_getEigenvectorElement ((structCCA *)ONLY (classCCA),
		GET_INTEGER (L"X or Y"), GET_INTEGER (L"Eigenvector number"),
		GET_INTEGER (L"Element number"))));
END

FORM (CCA_getZeroCorrelationProbability, L"CCA: Get zero correlation probability", L"CCA: Get zero correlation probability...")
	NATURAL (L"Coefficient number", L"1")
	OK
DO
	double p, chisq; long ndf;
	CCA_getZeroCorrelationProbability ((structCCA *)ONLY (classCCA), GET_INTEGER (L"Coefficient number"),
		&chisq, &ndf, &p);
	Melder_information6 (Melder_double (p), L" (=probability for chisq = ", Melder_double (chisq), L" and ndf = ", Melder_integer (ndf), L")");
END

DIRECT (CCA_and_Correlation_factorLoadings)
	CCA cca = (structCCA *)ONLY (classCCA);
	if (! praat_new2 (CCA_and_Correlation_factorLoadings (cca,
		(structCorrelation *)ONLY (classCorrelation)), Thing_getName (cca), L"_loadings")) return 0;
END

FORM (CCA_and_Correlation_getVarianceFraction, L"CCA & Correlation: Get variance fraction", L"CCA & Correlation: Get variance fraction...")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Get the fraction of variance from the data in set...")
	OPTIONMENU (L"X or Y", 1)
	OPTION (L"y")
	OPTION (L"x")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"extracted by...")
	NATURAL (L"left Canonical variate range", L"1")
	NATURAL (L"right Canonical variate range", L"1")
	OK
DO
	int x_or_y = GET_INTEGER (L"X or Y");
	int cv_from = GET_INTEGER (L"left Canonical variate range");
	int cv_to = GET_INTEGER (L"right Canonical variate range");
	Melder_information7 (Melder_double (CCA_and_Correlation_getVarianceFraction ((structCCA *)ONLY (classCCA),
		(structCorrelation *)ONLY (classCorrelation), x_or_y, cv_from, cv_to)), L" (fraction variance from ",
		(x_or_y == 1 ? L"y" : L"x"), L", extracted by canonical variates ", Melder_integer (cv_from), L" to ",
		 Melder_integer (cv_to));
END

FORM (CCA_and_Correlation_getRedundancy_sl, L"CCA & Correlation: Get Stewart-Love redundancy", L"CCA & Correlation: Get redundancy (sl)...")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Get the redundancy of the data in set...")
	OPTIONMENU (L"X or Y", 1)
	OPTION (L"y")
	OPTION (L"x")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"extracted by...")
	NATURAL (L"left Canonical variate range", L"1")
	NATURAL (L"right Canonical variate range", L"1")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"...given the availability of the data in the other set.")
	OK
DO
	int x_or_y = GET_INTEGER (L"X or Y");
	int cv_from = GET_INTEGER (L"left Canonical variate range");
	int cv_to = GET_INTEGER (L"right Canonical variate range");
	Melder_information7 (Melder_double (CCA_and_Correlation_getRedundancy_sl ((structCCA *)ONLY (classCCA), (structCorrelation *)ONLY (classCorrelation),
		x_or_y, cv_from, cv_to)), L" (redundancy from ", (x_or_y == 1 ? L"y" : L"x"), L" extracted by canonical variates ",
		Melder_integer (cv_from), L" to ", Melder_integer (cv_to));
END


DIRECT (CCA_and_TableOfReal_factorLoadings)
	CCA cca = (structCCA *)ONLY (classCCA);
	if (! praat_new2 (CCA_and_TableOfReal_factorLoadings (cca,
		(structTableOfReal *)(structTableOfReal *)ONLY (classTableOfReal)), Thing_getName (cca), L"_loadings")) return 0;
END

FORM (CCA_and_TableOfReal_scores, L"CCA & TableOfReal: To TableOfReal (scores)", L"CCA & TableOfReal: To TableOfReal (scores)...")
	INTEGER (L"Number of canonical correlations", L"0 (=all)")
	OK
DO
	CCA cca = (structCCA *)ONLY (classCCA);
	if (! praat_new2 (CCA_and_TableOfReal_scores (cca, (structTableOfReal *)(structTableOfReal *)ONLY (classTableOfReal),
		GET_INTEGER (L"Number of canonical correlations")),
		Thing_getName (cca), L"_scores")) return 0;
END

FORM (CCA_and_TableOfReal_predict, L"CCA & TableOfReal: Predict", L"CCA & TableOfReal: Predict...")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"The data set from which to predict starts at...")
	INTEGER (L"Column number", L"1")
	OK
DO
	NEW (CCA_and_TableOfReal_predict ((structCCA *)ONLY (classCCA), (structTableOfReal *)ONLY(classTableOfReal),
		GET_INTEGER (L"Column number")))
END

/***************** ChebyshevSeries ****************************************/

DIRECT (ChebyshevSeries_help)
	Melder_help (L"ChebyshevSeries");
END

FORM (ChebyshevSeries_create, L"Create ChebyshevSeries", L"Create ChebyshevSeries...")
	WORD (L"Name", L"cs")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Domain")
	REAL (L"Xmin", L"-1")
	REAL (L"Xmax", L"1")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"ChebyshevSeries(x) = c[1] T[0](x) + c[2] T[1](x) + ... c[n+1] T[n](x)")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"T[k] is a Chebyshev polynomial of degree k")
	SENTENCE (L"Coefficients (c[k])", L"0 0 1.0")
	OK
DO
	double xmin = GET_REAL (L"Xmin"), xmax = GET_REAL (L"Xmax");
	REQUIRE (xmin < xmax, L"Xmin must be smaller than Xmax.")
	if (! praat_new1 (ChebyshevSeries_createFromString (xmin, xmax,
		GET_STRING (L"Coefficients")), GET_STRING (L"Name"))) return 0;
END

DIRECT (ChebyshevSeries_to_Polynomial)
	EVERY_TO (ChebyshevSeries_to_Polynomial ((structChebyshevSeries *)OBJECT))
END

/***************** ClassificationTable ****************************************/

DIRECT (ClassificationTable_help)
	Melder_help (L"ClassificationTable");
END

DIRECT (ClassificationTable_to_Confusion)
	EVERY_TO (ClassificationTable_to_Confusion ((structClassificationTable *)OBJECT))
END

DIRECT (ClassificationTable_to_Correlation_columns)
	EVERY_TO (ClassificationTable_to_Correlation_columns ((structClassificationTable *)OBJECT))
END

DIRECT (ClassificationTable_to_Strings_maximumProbability)
	EVERY_TO (ClassificationTable_to_Strings_maximumProbability ((structClassificationTable *)OBJECT))
END

/********************** Confusion *******************************************/

// option marginals draw one extra row and column with sums.
void Confusion_drawAsNumbers (I, Graphics g, int marginals, int iformat, int precision)
{
	iam (Confusion);
	TableOfReal thee = NULL;
	thee = marginals ? Confusion_to_TableOfReal_marginals (me) : (TableOfReal) me; 
	if (thee == NULL) return;
	TableOfReal_drawAsNumbers (thee, g, 1, thy numberOfRows, iformat, precision);
	if (marginals) forget (thee);
}

DIRECT (Confusion_help)
	Melder_help (L"Confusion");
END

DIRECT (Confusion_to_TableOfReal_marginals)
	EVERY_TO (Confusion_to_TableOfReal_marginals ((structConfusion *)OBJECT))
END

DIRECT (Confusion_difference)
	Confusion c1 = NULL, c2 = NULL;
	WHERE (SELECTED && CLASS == classConfusion) { if (c1) c2 = (structConfusion *)OBJECT; else c1 = (structConfusion *)OBJECT; }
    NEW (Confusion_difference (c1, c2))
END

FORM (Confusion_condense, L"Confusion: Condense", L"Confusion: Condense...")
	SENTENCE (L"Search", L"a")
	SENTENCE (L"Replace", L"a")
	INTEGER (L"Replace limit", L"0 (=unlimited)")
	RADIO (L"Search and replace are", 1)
	RADIOBUTTON (L"Literals")
	RADIOBUTTON (L"Regular Expressions")
	OK
DO
	EVERY_TO (Confusion_condense ((structConfusion *)OBJECT, GET_STRING (L"Search"),
		GET_STRING (L"Replace"), GET_INTEGER (L"Replace limit"),
		GET_INTEGER (L"Search and replace are") - 1))
END

FORM (Confusion_drawAsNumbers, L"ui/editors/AmplitudeTierEditor.h", L"ui/editors/AmplitudeTierEditor.h")
	BOOLEAN (L"Draw marginals", 1)
	RADIO (L"Format", 3)
		RADIOBUTTON (L"decimal")
		RADIOBUTTON (L"exponential")
		RADIOBUTTON (L"free")
		RADIOBUTTON (L"rational")
	NATURAL (L"Precision", L"5")
	OK
DO
	EVERY_DRAW (Confusion_drawAsNumbers ((structConfusion *)OBJECT, GRAPHICS,
		GET_INTEGER (L"Draw marginals"),
		GET_INTEGER (L"Format"), GET_INTEGER (L"Precision")))
END

DIRECT (Confusion_getFractionCorrect)
	double f; long n;
	Confusion_getFractionCorrect ((structConfusion *)ONLY (classConfusion), &f, &n);
	Melder_information2 (Melder_double (f), L" (fraction correct)");
END

/******************* Confusion & Matrix *************************************/

#define Pointer_members Polygon_members
#define Pointer_methods Polygon_methods
class_create (Pointer, Polygon);
class_methods (Pointer, Polygon)
class_methods_end

#define NPOINTS 6
static Any Pointer_create (void)
{
	Pointer me = NULL; long i;
	double x[NPOINTS+1] = { 0, 0, 0.9, 1, 0.9, 0, 0 };
	double y[NPOINTS+1] = { 0, 0, 0, 0.5,   1, 1, 0 };
	if (! (me = (Pointer) Polygon_create (NPOINTS))) { forget (me); return me; }
	for (i = 1; i <= NPOINTS; i++)
	{
		my x[i] = x[i]; my y[i] = y[i];
	}
	return me;
}

static void Pointer_draw (I, Any graphics)
{
	iam (Polygon);
	Graphics_polyline (graphics, my numberOfPoints, & my x[1], & my y[1]);
}

/* 1. Draw my rowLabels centered at ( matrix->z[i][1], matrix->z[i][2]).
 * 2. Draw arrows and circles according to:
 *	for (i=1; i <= my numberOfRows; i++)
 *	{
 *		if (index != 0 && index != i) continue;
 *      draw circle at i of width: my z[i][i]/rowSum;
 *		for (j=1; j <= my numberOfColumns; j++)
 *		{
 *			if (i != j && 100*my data[i][j]/rowSum > lowerPercentage) 
 *				draw arrow from i to j of width: my data[i][j]/rowSum;
 *		}
 *	}
 */
void Confusion_Matrix_draw (Confusion me, Matrix thee, Graphics g, long index,
	double lowerPercentage, double xmin, double xmax,
	double ymin, double ymax, int garnish)
{
	long i, j, ib = 1, ie = my numberOfRows;
	double rmax, rmin;
	
	if (index > 0 && index <= my numberOfColumns) ib = ie = index;
	 
	if(	thy ny != my numberOfRows)
	{
		(void) Melder_error1 (L"Confusion_Matrix_draw: number of positions.");
		return;
	}
	
    if (xmax <= xmin) (void) Matrix_getWindowExtrema (thee, 1, 1, 1, thy ny,
		 &xmin, &xmax);
		 
	if (xmax <= xmin) return;
	 
    if (ymax <= ymin) (void) Matrix_getWindowExtrema (thee, 2, 2, 1, thy ny,
		 &ymin, &ymax);
		 
	if (ymax <= ymin) return;
	rmax = fabs (xmax - xmin) / 10;
	rmin = rmax / 10;
	
    Graphics_setInner (g);
    Graphics_setWindow (g, xmin - rmax, xmax + rmax, ymin - rmax, ymax + rmax); 
    Graphics_setTextAlignment (g, Graphics_CENTRE, Graphics_HALF);
    for (i=1; i <= my numberOfRows; i++)
	{
    	Graphics_text (g, thy z[i][1], thy z[i][2], my rowLabels[i]);
	}
	for (i=ib; i <= ie; i++)
	{
		double x1, y1, r, xSum = 0;
		
		for (j=1; j <= my numberOfColumns; j++)
		{
			xSum += my data[i][j];
		}
		
		if (xSum <= 0) continue; /* no confusions */
		
		x1 = thy z[i][1]; y1 = thy z[i][2];
		r = rmax * my data[i][i] / xSum;
		
		Graphics_circle (g, x1, y1, r > rmin ? r : rmin);
		
		for (j=1; j <= my numberOfColumns; j++)
		{
			Pointer p = NULL;
			double xs, ys;
			double x2 = thy z[j][1], y2 = thy z[j][2];
			double  perc =  100 * my data[i][j] / xSum;
			double dx = x2 - x1, dy = y2 - y1;
			double alpha = atan2 (dy, dx);
			 
			if (perc == 0 || perc < lowerPercentage || j == i) continue;
			
			xmin = x1; xmax = x2;
			if (x2 < x1)
			{
				xmin = x2; xmax = x1;
			}
			ymin = y1; xmax = y2;
			if (y2 < y1)
			{
				ymin = y2; ymax = y1;
			}
			if ((p = (structPointer *)Pointer_create()) == NULL) return;
			xs = (xs = sqrt (dx * dx + dy * dy) - 2.2 * r) < 0 ? 0 : xs;
			ys = perc * rmax / 100; 
			Polygon_scale (p, xs, ys);
			Polygon_translate (p, x1, y1 - ys / 2);
			Polygon_rotate (p, alpha, x1, y1);
			Polygon_translate (p, 1.1 * r * cos (alpha) ,
				1.1 * r * sin (alpha));
			Pointer_draw (p, g);
			forget (p); 	
		}
	}
	
	Graphics_unsetInner (g);
	
    if (garnish)
    {
    	Graphics_drawInnerBox (g);
    	Graphics_marksBottom (g, 2, 1, 1, 0);
		if (ymin * ymax < 0.0) Graphics_markLeft (g, 0.0, 1, 1, 1, NULL);
    	Graphics_marksLeft (g, 2, 1, 1, 0);
		if (xmin * xmax < 0.0) Graphics_markBottom (g, 0.0, 1, 1, 1, NULL);
    }
}

FORM (Confusion_Matrix_draw, L"Confusion & Matrix: Draw confusions with arrows", 0)
    INTEGER (L"Category position", L"0 (=all)")
    REAL (L"lower level(%)", L"0")
    REAL (L"left Horizontal range", L"0.0")
    REAL (L"right Horizontal range", L"0.0")
    REAL (L"left Vertical range", L"0.0")
    REAL (L"right Vertical range", L"0.0")
    BOOLEAN (L"Garnish", 1)
	OK
DO
	long categoryPosition = GET_INTEGER (L"Category position");
	REQUIRE (categoryPosition >= 0, L"Category position must be >= 0")
	EVERY_DRAW (Confusion_Matrix_draw((structConfusion *)ONLY(classConfusion), (structMatrix *)ONLY(classMatrix), GRAPHICS,
		categoryPosition, GET_REAL (L"lower level(%)"),
		GET_REAL (L"left Horizontal range"), GET_REAL (L"right Horizontal range"),
		GET_REAL (L"left Vertical range"), GET_REAL (L"right Vertical range"),
		GET_INTEGER (L"Garnish")))
END

/**********************Correlation *******************************************/

DIRECT (Correlation_help)
	Melder_help (L"Correlation");
END

FORM (Correlation_confidenceIntervals, L"Correlation: Confidence intervals...", L"Correlation: Confidence intervals...")
	POSITIVE (L"Confidence level (0-1)", L"0.95")
	INTEGER (L"Number of tests (Bonferroni correction)", L"0")
	RADIO (L"Approximation", 1)
	RADIOBUTTON (L"Ruben")
	RADIOBUTTON (L"Fisher")
	OK
DO
	double cl = GET_REAL (L"Confidence level");
	double numberOfTests = GET_INTEGER (L"Number of tests");
	EVERY_TO (Correlation_confidenceIntervals ((structCorrelation *)OBJECT, cl, numberOfTests,
		GET_INTEGER (L"Approximation")))
END

FORM (Correlation_testDiagonality_bartlett, L"Correlation: Get diagonality (bartlett)", L"SSCP: Get diagonality (bartlett)...")
	NATURAL (L"Number of contraints", L"1")
	OK
DO
	double chisq, p;
	long nc = GET_INTEGER (L"Number of contraints");
	Correlation me = (structCorrelation *)ONLY_OBJECT;
	Correlation_testDiagonality_bartlett (me, nc, &chisq, &p);
	Melder_information5 (Melder_double (p), L" (=probability, based on chisq = ",
		Melder_double (chisq), L"and ndf = ", Melder_integer (my numberOfRows * (my numberOfRows - 1) / 2));
END

DIRECT (Correlation_to_PCA)
	EVERY_TO (SSCP_to_PCA (OBJECT))
END

/**********************Covariance *******************************************/

DIRECT (Covariance_help)
	Melder_help (L"Covariance");
END

FORM (Covariance_createSimple, L"Create simple Covariance", L"Create simple Covariance...")
	WORD (L"Name", L"c")
	SENTENCE (L"Covariances", L"1.0 0.0 1.0")
	SENTENCE (L"Centroid", L"0.0 0.0")
	POSITIVE (L"Number of observations", L"100.0")
	OK
DO
	if (! praat_new1 (Covariance_createSimple (GET_STRING (L"Covariances"), GET_STRING (L"Centroid"),
		GET_REAL (L"Number of observations")), GET_STRING (L"Name"))) return 0;
END

FORM (Covariance_getProbabilityAtPosition, L"Covariance: Get probability at position", 0)
	SENTENCE (L"Position", L"10.0 20.0")
	OK
DO
	wchar_t *position = GET_STRING (L"Position");
	double p = Covariance_getProbabilityAtPosition_string ((structCovariance *)ONLY_OBJECT, position);
	Melder_information4 (Melder_double (p), L" (= probability at position ", position, L")");
END

FORM (Covariance_getSignificanceOfOneMean, L"Covariance: Get significance of one mean", L"Covariance: Get significance of one mean...")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Get probability that the mean with")
	NATURAL (L"Index", L"1")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"differs from")
	REAL (L"Value", L"0.0")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"(Null hypothesis: the observed difference is due to chance.)")
	OK
DO
	double t, p; double ndf;
	Covariance_getSignificanceOfOneMean ((structCovariance *)ONLY_OBJECT, GET_INTEGER (L"Index"),
		GET_REAL (L"Value"), &p, &t , &ndf);
	Melder_information5 (Melder_double (p), L" (=probability, based on t = ",
		Melder_double (t), L" and ndf = ", Melder_integer (ndf));
END

FORM (Covariance_getSignificanceOfMeansDifference, L"Covariance: Get significance of means difference", L"Covariance: Get significance of means difference...")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Get probability that the difference between means")
	NATURAL (L"Index1", L"1")
	NATURAL (L"Index2", L"2")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"differs from")
	REAL (L"Value", L"0.0")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"when the means are")
	BOOLEAN (L"Paired", 1)
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"and have")
	BOOLEAN (L"Equal variances", 1)
	OK
DO
	double t, p; double ndf;
	Covariance_getSignificanceOfMeansDifference ((structCovariance *)ONLY_OBJECT,
		GET_INTEGER (L"Index1"), GET_INTEGER (L"Index2"),
		GET_REAL (L"Value"), GET_INTEGER (L"Paired"),
		GET_INTEGER (L"Equal variances"), &p, &t , &ndf);
	Melder_information6 (Melder_double (p), L" (=probability, based on t = ",
		Melder_double (t), L"and ndf = ", Melder_integer (ndf), L")");
END

FORM (Covariance_getSignificanceOfOneVariance, L"Covariance: Get significance of one variance", L"Covariance: Get significance of one variance...")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Get probability that the variance with")
	NATURAL (L"Index", L"1")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"differs from")
	REAL (L"Value", L"0.0")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"(Null hypothesis: the observed difference is due to chance.)")
	OK
DO
	double chisq, p; long ndf;
	Covariance_getSignificanceOfOneVariance ((structCovariance *)ONLY_OBJECT, GET_INTEGER (L"Index"),
		GET_REAL (L"Value"), &p, &chisq , &ndf);
	Melder_information5 (Melder_double (p), L" (=probability, based on chisq = ",
		Melder_double (chisq), L"and ndf = ", Melder_integer (ndf));
END

FORM (Covariance_getSignificanceOfVariancesRatio, L"Covariance: Get significance of variances ratio", L"Covariance: Get significance of variances ratio...")
	NATURAL (L"Index1", L"1")
	NATURAL (L"Index2", L"2")
	REAL (L"Hypothesized ratio", L"1.0")
	OK
DO
	double f, p; long ndf;
	Covariance_getSignificanceOfVariancesRatio ((structCovariance *)ONLY_OBJECT,
		GET_INTEGER (L"Index1"), GET_INTEGER (L"Index2"),
		GET_REAL (L"Hypothesized ratio"), &p, &f , &ndf);
	Melder_information7 (Melder_double (p), L" (=probability, based on F = ",
		Melder_double (f), L"and ndf1 = ", Melder_integer (ndf),
		L" and ndf2 = ", Melder_integer (ndf));
END

FORM (Covariance_getFractionVariance, L"Covariance: Get fraction variance", L"Covariance: Get fraction variance...")
	NATURAL (L"From dimension", L"1")
	NATURAL (L"To dimension", L"1")
	OK
DO
	Melder_information1 (Melder_double (SSCP_getFractionVariation (ONLY_OBJECT,
		GET_INTEGER (L"From dimension"), GET_INTEGER (L"To dimension"))));
END

FORM (Covariances_reportMultivariateMeanDifference, L"Covariances: Report multivariate mean difference",
	L"Covariances: Report multivariate mean difference...")
	BOOLEAN (L"Covariances are equal", 1)
	OK
DO
	Covariance c1 = NULL, c2 = NULL;
	double prob, fisher, df1, df2, difference;
	int equalCovariances = GET_INTEGER (L"Covariances are equal");
	WHERE (SELECTED && CLASS == classCovariance)
	{
		if (c1) c2 = (structCovariance *)OBJECT; else c1 = (structCovariance *)OBJECT;
	}
	MelderInfo_open ();
	difference = Covariances_getMultivariateCentroidDifference (c1, c2, equalCovariances, &prob, &fisher, &df1, &df2);
	MelderInfo_writeLine3 (L"Under the assumption that the two covariances are", (equalCovariances ? L" " : L" not "),L"equal:");
	MelderInfo_writeLine2 (L"Difference between multivariate means = ", Melder_double (difference));
	MelderInfo_writeLine2 (L"Fisher's F = ", Melder_double (fisher));
	MelderInfo_writeLine2 (L"Significance from zero = ", Melder_double (prob));
	MelderInfo_writeLine4 (L"Degrees of freedom = ", Melder_double (df1), L", ", Melder_double (df2));
	MelderInfo_writeLine4 (L"(Number of observations = ", Melder_integer (c1->numberOfObservations), L", ",
		Melder_integer (c2->numberOfObservations));
	MelderInfo_writeLine3 (L"Dimension of covariance matrices = ", Melder_integer (c1-> numberOfRows), L")");
	MelderInfo_close ();
END

FORM (Covariance_to_TableOfReal_randomSampling, L"Covariance: To TableOfReal (random sampling)", L"Covariance: To TableOfReal (random sampling)...")
	INTEGER (L"Number of data points", L"0")
	OK
DO
	EVERY_TO (Covariance_to_TableOfReal_randomSampling ((structCovariance *)OBJECT,
		GET_INTEGER (L"Number of data points")))
END

DIRECT (Covariances_reportEquality)
	Ordered covars = Ordered_create ();
	if (covars == NULL) return 0;
	WHERE (SELECTED)
	{
		if (! Collection_addItem (covars, OBJECT))
		{
			covars -> size = 0; forget (covars); return 0;
		}
	}
	MelderInfo_open ();
	{
		double chisq, p, df;
		Covariances_equality (covars, 1, &p, &chisq, &df);
		MelderInfo_writeLine1 (L"Difference between covariance matrices:");
		MelderInfo_writeLine2 (L"Significance of difference (bartlett) = ", Melder_double (p));
		MelderInfo_writeLine2 (L"Chi-squared = ", Melder_double (chisq));
		MelderInfo_writeLine2 (L"Degrees of freedom = ", Melder_double (df));
		Covariances_equality (covars, 2, &p, &chisq, &df);
		MelderInfo_writeLine2 (L"Significance of difference (wald) = ", Melder_double (p));
		MelderInfo_writeLine2 (L"Chi-squared = ", Melder_double (chisq));
		MelderInfo_writeLine2 (L"Degrees of freedom = ", Melder_double (df));
	}
	MelderInfo_close ();
	covars -> size = 0; forget (covars);
END

DIRECT (Covariance_to_Correlation)
	EVERY_TO (SSCP_to_Correlation (OBJECT))
END

DIRECT (Covariance_to_PCA)
	EVERY_TO (SSCP_to_PCA (OBJECT))
END

FORM (Covariance_and_TableOfReal_mahalanobis, L"Covariance & TableOfReal: To TableOfReal (mahalanobis)", L"Covariance & TableOfReal: To TableOfReal (mahalanobis)...")
	BOOLEAN (L"Centroid from table", 0)
	OK
DO
	NEW (Covariance_and_TableOfReal_mahalanobis ((structCovariance *)ONLY (classCovariance), (structTableOfReal *)ONLY (classTableOfReal),
		GET_INTEGER (L"Centroid from table")))
END

/********************** Discriminant **********************************/

void SSCPs_drawConcentrationEllipses (SSCPs me, Graphics g, double scale,
	int confidence, wchar_t *label, long d1, long d2, double xmin, double xmax,
	double ymin, double ymax, int fontSize, int garnish);

void Discriminant_drawTerritorialMap (Discriminant me, Graphics g,
	int discriminantDirections, long d1, long d2, double xmin, double xmax,
	double ymin, double ymax, int fontSize, int poolCovarianceMatrices,
	int garnish)
{
	(void) me;(void) g;(void) discriminantDirections;(void) d1;(void) d2;
	(void) xmin;(void) xmax;(void) ymin;
	(void) ymax;(void) fontSize;(void) poolCovarianceMatrices;(void) garnish;

}

void Discriminant_drawConcentrationEllipses (Discriminant me, Graphics g,
	double scale, int confidence, wchar_t *label, int discriminantDirections, long d1, long d2,
	double xmin, double xmax, double ymin, double ymax, int fontSize, int garnish)
{
	SSCPs thee;
	long numberOfFunctions = Discriminant_getNumberOfFunctions (me);
	double *v1, *v2;

	if (! discriminantDirections)
	{
		SSCPs_drawConcentrationEllipses (my groups, g, scale, confidence, label,
			d1, d2, xmin, xmax, ymin, ymax, fontSize, garnish);
		return;
	}

	if (numberOfFunctions <= 1)
	{
		Melder_warning1 (L"Discriminant_drawConcentrationEllipses: Nothing drawn "
			"because there is only one dimension in the discriminant space.");
		return;
	}

	/*
		Project SSCPs on eigenvectors.
	*/

	if (d1 == 0 && d2 == 0)
	{
		d1 = 1;
		d2 = MIN (numberOfFunctions, d1 + 1);
	}
	else if (d1 < 0 || d2 > numberOfFunctions) return;

	v1 = my eigenvectors[d1]; v2 = my eigenvectors[d2];

	if ((thee = SSCPs_toTwoDimensions (my groups, v1, v2)) == NULL) return;

	SSCPs_drawConcentrationEllipses (thee, g, scale, confidence, label, 1, 2,
		xmin, xmax, ymin, ymax, fontSize, 0);

	if (garnish)
	{
		wchar_t label[40];
    	Graphics_drawInnerBox (g);
    	Graphics_marksLeft (g, 2, 1, 1, 0);
    	swprintf (label, 40, L"function %ld", d2);
    	Graphics_textLeft (g, 1, label);
    	Graphics_marksBottom (g, 2, 1, 1, 0);
    	swprintf (label, 40, L"function %ld", d1);
		Graphics_textBottom (g, 1, label);
	}

	forget (thee);
}

DIRECT (Discriminant_help)
	Melder_help (L"Discriminant");
END

DIRECT (Discriminant_setGroupLabels)
	if (! Discriminant_setGroupLabels ((structDiscriminant *)ONLY(classDiscriminant),
		(structStrings *)ONLY (classStrings))) return 0;
END

FORM (Discriminant_and_Pattern_to_Categories, L"Discriminant & Pattern: To Categories", L"Discriminant & Pattern: To Categories...")
	BOOLEAN (L"Pool covariance matrices", 1)
	BOOLEAN (L"Use apriori probabilities", 1)
	OK
DO
	NEW (Discriminant_and_Pattern_to_Categories
		((structDiscriminant *)ONLY (classDiscriminant), (structPattern *)ONLY_GENERIC (classPattern),
		GET_INTEGER (L"Pool covariance matrices"),
		GET_INTEGER (L"Use apriori probabilities")))
END

FORM (Discriminant_and_TableOfReal_to_Configuration, L"Discriminant & TableOfReal: To Configuration", L"Discriminant & TableOfReal: To Configuration...")
	INTEGER (L"Number of dimensions", L"0")
	OK
DO
	long dimension = GET_INTEGER (L"Number of dimensions");
	REQUIRE (dimension >= 0, L"Number of dimensions must be greater equal zero.")
	NEW (Discriminant_and_TableOfReal_to_Configuration
		((structDiscriminant *)ONLY (classDiscriminant), (structTableOfReal *)ONLY_GENERIC (classTableOfReal),
			dimension))
END

DIRECT (hint_Discriminant_and_TableOfReal_to_ClassificationTable)
	Melder_information1 (L"You can use the Discriminant as a classifier by \nselecting a Discriminant and a TableOfReal object together.");
END

FORM (Discriminant_and_TableOfReal_to_ClassificationTable, L"Discriminant & TableOfReal: To ClassificationTable", L"Discriminant & TableOfReal: To ClassificationTable...")
	BOOLEAN (L"Pool covariance matrices", 1)
	BOOLEAN (L"Use apriori probabilities", 1)
	OK
DO
	Discriminant d = (structDiscriminant *)ONLY (classDiscriminant);
	TableOfReal t = (structTableOfReal *)ONLY_GENERIC (classTableOfReal);
	if (! praat_new3 (Discriminant_and_TableOfReal_to_ClassificationTable
		(d,	t, GET_INTEGER (L"Pool covariance matrices"),
		GET_INTEGER (L"Use apriori probabilities")),
		Thing_getName (d), L"_", Thing_getName (t))) return 0;
END

FORM (Discriminant_and_TableOfReal_mahalanobis, L"Discriminant & TableOfReal: To TableOfReal (mahalanobis)", L"Discriminant & TableOfReal: To TableOfReal (mahalanobis)...")
	SENTENCE (L"Group label", L"ui/editors/AmplitudeTierEditor.h")
	BOOLEAN (L"Pool covariance matrices", 0)
	OK
DO
	Discriminant d = (structDiscriminant *)ONLY (classDiscriminant);
	long group = Discriminant_groupLabelToIndex (d, GET_STRING (L"Group label"));
	REQUIRE (group > 0, L"Group label does not exist.")
	NEW (Discriminant_and_TableOfReal_mahalanobis (d, (structTableOfReal *)ONLY(classTableOfReal), group,
		GET_INTEGER (L"Pool covariance matrices")))
END

FORM (Discriminant_getWilksLambda, L"Discriminant: Get Wilks' lambda", L"Discriminant: Get Wilks' lambda...")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Product (i=from..numberOfEigenvalues, 1 / (1 + eigenvalue[i]))")
	INTEGER (L"From", L"1")
	OK
DO
	long from = GET_INTEGER (L"From");
	REQUIRE (from >= 1, L"Number must be greater than or equal to one.")
	Melder_information1 (Melder_double (Discriminant_getWilksLambda ((structDiscriminant *)ONLY_OBJECT, from)));
END

FORM (Discriminant_getCumulativeContributionOfComponents, L"Discriminant: Get cumulative contribution of components", L"Eigen: Get cumulative contribution of components...")
	NATURAL (L"From component", L"1")
	NATURAL (L"To component", L"1")
	OK
DO
	Melder_information1 (Melder_double (Eigen_getCumulativeContributionOfComponents
		(ONLY_OBJECT, GET_INTEGER (L"From component"),
		GET_INTEGER (L"To component"))));
END


FORM (Discriminant_getPartialDiscriminationProbability, L"Discriminant: Get partial discrimination probability", L"Discriminant: Get partial discrimination probability...")
	INTEGER (L"Number of dimensions", L"1")
	OK
DO
	long ndf, n = GET_INTEGER (L"Number of dimensions");
	double chisq, p;
	REQUIRE (n >= 0, L"Number of dimensions must be greater than or equal to zero.")
	Discriminant_getPartialDiscriminationProbability ((structDiscriminant *)ONLY_OBJECT, n, &p, &chisq, &ndf);
	Melder_information5 (Melder_double (p), L" (=probability, based on chisq = ", Melder_double (chisq), L"and ndf = ", Melder_integer (ndf));
END

DIRECT (Discriminant_getHomegeneityOfCovariances_box)
	Discriminant thee = (structDiscriminant *)ONLY_OBJECT;
	double chisq, p;
	long ndf;
	SSCPs_getHomegeneityOfCovariances_box (thy groups, &p, &chisq, &ndf);
	Melder_information5 (Melder_double (p), L" (=probability, based on chisq = ",
		Melder_double (chisq), L"and ndf = ", Melder_integer (ndf));
END

DIRECT (Discriminant_reportEqualityOfCovariances_wald)
	Discriminant thee = (structDiscriminant *)ONLY_OBJECT;
	double chisq, prob, df;
	MelderInfo_open ();
	Covariances_equality ((Ordered)(thy groups), 2, &prob, &chisq, &df);
	MelderInfo_writeLine1 (L"Wald test for equality of covariance matrices:");
	MelderInfo_writeLine2 (L"Chi squared: ", Melder_double (chisq));
	MelderInfo_writeLine2 (L"Significance: ", Melder_double (prob));
	MelderInfo_writeLine2 (L"Degrees of freedom: ", Melder_double (df));
	MelderInfo_writeLine2 (L"Number of matrices: ", Melder_integer (thy groups -> size));
	MelderInfo_close ();
END

FORM (Discriminant_getConcentrationEllipseArea, L"Discriminant: Get concentration ellipse area", L"Discriminant: Get concentration ellipse area...")
	SENTENCE (L"Group label", L"ui/editors/AmplitudeTierEditor.h")
	POSITIVE (L"Number of sigmas", L"1.0")
	BOOLEAN (L"Discriminant plane", 1)
	INTEGER (L"X-dimension", L"1")
	INTEGER (L"Y-dimension", L"2")
	OK
DO
	Discriminant d = (structDiscriminant *)ONLY_OBJECT;
	long group = Discriminant_groupLabelToIndex (d, GET_STRING (L"Group label"));
	REQUIRE (group > 0, L"Group label does not exist.")
	Melder_information1 (Melder_double (Discriminant_getConcentrationEllipseArea(d, group,
		GET_REAL (L"Number of sigmas"), 0, GET_INTEGER (L"Discriminant plane"),
		GET_INTEGER (L"X-dimension"), GET_INTEGER (L"Y-dimension"))));
END

FORM (Discriminant_getConfidenceEllipseArea, L"Discriminant: Get confidence ellipse area", L"Discriminant: Get confidence ellipse area...")
	SENTENCE (L"Group label", L"ui/editors/AmplitudeTierEditor.h")
	POSITIVE (L"Confidence level (0-1)", L"0.95")
	BOOLEAN (L"Discriminant plane", 1)
	INTEGER (L"X-dimension", L"1")
	INTEGER (L"Y-dimension", L"2")
	OK
DO
	Discriminant d = (structDiscriminant *)ONLY_OBJECT;
	long group = Discriminant_groupLabelToIndex (d, GET_STRING (L"Group label"));
	REQUIRE (group > 0, L"Group label does not exist.")
	Melder_information1 (Melder_double (Discriminant_getConcentrationEllipseArea(d, group,
		GET_REAL (L"Confidence level"), 1, GET_INTEGER (L"Discriminant plane"),
		GET_INTEGER (L"X-dimension"), GET_INTEGER (L"Y-dimension"))));
END

FORM (Discriminant_getLnDeterminant_group, L"Discriminant: Get determinant (group)", L"Discriminant: Get determinant (group)...")
	SENTENCE (L"Group label", L"ui/editors/AmplitudeTierEditor.h")
	OK
DO
	Discriminant d = (structDiscriminant *)ONLY_OBJECT;
	long group = Discriminant_groupLabelToIndex (d, GET_STRING (L"Group label"));
	REQUIRE (group > 0, L"Group label does not exist.")
	Melder_information1 (Melder_double (Discriminant_getLnDeterminant_group (d, group)));
END

DIRECT (Discriminant_getLnDeterminant_total)
	Melder_information1 (Melder_double (Discriminant_getLnDeterminant_total ((structDiscriminant *)ONLY_OBJECT)));
END

FORM (Discriminant_invertEigenvector, L"Discriminant: Invert eigenvector", 0)
	NATURAL (L"Index of eigenvector", L"1")
	OK
DO
	EVERY (Eigen_invertEigenvector (OBJECT,
		GET_INTEGER (L"Index of eigenvector")))
END

FORM (Discriminant_drawSigmaEllipses, L"Discriminant: Draw sigma ellipses", L"Discriminant: Draw sigma ellipses...")
	POSITIVE (L"Number of sigmas", L"1.0")
	BOOLEAN (L"Discriminant plane", 1)
	INTEGER (L"X-dimension", L"1")
	INTEGER (L"Y-dimension", L"2")
	REAL (L"left Horizontal range", L"0.0")
	REAL (L"right Horizontal range", L"0.0")
	REAL (L"left Vertical range", L"0.0")
	REAL (L"right Vertical range", L"0.0")
	INTEGER (L"Label size", L"12")
	BOOLEAN (L"Garnish", 1)
	OK
DO
	EVERY_DRAW (Discriminant_drawConcentrationEllipses ((structDiscriminant *)OBJECT, GRAPHICS,
		GET_REAL (L"Number of sigmas"), 0, NULL, GET_INTEGER (L"Discriminant plane"),
		GET_INTEGER (L"X-dimension"), GET_INTEGER (L"Y-dimension"),
		GET_REAL (L"left Horizontal range"), GET_REAL (L"right Horizontal range"),
		GET_REAL (L"left Vertical range"), GET_REAL (L"right Vertical range"),
		GET_INTEGER (L"Label size"), GET_INTEGER (L"Garnish")))
END

FORM (Discriminant_drawOneSigmaEllipse, L"Discriminant: Draw one sigma ellipse", L"Discriminant: Draw one sigma ellipse...")
	SENTENCE (L"Label", L"ui/editors/AmplitudeTierEditor.h")
	POSITIVE (L"Number of sigmas", L"1.0")
	BOOLEAN (L"Discriminant plane", 1)
	INTEGER (L"X-dimension", L"1")
	INTEGER (L"Y-dimension", L"2")
	REAL (L"left Horizontal range", L"0.0")
	REAL (L"right Horizontal range", L"0.0")
	REAL (L"left Vertical range", L"0.0")
	REAL (L"right Vertical range", L"0.0")
	INTEGER (L"Label size", L"12")
	BOOLEAN (L"Garnish", 1)
	OK
DO
	EVERY_DRAW (Discriminant_drawConcentrationEllipses ((structDiscriminant *)OBJECT, GRAPHICS,
		GET_REAL (L"Number of sigmas"), 0, GET_STRING (L"Label"),
		GET_INTEGER (L"Discriminant plane"),
		GET_INTEGER (L"X-dimension"), GET_INTEGER (L"Y-dimension"),
		GET_REAL (L"left Horizontal range"), GET_REAL (L"right Horizontal range"),
		GET_REAL (L"left Vertical range"), GET_REAL (L"right Vertical range"),
		GET_INTEGER (L"Label size"), GET_INTEGER (L"Garnish")))
END

FORM (Discriminant_drawConfidenceEllipses, L"Discriminant: Draw confidence ellipses", 0)
	POSITIVE (L"Confidence level (0-1)", L"0.95")
	BOOLEAN (L"Discriminant plane", 1)
	INTEGER (L"X-dimension", L"1")
	INTEGER (L"Y-dimension", L"2")
	REAL (L"left Horizontal range", L"0.0")
	REAL (L"right Horizontal range", L"0.0")
	REAL (L"left Vertical range", L"0.0")
	REAL (L"right Vertical range", L"0.0")
	INTEGER (L"Label size", L"12")
	BOOLEAN (L"Garnish", 1)
	OK
DO
	EVERY_DRAW (Discriminant_drawConcentrationEllipses ((structDiscriminant *)OBJECT, GRAPHICS,
		GET_REAL (L"Confidence level"), 1, NULL, GET_INTEGER (L"Discriminant plane"),
		GET_INTEGER (L"X-dimension"), GET_INTEGER (L"Y-dimension"),
		GET_REAL (L"left Horizontal range"), GET_REAL (L"right Horizontal range"),
		GET_REAL (L"left Vertical range"), GET_REAL (L"right Vertical range"),
		GET_INTEGER (L"Label size"), GET_INTEGER (L"Garnish")))
END


FORM (Discriminant_drawOneConfidenceEllipse, L"Discriminant: Draw one confidence ellipse", 0)
	SENTENCE (L"Label", L"ui/editors/AmplitudeTierEditor.h")
	POSITIVE (L"Confidence level (0-1)", L"0.95")
	BOOLEAN (L"Discriminant plane", 1)
	INTEGER (L"X-dimension", L"1")
	INTEGER (L"Y-dimension", L"2")
	REAL (L"left Horizontal range", L"0.0")
	REAL (L"right Horizontal range", L"0.0")
	REAL (L"left Vertical range", L"0.0")
	REAL (L"right Vertical range", L"0.0")
	INTEGER (L"Label size", L"12")
	BOOLEAN (L"Garnish", 1)
	OK
DO
	EVERY_DRAW (Discriminant_drawConcentrationEllipses ((structDiscriminant *)OBJECT, GRAPHICS,
		GET_REAL (L"Confidence level"), 1, GET_STRING (L"Label"), GET_INTEGER (L"Discriminant plane"),
		GET_INTEGER (L"X-dimension"), GET_INTEGER (L"Y-dimension"),
		GET_REAL (L"left Horizontal range"), GET_REAL (L"right Horizontal range"),
		GET_REAL (L"left Vertical range"), GET_REAL (L"right Vertical range"),
		GET_INTEGER (L"Label size"), GET_INTEGER (L"Garnish")))
END

DIRECT (Discriminant_extractBetweenGroupsSSCP)
	EVERY_TO (Discriminant_extractBetweenGroupsSSCP ((structDiscriminant *)OBJECT))
END
/*
FORM (Discriminant_extractCoefficients, "Extract coefficients...", 0)
	RADIO (L"Function coefficients", 1)
	RADIOBUTTON (L"Raw")
	RADIOBUTTON (L"Unstandardized")
	RADIOBUTTON (L"Standardized")
	OK
DO
	EVERY_TO (Discriminant_extractCoefficients (OBJECT, GET_INTEGER (L"Function coefficients")))
END
*/
DIRECT (Discriminant_extractGroupCentroids)
	EVERY_TO (Discriminant_extractGroupCentroids ((structDiscriminant *)OBJECT))
END

DIRECT (Discriminant_extractGroupStandardDeviations)
	EVERY_TO (Discriminant_extractGroupStandardDeviations ((structDiscriminant *)OBJECT))
END

DIRECT (Discriminant_extractGroupLabels)
	EVERY_TO (Discriminant_extractGroupLabels ((structDiscriminant *)OBJECT))
END

DIRECT (Discriminant_extractPooledWithinGroupsSSCP)
	EVERY_TO (Discriminant_extractPooledWithinGroupsSSCP ((structDiscriminant *)OBJECT))
END

FORM (Discriminant_extractWithinGroupSSCP, L"Discriminant: Extract within-group SSCP", L"Discriminant: Extract within-group SSCP...")
	NATURAL (L"Group index", L"1")
	OK
DO
	EVERY_TO (Discriminant_extractWithinGroupSSCP ((structDiscriminant *)OBJECT,
		GET_INTEGER (L"Group index")))
END

DIRECT (Discriminant_getNumberOfFunctions)
	Melder_information1 (Melder_integer (Discriminant_getNumberOfFunctions ((structDiscriminant *)ONLY_OBJECT)));
END

DIRECT (Discriminant_getDimensionOfFunctions)
	Melder_information1 (Melder_integer (Eigen_getDimensionOfComponents (ONLY_OBJECT)));
END

DIRECT (Discriminant_getNumberOfGroups)
	Melder_information1 (Melder_integer (Discriminant_getNumberOfGroups ((structDiscriminant *)ONLY_OBJECT)));
END

FORM (Discriminant_getNumberOfObservations, L"Discriminant: Get number of observations", L"Discriminant: Get number of observations...")
	INTEGER (L"Group", L"0 (=total)")
	OK
DO
	Melder_information1 (Melder_integer (Discriminant_getNumberOfObservations ((structDiscriminant *)ONLY_OBJECT, GET_INTEGER (L"Group"))));
END


/********************** DTW *******************************************/

void Sound_draw_btlr (Sound me, Graphics g, double tmin, double tmax, double amin, double amax, int direction, int garnish);

static void DTW_paintDistances_raw (DTW me, Any g, double xmin, double xmax, double ymin,
	double ymax, double minimum, double maximum, int garnish, int inset)
{
	long ixmin, ixmax, iymin, iymax;
	if (xmax <= xmin) { xmin = my xmin; xmax = my xmax; }
	if (ymax <= ymin) { ymin = my ymin; ymax = my ymax; }
	(void) Matrix_getWindowSamplesX (me, xmin - 0.49999 * my dx, xmax + 0.49999 * my dx,
		& ixmin, & ixmax);
	(void) Matrix_getWindowSamplesY (me, ymin - 0.49999 * my dy, ymax + 0.49999 * my dy,
		& iymin, & iymax);
	if (maximum <= minimum)
		(void) Matrix_getWindowExtrema (me, ixmin, ixmax, iymin, iymax, & minimum, & maximum);
	if (maximum <= minimum) { minimum -= 1.0; maximum += 1.0; }
	if (xmin >= xmax || ymin >= ymax) return;
	if (inset) Graphics_setInner (g);
	Graphics_setWindow (g, xmin, xmax, ymin, ymax);
	Graphics_cellArray (g, my z,
			ixmin, ixmax, Matrix_columnToX (me, ixmin - 0.5), Matrix_columnToX (me, ixmax + 0.5),
			iymin, iymax, Matrix_rowToY (me, iymin - 0.5), Matrix_rowToY (me, iymax + 0.5),
			minimum, maximum);
	Graphics_rectangle (g, xmin, xmax, ymin, ymax);
	if (inset) Graphics_unsetInner (g);
	if (garnish)
	{
		Graphics_marksBottom (g, 2, 1, 1, 0);
		Graphics_marksLeft (g, 2, 1, 1, 0);
	}
}

void DTW_paintDistances (DTW me, Any g, double xmin, double xmax, double ymin,
	double ymax, double minimum, double maximum, int garnish)
{
	DTW_paintDistances_raw (me, g, xmin, xmax, ymin, ymax, minimum, maximum, garnish, 1);
}

static int DTW_and_Sounds_checkDomains (DTW me, Sound *y, Sound *x, double *xmin, double *xmax, double *ymin, double *ymax)
{
	if (my ymin == (*y) -> xmin && my ymax == (*y) -> xmax)
	{
		if (my xmin != (*x) -> xmin || my xmax != (*x) -> xmax)
		{
			return Melder_error1 (L"The domains of the DTW and the sound('s) don't match");
		}
	}
	else if (my ymin == (*x) -> xmin && my ymax == (*x) -> xmax)
	{
		if (my xmin != (*y) -> xmin || my xmax != (*y) -> xmax)
		{
			return Melder_error1 (L"The domains of the DTW and the sound('s) don't match");
		}
		Sound tmp = *y; *y = *x; *x = tmp; // swap x and y
	}
	else
	{
		return Melder_error1 (L"The domains of the DTW and the sound('s) don't match");
	}

	if (*xmin >= *xmax)
	{
		*xmin = my xmin; *xmax = my xmax;
	}
	if (*ymin >= *ymax)
	{
		*ymin = my ymin; *ymax = my ymax;
	}
	return 1;
}

static void drawBox (Graphics g)
{
	double x1WC, x2WC, y1WC, y2WC;
	double lineWidth = Graphics_inqLineWidth (g);
	Graphics_inqWindow (g, &x1WC, &x2WC, &y1WC, &y2WC);
	Graphics_setLineWidth (g, 2.0*lineWidth);
	Graphics_rectangle (g, x1WC, x2WC, y1WC, y2WC);
	Graphics_setLineWidth (g, lineWidth);
}

/*
  In a picture with a DTW and a left and bottom Sound, we want "width" of the vertical sound
  and the "height" of the horizontal Sound t be equal.
  Given the horizontal fraction of the DTW-part, this routine returns the vertical part.
*/
static double _DTW_and_Sounds_getPartY (Graphics g, double dtw_part_x)
{
	double x1NDC, x2NDC, y1NDC, y2NDC;
	Graphics_inqViewport (g, &x1NDC, &x2NDC, &y1NDC, &y2NDC);
	return 1 - ((1 - dtw_part_x) * (x2NDC -x1NDC))/(y2NDC -y1NDC);
}

static void DTW_drawPath_raw (DTW me, Any g, double xmin, double xmax, double ymin,
	double ymax, int garnish, int inset)
{
	DTW_Path_Query thee = & my pathQuery;
	long i;

	if (xmin >= xmax)
	{
		xmin = my xmin; xmax = my xmax;
	}
	if (ymin >= ymax)
	{
		ymin = my ymin; ymax = my ymax;
	}

	if (inset) Graphics_setInner (g);
	Graphics_setWindow (g, xmin, xmax, ymin, ymax);

	for (i = 1; i < thy nxy; i++)
	{
		double x1, y1, x2, y2;
		if (NUMclipLineWithinRectangle (thy xytimes[i].x, thy xytimes[i].y,
			thy xytimes[i+1].x, thy xytimes[i+1].y,
			xmin, ymin, xmax, ymax, &x1, &y1, &x2, &y2))
		{
			Graphics_line (g, x1, y1, x2, y2);
		}
	}

	if (inset) Graphics_unsetInner (g);
	if (garnish)
	{
		Graphics_drawInnerBox(g);
		Graphics_marksBottom (g, 2, 1, 1, 0);
		Graphics_marksLeft (g, 2, 1, 1, 0);
	}
}

void DTW_drawPath (DTW me, Any g, double xmin, double xmax, double ymin,
	double ymax, int garnish)
{
	DTW_drawPath_raw (me, g, xmin, xmax, ymin, ymax, garnish, 1);
}

static void DTW_drawWarpX_raw (DTW me, Graphics g, double xmin, double xmax, double ymin, double ymax, double tx, int garnish, int inset)
{
	double ty = DTW_getYTime (me, tx);
	int lineType = Graphics_inqLineType (g);

	if (xmin >= xmax)
	{
		xmin = my xmin; xmax = my xmax;
	}
	if (ymin >= ymax)
	{
		ymin = my ymin; ymax = my ymax;
	}

	if (inset) Graphics_setInner (g);
	Graphics_setWindow (g, xmin, xmax, ymin, ymax);

	ty = DTW_getYTime (me, tx);
	Graphics_setLineType (g, Graphics_DOTTED);

	Graphics_line (g, tx, ymin, tx, ty);
	Graphics_line (g, tx, ty, xmin, ty);

	Graphics_setLineType (g, lineType);

	if (inset) Graphics_unsetInner (g);

	if (garnish)
	{
		Graphics_markBottom (g, tx, 1, 1, 0, NULL);
		Graphics_markLeft (g, ty, 1, 1, 0, NULL);
	}
}

void DTW_drawWarpX (DTW me, Graphics g, double xmin, double xmax, double ymin, double ymax, double tx, int garnish)
{
	DTW_drawWarpX_raw (me, g, xmin, xmax, ymin, ymax, tx, garnish, 1);
}

void DTW_and_Sounds_draw (DTW me, Sound y, Sound x, Graphics g, double xmin, double xmax,
	double ymin, double ymax, int garnish)
{
	double xmin3, ymin3, dtw_part_x = 0.85, dtw_part_y = dtw_part_x;
	Graphics_Viewport vp, ovp;

	if (! DTW_and_Sounds_checkDomains (me, &y, &x, &xmin, &xmax, &ymin, &ymax)) return;

	Graphics_setInner (g);
	ovp = g -> outerViewport; // save for unsetInner

	dtw_part_y = _DTW_and_Sounds_getPartY (g, dtw_part_x);

	/* DTW */

	vp = Graphics_insetViewport (g, 1 - dtw_part_x, 1, 1 - dtw_part_y, 1);
	DTW_paintDistances_raw (me, g, xmin, xmax, ymin, ymax, 0, 0, 0, 0);
	DTW_drawPath_raw (me, g, xmin, xmax, ymin, ymax, 0, 0);
	drawBox(g);
	Graphics_resetViewport (g, vp);

	/* Sound y */

	vp = Graphics_insetViewport (g, 0, 1 - dtw_part_x, 1 - dtw_part_y, 1);
	Sound_draw_btlr (y, g, ymin, ymax, -1, 1, FROM_BOTTOM_TO_TOP, 0);
	if (garnish) drawBox(g);
	Graphics_resetViewport (g, vp);

	/* Sound x */

	vp = Graphics_insetViewport (g, 1 - dtw_part_x, 1, 0, 1 - dtw_part_y);
	Sound_draw_btlr (x, g, xmin, xmax, -1, 1, FROM_LEFT_TO_RIGHT, 0);
	if (garnish) drawBox(g);
	Graphics_resetViewport (g, vp);


	/* Set window coordinates so that margins will work, i.e. extend time domains */

	xmin3 = xmax - (xmax - xmin) / dtw_part_x;
	ymin3 = ymax - (ymax - ymin) / dtw_part_y;
	Graphics_setWindow (g, xmin3, xmax, ymin3, ymax);

	g -> outerViewport = ovp; // restore from _setInner
	Graphics_unsetInner (g);

	if (garnish)
	{
		Graphics_markLeft (g, ymin, 1, 1, 0, NULL);
		Graphics_markLeft (g, ymax, 1, 1, 0, NULL);

		Graphics_markBottom (g, xmin, 1, 1, 0, NULL);
		Graphics_markBottom (g, xmax, 1, 1, 0, NULL);
	}
}

void DTW_and_Sounds_drawWarpX (DTW me, Sound yy, Sound xx, Graphics g, double xmin, double xmax,
	double ymin, double ymax, double tx, int garnish)
{
	Sound y = yy, x = xx;
	double dtw_part_x = 0.85, dtw_part_y;
	double ty = DTW_getYTime (me, tx);
	int lineType = Graphics_inqLineType (g);

	if (! DTW_and_Sounds_checkDomains (me, &y, &x, &xmin, &xmax, &ymin, &ymax )) return;

	Graphics_setInner (g);
	dtw_part_y = _DTW_and_Sounds_getPartY (g, dtw_part_x);

	xmin = xmax - (xmax - xmin) / dtw_part_x;
	ymin = ymax - (ymax - ymin) / dtw_part_y;
	Graphics_setWindow (g, xmin, xmax, ymin, ymax);

	ty = DTW_getYTime (me, tx);
	Graphics_setLineType (g, Graphics_DOTTED);

	Graphics_line (g, tx, ymin, tx, ty);
	Graphics_line (g, tx, ty, xmin, ty);

	Graphics_setLineType (g, lineType);

	Graphics_unsetInner (g);

	if (garnish)
	{
		Graphics_markBottom (g, tx, 1, 1, 0, NULL);
		Graphics_markLeft (g, ty, 1, 1, 0, NULL);
	}
}

/* nog aanpassen, dl = sqrt (dx^2 + dy^2) */
void DTW_drawDistancesAlongPath (DTW me, Any g, double xmin, double xmax,
	double dmin, double dmax, int garnish)
{
	long i, ixmax, ixmin;
	double *d = NULL;

	if (! (d = NUMdvector (1, my nx))) return;

	if (xmin >= xmax)
	{
		xmin = my xmin; xmax = my xmax;
	}
	if(	! Matrix_getWindowSamplesX (me, xmin, xmax, &ixmin, &ixmax)) return;

	i = 1;
	while (i < my pathLength && my path[i].x < ixmin) i++;
	ixmin = i;

	while (i <= my pathLength && my path[i].x < ixmax) i++;
	ixmax = i;

	if ((d = NUMdvector (ixmin, ixmax)) == NULL) return;

	for (i = ixmin; i <= ixmax; i++)
	{
		d[i] = my z[my path[i].y][i];
	}

	if (dmin >= dmax)
	{
		NUMdvector_extrema (d, ixmin, ixmax, &dmin, &dmax);
	}
	else
	{
		for (i = ixmin; i <= ixmax; i++)
		{
			if (d[i] > dmax)
			{
				d[i] = dmax;
			}
			else if (d[i] < dmin)
			{
				d[i] = dmin;
			}
		}
	}

	Graphics_setInner (g);
	Graphics_setWindow (g, xmin, xmax, dmin, dmax);
	Graphics_function (g, d, ixmin, ixmax, xmin, xmax);
	Graphics_unsetInner (g);

	if (garnish)
	{
		Graphics_drawInnerBox(g);
		Graphics_textLeft (g, 1, L"distance");
		Graphics_marksBottom (g, 2, 1, 1, 0);
		Graphics_marksLeft (g, 2, 1, 1, 0);
	}

	NUMdvector_free (d, ixmin);
}

FORM (DTW_and_Sounds_draw, L"DTW & Sounds: Draw", L"DTW & Sounds: Draw...")
	REAL (L"left Horizontal range", L"0.0")
	REAL (L"right Horizontal range", L"0.0")
	REAL (L"left Vertical range", L"0.0")
	REAL (L"right Vertical range", L"0.0")
	BOOLEAN (L"Garnish", 1)
	OK
DO
	Sound s1 = NULL, s2 = NULL;
	WHERE (SELECTED && CLASS == classSound)
	{
		if (s1) s2 = (structSound *)OBJECT; else s1 = (structSound *)OBJECT;
	}
	praat_picture_open ();
	DTW_and_Sounds_draw ((structDTW *)ONLY (classDTW), s2, s1, GRAPHICS,
		GET_REAL (L"left Horizontal range"), GET_REAL (L"right Horizontal range"),
		GET_REAL (L"left Vertical range"), GET_REAL (L"right Vertical range"),
		GET_INTEGER (L"Garnish"));
	praat_picture_close ();
	return 1;
END

FORM (DTW_and_Sounds_drawWarpX, L"DTW & Sounds: Draw warp (x)", L"DTW & Sounds: Draw warp (x)...")
	REAL (L"left Horizontal range", L"0.0")
	REAL (L"right Horizontal range", L"0.0")
	REAL (L"left Vertical range", L"0.0")
	REAL (L"right Vertical range", L"0.0")
	REAL (L"Time (s)", L"0.1")
	BOOLEAN (L"Garnish", 1)
	OK
DO
	Sound s1 = NULL, s2 = NULL;
	WHERE (SELECTED && CLASS == classSound)
	{
		if (s1) s2 = (structSound *)OBJECT; else s1 = (structSound *)OBJECT;
	}
	praat_picture_open ();
	DTW_and_Sounds_drawWarpX ((structDTW *)ONLY (classDTW), s2, s1, GRAPHICS,
		GET_REAL (L"left Horizontal range"), GET_REAL (L"right Horizontal range"),
		GET_REAL (L"left Vertical range"), GET_REAL (L"right Vertical range"),
		GET_REAL (L"Time"), GET_INTEGER (L"Garnish"));
	praat_picture_close ();
	return 1;
END

void DTW_constraints_addCommonFields (UiForm *dia)
{
	UiForm::UiField *radio;
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Boundary conditions")
	BOOLEAN (L"Match begin positions", 0)
	BOOLEAN (L"Match end positions", 0)
	RADIO (L"Slope constraints", 1)
	RADIOBUTTON (L"no restriction")
	RADIOBUTTON (L"1/3 < slope < 3")
	RADIOBUTTON (L"1/2 < slope < 2")
	RADIOBUTTON (L"2/3 < slope < 3/2")
}

void DTW_constraints_getCommonFields (UiForm *dia, int *begin, int *end, int *slope)
{
	*begin = GET_INTEGER (L"Match begin positions");
	*end = GET_INTEGER (L"Match end positions");
	*slope = GET_INTEGER (L"Slope constraints");
}

DIRECT (DTW_help) Melder_help (L"DTW"); END

FORM (DTW_drawPath, L"DTW: Draw path", 0)
    REAL (L"left Horizontal range", L"0.0")
    REAL (L"right Horizontal range", L"0.0")
    REAL (L"left Vertical range", L"0.0")
    REAL (L"right Vertical range", L"0.0")
    BOOLEAN (L"Garnish", 0);
    OK
DO
    EVERY_DRAW (DTW_drawPath ((structDTW *)OBJECT, GRAPHICS,
		GET_REAL (L"left Horizontal range"), GET_REAL (L"right Horizontal range"),
		GET_REAL (L"left Vertical range"), GET_REAL (L"right Vertical range"),
		GET_INTEGER (L"Garnish")))
END

FORM (DTW_drawDistancesAlongPath, L"DTW: Draw distances along path", 0)
    REAL (L"left Horizontal range", L"0.0")
    REAL (L"right Horizontal range", L"0.0")
    REAL (L"left Vertical range", L"0.0")
    REAL (L"right Vertical range", L"0.0")
    BOOLEAN (L"Garnish", 0);
    OK
DO
    EVERY_DRAW (DTW_drawDistancesAlongPath ((structDTW *)OBJECT, GRAPHICS,
		GET_REAL (L"left Horizontal range"), GET_REAL (L"right Horizontal range"),
		GET_REAL (L"left Vertical range"), GET_REAL (L"right Vertical range"),
		GET_INTEGER (L"Garnish")))
END

FORM (DTW_paintDistances, L"DTW: Paint distances", 0)
	REAL (L"left Horizontal range", L"0.0")
	REAL (L"right Horizontal range", L"0.0")
	REAL (L"left Vertical range", L"0.0")
	REAL (L"right Vertical range", L"0.0")
	REAL (L"Minimum", L"0.0")
	REAL (L"Maximum", L"0.0")
    BOOLEAN (L"Garnish", 0);
	OK
DO
	EVERY_DRAW (DTW_paintDistances ((structDTW *)OBJECT, GRAPHICS,
		GET_REAL (L"left Horizontal range"), GET_REAL (L"right Horizontal range"),
		GET_REAL (L"left Vertical range"), GET_REAL (L"right Vertical range"),
		GET_REAL (L"Minimum"), GET_REAL (L"Maximum"),
		GET_INTEGER (L"Garnish")))
END

FORM (DTW_drawWarpX, L"DTW: Draw warp (x)", L"DTW: Draw warp (x)...")
	REAL (L"left Horizontal range", L"0.0")
	REAL (L"right Horizontal range", L"0.0")
	REAL (L"left Vertical range", L"0.0")
	REAL (L"right Vertical range", L"0.0")
	REAL (L"Time (s)", L"0.1")
    BOOLEAN (L"Garnish", 0);
	OK
DO
	EVERY_DRAW (DTW_drawWarpX ((structDTW *)OBJECT, GRAPHICS,
		GET_REAL (L"left Horizontal range"), GET_REAL (L"right Horizontal range"),
		GET_REAL (L"left Vertical range"), GET_REAL (L"right Vertical range"),
		GET_REAL (L"Time"), GET_INTEGER (L"Garnish")))
END

FORM (DTW_getPathY, L"DTW: Get time along path", L"DTW: Get time along path...")
	REAL (L"Time (s)", L"0.0")
	OK
DO
	Melder_information1 (Melder_double (DTW_getPathY ((structDTW *)ONLY_OBJECT, GET_REAL (L"Time"))));
END

FORM (DTW_getYTime, L"DTW: Get y time", L"DTW: Get y time...")
	REAL (L"Time at x (s)", L"0.0")
	OK
DO
	Melder_information1 (Melder_double (DTW_getYTime ((structDTW *)ONLY_OBJECT, GET_REAL (L"Time at x"))));
END

FORM (DTW_getXTime, L"DTW: Get x time", L"DTW: Get x time...")
	REAL (L"Time at y (s)", L"0.0")
	OK
DO
	Melder_information1 (Melder_double (DTW_getXTime ((structDTW *)ONLY_OBJECT, GET_REAL (L"Time at y"))));
END

FORM (DTW_getMaximumConsecutiveSteps, L"DTW: Get maximum consecutive steps", L"DTW: Get maximum consecutive steps...")
	OPTIONMENU (L"Direction", 1)
	OPTION (L"X")
	OPTION (L"Y")
	OPTION (L"Diagonaal")
	OK
DO
	int direction[] = {DTW_START, DTW_X, DTW_Y, DTW_XANDY};
	wchar_t *string[] = {L"ui/editors/AmplitudeTierEditor.h", L"x", L"y", L"diagonal"};
	int d = GET_INTEGER (L"Direction");
	Melder_information4 (Melder_integer (DTW_getMaximumConsecutiveSteps ((structDTW *)ONLY_OBJECT, direction[d])),
		L" (=maximum number of consecutive steps in ", string[d], L" direction");
END

DIRECT (DTW_getWeightedDistance)
	DTW me = (structDTW *)ONLY_OBJECT;
	Melder_information1 (Melder_double (my weightedDistance));
END

FORM (DTW_findPath, L"DTW: Find path", 0)
	DTW_constraints_addCommonFields (dia);
	OK
DO
	int begin, end, slope;
	DTW_constraints_getCommonFields (dia, &begin, &end, &slope);
	EVERY (DTW_findPath ((structDTW *)OBJECT, begin, end, slope))
END

#ifdef INCLUDE_DTW_SLOPES
FORM (DTW_pathFinder_slopes, L"DTW: Find path (slopes)", L"DTW: Find path (slopes)...")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Slope constraints:")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"This number of")
	INTEGER (L"Non-diagonal steps", L"1")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"must be followed by at least this number of")
	INTEGER (L"Diagonal steps", L"0 (=no constraints)")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Directional weights")
	REAL (L"X weight", L"1.0")
	REAL (L"Y weight", L"1.0")
	REAL (L"Diagonal weight", L"2.0")
	OK
DO
	EVERY_CHECK (DTW_pathFinder_slopes ((structDTW *)OBJECT, GET_INTEGER (L"Non-diagonal steps"), GET_INTEGER (L"Diagonal steps"),
		GET_REAL (L"X weight"), GET_REAL (L"Y weight"), GET_REAL (L"Diagonal weight")))
END
#endif

FORM (DTW_pathFinder_band, L"DTW: Find path (Sakoe-Chiba band)", L"DTW: Find path (band)...")
	REAL (L"Adjustment window duration (s)", L"0.1")
	BOOLEAN (L"Adjustment window includes end", 0)
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Directional weights")
	REAL (L"X weight", L"1.0")
	REAL (L"Y weight", L"1.0")
	REAL (L"Diagonal weight", L"2.0")
	OK
DO
	EVERY_CHECK (DTW_pathFinder_band ((structDTW *)OBJECT, GET_REAL (L"Adjustment window duration"),
		GET_INTEGER (L"Adjustment window includes end"),
		GET_REAL (L"X weight"), GET_REAL (L"Y weight"), GET_REAL (L"Diagonal weight")))
END

FORM (DTW_to_Polygon_slopes, L"DTW: To Polygon (slopes)", L"DTW: To Polygon (slopes)...")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Slope constraints:")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"This number of")
	INTEGER (L"Non-diagonal steps", L"1")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"must be followed by at least this number of")
	INTEGER (L"Diagonal steps", L"0 (=no constraints)")
	OK
DO
	EVERY_TO (DTW_to_Polygon_slopes ((structDTW *)OBJECT, GET_INTEGER (L"Non-diagonal steps"), GET_INTEGER (L"Diagonal steps")))
END

FORM (DTW_to_Polygon_band, L"DTW: To Polygon (band)", L"DTW: To Polygon (band)...")
	REAL (L"Adjustment window duration (s)", L"0.1")
	BOOLEAN (L"Adjustment window includes end", 0)
	OK
DO
	EVERY_TO (DTW_to_Polygon_band ((structDTW *)OBJECT, GET_REAL (L"Adjustment window duration"), GET_INTEGER (L"Adjustment window includes end")))
END

DIRECT (DTW_distancesToMatrix)
	EVERY_TO (DTW_distancesToMatrix ((structDTW *)OBJECT))
END

DIRECT (DTW_swapAxes)
	EVERY_TO (DTW_swapAxes ((structDTW *)OBJECT))
END

DIRECT (DTW_and_TextGrid_to_TextGrid)
	NEW (DTW_and_TextGrid_to_TextGrid ((structDTW *)ONLY (classDTW), (structTextGrid *)ONLY (classTextGrid)))
END
/******************** Eigen ********************************************/

static void Graphics_ticks (Graphics g, double min, double max, int hasNumber,
	int hasTick, int hasDottedLine, int integers)
{
	double range = max - min, scale = 1, tick = min, dtick = 1;

	if (range == 0)
	{
		return;
	}
	else if (range > 1)
	{
		while (range / scale > 10) scale *= 10;
		range /= scale;
	}
	else
	{
		while (range / scale < 10) scale /= 10;
		range *= scale;
	}

	if (range < 3) dtick = 0.5;
	dtick *= scale;
	tick = dtick * floor (min / dtick);
	if (tick < min) tick += dtick;
	while (tick <= max)
	{
		double num = integers ? floor (tick + 0.5) : tick;
		Graphics_markBottom (g, num, hasNumber, hasTick, hasDottedLine, NULL);
		tick += dtick;
	}
}

void Eigen_drawEigenvalues (I, Graphics g, long first, long last, double ymin, double ymax,
	int fractionOfTotal, int cumulative, double size_mm, const wchar_t *mark, int garnish)
{
	iam (Eigen);
	double xmin = first, xmax = last, scale = 1, sumOfEigenvalues = 0;
	long i;

	if (first < 1) first = 1;
	if (last < 1 || last > my numberOfEigenvalues) last = my numberOfEigenvalues;
	if (last <= first )
	{
		first = 1; last = my numberOfEigenvalues;
	}
	xmin = first - 0.5; xmax = last + 0.5;
	if (fractionOfTotal || cumulative)
	{
		sumOfEigenvalues = Eigen_getSumOfEigenvalues (me, 0, 0);
		if (sumOfEigenvalues <= 0)  sumOfEigenvalues = 1;
		scale = sumOfEigenvalues;
	}
	if (ymax <= ymin)
	{
		ymax = Eigen_getSumOfEigenvalues (me, (cumulative ? 1 : first), first) / scale;
		ymin = Eigen_getSumOfEigenvalues (me, (cumulative ? 1 : last), last) / scale;
		if (ymin > ymax)
		{
			double tmp = ymin; ymin = ymax; ymax = tmp;
		}
	}
	Graphics_setInner (g);
	Graphics_setWindow (g, xmin, xmax, ymin, ymax);
	for (i = first; i <= last; i++)
	{
		double accu = Eigen_getSumOfEigenvalues (me, (cumulative ? 1 : i), i);
		Graphics_mark (g, i, accu / scale, size_mm, mark);
	}
	Graphics_unsetInner (g);
	if (garnish)
	{
    	Graphics_drawInnerBox (g);
		Graphics_textLeft (g, 1, fractionOfTotal ? (cumulative ? L"Cumulative fractional eigenvalue" : L"Fractional eigenvalue") :
			(cumulative ? L"Cumulative eigenvalue" : L"Eigenvalue"));
		Graphics_ticks (g, first, last, 1, 1, 0, 1);
    	Graphics_marksLeft (g, 2, 1, 1, 0);
		Graphics_textBottom (g, 1, L"Index");
	}
}

void Eigen_drawEigenvector (I, Graphics g, long ivec, long first, long last,
	double ymin, double ymax, int weigh, double size_mm, const wchar_t *mark,
	int connect, wchar_t **rowLabels, int garnish)
{
	iam (Eigen);
	long i;
	double xmin = first, xmax = last, *vec, w;

	if (ivec < 1 || ivec > my numberOfEigenvalues) return;

	if (last <= first )
	{
		first = 1; last = my dimension;
		xmin = 0.5; xmax = last + 0.5;
	}
	vec = my eigenvectors[ivec];
	w = weigh ? sqrt (my eigenvalues[ivec]) : 1;
	/*
		If ymax < ymin the eigenvector will automatically be drawn inverted.
	*/

	if (ymax == ymin)
	{
		NUMdvector_extrema (vec, first, last, &ymin, &ymax);
		ymax *= w; ymin *= w;
	}
	Graphics_setInner (g);
	Graphics_setWindow (g, xmin, xmax, ymin, ymax);

	for (i = first; i <= last; i++)
	{
		Graphics_mark (g, i, w * vec[i], size_mm, mark);
		if (connect && i > first) Graphics_line (g, i - 1, w * vec[i-1], i, w * vec[i]);
	}
	Graphics_unsetInner (g);
	if (garnish)
	{
		Graphics_markBottom (g, first, 0, 1, 0, rowLabels ? rowLabels[first] : Melder_integer (first));
		Graphics_markBottom (g, last, 0, 1, 0, rowLabels ? rowLabels[last] : Melder_integer (last));
    	Graphics_drawInnerBox (g);
    	if (ymin * ymax < 0) Graphics_markLeft (g, 0.0, 1, 1, 1, NULL);
    	Graphics_marksLeft (g, 2, 1, 1, 0);
		if (rowLabels == NULL) Graphics_textBottom (g, 1, L"Element number");
	}
}

DIRECT (Eigen_drawEigenvalues_scree)
	Melder_warning1 (L"The command \"Draw eigenvalues (scree)...\" has been "
		"removed.\n To get a scree plot use \"Draw eigenvalues...\" with the "
		"arguments\n 'Fraction of eigenvalues summed' and 'Cumulative' unchecked.");
END

FORM (Eigen_drawEigenvalues, L"Eigen: Draw eigenvalues", L"Eigen: Draw eigenvalues...")
	INTEGER (L"left Eigenvalue range", L"0")
	INTEGER (L"right Eigenvalue range", L"0")
	REAL (L"left Amplitude range", L"0.0")
	REAL (L"right Amplitude range", L"0.0")
	BOOLEAN (L"Fraction of eigenvalues summed", 0)
	BOOLEAN (L"Cumulative", 0)
	POSITIVE (L"Mark size (mm)", L"1.0")
	SENTENCE (L"Mark string (+xo.)", L"+")
	BOOLEAN (L"Garnish", 1)
	OK
DO
	EVERY_DRAW (Eigen_drawEigenvalues (OBJECT, GRAPHICS,
		GET_INTEGER (L"left Eigenvalue range"), GET_INTEGER (L"right Eigenvalue range"),
		GET_REAL (L"left Amplitude range"), GET_REAL (L"right Amplitude range"),
		GET_INTEGER (L"Fraction of eigenvalues summed"), GET_INTEGER (L"Cumulative"),
		GET_REAL (L"Mark size"),
		GET_STRING (L"Mark string"), GET_INTEGER (L"Garnish")))
END

FORM (Eigen_drawEigenvector, L"Eigen: Draw eigenvector", L"Eigen: Draw eigenvector...")
	INTEGER (L"Eigenvector number", L"1")
	BOOLEAN (L"Component loadings", 0)
	INTEGER (L"left Element range", L"0")
	INTEGER (L"right Element range", L"0")
	REAL (L"left Amplitude range", L"-1.0")
	REAL (L"right Amplitude range", L"1.0")
	POSITIVE (L"Mark size (mm)", L"1.0")
	SENTENCE (L"Mark string (+xo.)", L"+")
	BOOLEAN (L"Connect points", 1)
	BOOLEAN (L"Garnish", 1)
	OK
DO
	EVERY_DRAW (Eigen_drawEigenvector (OBJECT, GRAPHICS,
		GET_INTEGER (L"Eigenvector number"),
		GET_INTEGER (L"left Element range"), GET_INTEGER (L"right Element range"),
		GET_REAL (L"left Amplitude range"), GET_REAL (L"right Amplitude range"),
		GET_INTEGER (L"Component loadings"), GET_REAL (L"Mark size"),
		GET_STRING (L"Mark string"), GET_INTEGER (L"Connect points"), NULL,
		GET_INTEGER (L"Garnish")))
END

DIRECT (Eigen_getNumberOfEigenvalues)
	Eigen me = (structEigen *)ONLY_OBJECT;
	Melder_information1 (Melder_integer (my numberOfEigenvalues));
END

DIRECT (Eigen_getDimension)
	Eigen me = (structEigen *)ONLY_OBJECT;
	Melder_information1 (Melder_integer (my dimension));
END

FORM (Eigen_getEigenvalue, L"Eigen: Get eigenvalue", L"Eigen: Get eigenvalue...")
	NATURAL (L"Eigenvalue number", L"1")
	OK
DO
	Eigen me = (structEigen *)ONLY_OBJECT;
	long number = GET_INTEGER (L"Eigenvalue number");
	if (number > my numberOfEigenvalues) return Melder_error2 (L"DO_Eigen_getEigenvalue: Eigenvalue number must be smaller than ", Melder_integer(my numberOfEigenvalues + 1));
	Melder_information1 (Melder_double (my eigenvalues[number]));
END

FORM (Eigen_getSumOfEigenvalues, L"Eigen:Get sum of eigenvalues", L"Eigen: Get sum of eigenvalues...")
	INTEGER (L"left Eigenvalue range",  L"0")
	INTEGER (L"right Eigenvalue range", L"0")
	OK
DO
	Melder_information1 (Melder_double (Eigen_getSumOfEigenvalues(ONLY_OBJECT,
		GET_INTEGER (L"left Eigenvalue range"), GET_INTEGER (L"right Eigenvalue range"))));
END

FORM (Eigen_getEigenvectorElement, L"Eigen: Get eigenvector element", L"Eigen: Get eigenvector element...")
	NATURAL (L"Eigenvector number", L"1")
	NATURAL (L"Element number", L"1")
	OK
DO
	Melder_information1 (Melder_double (Eigen_getEigenvectorElement (ONLY_OBJECT,
		GET_INTEGER (L"Eigenvector number"), GET_INTEGER (L"Element number"))));
END

DIRECT (Eigens_alignEigenvectors)
	Ordered c = Ordered_create ();
	int status = 0;
	if (! c) return 0;
	WHERE (SELECTED)
	{
		if (! Collection_addItem (c, OBJECT)) goto end;
	}
	status = Eigens_alignEigenvectors (c);
end:
	c -> size = 0;
	forget (c);
	if (status == 0) return 0;
END

FORM (Eigen_and_Matrix_project, L"Eigen & Matrix: Project", L"Eigen & Matrix: Project...")
	INTEGER (L"Number of dimensions", L"0")
	OK
DO
	NEW (Eigen_and_Matrix_project ((structEigen *)ONLY_GENERIC (classEigen),
		(structMatrix *)ONLY_GENERIC (classMatrix), GET_INTEGER (L"Number of dimensions")))
END

DIRECT (Eigen_and_SSCP_project)
	NEW (Eigen_and_SSCP_project ((structEigen *)ONLY_GENERIC (classEigen),
		(structSSCP *)ONLY (classSSCP)))
END

DIRECT (Eigen_and_Covariance_project)
	NEW (Eigen_and_Covariance_project ((structEigen *)ONLY_GENERIC (classEigen),
		(structCovariance *)ONLY (classCovariance)))
END

/******************** Index ********************************************/

DIRECT (Index_help)
	Melder_help (L"Index");
END

DIRECT (Index_getNumberOfClasses)
	Index thee = (structIndex *)ONLY_OBJECT;
	Melder_information1 (Melder_integer (thy classes -> size));
END

FORM (StringsIndex_getClassLabel, L"StringsIndex: Get class label", L"StringsIndex: Get class label...")
	NATURAL (L"Class index", L"1")
	OK
DO
	StringsIndex thee = (structStringsIndex *)ONLY_OBJECT;
	long klas = GET_INTEGER (L"Class index");
	long numberOfClasses = thy classes -> size;
	SimpleString ss;
	if (klas > numberOfClasses) return Melder_error3 (L"Element index must be less than or equal ", Melder_integer (numberOfClasses), L".");
	ss = (structSimpleString *)thy classes -> item[klas];
	Melder_information1 (ss -> string);
END

FORM (StringsIndex_getLabel, L"StringsIndex: Get label", L"StringsIndex: Get label...")
	NATURAL (L"Element index", L"1")
	OK
DO
	StringsIndex thee = (structStringsIndex *)ONLY_OBJECT;
	long klas, index = GET_INTEGER (L"Element index");
	SimpleString ss;
	if (index > thy numberOfElements) return Melder_error3 (L"Element index must be less than or equal ", Melder_integer (thy numberOfElements), L".");
	klas = thy classIndex[index];
	ss = (structSimpleString *)thy classes -> item [klas];
	Melder_information1 (ss -> string);
END

FORM (Index_getIndex, L"Index: Get index", L"Index: Get index...")
	NATURAL (L"Element index", L"1")
	OK
DO
	Index thee = (structIndex *)ONLY_OBJECT;
	long index = GET_INTEGER (L"Element index");
	if (index > thy numberOfElements) return Melder_error3 (L"Element index must be less than or equal ", Melder_integer (thy numberOfElements), L".");
	Melder_information1 (Melder_integer (thy classIndex[index]));
END

FORM (StringsIndex_getClassIndex, L"StringsIndex: Get class index", L"StringsIndex: Get class index...")
	WORD (L"Class label", L"label")
	OK
DO
	StringsIndex thee = (structStringsIndex *)ONLY_OBJECT;
	wchar_t *klasLabel = GET_STRING (L"Class label");
	long index = StringsIndex_getClass (thee, klasLabel);
	Melder_information1 (Melder_integer (index));
END

FORM (Index_extractPart, L"Index: Extract part", L"Index: Extract part...")
	INTEGER (L"left Range", L"0")
	INTEGER (L"right Range", L"0")
	OK
DO
	Index thee = (structIndex *)ONLY_OBJECT;
	if (! praat_new2 (Index_extractPart (thee, GET_INTEGER (L"left Range"), GET_INTEGER (L"right Range")),
		Thing_getName (thee), L"_part")) return 0;
END

FORM (Index_to_Permutation, L"Index: To Permutation", L"Index: To Permutation...")
	BOOLEAN (L"Permute within classes", 1)
	OK
DO
	EVERY_TO (Index_to_Permutation_permuteRandomly (OBJECT, GET_INTEGER (L"Permute within classes")))
END

DIRECT (StringsIndex_to_Strings)
	EVERY_TO (StringsIndex_to_Strings ((structStringsIndex *)OBJECT))
END

/******************** Excitation ********************************************/

DIRECT (Excitation_to_Excitations)
	Excitations e = Excitations_create (100);
	if (! e) return 0;
	WHERE_DOWN (SELECTED)
	{
		(void) Collection_addItem (e, Data_copy (OBJECT));
	}
	praat_show();
	NEW (e)
END

/******************** Excitations ********************************************/

FORM (Excitations_formula,L"Excitations: Formula", 0)
	LABEL (L"label", L"for all objects in Excitations do { for col := 1 to ncol do { self [col] := `formula' ; x := x + dx } }")
	TEXTFIELD (L"formula", L"self")
	OK
DO
	WHERE (SELECTED && Thing_member (OBJECT, classExcitations))
	{
		Ordered ms = (structOrdered *)OBJECT;
		int j;
		for (j = 1; j <= ms -> size; j++)
			if (! Matrix_formula ((structMatrix *)ms->item[j], GET_STRING (L"formula"), interpreter, NULL)) break;
		praat_dataChanged (OBJECT);
		iferror return 0;
	}
END

DIRECT (Excitations_addItem)
	Excitations e = NULL;
	WHERE (SELECTED && CLASS == classExcitations) e = (structExcitations *)OBJECT;
	WHERE_DOWN (SELECTED && CLASS == classExcitation)
	{
		(void) Collection_addItem (e, Data_copy (OBJECT));
		praat_show();
	}
	praat_show();
END

FORM (Excitations_getItem, L"Excitations: Get item", 0)
	NATURAL (L"Item number", L"1")
	OK
DO
	WHERE (SELECTED && CLASS == classExcitations)
	{
		Excitation me = (structExcitation *)Excitations_getItem ((structExcitations *)OBJECT, GET_INTEGER (L"Item number"));
		if (me == NULL || ! praat_new1 (me, Thing_getName (me))) return 0;
	}
END

DIRECT (Excitations_append)
   Data e1 = NULL, e2 = NULL;
   WHERE (SELECTED && CLASS == classExcitations) { if (e1) e2 = (structData *)OBJECT; else e1 = (structData *)OBJECT; }
   NEW (Collections_merge (e1, e2))
END

FORM (Excitations_to_Pattern,L"Excitations: To Pattern", 0)
	NATURAL (L"Join", L"1")
	OK
DO
    EVERY_TO (Excitations_to_Pattern ((structExcitations *)OBJECT, GET_INTEGER (L"Join")))
END

DIRECT (Excitations_to_TableOfReal)
	EVERY_TO (Excitations_to_TableOfReal ((structExcitations *)OBJECT))
END

/************************* FilterBank ***********************************/

double FilterBank_getFrequencyInHertz (I, double f, int scale_from)
{
	(void) void_me; 
	return scaleFrequency (f, scale_from, FilterBank_HERTZ);
}

double FilterBank_getFrequencyInBark (I, double f, int scale_from)
{
	(void) void_me; 
	return scaleFrequency (f, scale_from, FilterBank_BARK);
}

double FilterBank_getFrequencyInMel (I, double f, int scale_from)
{
	(void) void_me; 
	return scaleFrequency (f, scale_from, FilterBank_MEL);
}

/*void FilterBank_and_PCA_drawComponent (I, PCA thee, Graphics g, long component, double dblevel,
	double frequencyOffset, double scale, double tmin, double tmax, double fmin, double fmax)
{
	iam (FilterBank);
	FilterBank fcopy = NULL;
	Matrix him = NULL;
	long j;
	
	if (component < 1 || component > thy numberOfEigenvalues)
	{
		(void) Melder_error1 (L"Component too large.");
	}
	// Scale Intensity
	fcopy = Data_copy (me);
	if (fcopy == NULL)  return;
	FilterBank_equalizeIntensities (fcopy, dblevel);
	him = Eigen_and_Matrix_project (thee, fcopy, component);
	if (him == NULL) goto end;
	for (j = 1; j<= my nx; j++)
	{
		fcopy -> z[component][j] = frequencyOffset + scale * fcopy -> z[component][j];	
	}
	Matrix_drawRows (fcopy, g, tmin, tmax, component-0.5, component+0.5, fmin, fmax);
end:
	forget (fcopy);
	forget (him);
}*/

FORM (FilterBank_drawFilters, L"FilterBank: Draw filters", 0)
	REAL (L"left Time range (s)", L"0.0")
	REAL (L"right Time range (s)", L"0.0")
	REAL (L"left Frequency range", L"0.0")
	REAL (L"right Frequency range", L"0.0")
	REAL (L"left Amplitude range", L"0.0")
	REAL (L"right Amplitude range", L"0.0")
	OK
DO
	EVERY_DRAW (Matrix_drawRows ((structMatrix *)OBJECT, GRAPHICS,
		GET_REAL (L"left Time range"), GET_REAL (L"right Time range"),
		GET_REAL (L"left Frequency range"), GET_REAL (L"right Frequency range"),
		GET_REAL (L"left Amplitude range"), GET_REAL (L"right Amplitude range")))
END

FORM (FilterBank_drawOneContour, L"FilterBank: Draw one contour", 0)
	REAL (L"left Time range (s)", L"0.0")
	REAL (L"right Time range (s)", L"0.0")
	REAL (L"left Frequency range", L"0.0")
	REAL (L"right Frequency range", L"0.0")
	REAL (L"Height (dB)", L"40.0")
	OK
DO
	EVERY_DRAW (Matrix_drawOneContour ((structMatrix *)OBJECT, GRAPHICS,
		GET_REAL (L"left Time range"), GET_REAL (L"right Time range"),
		GET_REAL (L"left Frequency range"), GET_REAL (L"right Frequency range"),
		GET_REAL (L"Height")))
END

FORM (FilterBank_drawContours, L"FilterBank: Draw contours", 0)
	REAL (L"left Time range (s)", L"0.0")
	REAL (L"right Time range (s)", L"0.0")
	REAL (L"left Frequency range", L"0.0")
	REAL (L"right Frequency range", L"0.0")
	REAL (L"left Amplitude range", L"0.0")
	REAL (L"right Amplitude range", L"0.0")
	OK
DO
	EVERY_DRAW (Matrix_drawContours ((structMatrix *)OBJECT, GRAPHICS,
		GET_REAL (L"left Time range"), GET_REAL (L"right Time range"),
		GET_REAL (L"left Frequency range"), GET_REAL (L"right Frequency range"),
		GET_REAL (L"left Amplitude range"), GET_REAL (L"right Amplitude range")))
END

FORM (FilterBank_drawFrequencyScales, L"FilterBank: Draw frequency scales", L"FilterBank: Draw frequency scales...")
	RADIO (L"Horizontal frequency scale", 1)
	RADIOBUTTON (L"Hertz")
	RADIOBUTTON (L"Bark")
	RADIOBUTTON (L"mel")
	REAL (L"left Horizontal frequency range", L"0.0")
	REAL (L"right Horizontal frequency range", L"0.0")
	RADIO (L"Vertical frequency scale", 1)
	RADIOBUTTON (L"Hertz")
	RADIOBUTTON (L"Bark")
	RADIOBUTTON (L"mel")
	REAL (L"left Vertical frequency range", L"0.0")
	REAL (L"right Vertical frequency range", L"0.0")
	BOOLEAN (L"Garnish", 1)
	OK
DO
	EVERY_DRAW (FilterBank_drawFrequencyScales ((structMatrix *)OBJECT, GRAPHICS,
		GET_INTEGER (L"Horizontal frequency scale"),
		GET_REAL (L"left Horizontal frequency range"),
		GET_REAL (L"right Horizontal frequency range"),
		GET_INTEGER (L"Vertical frequency scale"),
		GET_REAL (L"left Vertical frequency range"),
		GET_REAL (L"right Vertical frequency range"),GET_INTEGER (L"Garnish")))
END

FORM (FilterBank_paintImage, L"FilterBank: Paint image", 0)
	REAL (L"left Time range (s)", L"0.0")
	REAL (L"right Time range (s)", L"0.0")
	REAL (L"left Frequency range", L"0.0")
	REAL (L"right Frequency range", L"0.0")
	REAL (L"left Amplitude range", L"0.0")
	REAL (L"right Amplitude range", L"0.0")
	OK
DO
	EVERY_DRAW (Matrix_paintImage ((structMatrix *)OBJECT, GRAPHICS,
		GET_REAL (L"left Time range"), GET_REAL (L"right Time range"),
		GET_REAL (L"left Frequency range"), GET_REAL (L"right Frequency range"),
		GET_REAL (L"left Amplitude range"), GET_REAL (L"right Amplitude range")))
END

FORM (FilterBank_paintContours, L"FilterBank: Paint contours", 0)
	REAL (L"left Time range (s)", L"0.0")
	REAL (L"right Time range (s)", L"0.0")
	REAL (L"left Frequency range", L"0.0")
	REAL (L"right Frequency range", L"0.0")
	REAL (L"left Amplitude range", L"0.0")
	REAL (L"right Amplitude range", L"0.0")
	OK
DO
	EVERY_DRAW (Matrix_paintContours ((structMatrix *)OBJECT, GRAPHICS,
		GET_REAL (L"left Time range"), GET_REAL (L"right Time range"),
		GET_REAL (L"left Frequency range"), GET_REAL (L"right Frequency range"),
		GET_REAL (L"left Amplitude range"), GET_REAL (L"right Amplitude range")))
END


FORM (FilterBank_paintCells, L"FilterBank: Paint cells", 0)
	REAL (L"left Time range (s)", L"0.0")
	REAL (L"right Time range (s)", L"0.0")
	REAL (L"left Frequency range", L"0.0")
	REAL (L"right Frequency range", L"0.0")
	REAL (L"left Amplitude range", L"0.0")
	REAL (L"right Amplitude range", L"0.0")
	OK
DO
	EVERY_DRAW (Matrix_paintCells ((structMatrix *)OBJECT, GRAPHICS,
		GET_REAL (L"left Time range"), GET_REAL (L"right Time range"),
		GET_REAL (L"left Frequency range"), GET_REAL (L"right Frequency range"),
		GET_REAL (L"left Amplitude range"), GET_REAL (L"right Amplitude range")))
END

FORM (FilterBank_paintSurface, L"FilterBank: Paint surface", 0)
	REAL (L"left Time range (s)", L"0.0")
	REAL (L"right Time range (s)", L"0.0")
	REAL (L"left Frequency range", L"0.0")
	REAL (L"right Frequency range", L"0.0")
	REAL (L"left Amplitude range", L"0.0")
	REAL (L"right Amplitude range", L"0.0")
	OK
DO
	EVERY_DRAW (Matrix_paintSurface ((structMatrix *)OBJECT, GRAPHICS,
		GET_REAL (L"left Time range"), GET_REAL (L"right Time range"),
		GET_REAL (L"left Frequency range"), GET_REAL (L"right Frequency range"),
		GET_REAL (L"left Amplitude range"), GET_REAL (L"right Amplitude range"),
		30, 45))
END

FORM (FilterBank_getFrequencyInHertz, L"FilterBank: Get frequency in Hertz", L"FilterBank: Get frequency in Hertz...")
	REAL (L"Frequency", L"10.0")
	RADIO (L"Unit", 2)
	RADIOBUTTON (L"Hertz")
	RADIOBUTTON (L"Bark")
	RADIOBUTTON (L"mel")
	OK
DO
	double f = FilterBank_getFrequencyInHertz ((structFilterBank *)ONLY_OBJECT,
		GET_REAL (L"Frequency"), GET_INTEGER (L"Unit"));
	Melder_informationReal (f, L"Hertz");
END

FORM (FilterBank_getFrequencyInBark, L"FilterBank: Get frequency in Bark", L"FilterBank: Get frequency in Bark...")
	REAL (L"Frequency", L"93.17")
	RADIO (L"Unit", 1)
	RADIOBUTTON (L"Hertz")
	RADIOBUTTON (L"Bark")
	RADIOBUTTON (L"mel")
	OK
DO
	Melder_informationReal (FilterBank_getFrequencyInBark ((structFilterBank *)ONLY_OBJECT, GET_REAL (L"Frequency"),
		GET_INTEGER (L"Unit")), L"Bark");
END

FORM (FilterBank_getFrequencyInMel, L"FilterBank: Get frequency in mel", L"FilterBank: Get frequency in mel...")
	REAL (L"Frequency", L"1000.0")
	RADIO (L"Unit", 1)
	RADIOBUTTON (L"Hertz")
	RADIOBUTTON (L"Bark")
	RADIOBUTTON (L"mel")
	OK
DO
	double f = FilterBank_getFrequencyInMel ((structFilterBank *)ONLY_OBJECT,
		GET_REAL (L"Frequency"), GET_INTEGER (L"Unit"));
	Melder_informationReal (f, L"mel");
END

FORM (FilterBank_equalizeIntensities, L"FilterBank: Equalize intensities", L"ui/editors/AmplitudeTierEditor.h")
	REAL (L"Intensity (dB)", L"80.0")
	OK
DO
	EVERY (FilterBank_equalizeIntensities ((structFilterBank *)OBJECT, GET_REAL (L"Intensity")))
END

DIRECT (FilterBank_to_Matrix)
	EVERY_TO (FilterBank_to_Matrix ((structFilterBank *)OBJECT))
END

DIRECT (FilterBank_to_Intensity)
	EVERY_TO (FilterBank_to_Intensity ((structFilterBank *)OBJECT))
END

/*********** FormantFilter *******************************************/

void FormantFilter_drawFilterFunctions (FormantFilter me, Graphics g, double bandwidth,
	int toFreqScale, int fromFilter, int toFilter, double zmin, double zmax, 
	int dbScale, double ymin, double ymax, int garnish)
{
	long i, j, n = 1000; 
	double *a = NULL;
		
	if (! checkLimits (me, FilterBank_HERTZ, toFreqScale, & fromFilter, & toFilter, 
		& zmin, & zmax, dbScale, & ymin, & ymax)) return;
	
	if (bandwidth <= 0)
	{
		Melder_warning1 (L"Bandwidth must be greater than zero.");
	}
		
	a = NUMdvector (1, n);
	if (a == NULL) return;
		
	Graphics_setInner (g);
	Graphics_setWindow (g, zmin, zmax, ymin, ymax);
	
	for (j = fromFilter; j <= toFilter; j++)
	{
		double df = (zmax - zmin) / (n - 1);
		double fc = my y1 + (j - 1) * my dy;
		long ibegin, iend;
		
		for (i = 1; i <= n; i++)
		{
			double z, f = zmin + (i - 1) * df;
			
			z = scaleFrequency (f, toFreqScale, FilterBank_HERTZ);
			if (z == NUMundefined)
			{
				a[i] = NUMundefined;
			}
			else
			{
				a[i] = NUMformantfilter_amplitude (fc, bandwidth, z);			
				if (dbScale) a[i] = to_dB (a[i], 10, ymin);
			}
		}
		
		setDrawingLimits (a, n, ymin, ymax,	&ibegin, &iend);
		
		if (ibegin <= iend)
		{
			double fmin = zmin + (ibegin - 1) * df;
			double fmax = zmax - (n - iend) * df;
			Graphics_function (g, a, ibegin, iend, fmin, fmax);
		}
	}
		
			
	Graphics_unsetInner (g);
	
	if (garnish)
	{
		double distance = dbScale ? 10 : 1;
		const wchar_t *ytext = dbScale ? L"Amplitude (dB)" : L"Amplitude";
		Graphics_drawInnerBox (g);
    	Graphics_marksBottom (g, 2, 1, 1, 0);
    	Graphics_marksLeftEvery (g, 1, distance, 1, 1, 0);
    	Graphics_textLeft (g, 1, ytext);
    	Graphics_textBottom (g, 1, GetFreqScaleText (toFreqScale));
	}
	
	NUMdvector_free (a, 1);
}

DIRECT (FormantFilter_help)
	Melder_help (L"FormantFilter");
END

FORM (FormantFilter_drawFilterFunctions, L"FormantFilter: Draw filter functions", L"FilterBank: Draw filter functions...")
	INTEGER (L"left Filter range", L"0")
	INTEGER (L"right Filter range", L"0")
	POSITIVE (L"Bandwidth (Hz)", L"100.0")
	RADIO (L"Frequency scale", 1)
	RADIOBUTTON (L"Hertz")
	RADIOBUTTON (L"Bark")
	RADIOBUTTON (L"mel")
	REAL (L"left Frequency range", L"0.0")
	REAL (L"right Frequency range", L"0.0")
	BOOLEAN (L"Amplitude scale in dB", 1)
	REAL (L"left Amplitude range", L"0.0")
	REAL (L"right Amplitude range", L"0.0")
	BOOLEAN (L"Garnish", 1)
	OK
DO
	EVERY_DRAW (FormantFilter_drawFilterFunctions ((structFormantFilter *)OBJECT, GRAPHICS,
		GET_REAL (L"Bandwidth"), GET_INTEGER (L"Frequency scale"),
		GET_INTEGER (L"left Filter range"), GET_INTEGER (L"right Filter range"),
		GET_REAL (L"left Frequency range"), GET_REAL (L"right Frequency range"),
		GET_INTEGER (L"Amplitude scale in dB"),
		GET_REAL (L"left Amplitude range"), GET_REAL (L"right Amplitude range"), GET_INTEGER (L"Garnish")))
END

FORM (FormantFilter_drawSpectrum, L"FormantFilter: Draw spectrum (slice)", L"FilterBank: Draw spectrum (slice)...")
	POSITIVE (L"Time (s)", L"0.1")
	REAL (L"left Frequency range (Hz)", L"0.0")
	REAL (L"right Frequency range (Hz)", L"0.0")
	REAL (L"left Amplitude range (dB)", L"0.0")
	REAL (L"right Amplitude range (dB)", L"0.0")
	BOOLEAN (L"Garnish", 1)
	OK
DO
	EVERY_DRAW (FilterBank_drawTimeSlice ((structFilterBank *)OBJECT, GRAPHICS,
		GET_REAL (L"Time"), GET_REAL (L"left Frequency range"),
		GET_REAL (L"right Frequency range"), GET_REAL (L"left Amplitude range"),
		GET_REAL (L"right Amplitude range"), L"Hz", GET_INTEGER (L"Garnish")))
END

/****************** FormantGrid  *********************************/

void RealTier_draw (I, Graphics g, double tmin, double tmax, double fmin, double fmax,
	int garnish, const wchar_t *method, const wchar_t *quantity);

void FormantGrid_draw (FormantGrid me, Graphics g, double xmin, double xmax, double ymin, double ymax, bool bandwidths, bool garnish, const wchar_t *method)
{
	Ordered tiers = bandwidths ? my bandwidths : my formants;
	
	if (xmax <= xmin)
	{
		xmin = my xmin; xmax = my xmax;
	}
	if (ymax <= ymin)
	{
		ymin = 0; ymax = bandwidths ? 1000: 8000;
	}
	for (long iformant = 1; iformant <= my formants -> size; iformant++)
	{
		wchar_t *quantity = NULL;
		bool garnish2 = false;
		RealTier tier = (structRealTier *)tiers -> item[iformant];
		if (iformant == my formants -> size)
		{
			quantity = L"Frequency (Hz)";
			if (garnish) garnish2 = true;
		} 
		RealTier_draw (tier, g, xmin, xmax, ymin, ymax, garnish2, method, quantity);
	}
}

FORM (old_FormantGrid_draw, L"FormantGrid: Draw", 0)
	REAL (L"left Time range (s)", L"0.0")
	REAL (L"right Time range (s)", L"0.0 (=all)")
	REAL (L"left Frequency range (Hz)", L"0.0")
	REAL (L"right Frequency range (Hz)", L"0.0 (=auto)")
	BOOLEAN (L"Bandwidths", false)
	BOOLEAN (L"Garnish", true)
	OK
DO
	EVERY_DRAW (FormantGrid_draw ((structFormantGrid *)OBJECT, GRAPHICS,
		GET_REAL (L"left Time range"), GET_REAL (L"right Time range"),
		GET_REAL (L"left Frequency range"), GET_REAL (L"right Frequency range"),
		GET_INTEGER (L"Bandwidths"), GET_INTEGER (L"Garnish"), L"lines and speckles"))
END

FORM (FormantGrid_draw, L"FormantGrid: Draw", 0)
	REAL (L"left Time range (s)", L"0.0")
	REAL (L"right Time range (s)", L"0.0 (=all)")
	REAL (L"left Frequency range (Hz)", L"0.0")
	REAL (L"right Frequency range (Hz)", L"0.0 (=auto)")
	BOOLEAN (L"Bandwidths", false)
	BOOLEAN (L"Garnish", true)
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"ui/editors/AmplitudeTierEditor.h")
	OPTIONMENU (L"Drawing method", 1)
		OPTION (L"lines")
		OPTION (L"speckles")
		OPTION (L"lines and speckles")
	OK
DO_ALTERNATIVE (old_FormantGrid_draw)
	EVERY_DRAW (FormantGrid_draw ((structFormantGrid *)OBJECT, GRAPHICS,
		GET_REAL (L"left Time range"), GET_REAL (L"right Time range"),
		GET_REAL (L"left Frequency range"), GET_REAL (L"right Frequency range"),
		GET_INTEGER (L"Bandwidths"), GET_INTEGER (L"Garnish"), GET_STRING (L"Drawing method")))
END

/****************** FunctionTerms  *********************************/

static void NUMdcvector_extrema_re (dcomplex v[], long lo, long hi,
	double *min, double *max)
{
	long i;
	*min = *max = v[lo].re;
	for (i=lo+1; i <= hi; i++)
	{
		if (v[i].re < *min) *min = v[i].re;
		else if (v[i].re > *max) *max = v[i].re;
	}
}

static void NUMdcvector_extrema_im (dcomplex v[], long lo, long hi,
	double *min, double *max)
{
	long i;
	*min = *max = v[lo].im;
	for (i=lo+1; i <= hi; i++)
	{
		if (v[i].im < *min) *min = v[i].im;
		else if (v[i].im > *max) *max = v[i].im;
	}
}

static void Graphics_polyline_clipTopBottom (Graphics g, double *x, double *y,
	long numberOfPoints, double ymin, double ymax)
{
	double xb, xe, yb, ye, x1, x2, y1, y2;
	long i, index = 0;
	
	if (numberOfPoints < 2) return;
	
	xb = x1 = x[0]; yb = y1 = y[0];
	for (i = 1; i < numberOfPoints; i++)
	{
		x2 = x[i]; y2 = y[i];
		
		if (! ((y1 > ymax && y2 > ymax) || (y1 < ymin && y2 < ymin)))
		{
			double dxy = (x2 - x1) / (y1 - y2);
			double xcros_max = x1 - (ymax - y1) * dxy;
			double xcros_min = x1 - (ymin - y1) * dxy;
			if (y1 > ymax && y2 < ymax)
			{
				/*
					Line enters from above: start new segment.
					Save start values.
				*/
				xb = x[i-1]; yb = y[i-1]; index = i - 1;
				y[i-1] = ymax; x[i-1] = xcros_max;
			}
			if (y1 > ymin && y2 < ymin)
			{
				/*
					Line leaves at bottom: draw segment.
					Save end values and restore them
					Origin of arrays for Graphics_polyline are at element 0 !!!
				*/
				xe = x[i]; ye = y[i];
				y[i] = ymin; x[i] = xcros_min;
				
				Graphics_polyline (g, i - index + 1, x + index, y + index);
				
				x[index] = xb; y[index] = yb; x[i] = xe; y[i] = ye;
			}
			if (y1 < ymin && y2 > ymin)
			{
				/*
					Line enters from below: start new segment.
					Save start values
				*/
				xb = x[i-1]; yb = y[i-1]; index = i - 1;
				y[i-1] = ymin; x[i-1] = xcros_min;
			}
			if (y1 < ymax && y2 > ymax)
			{
				/*
					Line leaves at top: draw segment.
					Save and restore
				*/
				xe = x[i]; ye = y[i];
				y[i] = ymax; x[i] =  xcros_max;

				Graphics_polyline (g, i - index + 1, x + index, y + index);
				
				x[index] = xb; y[index] = yb; x[i] = xe; y[i] = ye;
			}
		}
		else index = i;
		y1 = y2; x1 = x2;
	}
	if (index < numberOfPoints - 1)
	{
		Graphics_polyline (g, numberOfPoints - index, x + index, y + index);
		x[index] = xb; y[index] = yb;
	}
}

/*
	Extrapolate only for functions whose domain is extendable and that can be extrapolated.
	Polynomials can be extrapolated.
	LegendreSeries and ChebyshevSeries cannot be extrapolated.
*/
void FunctionTerms_draw (I, Graphics g, double xmin, double xmax, double ymin,
	double ymax, int extrapolate, int garnish)
{
	iam (FunctionTerms);
	double dx, fxmin, fxmax, x1, x2;
	double *x = NULL, *y = NULL;
	long i, numberOfPoints = 1000;
		
	if (((y = NUMdvector (1, numberOfPoints +1)) == NULL) ||
		((x = NUMdvector (1, numberOfPoints +1)) == NULL)) goto end;

	if (xmax <= xmin)
	{
		xmin = my xmin; xmax = my xmax;
	}
	
	fxmin = xmin; fxmax = xmax;
	if (! extrapolate)
	{
		if (xmax < my xmin || xmin > my xmax) return;
		if (xmin < my xmin) fxmin = my xmin;
		if (xmax > my xmax) fxmax = my xmax;
	}
	
	if (ymax <= ymin)
	{
		FunctionTerms_getExtrema (me, fxmin, fxmax, &x1, &ymin, &x2, &ymax);
	}
	
	Graphics_setInner (g);
	Graphics_setWindow (g, xmin, xmax, ymin, ymax);

	/*
		Draw only the parts within [fxmin, fxmax] X [ymin, ymax].
	*/
	
	dx = (fxmax - fxmin) / (numberOfPoints - 1);	
	for (i=1; i <= numberOfPoints; i++)
	{
		x[i] = fxmin + (i - 1.0) * dx;
		y[i] = FunctionTerms_evaluate (me, x[i]);
	}
	Graphics_polyline_clipTopBottom (g, x+1, y+1, numberOfPoints, ymin, ymax);	
	Graphics_unsetInner (g);
	
	if (garnish)
	{
		Graphics_drawInnerBox (g);
    	Graphics_marksBottom (g, 2, 1, 1, 0);
    	Graphics_marksLeft (g, 2, 1, 1, 0);
	}
end:
	NUMdvector_free (y, 1); NUMdvector_free (x, 1);
}

void FunctionTerms_drawBasisFunction (I, Graphics g, long index, double xmin,
	double xmax, double ymin, double ymax, int extrapolate, int garnish)
{
	iam (FunctionTerms);
	FunctionTerms thee;
	long i;
	
	if (index < 1 || index > my numberOfCoefficients) return;
	if ((thee = (structFunctionTerms *)Data_copy (me)) == NULL) return;
	
	for (i=1; i < index; i++) thy coefficients[i] = 0;
	for (i=index+1; i <= my numberOfCoefficients; i++) thy coefficients[i] = 0;
	thy coefficients[index] = 1;
	
	FunctionTerms_draw (thee, g, xmin, xmax, ymin, ymax, extrapolate, garnish);
	
	forget (thee);
}

FORM (FunctionTerms_draw, L"FunctionTerms: Draw", 0)
	REAL (L"Xmin", L"0.0")
	REAL (L"Xmax", L"0.0")
	REAL (L"left Vertical range", L"0.0")
	REAL (L"right Vertical range", L"0.0")
	BOOLEAN (L"Extrapolate", 0)
	BOOLEAN (L"Garnish", 1)
	OK
DO
	EVERY_DRAW (FunctionTerms_draw ((structFunctionTerms *)OBJECT, GRAPHICS, GET_REAL (L"Xmin"), GET_REAL (L"Xmax"),
		GET_REAL (L"left Vertical range"), GET_REAL (L"right Vertical range"),
		GET_INTEGER (L"Extrapolate"), GET_INTEGER (L"Garnish")))
END

FORM (FunctionTerms_drawBasisFunction, L"FunctionTerms: Draw basis function", 0)
	NATURAL (L"Index", L"1")
	REAL (L"Xmin", L"0.0")
	REAL (L"Xmax", L"0.0")
	REAL (L"left Vertical range", L"0.0")
	REAL (L"right Vertical range", L"0.0")
	BOOLEAN (L"Extrapolate", 0)
	BOOLEAN (L"Garnish", 1)
	OK
DO
	EVERY_DRAW (FunctionTerms_drawBasisFunction ((structFunctionTerms *)OBJECT, GRAPHICS,
		GET_INTEGER (L"Index"),
		GET_REAL (L"Xmin"), GET_REAL (L"Xmax"),
		GET_REAL (L"left Vertical range"), GET_REAL (L"right Vertical range"),
		GET_INTEGER (L"Extrapolate"), GET_INTEGER (L"Garnish")))
END

FORM (FunctionTerms_evaluate, L"FunctionTerms: Evaluate", 0)
	REAL (L"X", L"0.0")
	OK
DO
	FunctionTerms f = (structFunctionTerms *)ONLY_GENERIC (classFunctionTerms);
	Melder_information1 (Melder_double (FunctionTerms_evaluate (f, GET_REAL (L"X"))));
END

DIRECT (FunctionTerms_getNumberOfCoefficients)
	FunctionTerms f = (structFunctionTerms *)ONLY_GENERIC (classFunctionTerms);
	Melder_information1 (Melder_integer (f -> numberOfCoefficients));
END

FORM (FunctionTerms_getCoefficient, L"FunctionTerms: Get coefficient", 0)
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"p(x) = c[1] + c[2] x + ... c[n+1] x^n")
	NATURAL (L"Index", L"1")
	OK
DO
	long index = GET_INTEGER (L"Index");
	FunctionTerms f = (structFunctionTerms *)ONLY_GENERIC (classFunctionTerms);
	REQUIRE (index <= f -> numberOfCoefficients, L"Index too large.")
	Melder_information1 (Melder_double (f -> coefficients[index]));
END

DIRECT (FunctionTerms_getDegree)
	FunctionTerms f = (structFunctionTerms *)ONLY_GENERIC (classFunctionTerms);
	Melder_information1 (Melder_integer (FunctionTerms_getDegree (f)));
END

FORM (FunctionTerms_getMaximum, L"FunctionTerms: Get maximum", L"Polynomial: Get maximum...")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Interval")
	REAL (L"Xmin", L"0.0")
	REAL (L"Xmax", L"0.0")
	OK
DO
	FunctionTerms f = (structFunctionTerms *)ONLY_GENERIC (classFunctionTerms);
	double x = FunctionTerms_getMaximum (f, GET_REAL (L"Xmin"),
		GET_REAL (L"Xmax"));
	Melder_information1 (Melder_double (x));
END

FORM (FunctionTerms_getMinimum, L"FunctionTerms: Get minimum", L"Polynomial: Get minimum...")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Interval")
	REAL (L"Xmin", L"0.0")
	REAL (L"Xmax", L"0.0")
	OK
DO
	FunctionTerms f = (structFunctionTerms *)ONLY_GENERIC (classFunctionTerms);
	double x = FunctionTerms_getMinimum (f, GET_REAL (L"Xmin"),
		GET_REAL (L"Xmax"));
	Melder_information1 (Melder_double (x));
END

FORM (FunctionTerms_getXOfMaximum, L"FunctionTerms: Get x of maximum", L"Polynomial: Get x of maximum...")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Interval")
	REAL (L"Xmin", L"0.0")
	REAL (L"Xmax", L"0.0")
	OK
DO
	FunctionTerms f = (structFunctionTerms *)ONLY_GENERIC (classFunctionTerms);
	double x = FunctionTerms_getXOfMaximum (f, GET_REAL (L"Xmin"),
		GET_REAL (L"Xmax"));
	Melder_information1 (Melder_double (x));
END

FORM (FunctionTerms_getXOfMinimum, L"FunctionTerms: Get x of minimum", L"Polynomial: Get x of minimum...")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Interval")
	REAL (L"Xmin", L"0.0")
	REAL (L"Xmax", L"0.0")
	OK
DO
	FunctionTerms f = (structFunctionTerms *)ONLY_GENERIC (classFunctionTerms);
	double x = FunctionTerms_getXOfMinimum (f, GET_REAL (L"Xmin"),
		GET_REAL (L"Xmax"));
	Melder_information1 (Melder_double (x));
END

FORM (FunctionTerms_setCoefficient, L"FunctionTerms: Set coefficient", 0)
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"p(x) = c[1]F[0] + c[2]F[1] + ... c[n+1]F[n]")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"F[k] is of degree k")
	NATURAL (L"Index", L"1")
	REAL (L"Value", L"0.0")
	OK
DO
	FunctionTerms f = (structFunctionTerms *)ONLY_GENERIC (classFunctionTerms);
	if (! FunctionTerms_setCoefficient (f, GET_INTEGER (L"Index"),
		GET_REAL (L"Value"))) return 0;
END

FORM (FunctionTerms_setDomain, L"FunctionTerms: Set domain", 0)
	REAL (L"Xmin", L"0.0")
	REAL (L"Xmax", L"2.0")
	OK
DO
	FunctionTerms f = (structFunctionTerms *)ONLY_GENERIC (classFunctionTerms);
	double xmin = GET_REAL (L"Xmin"), xmax = GET_REAL (L"Xmax");
	REQUIRE (xmin < xmax, L"Xmax must be larger than Xmin.")
	FunctionTerms_setDomain (f, xmin, xmax);
END

/***************** Intensity ***************************************************/

FORM (Intensity_to_TextGrid_detectSilences, L"Intensity: To TextGrid (silences)", L"Intensity: To TextGrid (silences)...")
	REAL (L"Silence threshold (dB)", L"-25.0")
	POSITIVE (L"Minimum silent interval duration (s)", L"0.1")
	POSITIVE (L"Minimum sounding interval duration (s)", L"0.1")
	WORD (L"Silent interval label", L"silent")
	WORD (L"Sounding interval label", L"sounding")
	OK
DO
	EVERY_TO (Intensity_to_TextGrid_detectSilences ((structIntensity *)OBJECT, GET_REAL (L"Silence threshold"),
		GET_REAL (L"Minimum silent interval duration"), GET_REAL (L"Minimum sounding interval duration"),
		GET_STRING (L"Silent interval label"), GET_STRING (L"Sounding interval label")))
END

/***************** ISpline ***************************************************/

DIRECT (ISpline_help) Melder_help (L"ISpline"); END

FORM (ISpline_create, L"Create ISpline", L"Create ISpline...")
	WORD (L"Name", L"ispline")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Domain")
	REAL (L"Xmin", L"0")
	REAL (L"Xmax", L"1")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"ISpline(x) = c[1] I[1](x) + c[2] I[1](x) + ... c[n] I[n](x)")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"all I[k] are polynomials of degree \"Degree\"ui/editors/AmplitudeTierEditor.h")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Relation: numberOfCoefficients == numberOfInteriorKnots + degree")
	INTEGER (L"Degree", L"3")
	SENTENCE (L"Coefficients (c[k])", L"1.2 2.0 1.2 1.2 3.0 0.0")
	SENTENCE (L"Interior knots" , L"0.3 0.5 0.6")
	OK
DO
	double xmin = GET_REAL (L"Xmin"), xmax = GET_REAL (L"Xmax");
	long degree = GET_INTEGER (L"Degree");
	REQUIRE (xmin < xmax, L"Xmin must be smaller than Xmax.")
	if (! praat_new1 (ISpline_createFromStrings (xmin, xmax, degree,
		GET_STRING (L"Coefficients"), GET_STRING (L"Interior knots")),
		GET_STRING (L"Name"))) return 0;
END

/******************* KlattTable  *********************************/

DIRECT (KlattTable_help) Melder_help (L"KlattTable"); END

DIRECT (KlattTable_createExample)
	if (! praat_new1 (KlattTable_createExample (), L"example")) return 0;
END

FORM (KlattTable_to_Sound, L"KlattTable: To Sound", L"KlattTable: To Sound...")
	POSITIVE (L"Sampling frequency", L"16000")
	RADIO (L"Synthesis model", 1)
	RADIOBUTTON (L"Cascade")
	RADIOBUTTON (L"Parallel")
	NATURAL (L"Number of formants", L"5")
	POSITIVE (L"Frame duration (s)", L"0.005")
	REAL (L"Flutter percentage (%)", L"0.0")
	OPTIONMENU (L"Voicing source", 1)
	OPTION (L"Impulsive")
	OPTION (L"Natural")
	OPTIONMENU (L"Output type", 1)
	OPTION (L"Sound")
	OPTION (L"Voicing")
	OPTION (L"Aspiration")
	OPTION (L"Frication")
	OPTION (L"Cascade-glottal-output")
	OPTION (L"Parallel-glottal-output")
	OPTION (L"Bypass-output")
	OPTION (L"All-excitations")
	OK
DO
	double flutter = GET_REAL (L"Flutter percentage");
	int outputType = GET_INTEGER (L"Output type") - 1;
	REQUIRE (flutter >= 0 && flutter <= 100, L"Flutter must be between 0 and 100%")
	EVERY_TO (KlattTable_to_Sound ((structKlattTable *)OBJECT, GET_REAL (L"Sampling frequency"), GET_INTEGER (L"Synthesis model"),
		GET_INTEGER (L"Number of formants"), GET_REAL (L"Frame duration"), GET_INTEGER (L"Voicing source"),
		GET_REAL (L"Flutter percentage"), outputType))
END

FORM (KlattTable_to_KlattGrid, L"KlattTable: To KlattGrid", 0)
	POSITIVE (L"Frame duration (s)", L"0.002")
	OK
DO
	EVERY_TO (KlattTable_to_KlattGrid ((structKlattTable *)OBJECT, GET_REAL (L"Frame duration")))
END

DIRECT (KlattTable_to_Table)
	EVERY_TO (KlattTable_to_Table ((structKlattTable *)OBJECT))
END

DIRECT (Table_to_KlattTable)
	EVERY_TO (Table_to_KlattTable ((structTable *)OBJECT))
END

/******************* LegendreSeries *********************************/

FORM (LegendreSeries_create, L"Create LegendreSeries", L"Create LegendreSeries...")
	WORD (L"Name", L"ls")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Domain")
	REAL (L"Xmin", L"-1")
	REAL (L"Xmax", L"1")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"LegendreSeries(x) = c[1] P[0](x) + c[2] P[1](x) + ... c[n+1] P[n](x)")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"P[k] is a Legendre polynomial of degree k")
	SENTENCE (L"Coefficients", L"0 0 1.0")
	OK
DO
	double xmin = GET_REAL (L"Xmin"), xmax = GET_REAL (L"Xmax");
	REQUIRE (xmin < xmax, L"Xmin must be smaller than Xmax.")
	if (! praat_new1 (LegendreSeries_createFromString (xmin, xmax,
		GET_STRING (L"Coefficients")), GET_STRING (L"Name"))) return 0;
END

DIRECT (LegendreSeries_help) Melder_help (L"LegendreSeries"); END

DIRECT (LegendreSeries_to_Polynomial)
	EVERY_TO (LegendreSeries_to_Polynomial ((structLegendreSeries *)OBJECT))
END
/********************* LongSound **************************************/

FORM_READ (LongSounds_appendToExistingSoundFile, L"LongSound: Append to existing sound file", 0, false)
	if (! pr_LongSounds_appendToExistingSoundFile (file)) return 0;
END

FORM_WRITE (LongSounds_writeToStereoAiffFile, L"LongSound: Save as AIFF file", 0, L"aiff")
	if (! pr_LongSounds_writeToStereoAudioFile (file, Melder_AIFF)) return 0;
END

FORM_WRITE (LongSounds_writeToStereoAifcFile, L"LongSound: Save as AIFC file", 0, L"aifc")
	if (! pr_LongSounds_writeToStereoAudioFile (file, Melder_AIFC)) return 0;
END

FORM_WRITE (LongSounds_writeToStereoWavFile, L"LongSound: Save as WAV file", 0, L"wav")
	if (! pr_LongSounds_writeToStereoAudioFile (file, Melder_WAV)) return 0;
END

FORM_WRITE (LongSounds_writeToStereoNextSunFile, L"LongSound: Save as NeXT/Sun file", 0, L"au")
	if (! pr_LongSounds_writeToStereoAudioFile (file, Melder_NEXT_SUN)) return 0;
END

FORM_WRITE (LongSounds_writeToStereoNistFile, L"LongSound: Save as NIST file", 0, L"nist")
	if (! pr_LongSounds_writeToStereoAudioFile (file, Melder_NIST)) return 0;
END

/******************* Matrix **************************************************/

/* Draw a Matrix as small squares whose area correspond to the matrix element */
/* The square is filled with black if the weights are negative					*/
void Matrix_drawAsSquares (I, Any g, double xmin, double xmax, double ymin, double ymax, int garnish)
{
    iam (Matrix);
	Graphics_Colour colour = Graphics_inqColour (g);
    long i, j, ixmin, ixmax, iymin, iymax, nx, ny;
    double dx, dy, min, max, wAbsMax;

	if (xmax <= xmin) { xmin = my xmin; xmax = my xmax; }
	nx = Matrix_getWindowSamplesX (me, xmin, xmax, &ixmin, &ixmax);
	if (ymax <= ymin) { ymin = my ymin; ymax = my ymax; }
	ny = Matrix_getWindowSamplesY (me, ymin, ymax, &iymin, &iymax);
	max = nx > ny ? nx : ny;
	dx = (xmax - xmin) / max; dy = (ymax - ymin) / max;
    Graphics_setInner (g);
    Graphics_setWindow (g, xmin, xmax, ymin, ymax);
    Matrix_getWindowExtrema (me, ixmin, ixmax, iymin, iymax, & min, & max);
    wAbsMax = fabs (max) > fabs (min) ? fabs (max) : fabs (min);
    for (i = iymin; i <= iymax; i++)
    {
		double y = Matrix_rowToY (me, i);
		for (j = ixmin; j <= ixmax; j++)
		{
			double x = Matrix_columnToX (me, j);
	    	double d = 0.95 * sqrt (fabs (my z[i][j]) / wAbsMax);
	    	double x1WC = x - d * dx / 2, x2WC = x + d * dx / 2;
	    	double y1WC = y - d * dy / 2, y2WC = y + d * dy / 2;
			if (my z[i][j] > 0) Graphics_setColour (g, Graphics_WHITE);
	    	Graphics_fillRectangle (g, x1WC, x2WC, y1WC, y2WC);
	    	Graphics_setColour (g, colour);
	    	Graphics_rectangle (g, x1WC, x2WC , y1WC, y2WC);
		}
    }
    Graphics_setGrey (g, 0.0);
    Graphics_unsetInner (g);
    if (garnish)
    {
		Graphics_drawInnerBox (g);
		Graphics_marksLeft (g, 2, 1, 1, 0);
		if (ymin * ymax < 0.0) Graphics_markLeft (g, 0.0, 1, 1, 1, NULL);
		Graphics_marksBottom (g, 2, 1, 1, 0);
		if (xmin * xmax < 0.0) Graphics_markBottom (g, 0.0, 1, 1, 1, NULL);
    }
}

void Matrix_drawDistribution (I, Graphics g, double xmin, double xmax,
	double ymin, double ymax, double minimum, double maximum, long nBins,
	double freqMin, double freqMax, int cumulative, int garnish)
{
	iam (Matrix);
	long i, j, ixmin, ixmax, iymin, iymax, nxy = 0, *freq = NULL;
	double binWidth, fi = 0;

	if (nBins <= 0) return;
	if (xmax <= xmin)
	{
		xmin = my xmin; xmax = my xmax;
	}
	if (ymax <= ymin)
	{
		ymin = my ymin; ymax = my ymax;
	}
	if (Matrix_getWindowSamplesX (me, xmin, xmax, & ixmin, & ixmax) == 0 ||
		Matrix_getWindowSamplesY (me, ymin, ymax, & iymin, & iymax) == 0)
			return;
	if (maximum <= minimum) Matrix_getWindowExtrema (me, ixmin, ixmax, iymin,
		iymax, & minimum, & maximum);
	if (maximum <= minimum)
	{
		minimum -= 1.0; maximum += 1.0;
	}

	/*
		Count the numbers per bin and the total
	*/

	if (nBins < 1) nBins = 10;
	if ((freq = NUMlvector (1, nBins)) == NULL) return;
	binWidth = (maximum - minimum) / nBins;
	for (i = iymin; i <= iymax; i++)
	{
		for (j = ixmin; j <= ixmax; j++)
		{
			long bin = 1 + floor ((my z[i][j] - minimum) / binWidth);
			if (bin <= nBins && bin > 0)
			{
				freq[bin]++; nxy ++;
			}
		}
	}

	if (freqMax <= freqMin)
	{
		if (cumulative)
		{
			freqMin = 0; freqMax = 1.0;
		}
		else
		{
			NUMlvector_extrema (freq, 1, nBins, & freqMin, & freqMax);
			if (freqMax <= freqMin)
			{
				freqMin = freqMin > 1 ? freqMin-1: 0;
				freqMax += 1.0;
			}
		}
	}

	Graphics_setInner (g);
	Graphics_setWindow (g, minimum, maximum, freqMin, freqMax);

	for (i = 1; i <= nBins; i++)
	{
		double ftmp = freq[i];
		fi = cumulative ? fi + freq[i] / nxy : freq[i];
		ftmp = fi;
		if (ftmp > freqMax) ftmp = freqMax;
		if (ftmp > freqMin) Graphics_rectangle (g, minimum + (i-1) *
			binWidth, minimum + i * binWidth, freqMin, ftmp);
	}
    Graphics_unsetInner (g);
    if (garnish)
    {
    	Graphics_drawInnerBox (g);
    	Graphics_marksBottom (g, 2, 1, 1, 0);
    	Graphics_marksLeft (g, 2, 1, 1, 0);
    	if (! cumulative) Graphics_textLeft (g, 1, L"Number/bin");
    }
	NUMlvector_free (freq, 1);
}

void Matrix_drawSliceY (I, Graphics g, double x, double ymin, double ymax,
	double min, double max)
{
	iam (Matrix);
	long i, ix, iymin, iymax, ny;
	double *y;

	if (x < my xmin || x > my xmax) return;
	ix = Matrix_xToNearestColumn (me, x);

	if (ymax <= ymin)
	{
		ymin = my ymin;
		ymax = my ymax;
	}

	ny = Matrix_getWindowSamplesY (me, ymin, ymax, &iymin, &iymax);
	if (ny < 1) return;

	if (max <= min)
	{
		Matrix_getWindowExtrema (me, ix, ix, iymin, iymax, &min, &max);
	}
	if (max <= min)
	{
		min -= 0.5; max += 0.5;
	}
	y = NUMdvector (iymin, iymax);
	if (y == NULL) return;

	Graphics_setWindow (g, ymin, ymax, min, max);
	Graphics_setInner (g);

	for (i = iymin; i <= iymax; i++)
	{
		y[i] = my z[i][ix];
	}
	Graphics_function (g, y, iymin, iymax, Matrix_rowToY (me, iymin),
		 Matrix_rowToY (me, iymax));
	Graphics_unsetInner (g);
	NUMdvector_free (y, 1);
}

FORM (Matrix_drawAsSquares,L"Matrix: Draw as squares", L"Matrix: Draw as squares...")
    REAL (L"left Horizontal range", L"0.0")
    REAL (L"right Horizontal range", L"0.0")
    REAL (L"left Vertical range", L"0.0")
    REAL (L"right Vertical range", L"0.0")
    BOOLEAN (L"Garnish", 1)
	OK
DO
	EVERY_DRAW (Matrix_drawAsSquares ((structMatrix *)OBJECT, GRAPHICS,
    	GET_REAL (L"left Horizontal range"), GET_REAL (L"right Horizontal range"),
    	GET_REAL (L"left Vertical range"), GET_REAL (L"right Vertical range"),
		GET_INTEGER (L"Garnish")))
END

FORM (Matrix_drawDistribution, L"Matrix: Draw distribution", L"Matrix: Draw distribution...")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Selection of (part of) Matrix")
    REAL (L"left Horizontal range", L"0.0")
    REAL (L"right Horizontal range", L"0.0")
    REAL (L"left Vertical range", L"0.0")
    REAL (L"right Vertical range", L"0.0")
    LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Selection of Matrix values")
    REAL (L"Minimum value", L"0.0")
    REAL (L"Maximum value", L"0.0")
    LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Display of the distribution")
    NATURAL (L"Number of bins", L"10")
    REAL (L"Minimum frequency", L"0.0")
    REAL (L"Maximum frequency", L"0.0")
    BOOLEAN (L"Garnish", 1)
	OK
DO
	EVERY_DRAW (Matrix_drawDistribution ((structMatrix *)OBJECT, GRAPHICS,
    	GET_REAL (L"left Horizontal range"), GET_REAL (L"right Horizontal range"),
    	GET_REAL (L"left Vertical range"), GET_REAL (L"right Vertical range"),
    	GET_REAL (L"Minimum value"), GET_REAL (L"Maximum value"),
    	GET_INTEGER (L"Number of bins"),
    	GET_REAL (L"Minimum frequency"), GET_REAL (L"Maximum frequency"), 0,
		GET_INTEGER (L"Garnish")))
END

FORM (Matrix_drawCumulativeDistribution, L"Matrix: Draw cumulative distribution", L"ui/editors/AmplitudeTierEditor.h")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Selection of (part of) Matrix")
    REAL (L"left Horizontal range", L"0.0")
    REAL (L"right Horizontal range", L"0.0")
    REAL (L"left Vertical range", L"0.0")
    REAL (L"right Vertical range", L"0.0")
    LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Selection of Matrix values")
    REAL (L"Minimum value", L"0.0")
    REAL (L"Maximum value", L"0.0")
    LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Display of the distribution")
    NATURAL (L"Number of bins", L"10")
    REAL (L"Minimum", L"0.0")
    REAL (L"Maximum", L"0.0")
    BOOLEAN (L"Garnish", 1)
	OK
DO
	EVERY_DRAW (Matrix_drawDistribution ((structMatrix *)OBJECT, GRAPHICS,
    	GET_REAL (L"left Horizontal range"), GET_REAL (L"right Horizontal range"),
    	GET_REAL (L"left Vertical range"), GET_REAL (L"right Vertical range"),
    	GET_REAL (L"Minimum value"), GET_REAL (L"Maximum value"),
    	GET_INTEGER (L"Number of bins"),
    	GET_REAL (L"Minimum"), GET_REAL (L"Maximum"), 1,
		GET_INTEGER (L"Garnish")))
END

FORM (Matrix_scale, L"Matrix: Scale", 0)
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"self[row, col] := self[row, col] / `Scale factor'")
	RADIO (L"Scale factor", 1)
	RADIOBUTTON (L"Extremum in matrix")
	RADIOBUTTON (L"Extremum in each row")
	RADIOBUTTON (L"Extremum in each column")
	OK
DO
	int scale = GET_INTEGER (L"Scale factor");
	REQUIRE (scale > 0 && scale < 4, L"Illegal value for scale.")
	EVERY (Matrix_scale ((structMatrix *)OBJECT, scale))
END

DIRECT (Matrix_transpose)
	EVERY_TO (Matrix_transpose ((structMatrix *)OBJECT))
END

FORM (Matrix_solveEquation, L"Matrix: Solve equation", L"Matrix: Solve equation...")
	REAL (L"Tolerance", L"1.19e-7")
	OK
DO
	WHERE (SELECTED)
	{
		if (! praat_new2 (Matrix_solveEquation ((structMatrix *)OBJECT, GET_REAL (L"Tolerance")), NAME, L"_solution")) return 0;
	}
END

DIRECT (Matrix_Categories_to_TableOfReal)
	NEW (Matrix_and_Categories_to_TableOfReal ((structMatrix *)ONLY_GENERIC (classMatrix),
		(structCategories *)ONLY (classCategories)))
END



FORM (Matrix_scatterPlot, L"Matrix: Scatter plot", 0)
    NATURAL (L"Column for X-axis", L"1")
    NATURAL (L"Column for Y-axis", L"2")
    REAL (L"left Horizontal range", L"0.0")
    REAL (L"right Horizontal range", L"0.0")
    REAL (L"left Vertical range", L"0.0")
    REAL (L"right Vertical range", L"0.0")
	POSITIVE (L"Mark size (mm)", L"1.0")
	SENTENCE (L"Mark string (+xo.)", L"+")
    BOOLEAN (L"Garnish", 1)
	OK
DO
    long x = GET_INTEGER (L"Column for X-axis");
	long y = GET_INTEGER (L"Column for Y-axis");
    REQUIRE (x != 0 && y != 0, L"X and Y component must differ from 0.")
    EVERY_DRAW (Matrix_scatterPlot ((structMatrix *)OBJECT, GRAPHICS, x, y,
    	GET_REAL (L"left Horizontal range"), GET_REAL (L"right Horizontal range"),
    	GET_REAL (L"left Vertical range"), GET_REAL (L"right Vertical range"),
    	GET_REAL (L"Mark size"), GET_STRING (L"Mark string"),
		GET_INTEGER (L"Garnish")))
END

DIRECT (Matrix_to_Activation)
	EVERY_TO (Matrix_to_Activation ((structMatrix *)OBJECT))
END

FORM (Matrices_to_DTW, L"Matrices: To DTW", L"Matrix: To DTW...")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Distance  between cepstral coefficients")
	REAL (L"Distance metric", L"2.0")
	DTW_constraints_addCommonFields (dia);
	OK
DO
	Matrix m1 = NULL, m2 = NULL;
	int begin, end, slope;
	DTW_constraints_getCommonFields (dia, &begin, &end, &slope);
	WHERE (SELECTED && Thing_member (OBJECT, classMatrix))
	{
		if (m1) m2 = (structMatrix *)OBJECT; else m1 = (structMatrix *)OBJECT;
	}
	NEW (Matrices_to_DTW (m1, m2, begin, end, slope, GET_REAL (L"Distance metric")))

END

FORM (Matrix_to_Pattern, L"Matrix: To Pattern", 0)
	NATURAL (L"Join", L"1")
	OK
DO
	EVERY_TO (Matrix_to_Pattern ((structMatrix *)OBJECT, GET_INTEGER (L"Join")))
END

/***** MATRIXFT *************/

DIRECT (Matrixft_getHighestFrequency)
	Matrix me = (structMatrix *)ONLY_OBJECT;
	Melder_information1 (Melder_double (my ymax));
END

DIRECT (Matrixft_getLowestFrequency)
	Matrix me = (structMatrix *)ONLY_OBJECT;
	Melder_information1 (Melder_double (my ymin));
END

DIRECT (Matrixft_getNumberOfFrequencies)
	Matrix me = (structMatrix *)ONLY_OBJECT;
	Melder_information1 (Melder_integer (my ny));
END

DIRECT (Matrixft_getFrequencyDistance)
	Matrix me = (structMatrix *)ONLY_OBJECT;
	Melder_information1 (Melder_double (my dy));
END

FORM (Matrixft_getFrequencyOfRow, L"Get frequency of row", 0)
	NATURAL (L"Row number", L"1")
	OK
DO
	Melder_information1 (Melder_double (Matrix_rowToY ((structMatrix *)ONLY_OBJECT, GET_INTEGER (L"Row number"))));
END

FORM (Matrixft_getXofColumn, L"Get time of column", 0)
	NATURAL (L"Column number", L"1")
	OK
DO
	Melder_information1 (Melder_double (Matrix_columnToX ((structMatrix *)ONLY_OBJECT, GET_INTEGER (L"Column number"))));
END

FORM (Matrixft_getValueInCell, L"Get value in cell", 0)
	POSITIVE (L"Time (s)", L"0.5")
	POSITIVE (L"Frequency", L"1")
	OK
DO
	Matrix me = (structMatrix *)ONLY_OBJECT;
	long row, col;
	double ta, t = GET_REAL (L"Time");
	double fa, f = GET_REAL (L"Frequency");
	REQUIRE (f>= my xmin && f <= my ymax, L"Frequency out of range.")
	REQUIRE (t>= my xmin && t <= my xmax, L"Time out of range.")
	col = Matrix_xToNearestColumn (me, t);
	if (col < 1) col = 1;
	if (col > my nx) col = my nx;
	row = Matrix_yToNearestRow (me, f);
	if (row < 1) row = 1;
	if (row > my ny) row = my ny;
	ta = Matrix_columnToX (me, col);
	fa = Matrix_rowToY (me, row);
	Melder_information6 (Melder_single (my z[row][col]), L" (delta t: ", Melder_double (ta - t), L" f: ",
		Melder_double (fa - f), L")");
END

/**************** MelFilter *******************************************/

void MelFilter_drawFilterFunctions (MelFilter me, Graphics g,
	int toFreqScale, int fromFilter, int toFilter, double zmin, double zmax, 
	int dbScale, double ymin, double ymax, int garnish)
{
	long i, j, n = 1000; 
	double *a = NULL;

	if (! checkLimits (me, FilterBank_MEL, toFreqScale, & fromFilter, & toFilter, 
		& zmin, & zmax, dbScale, & ymin, & ymax)) return;
		
	a = NUMdvector (1, n);
	if (a == NULL) return;
	
	Graphics_setInner (g);
	Graphics_setWindow (g, zmin, zmax, ymin, ymax);

	for (j = fromFilter; j <= toFilter; j++)
	{
		double df = (zmax - zmin) / (n - 1); 
		double fc_mel = my y1 + (j - 1) * my dy;
		double fc_hz = MELTOHZ (fc_mel);
		double fl_hz = MELTOHZ (fc_mel - my dy);
		double fh_hz = MELTOHZ (fc_mel + my dy);
		long ibegin, iend;
		
		for (i = 1; i <= n; i++)
		{
			double z, f = zmin + (i - 1) * df;
			
			/* Filterfunction: triangular on a linear frequency scale
				AND a linear amplitude scale.
			*/
			z = scaleFrequency (f, toFreqScale, FilterBank_HERTZ);
			if (z == NUMundefined)
			{
				a[i] = NUMundefined;
			}
			else
			{
				a[i] = NUMtriangularfilter_amplitude (fl_hz, fc_hz, fh_hz, z);
				if (dbScale) a[i] = to_dB (a[i], 10, ymin);
			}
			
		}
				
		setDrawingLimits (a, n, ymin, ymax,	&ibegin, &iend);
		
		if (ibegin <= iend)
		{
			double fmin = zmin + (ibegin - 1) * df;
			double fmax = zmax - (n - iend) * df;
			Graphics_function (g, a, ibegin, iend, fmin, fmax);
		}
	}

	Graphics_unsetInner (g);
	
	if (garnish)
	{
		double distance = dbScale ? 10 : 1;
		const wchar_t *ytext = dbScale ? L"Amplitude (dB)" : L"Amplitude";
		Graphics_drawInnerBox (g);
    	Graphics_marksBottom (g, 2, 1, 1, 0);
    	Graphics_marksLeftEvery (g, 1, distance, 1, 1, 0);
    	Graphics_textLeft (g, 1, ytext);
    	Graphics_textBottom (g, 1, GetFreqScaleText (toFreqScale));
	}
	
	NUMdvector_free (a, 1);
}

/*
void MelFilter_drawFilters (MelFilter me, Graphics g, long from, long to,
	double fmin, double fmax, double ymin, double ymax, int dbscale, 
	int garnish)
{
	long i;
	double df = my dy;

	if (fmin >= fmax)
	{
		fmin = my ymin;
		fmax = my ymax;
	}
	if (from >= to)
	{
		from = 1;
		to = my ny;
	}
	Graphics_setWindow (g, my ymin, my ymax, 0, 1);
	Graphics_setInner (g);	
	for (i = from; i <= to; i++)
	{
		double fc = my y1 + (i - 1) * df;
		double fc_hz = MELTOHZ (fc);
		double fl_hz = MELTOHZ (fc - df);
		double fh_hz = MELTOHZ (fc + df);
		*//*
			Draw triangle
		*//*
		Graphics_line (g, fl_hz, 0, fc_hz, 1);
		Graphics_line (g, fc_hz, 1, fh_hz, 0); 
	}
	Graphics_unsetInner (g);
}*/

DIRECT (MelFilter_help)
	Melder_help (L"MelFilter");
END

FORM (MelFilter_drawFilterFunctions, L"MelFilter: Draw filter functions", L"FilterBank: Draw filter functions...")
	INTEGER (L"left Filter range", L"0")
	INTEGER (L"right Filter range", L"0")
	RADIO (L"Frequency scale", 1)
	RADIOBUTTON (L"Hertz")
	RADIOBUTTON (L"Bark")
	RADIOBUTTON (L"Mel")
	REAL (L"left Frequency range", L"0.0")
	REAL (L"right Frequency range", L"0.0")
	BOOLEAN (L"Amplitude scale in dB", 0)
	REAL (L"left Amplitude range", L"0.0")
	REAL (L"right Amplitude range", L"0.0")
	BOOLEAN (L"Garnish", 1)
	OK
DO
	EVERY_DRAW (MelFilter_drawFilterFunctions ((structMelFilter *)OBJECT, GRAPHICS,
		GET_INTEGER (L"Frequency scale"),
		GET_INTEGER (L"left Filter range"), GET_INTEGER (L"right Filter range"),
		GET_REAL (L"left Frequency range"), GET_REAL (L"right Frequency range"),
		GET_INTEGER (L"Amplitude scale in dB"),
		GET_REAL (L"left Amplitude range"), GET_REAL (L"right Amplitude range"),
		GET_INTEGER (L"Garnish")))
END

FORM (MelFilter_drawSpectrum, L"MelFilter: Draw spectrum (slice)", L"FilterBank: Draw spectrum (slice)...")
	POSITIVE (L"Time (s)", L"0.1")
	REAL (L"left Frequency range (mel)", L"0.0")
	REAL (L"right Frequency range (mel)", L"0.0")
	REAL (L"left Amplitude range (dB)", L"0.0")
	REAL (L"right Amplitude range (dB)", L"0.0")
	BOOLEAN (L"Garnish", 1)
	OK
DO
	EVERY_DRAW (FilterBank_drawTimeSlice ((structFilterBank *)OBJECT, GRAPHICS,
		GET_REAL (L"Time"), GET_REAL (L"left Frequency range"),
		GET_REAL (L"right Frequency range"), GET_REAL (L"left Amplitude range"),
		GET_REAL (L"right Amplitude range"), L"Mels", GET_INTEGER (L"Garnish")))
END

FORM (MelFilter_to_MFCC, L"MelFilter: To MFCC", L"MelFilter: To MFCC...")
	NATURAL (L"Number of coefficients", L"12")
	OK
DO
	EVERY_TO (MelFilter_to_MFCC ((structMelFilter *)OBJECT,
		GET_INTEGER (L"Number of coefficients")))
END

/**************** MFCC *******************************************/

DIRECT (MFCC_help)
	Melder_help (L"MFCC");
END

FORM (MFCC_to_MelFilter, L"MFCC: To MelFilter", L"MFCC: To MelFilter...")
	INTEGER (L"From coefficient", L"0")
	INTEGER (L"To coefficient", L"0")
	POSITIVE (L"Position of first filter (mel)", L"100.0")
	POSITIVE (L"Distance between filters (mel)", L"100.0")
	OK
DO
	EVERY_TO (MFCC_to_MelFilter ((structMFCC *)OBJECT, GET_INTEGER (L"From coefficient"),
		GET_INTEGER (L"To coefficient"), GET_REAL (L"Position of first filter"),
		GET_REAL (L"Distance between filters")))
END

/**************** MSpline *******************************************/

FORM (MSpline_create, L"Create MSpline", L"Create MSpline...")
	WORD (L"Name", L"mspline")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Domain")
	REAL (L"Xmin", L"0")
	REAL (L"Xmax", L"1")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"MSpline(x) = c[1] M[1](x) + c[2] M[1](x) + ... c[n] M[n](x)")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"all M[k] are polynomials of degree \"Degree\"ui/editors/AmplitudeTierEditor.h")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Relation: numberOfCoefficients == numberOfInteriorKnots + degree + 1")
	INTEGER (L"Degree", L"2")
	SENTENCE (L"Coefficients (c[k])", L"1.2 2.0 1.2 1.2 3.0 0.0")
	SENTENCE (L"Interior knots" , L"0.3 0.5 0.6")
	OK
DO
	double xmin = GET_REAL (L"Xmin"), xmax = GET_REAL (L"Xmax");
	long degree = GET_INTEGER (L"Degree");
	REQUIRE (xmin < xmax, L"Xmin must be smaller than Xmax.")
	if (! praat_new1 (MSpline_createFromStrings (xmin, xmax, degree,
		GET_STRING (L"Coefficients"), GET_STRING (L"Interior knots")),
		GET_STRING (L"Name"))) return 0;
END

DIRECT (MSpline_help) Melder_help (L"MSpline"); END

/********************** Pattern *******************************************/

void Pattern_draw (I, Graphics g, long pattern, double xmin, double xmax,
	double ymin, double ymax, int garnish)
{
	iam (Pattern);
	Matrix_drawRows (me, g, xmin, xmax, pattern - 0.5, pattern + 0.5, ymin, ymax);
	if (garnish)
	{
		Graphics_drawInnerBox (g);
    	Graphics_marksBottom (g, 2, 1, 1, 0);
    	Graphics_marksLeft (g, 2, 1, 1, 0);		
	}
}

DIRECT (Pattern_and_Categories_to_Discriminant)
	Pattern p = (structPattern *)ONLY (classPattern);
	Categories c = (structCategories *)ONLY (classCategories);
	if (! praat_new3 (Pattern_and_Categories_to_Discriminant (p, c),
		Thing_getName (p), L"_", Thing_getName (c))) return 0;
END

FORM (Pattern_draw, L"Pattern: Draw", 0)
	NATURAL (L"Pattern number", L"1")
	REAL (L"left Horizontal range", L"0.0")
	REAL (L"right Horizontal range", L"0.0")
	REAL (L"left Vertical range", L"0.0")
	REAL (L"right Vertical range", L"0.0")
	BOOLEAN (L"Garnish", 1)
	OK
DO
	EVERY_DRAW (Pattern_draw ((structPattern *)OBJECT, GRAPHICS, GET_INTEGER (L"Pattern number"),
		GET_REAL (L"left Horizontal range"), GET_REAL (L"right Horizontal range"),
		GET_REAL (L"left Vertical range"), GET_REAL (L"right Vertical range"), GET_INTEGER (L"Garnish")))
END

FORM (Pattern_formula, L"Pattern: Formula", 0)
	LABEL (L"label", L"        y := 1; for row := 1 to nrow do { x := 1; "
		"for col := 1 to ncol do { self [row, col] := `formula' ; x := x + 1 } "
		"y := y + 1 }}")
	TEXTFIELD (L"formula", L"self")
	OK
DO
	if (! praat_Fon_formula (dia, interpreter)) return 0;
END

FORM (Pattern_setValue, L"Pattern: Set value", L"Pattern: Set value...")
	NATURAL (L"Row number", L"1")
	NATURAL (L"Column number", L"1")
	REAL (L"New value", L"0.0")
	OK
DO
	WHERE (SELECTED) {
		Pattern me = (structPattern *)OBJECT;
		long row = GET_INTEGER (L"Row number"), column = GET_INTEGER (L"Column number");
		REQUIRE (row <= my ny, L"Row number must not be greater than number of rows.")
		REQUIRE (column <= my nx, L"Column number must not be greater than number of columns.")
		my z [row] [column] = GET_REAL (L"New value");
		praat_dataChanged (me);
	}
END

DIRECT (Pattern_to_Matrix)
	NEW (Pattern_to_Matrix ((structPattern *)ONLY (classPattern)))
END

/******************* PCA ******************************/

DIRECT (PCA_help)
	Melder_help (L"PCA");
END

DIRECT (hint_PCA_and_TableOfReal_to_Configuration)
	Melder_information1 (L"You can get principal components by selecting a PCA and a TableOfReal\n"
		"together and choosing \"To Configuration...\".");
END

DIRECT (hint_PCA_and_Covariance_Project)
	Melder_information1 (L"You can get a new Covariance object rotated to the directions of the direction vectors\n"
		" in the PCA object by selecting a PCA and a Covariance object together.");
END

DIRECT (hint_PCA_and_Configuration_to_TableOfReal_reconstruct)
	Melder_information1 (L"You can reconstruct the original TableOfReal as well as possible from\n"
		" the principal components in the Configuration and the direction vectors in the PCA object.");
END

FORM (PCA_and_TableOfReal_getFractionVariance,L"PCA & TableOfReal: Get fraction variance", L"PCA & TableOfReal: Get fraction variance...")
	NATURAL (L"left Principal component range", L"1")
	NATURAL (L"right Principal component range", L"1")
	OK
DO
	Melder_information1 (Melder_double (PCA_and_TableOfReal_getFractionVariance
		((structPCA *)ONLY (classPCA), ONLY_GENERIC (classTableOfReal),
		GET_INTEGER (L"left Principal component range"),
		GET_INTEGER (L"right Principal component range"))));
END

DIRECT (PCA_and_Configuration_to_TableOfReal_reconstruct)
	NEW (PCA_and_Configuration_to_TableOfReal_reconstruct ((structPCA *)ONLY (classPCA),
		ONLY (classConfiguration)))
END

FORM (PCA_and_TableOfReal_to_Configuration, L"PCA & TableOfReal: To Configuration", L"PCA & TableOfReal: To Configuration...")
	INTEGER (L"Number of dimensions", L"0 (=all)")
	OK
DO
	long dimension = GET_INTEGER (L"Number of dimensions");
	REQUIRE (dimension >= 0, L"Number of dimensions must be greater equal zero.")
	NEW (PCA_and_TableOfReal_to_Configuration ((structPCA *)ONLY (classPCA),
		ONLY_GENERIC (classTableOfReal), dimension))
END

FORM (PCA_getEqualityOfEigenvalues, L"PCA: Get equality of eigenvalues", L"PCA: Get equality of eigenvalues...")
	INTEGER (L"left Eigenvalue range", L"0")
	INTEGER (L"right Eigenvalue range", L"0")
	BOOLEAN (L"Conservative test", 0)
	OK
DO
	long ndf; double p, chisq;
	PCA_getEqualityOfEigenvalues ((structPCA *)ONLY_OBJECT, GET_INTEGER (L"left Eigenvalue range"),
		GET_INTEGER (L"right Eigenvalue range"), GET_INTEGER (L"Conservative test"),
		&p, &chisq, &ndf);
	Melder_information5 (Melder_double (p), L" (=probability, based on chisq = ",
		Melder_double (chisq), L"and ndf = ", Melder_integer (ndf));
END

FORM (PCA_getNumberOfComponentsVAF, L"PCA: Get number of components (VAF)", L"PCA: Get number of components (VAF)...")
	POSITIVE (L"Variance fraction (0-1)", L"0.95")
	OK
DO
	double f = GET_REAL (L"Variance fraction");
	REQUIRE (f > 0 && f <= 1, L"The variance fraction must be in interval (0-1).")
	Melder_information1 (Melder_integer (Eigen_getDimensionOfFraction ((structEigen *)ONLY_OBJECT, f)));
END

FORM (PCA_getFractionVAF, L"PCA: Get fraction variance accounted for", L"PCA: Get fraction variance accounted for...")
	NATURAL (L"left Principal component range", L"1")
	NATURAL (L"right Principal component range", L"1")
	OK
DO
	long from = GET_INTEGER (L"left Principal component range");
	long to = GET_INTEGER (L"right Principal component range");
	REQUIRE (from <= to, L"The second component must be greater than or equal to the first component.")
	Melder_information1 (Melder_double (Eigen_getCumulativeContributionOfComponents	((structEigen *)ONLY_OBJECT, from, to)));
END

FORM (PCA_invertEigenvector, L"PCA: Invert eigenvector", 0)
	NATURAL (L"Eigenvector number", L"1")
	OK
DO
	EVERY (Eigen_invertEigenvector ((structEigen *)OBJECT, GET_INTEGER (L"Eigenvector number")))
END

FORM (PCA_to_TableOfReal_reconstruct1, L"PCA: To TableOfReal (reconstruct)", L"PCA: To TableOfReal (reconstruct 1)...")
	SENTENCE (L"Coefficients", L"1.0 1.0")
	OK
DO
	EVERY_TO (PCA_to_TableOfReal_reconstruct1 ((structPCA *)OBJECT, GET_STRING (L"Coefficients")))
END

FORM (PCAs_to_Procrustes, L"PCA & PCA: To Procrustes", L"PCA & PCA: To Procrustes...")
	NATURAL (L"left Eigenvector range", L"1")
	NATURAL (L"right Eigenvector range", L"2")
	OK
DO
	long from = GET_INTEGER (L"left Eigenvector range");
	long to = GET_INTEGER (L"right Eigenvector range");
	PCA me = NULL, thee = NULL;
	WHERE (SELECTED) { if (me) thee = (structPCA *)OBJECT; else me = (structPCA *)OBJECT; }
	if (! praat_new3 (Eigens_to_Procrustes (me, thee, from, to), Thing_getName (me), L"_",
		Thing_getName (thee))) return 0;
END


DIRECT (PCAs_getAngleBetweenPc1Pc2Plane_degrees)
	PCA me = NULL, thee = NULL;
	WHERE (SELECTED) { if (me) thee = (structPCA *)OBJECT; else me = (structPCA *)OBJECT; }
	Melder_information2 (Melder_double (Eigens_getAngleBetweenEigenplanes_degrees (me, thee)),
		L" degrees (=angle of intersection between the two pc1-pc2 eigenplanes)");
END

/******************* Permutation **************************************/

DIRECT (Permutation_help)
	Melder_help (L"Permutation");
END

FORM (Permutation_create, L"Create Permutation", L"Create Permutation...")
	WORD (L"Name", L"p")
	NATURAL (L"Number of elements", L"10")
	BOOLEAN (L"Identity Permutation", 1)
	OK
DO
	Permutation p = Permutation_create (GET_INTEGER (L"Number of elements"));
	int identity = GET_INTEGER (L"Identity Permutation");
	if (! identity && ! Permutation_permuteRandomly_inline (p, 0, 0))
	{
		forget (p); return 0;
	}
	if (! praat_new1 (p, GET_STRING (L"Name"))) return 0;
END

DIRECT (Permutation_getNumberOfElements)
	Permutation p = (structPermutation *)ONLY_OBJECT;
	Melder_information1 (Melder_integer (p -> numberOfElements));
END

FORM (Permutation_getValueAtIndex, L"Permutation: Get value", L"Permutation: Get value...")
	NATURAL (L"Index", L"1")
	OK
DO
	long index = GET_INTEGER (L"Index");
	Melder_information4 (Melder_integer (Permutation_getValueAtIndex ((structPermutation *)ONLY_OBJECT, index)), L" (value, at index = ",
		Melder_integer (index), L")");
END

FORM (Permutation_getIndexAtValue, L"Permutation: Get index", L"Permutation: Get index...")
	NATURAL (L"Value", L"1")
	OK
DO
	long value = GET_INTEGER (L"Value");
	Melder_information4 (Melder_integer (Permutation_getIndexAtValue ((structPermutation *)ONLY_OBJECT, value)), L" (index, at value = ",
		Melder_integer (value), L")");
END

DIRECT (Permutation_sort)
	Permutation_sort ((structPermutation *)ONLY_OBJECT);
	praat_dataChanged (ONLY_OBJECT);
END

FORM (Permutation_swapBlocks, L"Permutation: Swap blocks", L"Permutation: Swap blocks...")
	NATURAL (L"From index", L"1")
	NATURAL (L"To index", L"2")
	NATURAL (L"Block size", L"1")
	OK
DO
	if (! Permutation_swapBlocks ((structPermutation *)ONLY_OBJECT, GET_INTEGER (L"From index"), GET_INTEGER (L"To index"), GET_INTEGER (L"Block size"))) return 0;
	praat_dataChanged (ONLY_OBJECT);
END

FORM (Permutation_swapPositions, L"Permutation: Swap positions", L"Permutation: Swap positions...")
	NATURAL (L"First index", L"1")
	NATURAL (L"Second index", L"2")
	OK
DO
	if (! Permutation_swapPositions ((structPermutation *)ONLY_OBJECT, GET_INTEGER (L"First index"), GET_INTEGER (L"Second index"))) return 0;
	praat_dataChanged (ONLY_OBJECT);
END

FORM (Permutation_swapNumbers, L"Permutation: Swap numbers", L"Permutation: Swap numbers...")
	NATURAL (L"First number", L"1")
	NATURAL (L"Second number", L"2")
	OK
DO
	if (! Permutation_swapNumbers ((structPermutation *)ONLY_OBJECT, GET_INTEGER (L"First number"), GET_INTEGER (L"Second number"))) return 0;
	praat_dataChanged (ONLY_OBJECT);
END

FORM (Permutation_swapOneFromRange, L"Permutation: Swap one from range", L"Permutation: Swap one from range...")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"A randomly chosen element from ")
	INTEGER (L"left Index range", L"0")
	INTEGER (L"right Index range", L"0")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"is swapped with the element at")
	NATURAL (L"Index", L"1")
	BOOLEAN (L"Forbid same", 1)
	OK
DO
	if (! Permutation_swapOneFromRange ((structPermutation *)ONLY_OBJECT, GET_INTEGER (L"left Index range"), GET_INTEGER (L"right Index range"),
		GET_INTEGER (L"Index"), GET_INTEGER (L"Forbid same"))) return 0;
	praat_dataChanged (ONLY_OBJECT);
END

FORM (Permutation_permuteRandomly, L"Permutation: Permute randomly", L"Permutation: Permute randomly...")
	INTEGER (L"left Index range", L"0")
	INTEGER (L"right Index range", L"0")
	OK
DO
	Permutation p = (structPermutation *)ONLY_OBJECT;
	if (! praat_new2 (Permutation_permuteRandomly (p, GET_INTEGER (L"left Index range"),
		GET_INTEGER (L"right Index range")), Thing_getName (p), L"_randomly")) return 0;
END

FORM (Permutation_rotate, L"Permutation: Rotate", L"Permutation: Rotate...")
	INTEGER (L"left Index range", L"0")
	INTEGER (L"right Index range", L"0")
	INTEGER (L"Step size", L"1")
	OK
DO
	Permutation p = (structPermutation *)ONLY_OBJECT;
	long step = GET_INTEGER (L"Step size");
	if (! praat_new3 (Permutation_rotate (p, GET_INTEGER (L"left Index range"), GET_INTEGER (L"right Index range"), step), Thing_getName (p), L"_rotate", Melder_integer (step))) return 0;
END

FORM (Permutation_reverse, L"Permutation: Reverse", L"Permutation: Reverse...")
	INTEGER (L"left Index range", L"0")
	INTEGER (L"right Index range", L"0")
	OK
DO
	Permutation p = (structPermutation *)ONLY_OBJECT;
	if (! praat_new2 (Permutation_reverse (p, GET_INTEGER (L"left Index range"), GET_INTEGER (L"right Index range")),
		Thing_getName (p), L"_reverse")) return 0;
END

FORM (Permutation_permuteBlocksRandomly, L"Permutation: Permute blocks randomly", L"Permutation: Permute randomly (blocks)...")
	INTEGER (L"left Index range", L"0")
	INTEGER (L"right Index range", L"0")
	NATURAL (L"Block size", L"12")
	BOOLEAN (L"Permute within blocks", 1)
	BOOLEAN (L"No doublets", 0)
	OK
DO
	Permutation p = (structPermutation *)ONLY_OBJECT;
	long blocksize = GET_INTEGER (L"Block size");
	if (! praat_new3 (Permutation_permuteBlocksRandomly (p, GET_INTEGER (L"left Index range"),
		GET_INTEGER (L"right Index range"), blocksize, GET_INTEGER (L"Permute within blocks"), GET_INTEGER (L"No doublets")),
		Thing_getName (p), L"_blocks", Melder_integer(blocksize))) return 0;
END

FORM (Permutation_interleave, L"Permutation: Interleave", L"Permutation: Interleave...")
	INTEGER (L"left Index range", L"0")
	INTEGER (L"right Index range", L"0")
	NATURAL (L"Block size", L"12")
	INTEGER (L"Offset", L"0")
	OK
DO
	Permutation p = (structPermutation *)ONLY_OBJECT;
	if (! praat_new2 (Permutation_interleave ((structPermutation *)ONLY_OBJECT, GET_INTEGER (L"left Index range"), GET_INTEGER (L"right Index range"),
		GET_INTEGER (L"Block size"), GET_INTEGER (L"Offset")), Thing_getName (p), L"_interleave")) return 0;
END

DIRECT (Permutation_invert)
	Permutation p = (structPermutation *)ONLY_OBJECT;
	if (! praat_new2 (Permutation_invert (p), Thing_getName (p), L"_inverse")) return 0;
END

DIRECT (Permutations_multiply)
	long np = 0, n = 0;
	Permutation buf = NULL, thee = NULL;

	WHERE (SELECTED)
	{
		Permutation me = (structPermutation *)OBJECT;
		if (n == 0)
		{
			n = my numberOfElements;
		}
		else if (my numberOfElements != n)
		{
			return Melder_error1 (L"To apply a number of permutations they all must have the same number of elements.");
		}
		np += 1;
	}
	WHERE (SELECTED)
	{
		Permutation p = (structPermutation *)OBJECT;
		if (thee == NULL)
		{
			thee = (structPermutation *)Data_copy (p);
			if (thee == NULL) return 0;
		}
		else
		{
			long i;
			buf = (structPermutation *)Data_copy (thee);
			if (buf == NULL) goto end;
			for (i = 1; i <= n; i++)
			{
				thy p[i] = buf -> p[p -> p[i]];
			}
			forget (buf);
		}
	}
end:
	if (Melder_hasError ())
	{
		forget (thee); return 0;
	}
	if (! praat_new2 (thee, L"multiply", Melder_integer(np))) return 0;
END

DIRECT (Permutations_next)
	Permutation p = (structPermutation *)ONLY_OBJECT;
	Permutation_next_inline (p);
	praat_dataChanged (p);
END

DIRECT (Permutations_previous)
	Permutation p = (structPermutation *)ONLY_OBJECT;
	Permutation_previous_inline (p);
	praat_dataChanged (p);
END

FORM (Pitches_to_DTW, L"Pitches: To DTW", L"Pitches: To DTW...")
	REAL (L"Voiced-unvoiced costs", L"24.0")
	REAL (L"Time costs weight", L"10.0")
	DTW_constraints_addCommonFields (dia);
	OK
DO
	Pitch p1 = NULL, p2 = NULL;
	int begin, end, slope;
	DTW_constraints_getCommonFields (dia, &begin, &end, &slope);
	WHERE (SELECTED) { if (p1) p2 = (structPitch *)OBJECT; else p1 = (structPitch *)OBJECT; }
	if (! praat_new4 (Pitches_to_DTW (p1, p2, GET_REAL (L"Voiced-unvoiced costs"), GET_REAL (L"Time costs weight"), begin, end, slope), L"dtw_", p1 -> name, L"_", p2 -> name)) return 0;

END

FORM (PitchTier_to_Pitch, L"PitchTier: To Pitch", L"PitchTier: To Pitch...")
	POSITIVE (L"Step size", L"0.02")
	POSITIVE (L"Pitch floor", L"60.0")
	POSITIVE (L"Pitch ceiling", L"400.0")
	OK
DO
	EVERY_TO (PitchTier_to_Pitch ((structPitchTier *)ONLY(classPitchTier), GET_REAL (L"Step size"),
		GET_REAL (L"Pitch floor"),GET_REAL (L"Pitch ceiling")))
END

/******************* Polygon & Categories *************************************/

FORM (Polygon_translate, L"Polygon: Translate", L"Polygon: Translate...")
	REAL (L"X", L"0.0")
	REAL (L"Y", L"0.0")
	OK
DO
	Polygon_translate ((structPolygon *)ONLY(classPolygon), GET_REAL (L"X"), GET_REAL (L"Y"));
END

FORM (Polygon_rotate, L"Polygon: Rotate", L"Polygon: Rotate...")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Rotate counterclockwise over the")
	REAL (L"Angle (degrees)", L"0.0")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"With respect to the point")
	REAL (L"X", L"0.0")
	REAL (L"Y", L"0.0")
	OK
DO
	Polygon_rotate ((structPolygon *)ONLY(classPolygon), GET_REAL (L"Angle"), GET_REAL (L"X"), GET_REAL (L"Y"));
END

FORM (Polygon_scale, L"Polygon: Scale polygon", 0)
	REAL (L"X", L"0.0")
	REAL (L"Y", L"0.0")
	OK
DO
	Polygon_scale ((structPolygon *)ONLY(classPolygon), GET_REAL (L"X"), GET_REAL (L"Y"));
END

void SimpleString_draw (SimpleString me, Any g, double xWC, double yWC);

void OrderedOfString_drawItem (I, Any g, long index, double xWC, double yWC)
{
	iam (OrderedOfString);
    if (index > 0 && index <= my size) 
	{
		SimpleString_draw ((structSimpleString *)my item[index], g, xWC, yWC);
	}
}

/* reverse axis when min > max */
void Polygon_Categories_draw (I, thou, Any graphics, double xmin, double xmax,
	double ymin, double ymax, int garnish)
{
    iam (Polygon); thouart (Categories);
    double min, max, tmp;

    if (my numberOfPoints != thy size) return;

    if (xmax == xmin)
    {
		NUMdvector_extrema (my x, 1, my numberOfPoints, & min, & max);
		tmp = max - min == 0 ? 0.5 : 0.0;
		xmin = min - tmp; xmax = max + tmp;
    }

    if (ymax == ymin)
    {
		NUMdvector_extrema (my y, 1, my numberOfPoints, & min, & max);
		tmp = max - min == 0 ? 0.5 : 0.0;
		ymin = min - tmp; ymax = max + tmp;
    }

    Graphics_setInner (graphics);
    Graphics_setWindow (graphics, xmin, xmax, ymin, ymax);
    Graphics_setTextAlignment (graphics, Graphics_CENTRE, Graphics_HALF);

    for (long i = 1; i <= my numberOfPoints; i++)
	{
		OrderedOfString_drawItem (thee, graphics, i, my x[i], my y[i]);
	}
    Graphics_unsetInner (graphics);
    if (garnish)
    {
		Graphics_drawInnerBox (graphics);
		Graphics_marksLeft (graphics, 2, 1, 1, 0);
		if (ymin * ymax < 0.0)
		{
			Graphics_markLeft (graphics, 0.0, 1, 1, 1, NULL);
		}
		Graphics_marksBottom (graphics, 2, 1, 1, 0);
		if (xmin * xmax < 0.0)
		{
			Graphics_markBottom (graphics, 0.0, 1, 1, 1, NULL);
		}
	}
}

FORM (Polygon_Categories_draw, L"Polygon & Categories: Draw", 0)
    REAL (L"left Horizontal range", L"0.0")
    REAL (L"right Horizontal range", L"0.0")
    REAL (L"left Vertical range", L"0.0")
    REAL (L"right Vertical range", L"0.0")
    BOOLEAN (L"Garnish", 1)
	OK
DO
	EVERY_DRAW (Polygon_Categories_draw ((structPolygon *)ONLY(classPolygon),
		ONLY(classCategories),
		GRAPHICS, GET_REAL (L"left Horizontal range"), GET_REAL (L"right Horizontal range"),
		GET_REAL (L"left Vertical range"), GET_REAL (L"right Vertical range"),
		GET_INTEGER (L"Garnish")))
END

DIRECT (Polygon_reverseX)
	Polygon_reverseX ((structPolygon *)ONLY(classPolygon));
END

DIRECT (Polygon_reverseY)
	Polygon_reverseY ((structPolygon *)ONLY(classPolygon));
END

/***************** Polynomial *******************/

DIRECT (Polynomial_help) Melder_help (L"Polynomial"); END

FORM (Polynomial_create, L"Create Polynomial", L"Create Polynomial...")
	WORD (L"Name", L"p")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Domain of polynomial")
	REAL (L"Xmin", L"-3")
	REAL (L"Xmax", L"4")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"p(x) = c[1] + c[2] x + ... c[n+1] x^n")
	SENTENCE (L"Coefficients", L"2.0 -1.0 -2.0 1.0")
	OK
DO
	double xmin = GET_REAL (L"Xmin"), xmax = GET_REAL (L"Xmax");
	REQUIRE (xmin < xmax, L"Xmin must be smaller than Xmax.")
	if (! praat_new1 (Polynomial_createFromString (xmin, xmax,
		GET_STRING (L"Coefficients")), GET_STRING (L"Name"))) return 0;
END

FORM (Polynomial_getArea, L"Polynomial: Get area", L"Polynomial: Get area...")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Interval")
	REAL (L"Xmin", L"0.0")
	REAL (L"Xmax", L"0.0")
	OK
DO
	double area = Polynomial_getArea ((structPolynomial *)ONLY (classPolynomial),
		GET_REAL (L"Xmin"), GET_REAL (L"Xmax"));
	Melder_information1 (Melder_double (area));
END

DIRECT (Polynomial_getDerivative)
	EVERY_TO (Polynomial_getDerivative ((structPolynomial *)OBJECT))
END

DIRECT (Polynomial_getPrimitive)
	EVERY_TO (Polynomial_getPrimitive ((structPolynomial *)OBJECT))
END

FORM (Polynomial_scaleX, L"Polynomial: Scale x", L"Polynomial: Scale x...")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"New domain")
	REAL (L"Xmin", L"-1.0")
	REAL (L"Xmax", L"1.0")
	OK
DO
	double xmin = GET_REAL (L"Xmin"), xmax = GET_REAL (L"Xmax");
	REQUIRE (xmin < xmax, L"Xmin must be smaller than Xmax.")
	EVERY_TO (Polynomial_scaleX ((structPolynomial *)OBJECT, xmin, xmax))
END

DIRECT (Polynomial_scaleCoefficients_monic)
	EVERY (Polynomial_scaleCoefficients_monic ((structPolynomial *)OBJECT))
END

DIRECT (Polynomial_to_Roots)
	EVERY_TO (Polynomial_to_Roots ((structPolynomial *)OBJECT))
END

FORM (Polynomial_evaluate_z, L"Polynomial: Get value (complex)", L"Polynomial: Get value (complex)...")
	REAL (L"Real part", L"0.0")
	REAL (L"Imaginary part", L"0.0")
	OK
DO
	dcomplex p, z = dcomplex_create (GET_REAL (L"Real part"), GET_REAL (L"Imaginary part"));
	Polynomial_evaluate_z ((structPolynomial *)ONLY_OBJECT, &z, &p);
	Melder_information4 (Melder_double (p.re), L" + ", Melder_double (p.im), L" i");
END


FORM (Polynomial_to_Spectrum, L"Polynomial: To Spectrum", L"Polynomial: To Spectrum...")
	POSITIVE (L"Nyquist frequency (Hz)", L"5000.0")
	NATURAL (L"Number of frequencies (>1)", L"1025")
	OK
DO
	long n = GET_INTEGER (L"Number of frequencies");
	REQUIRE (n > 1, L"\"Number of frequencies\" must be greater than 1.")
	EVERY_TO (Polynomial_to_Spectrum ((structPolynomial *)OBJECT, GET_REAL (L"Nyquist frequency"),
		n, 1.0))
END

DIRECT (Polynomials_multiply)
	Polynomial p1 = NULL, p2 = NULL;
	WHERE (SELECTED) { if (p1) p2 = (structPolynomial *)OBJECT; else p1 = (structPolynomial *)OBJECT; }
	if (! praat_new4 (Polynomials_multiply (p1, p2), p1->name, L"_", p2->name, L"_mul")) return 0;
END

FORM (Polynomials_divide, L"Polynomials: Divide", L"Polynomials: Divide...")
	BOOLEAN (L"Want quotient", 1)
	BOOLEAN (L"Want remainder", 1)
	OK
DO
	/* With gcc (GCC) 3.2.2 20030217 (Red Hat Linux 8.0 3.2.2-2)
		The following line initiates pq = NULL and I don't know why
	Polynomial p1 = NULL, p2 = NULL, pq, pr;
	*/
	Polynomial p1 = NULL, p2 = NULL, s, r, q;
	int wantq = GET_INTEGER (L"Want quotient");
	int wantr = GET_INTEGER (L"Want remainder");
	REQUIRE (wantq || wantr, L"Either \'Want quotient\' or \'Want remainder\' must be chosen")
	WHERE (SELECTED) { if (p1) p2 = (structPolynomial *)OBJECT; else p1 = (structPolynomial *)OBJECT; }
	if (! wantq) q = NULL;
	if (! wantr) r = NULL;
	s = Polynomial_create (0,1,1);
	forget (s);
	if (! Polynomials_divide (p1, p2, &q, &r)) return 0;
	if (wantq && ! praat_new2 (q, p1 -> name, L"_q")) return 0;
	if (wantr && ! praat_new2 (r, p1 -> name, L"_r")) return 0;
END

/********************* Roots ******************************/

void Roots_draw (Roots me, Graphics g, double rmin, double rmax, double imin,
	double imax, wchar_t *symbol, int fontSize, int garnish)
{
	long i; int oldFontSize = Graphics_inqFontSize (g);
	double eps = 1e-6, denum;
	
	if (rmax <= rmin)
	{
		NUMdcvector_extrema_re (my v, 1, my max, &rmin, &rmax);
	}
	denum = fabs (rmax) > fabs (rmin) ? fabs(rmax) : fabs (rmin);
	if (denum == 0) denum = 1;
	if (fabs((rmax - rmin) / denum) < eps)
	{
		rmin -= 1; rmax += 1; 
	}
	if (imax <= imin)
	{
		NUMdcvector_extrema_im (my v, 1, my max, &imin, &imax);
	}
	denum = fabs (imax) > fabs (imin) ? fabs(imax) : fabs (imin);
	if (denum == 0) denum = 1;
	if (fabs((imax - imin) / denum) < eps)
	{
		imin -= 1; imax += 1;
	}
	Graphics_setInner (g);
	Graphics_setWindow (g, rmin, rmax, imin, imax);
	Graphics_setFontSize (g, fontSize);
	Graphics_setTextAlignment (g, Graphics_CENTRE, Graphics_HALF);
	for (i=1; i <= my max; i++)
	{
		double re = my v[i].re, im = my v[i].im;
		if (re >= rmin && re <= rmax && im >= imin && im <= imax)
		{
			Graphics_text (g, re, im, symbol);
		}
	}
	Graphics_setFontSize (g, oldFontSize);
	Graphics_unsetInner (g);
	if (garnish)
	{
    	Graphics_drawInnerBox (g);
 		if (rmin * rmax < 0) Graphics_markLeft (g, 0, 1, 1, 1, L"0");
 		if (imin * imax < 0) Graphics_markBottom (g, 0, 1, 1, 1, L"0");
    	Graphics_marksLeft (g, 2, 1, 1, 0);
 		Graphics_textLeft (g, 1, L"Imaginary part");
    	Graphics_marksBottom (g, 2, 1, 1, 0);
 		Graphics_textBottom (g, 1, L"Real part");
	}
}

DIRECT (Roots_help) Melder_help (L"Roots"); END

FORM (Roots_draw, L"Roots: Draw", 0)
	REAL (L"Minimum of real axis", L"0.0")
	REAL (L"Maximum of real axis", L"0.0")
	REAL (L"Minimum of imaginary axis", L"0.0")
	REAL (L"Maximum of imaginary axis", L"0.0")
	SENTENCE (L"Mark string (+x0...)", L"o")
	NATURAL (L"Mark size", L"12")
	BOOLEAN (L"Garnish", 0)
	OK
DO
	EVERY_DRAW (Roots_draw ((structRoots *)OBJECT, GRAPHICS,
		GET_REAL (L"Minimum of real axis"), GET_REAL (L"Maximum of real axis"),
		GET_REAL (L"Minimum of imaginary axis"),
		GET_REAL (L"Maximum of imaginary axis"),
		GET_STRING (L"Mark string"), GET_INTEGER (L"Mark size"),
		GET_INTEGER (L"Garnish")))
END

DIRECT (Roots_getNumberOfRoots)
	Melder_information1 (Melder_integer (Roots_getNumberOfRoots ((structRoots *)ONLY (classRoots))));
END

FORM (Roots_getRoot, L"Roots: Get root", 0)
	NATURAL (L"Root number", L"1")
	OK
DO
	dcomplex z = Roots_getRoot ((structRoots *)ONLY (classRoots), GET_INTEGER (L"Root number"));
	Melder_information4 (Melder_double (z.re), (z.im < 0 ? L" - " : L" + "), Melder_double (fabs(z.im)), L" i");
END

FORM (Roots_getRealPartOfRoot, L"Roots: Get real part", 0)
	NATURAL (L"Root number", L"1")
	OK
DO
	dcomplex z = Roots_getRoot ((structRoots *)ONLY (classRoots), GET_INTEGER (L"Root number"));
	Melder_information1 (Melder_double (z.re));
END

FORM (Roots_getImaginaryPartOfRoot, L"Roots: Get imaginary part", 0)
	NATURAL (L"Root number", L"1")
	OK
DO
	dcomplex z = Roots_getRoot ((structRoots *)ONLY (classRoots), GET_INTEGER (L"Root number"));
	Melder_information1 (Melder_double (z.im));
END

FORM (Roots_setRoot, L"Roots: Set root", 0)
	NATURAL (L"Root number", L"1")
	REAL (L"Real part", L"1.0/sqrt(2)")
	REAL (L"Imaginary part", L"1.0/sqrt(2)")
	OK
DO
	if (! Roots_setRoot ((structRoots *)ONLY_OBJECT, GET_INTEGER (L"Root number"),
		GET_REAL (L"Real part"), GET_REAL (L"Imaginary part"))) return 0;
END

FORM (Roots_to_Spectrum, L"Roots: To Spectrum", L"Roots: To Spectrum...")
	POSITIVE (L"Nyquist frequency (Hz)", L"5000.0")
	NATURAL (L"Number of frequencies (>1)", L"1025")
	OK
DO
	long n = GET_INTEGER (L"Number of frequencies");
	REQUIRE (n > 1, L"\"Number of frequencies\" must be greater than 1.")
	EVERY_TO (Roots_to_Spectrum ((structRoots *)OBJECT, GET_REAL (L"Nyquist frequency"),
		n, 1.0))
END

DIRECT (Roots_and_Polynomial_polish)
	 Roots_and_Polynomial_polish ((structRoots *)ONLY(classRoots), (structPolynomial *)ONLY(classPolynomial));
END

/*****************************************************************************/

DIRECT (Praat_ReportFloatingPointProperties)
	if (! NUMfpp) NUMmachar ();
	MelderInfo_open ();
	MelderInfo_writeLine1 (L"Double precision floating point properties of this machine,");
	MelderInfo_writeLine1 (L"as calculated by algorithms from the Binary Linear Algebra System (BLAS)");
	MelderInfo_writeLine2 (L"Radix: ", Melder_double (NUMfpp -> base));
	MelderInfo_writeLine2 (L"Number of digits in mantissa: ", Melder_double (NUMfpp -> t));
	MelderInfo_writeLine2 (L"Smallest exponent before (gradual) underflow (expmin): ", Melder_integer (NUMfpp -> emin));
	MelderInfo_writeLine2 (L"Largest exponent before overflow (expmax): ", Melder_integer (NUMfpp -> emax));
	MelderInfo_writeLine2 (L"Does rounding occur in addition: ", (NUMfpp -> rnd == 1 ? L"yes" : L"no"));
	MelderInfo_writeLine2 (L"Quantization step (d): ", Melder_double (NUMfpp -> prec));
	MelderInfo_writeLine2 (L"Quantization error (eps = d/2): ", Melder_double (NUMfpp -> eps));
	MelderInfo_writeLine2 (L"Underflow threshold (= radix ^ (expmin - 1)): ", Melder_double (NUMfpp -> rmin));
	MelderInfo_writeLine2 (L"Safe minimum (such that its inverse does not overflow): ", Melder_double (NUMfpp -> sfmin));
	MelderInfo_writeLine2 (L"Overflow threshold (= (1 - eps) * radix ^ expmax): ", Melder_double (NUMfpp -> rmax));
	MelderInfo_close ();
END

/******************** Sound ****************************************/

static void Sound_create_addCommonFields (UiForm *dia)
{
	REAL (L"Starting time (s)", L"0.0")
	REAL (L"Finishing time (s)", L"0.1")
	POSITIVE (L"Sampling frequency (Hz)", L"44100.0")
}

static int Sound_create_checkCommonFields (UiForm *dia, double *startingTime, double *finishingTime,
	double *samplingFrequency)
{
	double numberOfSamples_real;
	*startingTime = GET_REAL (L"Starting time");
	*finishingTime = GET_REAL (L"Finishing time");
	*samplingFrequency = GET_REAL (L"Sampling frequency");
	numberOfSamples_real = floor ((*finishingTime - *startingTime) * *samplingFrequency + 0.5);
	if (*finishingTime <= *startingTime)
	{
		if (*finishingTime == *startingTime)
			(void) Melder_error1 (L"A Sound cannot have a duration of zero.\n");
		else
			(void) Melder_error1 (L"A Sound cannot have a duration less than zero.\n");
		if (*startingTime == 0.0)
			return Melder_error1 (L"Please set the finishing time to something greater than 0 seconds.");
		else
			return Melder_error1 (L"Please lower the starting time or raise the finishing time.");
	}
	if (*samplingFrequency <= 0.0)
	{
		(void) Melder_error1 (L"A Sound cannot have a negative sampling frequency.\n");
		return Melder_error1 (L"Please set the sampling frequency to something greater than zero, e.g. 44100 Hz.");
	}
	if (numberOfSamples_real < 1.0)
	{
		(void) Melder_error1 (L"A Sound cannot have zero samples.\n");
		if (*startingTime == 0.0)
			return Melder_error1 (L"Please raise the finishing time.");
		else
			return Melder_error1 (L"Please lower the starting time or raise the finishing time.");
	}
	if (numberOfSamples_real > LONG_MAX)
	{
		(void) Melder_error5 (L"A Sound cannot have ", Melder_bigInteger (numberOfSamples_real), L" samples; the maximum is ", Melder_bigInteger (LONG_MAX), L" samples.\n");
		if (*startingTime == 0.0)
			return Melder_error1 (L"Please lower the finishing time or the sampling frequency.");
		else
			return Melder_error1 (L"Please raise the starting time, lower the finishing time, or lower the sampling frequency.");
	}
	return 1;
}

static int Sound_create_check (Sound me, double startingTime, double finishingTime, double samplingFrequency)
{
	if (me != NULL) return 1;

	if (wcsstr (Melder_getError (), L"memory"))
	{
		double numberOfSamples_real = floor ((finishingTime - startingTime) * samplingFrequency + 0.5);
		Melder_clearError ();
		(void) Melder_error3 (L"There is not enough memory to create a Sound that contains ", Melder_bigInteger (numberOfSamples_real), L" samples.\n");
		if (startingTime == 0.0)
			return Melder_error1 (L"You could lower the finishing time or the sampling frequency and try again.");
		else
			return Melder_error1 (L"You could raise the starting time or lower the finishing time or the sampling frequency, and try again.");
	}

	return 0;
}

FORM (Sound_and_Pitch_to_FormantFilter, L"Sound & Pitch: To FormantFilter", L"Sound & Pitch: To FormantFilter...")
	POSITIVE (L"Analysis window duration (s)", L"0.015")
	POSITIVE (L"Time step (s)", L"0.005")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Filter bank parameters")
	POSITIVE (L"Position of first filter (Hz)", L"100.0")
	POSITIVE (L"Distance between filters (Hz)", L"50.0")
	REAL (L"Maximum frequency", L"0");
	POSITIVE (L"Relative bandwidth", L"1.1")
	OK
DO
	 if (! praat_new1 (Sound_and_Pitch_to_FormantFilter ((structSound *)ONLY(classSound),
	 	(structPitch *)ONLY(classPitch), GET_REAL (L"Analysis window duration"),
		GET_REAL (L"Time step"), GET_REAL (L"Position of first filter"),
		GET_REAL (L"Maximum frequency"), GET_REAL (L"Distance between filters"),
		GET_REAL (L"Relative bandwidth")), NULL)) return 0;
END

FORM (Sound_and_Pitch_changeGender, L"Sound & Pitch: Change gender", L"Sound & Pitch: Change gender...")
	POSITIVE (L"Formant shift ratio", L"1.2")
	REAL (L"New pitch median (Hz)", L"0.0 (=no change)")
	POSITIVE (L"Pitch range factor", L"1.0 (=no change)")
	POSITIVE (L"Duration factor", L"1.0")
	OK
DO
	if (! praat_new1 (Sound_and_Pitch_changeGender_old ((structSound *)ONLY(classSound), (structPitch *)ONLY(classPitch),
		GET_REAL (L"Formant shift ratio"), GET_REAL (L"New pitch median"),
		GET_REAL (L"Pitch range factor"), GET_REAL (L"Duration factor")), NULL)) return 0;
END

FORM (Sound_and_Pitch_changeSpeaker, L"Sound & Pitch: Change speaker", L"Sound & Pitch: Change speaker...")
	POSITIVE (L"Multiply formants by", L"1.1 (male->female)")
	POSITIVE (L"Multiply pitch by", L"1.8 (male->female")
	REAL (L"Multiply pitch range by", L"1.0 (=no change)")
	POSITIVE (L"Multiply duration", L"1.0")
	OK
DO
	if (! praat_new1 (Sound_and_Pitch_changeSpeaker ((structSound *)ONLY(classSound), (structPitch *)ONLY(classPitch),
		GET_REAL (L"Multiply formants by"), GET_REAL (L"Multiply pitch by"),
		GET_REAL (L"Multiply pitch range by"), GET_REAL (L"Multiply duration")), NULL)) return 0;
END

FORM (Sound_createFromGammaTone, L"Create a gammatone", L"Create Sound from gammatone...")
	WORD (L"Name", L"gammatone")
	Sound_create_addCommonFields (dia);
	INTEGER (L"Gamma", L"4")
	POSITIVE (L"Frequency (Hz)", L"1000.0")
	REAL (L"Bandwidth (Hz)", L"150.0")
	REAL (L"Initial phase (radians)", L"0.0")
	REAL (L"Addition factor", L"0.0")
	BOOLEAN (L"Scale amplitudes", 1)
	OK
DO
	Sound sound = NULL;
	double startingTime, finishingTime, samplingFrequency;
	long gamma = GET_INTEGER (L"Gamma");
	double bandwidth = GET_REAL (L"Bandwidth");
	double f = GET_REAL (L"Frequency");

	if (! Sound_create_checkCommonFields (dia, &startingTime, &finishingTime, &samplingFrequency))
		return 0;
	if (f >= samplingFrequency / 2) return Melder_error2
		(L"Frequency cannot be larger than half the sampling frequency.\n"
			"Please use a frequency smaller than ", Melder_double (samplingFrequency / 2));
	if (gamma < 0) return Melder_error1 (L"Gamma cannot be negative.\nPlease use a positive or zero gamma.");
	if (bandwidth < 0) return Melder_error1 (L"Bandwidth cannot be negative.\nPlease use a positive or zero bandwidth.");
	sound = Sound_createGammaTone (startingTime, finishingTime, samplingFrequency, gamma, f,
		bandwidth, GET_REAL (L"Initial phase"), GET_REAL (L"Addition factor"),
		GET_INTEGER (L"Scale amplitudes"));
	if (! Sound_create_check (sound, startingTime, finishingTime, samplingFrequency) ||
		! praat_new1 (sound, GET_STRING (L"Name"))) return 0;
END

FORM (Sound_createFromShepardTone, L"Create a Shepard tone", L"Create Sound from Shepard tone...")
	WORD (L"Name", L"shepardTone")
	Sound_create_addCommonFields (dia);
	POSITIVE (L"Lowest frequency (Hz)", L"4.863")
	NATURAL (L"Number of components", L"10")
	REAL (L"Frequency change (semitones/s)", L"4.0")
	REAL (L"Amplitude range (dB)", L"30.0")
	REAL (L"Octave shift fraction ([0,1))", L"0.0")
	OK
DO
	Sound sound = NULL;
	double startingTime, finishingTime, samplingFrequency;
	double amplitudeRange = GET_REAL (L"Amplitude range");
	double octaveShiftFraction = GET_REAL (L"Octave shift fraction");
	if (! Sound_create_checkCommonFields (dia, &startingTime, &finishingTime, &samplingFrequency)) return 0;
	if (amplitudeRange < 0) return Melder_error1 (L"Amplitude range cannot be negative.\nPlease use a positive or zero amplitude range.");
	sound = Sound_createShepardToneComplex (startingTime, finishingTime, samplingFrequency,
		GET_REAL (L"Lowest frequency"), GET_INTEGER (L"Number of components"),
		GET_REAL (L"Frequency change"), GET_REAL (L"Amplitude range"), octaveShiftFraction);

	if (! Sound_create_check (sound, startingTime, finishingTime, samplingFrequency) ||
		! praat_new1 (sound, GET_STRING (L"Name"))) return 0;
END

FORM (Sound_drawWhere, L"Sound: Draw where", L"Sound: Draw where...")
	REAL (L"left Time range (s)", L"0.0")
	REAL (L"right Time range", L"0.0 (= all)")
	REAL (L"left Vertical range", L"0.0")
	REAL (L"right Vertical range", L"0.0 (= auto)")
	BOOLEAN (L"Garnish", 1)
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"ui/editors/AmplitudeTierEditor.h")
	OPTIONMENU (L"Drawing method", 1)
		OPTION (L"Curve")
		OPTION (L"Bars")
		OPTION (L"Poles")
		OPTION (L"Speckles")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Draw only those parts where the following condition holds:")
	TEXTFIELD (L"Formula", L"x < xmin + (xmax - xmin) / 2; first half")
	OK
DO
	long numberOfBisections = 10;
	EVERY_DRAW (Sound_drawWhere ((structSound *)OBJECT, GRAPHICS, GET_REAL (L"left Time range"), GET_REAL (L"right Time range"), GET_REAL (L"left Vertical range"), GET_REAL (L"right Vertical range"), GET_INTEGER (L"Garnish"),
	GET_STRING (L"Drawing method"), numberOfBisections, GET_STRING (L"Formula"), interpreter))
END

FORM (Sound_to_TextGrid_detectSilences, L"Sound: To TextGrid (silences)", L"Sound: To TextGrid (silences)...")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Parameters for the intensity analysis")
	POSITIVE (L"Minimum pitch (Hz)", L"100")
	REAL (L"Time step (s)", L"0.0 (= auto)")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Silent intervals detection")
	REAL (L"Silence threshold (dB)", L"-25.0")
	POSITIVE (L"Minimum silent interval duration (s)", L"0.1")
	POSITIVE (L"Minimum sounding interval duration (s)", L"0.1")
	WORD (L"Silent interval label", L"silent")
	WORD (L"Sounding interval label", L"sounding")
	OK
DO
	EVERY_TO (Sound_to_TextGrid_detectSilences ((structSound *)OBJECT, GET_REAL (L"Minimum pitch"), GET_REAL (L"Time step"),
		GET_REAL (L"Silence threshold"), GET_REAL (L"Minimum silent interval duration"),
		GET_REAL (L"Minimum sounding interval duration"), GET_STRING (L"Silent interval label"),
		GET_STRING (L"Sounding interval label")))

END

FORM (Sound_to_BarkFilter, L"Sound: To BarkFilter", L"Sound: To BarkFilter...")
	POSITIVE (L"Window length (s)", L"0.015")
	POSITIVE (L"Time step (s)", L"0.005")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Filter bank parameters")
	POSITIVE (L"Position of first filter (bark)", L"1.0")
	POSITIVE (L"Distance between filters (bark)", L"1.0")
	REAL (L"Maximum frequency (bark)", L"0");
	OK
DO
	EVERY_TO (Sound_to_BarkFilter ((structSound *)OBJECT, GET_REAL (L"Window length"),
		GET_REAL (L"Time step"), GET_REAL (L"Position of first filter"),
		GET_REAL (L"Maximum frequency"), GET_REAL (L"Distance between filters")))
END

FORM (Sound_to_FormantFilter, L"Sound: To FormantFilter", L"Sound: To FormantFilter...")
	POSITIVE (L"Window length (s)", L"0.015")
	POSITIVE (L"Time step (s)", L"0.005")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Filter bank parameters")
	POSITIVE (L"Position of first filter (Hz)", L"100.0")
	POSITIVE (L"Distance between filters (Hz)", L"50.0")
	REAL (L"Maximum frequency", L"0");
	POSITIVE (L"Relative bandwidth", L"1.1")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Pitch analysis")
	REAL (L"Minimum pitch (Hz)", L"75.0")
	REAL (L"Maximum pitch (Hz)", L"600.0")
	OK
DO
	EVERY_TO (Sound_to_FormantFilter ((structSound *)OBJECT, GET_REAL (L"Window length"),
		GET_REAL (L"Time step"), GET_REAL (L"Position of first filter"),
		GET_REAL (L"Maximum frequency"), GET_REAL (L"Distance between filters"),
		GET_REAL (L"Relative bandwidth"), GET_REAL (L"Minimum pitch"),
		GET_REAL (L"Maximum pitch")))
END

FORM (Sound_to_MelFilter, L"Sound: To MelFilter", L"Sound: To MelFilter...")
	POSITIVE (L"Window length (s)", L"0.015")
	POSITIVE (L"Time step (s)", L"0.005")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Filter bank parameters")
	POSITIVE (L"Position of first filter (mel)", L"100.0")
	POSITIVE (L"Distance between filters (mel)", L"100.0")
	REAL (L"Maximum frequency (mel)", L"0.0");
	OK
DO
	EVERY_TO (Sound_to_MelFilter ((structSound *)OBJECT, GET_REAL (L"Window length"),
		GET_REAL (L"Time step"), GET_REAL (L"Position of first filter"),
		GET_REAL (L"Maximum frequency"), GET_REAL (L"Distance between filters")))
END

FORM (Sound_to_Pitch_shs, L"Sound: To Pitch (shs)", L"Sound: To Pitch (shs)...")
	POSITIVE (L"Time step (s)", L"0.01")
	POSITIVE (L"Minimum pitch (Hz)", L"50.0")
	NATURAL (L"Max. number of candidates (Hz)", L"15")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Algorithm parameters")
	POSITIVE (L"Maximum frequency component (Hz)", L"1250.0")
	NATURAL (L"Max. number of subharmonics", L"15")
	POSITIVE (L"Compression factor (<=1)", L"0.84")
	POSITIVE (L"Ceiling (Hz)", L"600.0")
	NATURAL (L"Number of points per octave", L"48");
	OK
DO
	double minimumPitch = GET_REAL (L"Minimum pitch");
	double fmax = GET_REAL (L"Maximum frequency component");
	double ceiling = GET_REAL (L"Ceiling");
	REQUIRE (minimumPitch < ceiling, L"Minimum pitch should be smaller than ceiling.")
	REQUIRE (ceiling <= fmax, L"Maximum frequency must be greater than or equal to ceiling.")
	EVERY_TO (Sound_to_Pitch_shs ((structSound *)OBJECT, GET_REAL (L"Time step"),
		minimumPitch, fmax, ceiling,
		GET_INTEGER (L"Max. number of subharmonics"),
		GET_INTEGER (L"Max. number of candidates"),
		GET_REAL (L"Compression factor"),
		GET_INTEGER (L"Number of points per octave")))
END

FORM (Sound_fadeIn, L"Sound: Fade in", L"Sound: Fade in...")
	OPTIONMENU (L"Channel", 1)
	OPTION (L"All")
	OPTION (L"Left")
	OPTION (L"Right")
	REAL (L"Time (s)", L"-10000.0")
	REAL (L"Fade time (s)", L"0.005")
	BOOLEAN (L"Silent from start", 0)
	OK
DO
	long channel = GET_INTEGER (L"Channel") - 1;
	WHERE (SELECTED)
	{
		Sound_fade ((structSound *)OBJECT, channel, GET_REAL (L"Time"), GET_REAL (L"Fade time"), -1, GET_INTEGER (L"Silent from start"));
		praat_dataChanged (OBJECT);
	}
END

FORM (Sound_fadeOut, L"Sound: Fade out", L"Sound: Fade out...")
	OPTIONMENU (L"Channel", 1)
	OPTION (L"All")
	OPTION (L"Left")
	OPTION (L"Right")
	REAL (L"Time (s)", L"10000.0")
	REAL (L"Fade time (s)", L"-0.005")
	BOOLEAN (L"Silent to end", 0)
	OK
DO
	long channel = GET_INTEGER (L"Channel") - 1;
	WHERE (SELECTED)
	{
		Sound_fade ((structSound *)OBJECT, channel, GET_REAL (L"Time"), GET_REAL (L"Fade time"), 1, GET_INTEGER (L"Silent to end"));
		praat_dataChanged (OBJECT);
	}
END

FORM (Sound_to_KlattGrid_simple, L"Sound: To KlattGrid (simple)", L"Sound: To KlattGrid (simple)...")
	POSITIVE (L"Time step (s)", L"0.005")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Formant determination")
	NATURAL (L"Max. number of formants", L"5")
	POSITIVE (L"Maximum formant (Hz)", L"5500 (=adult female)")
	POSITIVE (L"Window length (s)", L"0.025")
	POSITIVE (L"Pre-emphasis from (Hz)", L"50.0")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Pitch determination")
	POSITIVE (L"Pitch floor (Hz)", L"60.0")
	POSITIVE (L"Pitch ceiling (Hz)", L"600.0")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Intensity determination")
	POSITIVE (L"Minimum pitch (Hz)", L"100.0")
	BOOLEAN (L"Subtract mean", 1)
	OK
DO
	WHERE (SELECTED)
	{
		if (! praat_new1 (Sound_to_KlattGrid_simple ((structSound *)OBJECT, GET_REAL (L"Time step"),
			GET_INTEGER (L"Max. number of formants"), GET_REAL (L"Maximum formant"),
			GET_REAL (L"Window length"), GET_REAL (L"Pre-emphasis from"),
			GET_REAL (L"Pitch floor"), GET_REAL (L"Pitch ceiling"),
			GET_REAL (L"Minimum pitch"), GET_INTEGER (L"Subtract mean")), FULL_NAME)) return 0;
	}
END

/*void SPINET_spectralRepresentation (SPINET me, Graphics g, double fromTime, double toTime,
	double fromErb, double toErb, double minimum, double maximum, int enhanced,
	int garnish)
{
	long i, j; double **z = enhanced ? my s : my y;
	Matrix thee = Matrix_create (my xmin, my xmax, my nx, my dx, my x1,
		my ymin, my ymax, my ny, my dy, my y1);
	if (! thee) return;
	for (j=1; j <= my ny; j++) for (i=1; i <= my nx; i++)
		thy z[j][i] = z[j][i]; // > 0 ? 10 * log10 (z[i][j] / 2e-5) : 0;
	Matrix_paintCells (thee, g, fromTime, toTime, fromErb, toErb, minimum, maximum);
	if (garnish)
	{
		Graphics_drawInnerBox(g);
		Graphics_textBottom (g, 1, L"Time (s)");
		Graphics_marksBottom( g, 2, 1, 1, 0);
		Graphics_textLeft (g, 1, L"Frequency (ERB)");
		Graphics_marksLeft( g, 2, 1, 1, 0);
		Graphics_textTop (g, 0, enhanced ? L"Cooperative interaction output" : 
			L"Gammatone filterbank output");
	}
	forget (thee);
}

void SPINET_drawSpectrum (SPINET me, Graphics g, double time, double fromErb, double toErb,
	double minimum, double maximum, int enhanced, int garnish)
{
	long i, ifmin, ifmax, icol = Sampled2_xToColumn (me, time);
	double **z = enhanced ? my s : my y; double *spec;
	if (icol < 1 || icol > my nx) return;
	if (toErb <= fromErb) { fromErb = my ymin; toErb = my ymax; }
	if (! Sampled2_getWindowSamplesY (me, fromErb, toErb, &ifmin, &ifmax) ||
		! (spec = NUMdvector (1, my ny))) return;
		
	for (i=1; i <= my ny; i++) spec[i] = z[i][icol];
	if (maximum <= minimum) NUMdvector_extrema (spec, ifmin, ifmax, &minimum, &maximum);
	if (maximum <= minimum) { minimum -= 1; maximum += 1; }
	for (i=ifmin; i <= ifmax; i++)
		if (spec[i] > maximum) spec[i] = maximum;
		else if (spec[i] < minimum) spec[i] = minimum;
	Graphics_setInner (g);
	Graphics_setWindow (g, fromErb, toErb, minimum, maximum);
	Graphics_function (g, spec, ifmin, ifmax,
		Sampled2_rowToY (me, ifmin), Sampled2_rowToY (me, ifmax));
	Graphics_unsetInner (g);
	if (garnish)
	{
		Graphics_drawInnerBox(g);
		Graphics_textBottom (g, 1, L"Frequency (ERB)");
		Graphics_marksBottom( g, 2, 1, 1, 0);
		Graphics_textLeft (g, 1, L"strength");
		Graphics_marksLeft( g, 2, 1, 1, 0);
	}
	NUMdvector_free (spec, 1);
}*/

FORM (Sound_to_Pitch_SPINET, L"Sound: To SPINET", L"Sound: To SPINET...")
	POSITIVE (L"Time step (s)", L"0.005")
	POSITIVE (L"Window length (s)", L"0.040")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Gammatone filter bank")
	POSITIVE (L"Minimum filter frequency (Hz)", L"70.0")
	POSITIVE (L"Maximum filter frequency (Hz)", L"5000.0")
	NATURAL (L"Number of filters", L"250");
	POSITIVE (L"Ceiling (Hz)", L"500.0")
	NATURAL (L"Max. number of candidates", L"15")
	OK
DO
	double fmin = GET_REAL (L"Minimum filter frequency");
	double fmax = GET_REAL (L"Maximum filter frequency");
	REQUIRE (fmax > fmin, L"Maximum frequency must be larger than minimum frequency.")
	EVERY_TO (Sound_to_Pitch_SPINET ((structSound *)OBJECT, GET_REAL (L"Time step"),
		GET_REAL (L"Window length"),
		fmin, fmax, GET_INTEGER (L"Number of filters"),
		GET_REAL (L"Ceiling"), GET_INTEGER (L"Max. number of candidates")))
END

FORM (Sound_to_Polygon, L"Sound: To Polygon", L"Sound: To Polygon...")
	OPTIONMENU (L"Channel", 1)
		OPTION (L"Left")
		OPTION (L"Right")
	REAL (L"left Time range (s)", L"0.0")
	REAL (L"right Time range (s)", L"0.0")
	REAL (L"left Vertical range", L"0.0")
	REAL (L"right Vertical range", L"0.0")
	REAL (L"Connect at", L"0.0")
	OK
DO
	Sound me = (structSound *)ONLY (classSound);
	long channel = GET_INTEGER (L"Channel");
	if (channel > my ny) channel = 1;
	NEW (Sound_to_Polygon (me, channel, GET_REAL (L"left Time range"), GET_REAL (L"right Time range"),
		GET_REAL (L"left Vertical range"), GET_REAL (L"right Vertical range"), GET_REAL (L"Connect at")))
END

FORM (Sounds_to_Polygon_enclosed, L"Sounds: To Polygon (enclosed)", L"Sounds: To Polygon (enclosed)...")
	OPTIONMENU (L"Channel", 1)
		OPTION (L"Left")
		OPTION (L"Right")
	REAL (L"left Time range (s)", L"0.0")
	REAL (L"right Time range (s)", L"0.0")
	REAL (L"left Vertical range", L"0.0")
	REAL (L"right Vertical range", L"0.0")
	OK
DO
	Sound s1 = NULL, s2 = NULL;
	WHERE (SELECTED)
	{
		if (s1) s2 = (structSound *)OBJECT; else s1 = (structSound *)OBJECT;
	}
	long channel = GET_INTEGER (L"Channel");
	NEW (Sounds_to_Polygon_enclosed (s1, s2, channel, GET_REAL (L"left Time range"), GET_REAL (L"right Time range"),
		GET_REAL (L"left Vertical range"), GET_REAL (L"right Vertical range")))
END

FORM (Sound_filterByGammaToneFilter4, L"Sound: Filter (gammatone)", L"Sound: Filter (gammatone)...")
	POSITIVE (L"Centre frequency (Hz)", L"1000.0")
	POSITIVE (L"Bandwidth (Hz)", L"150.0")
	OK
DO
	EVERY_TO (Sound_filterByGammaToneFilter4 ((structSound *)OBJECT,
		GET_REAL (L"Centre frequency"), GET_REAL (L"Bandwidth")))
END

FORM (Sound_changeSpeaker, L"Sound: Change speaker", L"Sound: Change speaker...")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Pitch measurement parameters")
	POSITIVE (L"Pitch floor (Hz)", L"75.0")
	POSITIVE (L"Pitch ceiling (Hz)", L"600.0")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Modification parameters")
	POSITIVE (L"Multiply formants by", L"1.2")
	POSITIVE (L"Multiply pitch by", L"1.0")
	REAL (L"Multiply pitch range by", L"1.0 (=no change)")
	POSITIVE (L"Multiply duration by", L"1.0")
	OK
DO
	double minimumPitch = GET_REAL (L"Pitch floor");
	double maximumPitch = GET_REAL (L"Pitch ceiling");
	REQUIRE (minimumPitch < maximumPitch, L"Maximum pitch should be greater than minimum pitch.")
	EVERY_TO (Sound_changeSpeaker ((structSound *)OBJECT, minimumPitch, maximumPitch,
		GET_REAL (L"Multiply formants by"), GET_REAL (L"Multiply pitch by"),
		GET_REAL (L"Multiply pitch range by"), GET_REAL (L"Multiply duration by")))
END

FORM (Sound_changeGender, L"Sound: Change gender", L"Sound: Change gender...")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Pitch measurement parameters")
	POSITIVE (L"Pitch floor (Hz)", L"75.0")
	POSITIVE (L"Pitch ceiling (Hz)", L"600.0")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Modification parameters")
	POSITIVE (L"Formant shift ratio", L"1.2")
	REAL (L"New pitch median (Hz)", L"0.0 (=no change)")
	REAL (L"Pitch range factor", L"1.0 (=no change)")
	POSITIVE (L"Duration factor", L"1.0")
	OK
DO
	double minimumPitch = GET_REAL (L"Pitch floor");
	double maximumPitch = GET_REAL (L"Pitch ceiling");
	double pitchrf = GET_REAL (L"Pitch range factor");
	REQUIRE (minimumPitch < maximumPitch, L"Maximum pitch should be greater than minimum pitch.")
	REQUIRE (pitchrf >= 0, L"Pitch range factor may not be negative")
	EVERY_TO (Sound_changeGender_old ((structSound *)OBJECT, minimumPitch, maximumPitch,
		GET_REAL (L"Formant shift ratio"), GET_REAL (L"New pitch median"),
		pitchrf, GET_REAL (L"Duration factor")))
END

FORM (Sound_paintWhere, L"Sound paint where", L"Sound: Paint where...")
	COLOUR (L"Colour (0-1, name, or {r,g,b})", L"0.5")
	REAL (L"left Time range (s)", L"0.0")
	REAL (L"right Time range (s)", L"0.0")
	REAL (L"left Vertical range", L"0.0")
	REAL (L"right Vertical range", L"0.0")
	REAL (L"Fill from level", L"0.0")
	BOOLEAN (L"Garnish", 1)
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Paint only those parts where the following condition holds:")
	TEXTFIELD (L"Formula", L"1; always")
	OK
DO
	long numberOfBisections = 10;
	EVERY_DRAW (Sound_paintWhere ((structSound *)OBJECT, GRAPHICS, GET_COLOUR (L"Colour"),
		GET_REAL (L"left Time range"), GET_REAL (L"right Time range"),
		GET_REAL (L"left Vertical range"), GET_REAL (L"right Vertical range"), GET_REAL (L"Fill from level"),
		GET_INTEGER (L"Garnish"), numberOfBisections, GET_STRING (L"Formula"), interpreter))
END

FORM (Sounds_paintEnclosed, L"Sounds paint enclosed", L"Sounds: Paint enclosed...")
	COLOUR (L"Colour (0-1, name, or {r,g,b})", L"0.5")
	REAL (L"left Time range (s)", L"0.0")
	REAL (L"right Time range (s)", L"0.0")
	REAL (L"left Vertical range", L"0.0")
	REAL (L"right Vertical range", L"0.0")
	BOOLEAN (L"Garnish", 1)
	OK
DO
	Sound s1 = NULL, s2 = NULL;
	WHERE (SELECTED) { if (s1) s2 = (structSound *)OBJECT; else s1 = (structSound *)OBJECT; }
	EVERY_DRAW (Sounds_paintEnclosed (s1, s2, GRAPHICS, GET_COLOUR (L"Colour"),
		GET_REAL (L"left Time range"), GET_REAL (L"right Time range"),
		GET_REAL (L"left Vertical range"), GET_REAL (L"right Vertical range"), GET_INTEGER (L"Garnish")))
END

FORM_READ (Sound_readFromRawFileLE, L"Read Sound from raw Little Endian file", 0, true)
	if (! praat_new1 (Sound_readFromRawFile (file, NULL, 16, 1, 0, 0,
		16000), MelderFile_name (file))) return 0;
END

FORM_READ (Sound_readFromRawFileBE, L"Read Sound from raw 16-bit Little Endian file", 0, true)
	if (! praat_new1 (Sound_readFromRawFile (file, NULL, 16, 0, 0, 0,
		16000), MelderFile_name (file))) return 0;
END

FORM_READ (KlattTable_readFromRawTextFile, L"KlattTable_readFromRawTextFile", 0, true)
	if (! praat_new1 (KlattTable_readFromRawTextFile (file), MelderFile_name (file))) return 0;
END

FORM_WRITE (Sound_writeToRawFileBE, L"Sound: Save as raw 16-bit Big Endian file", 0, L"raw")
	if (! Sound_writeToRawFile ((structSound *)ONLY_OBJECT, file, 0, 0, 16, 0)) return 0;
END

FORM_WRITE (Sound_writeToRawFileLE, L"Sound: Save as raw 16-bit Little Endian file", 0, L"raw")
	if (! Sound_writeToRawFile ((structSound *)ONLY_OBJECT, file, 0, 1, 16, 0)) return 0;
END

/************ Spectrograms *********************************************/

FORM (Spectrograms_to_DTW, L"Spectrograms: To DTW", 0)
	DTW_constraints_addCommonFields (dia);
	OK
DO
	Spectrogram s1 = NULL, s2 = NULL;
	int begin, end, slope;
	DTW_constraints_getCommonFields (dia, &begin, &end, &slope);
	WHERE (SELECTED && CLASS == classSpectrogram)
	{
		if (s1) s2 = (structSpectrogram *)OBJECT; else s1 = (structSpectrogram *)OBJECT;
	}
	NEW (Spectrograms_to_DTW (s1, s2, begin, end, slope, 1))
END

/**************** Spectrum *******************************************/

void Spectrum_drawPhases (Spectrum me, Graphics g, double fmin, double fmax,
	double phase_min, double phase_max, int unwrap, int garnish)
{
	Matrix thee; long i;
	int reverse_sign = my z[1][1] < 0;

	if (unwrap)
	{
		thee = Spectrum_unwrap (me);
		if (thee == NULL)
		{
		    Melder_warning1 (L"Spectrum_drawPhases: Spectrum has not been unwrapped.");
		    return;
		}
	}
	else
	{
		if ((thee = Matrix_create (my xmin, my xmax, my nx, my dx, my x1,
			1, 2, 2, 1, 1)) == NULL) return;
		for (i = 1; i <= my nx; i ++)
		{
			thy z[2][i] = PPVPHA (my z[1][i], my z[2][i], reverse_sign);
		}
	}
	
	Matrix_drawRows (thee, g, fmin, fmax, 1.9, 2.1, phase_min, phase_max);
	if (garnish)
	{
	
	}	

	forget (thee);
}

FORM (Spectrum_drawPhases, L"Spectrum: Draw phases", L"Spectrum: Draw phases...")
	REAL (L"From frequency (Hz)", L"0.0")
	REAL (L"To frequency (Hz)", L"0.0")
	REAL (L"Minimum phase (dB/Hz)", L"0.0 (= auto)")
	REAL (L"Maximum phase (dB/Hz)", L"0.0 (= auto)")
	BOOLEAN (L"Unwrap", 1)
	BOOLEAN (L"Garnish", 1)
	OK
DO
	EVERY_DRAW (Spectrum_drawPhases ((structSpectrum *)OBJECT, GRAPHICS,
		GET_REAL (L"From frequency"), GET_REAL (L"To frequency"),
		GET_REAL (L"Minimum phase"), GET_REAL (L"Maximum phase"),
		GET_INTEGER (L"Unwrap"), GET_INTEGER (L"Garnish")))
END

DIRECT (Spectrum_conjugate)
	WHERE (SELECTED)
	{
		Spectrum_conjugate ((structSpectrum *)OBJECT);
		praat_dataChanged (OBJECT);
	}
END

DIRECT (Spectra_multiply)
	Spectrum s1 = NULL, s2 = NULL;
	WHERE (SELECTED && CLASS == classSpectrum)
	{
		if (s1) s2 = (structSpectrum *)OBJECT; else s1 = (structSpectrum *)OBJECT;
	}
	NEW (Spectra_multiply (s1, s2))
END

DIRECT (Spectrum_unwrap)
	EVERY_TO (Spectrum_unwrap ((structSpectrum *)OBJECT))
END

DIRECT (Spectrum_to_Cepstrum)
	EVERY_TO (Spectrum_to_Cepstrum ((structSpectrum *)OBJECT))
END

/************* Spline *************************************************/

void Spline_drawKnots (I, Graphics g, double xmin, double xmax, double ymin, 
	double ymax, int garnish)
{
	iam (Spline);
	long i, order = Spline_getOrder (me);
	double x1, x2;
	wchar_t ts[20] = L"";
	
	if (xmax <= xmin)
	{
		xmin = my xmin; xmax = my xmax;
	}
	
	if (xmax < my xmin || xmin > my xmax) return;
	
	if (ymax <= ymin)
	{
		FunctionTerms_getExtrema (me, xmin, xmax, &x1, &ymin, &x2, &ymax);
	}
	
	Graphics_setWindow (g, xmin, xmax, ymin, ymax);
	
	if (my knots[1] >= xmin && my knots[1] <= xmax)
	{
		if (garnish)
		{
    		if (order == 1) swprintf (ts, 20, L"t__1_");
			else if (order == 2) swprintf (ts, 20, L"{t__1_, t__2_}");
			else swprintf (ts, 20, L"{t__1_..t__%ld_}", order);
		}
		Graphics_markTop (g, my knots[1], 0, 1, 1, ts);
	}
	for (i=2; i <= my numberOfKnots - 1; i++)
	{
		if (my knots[i] >= xmin && my knots[i] <= xmax)
		{
			if (garnish) swprintf (ts, 20, L"t__%ld_", i + order - 1);
			Graphics_markTop (g, my knots[i], 0, 1, 1, ts); 
		}
	}
	if (my knots[my numberOfKnots] >= xmin &&
		my knots[my numberOfKnots] <= xmax)
	{
		if (garnish)
		{
			long numberOfKnots = my numberOfKnots + 2 * (order - 1);
    		if (order == 1)
			{
				swprintf (ts, 20, L"t__%ld_", numberOfKnots);
			}
			else if (order == 2)
			{
				swprintf (ts, 20, L"{t__%d_, t__%ld_}", numberOfKnots - 1,
					numberOfKnots);
			}
			else
			{
				swprintf (ts, 20, L"{t__%d_..t__%ld_}", numberOfKnots - order + 1,
					numberOfKnots);
			}
		}
		Graphics_markTop (g, my knots[my numberOfKnots], 0, 1, 1, ts);
	}
}

FORM (Spline_drawKnots, L"Spline: Draw knots", 0)
	REAL (L"Xmin", L"0.0")
	REAL (L"Xmax", L"0.0")
	REAL (L"left Vertical range", L"0.0")
	REAL (L"right Vertical range", L"0.0")
	BOOLEAN (L"Garnish", 1)
	OK
DO
	EVERY_DRAW (Spline_drawKnots ((structSpline *)OBJECT, GRAPHICS,
		GET_REAL (L"Xmin"), GET_REAL (L"Xmax"),
		GET_REAL (L"left Vertical range"), GET_REAL (L"right Vertical range"),
		GET_INTEGER (L"Garnish")))
END

DIRECT (Spline_getOrder)
	Melder_information1 (Melder_integer (Spline_getOrder ((structSpline *)ONLY_OBJECT)));
END

FORM (Spline_scaleX, L"Spline: Scale x", L"Spline: Scale x...")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"New domain")
	REAL (L"Xmin", L"-1.0")
	REAL (L"Xmax", L"1.0")
	OK
DO
	double xmin = GET_REAL (L"Xmin"), xmax = GET_REAL (L"Xmax");
	REQUIRE (xmin < xmax, L"Xmin must be smaller than Xmax.")
	EVERY_TO (Spline_scaleX ((structSpline *)OBJECT, xmin, xmax))
END

/************ SSCP ***************************************************/

static void _SSCP_drawTwoDimensionalEllipse (SSCP me, Graphics g, double scale,
	int fontSize)
{
	double a, angle, b, cs, sn, twoPi = 2 * NUMpi;
	long nsteps = 100;
	double angle_inc = twoPi / nsteps;
	double *x = NULL, *y = NULL; long i;
	wchar_t *name;

	x = NUMdvector (0, nsteps);
	if (x == NULL) return;
	y = NUMdvector (0, nsteps);
	if (y == NULL) goto end;

	/*
		Get principal axes and orientation for the ellipse by performing the
		eigen decomposition of a symmetric 2-by-2 matrix.
		Principal axes are a and b with eigenvector/orientation (cs, sn).
	*/

	NUMeigencmp22 (my data[1][1], my data[1][2], my data[2][2], &a, &b, &cs, &sn);

	/*
		1. Take sqrt to get units of 'std_dev'
	*/

	a = scale * sqrt (a) / 2; b = scale * sqrt (b) / 2;
	x[nsteps] = x[0] = my centroid[1] + cs * a;
	y[nsteps] = y[0] = my centroid[2] + sn * a;
	for (angle = 0, i = 1; i < nsteps; i++, angle += angle_inc)
	{
		double xc = a * cos (angle);
		double yc = b * sin (angle);
		double xt = xc * cs - yc * sn;
		y[i] = my centroid[2] + xc * sn + yc * cs;
		x[i] = my centroid[1] + xt;
	}
	Graphics_polyline (g, nsteps + 1, x, y);
	if (fontSize > 0 && (name = Thing_getName (me)))
	{
		int oldFontSize = Graphics_inqFontSize (g);
		Graphics_setFontSize (g, fontSize);
		Graphics_setTextAlignment (g, Graphics_CENTRE, Graphics_HALF);
		Graphics_text (g, my centroid[1], my centroid[2], name);
		Graphics_setFontSize (g, oldFontSize);
	}
end:
	NUMdvector_free (x, 0);
	NUMdvector_free (y, 0);
}

/*
void SSCP_and_TableOfReal_drawMahalanobisDistances (I, thou, Graphics g, long rowb, long rowe, double ymin,
	double ymax, double chiSqFraction, int garnish)
{
}*/

void SSCP_drawConcentrationEllipse (SSCP me, Graphics g, double scale,
	int confidence, long d1, long d2, double xmin, double xmax,
	double ymin, double ymax, int garnish)
{
	SSCP thee;
	double xmn, xmx, ymn, ymx;
	long p = my numberOfColumns;

	if (d1 < 1 || d1 > p || d2 < 1 || d2 > p || d1 == d2) return;

	if (! (thee = SSCP_extractTwoDimensions (me, d1, d2))) return;

	SSCP_getEllipseBoundingBoxCoordinates (thee, scale, confidence, &xmn, &xmx, &ymn, &ymx);

	if (xmax == xmin)
	{
		xmin = xmn; xmax = xmx;
	}

	if (ymax == ymin)
	{
		ymin = ymn; ymax = ymx;
	}

	Graphics_setWindow (g, xmin, xmax, ymin, ymax);
	Graphics_setInner (g);

	scale = SSCP_ellipseScalefactor (thee, scale, confidence);
	if (scale < 0) return;
	_SSCP_drawTwoDimensionalEllipse (thee, g, scale, 0);

	Graphics_unsetInner (g);
	if (garnish)
	{
		Graphics_drawInnerBox (g);
    	Graphics_marksLeft (g, 2, 1, 1, 0);
		Graphics_marksBottom (g, 2, 1, 1, 0);
	}
	forget (thee);
}

void SSCPs_drawConcentrationEllipses (SSCPs me, Graphics g, double scale,
	int confidence, wchar_t *label, long d1, long d2, double xmin, double xmax,
	double ymin, double ymax, int fontSize, int garnish)
{
	SSCPs thee;
	SSCP t = (structSSCP *)my item[1];
	double xmn, xmx, ymn, ymx;
	long i, p = t -> numberOfColumns;

	if (d1 < 1 || d1 > p || d2 < 1 || d2 > p || d1 == d2) return;

	if (! (thee = SSCPs_extractTwoDimensions (me, d1, d2))) return;
	SSCPs_getEllipsesBoundingBoxCoordinates (thee, scale, confidence, &xmn, &xmx, &ymn, &ymx);

	if (xmin == xmax)
	{
		xmin = xmn; xmax = xmx;
	}

	if (ymin == ymax)
	{
		ymin = ymn; ymax = ymx;
	}

	Graphics_setWindow (g, xmin, xmax, ymin, ymax);
	Graphics_setInner (g);


	for (i = 1; i <= thy size; i++)
	{
		double lscale;
		t = (structSSCP *)thy item[i];
		lscale = SSCP_ellipseScalefactor (t, scale, confidence);
		if (lscale < 0) continue;
		if (label == NULL || Melder_wcscmp (label, Thing_getName (t)) == 0)
		{
			_SSCP_drawTwoDimensionalEllipse (t, g, lscale, fontSize);
		}
	}

	Graphics_unsetInner (g);
	if (garnish)
	{
		wchar_t text[20];
		t = (structSSCP *)my item[1];
    	Graphics_drawInnerBox (g);
    	Graphics_marksLeft (g, 2, 1, 1, 0);
		swprintf (text, 20, L"Dimension %ld", d2);
    	Graphics_textLeft (g, 1, t -> columnLabels[d2] ? t -> columnLabels[d2] : text);
    	Graphics_marksBottom (g, 2, 1, 1, 0);
		swprintf (text, 20, L"Dimension %ld", d1);
		Graphics_textBottom (g, 1, t -> columnLabels[d1] ? t -> columnLabels[d1] : text);
	}
	forget (thee);
}

DIRECT (SSCP_help) Melder_help (L"SSCP"); END

FORM (SSCP_drawConfidenceEllipse, L"SSCP: Draw confidence ellipse", 0)
	POSITIVE (L"Confidence level", L"0.95")
	NATURAL (L"Index for X-axis", L"1")
	NATURAL (L"Index for Y-axis", L"2")
	REAL (L"left Horizontal range", L"0.0")
	REAL (L"right Horizontal range", L"0.0")
	REAL (L"left Vertical range", L"0.0")
	REAL (L"right Vertical range", L"0.0")
	BOOLEAN (L"Garnish", 1)
	OK
DO
	EVERY_DRAW (SSCP_drawConcentrationEllipse ((structSSCP *)OBJECT, GRAPHICS,
		GET_REAL (L"Confidence level"), 1,
		GET_INTEGER (L"Index for X-axis"), GET_INTEGER (L"Index for Y-axis"),
		GET_REAL (L"left Horizontal range"), GET_REAL (L"right Horizontal range"), GET_REAL (L"left Vertical range"),
		GET_REAL (L"right Vertical range"), GET_INTEGER (L"Garnish")))
END

FORM (SSCP_drawSigmaEllipse, L"SSCP: Draw sigma ellipse", L"SSCP: Draw sigma ellipse...")
	POSITIVE (L"Number of sigmas", L"1.0")
	NATURAL (L"Index for X-axis", L"1")
	NATURAL (L"Index for Y-axis", L"2")
	REAL (L"left Horizontal range", L"0.0")
	REAL (L"right Horizontal range", L"0.0")
	REAL (L"left Vertical range", L"0.0")
	REAL (L"right Vertical range", L"0.0")
	BOOLEAN (L"Garnish", 1)
	OK
DO
	EVERY_DRAW (SSCP_drawConcentrationEllipse ((structSSCP *)OBJECT, GRAPHICS,
		GET_REAL (L"Number of sigmas"), 0,
		GET_INTEGER (L"Index for X-axis"), GET_INTEGER (L"Index for Y-axis"),
		GET_REAL (L"left Horizontal range"), GET_REAL (L"right Horizontal range"),
		GET_REAL (L"left Vertical range"), GET_REAL (L"right Vertical range"), GET_INTEGER (L"Garnish")))
END

DIRECT (SSCP_extractCentroid)
	EVERY_CHECK (praat_new2 (SSCP_extractCentroid ((structSSCP *)OBJECT),
		Thing_getName (OBJECT), L"_centroid"))
END

FORM (SSCP_getConfidenceEllipseArea, L"SSCP: Get confidence ellipse area", L"SSCP: Get confidence ellipse area...")
	POSITIVE (L"Confidence level", L"0.95")
	NATURAL (L"Index for X-axis", L"1")
	NATURAL (L"Index for Y-axis", L"2")
	OK
DO
	double conf = GET_REAL (L"Confidence level");
	long d1 = GET_INTEGER (L"Index for X-axis");
	long d2 = GET_INTEGER (L"Index for Y-axis");
	Melder_information1 (Melder_double (SSCP_getConcentrationEllipseArea ((structSSCP *)ONLY_OBJECT, conf, 1, d1, d2)));
END

FORM (SSCP_getFractionVariation, L"SSCP: Get fraction variation", L"SSCP: Get fraction variation...")
	NATURAL (L"From dimension", L"1")
	NATURAL (L"To dimension", L"1")
	OK
DO
	Melder_information1 (Melder_double (SSCP_getFractionVariation ((structSSCP *)ONLY_OBJECT,
		GET_INTEGER (L"From dimension"), GET_INTEGER (L"To dimension"))));
END


FORM (SSCP_getConcentrationEllipseArea, L"SSCP: Get sigma ellipse area", L"SSCP: Get sigma ellipse area...")
	POSITIVE (L"Number of sigmas", L"1.0")
	NATURAL (L"Index for X-axis", L"1")
	NATURAL (L"Index for Y-axis", L"2")
	OK
DO
	double nsigmas = GET_REAL (L"Number of sigmas");
	long d1 = GET_INTEGER (L"Index for X-axis");
	long d2 = GET_INTEGER (L"Index for Y-axis");
	Melder_information1 (Melder_double (SSCP_getConcentrationEllipseArea ((structSSCP *)ONLY_OBJECT,
		nsigmas, 0, d1, d2)));
END

DIRECT (SSCP_getDegreesOfFreedom)
	Melder_information1 (Melder_double (SSCP_getDegreesOfFreedom ((structSSCP *)ONLY_OBJECT)));
END

DIRECT (SSCP_getNumberOfObservations)
	SSCP me = (structSSCP *)ONLY_OBJECT;
	Melder_information1 (Melder_integer (my numberOfObservations));
END

DIRECT (SSCP_getTotalVariance)
	Melder_information1 (Melder_double (SSCP_getTotalVariance ((structSSCP *)ONLY_OBJECT)));
END

FORM (SSCP_getCentroidElement, L"SSCP: Get centroid element", L"SSCP: Get centroid element")
	NATURAL (L"Number", L"1")
	OK
DO
	SSCP me = (structSSCP *)ONLY_OBJECT;
	long number = GET_INTEGER (L"Number");
	if (number < 1 || number > my numberOfColumns)
	{
		return Melder_error3 (L"SSCP_getCentroidElement: \"Number\" must be smaller than ", Melder_integer (my numberOfColumns + 1), L".");
	}
	Melder_information1 (Melder_double (my centroid[number]));
END

DIRECT (SSCP_getLnDeterminant)
	Melder_information1 (Melder_double (SSCP_getLnDeterminant ((structSSCP *)ONLY_OBJECT)));
END

FORM (SSCP_testDiagonality_bartlett, L"SSCP: Get diagonality (bartlett)", L"SSCP: Get diagonality (bartlett)...")
	NATURAL (L"Number of contraints", L"1")
	OK
DO
	double chisq, p;
	long nc = GET_INTEGER (L"Number of contraints");
	SSCP me = (structSSCP *)ONLY_OBJECT;
	SSCP_testDiagonality_bartlett (me, nc, &chisq, &p);
	Melder_information6 (Melder_double (p), L" (=probability for chisq = ", Melder_double (chisq), L" and ndf = ",
		Melder_integer (my numberOfRows * (my numberOfRows - 1) / 2), L")");
END

DIRECT (SSCP_to_Correlation)
	EVERY_TO (SSCP_to_Correlation ((structSSCP *)OBJECT))
END

FORM (SSCP_to_Covariance, L"SSCP: To Covariance", L"SSCP: To Covariance...")
	NATURAL (L"Number of constraints", L"1")
	OK
DO
	long noc = GET_INTEGER (L"Number of constraints");
	EVERY_TO (SSCP_to_Covariance ((structSSCP *)OBJECT, noc))
END

DIRECT (SSCP_to_PCA)
	EVERY_TO (SSCP_to_PCA ((structSSCP *)OBJECT))
END

/******************* Strings ****************************/

DIRECT (Strings_append)
	int status = 0;
	Ordered me = Ordered_create ();
	if (! me) return 0;
	WHERE (SELECTED)
	{
		if (! Collection_addItem (me, OBJECT)) goto Strings_append_end;
	}
	if (praat_new1 (Strings_append (me), L"appended")) status = 1;
Strings_append_end:
	my size = 0; forget (me);
	return status;
END

DIRECT (Strings_to_Categories)
	EVERY_TO (Strings_to_Categories ((structStrings *)OBJECT))
END

FORM (Strings_setString, L"Strings: Set string", L"Strings: Set string...")
	NATURAL (L"Index", L"1")
	SENTENCE (L"String", L"ui/editors/AmplitudeTierEditor.h")
	OK
DO
	if (! Strings_setString ((structStrings *)ONLY (classStrings), GET_STRING (L"String"),
		GET_INTEGER (L"Index"))) return 0;
END

FORM (Strings_change, L"Strings: Change", L"Strings: Change")
	SENTENCE (L"Search", L"a")
	SENTENCE (L"Replace", L"a")
	INTEGER (L"Replace limit", L"0 (=unlimited)")
	RADIO (L"Search and replace are:", 1)
	RADIOBUTTON (L"Literals")
	RADIOBUTTON (L"Regular Expressions")
	OK
DO
	long nmatches, nstringmatches;
	EVERY_TO (Strings_change ((structStrings *)OBJECT, GET_STRING (L"Search"),
		GET_STRING (L"Replace"), GET_INTEGER (L"Replace limit"), &nmatches,
		&nstringmatches, GET_INTEGER (L"Search and replace are") - 1))
END

FORM (Strings_extractPart, L"Strings: Extract part", L"ui/editors/AmplitudeTierEditor.h")
	NATURAL (L"From index", L"1")
	NATURAL (L"To index", L"1")
	OK
DO
	EVERY_TO (Strings_extractPart ((structStrings *)OBJECT, GET_INTEGER (L"From index"), GET_INTEGER (L"To index")))
END

FORM (Strings_to_Permutation, L"Strings: To Permutation", L"Strings: To Permutation...")
	BOOLEAN (L"Sort", 1)
	OK
DO
	EVERY_TO (Strings_to_Permutation ((structStrings *)OBJECT, GET_INTEGER (L"Sort")))
END

DIRECT (Strings_and_Permutation_permuteStrings)
	NEW (Strings_and_Permutation_permuteStrings ((structStrings *)ONLY (classStrings), (structPermutation *)ONLY (classPermutation)))
END

FORM (SVD_to_TableOfReal, L"SVD: To TableOfReal", L"SVD: To TableOfReal...")
	NATURAL (L"First component", L"1")
	INTEGER (L"Last component", L"0 (=all)")
	OK
DO
	EVERY_TO (SVD_to_TableOfReal ((structSVD *)OBJECT, GET_INTEGER (L"First component"),
		GET_INTEGER (L"Last component")))
END

DIRECT (SVD_extractLeftSingularVectors)
	SVD svd = (structSVD *)ONLY (classSVD);
	if (! praat_new2 (SVD_extractLeftSingularVectors (svd), Thing_getName (svd), L"_lsv")) return 0;
END

DIRECT (SVD_extractRightSingularVectors)
	SVD svd = (structSVD *)ONLY (classSVD);
	if (! praat_new2 (SVD_extractRightSingularVectors (svd), Thing_getName (svd), L"_rsv")) return 0;
END

DIRECT (SVD_extractSingularValues)
	SVD svd = (structSVD *)ONLY (classSVD);
	if (! praat_new2 (SVD_extractSingularValues (svd), Thing_getName (svd), L"_sv")) return 0;
END

/******************* Table ****************************/

void Table_drawScatterPlotWithConfidenceIntervals (Table me, Graphics g, long xcolumn, long ycolumn,
	double xmin, double xmax, double ymin, double ymax, long xci_min, long xci_max,
	long yci_min, long yci_max, double bar_mm, int garnish)
{
	long nrows = my rows -> size;
	double x2min, x1max, y1max, y2min;
	double bar = ceil (bar_mm * g -> resolution / 25.4);
	
	// check validity of columns
	if (xcolumn < 1 || xcolumn > nrows || ycolumn < 1 || ycolumn > nrows) return;
	if (labs (xci_min) > nrows || labs (xci_max) > nrows ||
		labs (yci_min) > nrows || labs (yci_max) > nrows) return;
	
	if (xmin >= xmax && 
		Table_getExtrema (me, xci_min, &xmin, &x1max) &&
		Table_getExtrema (me, xci_max, &x2min, &xmax) &&
		xmin >= xmax) return;
	
	if (ymin >= ymax && 
		Table_getExtrema (me, yci_min, &ymin, &y1max) &&
		Table_getExtrema (me, yci_max, &y2min, &ymax) &&
		ymin >= ymax) return;
	
    Graphics_setWindow (g, xmin, xmax, ymin, ymax);
    Graphics_setInner (g);

	for (long row = 1; row <= nrows; row++)
	{
		double x  = Table_getNumericValue_Assert (me, row, xcolumn);
		double y  = Table_getNumericValue_Assert (me, row, ycolumn);
		double x1 = Table_getNumericValue_Assert (me, row, xci_min);	
		double x2 = Table_getNumericValue_Assert (me, row, xci_max);
		double y1 = Table_getNumericValue_Assert (me, row, yci_min);	
		double y2 = Table_getNumericValue_Assert (me, row, yci_max);
		double xo1, yo1, xo2, yo2;
		
		if (xci_min > 0)
		{
			if (NUMclipLineWithinRectangle (x1, y, x, y, xmin, ymin, xmax, ymax,
				&xo1, &yo1, &xo2, &yo2)) Graphics_line (g, xo1, yo1, xo2, yo2);
			if (bar > 0 && NUMclipLineWithinRectangle (x1, y - bar/2, x1, y + bar/2, 
				xmin, ymin, xmax, ymax,	&xo1, &yo1, &xo2, &yo2)) Graphics_line (g, xo1, yo1, xo2, yo2);
		}
		if (xci_max > 0)
		{
			if (NUMclipLineWithinRectangle (x, y, x2, y, xmin, ymin, xmax, ymax,
				&xo1, &yo1, &xo2, &yo2)) Graphics_line (g, xo1, yo1, xo2, yo2);
			if (bar > 0 && NUMclipLineWithinRectangle (x2, y - bar/2, x2, y + bar/2, 
				xmin, ymin, xmax, ymax,	&xo1, &yo1, &xo2, &yo2)) Graphics_line (g, xo1, yo1, xo2, yo2);
		}
		if (yci_min > 0)
		{
			if (NUMclipLineWithinRectangle (x, y1, x, y, xmin, ymin, xmax, ymax,
				&xo1, &yo1, &xo2, &yo2)) Graphics_line (g, xo1, yo1, xo2, yo2);
			if (bar > 0 && NUMclipLineWithinRectangle (x - bar/2, y1, x + bar/2, y1, 
				xmin, ymin, xmax, ymax,	&xo1, &yo1, &xo2, &yo2)) Graphics_line (g, xo1, yo1, xo2, yo2);
		}
		if (yci_max > 0)
		{
			if (NUMclipLineWithinRectangle (x, y, x, y2, xmin, ymin, xmax, ymax,
				&xo1, &yo1, &xo2, &yo2)) Graphics_line (g, xo1, yo1, xo2, yo2);
			if (bar > 0 && NUMclipLineWithinRectangle (x - bar/2, y2, x + bar/2, y2, 
				xmin, ymin, xmax, ymax,	&xo1, &yo1, &xo2, &yo2)) Graphics_line (g, xo1, yo1, xo2, yo2);
		}
	}
	
    Graphics_unsetInner (g);
	
	if (garnish)
	{
		Graphics_drawInnerBox (g);
		Graphics_marksLeft (g, 2, 1, 1, 0);
		Graphics_marksBottom (g, 2, 1, 1, 0);
	}

}

DIRECT (Table_createFromPetersonBarneyData)
	if (! praat_new1 (Table_createFromPetersonBarneyData (), L"pb")) return 0;
END

DIRECT (Table_createFromPolsVanNieropData)
	if (! praat_new1 (Table_createFromPolsVanNieropData (), L"pvn")) return 0;
END

DIRECT (Table_createFromWeeninkData)
	if (! praat_new1 (Table_createFromWeeninkData (), L"m10w10c10")) return 0;
END

FORM (Table_drawScatterPlotWithConfidenceIntervals, L"Table: Scatter plot (confidence intervals)", L"ui/editors/AmplitudeTierEditor.h")
	NATURAL (L"Horizontal axis column", L"1")
	REAL (L"left Horizontal range", L"0.0")
	REAL (L"right Horizontal range", L"0.0")
	INTEGER (L"left Horizontal confidence interval column", L"3")
	INTEGER (L"right Horizontal confidence interval column", L"4")
	NATURAL (L"Vertical axis column", L"2")
	REAL (L"left Vertical range", L"0.0")
	REAL (L"right Vertical range", L"0.0")
	INTEGER (L"left Vertical confidence interval column", L"5")
	INTEGER (L"right Vertical confidence interval column", L"6")
	REAL (L"Bar size (mm)", L"1.0")
	BOOLEAN (L"Garnish", 1);
	OK
DO
	Table_drawScatterPlotWithConfidenceIntervals ((structTable *)ONLY (classTable), GRAPHICS,
		GET_INTEGER (L"Horizontal axis column"), GET_INTEGER (L"Vertical axis column"),
		GET_REAL (L"left Horizontal range"), GET_REAL (L"right Horizontal range"),
		GET_REAL (L"left Vertical range"), GET_REAL (L"right Vertical range"),
		GET_INTEGER (L"left Horizontal confidence interval column"), GET_INTEGER (L"right Horizontal confidence interval column"),
		GET_INTEGER (L"left Vertical confidence interval column"), GET_INTEGER (L"right Vertical confidence interval column"),
		GET_REAL (L"Bar size"), GET_INTEGER (L"Garnish"));
END

/******************* TableOfReal ****************************/

#define Graphics_ARROW 1
#define Graphics_TWOWAYARROW 2
#define Graphics_LINE 3

void TableOfReal_drawRowsAsHistogram (I, Graphics g, wchar_t *rows, long colb, long cole,
	double ymin, double ymax, double xoffsetFraction, double interbarFraction,
	double interbarsFraction, wchar_t *greys, int garnish)
{
	iam (TableOfReal);
	long i, j, irow, nrows, ncols, ngreys;
	double *irows, *igreys, grey, bar_width = 1, xb, dx, x1, x2, y1, y2;

	if (colb >= cole)
	{
		colb = 1; cole = my numberOfColumns;
	}
	if (colb <= cole && (colb < 1 || cole > my numberOfColumns))
	{
		    Melder_warning1 (L"Invalid columns");
			return;
	}

	irows = NUMstring_to_numbers (rows, &nrows);
	if (irows == NULL)
	{
		 Melder_warning1 (L"No rows!");
		 return;
	}

	for (i = 1; i <= nrows; i++)
	{
		irow = irows[i];
		if (irow < 0 || irow > my numberOfRows)
		{
			 Melder_warning3 (L"Invalid row (", Melder_integer (irow), L").");
			 goto end;
		}
		if (ymin >= ymax)
		{
			double min, max;
			NUMdvector_extrema (my data[irow], colb, cole, &min, &max);
			if (i > 1)
			{
				if (min < ymin) ymin = min;
				if (max > ymax) ymax = max;
			}
			else
			{
				ymin = min; ymax = max;
			}
		}
	}

	igreys = NUMstring_to_numbers (greys, &ngreys);
	if (igreys == NULL)
	{
		 Melder_warning1 (L"No greys!");
		 return;
	}

    Graphics_setWindow (g, 0, 1, ymin, ymax);
    Graphics_setInner (g);

	ncols = cole - colb + 1;
	bar_width /= ncols * nrows + 2 * xoffsetFraction + (ncols - 1) * interbarsFraction +
		ncols * (nrows -1) * interbarFraction;
    dx = (interbarsFraction + nrows + (nrows -1) * interbarFraction) * bar_width;

	for (i = 1; i <= nrows; i++)
	{
		irow = irows[i];
		xb = xoffsetFraction * bar_width + (i - 1) * (1 + interbarFraction) * bar_width;

		x1 = xb;
		grey = i <= ngreys ? igreys[i] : igreys[ngreys];
    	for (j = colb; j <= cole; j++)
    	{
			x2 = x1 + bar_width;
			y1 = ymin; y2 = my data[irow][j];
			if (y2 > ymin)
			{
				if (y2 > ymax) y2 = ymax;
				Graphics_setGrey (g, grey);
				Graphics_fillRectangle (g, x1, x2, y1, y2);
				Graphics_setGrey (g, 0); /* Black */
				Graphics_rectangle (g, x1, x2, y1, y2);
			}
			x1 += dx;
    	}
	}

    Graphics_unsetInner (g);

	if (garnish)
	{
		xb = (xoffsetFraction + 0.5 * (nrows + (nrows - 1) * interbarFraction)) * bar_width;
		for (j = colb; j <= cole; j++)
		{
			if (my columnLabels[j]) Graphics_markBottom (g, xb, 0, 0, 0, my columnLabels[j]);
			xb += dx;
		}
		Graphics_drawInnerBox (g);
		Graphics_marksLeft (g, 2, 1, 1, 0);
	}

end:

	NUMdvector_free (irows, 1);
}

void TableOfReal_drawBiplot (I, Graphics g, double xmin, double xmax,
	double ymin, double ymax, double sv_splitfactor, int labelsize, int garnish)
{
	iam (TableOfReal);
	SVD svd;
	double lambda1, lambda2;
	double *x = NULL, *y = NULL;
	long i, numberOfZeroed;
	long nr = my numberOfRows, nc = my numberOfColumns, nmin;
	long nPoints = nr + nc;
	int fontsize = Graphics_inqFontSize (g);

	svd = SVD_create (nr, nc);
	if (svd == NULL) goto end;

	NUMdmatrix_copyElements (my data, svd -> u, 1, nr, 1, nc);
	NUMcentreColumns (svd -> u, 1, nr, 1, nc, NULL);

	if (! SVD_compute (svd)) goto end;
	numberOfZeroed = SVD_zeroSmallSingularValues (svd, 0);

	nmin = MIN (nr, nc) - numberOfZeroed;
	if (nmin < 2)
	{
		Melder_warning1 (L" There must be at least two (independent) columns in the table.");
		goto end;
	}

	x = NUMdvector (1, nPoints);
	if (x == NULL) goto end;
	y = NUMdvector (1, nPoints);
	if (y == NULL) goto end;

	lambda1 = pow (svd -> d[1], sv_splitfactor);
	lambda2 = pow (svd -> d[2], sv_splitfactor);
	for (i = 1; i <= nr; i++)
	{
		x[i] = svd -> u[i][1] * lambda1;
		y[i] = svd -> u[i][2] * lambda2;
	}
	lambda1 = svd -> d[1] / lambda1;
	lambda2 = svd -> d[2] / lambda2;
	for (i = 1; i <= nc; i++)
	{
			x[nr + i] = svd -> v[i][1] * lambda1;
			y[nr + i] = svd -> v[i][2] * lambda2;
	}

	if (xmax <= xmin) NUMdvector_extrema (x, 1, nPoints, &xmin, &xmax);
	if (xmax <= xmin) { xmax += 1; xmin -= 1; }
	if (ymax <= ymin) NUMdvector_extrema (y, 1, nPoints, &ymin, &ymax);
	if (ymax <= ymin) { ymax += 1; ymin -= 1; }

    Graphics_setWindow (g, xmin, xmax, ymin, ymax);
    Graphics_setInner (g);
	if (labelsize > 0) Graphics_setFontSize (g, labelsize);
    Graphics_setTextAlignment (g, Graphics_CENTRE, Graphics_HALF);

	for (i = 1; i <= nPoints; i++)
	{
		wchar_t *label;
		if (i <= nr)
		{
			label = my rowLabels[i];
			if (label == NULL) label = L"?__r_";
		}
		else
		{
			label = my columnLabels[i - nr];
			if (label == NULL) label = L"?__c_";
		}
		Graphics_text (g, x[i], y[i], label);
	}

    Graphics_unsetInner (g);

	if (garnish)
	{
		Graphics_drawInnerBox (g);
		Graphics_marksLeft (g, 2, 1, 1, 0);
		Graphics_marksBottom (g, 2, 1, 1, 0);
	}

	if (labelsize > 0) Graphics_setFontSize (g, fontsize);

end:
	forget (svd);
	NUMdvector_free (x, 1);
	NUMdvector_free (y, 1);
}

/*
	Draw a box plot of data[1..ndata]. The vertical center line of the plot
	is at position 'x'. The rectangle box is 2*w wide, the whisker 2*r.
	All drawing outside [ymin, ymax] is clipped.
*/
static void Graphics_drawBoxPlot (Graphics g, double data[], long ndata,
	double x, double r, double w, double ymin, double ymax)
{
	double lowerOuterFence, lowerInnerFence, mean;
	double q75, q25, q50, upperInnerFence, upperOuterFence, hspread;
	double lowerWhisker, upperWhisker, y1, y2;
	int lineType = Graphics_inqLineType (g);
	long i, ie;

	Melder_assert (r > 0 && w > 0);
	if (ndata < 3) return;

	/*
		Sort the data (increasing: data[1] <= ... <= data[ndata]).
		Get the median (q50) and the upper and lower quartile points
		(q25 and q75).
		Now q25 and q75 are the lower and upper hinges, respectively.
		The fances can be calcultaed from q25 and q75.
		The spread is defined as the interquartile range or midrange
		|q75 - q25|.
		The fences are defined as:
		(lower/upper) innerfence = (lower/upper) hinge +/- 1.5 hspread
		(lower/upper) outerfence = (lower/upper) hinge +/- 3.0 hspread
	*/

	NUMsort_d (ndata, data);

	if (ymax <= ymin)
	{
		ymin = data[1]; ymax = data[ndata];
	}
	if (data[1] > ymax || data[ndata] < ymin) return;

	for (mean=0, i=1; i <= ndata; i++) mean += data[i];
	mean /= ndata;

	q25 = NUMquantile (ndata, data, 0.25);
	q50 = NUMquantile (ndata, data, 0.5);
	q75 = NUMquantile (ndata, data, 0.75);

	hspread = fabs (q75 - q25);
	lowerOuterFence = q25 - 3.0 * hspread;
	lowerInnerFence = q25 - 1.5 * hspread;
	upperInnerFence = q75 + 1.5 * hspread;
	upperOuterFence = q75 + 3.0 * hspread;

	/*
		Decide whether there are outliers that have to be drawn.
		First process data from below (data are sorted).
	*/

	i = 1; ie = ndata;
	while (i <= ie && data[i] < ymin) i++;
    Graphics_setTextAlignment (g, Graphics_CENTRE, Graphics_HALF);
	while (i <= ie && data[i] < lowerOuterFence)
	{
		Graphics_text (g, x, data[i], L"o"); i++;
	}
	while (i <= ie && data[i] < lowerInnerFence)
	{
		Graphics_text (g, x, data[i], L"*"); i++;
	}
	lowerWhisker = data[i] < q25 ? data[i] : lowerInnerFence;
	if (lowerWhisker > ymax) return;

	/*
		Next process data from above.
	*/

	i = ndata; ie = i;
	while (i >= ie && data[i] > ymax) i--;
	while (i >= ie && data[i] > upperOuterFence)
	{
		Graphics_text (g, x, data[i], L"o"); i--;
	}
	while (i >= ie && data[i] > upperInnerFence)
	{
		Graphics_text (g, x, data[i], L"*"); i--;
	}
	upperWhisker = data[i] > q75 ? data[i] : upperInnerFence;
	if (upperWhisker < ymin) return;

	/*
		Determine what parts of the "box" have to be drawn within the
		range [ymin, ymax].
		Horizontal lines first.
	*/

	y1 = lowerWhisker;
	if (ymax > y1 && ymin < y1)
		Graphics_line (g, x - r, y1, x + r, y1);
	y1 = q25;
	if (ymax > y1 && ymin < y1)
		Graphics_line (g, x - w, y1, x + w, y1);
	y1 = q50;
	if (ymax > y1 && ymin < y1)
		Graphics_line (g, x - w, y1, x + w, y1);
	y1 = q75;
	if (ymax > y1 && ymin < y1)
		Graphics_line (g, x - w, y1, x + w, y1);
	y1 = upperWhisker;
	if (ymax > y1 && ymin < y1)
		Graphics_line (g, x - r, y1, x + r, y1);

	/*
		Extension: draw the mean too.
	*/

	y1 = mean;
	if (ymax > y1 && ymin < y1)
	{
		Graphics_setLineType (g, Graphics_DOTTED);
		Graphics_line (g, x - w, y1, x + w, y1);
		Graphics_setLineType (g, lineType);
	}

	/*
		Now process the vertical lines.
	*/

	y1 = lowerWhisker; y2 = q25;
	if (ymax > y1 && ymin < y2)
	{
		y1 = MAX (y1, ymin);
		y2 = MIN (y2, ymax);
		Graphics_line (g, x, y1, x, y2);
	}
	y1 = q25; y2 = q75;
	if (ymax > y1 && ymin < y2)
	{
		y1 = MAX (y1, ymin);
		y2 = MIN (y2, ymax);
		Graphics_line (g, x - w, y1, x - w, y2);
		Graphics_line (g, x + w, y1, x + w, y2);
	}
	y1 = q75; y2 = upperWhisker;
	if (ymax > y1 && ymin < y2)
	{
		y1 = MAX (y1, ymin);
		y2 = MIN (y2, ymax);
		Graphics_line (g, x, y1, x, y2);
	}
}

void TableOfReal_drawBoxPlots (I, Graphics g, long rowmin, long rowmax,
	long colmin, long colmax, double ymin, double ymax, int garnish)
{
	iam (TableOfReal);
	double *data = NULL;
	long i, j, numberOfRows;

	if (rowmax < rowmin || rowmax < 1)
	{
		rowmin = 1;
		rowmax = my numberOfRows;
	}
	if (rowmin < 1) rowmin = 1;
	if (rowmax > my numberOfRows) rowmax = my numberOfRows;
	numberOfRows = rowmax - rowmin + 1;
	if (colmax < colmin || colmax < 1)
	{
		colmin = 1;
		colmax = my numberOfColumns;
	}
	if (colmin < 1) colmin = 1;
	if (colmax > my numberOfColumns) colmax = my numberOfColumns;
	if (ymax <= ymin) NUMdmatrix_extrema (my data, rowmin, rowmax,
		colmin, colmax, &ymin, &ymax);
	if ((data = NUMdvector (1, numberOfRows)) == NULL) return;

    Graphics_setWindow (g, colmin - 0.5, colmax + 0.5, ymin, ymax);
    Graphics_setInner (g);

	for (j = colmin; j <= colmax; j++)
	{
		double x = j, r = 0.05, w = 0.2, t;
		long ndata = 0;

		for (i = 1; i <= numberOfRows; i++)
		{
			if ((t = my data[rowmin+i-1][j]) != NUMundefined) data[++ndata] = t;
		}
		Graphics_drawBoxPlot (g, data, ndata, x, r, w, ymin, ymax);
	}
	Graphics_unsetInner (g);
	if (garnish)
	{
		Graphics_drawInnerBox (g);
		for (j = colmin; j <= colmax; j++)
		{
			if (my columnLabels && my columnLabels[j] && my columnLabels[j][0])
				Graphics_markBottom (g, j, 0, 1, 0, my columnLabels [j]);
		}
    	Graphics_marksLeft (g, 2, 1, 1, 0);
	}
	NUMdvector_free (data, 1);
}

void TableOfReal_drawScatterPlotMatrix (I, Graphics g, long colb, long cole,
	double fractionWhite)
{
	iam (TableOfReal);
	double *xmin = NULL, *xmax = NULL;
	long i, j, k, m = my numberOfRows, n;

	if (colb == 0 && cole == 0)
	{
		colb = 1; cole = my numberOfColumns;
	}
	else if (cole < colb || colb < 1 || cole > my numberOfColumns) return;

	n = cole - colb + 1;
	if (n == 1) return;

	if (! (xmin = NUMdvector (colb, cole)) ||
		! (xmax = NUMdvector (colb, cole))) goto end;

	for (j=colb; j <= cole; j++)
	{
		xmin[j] = xmax[j] = my data[1][j];
	}
	for (i=2; i <= m; i++)
	{
		for (j=colb; j <= cole; j++)
		{
			if (my data[i][j] > xmax[j]) xmax[j] = my data[i][j];
			else if (my data[i][j] < xmin[j]) xmin[j] = my data[i][j];
		}
	}
	for (j=colb; j <= cole; j++)
	{
		double extra = fractionWhite * fabs (xmax[j] - xmin[j]);
		if (extra == 0) extra = 0.5;
		xmin[j] -= extra; xmax[j] += extra;
	}

	Graphics_setWindow (g, 0, n, 0, n);
    Graphics_setInner (g);
	Graphics_line (g, 0, n, n, n);
	Graphics_line (g, 0, 0, 0, n);
	Graphics_setTextAlignment (g, Graphics_CENTRE, Graphics_HALF);

	for (i=1; i <= n; i++)
	{
		long xcol, ycol = colb + i - 1; const wchar_t *mark; wchar_t label[20];
		Graphics_line (g, 0, n - i, n, n - i);
		Graphics_line (g, i, n, i, 0);
		for (j=1; j <= n; j++)
		{
			xcol = colb + j -1;
			if (i == j)
			{
				mark = my columnLabels[xcol];
				if (! mark)
				{
					swprintf (label, 20, L"Column %ld", xcol); mark = label;
				}
				Graphics_text (g, j - 0.5, n - i + 0.5, mark);
			}
			else
			{
				for (k=1; k <= m; k++)
				{
					double x = j - 1 + (my data[k][xcol] - xmin[xcol]) /
						(xmax[xcol] - xmin[xcol]);
					double y = n - i + (my data[k][ycol] - xmin[ycol]) /
						(xmax[ycol] - xmin[ycol]);
					mark = EMPTY_STRING (my rowLabels[k]) ? L"+" : my rowLabels[k];
					Graphics_text (g, x, y, mark);
				}
			}
		}
	}
    Graphics_unsetInner (g);
end:
	NUMdvector_free (xmin, colb); NUMdvector_free (xmax, colb);
}

/* NUMundefined ??? */
void NUMdmatrix_getColumnExtrema (double **a, long rowb, long rowe, long icol, double *min, double *max)
{
	long i;
	*min = *max = a[rowb][icol];
	for (i=rowb+1; i <= rowe; i++)
	{
		double t = a[i][icol];
		if (t > *max) *max = t;
		else if (t < *min) *min = t;
	}
}

void TableOfReal_drawScatterPlot (I, Graphics g, long icx, long icy, long rowb,
	long rowe, double xmin, double xmax, double ymin, double ymax,
	int labelSize, int useRowLabels, wchar_t *label, int garnish)
{
    iam (TableOfReal);
	double tmp, m = my numberOfRows, n = my numberOfColumns;
    long i, noLabel = 0;
	int fontSize = Graphics_inqFontSize (g);

    if (icx < 1 || icx > n || icy < 1 || icy > n) return;
    if (rowb < 1) rowb = 1;
    if (rowe > m) rowe = m;
    if (rowe <= rowb)
    {
    	rowb = 1; rowe = m;
    }

    if (xmax == xmin)
    {
		NUMdmatrix_getColumnExtrema (my data, rowb, rowe, icx, & xmin, & xmax);
		tmp = xmax - xmin == 0 ? 0.5 : 0.0;
		xmin -= tmp; xmax += tmp;
    }
    if (ymax == ymin)
    {
		NUMdmatrix_getColumnExtrema (my data, rowb, rowe, icy, & ymin, & ymax);
		tmp = ymax - ymin == 0 ? 0.5 : 0.0;
		ymin -= tmp; ymax += tmp;
    }

    Graphics_setWindow (g, xmin, xmax, ymin, ymax);
    Graphics_setInner (g);
	Graphics_setTextAlignment (g, Graphics_CENTRE, Graphics_HALF);
	Graphics_setFontSize (g, labelSize);

    for (i=rowb; i <= rowe; i++)
    {
    	double x = my data[i][icx], y = my data[i][icy];

		if (((xmin < xmax && x >= xmin && x <= xmax) || (xmin > xmax && x <= xmin && x >= xmax)) &&
			((ymin < ymax && y >= ymin && y <= ymax) || (ymin > ymax && y <= ymin && y >= ymax)))
		{
			wchar_t *plotLabel = useRowLabels ? my rowLabels[i] : label;
			if (! NUMstring_containsPrintableCharacter (plotLabel))
			{
				noLabel++;
				continue;
			}
			Graphics_text (g, x, y, plotLabel);
		}
	}

	Graphics_setFontSize (g, fontSize);
    Graphics_unsetInner (g);

    if (garnish)
    {
		Graphics_drawInnerBox (g);
		if (ymin < ymax)
		{
			if (my columnLabels[icx]) Graphics_textBottom (g, 1, my columnLabels[icx]);
			Graphics_marksBottom (g, 2, 1, 1, 0);
		}
		else
		{
			if (my columnLabels[icx]) Graphics_textTop (g, 1, my columnLabels[icx]);
			Graphics_marksTop (g, 2, 1, 1, 0);
		}
		if (xmin < xmax)
		{
			if (my columnLabels[icy]) Graphics_textLeft (g, 1, my columnLabels[icy]);
			Graphics_marksLeft (g, 2, 1, 1, 0);
		}
		else
		{
			if (my columnLabels[icy]) Graphics_textRight (g, 1, my columnLabels[icy]);
			Graphics_marksRight (g, 2, 1, 1, 0);
		}
	}
	if (noLabel > 0) Melder_warning4 (Melder_integer (noLabel), L" from ", Melder_integer (my numberOfRows), L" labels are "
		"not visible because they are empty or they contain only spaces or non-printable characters");
}

void TableOfReal_drawVectors (I, Graphics g, long colx1, long coly1,
	long colx2, long coly2, double xmin, double xmax,
	double ymin, double ymax, int vectype, int labelsize, int garnish)
{
	iam (TableOfReal);
	long i, nx = my numberOfColumns, ny = my numberOfRows;
	double min, max;
	int fontsize = Graphics_inqFontSize (g);

	if (colx1 < 1 || colx1 > nx || coly1 < 1 || coly1 > nx)
	{
		Melder_warning3 (L"The index in the \"From\" column(s) must be in range [1, ", Melder_integer (nx), L"].");
		return;
	}
	if (colx2 < 1 || colx2 > nx || coly2 < 1 || coly2 > nx)
	{
		Melder_warning3 (L"The index in the \"To\" column(s) must be in range [1, ", Melder_integer (nx), L"].");
		return;
	}

	if (xmin >= xmax)
	{
		NUMdmatrix_extrema (my data, 1, ny, colx1, colx1, &min, &max);
		NUMdmatrix_extrema (my data, 1, ny, colx2, colx2, &xmin, &xmax);
		if (min < xmin) xmin = min;
		if (max > xmax) xmax = max;
	}
	if (ymin >= ymax)
	{
		NUMdmatrix_extrema (my data, 1, ny, coly1, coly1, &min, &max);
		NUMdmatrix_extrema (my data, 1, ny, coly2, coly2, &ymin, &ymax);
		if (min < ymin) ymin = min;
		if (max > ymax) ymax = max;
	}
	if (xmin == xmax)
	{
		if (ymin == ymax) return;
		xmin -= 0.5;
		xmax += 0.5;
	}
	if (ymin == ymax)
	{
		ymin -= 0.5;
		ymax += 0.5;
	}

	Graphics_setWindow (g, xmin, xmax, ymin, ymax);
	Graphics_setInner (g);
	Graphics_setTextAlignment (g, Graphics_CENTRE, Graphics_HALF);

	if (labelsize > 0) Graphics_setFontSize (g, labelsize);
	for (i = 1; i <= ny; i++)
	{
		float x1 = my data[i][colx1];
		float y1 = my data[i][coly1];
		float x2 = my data[i][colx2];
		float y2 = my data[i][coly2];
		const wchar_t *mark = EMPTY_STRING (my rowLabels[i]) ? L"" : my rowLabels[i];
		if (vectype == Graphics_LINE)
			Graphics_line (g, x1, y1, x2, y2);
		else if (vectype == Graphics_TWOWAYARROW)
		{
			Graphics_arrow (g, x1, y1, x2, y2);
			Graphics_arrow (g, x2, y2, x1, y1);
		}
		else /*if (vectype == Graphics_ARROW) */
			Graphics_arrow (g, x1, y1, x2, y2);
		if (labelsize <= 0) continue;
		Graphics_text (g, x1, y1, mark);

	}
	if (labelsize > 0) Graphics_setFontSize (g, fontsize);
	Graphics_unsetInner (g);
	if (garnish)
	{
	    Graphics_drawInnerBox (g);
    	Graphics_marksLeft (g, 2, 1, 1, 0);
    	Graphics_marksBottom (g, 2, 1, 1, 0);
	}
}

void TableOfReal_drawColumnAsDistribution (I, Graphics g, int column, double minimum, double maximum, long nBins,
	double freqMin, double freqMax, int cumulative, int garnish)
{
	iam (TableOfReal);
	if (column < 1 || column > my numberOfColumns) return;
	Matrix thee = TableOfReal_to_Matrix (me);
	Matrix_drawDistribution (thee, g,  column-0.5, column+0.5, 0, 0,
		minimum, maximum, nBins, freqMin,  freqMax,  cumulative,  garnish);
	if (garnish && my columnLabels[column] != NULL) Graphics_textBottom (g, 1, my columnLabels[column]);
	forget (thee);
}


FORM (TableOfReal_reportMultivariateNormality, L"TableOfReal: Report multivariate normality (BHEP)", L"TableOfReal: Report multivariate normality (BHEP)...")
	REAL (L"Smoothing parameter", L"0.0")
	OK
DO
	TableOfReal me = (structTableOfReal *)ONLY (classTableOfReal);
	double tnb, lnmu, lnvar;
	double h = GET_REAL (L"Smoothing parameter");
	double prob = TableOfReal_normalityTest_BHEP (me, &h, &tnb, &lnmu, &lnvar);
	MelderInfo_open ();
	MelderInfo_writeLine1 (L"Baringhaus–Henze–Epps–Pulley normality test:");
	MelderInfo_writeLine2 (L"Significance of normality: ", Melder_double (prob));
	MelderInfo_writeLine2 (L"BHEP statistic: ", Melder_double (tnb));
	MelderInfo_writeLine2 (L"Lognormal mean: ", Melder_double (lnmu));
	MelderInfo_writeLine2 (L"Lognormal variance: ", Melder_double (lnvar));
	MelderInfo_writeLine2 (L"Smoothing: ", Melder_double (h));
	MelderInfo_writeLine2 (L"Sample size: ", Melder_integer (my numberOfRows));
	MelderInfo_writeLine2 (L"Number of variables: ", Melder_integer (my numberOfColumns));
	MelderInfo_close ();
END

DIRECT (TableOfReal_and_Permutation_permuteRows)
	TableOfReal t = (structTableOfReal *)ONLY (classTableOfReal);
	Permutation p = (structPermutation *)ONLY (classPermutation);
	if (! praat_new3 (TableOfReal_and_Permutation_permuteRows (t, p),
		Thing_getName (t), L"_", Thing_getName (p))) return 0;
END

DIRECT (TableOfReal_to_Permutation_sortRowlabels)
	EVERY_TO (TableOfReal_to_Permutation_sortRowLabels ((structTableOfReal *)OBJECT))
END

DIRECT (TableOfReal_appendColumns)
	Collection me = Collection_create (classTableOfReal, 10);
	if (! me) return 0;
	WHERE (SELECTED)
		if (! Collection_addItem (me, OBJECT))
		{
			my size = 0; forget (me); return 0;
		}
	if (! praat_new1 (TableOfReal_appendColumnsMany (me), L"columns_appended"))
	{
		my size = 0; forget (me); return 0;
	}
	my size = 0; forget (me);
END

FORM (TableOfReal_createFromPolsData_50males, L"Create TableOfReal (Pols 1973)", L"Create TableOfReal (Pols 1973)...")
	BOOLEAN (L"Include formant levels", 0)
	OK
DO
	if (! praat_new1 (TableOfReal_createFromPolsData_50males
		(GET_INTEGER (L"Include formant levels")), L"pols_50males")) return 0;
END

FORM (TableOfReal_createFromVanNieropData_25females, L"Create TableOfReal (Van Nierop 1973)...", L"Create TableOfReal (Van Nierop 1973)...")
	BOOLEAN (L"Include formant levels", 0)
	OK
DO
	if (! praat_new1 (TableOfReal_createFromVanNieropData_25females
		(GET_INTEGER (L"Include formant levels")), L"vannierop_25females")) return 0;
END

FORM (TableOfReal_createFromWeeninkData, L"Create TableOfReal (Weenink 1985)...", L"Create TableOfReal (Weenink 1985)...")
	RADIO (L"Speakers group", 1)
	RADIOBUTTON (L"Men")
	RADIOBUTTON (L"Women")
	RADIOBUTTON (L"Children")
	OK
DO
	int type = GET_INTEGER (L"Speakers group");
	if (! praat_new1 (TableOfReal_createFromWeeninkData (type),
		(type == 1 ? L"m10" : type == 2 ? L"w10" : L"c10"))) return 0;
END

FORM (TableOfReal_drawScatterPlot, L"TableOfReal: Draw scatter plot", L"TableOfReal: Draw scatter plot...")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Select the part of the table")
	NATURAL (L"Horizontal axis column number", L"1")
	NATURAL (L"Vertical axis column number", L"2")
	INTEGER (L"left Row number range", L"0")
	INTEGER (L"right Row number range", L"0")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Select the drawing area limits")
	REAL (L"left Horizontal range", L"0.0")
	REAL (L"right Horizontal range", L"0.0")
	REAL (L"left Vertical range", L"0.0")
	REAL (L"right Vertical range", L"0.0")
	NATURAL (L"Label size", L"12")
	BOOLEAN (L"Use row labels", 0)
	WORD (L"Label", L"+")
	BOOLEAN (L"Garnish", 1)
	OK
DO
	EVERY_DRAW (TableOfReal_drawScatterPlot ((structTableOfReal *)OBJECT, GRAPHICS,
			GET_INTEGER (L"Horizontal axis column number"),
			GET_INTEGER (L"Vertical axis column number"),
			GET_INTEGER (L"left Row number range"), GET_INTEGER (L"right Row number range"),
			GET_REAL (L"left Horizontal range"), GET_REAL (L"right Horizontal range"),
			GET_REAL (L"left Vertical range"), GET_REAL (L"right Vertical range"),
			GET_INTEGER (L"Label size"), GET_INTEGER (L"Use row labels"),
			GET_STRING (L"Label"), GET_INTEGER (L"Garnish")))
END

FORM (TableOfReal_drawScatterPlotMatrix, L"TableOfReal: Draw scatter plots matrix", 0)
	INTEGER (L"From column", L"0")
	INTEGER (L"To column", L"0")
	POSITIVE (L"Fraction white", L"0.1")
	OK
DO
	EVERY_DRAW (TableOfReal_drawScatterPlotMatrix ((structTableOfReal *)OBJECT, GRAPHICS,
		GET_INTEGER (L"From column"), GET_INTEGER (L"To column"),
		GET_REAL (L"Fraction white")))
END

FORM (TableOfReal_drawBiplot, L"TableOfReal: Draw biplot", L"TableOfReal: Draw biplot...")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"ui/editors/AmplitudeTierEditor.h")
	REAL (L"left Horizontal range", L"0.0")
	REAL (L"right Horizontal range", L"0.0")
	REAL (L"left Vertical range", L"0.0")
	REAL (L"right Vertical range", L"0.0")
	REAL (L"Split factor", L"0.5")
	INTEGER (L"Label size", L"10")
	BOOLEAN (L"Garnish", 1)
	OK
DO
	EVERY_DRAW (TableOfReal_drawBiplot ((structTableOfReal *)OBJECT, GRAPHICS,
		GET_REAL (L"left Horizontal range"), GET_REAL (L"right Horizontal range"),
		GET_REAL (L"left Vertical range"), GET_REAL (L"right Vertical range"), GET_REAL (L"Split factor"),
		GET_INTEGER (L"Label size"), GET_INTEGER (L"Garnish")))
END

FORM (TableOfReal_drawVectors, L"Draw vectors", L"TableOfReal: Draw vectors...")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"From (x1, y1) to (x2, y2)")
	NATURAL (L"left From columns (x1, y1)", L"1")
	NATURAL (L"right From columns (x1, y1)", L"2")
	NATURAL (L"left To columns (x2, y2)", L"3")
	NATURAL (L"right To columns (x2, y2)", L"4")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Select the drawing area")
	REAL (L"left Horizontal range", L"0.0")
	REAL (L"right Horizontal range", L"0.0")
	REAL (L"left Vertical range", L"0.0")
	REAL (L"right Vertical range", L"0.0")
	RADIO (L"Vector type", 1)
	RADIOBUTTON (L"Arrow")
	RADIOBUTTON (L"Double arrow")
	RADIOBUTTON (L"Line")
	INTEGER (L"Label size", L"10")
	BOOLEAN (L"Garnish", 1)
	OK
DO
	EVERY_DRAW (TableOfReal_drawVectors ((structTableOfReal *)OBJECT, GRAPHICS,
		GET_INTEGER (L"left From columns"), GET_INTEGER (L"right From columns"),
		GET_INTEGER (L"left To columns"), GET_INTEGER (L"right To columns"),
		GET_REAL (L"left Horizontal range"), GET_REAL (L"right Horizontal range"),
		GET_REAL (L"left Vertical range"), GET_REAL (L"right Vertical range"),
		GET_INTEGER (L"Vector type"), GET_INTEGER (L"Label size"),
		GET_INTEGER (L"Garnish")))
END

FORM (TableOfReal_drawRowAsHistogram, L"Draw row as histogram", L"TableOfReal: Draw rows as histogram...")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Select from the table")
	WORD (L"Row number", L"1")
	INTEGER (L"left Column range", L"0")
    INTEGER (L"right Column range", L"0")
    LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Vertical drawing range")
    REAL (L"left Vertical range", L"0.0")
    REAL (L"right Vertical range", L"0.0")
    LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Offset and distance in units of 'bar width'")
    REAL (L"Horizontal offset", L"0.5")
    REAL (L"Distance between bars", L"1.0")
    WORD (L"Grey value (1=white)", L"0.7")
    BOOLEAN (L"Garnish", 1)
	OK
DO
	EVERY_DRAW (TableOfReal_drawRowsAsHistogram ((structTableOfReal *)OBJECT, GRAPHICS, GET_STRING (L"Row number"),
		GET_INTEGER (L"left Column range"), GET_INTEGER (L"right Column range"),
		GET_REAL (L"left Vertical range"), GET_REAL (L"right Vertical range"),
		GET_REAL (L"Horizontal offset"), 0,
		GET_REAL (L"Distance between bars"), GET_STRING (L"Grey value"),
		GET_INTEGER (L"Garnish")))
END

FORM (TableOfReal_drawRowsAsHistogram, L"Draw rows as histogram", L"TableOfReal: Draw rows as histogram...")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Select from the table")
	SENTENCE (L"Row numbers", L"1 2")
	INTEGER (L"left Column range", L"0")
    INTEGER (L"right Column range", L"0")
    LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Vertical drawing range")
    REAL (L"left Vertical range", L"0.0")
    REAL (L"right Vertical range", L"0.0")
    LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Offset and distance in units of 'bar width'")
    REAL (L"Horizontal offset", L"1.0")
    REAL (L"Distance between bar groups", L"1.0")
    REAL (L"Distance between bars", L"0.0")
    SENTENCE (L"Grey values (1=white)", L"1 1")
    BOOLEAN (L"Garnish", 1)
	OK
DO
	EVERY_DRAW (TableOfReal_drawRowsAsHistogram ((structTableOfReal *)OBJECT, GRAPHICS,
		GET_STRING (L"Row numbers"),
		GET_INTEGER (L"left Column range"), GET_INTEGER (L"right Column range"),
		GET_REAL (L"left Vertical range"), GET_REAL (L"right Vertical range"),
		GET_REAL (L"Horizontal offset"), GET_REAL (L"Distance between bars"),
		GET_REAL (L"Distance between bar groups"), GET_STRING (L"Grey values"),
		GET_INTEGER (L"Garnish")))
END

FORM (TableOfReal_drawBoxPlots, L"TableOfReal: Draw box plots", L"TableOfReal: Draw box plots...")
	INTEGER (L"From row", L"0")
	INTEGER (L"To row", L"0")
	INTEGER (L"From column", L"0")
	INTEGER (L"To column", L"0")
	REAL (L"left Vertical range", L"0.0")
	REAL (L"right Vertical range", L"0.0")
	BOOLEAN (L"Garnish", 1)
	OK
DO
	EVERY_DRAW (TableOfReal_drawBoxPlots ((structTableOfReal *)OBJECT, GRAPHICS,
		GET_INTEGER (L"From row"), GET_INTEGER (L"To row"),
		GET_INTEGER (L"From column"), GET_INTEGER (L"To column"),
		GET_REAL (L"left Vertical range"), GET_REAL (L"right Vertical range"), GET_INTEGER (L"Garnish")))
END

FORM (TableOfReal_drawColumnAsDistribution, L"TableOfReal: Draw column as distribution", L"TableOfReal: Draw column as distribution...")
	NATURAL (L"Column number", L"1")
    REAL (L"left Value range", L"0.0")
    REAL (L"right Value range", L"0.0")
    REAL (L"left Frequency range", L"0.0")
    REAL (L"right frequency range", L"0.0")
    NATURAL (L"Number of bins", L"10")
    BOOLEAN (L"Garnish", 1)
	OK
DO
	EVERY_DRAW (TableOfReal_drawColumnAsDistribution ((structTableOfReal *)OBJECT, GRAPHICS, GET_INTEGER (L"Column number"),
		GET_REAL (L"left Value range"), GET_REAL (L"right Value range"),GET_INTEGER (L"Number of bins"),
		GET_REAL (L"left Frequency range"), GET_REAL (L"right frequency range"), 0, GET_INTEGER (L"Garnish")))
END

FORM (TableOfReal_to_Configuration_lda, L"TableOfReal: To Configuration (lda)", L"TableOfReal: To Configuration (lda)...")
	INTEGER (L"Number of dimensions", L"0")
	OK
DO
	long dimension = GET_INTEGER (L"Number of dimensions");
	REQUIRE (dimension >= 0, L"Number of dimensions must be greater equal zero.")
	EVERY_TO (TableOfReal_to_Configuration_lda ((structTableOfReal *)OBJECT, dimension))
END

FORM (TableOfReal_to_CCA, L"TableOfReal: To CCA", L"TableOfReal: To CCA...")
	NATURAL (L"Dimension of dependent variate", L"2")
	OK
DO
	EVERY_TO (TableOfReal_to_CCA ((structTableOfReal *)OBJECT, GET_INTEGER (L"Dimension of dependent variate")))
END

FORM (TableOfReal_to_Configuration_pca, L"TableOfReal: To Configuration (pca)", L"TableOfReal: To Configuration (pca)...")
	NATURAL (L"Number of dimensions", L"2")
	OK
DO
	EVERY_TO (TableOfReal_to_Configuration_pca ((structTableOfReal *)OBJECT,
		GET_INTEGER (L"Number of dimensions")))
END

DIRECT (TableOfReal_to_Discriminant)
	EVERY_TO (TableOfReal_to_Discriminant ((structTableOfReal *)OBJECT))
END

DIRECT (TableOfReal_to_PCA)
	EVERY_TO (TableOfReal_to_PCA ((structTableOfReal *)OBJECT))
END

FORM (TableOfReal_to_SSCP, L"TableOfReal: To SSCP", L"TableOfReal: To SSCP...")
	INTEGER (L"Begin row", L"0")
	INTEGER (L"End row", L"0")
	INTEGER (L"Begin column", L"0")
	INTEGER (L"End column", L"0")
	OK
DO
	EVERY_TO (TableOfReal_to_SSCP ((structTableOfReal *)OBJECT, GET_INTEGER (L"Begin row"), GET_INTEGER (L"End row"),
		GET_INTEGER (L"Begin column"), GET_INTEGER (L"End column")))
END

/* For the inheritors */
DIRECT (TableOfReal_to_TableOfReal)
	EVERY_TO (TableOfReal_to_TableOfReal ((structTableOfReal *)OBJECT))
END

DIRECT (TableOfReal_to_Correlation)
	EVERY_TO (TableOfReal_to_Correlation ((structTableOfReal *)OBJECT))
END
DIRECT (TableOfReal_to_Correlation_rank)
	EVERY_TO (TableOfReal_to_Correlation_rank ((structTableOfReal *)OBJECT))
END

DIRECT (TableOfReal_to_Covariance)
	EVERY_TO (TableOfReal_to_Covariance ((structTableOfReal *)OBJECT))
END

DIRECT (TableOfReal_to_SVD)
	EVERY_TO (TableOfReal_to_SVD ((structTableOfReal *)OBJECT))
END

DIRECT (TablesOfReal_to_Eigen_gsvd)
	TableOfReal me = NULL, thee = NULL;
	WHERE (SELECTED)
	{
		if (me) thee = (structTableOfReal *)OBJECT;
		else me = (structTableOfReal *)OBJECT;
	}
	NEW (TablesOfReal_to_Eigen_gsvd (me, thee))
END

FORM (TableOfReal_and_TableOfReal_crossCorrelations, L"TableOfReal & TableOfReal: Cross-correlations", 0)
	OPTIONMENU (L"Correlations between", 1)
	OPTION (L"Rows")
	OPTION (L"Columns")
	BOOLEAN (L"Center", 0)
	BOOLEAN (L"Normalize", 0)
	OK
DO
	TableOfReal t1 = NULL, t2 = NULL;
	int by_columns = GET_INTEGER (L"Correlations between") - 1;
	WHERE (SELECTED && CLASS == classTableOfReal) { if (t1) t2 = (structTableOfReal *)OBJECT; else t1 = (structTableOfReal *)OBJECT; }
	if (! praat_new1 (TableOfReal_and_TableOfReal_crossCorrelations (t1, t2, by_columns,
		GET_INTEGER (L"Center"), GET_INTEGER (L"Normalize")),
		(by_columns ? L"by_columns" : L"by_rows"))) return 0;
END

DIRECT (TablesOfReal_to_GSVD)
	TableOfReal me = NULL, thee = NULL;
	WHERE (SELECTED)
	{
		if (me) thee = (structTableOfReal *)OBJECT;
		else me = (structTableOfReal *)OBJECT;
	}
	NEW (TablesOfReal_to_GSVD (me, thee))
END

FORM (TableOfReal_choleskyDecomposition, L"TableOfReal: Cholesky decomposition", 0)
	BOOLEAN (L"Upper (else L)", 0)
	BOOLEAN (L"Inverse", 0)
	OK
DO
	EVERY_TO (TableOfReal_choleskyDecomposition ((structTableOfReal *)OBJECT, GET_INTEGER (L"Upper"), GET_INTEGER (L"Inverse")))
END

FORM (TableOfReal_to_Pattern_and_Categories, L"TableOfReal: To Pattern and Categories", L"TableOfReal: To Pattern and Categories...")
	INTEGER (L"left Row range", L"0")
	INTEGER (L"right Row range", L"0 (=all)")
	INTEGER (L"left Column range", L"0")
	INTEGER (L"right Column range", L"0 (=all)")
	OK
DO
	Pattern p; Categories c; TableOfReal t = (structTableOfReal *)ONLY_OBJECT;
	if (TableOfReal_to_Pattern_and_Categories (t, GET_INTEGER (L"left Row range"),
		GET_INTEGER (L"right Row range"), GET_INTEGER (L"left Column range"),
		GET_INTEGER (L"right Column range"), &p, &c))
	{
		wchar_t *name = Thing_getName (t);
		praat_new1 (p, name);
		praat_new1 (c, name);
	}
END

FORM (TableOfReal_getColumnSum, L"TableOfReal: Get column sum", L"ui/editors/AmplitudeTierEditor.h")
	INTEGER (L"Column", L"1")
	OK
DO
	Melder_information1 (Melder_double (TableOfReal_getColumnSum ((structTableOfReal *)ONLY_GENERIC(classTableOfReal), GET_INTEGER (L"Column"))));
END

FORM (TableOfReal_getRowSum, L"TableOfReal: Get row sum", L"ui/editors/AmplitudeTierEditor.h")
	INTEGER (L"Row", L"1")
	OK
DO
	Melder_information1 (Melder_double (TableOfReal_getRowSum ((structTableOfReal *)ONLY_GENERIC(classTableOfReal), GET_INTEGER (L"Row"))));
END

DIRECT (TableOfReal_getGrandSum)
	Melder_information1 (Melder_double (TableOfReal_getGrandSum ((structTableOfReal *)ONLY_GENERIC(classTableOfReal))));
END

FORM (TableOfReal_meansByRowLabels, L"TableOfReal: Means by row labels", L"TableOfReal: To TableOfReal (means by row labels)...")
    BOOLEAN (L"Expand", 0)
	OK
DO
	EVERY_CHECK(praat_new2 (TableOfReal_meansByRowLabels ((structTableOfReal *)OBJECT, GET_INTEGER (L"Expand"), 0), NAME, L"_byrowlabels"))
END

FORM (TableOfReal_mediansByRowLabels, L"TableOfReal: Medians by row labels", L"TableOfReal: To TableOfReal (medians by row labels)...")
    BOOLEAN (L"Expand", 0)
	OK
DO
	EVERY_CHECK(praat_new2 (TableOfReal_meansByRowLabels (OBJECT, GET_INTEGER (L"Expand"), 1), NAME, L"_byrowlabels"))
END

/***** TableOfReal and FilterBank  *****/

FORM (TextGrid_extendTime, L"TextGrid: Extend time", L"TextGrid: Extend time...")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"ui/editors/AmplitudeTierEditor.h")
	POSITIVE (L"Extend domain by (s)", L"1.0")
	RADIO (L"At", 1)
	RADIOBUTTON (L"End")
	RADIOBUTTON (L"Start")
	OK
DO
	WHERE (SELECTED)
	{
		TextGrid_extendTime ((structTextGrid *)OBJECT, GET_REAL (L"Extend domain by"), GET_INTEGER (L"At") - 1);
		praat_dataChanged (OBJECT);
	}
END

FORM (TextGrid_replaceIntervalTexts, L"TextGrid: Replace interval text", L"TextGrid: Replace interval text...")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"ui/editors/AmplitudeTierEditor.h")
	NATURAL (L"Tier number", L"1")
	INTEGER (L"left Interval range", L"0")
	INTEGER (L"right Interval range", L"0")
	SENTENCE (L"Search", L"a")
	SENTENCE (L"Replace", L"a")
	RADIO (L"Search and replace strings are:", 1)
	RADIOBUTTON (L"Literals")
	RADIOBUTTON (L"Regular Expressions")
	OK
DO
	long from = GET_INTEGER (L"left Interval range");
	long to = GET_INTEGER (L"right Interval range");
	int regexp = GET_INTEGER (L"Search and replace strings are") - 1;
	wchar_t *search = GET_STRING (L"Search");
	long nmatches, nstringmatches;

	WHERE (SELECTED)
	{
		if (! TextGrid_changeLabels ((structTextGrid *)OBJECT, GET_INTEGER (L"Tier number"), from, to, search, GET_STRING (L"Replace"),
			regexp, &nmatches, &nstringmatches)) return 0;
		praat_dataChanged (OBJECT);
	}
END


FORM (TextGrid_replacePointTexts, L"TextGrid: Replace point text", L"TextGrid: Replace point text...")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"ui/editors/AmplitudeTierEditor.h")
	NATURAL (L"Tier number", L"1")
	INTEGER (L"left Interval range", L"0")
	INTEGER (L"right Interval range", L"0")
	SENTENCE (L"Search", L"a")
	SENTENCE (L"Replace", L"a")
	RADIO (L"Search and replace strings are:", 1)
	RADIOBUTTON (L"Literals")
	RADIOBUTTON (L"Regular Expressions")
	OK
DO
	long from = GET_INTEGER (L"left Interval range");
	long to = GET_INTEGER (L"right Interval range");
	long nmatches, nstringmatches;
	WHERE (SELECTED)
	{
		if (! TextGrid_changeLabels ((structTextGrid *)OBJECT, GET_INTEGER (L"Tier number"), from, to, GET_STRING (L"Search"), GET_STRING (L"Replace"),
			GET_INTEGER (L"Search and replace strings are")-1, &nmatches, &nstringmatches)) return 0;
		praat_dataChanged (OBJECT);
	}
END

FORM (TextGrid_setTierName, L"TextGrid: Set tier name", L"TextGrid: Set tier name...")
	NATURAL (L"Tier number:", L"1")
	SENTENCE (L"Name", L"ui/editors/AmplitudeTierEditor.h");
	OK
DO
	if (! TextGrid_setTierName ((structTextGrid *)ONLY_OBJECT, GET_INTEGER (L"Tier number"),
		GET_STRING (L"Name"))) return 0;
		praat_dataChanged (OBJECT);
END

static VowelEditor *vowelEditor = NULL;
DIRECT (VowelEditor_create)
	if (theCurrentPraatApplication -> batch) return Melder_error1 (L"Cannot edit from batch.");
	vowelEditor = new VowelEditor (theCurrentPraatApplication -> topShell, L"VowelEditor", NULL);
	if (vowelEditor == NULL) return 0;
END

static Any cmuAudioFileRecognizer (int nread, const char *header, MelderFile fs)
{
	return nread < 12 || header [0] != 6 || header [1] != 0 ?
	 NULL : Sound_readFromCmuAudioFile (fs);
}

void praat_CC_init (void *klas)
{
	praat_addAction1 (klas, 1, L"Paint...", 0, 1, DO_CC_paint);
	praat_addAction1 (klas, 1, L"Draw...", 0, 1, DO_CC_drawC0);
	praat_addAction1 (klas, 1, QUERY_BUTTON, 0, 0, 0);
	praat_TimeFrameSampled_query_init (klas);
	praat_addAction1 (klas, 1, L"Get value...", 0, 1, DO_CC_getValue);
	praat_addAction1 (klas, 0, L"To Matrix", 0, 0, DO_CC_to_Matrix);
	praat_addAction1 (klas, 2, L"To DTW...", 0, 0, DO_CCs_to_DTW);
}

static void praat_Eigen_Matrix_project (void *klase, void *klasm);
static void praat_Eigen_Matrix_project (void *klase, void *klasm)
{
	praat_addAction2 (klase, 1, klasm, 1, L"Project...", 0, 0, DO_Eigen_and_Matrix_project);
}

static void praat_Eigen_query_init (void *klas)
{
	praat_addAction1 (klas, 1, L"Get eigenvalue...", 0, 1, DO_Eigen_getEigenvalue);
	praat_addAction1 (klas, 1, L"Get sum of eigenvalues...", 0, 1, DO_Eigen_getSumOfEigenvalues);
	praat_addAction1 (klas, 1, L"Get number of eigenvectors", 0, 1, DO_Eigen_getNumberOfEigenvalues);
	praat_addAction1 (klas, 1, L"Get eigenvector dimension", 0, 1, DO_Eigen_getDimension);
	praat_addAction1 (klas, 1, L"Get eigenvector element...", 0, 1, DO_Eigen_getEigenvectorElement);
}

static void praat_Eigen_draw_init (void *klas)
{
	praat_addAction1 (klas, 0, L"Draw eigenvalues...", 0, 1, DO_Eigen_drawEigenvalues);
	praat_addAction1 (klas, 0, L"Draw eigenvalues (scree)...", 0, praat_DEPTH_1 | praat_HIDDEN, DO_Eigen_drawEigenvalues_scree);
	praat_addAction1 (klas, 0, L"Draw eigenvector...", 0, 1, DO_Eigen_drawEigenvector);
}

static void praat_Index_init (void *klas)
{
    praat_addAction1 (klas, 1, L"Get number of classes", 0, 0, DO_Index_getNumberOfClasses);
    praat_addAction1 (klas, 1, L"To Permutation...", 0, 0, DO_Index_to_Permutation);
    praat_addAction1 (klas, 1, L"Extract part...", 0, 0, DO_Index_extractPart);
}

static void praat_FilterBank_query_init (void *klas);
static void praat_FilterBank_query_init (void *klas)
{
	praat_addAction1 (klas, 0, QUERY_BUTTON, 0, 0, 0);
	praat_Matrixft_query_init (klas);
	praat_addAction1 (klas, 0, L"-- frequency scales --", 0, 1, 0);
	praat_addAction1 (klas, 1, L"Get frequency in Hertz...", 0, 1, DO_FilterBank_getFrequencyInHertz);
	praat_addAction1 (klas, 1, L"Get frequency in Bark...", 0, 1, DO_FilterBank_getFrequencyInBark);
	praat_addAction1 (klas, 1, L"Get frequency in mel...", 0, 1, DO_FilterBank_getFrequencyInMel);
}

static void praat_FilterBank_modify_init (void *klas);
static void praat_FilterBank_modify_init (void *klas)
{
	praat_addAction1 (klas, 0, MODIFY_BUTTON, 0, 0, 0);
	praat_addAction1 (klas, 0, L"Equalize intensities...", 0, 1, DO_FilterBank_equalizeIntensities);
}

static void praat_FilterBank_draw_init (void *klas);
static void praat_FilterBank_draw_init (void *klas)
{
	praat_addAction1 (klas, 0, DRAW_BUTTON, 0, 0, 0);
		praat_addAction1 (klas, 0, L"Draw filters...", 0, 1, DO_FilterBank_drawFilters);
		praat_addAction1 (klas, 0, L"Draw one contour...", 0, 1, DO_FilterBank_drawOneContour);
		praat_addAction1 (klas, 0, L"Draw contours...", 0, 1, DO_FilterBank_drawContours);
		praat_addAction1 (klas, 0, L"Paint image...", 0, 1, DO_FilterBank_paintImage);
		praat_addAction1 (klas, 0, L"Paint contours...", 0, 1, DO_FilterBank_paintContours);
		praat_addAction1 (klas, 0, L"Paint cells...", 0, 1, DO_FilterBank_paintCells);
		praat_addAction1 (klas, 0, L"Paint surface...", 0, 1, DO_FilterBank_paintSurface);
		praat_addAction1 (klas, 0, L"-- frequency scales --", 0, 1, 0);
		praat_addAction1 (klas, 0, L"Draw frequency scales...", 0, 1, DO_FilterBank_drawFrequencyScales);

}

static void praat_FilterBank_all_init (void *klas);
static void praat_FilterBank_all_init (void *klas)
{
	praat_FilterBank_draw_init (klas);
	praat_FilterBank_query_init (klas);
	praat_FilterBank_modify_init (klas);
	praat_addAction1 (klas, 0, L"To Intensity", 0, 0, DO_FilterBank_to_Intensity);
	praat_addAction1 (klas, 0, L"To Matrix", 0, 0, DO_FilterBank_to_Matrix);
}

static void praat_FunctionTerms_init (void *klas)
{
	praat_addAction1 (klas, 0, DRAW_BUTTON, 0, 0, 0);
	praat_addAction1 (klas, 0, L"Draw...", 0, 1, DO_FunctionTerms_draw);
	praat_addAction1 (klas, 0, L"Draw basis function...", 0, 1, DO_FunctionTerms_drawBasisFunction);
	praat_addAction1 (klas, 0, QUERY_BUTTON, 0, 0, 0);
		praat_addAction1 (klas, 1, L"Get number of coefficients", 0, 1, DO_FunctionTerms_getNumberOfCoefficients);
		praat_addAction1 (klas, 1, L"Get coefficient...", 0, 1, DO_FunctionTerms_getCoefficient);
		praat_addAction1 (klas, 1, L"Get degree", 0, 1, DO_FunctionTerms_getDegree);
		praat_addAction1 (klas, 0, L"-- function specifics --", 0, 1, 0);
		praat_addAction1 (klas, 1, L"Get value...", 0, 1, DO_FunctionTerms_evaluate);
		praat_addAction1 (klas, 1, L"Get minimum...", 0, 1, DO_FunctionTerms_getMinimum);
		praat_addAction1 (klas, 1, L"Get x of minimum...", 0, 1, DO_FunctionTerms_getXOfMinimum);
		praat_addAction1 (klas, 1, L"Get maximum...", 0, 1, DO_FunctionTerms_getMaximum);
		praat_addAction1 (klas, 1, L"Get x of maximum...", 0, 1, DO_FunctionTerms_getXOfMaximum);
	praat_addAction1 (klas, 0, L"Modify -", 0, 0, 0);
		praat_addAction1 (klas, 1, L"Set domain...", 0, 1, DO_FunctionTerms_setDomain);
		praat_addAction1 (klas, 1, L"Set coefficient...", 0, 1, DO_FunctionTerms_setCoefficient);
	praat_addAction1 (klas, 0, L"Analyse", 0, 0, 0);
}

/* Query buttons for frame-based frequency x time subclasses of matrix. */

void praat_Matrixft_query_init (void *klas)
{
	praat_TimeFrameSampled_query_init (klas);
	praat_addAction1 (klas, 1, L"Get time from column...", 0, 1, DO_Matrixft_getXofColumn);
	praat_addAction1 (klas, 1, L"-- frequencies --", 0, 1, 0);
	praat_addAction1 (klas, 1, L"Get lowest frequency", 0, 1, DO_Matrixft_getLowestFrequency);
	praat_addAction1 (klas, 1, L"Get highest frequency", 0, 1, DO_Matrixft_getHighestFrequency);
	praat_addAction1 (klas, 1, L"Get number of frequencies", 0, 1, DO_Matrixft_getNumberOfFrequencies);
	praat_addAction1 (klas, 1, L"Get frequency distance", 0, 1, DO_Matrixft_getFrequencyDistance);
	praat_addAction1 (klas, 1, L"Get frequency from row...", 0, 1, DO_Matrixft_getFrequencyOfRow);
	praat_addAction1 (klas, 1, L"-- get value --", 0, 1, 0);
	praat_addAction1 (klas, 1, L"Get value in cell...", 0, 1, DO_Matrixft_getValueInCell);
}

static void praat_Spline_init (void *klas)
{
	praat_FunctionTerms_init (klas);
	praat_addAction1 (klas, 0, L"Draw knots...", L"Draw basis function...", 1, DO_Spline_drawKnots);
	praat_addAction1 (klas, 1, L"Get order", L"Get degree", 1, DO_Spline_getOrder);
	praat_addAction1 (klas, 1, L"Scale x...", L"Analyse",	0, DO_Spline_scaleX);

}

static void praat_SSCP_query_init (void *klas)
{
	praat_addAction1 (klas, 1, L"-- statistics --", L"Get value...", 1, 0);
	praat_addAction1 (klas, 1, L"Get number of observations", L"-- statistics --", 1, DO_SSCP_getNumberOfObservations);
	praat_addAction1 (klas, 1, L"Get degrees of freedom", L"Get number of observations", 1, DO_SSCP_getDegreesOfFreedom);
	praat_addAction1 (klas, 1, L"Get centroid element...", L"Get degrees of freedom",1, DO_SSCP_getCentroidElement);
	praat_addAction1 (klas, 1, L"Get ln(determinant)", L"Get centroid element...", 1, DO_SSCP_getLnDeterminant);
}

static void praat_SSCP_extract_init (void *klas)
{
	praat_addAction1 (klas, 1, L"Extract centroid", EXTRACT_BUTTON, 1, DO_SSCP_extractCentroid);
}

FORM (SSCP_setValue, L"Covariance: Set value", L"Covariance: Set value...")
	NATURAL (L"Row number", L"1")
	NATURAL (L"Column number", L"1")
	REAL (L"New value", L"1.0")
	OK
DO
	if (! SSCP_setValue ((structSSCP *)ONLY_GENERIC (classSSCP), GET_INTEGER (L"Row number"), GET_INTEGER (L"Column number"),
		GET_REAL (L"New value"))) return 0;
END

FORM (SSCP_setCentroid, L"ui/editors/AmplitudeTierEditor.h", 0)
	NATURAL (L"Element number", L"1")
	REAL (L"New value", L"1.0")
	OK
DO
	if (! SSCP_setCentroid ((structSSCP *)ONLY_GENERIC (classSSCP), GET_INTEGER (L"Element number"), GET_REAL (L"New value"))) return 0;
END

void praat_SSCP_as_TableOfReal_init (void *klas)
{
	praat_TableOfReal_init (klas);
	praat_removeAction (klas, NULL, NULL, L"Set value...");
	praat_addAction1 (klas, 1, L"Set centroid...", L"Formula...", 1, DO_SSCP_setCentroid);
	praat_addAction1 (klas, 1, L"Set value...", L"Formula...", 1, DO_SSCP_setValue);
	praat_addAction1 (klas, 0, L"To TableOfReal", L"To Matrix", 1, DO_TableOfReal_to_TableOfReal);

}

void praat_TableOfReal_init2  (void *klas)
{
	praat_TableOfReal_init (klas);
	praat_addAction1 (klas, 0, L"To TableOfReal", L"To Matrix", 1, DO_TableOfReal_to_TableOfReal);
}

/* Query buttons for frame-based time-based subclasses of Sampled.
void praat_TimeFrameSampled_query_init (void *klas)
{
	praat_TimeFunction_query_init (klas);
	praat_addAction1 (klas, 1, L"Get number of frames", 0, 1,
		DO_TimeFrameSampled_getNumberOfFrames);
	praat_addAction1 (klas, 1, L"Get frame length", 0, 1,
		DO_TimeFrameSampled_getFrameLength);
	praat_addAction1 (klas, 1, L"Get time from frame...", 0, 1,
		DO_TimeFrameSampled_getTimeFromFrame);
	praat_addAction1 (klas, 1, L"Get frame from time...", 0, 1,
		DO_TimeFrameSampled_getFrameFromTime);
}
*/

INCLUDE_LIBRARY (praat_uvafon_MDS_init)
INCLUDE_LIBRARY (praat_KlattGrid_init)
INCLUDE_LIBRARY (praat_HMM_init)
INCLUDE_LIBRARY (praat_BSS_init)

extern "C" void praat_uvafon_David_init (void);
void praat_uvafon_David_init (void)
{
	Data_recognizeFileType (TextGrid_TIMITLabelFileRecognizer);
	Data_recognizeFileType (cmuAudioFileRecognizer);

    Thing_recognizeClassesByName (classActivation, classBarkFilter,
		classCategories, classCepstrum, classCCA,
		classChebyshevSeries,classClassificationTable, classConfusion,
    	classCorrelation, classCovariance, classDiscriminant, classDTW,
		classEigen, classExcitations, classFormantFilter, classIndex, classKlattTable,
		classPermutation,
		classISpline, classLegendreSeries,
		classMelFilter,
		classMSpline, classPattern, classPCA, classPolynomial, classRoots,
		classSimpleString, classStringsIndex, classSPINET, classSSCP, classSVD, NULL);

    praat_addMenuCommand (L"Objects", L"Goodies", L"Report floating point properties", 0, 0, DO_Praat_ReportFloatingPointProperties);

	praat_addMenuCommand (L"Objects", L"New", L"Create simple Covariance...", L"Create simple Matrix...", 1, DO_Covariance_createSimple);
 praat_addMenuCommand (L"Objects", L"New", L"Create Permutation...", 0, 0, DO_Permutation_create);
    praat_addMenuCommand (L"Objects", L"New", L"Polynomial", 0, 0, 0);
    	praat_addMenuCommand (L"Objects", L"New", L"Create Polynomial...", 0, 1, DO_Polynomial_create);
    	praat_addMenuCommand (L"Objects", L"New", L"Create LegendreSeries...", 0, 1, DO_LegendreSeries_create);
    	praat_addMenuCommand (L"Objects", L"New", L"Create ChebyshevSeries...", 0, 1, DO_ChebyshevSeries_create);
    	praat_addMenuCommand (L"Objects", L"New", L"Create MSpline...", 0, 1, DO_MSpline_create);
    	praat_addMenuCommand (L"Objects", L"New", L"Create ISpline...", 0, 1, DO_ISpline_create);
	praat_addMenuCommand (L"Objects", L"New", L"Create Sound from gammatone...", L"Create Sound from tone complex...", 1, DO_Sound_createFromGammaTone);
	praat_addMenuCommand (L"Objects", L"New", L"Create Sound from gamma-tone...", L"Create Sound from tone complex...", praat_DEPTH_1 | praat_HIDDEN, DO_Sound_createFromGammaTone);
	praat_addMenuCommand (L"Objects", L"New", L"Create Sound from Shepard tone...", L"Create Sound from gammatone...", 1, DO_Sound_createFromShepardTone);
	praat_addMenuCommand (L"Objects", L"New", L"Create Sound from VowelEditor...", L"Create Sound from Shepard tone...", praat_DEPTH_1, DO_VowelEditor_create);
	praat_addMenuCommand (L"Objects", L"New", L"Create formant table (Pols & Van Nierop 1973)", L"Create Table...", 1, DO_Table_createFromPolsVanNieropData);
	praat_addMenuCommand (L"Objects", L"New", L"Create formant table (Peterson & Barney 1952)", L"Create Table...", 1, DO_Table_createFromPetersonBarneyData);
	praat_addMenuCommand (L"Objects", L"New", L"Create formant table (Weenink 1985)", L"Create formant table (Peterson & Barney 1952)",1, DO_Table_createFromWeeninkData);
	praat_addMenuCommand (L"Objects", L"New", L"Create TableOfReal (Pols 1973)...", L"Create TableOfReal...", 1, DO_TableOfReal_createFromPolsData_50males);
	praat_addMenuCommand (L"Objects", L"New", L"Create TableOfReal (Van Nierop 1973)...", L"Create TableOfReal (Pols 1973)...", 1, DO_TableOfReal_createFromVanNieropData_25females);
	praat_addMenuCommand (L"Objects", L"New", L"Create TableOfReal (Weenink 1985)...", L"Create TableOfReal (Van Nierop 1973)...", 1, DO_TableOfReal_createFromWeeninkData);
	praat_addMenuCommand (L"Objects", L"New", L"Create KlattTable example", L"Create TableOfReal (Weenink 1985)...", praat_DEPTH_1+praat_HIDDEN, DO_KlattTable_createExample);

	praat_addMenuCommand (L"Objects", L"Open", L"Read Sound from raw 16-bit Little Endian file...", L"Read from special sound file", 1,
		 DO_Sound_readFromRawFileLE);
	praat_addMenuCommand (L"Objects", L"Open", L"Read Sound from raw 16-bit Big Endian file...", L"Read Sound from raw 16-bit Little Endian file...", 1, DO_Sound_readFromRawFileBE);
	praat_addMenuCommand (L"Objects", L"Open", L"Read KlattTable from raw text file...", L"Read Matrix from raw text file...", praat_HIDDEN, DO_KlattTable_readFromRawTextFile);

    praat_addAction1 (classActivation, 0, L"Modify", 0, 0, 0);
    praat_addAction1 (classActivation, 0, L"Formula...", 0, 0,
		DO_Activation_formula);
    praat_addAction1 (classActivation, 0, L"Hack", 0, 0, 0);
    praat_addAction1 (classActivation, 0, L"To Matrix", 0, 0,
		DO_Activation_to_Matrix);

	praat_addAction2 (classActivation, 1, classCategories, 1, L"To TableOfReal", 0, 0, DO_Matrix_Categories_to_TableOfReal);

	praat_addAction1 (classBarkFilter, 0, L"BarkFilter help", 0, 0, DO_BarkFilter_help);
	praat_FilterBank_all_init (classBarkFilter);
	praat_addAction1 (classBarkFilter, 0, L"Draw spectrum (slice)...", L"Draw filters...", 1, DO_BarkFilter_drawSpectrum);
	praat_addAction1 (classBarkFilter, 1, L"Draw filter functions...", L"Draw filters...", 1, DO_BarkFilter_drawSekeyHansonFilterFunctions);

    praat_addAction1 (classCategories, 0, L"Edit", 0, 0, DO_Categories_edit);
    praat_addAction1 (classCategories, 0, QUERY_BUTTON, 0, 0, 0);
	praat_addAction1 (classCategories, 1, L"Get number of categories", QUERY_BUTTON, 1, DO_Categories_getNumberOfCategories);
    praat_addAction1 (classCategories, 2, L"Get difference", QUERY_BUTTON, praat_HIDDEN | praat_DEPTH_1, DO_Categories_difference);
     praat_addAction1 (classCategories, 2, L"Get number of differences", QUERY_BUTTON, 1, DO_Categories_getNumberOfDifferences);
      praat_addAction1 (classCategories, 2, L"Get fraction different", QUERY_BUTTON, 1, DO_Categories_getFractionDifferent);
	praat_addAction1 (classCategories, 0, MODIFY_BUTTON, 0, 0, 0);
		praat_addAction1 (classCategories, 1, L"Append 1 category...", MODIFY_BUTTON,
		1, DO_Categories_append);
    praat_addAction1 (classCategories, 0, L"Extract", 0, 0, 0);
    praat_addAction1 (classCategories, 0, L"To unique Categories", 0, 0,
		DO_Categories_selectUniqueItems);
    praat_addAction1 (classCategories, 0, L"Analyse", 0, 0, 0);
    praat_addAction1 (classCategories, 2, L"To Confusion", 0, 0,
		DO_Categories_to_Confusion);
    praat_addAction1 (classCategories, 0, L"Synthesize", 0, 0, 0);
    praat_addAction1 (classCategories, 2, L"Join", 0, 0, DO_Categories_join);
    praat_addAction1 (classCategories, 0, L"Permute items", 0, 0, DO_Categories_permuteItems);
    praat_addAction1 (classCategories, 0, L"To Strings", 0, 0,
		DO_Categories_to_Strings);

	praat_addAction1 (classChebyshevSeries, 0, L"ChebyshevSeries help", 0, 0,
		DO_ChebyshevSeries_help);
	praat_FunctionTerms_init (classChebyshevSeries);
	praat_addAction1 (classChebyshevSeries, 0, L"To Polynomial", L"Analyse",
		0, DO_ChebyshevSeries_to_Polynomial);

	praat_addAction1 (classCCA, 1, L"Draw eigenvector...", 0, 0,
		 DO_CCA_drawEigenvector);
	praat_addAction1 (classCCA, 1, L"Get number of correlations", 0, 0,
		 DO_CCA_getNumberOfCorrelations);
	praat_addAction1 (classCCA, 1, L"Get correlation...", 0, 0,
		 DO_CCA_getCorrelationCoefficient);
	praat_addAction1 (classCCA, 1, L"Get eigenvector element...", 0, 0, DO_CCA_getEigenvectorElement);
	praat_addAction1 (classCCA, 1, L"Get zero correlation probability...", 0, 0, DO_CCA_getZeroCorrelationProbability);

	praat_addAction2 (classCCA, 1, classTableOfReal, 1, L"To TableOfReal (scores)...",
		0, 0, DO_CCA_and_TableOfReal_scores);
	praat_addAction2 (classCCA, 1, classTableOfReal, 1, L"To TableOfReal (loadings)",
		0, 0, DO_CCA_and_TableOfReal_factorLoadings);
	praat_addAction2 (classCCA, 1, classTableOfReal, 1, L"Predict...", 0, 0,
		DO_CCA_and_TableOfReal_predict);
	praat_addAction2 (classCCA, 1, classCorrelation, 1, L"To TableOfReal (loadings)",
		0, 0, DO_CCA_and_Correlation_factorLoadings);
	praat_addAction2 (classCCA, 1, classCorrelation, 1, L"Get variance fraction...",
		0, 0, DO_CCA_and_Correlation_getVarianceFraction);
	praat_addAction2 (classCCA, 1, classCorrelation, 1, L"Get redundancy (sl)...",
		0, 0, DO_CCA_and_Correlation_getRedundancy_sl);

	praat_addAction1 (classConfusion, 0, L"Confusion help", 0, 0,
		DO_Confusion_help);
    praat_TableOfReal_init2 (classConfusion);
	praat_removeAction (classConfusion, NULL, NULL, L"Draw as numbers...");
	praat_addAction1 (classConfusion, 0, L"Draw as numbers...", L"Draw -", 1, DO_Confusion_drawAsNumbers);
	praat_addAction1 (classConfusion, 0, L"-- confusion statistics --", L"Get value...", 1, 0);
	praat_addAction1 (classConfusion, 1, L"Get fraction correct", L"-- confusion statistics --", 1, DO_Confusion_getFractionCorrect);
	praat_addAction1 (classConfusion, 1, L"Get row sum...", L"Get fraction correct", 1, DO_TableOfReal_getRowSum);
 	praat_addAction1 (classConfusion, 1, L"Get column sum...", L"Get row sum...", 1, DO_TableOfReal_getColumnSum);
	praat_addAction1 (classConfusion, 1, L"Get grand sum", L"Get column sum...", 1, DO_TableOfReal_getGrandSum);
	praat_addAction1 (classConfusion, 0, L"To TableOfReal (marginals)", L"To TableOfReal", 0, DO_Confusion_to_TableOfReal_marginals);
	praat_addAction1 (classConfusion, 0, L"Analyse", 0, 0, 0);
	praat_addAction1 (classConfusion, 0, L"Condense...", 0, 0,
		DO_Confusion_condense);
    praat_addAction1 (classConfusion, 2, L"To difference matrix", 0, 0,
		DO_Confusion_difference);

    praat_addAction2 (classConfusion, 1, classMatrix, 1, L"Draw", 0, 0, 0);
    praat_addAction2 (classConfusion, 1, classMatrix, 1, L"Draw confusion...",
		0, 0, DO_Confusion_Matrix_draw);

	praat_addAction1 (classCovariance, 0, L"Covariance help", 0, 0,
		DO_Covariance_help);
    praat_SSCP_as_TableOfReal_init (classCovariance);
	praat_SSCP_query_init (classCovariance);
	praat_SSCP_extract_init (classCovariance);
	praat_addAction1 (classCovariance, 1, L"Get probability at position...", L"Get value...", 1, DO_Covariance_getProbabilityAtPosition);
	praat_addAction1 (classCovariance, 1, L"Get diagonality (bartlett)...", L"Get ln(determinant)", 1, DO_SSCP_testDiagonality_bartlett);
	praat_addAction1 (classCovariance, 1, L"Get significance of one mean...", L"Get diagonality (bartlett)...", 1, DO_Covariance_getSignificanceOfOneMean);
	praat_addAction1 (classCovariance, 1, L"Get significance of means difference...", L"Get significance of one mean...", 1, DO_Covariance_getSignificanceOfMeansDifference);
	praat_addAction1 (classCovariance, 1, L"Get significance of one variance...", L"Get significance of means difference...", 1, DO_Covariance_getSignificanceOfOneVariance);
	praat_addAction1 (classCovariance, 1, L"Get significance of variances ratio...", L"Get significance of one variance...", 1, DO_Covariance_getSignificanceOfVariancesRatio);
	praat_addAction1 (classCovariance, 1, L"Get fraction variance...", L"Get significance of variances ratio...", 1, DO_Covariance_getFractionVariance);
	praat_addAction1 (classCovariance, 2, L"Report multivariate mean difference...", L"Get fraction variance...", 1, DO_Covariances_reportMultivariateMeanDifference);
	praat_addAction1 (classCovariance, 2, L"Difference", L"Report multivariate mean difference...", praat_DEPTH_1 | praat_HIDDEN, DO_Covariances_reportEquality);
	praat_addAction1 (classCovariance, 0, L"Report equality of covariances", L"Report multivariate mean difference...", praat_DEPTH_1 | praat_HIDDEN, DO_Covariances_reportEquality);

	praat_addAction1 (classCovariance, 0, L"To TableOfReal (random sampling)...", 0, 0, DO_Covariance_to_TableOfReal_randomSampling);

	praat_addAction1 (classCovariance, 0, L"To Correlation", 0, 0, DO_Covariance_to_Correlation);
	praat_addAction1 (classCovariance, 0, L"To PCA", 0, 0, DO_Covariance_to_PCA);

	praat_addAction2 (classCovariance, 1, classTableOfReal, 1, L"To TableOfReal (mahalanobis)...", 0, 0, DO_Covariance_and_TableOfReal_mahalanobis);

	praat_addAction1 (classClassificationTable, 0, L"ClassificationTable help", 0, 0, DO_ClassificationTable_help);
	praat_TableOfReal_init (classClassificationTable);
	praat_addAction1 (classClassificationTable, 0, L"To Confusion", 0, 0, DO_ClassificationTable_to_Confusion);
	praat_addAction1 (classClassificationTable, 0, L"To Correlation (columns)", 0, 0, DO_ClassificationTable_to_Correlation_columns);
	praat_addAction1 (classClassificationTable, 0, L"To Strings (max. prob.)", 0, 0, DO_ClassificationTable_to_Strings_maximumProbability);

	praat_addAction1 (classCorrelation, 0, L"Correlation help", 0, 0, DO_Correlation_help);
    praat_TableOfReal_init2 (classCorrelation);
	praat_SSCP_query_init (classCorrelation);
	praat_SSCP_extract_init (classCorrelation);
	praat_addAction1 (classCorrelation, 1, L"Get diagonality (bartlett)...", L"Get ln(determinant)", 1, DO_Correlation_testDiagonality_bartlett);
	praat_addAction1 (classCorrelation, 0, L"Confidence intervals...", 0, 0, DO_Correlation_confidenceIntervals);
	praat_addAction1 (classCorrelation, 0, L"To PCA", 0, 0, DO_Correlation_to_PCA);

	praat_addAction1 (classDiscriminant, 0, L"Discriminant help", 0, 0, DO_Discriminant_help);
	praat_addAction1 (classDiscriminant, 0, DRAW_BUTTON, 0, 0, 0);
		praat_Eigen_draw_init (classDiscriminant);
		praat_addAction1 (classDiscriminant, 0, L"-- sscps --", 0, 1, 0);
		praat_addAction1 (classDiscriminant, 0, L"Draw sigma ellipses...", 0, 1, DO_Discriminant_drawSigmaEllipses);
		praat_addAction1 (classDiscriminant, 0, L"Draw one sigma ellipse...", 0, 1, DO_Discriminant_drawOneSigmaEllipse);
		praat_addAction1 (classDiscriminant, 0, L"Draw confidence ellipses...", 0, 1, DO_Discriminant_drawConfidenceEllipses);

    praat_addAction1 (classDiscriminant, 1, QUERY_BUTTON, 0, 0, 0);
		praat_addAction1 (classDiscriminant, 1, L"-- eigen structure --", 0, 1, 0);
		praat_Eigen_query_init (classDiscriminant);
		praat_addAction1 (classDiscriminant, 1, L"-- discriminant --", 0, 1, 0);
		praat_addAction1 (classDiscriminant, 1, L"Get number of functions", 0, 1, DO_Discriminant_getNumberOfFunctions);
		praat_addAction1 (classDiscriminant, 1, L"Get dimension of functions", 0, 1, DO_Discriminant_getDimensionOfFunctions);
		praat_addAction1 (classDiscriminant, 1, L"Get number of groups", 0, 1, DO_Discriminant_getNumberOfGroups);
		praat_addAction1 (classDiscriminant, 1, L"Get number of observations...", 0, 1, DO_Discriminant_getNumberOfObservations);
		praat_addAction1 (classDiscriminant, 1, L"-- tests --", 0, 1, 0);
		praat_addAction1 (classDiscriminant, 1, L"Get Wilks lambda...", 0, 1, DO_Discriminant_getWilksLambda);
		praat_addAction1 (classDiscriminant, 1, L"Get cumulative contribution of components...", 0, 1, DO_Discriminant_getCumulativeContributionOfComponents);
		praat_addAction1 (classDiscriminant, 1, L"Get partial discrimination probability...", 0, 1,
			DO_Discriminant_getPartialDiscriminationProbability);
		praat_addAction1 (classDiscriminant, 1, L"Get homogeneity of covariances (box)", 0, praat_DEPTH_1 | praat_HIDDEN,
			DO_Discriminant_getHomegeneityOfCovariances_box);
		praat_addAction1 (classDiscriminant, 1, L"Report equality of covariance matrices", 0, 1,
			DO_Discriminant_reportEqualityOfCovariances_wald);
		praat_addAction1 (classDiscriminant, 1, L"-- ellipses --", 0, 1, 0);
		praat_addAction1 (classDiscriminant, 1, L"Get sigma ellipse area...", 0, 1, DO_Discriminant_getConcentrationEllipseArea);
		praat_addAction1 (classDiscriminant, 1, L"Get confidence ellipse area...", 0, 1, DO_Discriminant_getConfidenceEllipseArea);
		praat_addAction1 (classDiscriminant, 1, L"Get ln(determinant_group)...", 0, 1, DO_Discriminant_getLnDeterminant_group);
		praat_addAction1 (classDiscriminant, 1, L"Get ln(determinant_total)", 0, 1, DO_Discriminant_getLnDeterminant_total);

	praat_addAction1 (classDiscriminant, 0, MODIFY_BUTTON, 0, 0, 0);
		praat_addAction1 (classDiscriminant, 1, L"Invert eigenvector...", 0, 1, DO_Discriminant_invertEigenvector);
		praat_addAction1 (classDiscriminant, 0, L"Align eigenvectors", 0, 1, DO_Eigens_alignEigenvectors);

	praat_addAction1 (classDiscriminant, 0, EXTRACT_BUTTON, 0, 0, 0);
		praat_addAction1 (classDiscriminant, 1, L"Extract pooled within-groups SSCP", 0, 1,
			DO_Discriminant_extractPooledWithinGroupsSSCP);
		praat_addAction1 (classDiscriminant, 1, L"Extract within-group SSCP...", 0, 1, DO_Discriminant_extractWithinGroupSSCP);
		praat_addAction1 (classDiscriminant, 1, L"Extract between-groups SSCP", 0, 1, DO_Discriminant_extractBetweenGroupsSSCP);
		praat_addAction1 (classDiscriminant, 1, L"Extract group centroids", 0, 1, DO_Discriminant_extractGroupCentroids);
		praat_addAction1 (classDiscriminant, 1, L"Extract group standard deviations", 0, 1, DO_Discriminant_extractGroupStandardDeviations);
		praat_addAction1 (classDiscriminant, 1, L"Extract group labels", 0, 1, DO_Discriminant_extractGroupLabels);

	praat_addAction1 (classDiscriminant , 0, L"& TableOfReal: To ClassificationTable?", 0, 0, DO_hint_Discriminant_and_TableOfReal_to_ClassificationTable);

/*		praat_addAction1 (classDiscriminant, 1, L"Extract coefficients...", 0, 1, DO_Discriminant_extractCoefficients);*/



	praat_Eigen_Matrix_project (classDiscriminant, classFormantFilter);
	praat_Eigen_Matrix_project (classDiscriminant, classBarkFilter);
	praat_Eigen_Matrix_project (classDiscriminant, classMelFilter);

	praat_addAction2 (classDiscriminant, 1, classPattern, 1, L"To Categories...", 0, 0, DO_Discriminant_and_Pattern_to_Categories);
	praat_addAction2 (classDiscriminant, 1, classSSCP, 1, L"Project", 0, 0, DO_Eigen_and_SSCP_project);
	praat_addAction2 (classDiscriminant, 1, classStrings, 1, L"Modify Discriminant", 0, 0, 0);
	praat_addAction2 (classDiscriminant, 1, classStrings, 1, L"Set group labels", 0, 0, DO_Discriminant_setGroupLabels);

	praat_addAction2 (classDiscriminant, 1, classTableOfReal, 1, L"To Configuration...", 0, 0, DO_Discriminant_and_TableOfReal_to_Configuration);
	praat_addAction2 (classDiscriminant, 1, classTableOfReal, 1, L"To ClassificationTable...", 0, 0,
		DO_Discriminant_and_TableOfReal_to_ClassificationTable);
	praat_addAction2 (classDiscriminant, 1, classTableOfReal, 1, L"To TableOfReal (mahalanobis)...", 0, 0, DO_Discriminant_and_TableOfReal_mahalanobis);


	praat_addAction1 (classDTW, 0, L"DTW help", 0, 0, DO_DTW_help);
	praat_addAction1 (classDTW, 0, L"Draw", 0, 0, 0);
    praat_addAction1 (classDTW, 0, L"Draw path...", 0, 0, DO_DTW_drawPath);
    praat_addAction1 (classDTW, 0, L"Paint distances...", 0, 0, DO_DTW_paintDistances);
    praat_addAction1 (classDTW, 0, L"Draw warp (x)...", 0, 0, DO_DTW_drawWarpX);
    praat_addAction1 (classDTW, 0, QUERY_BUTTON, 0, 0, 0);
    praat_addAction1 (classDTW, 1, L"Get distance (weighted)", 0, 1, DO_DTW_getWeightedDistance);
	praat_addAction1 (classDTW, 1, L"Get maximum consecutive steps...", 0, 1, DO_DTW_getMaximumConsecutiveSteps);
    praat_addAction1 (classDTW, 1, L"Get time along path...", 0, praat_DEPTH_1 | praat_HIDDEN, DO_DTW_getPathY);
    praat_addAction1 (classDTW, 1, L"Get y time...", 0, 1, DO_DTW_getYTime);
    praat_addAction1 (classDTW, 1, L"Get x time...", 0, 1, DO_DTW_getXTime);


    praat_addAction1 (classDTW, 0, L"Analyse", 0, 0, 0);
    praat_addAction1 (classDTW, 0, L"Find path...", 0, 0, DO_DTW_findPath);
	praat_addAction1 (classDTW, 0, L"Find path (band)...", 0, 0, DO_DTW_pathFinder_band);
#ifdef INCLUDE_DTW_SLOPES
	praat_addAction1 (classDTW, 0, L"Find path (slopes)...", 0, 0, DO_DTW_pathFinder_slopes);
#endif
	praat_addAction1 (classDTW, 0, L"To Polygon (band)...", 0, 0, DO_DTW_to_Polygon_band);
	praat_addAction1 (classDTW, 0, L"To Polygon (slopes)...", 0, 0, DO_DTW_to_Polygon_slopes);
    praat_addAction1 (classDTW, 0, L"To Matrix (distances)", 0, 0, DO_DTW_distancesToMatrix);
	praat_addAction1 (classDTW, 0, L"Swap axes", 0, 0, DO_DTW_swapAxes);

	praat_addAction2 (classDTW, 1, classTextGrid, 1, L"To TextGrid (warp times)", 0, 0, DO_DTW_and_TextGrid_to_TextGrid);

	praat_addAction2 (classDTW, 1, classSound, 2, L"Draw...", 0, 0, DO_DTW_and_Sounds_draw);
	praat_addAction2 (classDTW, 1, classSound, 2, L"Draw warp (x)...", 0, 0, DO_DTW_and_Sounds_drawWarpX);

	praat_Index_init (classStringsIndex);
    praat_addAction1 (classIndex, 0, L"Index help", 0, 0, DO_Index_help);
	praat_addAction1 (classStringsIndex, 1, L"Get class label...", 0, 0, DO_StringsIndex_getClassLabel);
    praat_addAction1 (classStringsIndex, 1, L"Get class index...", 0, 0, DO_StringsIndex_getClassIndex);
    praat_addAction1 (classStringsIndex, 1, L"Get label...", 0, 0, DO_StringsIndex_getLabel);
    praat_addAction1 (classIndex, 1, L"Get index...", 0, 0, DO_Index_getIndex);
	praat_addAction1 (classStringsIndex, 1, L"To Strings", 0, 0, DO_StringsIndex_to_Strings);

    praat_addAction1 (classExcitation, 0, L"Synthesize", L"To Formant...", 0, 0);
    praat_addAction1 (classExcitation, 0, L"To Excitations", L"Synthesize", 0, DO_Excitation_to_Excitations);

    praat_addAction1 (classExcitations, 0, L"Modify", 0, 0, 0);
    praat_addAction1 (classExcitations, 0, L"Formula...", 0, 0, DO_Excitations_formula);
    praat_addAction1 (classExcitations, 0, L"Extract", 0, 0, 0);
    praat_addAction1 (classExcitations, 0, L"Extract Excitation...", 0, 0, DO_Excitations_getItem);
    praat_addAction1 (classExcitations, 0, L"Synthesize", 0, 0, 0);
    praat_addAction1 (classExcitations, 0, L"Append", 0, 0, DO_Excitations_append);
    praat_addAction1 (classExcitations, 0, L"Convert", 0, 0, 0);
    praat_addAction1 (classExcitations, 0, L"To Pattern...", 0, 0, DO_Excitations_to_Pattern);
    praat_addAction1 (classExcitations, 0, L"To TableOfReal", 0, 0, DO_Excitations_to_TableOfReal);

    praat_addAction2 (classExcitations, 1, classExcitation, 0, L"Add to Excitations", 0, 0, DO_Excitations_addItem);


	praat_addAction1 (classFormantFilter, 0, L"FormantFilter help", 0, 0, DO_FormantFilter_help);
	praat_FilterBank_all_init (classFormantFilter);
	praat_addAction1 (classFormantFilter, 0, L"Draw spectrum (slice)...", L"Draw filters...", 1, DO_FormantFilter_drawSpectrum);
	praat_addAction1 (classFormantFilter, 0, L"Draw filter functions...", L"Draw filters...", 1, DO_FormantFilter_drawFilterFunctions);
	praat_addAction1 (classFormantGrid, 0, L"Draw...", L"Edit", 1, DO_FormantGrid_draw);

	praat_addAction1 (classIntensity, 0, L"To TextGrid (silences)...", L"To IntensityTier (valleys)", 0, DO_Intensity_to_TextGrid_detectSilences);

	praat_addAction1 (classISpline, 0, L"ISpline help", 0, 0, DO_ISpline_help);
	praat_Spline_init (classISpline);

	praat_addAction1 (classKlattTable, 0, L"KlattTable help", 0, 0, DO_KlattTable_help);
	praat_addAction1 (classKlattTable, 0, L"To Sound...", 0, 0, DO_KlattTable_to_Sound);
	praat_addAction1 (classKlattTable, 0, L"To KlattGrid...", 0, 0, DO_KlattTable_to_KlattGrid);
	praat_addAction1 (classKlattTable, 0, L"To Table", 0, 0, DO_KlattTable_to_Table);

	praat_addAction1 (classLegendreSeries, 0, L"LegendreSeries help", 0, 0, DO_LegendreSeries_help);
	praat_FunctionTerms_init (classLegendreSeries);
	praat_addAction1 (classLegendreSeries, 0, L"To Polynomial", L"Analyse", 0, DO_LegendreSeries_to_Polynomial);

	praat_addAction1 (classLongSound, 0, L"Append to existing sound file...", 0, 0, DO_LongSounds_appendToExistingSoundFile);
	praat_addAction1 (classSound, 0, L"Append to existing sound file...", 0, 0, DO_LongSounds_appendToExistingSoundFile);
	praat_addAction2 (classLongSound, 0, classSound, 0, L"Append to existing sound file...", 0, 0, DO_LongSounds_appendToExistingSoundFile);

	praat_addAction1 (classLongSound, 2, L"Save as stereo AIFF file...", L"Save as NIST file...", 1, DO_LongSounds_writeToStereoAiffFile);
	praat_addAction1 (classLongSound, 2, L"Write to stereo AIFF file...", L"Write to NIST file...", praat_HIDDEN + praat_DEPTH_1, DO_LongSounds_writeToStereoAiffFile);
	praat_addAction1 (classLongSound, 2, L"Save as stereo AIFC file...", L"Save as stereo AIFF file...", 1, DO_LongSounds_writeToStereoAifcFile);
	praat_addAction1 (classLongSound, 2, L"Write to stereo AIFC file...", L"Write to stereo AIFF file...", praat_HIDDEN + praat_DEPTH_1, DO_LongSounds_writeToStereoAifcFile);
	praat_addAction1 (classLongSound, 2, L"Save as stereo WAV file...", L"Save as stereo AIFC file...", 1, DO_LongSounds_writeToStereoWavFile);
	praat_addAction1 (classLongSound, 2, L"Write to stereo WAV file...", L"Write to stereo AIFC file...", praat_HIDDEN + praat_DEPTH_1, DO_LongSounds_writeToStereoWavFile);
	praat_addAction1 (classLongSound, 2, L"Save as stereo NeXt/Sun file...", L"Save as stereo WAV file...", 1, DO_LongSounds_writeToStereoNextSunFile);
	praat_addAction1 (classLongSound, 2, L"Write to stereo NeXt/Sun file...", L"Write to stereo WAV file...", praat_HIDDEN + praat_DEPTH_1, DO_LongSounds_writeToStereoNextSunFile);
	praat_addAction1 (classLongSound, 2, L"Save as stereo NIST file...", L"Save as stereo NeXt/Sun file...", 1, DO_LongSounds_writeToStereoNistFile);
	praat_addAction1 (classLongSound, 2, L"Write to stereo NIST file...", L"Write to stereo NeXt/Sun file...", praat_HIDDEN + praat_DEPTH_1, DO_LongSounds_writeToStereoNistFile);

	praat_addAction1 (classMatrix, 0, L"Scatter plot...", L"Paint cells...", 1, DO_Matrix_scatterPlot);
	praat_addAction1 (classMatrix, 0, L"Draw as squares...", L"Scatter plot...", 1, DO_Matrix_drawAsSquares);
	praat_addAction1 (classMatrix, 0, L"Draw distribution...", L"Draw as squares...", 1, DO_Matrix_drawDistribution);
	praat_addAction1 (classMatrix, 0, L"Draw cumulative distribution...", L"Draw distribution...", 1, DO_Matrix_drawCumulativeDistribution);
	praat_addAction1 (classMatrix, 0, L"Transpose", L"Synthesize", 0, DO_Matrix_transpose);
	praat_addAction1 (classMatrix, 0, L"Solve equation...", L"Analyse", 0, DO_Matrix_solveEquation);
    praat_addAction1 (classMatrix, 0, L"To Pattern...", L"To VocalTract", 1, DO_Matrix_to_Pattern);
	praat_addAction1 (classMatrix, 0, L"To Activation", L"To Pattern...", 1, DO_Matrix_to_Activation);
	praat_addAction1 (classMatrix, 2, L"To DTW...", L"To ParamCurve", 1, DO_Matrices_to_DTW);

	praat_addAction2 (classMatrix, 1, classCategories, 1, L"To TableOfReal", 0, 0, DO_Matrix_Categories_to_TableOfReal);

	praat_addAction1 (classMelFilter, 0, L"MelFilter help", 0, 0, DO_MelFilter_help);
	praat_FilterBank_all_init (classMelFilter);
	praat_addAction1 (classMelFilter, 0, L"Draw spectrum (slice)...", L"Draw filters...", 1, DO_MelFilter_drawSpectrum);
	praat_addAction1 (classMelFilter, 0, L"Draw filter functions...", L"Draw filters...", 1, DO_MelFilter_drawFilterFunctions);
	praat_addAction1 (classMelFilter, 0, L"To MFCC...", 0, 0, DO_MelFilter_to_MFCC);

	praat_addAction1 (classMFCC, 0, L"MFCC help", 0, 0, DO_MFCC_help);
	praat_CC_init (classMFCC);
	praat_addAction1 (classMFCC, 0, L"To MelFilter...", 0, 0, DO_MFCC_to_MelFilter);

	praat_addAction1 (classMSpline, 0, L"MSpline help", 0, 0, DO_MSpline_help);
	praat_Spline_init (classMSpline);

    praat_addAction1 (classPattern, 0, L"Draw", 0, 0, 0);
    praat_addAction1 (classPattern, 0, L"Draw...", 0, 0, DO_Pattern_draw);
	praat_addAction1 (classPattern, 0, MODIFY_BUTTON, 0, 0, 0);
    praat_addAction1 (classPattern, 0, L"Formula...", 0, 1, DO_Pattern_formula);
    praat_addAction1 (classPattern, 0, L"Set value...", 0, 1, DO_Pattern_setValue);
    praat_addAction1 (classPattern, 0, L"To Matrix", 0, 0, DO_Pattern_to_Matrix);

    praat_addAction2 (classPattern, 1, classCategories, 1, L"To TableOfReal", 0, 0, DO_Matrix_Categories_to_TableOfReal);

    praat_addAction2 (classPattern, 1, classCategories, 1, L"To Discriminant", 0, 0, DO_Pattern_and_Categories_to_Discriminant);

	praat_addAction1 (classPCA, 0, L"PCA help", 0, 0, DO_PCA_help);
	praat_addAction1 (classPCA, 0, DRAW_BUTTON, 0, 0, 0);
		praat_Eigen_draw_init (classPCA);
	praat_addAction1 (classPCA, 0, QUERY_BUTTON, 0, 0, 0);
		praat_Eigen_query_init (classPCA);
		praat_addAction1 (classPCA, 1, L"-- pca --", 0, 1, 0);
		praat_addAction1 (classPCA, 1, L"Get equality of eigenvalues...", 0, 1, DO_PCA_getEqualityOfEigenvalues);
		praat_addAction1 (classPCA, 1, L"Get fraction variance accounted for...", 0, 1, DO_PCA_getFractionVAF);
		praat_addAction1 (classPCA, 1, L"Get number of components (VAF)...", 0, 1, DO_PCA_getNumberOfComponentsVAF);
		praat_addAction1 (classPCA, 2, L"Get angle between pc1-pc2 planes", 0, 1, DO_PCAs_getAngleBetweenPc1Pc2Plane_degrees);
	praat_addAction1 (classPCA, 0, MODIFY_BUTTON, 0, 0, 0);
		praat_addAction1 (classPCA, 1, L"Invert eigenvector...", 0, 1, DO_PCA_invertEigenvector);
		praat_addAction1 (classPCA, 0, L"Align eigenvectors", 0, 1, DO_Eigens_alignEigenvectors);
	praat_addAction1 (classPCA, 2, L"To Procrustes...", 0, 0, DO_PCAs_to_Procrustes);
	praat_addAction1 (classPCA, 0, L"To TableOfReal (reconstruct 1)...", 0, 0, DO_PCA_to_TableOfReal_reconstruct1);
	praat_addAction1 (classPCA, 0, L"& TableOfReal: To Configuration?", 0, 0, DO_hint_PCA_and_TableOfReal_to_Configuration);
	praat_addAction1 (classPCA, 0, L"& Configuration (reconstruct)?", 0, 0, DO_hint_PCA_and_Configuration_to_TableOfReal_reconstruct);
	praat_addAction1 (classPCA, 0, L"& Covariance: Project?", 0, 0, DO_hint_PCA_and_Covariance_Project);
	praat_addAction2 (classPCA, 1, classConfiguration, 1, L"To TableOfReal (reconstruct)", 0, 0, DO_PCA_and_Configuration_to_TableOfReal_reconstruct);
	praat_addAction2 (classPCA, 1, classSSCP, 1, L"Project", 0, 0, DO_Eigen_and_SSCP_project);
	praat_addAction2 (classPCA, 1, classTableOfReal, 1, L"To Configuration...", 0, 0, DO_PCA_and_TableOfReal_to_Configuration);
	praat_addAction2 (classPCA, 1, classTableOfReal, 1, L"Get fraction variance...", 0, 0, DO_PCA_and_TableOfReal_getFractionVariance);
	praat_addAction2 (classPCA, 1, classCovariance, 1, L"Project", 0, 0, DO_Eigen_and_Covariance_project);

	praat_Eigen_Matrix_project (classPCA, classFormantFilter);
	praat_Eigen_Matrix_project (classPCA, classBarkFilter);
	praat_Eigen_Matrix_project (classPCA, classMelFilter);

	praat_addAction1 (classPermutation, 0, L"Permutation help", 0, 0, DO_Permutation_help);
	praat_addAction1 (classPermutation, 0, QUERY_BUTTON, 0, 0, 0);
	praat_addAction1 (classPermutation, 1, L"Get number of elements", 0, 1, DO_Permutation_getNumberOfElements);
	praat_addAction1 (classPermutation, 1, L"Get value...", 0, 1, DO_Permutation_getValueAtIndex);
	praat_addAction1 (classPermutation, 1, L"Get index...", 0, 1, DO_Permutation_getIndexAtValue);
	praat_addAction1 (classPermutation, 0, MODIFY_BUTTON, 0, 0, 0);
	praat_addAction1 (classPermutation, 1, L"Sort", 0, 1, DO_Permutation_sort);
	praat_addAction1 (classPermutation, 1, L"Swap blocks...", 0, 1, DO_Permutation_swapBlocks);
	praat_addAction1 (classPermutation, 1, L"Swap numbers...", 0, 1, DO_Permutation_swapNumbers);
	praat_addAction1 (classPermutation, 1, L"Swap positions...", 0, 1, DO_Permutation_swapPositions);
	praat_addAction1 (classPermutation, 1, L"Swap one from range...", 0, 1, DO_Permutation_swapOneFromRange);
	praat_addAction1 (classPermutation, 0, L"-- sequential permutations --", 0, 1, 0);
	praat_addAction1 (classPermutation, 0, L"Next", 0, 1, DO_Permutations_next);
	praat_addAction1 (classPermutation, 0, L"Previous", 0, 1, DO_Permutations_previous);
	praat_addAction1 (classPermutation, 1, L"Permute randomly...", 0, 0, DO_Permutation_permuteRandomly);
	praat_addAction1 (classPermutation, 1, L"Permute randomly (blocks)...", 0, 0, DO_Permutation_permuteBlocksRandomly);
	praat_addAction1 (classPermutation, 1, L"Interleave...", 0, 0, DO_Permutation_interleave);
	praat_addAction1 (classPermutation, 1, L"Rotate...", 0, 0, DO_Permutation_rotate);
	praat_addAction1 (classPermutation, 1, L"Reverse...", 0, 0, DO_Permutation_reverse);
	praat_addAction1 (classPermutation, 1, L"Invert", 0, 0, DO_Permutation_invert);
	praat_addAction1 (classPermutation, 0, L"Multiply", 0, 0, DO_Permutations_multiply);

	praat_addAction1 (classPitch, 2, L"To DTW...", L"To PointProcess", praat_HIDDEN, DO_Pitches_to_DTW);

	praat_addAction1 (classPitchTier, 0, L"To Pitch...", L"To Sound (sine)...", 1, DO_PitchTier_to_Pitch);
	praat_addAction1 (classPolygon, 0, L"Translate...", L"Modify", 0, DO_Polygon_translate);
	praat_addAction1 (classPolygon, 0, L"Rotate...", L"Translate...", 0, DO_Polygon_rotate);
	praat_addAction1 (classPolygon, 0, L"Scale...", L"Rotate...", 0, DO_Polygon_scale);
	praat_addAction1 (classPolygon, 0, L"Reverse X", L"Scale...", 0, DO_Polygon_reverseX);
	praat_addAction1 (classPolygon, 0, L"Reverse Y", L"Reverse X", 0, DO_Polygon_reverseY);

	praat_addAction2 (classPolygon, 1, classCategories, 1, L"Draw...", 0, 0, DO_Polygon_Categories_draw);

	praat_addAction1 (classPolynomial, 0, L"Polynomial help", 0, 0, DO_Polynomial_help);
	praat_FunctionTerms_init (classPolynomial);
		praat_addAction1 (classPolynomial, 0, L"-- area --", L"Get x of maximum...", 1, 0);
		praat_addAction1 (classPolynomial, 1, L"Get area...", L"-- area --", 1, DO_Polynomial_getArea);
		praat_addAction1 (classPolynomial, 0, L"-- monic --", L"Set coefficient...", 1, 0);
		praat_addAction1 (classPolynomial, 0, L"Scale coefficients (monic)", L"-- monic --", 1, DO_Polynomial_scaleCoefficients_monic);
	praat_addAction1 (classPolynomial, 1, L"Get value (complex)...", L"Get value...", 1, DO_Polynomial_evaluate_z);
	praat_addAction1 (classPolynomial, 0, L"To Spectrum...", L"Analyse", 0, DO_Polynomial_to_Spectrum);
	praat_addAction1 (classPolynomial, 0, L"To Roots", 0, 0, DO_Polynomial_to_Roots);
	praat_addAction1 (classPolynomial, 0, L"To Polynomial (derivative)", 0, 0, DO_Polynomial_getDerivative);
	praat_addAction1 (classPolynomial, 0, L"To Polynomial (primitive)", 0, 0, DO_Polynomial_getPrimitive);
	praat_addAction1 (classPolynomial, 0, L"Scale x...", 0, 0, DO_Polynomial_scaleX);
	praat_addAction1 (classPolynomial, 2, L"Multiply", 0, 0, DO_Polynomials_multiply);
	praat_addAction1 (classPolynomial, 2, L"Divide...", 0, 0, DO_Polynomials_divide);

	praat_addAction1 (classRoots, 1, L"Roots help", 0, 0, DO_Roots_help);
	praat_addAction1 (classRoots, 1, L"Draw...", 0, 0, DO_Roots_draw);
	praat_addAction1 (classRoots, 1, QUERY_BUTTON, 0, 0, 0);
		praat_addAction1 (classRoots, 1, L"Get number of roots", 0, 1, DO_Roots_getNumberOfRoots);
		praat_addAction1 (classRoots, 1, L"-- roots --", 0, 1, 0);
		praat_addAction1 (classRoots, 1, L"Get root...", 0, 1, DO_Roots_getRoot);
		praat_addAction1 (classRoots, 1, L"Get real part of root...", 0, 1, DO_Roots_getRealPartOfRoot);
		praat_addAction1 (classRoots, 1, L"Get imaginary part of root...", 0, 1, DO_Roots_getImaginaryPartOfRoot);
	praat_addAction1 (classRoots, 1, MODIFY_BUTTON, 0, 0, 0);
		praat_addAction1 (classRoots, 1, L"Set root...", 0, 1, DO_Roots_setRoot);
	praat_addAction1 (classRoots, 0, L"Analyse", 0, 0, 0);
	praat_addAction1 (classRoots, 0, L"To Spectrum...", 0, 0, DO_Roots_to_Spectrum);

	praat_addAction2 (classRoots, 1, classPolynomial, 1,L"Polish roots", 0, 0, DO_Roots_and_Polynomial_polish);

	praat_addAction1 (classSound, 1, L"Save as raw 16-bit Big Endian file...", 0, 0, DO_Sound_writeToRawFileBE);
	praat_addAction1 (classSound, 1, L"Write to raw 16-bit Big Endian file...", 0, praat_HIDDEN, DO_Sound_writeToRawFileBE);
	praat_addAction1 (classSound, 1, L"Save as raw 16-bit Little Endian file...", 0, 0, DO_Sound_writeToRawFileLE);
	praat_addAction1 (classSound, 1, L"Write to raw 16-bit Little Endian file...", 0, praat_HIDDEN, DO_Sound_writeToRawFileLE);

	praat_addAction1 (classSound, 0, L"To TextGrid (silences)...", L"To IntervalTier", 1, DO_Sound_to_TextGrid_detectSilences);

	praat_addAction1 (classSound, 0, L"Draw where...", L"Draw...", 1, DO_Sound_drawWhere);
//	praat_addAction1 (classSound, 0, L"Paint where...", L"Draw where...", praat_DEPTH_1 | praat_HIDDEN, DO_Sound_paintWhere);
	praat_addAction1 (classSound, 0, L"Paint where...", L"Draw where...", 1, DO_Sound_paintWhere);
//	praat_addAction1 (classSound, 2, L"Paint enclosed...", L"Paint where...", praat_DEPTH_1 | praat_HIDDEN, DO_Sounds_paintEnclosed);
	praat_addAction1 (classSound, 2, L"Paint enclosed...", L"Paint where...", 1, DO_Sounds_paintEnclosed);

	praat_addAction1 (classSound, 0, L"To Pitch (shs)...", L"To Pitch (cc)...", 1, DO_Sound_to_Pitch_shs);
	praat_addAction1 (classSound, 0, L"Fade in...", L"Multiply by window...", praat_HIDDEN + praat_DEPTH_1, DO_Sound_fadeIn);
	praat_addAction1 (classSound, 0, L"Fade out...", L"Fade in...", praat_HIDDEN + praat_DEPTH_1, DO_Sound_fadeOut);
	praat_addAction1 (classSound, 0, L"To Pitch (SPINET)...", L"To Pitch (cc)...", 1, DO_Sound_to_Pitch_SPINET);

	praat_addAction1 (classSound, 0, L"To FormantFilter...", L"To Cochleagram (edb)...", 1, DO_Sound_to_FormantFilter);

	praat_addAction1 (classSound, 0, L"To BarkFilter...", L"To FormantFilter...", 1, DO_Sound_to_BarkFilter);

	praat_addAction1 (classSound, 0, L"To MelFilter...", L"To BarkFilter...", 1, DO_Sound_to_MelFilter);

	praat_addAction1 (classSound, 0, L"To Polygon...", L"Down to Matrix", praat_DEPTH_1 | praat_HIDDEN, DO_Sound_to_Polygon);
	praat_addAction1 (classSound, 2, L"To Polygon (enclosed)...", L"Cross-correlate...", praat_DEPTH_1 | praat_HIDDEN, DO_Sounds_to_Polygon_enclosed);

	praat_addAction1 (classSound, 0, L"Filter (gammatone)...", L"Filter (formula)...", 1, DO_Sound_filterByGammaToneFilter4);

	praat_addAction1 (classSound, 0, L"Change gender...", L"Deepen band modulation...", 1, DO_Sound_changeGender);

	praat_addAction1 (classSound, 0, L"Change speaker...", L"Deepen band modulation...", praat_DEPTH_1 | praat_HIDDEN, DO_Sound_changeSpeaker);
	praat_addAction1 (classSound, 0, L"To KlattGrid (simple)...", L"To Manipulation...", 1, DO_Sound_to_KlattGrid_simple);
	praat_addAction2 (classSound, 1, classPitch, 1, L"To FormantFilter...", 0, 0, DO_Sound_and_Pitch_to_FormantFilter);

	praat_addAction2 (classSound, 1, classPitch, 1, L"Change gender...", 0, 0, DO_Sound_and_Pitch_changeGender);
	praat_addAction2 (classSound, 1, classPitch, 1, L"Change speaker...", 0, praat_HIDDEN, DO_Sound_and_Pitch_changeSpeaker);

    praat_addAction1 (classSpectrogram, 2, L"To DTW...", L"To Spectrum (slice)...", 0, DO_Spectrograms_to_DTW);

	praat_addAction1 (classSpectrum, 0, L"Draw phases...", L"Draw (log freq)...", 1, DO_Spectrum_drawPhases);
	praat_addAction1 (classSpectrum, 0, L"Conjugate", L"Formula...", praat_HIDDEN | praat_DEPTH_1, DO_Spectrum_conjugate);
	praat_addAction1 (classSpectrum, 2, L"Multiply", L"To Sound (fft)", praat_HIDDEN, DO_Spectra_multiply);
	praat_addAction1 (classSpectrum, 0, L"To Matrix (unwrap)", L"To Matrix", 0, DO_Spectrum_unwrap);
	praat_addAction1 (classSpectrum, 0, L"To Cepstrum", L"To Spectrogram", 0, DO_Spectrum_to_Cepstrum);

	praat_addAction1 (classSSCP, 0, L"SSCP help", 0, 0, DO_SSCP_help);
	praat_TableOfReal_init2 (classSSCP);
	praat_removeAction (classSSCP, NULL, NULL, L"Append");
	praat_addAction1 (classSSCP, 0, L"Draw sigma ellipse...", DRAW_BUTTON, 1, DO_SSCP_drawSigmaEllipse);
	praat_addAction1 (classSSCP, 0, L"Draw confidence ellipse...", DRAW_BUTTON, 1, DO_SSCP_drawConfidenceEllipse);
	praat_SSCP_query_init (classSSCP);
	praat_addAction1 (classSSCP, 1, L"Get diagonality (bartlett)...", L"Get ln(determinant)", 1, DO_SSCP_testDiagonality_bartlett);
	praat_addAction1 (classSSCP, 1, L"Get total variance", L"Get diagonality (bartlett)...", 1, DO_SSCP_getTotalVariance);
	praat_addAction1 (classSSCP, 1, L"Get sigma ellipse area...", L"Get total variance", 1, DO_SSCP_getConcentrationEllipseArea);
	praat_addAction1 (classSSCP, 1, L"Get confidence ellipse area...", L"Get sigma ellipse area...", 1, DO_SSCP_getConfidenceEllipseArea);
	praat_addAction1 (classSSCP, 1, L"Get fraction variation...", L"Get confidence ellipse area...", 1, DO_SSCP_getFractionVariation);
	praat_SSCP_extract_init (classSSCP);
	praat_addAction1 (classSSCP, 0, L"To PCA", 0, 0, DO_SSCP_to_PCA);
	praat_addAction1 (classSSCP, 0, L"To Correlation", 0, 0, DO_SSCP_to_Correlation);
	praat_addAction1 (classSSCP, 0, L"To Covariance...", 0, 0, DO_SSCP_to_Covariance);

	praat_addAction1 (classStrings, 0, L"To Categories", 0, 0, DO_Strings_to_Categories);
	praat_addAction1 (classStrings, 0, L"Append", 0, 0, DO_Strings_append);
	praat_addAction1 (classStrings, 1, L"Set string...", L"Genericize", 0, DO_Strings_setString);
	praat_addAction1 (classStrings, 0, L"Change...", L"Set string...", 0, DO_Strings_change);
	praat_addAction1 (classStrings, 0, L"Extract part...", L"Change...", 0, DO_Strings_extractPart);
	praat_addAction1 (classStrings, 0, L"To Permutation...", L"To Distributions", 0, DO_Strings_to_Permutation);

	praat_addAction1 (classSVD, 0, L"To TableOfReal...", 0, 0, DO_SVD_to_TableOfReal);
	praat_addAction1 (classSVD, 0, L"Extract left singular vectors", 0, 0, DO_SVD_extractLeftSingularVectors);
	praat_addAction1 (classSVD, 0, L"Extract right singular vectors", 0, 0, DO_SVD_extractRightSingularVectors);
	praat_addAction1 (classSVD, 0, L"Extract singular values", 0, 0, DO_SVD_extractSingularValues);

	praat_addAction1 (classTable, 0, L"Scatter plot (ci)...", 0, praat_DEPTH_1|praat_HIDDEN, DO_Table_drawScatterPlotWithConfidenceIntervals);
	praat_addAction1 (classTable, 0, L"To KlattTable", 0, praat_HIDDEN, DO_Table_to_KlattTable);

	praat_addAction1 (classTableOfReal, 1, L"Report multivariate normality...", L"Get column stdev (label)...",
		praat_DEPTH_1|praat_HIDDEN, DO_TableOfReal_reportMultivariateNormality);
	praat_addAction1 (classTableOfReal, 0, L"Append columns", L"Append", 1, DO_TableOfReal_appendColumns);
	praat_addAction1 (classTableOfReal, 0, L"Multivariate statistics -", 0, 0, 0);
	praat_addAction1 (classTableOfReal, 0, L"To Discriminant", 0, 1, DO_TableOfReal_to_Discriminant);
	praat_addAction1 (classTableOfReal, 0, L"To PCA", 0, 1, DO_TableOfReal_to_PCA);
	praat_addAction1 (classTableOfReal, 0, L"To SSCP...", 0, 1, DO_TableOfReal_to_SSCP);
	praat_addAction1 (classTableOfReal, 0, L"To Covariance", 0, 1, DO_TableOfReal_to_Covariance);
	praat_addAction1 (classTableOfReal, 0, L"To Correlation", 0, 1, DO_TableOfReal_to_Correlation);
	praat_addAction1 (classTableOfReal, 0, L"To Correlation (rank)", 0, 1, DO_TableOfReal_to_Correlation_rank);
	praat_addAction1 (classTableOfReal, 0, L"To CCA...", 0, 1, DO_TableOfReal_to_CCA);
	praat_addAction1 (classTableOfReal, 0, L"To TableOfReal (means by row labels)...", 0, 1, DO_TableOfReal_meansByRowLabels);
	praat_addAction1 (classTableOfReal, 0, L"To TableOfReal (medians by row labels)...", 0, 1, DO_TableOfReal_mediansByRowLabels);

	praat_addAction1 (classTableOfReal, 0, L"-- configurations --", 0, 1, 0);
	praat_addAction1 (classTableOfReal, 0, L"To Configuration (pca)...",	0, 1, DO_TableOfReal_to_Configuration_pca);
	praat_addAction1 (classTableOfReal, 0, L"To Configuration (lda)...", 0, 1, DO_TableOfReal_to_Configuration_lda);
	praat_addAction1 (classTableOfReal, 2, L"-- between tables --", L"To Configuration (lda)...", 1, 0);
	praat_addAction1 (classTableOfReal, 2, L"To TableOfReal (cross-correlations)...", 0, praat_HIDDEN + praat_DEPTH_1, DO_TableOfReal_and_TableOfReal_crossCorrelations);


	praat_addAction1 (classTableOfReal, 1, L"To Pattern and Categories...", L"To Matrix", 1, DO_TableOfReal_to_Pattern_and_Categories);
	praat_addAction1 (classTableOfReal, 1, L"Split into Pattern and Categories...", L"To Pattern and Categories...", praat_DEPTH_1 | praat_HIDDEN, DO_TableOfReal_to_Pattern_and_Categories);
	praat_addAction1 (classTableOfReal, 0, L"To Permutation (sort row labels)", L"To Matrix", 1, DO_TableOfReal_to_Permutation_sortRowlabels);

	praat_addAction1 (classTableOfReal, 1, L"To SVD", 0, praat_HIDDEN, DO_TableOfReal_to_SVD);
	praat_addAction1 (classTableOfReal, 2, L"To GSVD", 0, praat_HIDDEN, DO_TablesOfReal_to_GSVD);
	praat_addAction1 (classTableOfReal, 2, L"To Eigen (gsvd)", 0, praat_HIDDEN, DO_TablesOfReal_to_Eigen_gsvd);

 	praat_addAction1 (classTableOfReal, 0, L"To TableOfReal (cholesky)...", 0, praat_HIDDEN, DO_TableOfReal_choleskyDecomposition);

	praat_addAction1 (classTableOfReal, 0, L"-- scatter plots --", L"Draw top and bottom lines...", 1, 0);
	praat_addAction1 (classTableOfReal, 0, L"Draw scatter plot...", L"-- scatter plots --", 1, DO_TableOfReal_drawScatterPlot);
	praat_addAction1 (classTableOfReal, 0, L"Draw scatter plot matrix...", L"Draw scatter plot...", 1, DO_TableOfReal_drawScatterPlotMatrix);
	praat_addAction1 (classTableOfReal, 0, L"Draw box plots...", L"Draw scatter plot matrix...", 1, DO_TableOfReal_drawBoxPlots);
	praat_addAction1 (classTableOfReal, 0, L"Draw biplot...", L"Draw box plots...", 1, DO_TableOfReal_drawBiplot);
	praat_addAction1 (classTableOfReal, 0, L"Draw vectors...", L"Draw box plots...", praat_DEPTH_1 | praat_HIDDEN, DO_TableOfReal_drawVectors);
	praat_addAction1 (classTableOfReal, 1, L"Draw row as histogram...", L"Draw biplot...", praat_DEPTH_1 | praat_HIDDEN, DO_TableOfReal_drawRowAsHistogram);
	praat_addAction1 (classTableOfReal, 1, L"Draw rows as histogram...", L"Draw row as histogram...", praat_DEPTH_1 | praat_HIDDEN, DO_TableOfReal_drawRowsAsHistogram);
	praat_addAction1 (classTableOfReal, 1, L"Draw column as distribution...", L"Draw rows as histogram...", praat_DEPTH_1, DO_TableOfReal_drawColumnAsDistribution);

	praat_addAction2 (classStrings, 1, classPermutation, 1, L"Permute strings", 0, 0, DO_Strings_and_Permutation_permuteStrings);

	praat_addAction2 (classTableOfReal, 1, classPermutation, 1, L"Permute rows",	0, 0, DO_TableOfReal_and_Permutation_permuteRows);

	praat_addAction1 (classTextGrid, 0, L"Extend time...", L"Scale times...", 2, DO_TextGrid_extendTime);
	praat_addAction1 (classTextGrid, 1, L"Set tier name...", L"Remove tier...", 1, DO_TextGrid_setTierName);
			praat_addAction1 (classTextGrid, 0, L"Replace interval text...", L"Set interval text...", 2, DO_TextGrid_replaceIntervalTexts);
			praat_addAction1 (classTextGrid, 0, L"Replace point text...", L"Set point text...", 2, DO_TextGrid_replacePointTexts);

	praat_uvafon_MDS_init();
	praat_KlattGrid_init();
	praat_HMM_init();
	praat_BSS_init();
}

/* End of file praat_David.c */
