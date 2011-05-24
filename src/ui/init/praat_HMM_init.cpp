/* praat_HMM_init.c
 *
 * Copyright (C) 2010 David Weenink
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
	djmw 20101003
*/

#include "ui/praat.h"

#include "dwtools/GaussianMixture.h"
#include "dwtools/HMM.h"
#include "dwtools/Strings_extensions.h"
#include "stat/TableOfReal.h"
#include "ui/Interpreter.h"

void SSCPs_drawConcentrationEllipses (SSCPs me, Graphics g, double scale,
	int confidence, wchar_t *label, long d1, long d2, double xmin, double xmax,
	double ymin, double ymax, int fontSize, int garnish);

#undef praat_HIDDEN
#define praat_HIDDEN 0

#define GaussianMixture_OPTION_MENU_CRITERIA \
	OPTIONMENU (L"Criterion based on", 1) \
	OPTION (L"Likelihood") \
	OPTION (L"Message length") \
	OPTION (L"Bayes information") \
	OPTION (L"Akaike information") \
	OPTION (L"Akaike corrected") \
	OPTION (L"Complete-data ML")

Correlation GaussianMixture_and_TableOfReal_to_Correlation2 (GaussianMixture me, thou);

void GaussianMixture_and_PCA_drawMarginalPdf (GaussianMixture me, PCA thee, Graphics g, long d, double xmin, double xmax, double ymin, double ymax, long npoints, long nbins, int garnish)
{
	if (my dimension != thy dimension || d < 1 || d > my dimension)
	{ Melder_warning1 (L"Dimensions don't agree."); return;}

	if (npoints <= 1) npoints = 1000;
	double *p = NUMdvector (1, npoints);
	if (p == 0) return;
	double nsigmas = 2;

	if (xmax <= xmin &&
		! GaussianMixture_and_PCA_getIntervalAlongDirection (me, thee, d, nsigmas, &xmin, &xmax)) goto end;

	{
		double pmax = 0, dx = (xmax - xmin) / npoints, x1 = xmin + 0.5 * dx;
		double scalef = nbins <= 0 ? 1 : 1; // TODO
		for (long i = 1; i <= npoints; i++)
		{
			double x = x1 + (i - 1) * dx;
			p[i] = scalef * GaussianMixture_getMarginalProbabilityAtPosition (me, thy eigenvectors[d], x);
			if (p[i] > pmax) pmax = p[i];
		}
		if (ymin >= ymax) { ymin = 0; ymax = pmax; }

		Graphics_setInner (g);
		Graphics_setWindow (g, xmin, xmax, ymin, ymax);
		Graphics_function (g, p, 1, npoints, x1, xmax - 0.5 * dx);
		Graphics_unsetInner (g);

		if (garnish)
		{
			Graphics_drawInnerBox (g);
			Graphics_markBottom (g, xmin, 1, 1, 0, NULL);
			Graphics_markBottom (g, xmax, 1, 1, 0, NULL);
			Graphics_markLeft (g, ymin, 1, 1, 0, NULL);
			Graphics_markLeft (g, ymax, 1, 1, 0, NULL);
		}
	}
end:
	NUMdvector_free (p, 1);
}

void GaussianMixture_drawMarginalPdf (GaussianMixture me, Graphics g, long d, double xmin, double xmax, double ymin, double ymax, long npoints, long nbins, int garnish)
{
	if (d < 1 || d > my dimension)
	{ Melder_warning1 (L"Dimension doesn't agree."); return;}
	if (npoints <= 1) npoints = 1000;
	double *p = NUMdvector (1, npoints);
	if (p == 0) return;
	double nsigmas = 2, *v = NUMdvector (1, my dimension);
	if (v == NULL) goto end;

	if (xmax <= xmin &&
		! GaussianMixture_getIntervalAlongDirection (me, d, nsigmas, &xmin, &xmax)) goto end;

	{
		double pmax = 0, dx = (xmax - xmin) / (npoints - 1);
		double scalef = nbins <= 0 ? 1 : 1; // TODO
		for (long i = 1; i <= npoints; i++)
		{
			double x = xmin + (i - 1) * dx;
			for (long k = 1; k <= my dimension; k++) v[k] = k == d ? 1 : 0;
			p[i] = scalef * GaussianMixture_getMarginalProbabilityAtPosition (me, v, x);
			if (p[i] > pmax) pmax = p[i];
		}
		if (ymin >= ymax) { ymin = 0; ymax = pmax; }

		Graphics_setInner (g);
		Graphics_setWindow (g, xmin, xmax, ymin, ymax);
		Graphics_function (g, p, 1, npoints, xmin, xmax);
		Graphics_unsetInner (g);

		if (garnish)
		{
			Graphics_drawInnerBox (g);
			Graphics_markBottom (g, xmin, 1, 1, 0, NULL);
			Graphics_markBottom (g, xmax, 1, 1, 0, NULL);
			Graphics_markLeft (g, ymin, 1, 1, 0, NULL);
			Graphics_markLeft (g, ymax, 1, 1, 0, NULL);
		}
	}

end:
	NUMdvector_free (p, 1); NUMdvector_free (v, 1);
}

void GaussianMixture_and_PCA_drawConcentrationEllipses (GaussianMixture me, PCA him, Graphics g, double scale,
	int confidence, wchar_t *label, long d1, long d2,
	double xmin, double xmax, double ymin, double ymax, int fontSize, int garnish)
{
	if (my dimension != his dimension) { Melder_warning1 (L"Dimensions don't agree."); return;}
	SSCPs thee = NULL;
	int d1_inverted = 0, d2_inverted = 0;

	if (d1 < 0) { d1 = abs(d1); Eigen_invertEigenvector (him, d1); d1_inverted = 1; }
	if (d2 < 0) { d2 = abs(d2); Eigen_invertEigenvector (him, d2); d2_inverted = 1; }

	if ((thee = SSCPs_toTwoDimensions ((SSCPs) my covariances, his eigenvectors[d1], his eigenvectors[d2])) == NULL) goto end;

	if (d1_inverted) Eigen_invertEigenvector (him, d1);
	if (d2_inverted) Eigen_invertEigenvector (him, d2);

	SSCPs_drawConcentrationEllipses (thee, g, -scale, confidence, label, 1, 2,
		xmin, xmax, ymin, ymax, fontSize, 0);

	if (garnish)
	{
		wchar_t label[40];
    	Graphics_drawInnerBox (g);
    	Graphics_marksLeft (g, 2, 1, 1, 0);
    	swprintf (label, 40, L"pc %ld", d2);
    	Graphics_textLeft (g, 1, label);
    	Graphics_marksBottom (g, 2, 1, 1, 0);
    	swprintf (label, 40, L"pc %ld", d1);
		Graphics_textBottom (g, 1, label);
	}
end:
	forget (thee);
}

void GaussianMixture_drawConcentrationEllipses (GaussianMixture me, Graphics g,
	double scale, int confidence, wchar_t *label, int pcaDirections, long d1, long d2,
	double xmin, double xmax, double ymin, double ymax, int fontSize, int garnish)
{
	if (d1 == 0 && d2 == 0) { d1 = 1; d2 = 2; }
	if (abs(d1) > my dimension || abs(d2) > my dimension) return;

	if (! pcaDirections)
	{
		SSCPs_drawConcentrationEllipses ((SSCPs) my covariances, g, -scale, confidence, label,
			abs(d1), abs(d2), xmin, xmax, ymin, ymax, fontSize, garnish);
		return;
	}

	PCA him = GaussianMixture_to_PCA (me);
	if (him == NULL) return;

	GaussianMixture_and_PCA_drawConcentrationEllipses (me, him, g, scale, confidence, label, d1, d2,
	xmin, xmax, ymin, ymax, fontSize, garnish);

	forget (him);
}

DIRECT (GaussianMixture_help)
	Melder_help (L"GaussianMixture");
END

FORM (GaussianMixture_drawConcentrationEllipses, L"GaussianMixture: Draw concentration ellipses",
	L"GaussianMixture: Draw concentration ellipses...")
	POSITIVE (L"Number of sigmas", L"1.0")
	BOOLEAN (L"Principal component plane", 1)
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
	EVERY_DRAW (GaussianMixture_drawConcentrationEllipses ((structGaussianMixture *)OBJECT, GRAPHICS,
		GET_REAL (L"Number of sigmas"), 0, NULL, GET_INTEGER (L"Principal component plane"),
		GET_INTEGER (L"X-dimension"), GET_INTEGER (L"Y-dimension"),
		GET_REAL (L"left Horizontal range"), GET_REAL (L"right Horizontal range"),
		GET_REAL (L"left Vertical range"), GET_REAL (L"right Vertical range"),
		GET_INTEGER (L"Label size"), GET_INTEGER (L"Garnish")))
END

FORM (GaussianMixture_and_PCA_drawConcentrationEllipses, L"GaussianMixture & PCA: Draw concentration ellipses",
	L"GaussianMixture & PCA: Draw concentration ellipses...")
	POSITIVE (L"Number of sigmas", L"1.0")
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
	praat_picture_open ();
	GaussianMixture_and_PCA_drawConcentrationEllipses ((structGaussianMixture *)ONLY (classGaussianMixture), (structPCA *)ONLY (classPCA), GRAPHICS,
		GET_REAL (L"Number of sigmas"), 0, NULL, GET_INTEGER (L"X-dimension"), GET_INTEGER (L"Y-dimension"),
		GET_REAL (L"left Horizontal range"), GET_REAL (L"right Horizontal range"),
		GET_REAL (L"left Vertical range"), GET_REAL (L"right Vertical range"),
		GET_INTEGER (L"Label size"), GET_INTEGER (L"Garnish"));
	praat_picture_close ();
END

FORM (GaussianMixture_drawMarginalPdf, L"GaussianMixture: Draw marginal pdf", L"GaussianMixture: Draw marginal pdf...")
	INTEGER (L"X-dimension", L"1")
	REAL (L"left Horizontal range", L"0.0")
	REAL (L"right Horizontal range", L"0.0")
	REAL (L"left Vertical range", L"0.0")
	REAL (L"right Vertical range", L"0.0")
	NATURAL (L"Number of points", L"500")
	INTEGER (L"Number of bins", L"0.0")
	BOOLEAN (L"Garnish", 1)
	OK
DO
	EVERY_DRAW (GaussianMixture_drawMarginalPdf ((structGaussianMixture *)OBJECT, GRAPHICS,
		GET_INTEGER (L"X-dimension"), GET_REAL (L"left Horizontal range"), GET_REAL (L"right Horizontal range"),
		GET_REAL (L"left Vertical range"), GET_REAL (L"right Vertical range"),
		GET_INTEGER (L"Number of points"), GET_INTEGER (L"Number of bins"), GET_INTEGER (L"Garnish")))
END

DIRECT (GaussianMixture_getNumberOfComponents)
	GaussianMixture me = (structGaussianMixture *)ONLY_OBJECT;
	Melder_information2 (Melder_integer (my numberOfComponents), L" (= number of components)");
END

DIRECT (GaussianMixture_getDimensionOfComponent)
	GaussianMixture me = (structGaussianMixture *)ONLY_OBJECT;
	Melder_information2 (Melder_integer (my dimension), L" (= dimension of component)");
END

FORM (GaussianMixture_getProbabilityAtPosition, L"GaussianMixture: Get probability at position", 0)
	SENTENCE (L"Position", L"100.0 300.0")
	OK
DO
	wchar_t *position = GET_STRING (L"Position");
	double p = GaussianMixture_getProbabilityAtPosition_string ((structGaussianMixture *)ONLY_OBJECT, position);
	Melder_information3 (Melder_double (p), L" (= probability at position ", position);
END

FORM (GaussianMixture_splitComponent, L"GaussianMixture: Split component", L"GaussianMixture: Split component...")
	NATURAL (L"Component", L"1")
	OK
DO
	if (! GaussianMixture_splitComponent ((structGaussianMixture *)ONLY (classGaussianMixture), GET_INTEGER (L"Component"))) return 0;
END

FORM (GaussianMixture_and_PCA_drawMarginalPdf, L"GaussianMixture & PCA: Draw pdf function", L"GaussianMixture: Draw marginal pdf...")
	INTEGER (L"X-dimension", L"1")
	REAL (L"left Horizontal range", L"0.0")
	REAL (L"right Horizontal range", L"0.0")
	REAL (L"left Vertical range", L"0.0")
	REAL (L"right Vertical range", L"0.0")
	NATURAL (L"Number of points", L"500")
	INTEGER (L"Number of bins", L"0.0")
	BOOLEAN (L"Garnish", 1)
	OK
DO
	praat_picture_open ();
	GaussianMixture_and_PCA_drawMarginalPdf ((structGaussianMixture *)ONLY (classGaussianMixture), (structPCA *)ONLY (classPCA), GRAPHICS,
		GET_INTEGER (L"X-dimension"), GET_REAL (L"left Horizontal range"), GET_REAL (L"right Horizontal range"),
		GET_REAL (L"left Vertical range"), GET_REAL (L"right Vertical range"),
		GET_INTEGER (L"Number of points"), GET_INTEGER (L"Number of bins"), GET_INTEGER (L"Garnish"));
	praat_picture_close ();
END

FORM (GaussianMixture_and_PCA_to_Matrix_density, L"GaussianMixture & PCA: To Matrix density", L"GaussianMixture & PCA: To Matrix (density)...")
	INTEGER (L"X-dimension", L"1")
	INTEGER (L"Y-dimension", L"2")
	REAL (L"left Horizontal range", L"0.0")
	REAL (L"right Horizontal range", L"0.0")
	NATURAL (L"Number of columns", L"100")
	REAL (L"left Vertical range", L"0.0")
	REAL (L"right Vertical range", L"0.0")
	NATURAL (L"Number of rows", L"100")
	OK
DO
	GaussianMixture me = (structGaussianMixture *)ONLY (classGaussianMixture);
	PCA pca = (structPCA *)ONLY (classPCA);
	if ( ! praat_new3 (GaussianMixture_and_PCA_to_Matrix_density (me, pca, GET_INTEGER (L"X-dimension"),
		GET_INTEGER (L"Y-dimension"), GET_REAL (L"left Horizontal range"), GET_REAL (L"right Horizontal range"),
		GET_INTEGER (L"Number of columns"), GET_REAL (L"left Vertical range"), GET_REAL (L"right Vertical range"), GET_INTEGER (L"Number of rows")), Thing_getName (me), L"_", Thing_getName (pca))) return 0;
END

FORM (GaussianMixture_extractComponent, L"GaussianMixture: Extract component", 0)
	NATURAL (L"Component", L"1")
	OK
DO
	long component = GET_INTEGER (L"Component");
	WHERE (SELECTED)
	{
		Covariance cov = GaussianMixture_extractComponent ((structGaussianMixture *)OBJECT, component);
		if (! praat_new3 (cov, NAME, L"_", Thing_getName (cov))) return 0;
	}
END

DIRECT (GaussianMixture_extractCentroids)
	EVERY_TO (GaussianMixture_extractCentroids ((structGaussianMixture *)OBJECT))
END

DIRECT (GaussianMixture_extractMixingProbabilities)
	EVERY_TO (GaussianMixture_extractMixingProbabilities ((structGaussianMixture *)OBJECT))
END

DIRECT (GaussianMixture_to_PCA)
	EVERY_TO (GaussianMixture_to_PCA ((structGaussianMixture *)OBJECT))
END

FORM (GaussianMixture_to_TableOfReal_randomSampling, L"GaussianMixture: To TableOfReal (random sampling)", L"GaussianMixture: To TableOfReal (random sampling)...")
	NATURAL (L"Number of data points", L"100")
	OK
DO
	long numberOfpoints = GET_INTEGER (L"Number of data points");
	EVERY_TO (GaussianMixture_to_TableOfReal_randomSampling ((structGaussianMixture *)OBJECT, numberOfpoints))
END

DIRECT (GaussianMixture_to_Covariance_between)
	EVERY_CHECK (praat_new2 (GaussianMixture_to_Covariance_between ((structGaussianMixture *)OBJECT), NAME, L"_b"))
END

DIRECT (GaussianMixture_to_Covariance_within)
	EVERY_CHECK (praat_new2 (GaussianMixture_to_Covariance_within ((structGaussianMixture *)OBJECT), NAME, L"_w"))
END

DIRECT (GaussianMixture_to_Covariance_total)
	EVERY_CHECK (praat_new2 (GaussianMixture_to_Covariance_total ((structGaussianMixture *)OBJECT), NAME, L"_t"))
END

FORM (GaussianMixture_and_TableOfReal_getLikelihoodValue, L"GaussianMixture & TableOfReal: Get likelihood value",
	L"GaussianMixture & TableOfReal: Get likelihood value...")
	GaussianMixture_OPTION_MENU_CRITERIA
OK
DO
	TableOfReal thee = (structTableOfReal *)ONLY (classTableOfReal);
	int criterion = GET_INTEGER (L"Criterion based on") - 1;
	wchar_t *criterionText = GaussianMixture_criterionText (criterion);
	double lnpdn = GaussianMixture_and_TableOfReal_getLikelihoodValue ((structGaussianMixture *)ONLY (classGaussianMixture), thee, criterion);
	Melder_information6 (Melder_double (lnpdn / thy numberOfRows), L" (= ", criterionText, L", n = ",
		Melder_integer (thy numberOfRows), L")");
END

void HMM_draw (HMM me, Graphics g, int garnish)
{
	long is, js;
	double *xs = NULL, *ys = NULL;
	double xwidth = sqrt (my numberOfStates);
	double rstate = 0.3 / xwidth, r = xwidth / 3.0;
	double xmax = 1.2 * xwidth / 2, xmin = -xmax, ymin = xmin, ymax = xmax;

	if (((xs = NUMdvector (1, my numberOfStates)) == NULL) ||
		((ys = NUMdvector (1, my numberOfStates)) == NULL)) goto end;

	{
		Graphics_setWindow (g, xmin, xmax, ymin, ymax);
		// heuristic: all states on a circle until we have a better graph drawing algorithm.
		xs[1] = ys[1] = 0;
		if (my numberOfStates > 1)
		{
			for (is = 1; is <= my numberOfStates; is++)
			{
				double alpha = - NUMpi + NUMpi * 2 * (is - 1) / my numberOfStates;
				xs[is] = r * cos (alpha); ys[is] = r * sin (alpha);
			}
		}
		// reorder the positions such that state number 1 is most left and last state number is right.
		// if > 5 may be one in the middle with the most connections
		// ...
		// find fontsize
		int fontSize = Graphics_inqFontSize (g);
		wchar_t *widest_label = L"";
		double max_width = 0;
		for (is = 1; is <= my numberOfStates; is++)
		{
			HMM_State hmms = (structHMM_State *)my states -> item[is];
			double w = hmms -> label == NULL ? 0 : Graphics_textWidth (g, hmms -> label);
			if (w > max_width)
			{
				widest_label = hmms -> label;
				max_width = w;
			}
		}
		int new_fontSize = fontSize;
		while (max_width > 2 * rstate && new_fontSize > 4)
		{
			new_fontSize --;
			Graphics_setFontSize (g, new_fontSize);
			max_width = Graphics_textWidth (g, widest_label);
		}
		Graphics_setFontSize (g, new_fontSize);
		Graphics_setTextAlignment (g, Graphics_CENTRE, Graphics_HALF);
		for (is = 1; is <= my numberOfStates; is++)
		{
			HMM_State hmms = (structHMM_State *)my states -> item[is];
			Graphics_circle (g, xs[is], ys[is], rstate);
			Graphics_text (g, xs[is], ys[is], hmms -> label);
		}

		// draw connections from is to js
		// 1 -> 2 / 2-> : increase / decrease angle between 1 and 2 with pi /10
		// use cos(a+b) and cos(a -b) rules
		double cosb = cos (NUMpi/10), sinb = sin (NUMpi / 10);
		for (is = 1; is <= my numberOfStates; is++)
		{
			double x1 = xs[is], y1 = ys[is];
			for (js = 1; js <= my numberOfStates; js++)
			{
				if (my transitionProbs[is][js] > 0 && is != js)
				{
					double x2 = xs[js], y2 = ys[js];
					double dx = x2 - x1, dy = y2 - y1, h = sqrt (dx * dx + dy * dy), cosa = dx / h, sina = dy / h;
					double cosabp = cosa * cosb - sina * sinb, cosabm = cosa * cosb + sina * sinb;
					double sinabp = cosa * sinb + sina * cosb, sinabm = -cosa * sinb + sina * cosb;
					Graphics_arrow (g, x1 + rstate * cosabp, y1 + rstate * sinabp, x2 - rstate * cosabm, y2 - rstate * sinabm);
				}
				if (is == js)
				{
					double dx = - x1, dy = - y1, h = sqrt (dx * dx + dy * dy), cosa = dx / h, sina = dy / h;
					Graphics_doubleArrow (g, x1 - rstate * cosa, y1 - rstate * sina, x1 - 1.4 * rstate * cosa, y1 - 1.4 * rstate * sina);
				}
			}
		}
		if (garnish) Graphics_drawInnerBox (g);
	}
end:
	NUMdvector_free (xs, 1);
	NUMdvector_free (ys, 1);
}

DIRECT (HMM_help)
	Melder_help (L"HMM");
END

FORM (HMM_create, L"Create HMM", L"ui/editors/AmplitudeTierEditor.h")
	WORD (L"Name", L"hmm")
	BOOLEAN (L"Left to right model", 0)
	NATURAL (L"Number of states", L"3")
	NATURAL (L"Number of observations", L"3")
	OK
DO
	if (! praat_new1 (HMM_create (GET_INTEGER (L"Left to right model"), GET_INTEGER (L"Number of states"),
		GET_INTEGER (L"Number of observations")), GET_STRING (L"Name"))) return 0;
END

FORM (HMM_createSimple, L"HMM: Create simple", L"HMM: Create simple HMM...")
	WORD (L"Name", L"weather")
	BOOLEAN (L"Left to right model", 0)
	SENTENCE (L"States", L"Rainy Sunny")
	SENTENCE (L"Observations", L"Walk Shop Clean")
	OK
DO
	if (! praat_new1 (HMM_createSimple (GET_INTEGER (L"Left to right model"), GET_STRING (L"States"),
		GET_STRING (L"Observations")), GET_STRING (L"Name"))) return 0;
END

FORM (HMM_createContinuousModel, L"HMM: Create continuous model", 0)
	WORD (L"Name", L"cm")
	BOOLEAN (L"Left to right model", 0)
	NATURAL (L"Number of states", L"3")
	NATURAL (L"Number of symbols", L"10")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"For the Gaussian mixtures:")
	NATURAL (L"Number of components", L"3")
	NATURAL (L"Dimension of component", L"39")
	OPTIONMENU (L"Covariance matrices are", 1)
		OPTION (L"Complete")
		OPTION (L"Diagonal")
	OK
DO
	long componentStorage = GET_INTEGER (L"Covariance matrices are") - 1;
	long dimension = GET_INTEGER (L"Dimension of component");
	REQUIRE (componentStorage >= 0 && componentStorage <= dimension, L"Not a valid covariance matrix type")
	if (! praat_new1 (HMM_createContinuousModel (GET_INTEGER (L"Left to right model"),
		GET_INTEGER (L"Number of states"), GET_INTEGER (L"Number of symbols"),
		GET_INTEGER (L"Number of components"), dimension, componentStorage),
		GET_STRING (L"Name"))) return 0;
END

FORM (HMM_ObservationSequence_to_HMM, L"HMM_ObservationSequence: To HMM", 0)
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"(0 states gives a non-hidden model) ")
	INTEGER (L"Number of states", L"2")
	BOOLEAN (L"Left to right model", 0)
	OK
DO
	long numberOfStates = GET_INTEGER (L"Number of states");
	EVERY_CHECK (praat_new3 (HMM_createFromHMM_ObservationSequence ((structHMM_ObservationSequence *)OBJECT, numberOfStates, GET_INTEGER (L"Left to right model")),
		Thing_getName (OBJECT), L"_", Melder_integer (numberOfStates)))
END

FORM (HMM_draw, L"HMM: Draw", 0)
	BOOLEAN (L"Garnish", 0)
	OK
DO
	EVERY_DRAW (HMM_draw ((structHMM *)OBJECT, GRAPHICS, GET_INTEGER (L"Garnish")))
END

// xc1 < xc2
void HMM_and_HMM_StateSequence_drawTrellis (HMM me, HMM_StateSequence thee, Graphics g, int connect, int garnish)
{
	long numberOfTimes = thy numberOfStrings;
	StringsIndex si = HMM_and_HMM_StateSequence_to_StringsIndex (me, thee);
	double xmin = 0, xmax = numberOfTimes + 1, ymin = 0.5, ymax = my numberOfStates + 0.5;
	if (si == NULL) return;

	Graphics_setInner (g);
	Graphics_setWindow (g, xmin, xmax, ymin, ymax);

	double r = 0.2 / (numberOfTimes > my numberOfStates ? numberOfTimes : my numberOfStates);

	for (long it = 1; it <= numberOfTimes; it++)
	{
		for (long js = 1; js <= my numberOfStates; js++)
		{
			double xc = it, yc = js, x2 = it, y2 = js;
			Graphics_circle (g, xc, yc, r);
			if (it > 1)
			{
				for (long is = 1; is <= my numberOfStates; is++)
				{
					bool indexedConnection = si -> classIndex[it - 1] == is && si -> classIndex[it] == js;
					Graphics_setLineWidth (g, indexedConnection ? 2 : 1);
					Graphics_setLineType (g, indexedConnection ? Graphics_DRAWN : Graphics_DOTTED);
					double x1 = it - 1, y1 = is;
					if (connect || indexedConnection)
					{
						double a = (y1 - y2) / (x1 - x2), b = y1 - a * x1;
						// double xs11 = x1 - r / (a * a + 1), ys11 = a * xs11 + b;
						double xs12 = x1 + r / (a * a + 1), ys12 = a * xs12 + b;
						double xs21 = x2 - r / (a * a + 1), ys21 = a * xs21 + b;
						// double xs22 = x2 + r / (a * a + 1), ys22 = a * xs22 + b;
						Graphics_line (g, xs12, ys12, xs21, ys21);
					}
				}
			}
		}
	}
	Graphics_unsetInner (g);
	Graphics_setLineWidth (g, 1);
	Graphics_setLineType (g, Graphics_DRAWN);
	if (garnish)
	{
		Graphics_drawInnerBox (g);
		for (long js = 1; js <= my numberOfStates; js++)
		{
			HMM_State hmms = (structHMM_State *)my states -> item[js];
			Graphics_markLeft (g, js, 0, 0, 0, hmms -> label);
		}
		Graphics_marksBottomEvery (g, 1, 1, 1, 1, 0);
		Graphics_textBottom (g, 1, L"Time index");
	}
	forget (si);
}

FORM (HMM_and_HMM_StateSequence_drawTrellis, L"HMM & Strings: Draw trellis", 0)
	BOOLEAN (L"Connect", 1);
	BOOLEAN (L"Garnish", 1);
	OK
DO
	praat_picture_open ();
	HMM_and_HMM_StateSequence_drawTrellis ((structHMM *)ONLY (classHMM), (structHMM_StateSequence *)ONLY (classHMM_StateSequence), GRAPHICS, GET_INTEGER (L"Connect"),
		GET_INTEGER (L"Garnish"));
	praat_picture_close ();
	return 1;
END

FORM (HMM_getTransitionProbability, L"HMM: Get transition probability", L"HMM: Get transition probability...")
	NATURAL (L"From state number", L"1")
	NATURAL (L"To state number", L"1")
	OK
DO
	HMM me = (structHMM *)ONLY_OBJECT;
	long s1 = GET_INTEGER (L"From state number"), s2 = GET_INTEGER (L"To state number");
	REQUIRE (s1 <= my numberOfStates && s2 <= my numberOfStates, L"State number(s) too high.")
	Melder_information6 (Melder_double (my transitionProbs[s1][s2]), L" : [ ", Melder_integer(s1), L", ", Melder_integer(s2), L" ]");
END

FORM (HMM_getEmissionProbability, L"HMM: Get emission probability", L"HMM: Get emission probability...")
	NATURAL (L"From state number", L"1")
	NATURAL (L"To symbol number", L"1")
	OK
DO
	HMM me = (structHMM *)ONLY_OBJECT;
	long s1 = GET_INTEGER (L"From state number"), s2 = GET_INTEGER (L"To symbol number");
	REQUIRE (s1 <= my numberOfStates, L"State number too high.")
	REQUIRE (s2 <= my numberOfObservationSymbols, L"Symbol number too high.")
	Melder_information6 (Melder_double (my emissionProbs[s1][s2]), L" : [ ", Melder_integer(s1), L", ", Melder_integer(s2), L" ]");
END

FORM (HMM_getStartProbability, L"HMM: Get start probability", L"HMM: Get start probability...")
	NATURAL (L"State number", L"1")
	OK
DO
	HMM me = (structHMM *)ONLY_OBJECT;
	long s1 = GET_INTEGER (L"State number");
	REQUIRE (s1 <= my numberOfStates, L"State number too high.")
	Melder_information4 (Melder_double (my transitionProbs[0][s1]), L" : [ ", Melder_integer(s1), L" ]");
END

FORM (HMM_getProbabilityAtTimeBeingInState, L"HMM: Get probability of being in state at time",
		L"HMM: Get p (time, state)...")
	NATURAL (L"Time index", L"10")
	NATURAL (L"State number", L"1")
	OK
DO
	long istate = GET_INTEGER (L"State number");
	long itime = GET_INTEGER (L"Time index");
	double lnp = HMM_getProbabilityAtTimeBeingInState ((structHMM *)ONLY_OBJECT, itime, istate);
	Melder_information7 (Melder_double (lnp), L" (=ln(p), p = ", Melder_naturalLogarithm (lnp), L") Being in state ",
		Melder_integer (istate), L" at time ", Melder_integer (itime));
END

FORM (HMM_getProbabilityAtTimeBeingInStateEmittingSymbol, L"HMM: get probability being at time in state emitting symbol", L"HMM: Get p (time, state, symbol)...")
	NATURAL (L"Time index", L"10")
	NATURAL (L"State number", L"1")
	NATURAL (L"Symbol number", L"1")
	OK
DO
	long istate = GET_INTEGER (L"State number");
	long itime = GET_INTEGER (L"Time index");
	long isymbol= GET_INTEGER (L"Symbol number");
	double lnp = HMM_getProbabilityAtTimeBeingInStateEmittingSymbol ((structHMM *)ONLY_OBJECT, itime, istate, isymbol);
	Melder_information9 (Melder_double (lnp), L" (=ln(p), p = ", Melder_naturalLogarithm (lnp), L") Being in state ",
		Melder_integer (istate), L" emitting symbol ", Melder_integer (isymbol), L" at time ", Melder_integer (itime));
END

FORM (HMM_getProbabilityOfStayingInState, L"HMM: Get probability of staying in state",
	L"HMM: Get probability staying in state...")
	NATURAL (L"State number", L"1")
	NATURAL (L"Number of times", L"2")
	OK
DO
	Melder_informationReal (HMM_getProbabilityOfStayingInState ((structHMM *)ONLY_OBJECT, GET_INTEGER (L"State number"),
		GET_INTEGER (L"Number of times")), NULL);
END

FORM (HMM_getExpectedValueOfDurationInState, L"HMM: Get expected value of duration in state",
	L"HMM: Get expected duration in state...")
	NATURAL (L"State number", L"1")
	OK
DO
	Melder_informationReal (HMM_getExpectedValueOfDurationInState ((structHMM *)ONLY_OBJECT, GET_INTEGER (L"State number")),
		L" time units");
END

FORM (HMM_getSymbolLabel, L"HMM: Get symbol label", 0)
	NATURAL (L"Symbol number", L"1")
	OK
DO
	HMM me = (structHMM *)ONLY_OBJECT;
	long is = GET_INTEGER (L"Symbol number");
	REQUIRE (is <= my numberOfObservationSymbols, L"Symbol number too high.")
	HMM_Observation s = (structHMM_Observation *)my observationSymbols -> item[is];
	Melder_information1 (s -> label);
END

FORM (HMM_getStateLabel, L"HMM: Get state label", 0)
	NATURAL (L"State number", L"1")
	OK
DO
	HMM me = (structHMM *)ONLY_OBJECT;
	long is = GET_INTEGER (L"State number");
	REQUIRE (is <= my numberOfStates, L"State number too high.")
	HMM_State s = (structHMM_State *)my states -> item[is];
	Melder_information1 (s -> label);
END

FORM (HMM_and_HMM_getCrossEntropy, L"HMM & HMM: Get cross-entropy...", L"HMM & HMM: Get cross-entropy...")
	NATURAL (L"Observation length", L"2000")
	BOOLEAN (L"Symmetric", 1)
	OK
DO
	HMM m1 = NULL, m2 = NULL;
	long n = GET_INTEGER (L"Observation length");
	int sym = GET_INTEGER (L"Symmetric");
	WHERE (SELECTED) { if (m1 != NULL) m2 = (structHMM *)OBJECT; else m1 = (structHMM *)OBJECT; }
	double ce = HMM_and_HMM_getCrossEntropy (m1, m2, n, sym);
	Melder_information6 (Melder_double (ce), L" (=", (sym ? L"symmetric " : L"ui/editors/AmplitudeTierEditor.h"),
		L" cross-entropy between models for observation length = ", Melder_integer (n), L")");
END

DIRECT (HMM_and_HMM_and_HMM_ObservationSequence_getCrossEntropy)
	HMM m1 = NULL, m2 = NULL;
	HMM_ObservationSequence hmmos = (structHMM_ObservationSequence *)ONLY (classHMM_ObservationSequence);
	WHERE (SELECTED) { if (CLASS == classHMM) { if (m1 != NULL) m2 = (structHMM *)OBJECT; else m1 = (structHMM *)OBJECT;} }
	double ce = HMM_and_HMM_and_HMM_ObservationSequence_getCrossEntropy (m1, m2, hmmos);
	Melder_information2 (Melder_double (ce), L" (=symmetric cross-entropy between models)");
END

FORM (HMM_to_HMM_ObservationSequence, L"HMM: To HMM_ObservationSequence (generate observations)", L"HMM: To HMM_ObservationSequence...")
	INTEGER (L"Start state", L"0")
	NATURAL (L"Number of observations", L"20")
	OK
DO
	EVERY_TO (HMM_to_HMM_ObservationSequence ((structHMM *)OBJECT, GET_INTEGER (L"Start state"),
		GET_INTEGER (L"Number of observations")))
END

DIRECT (HMM_and_HMM_StateSequence_getProbability)
	double lnp = HMM_and_HMM_StateSequence_getProbability ((structHMM *)ONLY (classHMM), (structHMM_StateSequence *)ONLY (classHMM_StateSequence));
	Melder_information4 (Melder_double (lnp), L" (=ln(p), p = ", Melder_naturalLogarithm (lnp), L")");
END

DIRECT (HMM_and_HMM_ObservationSequence_getProbability)
	double lnp = HMM_and_HMM_ObservationSequence_getProbability ((structHMM *)ONLY (classHMM), (structHMM_ObservationSequence *)ONLY (classHMM_ObservationSequence));
	Melder_information4 (Melder_double (lnp), L" (=ln(p), p = ", Melder_naturalLogarithm (lnp), L")");
END

DIRECT (HMM_and_HMM_ObservationSequence_getCrossEntropy)
	double ce = HMM_and_HMM_ObservationSequence_getCrossEntropy ((structHMM *)ONLY (classHMM), (structHMM_ObservationSequence *)ONLY (classHMM_ObservationSequence));
	Melder_information2 (Melder_double (ce), L" (= cross-entropy)");
END

DIRECT (HMM_and_HMM_ObservationSequence_getPerplexity)
	double py = HMM_and_HMM_ObservationSequence_getPerplexity ((structHMM *)ONLY (classHMM), (structHMM_ObservationSequence *)ONLY (classHMM_ObservationSequence));
	Melder_information2 (Melder_double (py), L" (= perplexity)");
END

DIRECT (HMM_and_HMM_ObservationSequence_to_HMM_StateSequence)
  if (! praat_new4 (HMM_and_HMM_ObservationSequence_to_HMM_StateSequence ((structHMM *)ONLY (classHMM), (structHMM_ObservationSequence *)ONLY (classHMM_ObservationSequence)), Thing_getName (ONLY (classHMM)), L"_", Thing_getName (ONLY (classHMM_ObservationSequence)), L"_states")) return 0;
END

FORM (HMM_and_HMM_ObservationSequence_learn, L"HMM & HMM_ObservationSequence: Learn", L"HMM & HMM_ObservationSequences: Learn...")
	POSITIVE (L"Relative precision in log(p)", L"0.001")
	REAL (L"Minimum probability", L"0.00000000001")
	OK
DO
	HMM_ObservationSequences me = HMM_ObservationSequences_create ();
	double minProb = GET_REAL (L"Minimum probability");
	REQUIRE (minProb >= 0 && minProb < 1, L"A probabilty must be >= 0 and < 1!")
	WHERE (SELECTED)
	{
		Data hmmos = (structData *)OBJECT;
		if (CLASS == classHMM_ObservationSequence && ! Collection_addItem (me, hmmos)) goto end;
	}
	HMM_and_HMM_ObservationSequences_learn ((structHMM *)ONLY (classHMM), me, GET_REAL (L"Relative precision in log"), minProb);
end:
	for (long i = my size; i > 0; i--)
	{
		Collection_undangleItem (me, my item[i]);
	}
	forget (me);
END

FORM (HMM_setTransitionProbabilities, L"HMM: Set transition probabilities", L"HMM: Set transition probabilities...")
	NATURAL (L"State number", L"1")
	SENTENCE (L"Probabilities", L"0.1 0.9")
	OK
DO
	if (! HMM_setTransitionProbabilities ((structHMM *)ONLY_OBJECT, GET_INTEGER (L"State number"), GET_STRING (L"Probabilities"))) return 0;
END

FORM (HMM_setEmissionProbabilities, L"HMM: Set emission probabilities", L"HMM: Set emission probabilities...")
	NATURAL (L"State number", L"1")
	SENTENCE (L"Probabilities", L"0.1 0.7 0.2")
	OK
DO
	if (! HMM_setEmissionProbabilities ((structHMM *)ONLY_OBJECT, GET_INTEGER (L"State number"), GET_STRING (L"Probabilities"))) return 0;
END

// evaluate the numbers given to probabilities
static double *NUMwstring_to_probs (wchar_t *prob_string, long nwanted)
{
	double *p = NULL, psum = 0;
	long ip = 1;

	p = NUMdvector (1, nwanted);
	if (p == NULL) return NULL;

	for (wchar_t *token = Melder_firstToken (prob_string); token != NULL && ip <= nwanted; token = Melder_nextToken (), ip++)
	{
		double prob;
		if (! Interpreter_numericExpression_FIXME (token, &prob))
		{
			Melder_error5 (L"Item ", Melder_integer (ip), L" \"", token, L"\"is not a number.");
			goto end;
		}
		p[ip] = prob;
		psum += prob;
	}
	ip--;
	for (long i = 1; i <= ip; i++) p[i] /= psum; // to probabilities
end:
	if (Melder_hasError ()) NUMdvector_free (p, 1);
	return p;
}

int HMM_setStartProbabilities (HMM me, wchar_t *probs)
{
	double *p = NUMwstring_to_probs (probs, my numberOfStates);
	if (p == NULL) return 0;
	for (long i = 1; i <= my numberOfStates; i++) my transitionProbs[0][i] = p[i];
	NUMdvector_free (p, 1);
	return 1;
}

int HMM_setTransitionProbabilities (HMM me, long state_number, wchar_t *state_probs)
{
	if (state_number > my states -> size) return Melder_error1 (L"State number too large.");
	double *p = NUMwstring_to_probs (state_probs, my numberOfStates + 1); // 1 extra for final state
	if (p == NULL) return 0;
	for (long i = 1; i <= my numberOfStates + 1; i++) { my transitionProbs[state_number][i] = p[i]; }
	NUMdvector_free (p, 1);
	return 1;
}

int HMM_setEmissionProbabilities (HMM me, long state_number, wchar_t *emission_probs)
{
	if (state_number > my states -> size) return Melder_error1 (L"State number too large.");
	if (my notHidden) return Melder_error1 (L"The emission probs of this model are fixed.");
	double *p = NUMwstring_to_probs (emission_probs, my numberOfObservationSymbols);
	if (p == NULL) return 0;
	for (long i = 1; i <= my numberOfObservationSymbols; i++)
	{
		my emissionProbs[state_number][i] = p[i];
	}
	NUMdvector_free (p, 1);
	return 1;
}

FORM (HMM_setStartProbabilities, L"HMM: Set start probabilities", L"HMM: Set start probabilities...")
	SENTENCE (L"Probabilities", L"0.1 0.9")
	OK
DO
	if (! HMM_setStartProbabilities ((structHMM *)ONLY_OBJECT, GET_STRING (L"Probabilities"))) return 0;
END

DIRECT (HMM_extractTransitionProbabilities)
	EVERY_CHECK (praat_new2 (HMM_extractTransitionProbabilities ((structHMM *)OBJECT), Thing_getName (OBJECT), L"_t"))
END

DIRECT (HMM_extractEmissionProbabilities)
	EVERY_CHECK (praat_new2 (HMM_extractEmissionProbabilities ((structHMM *)OBJECT), Thing_getName (OBJECT), L"_e"))
END

FORM (HMM_ObservationSequence_to_TableOfReal, L"HMM_ObservationSequence: To TableOfReal ",
	L"HMM_ObservationSequence: To TableOfReal (bigrams)...")
	BOOLEAN (L"As probabilities", 1)
	OK
DO
	EVERY_TO (HMM_ObservationSequence_to_TableOfReal_transitions ((structHMM_ObservationSequence *)OBJECT, GET_INTEGER (L"As probabilities")))
END

FORM (HMM_and_HMM_ObservationSequence_to_TableOfReal, L"HMM & HMM_ObservationSequence: To TableOfReal", L"HMM & HMM_ObservationSequence: To TableOfReal (bigrams)...")
	BOOLEAN (L"As probabilities", 1)
	OK
DO
	HMM_ObservationSequence hmmos = (structHMM_ObservationSequence *)ONLY (classHMM_ObservationSequence);
	if (! praat_new2 (HMM_and_HMM_ObservationSequence_to_TableOfReal_transitions ((structHMM *)ONLY (classHMM), hmmos,
		GET_INTEGER (L"As probabilities")), Thing_getName (hmmos), L"_m")) return 0;
END

FORM (HMM_and_HMM_StateSequence_to_TableOfReal, L"HMM & HMM_StateSequence: To TableOfReal", 0)
	BOOLEAN (L"As probabilities", 1)
	OK
DO
	HMM_StateSequence hmmss = (structHMM_StateSequence *)ONLY (classHMM_StateSequence);
	 if (! praat_new2 (HMM_and_HMM_StateSequence_to_TableOfReal_transitions ((structHMM *)ONLY (classHMM), hmmss, GET_INTEGER (L"As probabilities")),
		 Thing_getName (hmmss), L"_m")) return 0;
END

FORM (HMM_StateSequence_to_TableOfReal, L"HMM_StateSequence: To TableOfReal", 0)
	BOOLEAN (L"As probabilities", 1)
	OK
DO
	EVERY_TO (Strings_to_TableOfReal_transitions ((structStrings *)OBJECT, GET_INTEGER (L"As probabilities")))
END

DIRECT (HMM_ObservationSequence_to_Strings)
	EVERY_TO (HMM_ObservationSequence_to_Strings ((structHMM_ObservationSequence *)OBJECT))
END

DIRECT (Strings_to_HMM_ObservationSequence)
	EVERY_TO (Strings_to_HMM_ObservationSequence ((structStrings *)OBJECT))
END

DIRECT (HMM_StateSequence_to_Strings)
	EVERY_TO (HMM_StateSequence_to_Strings ((structHMM_StateSequence *)OBJECT))
END

FORM (TableOfReal_to_GaussianMixture_fromRowlabels, L"TableOfReal: To GaussianMixture from row labels",
	L"TableOfReal: To GaussianMixture (row labels)...")
	OPTIONMENU (L"Covariance matrices are", 1)
		OPTION (L"Complete")
		OPTION (L"Diagonal")
	OK
DO
	long storage = GET_INTEGER (L"Covariance matrices are") - 1;
	EVERY_TO (TableOfReal_to_GaussianMixture_fromRowLabels ((structTableOfReal *)OBJECT, storage))
END

FORM (TableOfReal_to_GaussianMixture, L"TableOfReal: To GaussianMixture (no labels)",
	L"TableOfReal: To GaussianMixture...")
	NATURAL (L"Number of components", L"2")
	POSITIVE (L"Tolerance of minimizer", L"0.001")
	INTEGER (L"Maximum number of iterations", L"200")
	REAL (L"Stability coefficient lambda", L"0.001")
	OPTIONMENU (L"Covariance matrices are", 1)
		OPTION (L"Complete")
		OPTION (L"Diagonal")
	GaussianMixture_OPTION_MENU_CRITERIA
	OK
DO
	int criterion = GET_INTEGER (L"Criterion based on") - 1;
	double lambda = GET_REAL (L"Stability coefficient lambda");
	REQUIRE (lambda >= 0 && lambda < 1, L"Lambda must be in interval [0,1).")
	long storage = GET_INTEGER (L"Covariance matrices are") - 1;
	EVERY_TO (TableOfReal_to_GaussianMixture ((structTableOfReal *)OBJECT, GET_INTEGER (L"Number of components"),
		GET_REAL (L"Tolerance of minimizer"), GET_INTEGER (L"Maximum number of iterations"), lambda, storage, criterion))
END

FORM (GaussianMixture_and_TableOfReal_improveLikelihood, L"GaussianMixture & TableOfReal: Improve likelihood",
	L"GaussianMixture & TableOfReal: Improve likelihood...")
	POSITIVE (L"Tolerance of minimizer", L"0.001")
	NATURAL (L"Maximum number of iterations", L"200")
	REAL (L"Stability coefficient lambda", L"0.001")
	GaussianMixture_OPTION_MENU_CRITERIA
	OK
DO
	long criterion = GET_INTEGER (L"Criterion based on") - 1;
	double lambda = GET_REAL (L"Stability coefficient lambda");
	GaussianMixture me = (structGaussianMixture *)ONLY (classGaussianMixture);
	TableOfReal thee = (structTableOfReal *)ONLY (classTableOfReal);
	REQUIRE (lambda >= 0 && lambda < 1, L"Lambda must be in interval [0,1).")
	REQUIRE (thy numberOfColumns == my dimension, L"The number of columns and the dimension of the model do not agree.");
	REQUIRE (my numberOfComponents < thy numberOfRows / 2, L"Not enough data points.")
	if (! GaussianMixture_and_TableOfReal_improveLikelihood (me, thee, GET_REAL (L"Tolerance of minimizer"), GET_INTEGER (L"Maximum number of iterations"), lambda, criterion)) return 0;
END

FORM (GaussianMixture_and_TableOfReal_to_GaussianMixture_CEMM, L"GaussianMixture & TableOfReal: To GaussianMixture (CEMM)", L"GaussianMixture & TableOfReal: To GaussianMixture (CEMM)...")
	INTEGER (L"Minimum number of components", L"1")
	POSITIVE (L"Tolerance of minimizer", L"0.001")
	NATURAL (L"Maximum number of iterations", L"200")
	REAL (L"Stability coefficient lambda", L"0.001")
	GaussianMixture_OPTION_MENU_CRITERIA
	OK
DO
	double lambda = GET_REAL (L"Stability coefficient lambda");
	int criterion = GET_INTEGER (L"Criterion based on") - 1;
	GaussianMixture me = (structGaussianMixture *)ONLY (classGaussianMixture);
	TableOfReal thee = (structTableOfReal *)ONLY (classTableOfReal);
	REQUIRE (lambda >= 0 && lambda < 1, L"Lambda must be in interval [0,1).")
	REQUIRE (thy numberOfColumns == my dimension, L"The number of columns and the dimension of the model do not agree.");
	REQUIRE (my numberOfComponents < thy numberOfRows / 2, L"Not enough data points.")
	if (! praat_new1 (GaussianMixture_and_TableOfReal_to_GaussianMixture_CEMM (me, thee,
		GET_INTEGER (L"Minimum number of components"), GET_REAL (L"Tolerance of minimizer"),
		GET_INTEGER (L"Maximum number of iterations"), lambda, criterion), Thing_getName (me))) return 0;
END

DIRECT (GaussianMixture_and_TableOfReal_to_ClassificationTable)
	GaussianMixture me = (structGaussianMixture *)ONLY (classGaussianMixture);
	TableOfReal thee = (structTableOfReal *)ONLY (classTableOfReal);
	if (! praat_new3 (GaussianMixture_and_TableOfReal_to_ClassificationTable (me, thee),
		Thing_getName (me), L"_", Thing_getName (thee))) return 0;
END

DIRECT (GaussianMixture_and_TableOfReal_to_Correlation)
	GaussianMixture me = (structGaussianMixture *)ONLY (classGaussianMixture);
	TableOfReal thee = (structTableOfReal *)ONLY (classTableOfReal);
	if (! praat_new3 (GaussianMixture_and_TableOfReal_to_Correlation (me, thee),
		Thing_getName (me), L"_", Thing_getName (thee))) return 0;
END

FORM (GaussianMixture_and_TableOfReal_to_TableOfReal_BHEPNormalityTests, L"GaussianMixture & TableOfReal: To TableOfReal BHEP normality tests", L"GaussianMixture & TableOfReal: To TableOfReal (BHEP normality tests)...")
	REAL (L"Smoothing parameter", L"1.41")
	OK
DO
	GaussianMixture me = (structGaussianMixture *)ONLY (classGaussianMixture);
	TableOfReal thee = (structTableOfReal *)ONLY (classTableOfReal);
	double h = GET_REAL (L"Smoothing parameter");
	if (! praat_new3 (GaussianMixture_and_TableOfReal_to_TableOfReal_BHEPNormalityTests (me, thee, h),
		Thing_getName (me), L"_", Thing_getName (thee))) return 0;
END

extern "C" void praat_HMM_init (void);
void praat_HMM_init (void)
{
	Thing_recognizeClassesByName (classHMM, classHMM_State, classHMM_Observation,
		classHMM_ObservationSequence, classHMM_StateSequence, classGaussianMixture, NULL);

	praat_addMenuCommand (L"Objects", L"New", L"Markov models", 0, praat_HIDDEN, 0);
	praat_addMenuCommand (L"Objects", L"New", L"Create HMM...", 0, praat_HIDDEN + praat_DEPTH_1, DO_HMM_create);
	praat_addMenuCommand (L"Objects", L"New", L"Create simple HMM...", 0, praat_HIDDEN + praat_DEPTH_1, DO_HMM_createSimple);
	praat_addMenuCommand (L"Objects", L"New", L"Create continuous HMM...", 0, praat_HIDDEN + praat_DEPTH_1, DO_HMM_createContinuousModel);

	praat_addAction1 (classGaussianMixture, 0, L"GaussianMixture help", 0, 0, DO_GaussianMixture_help);
	praat_addAction1 (classGaussianMixture, 0, L"Draw concentration ellipses...", 0, 0, DO_GaussianMixture_drawConcentrationEllipses);
	praat_addAction1 (classGaussianMixture, 0, L"Draw marginal pdf...", 0, 0, DO_GaussianMixture_drawMarginalPdf);
	praat_addAction1 (classGaussianMixture, 0, L"Query -", 0, 0, 0);
	praat_addAction1 (classGaussianMixture, 1, L"Get number of components", 0, 1, DO_GaussianMixture_getNumberOfComponents);
	praat_addAction1 (classGaussianMixture, 1, L"Get dimension of component", 0, 1, DO_GaussianMixture_getDimensionOfComponent);
	praat_addAction1 (classGaussianMixture, 1, L"Get probability at position...", 0, 1, DO_GaussianMixture_getProbabilityAtPosition);
	praat_addAction1 (classGaussianMixture, 0, L"Modify -", 0, 0, 0);
	praat_addAction1 (classGaussianMixture, 1, L"Split component...", 0, 1, DO_GaussianMixture_splitComponent);
	praat_addAction1 (classGaussianMixture, 0, L"Extract -", 0, 0, 0);
	praat_addAction1 (classGaussianMixture, 0, L"Extract mixing probabilities", 0, 1, DO_GaussianMixture_extractMixingProbabilities);
	praat_addAction1 (classGaussianMixture, 0, L"Extract component...", 0, 1, DO_GaussianMixture_extractComponent);
	praat_addAction1 (classGaussianMixture, 0, L"Extract centroids", 0, 1, DO_GaussianMixture_extractCentroids);
	praat_addAction1 (classGaussianMixture, 0, L"To Covariance (between)", 0, 0, DO_GaussianMixture_to_Covariance_between);
	praat_addAction1 (classGaussianMixture, 0, L"To Covariance (within)", 0, 0, DO_GaussianMixture_to_Covariance_within);
	praat_addAction1 (classGaussianMixture, 0, L"To Covariance (total)", 0, 0, DO_GaussianMixture_to_Covariance_total);
	praat_addAction1 (classGaussianMixture, 0, L"To PCA", 0, 0, DO_GaussianMixture_to_PCA);
	praat_addAction1 (classGaussianMixture, 0, L"To TableOfReal (random sampling)...", 0, 0, DO_GaussianMixture_to_TableOfReal_randomSampling);

	praat_addAction2 (classGaussianMixture, 1, classTableOfReal, 1, L"Get likelihood value...", 0, 0, DO_GaussianMixture_and_TableOfReal_getLikelihoodValue);
	praat_addAction2 (classGaussianMixture, 1, classTableOfReal, 1, L"Improve likelihood...", 0, 0, DO_GaussianMixture_and_TableOfReal_improveLikelihood);
	praat_addAction2 (classGaussianMixture, 1, classTableOfReal, 1, L"To GaussianMixture (CEMM)...", 0, 0, DO_GaussianMixture_and_TableOfReal_to_GaussianMixture_CEMM);
	praat_addAction2 (classGaussianMixture, 1, classTableOfReal, 1, L"To ClassificationTable", 0, 0, DO_GaussianMixture_and_TableOfReal_to_ClassificationTable);
	praat_addAction2 (classGaussianMixture, 1, classTableOfReal, 1, L"To Correlation", 0, 0, DO_GaussianMixture_and_TableOfReal_to_Correlation);
	praat_addAction2 (classGaussianMixture, 1, classTableOfReal, 1, L"To TableOfReal (BHEP normality tests)...", 0, 0, DO_GaussianMixture_and_TableOfReal_to_TableOfReal_BHEPNormalityTests);

	praat_addAction2 (classGaussianMixture, 1, classPCA, 1, L"Draw concentration ellipses...", 0, 0, DO_GaussianMixture_and_PCA_drawConcentrationEllipses);
	praat_addAction2 (classGaussianMixture, 1, classPCA, 1, L"Draw marginal pdf...", 0, 0, DO_GaussianMixture_and_PCA_drawMarginalPdf);
	praat_addAction2 (classGaussianMixture, 1, classPCA, 1, L"To Matrix (density)...", 0, 0, DO_GaussianMixture_and_PCA_to_Matrix_density);


	praat_addAction1 (classHMM, 0, L"HMM help ", 0, 0, DO_HMM_help);
	praat_addAction1 (classHMM, 0, L"Draw...", 0, 0, DO_HMM_draw);
	praat_addAction1 (classHMM, 0, L"Query -", 0, 0, 0);
	praat_addAction1 (classHMM, 1, L"Get transition probability...", 0, 1, DO_HMM_getTransitionProbability);
	praat_addAction1 (classHMM, 1, L"Get emission probability...", 0, 1, DO_HMM_getEmissionProbability);
	praat_addAction1 (classHMM, 1, L"Get start probability...", 0, 1, DO_HMM_getStartProbability);
	praat_addAction1 (classHMM, 1, L"Get p (time, state)...", 0, 1, DO_HMM_getProbabilityAtTimeBeingInState);
	praat_addAction1 (classHMM, 1, L"Get p (time, state, symbol)...", 0, 1, DO_HMM_getProbabilityAtTimeBeingInStateEmittingSymbol);
	praat_addAction1 (classHMM, 1, L"Get probability staying in state...", 0, 1, DO_HMM_getProbabilityOfStayingInState);
	praat_addAction1 (classHMM, 1, L"Get expected duration in state...", 0, 1, DO_HMM_getExpectedValueOfDurationInState);
	praat_addAction1 (classHMM, 1, L"---- states / symbols -----", 0, 1, 0);
	praat_addAction1 (classHMM, 1, L"Get state label...", 0, 1, DO_HMM_getStateLabel);
	praat_addAction1 (classHMM, 1, L"Get symbol label...", 0, 1, DO_HMM_getSymbolLabel);
	praat_addAction1 (classHMM, 0, L"--- multiple HMMs ----", 0, 1, 0);
	praat_addAction1 (classHMM, 2, L"Get cross-entropy...", 0, 1, DO_HMM_and_HMM_getCrossEntropy);

	praat_addAction1 (classHMM, 0, L"Modify -", 0, 0, 0);
	praat_addAction1 (classHMM, 1, L"Set transition probabilities...", 0, 1, DO_HMM_setTransitionProbabilities);
	praat_addAction1 (classHMM, 1, L"Set emission probabilities...", 0, 1, DO_HMM_setEmissionProbabilities);
	praat_addAction1 (classHMM, 1, L"Set start probabilities...", 0, 1, DO_HMM_setStartProbabilities);

	praat_addAction1 (classHMM, 0, L"Extract -", 0, 0, 0);
	praat_addAction1 (classHMM, 0, L"Extract transition probabilities", 0, 1, DO_HMM_extractTransitionProbabilities);
	praat_addAction1 (classHMM, 0, L"Extract emission probabilities", 0, 1, DO_HMM_extractEmissionProbabilities);

	praat_addAction1 (classHMM, 0, L"To HMM_ObservationSequence...", 0, 0, DO_HMM_to_HMM_ObservationSequence);
	praat_addAction2 (classHMM, 1, classHMM_StateSequence, 1, L"Draw trellis...", 0, 0, DO_HMM_and_HMM_StateSequence_drawTrellis);
	praat_addAction2 (classHMM, 1, classHMM_StateSequence, 1, L"Get probability", 0, 0, DO_HMM_and_HMM_StateSequence_getProbability);
	praat_addAction2 (classHMM, 1, classHMM_StateSequence, 1, L"To TableOfReal (bigrams)...", 0, 0, DO_HMM_and_HMM_StateSequence_to_TableOfReal);
	praat_addAction2 (classHMM, 1, classHMM_ObservationSequence, 1, L"Get probability", 0, 0, DO_HMM_and_HMM_ObservationSequence_getProbability);
	praat_addAction2 (classHMM, 1, classHMM_ObservationSequence, 1, L"Get cross-entropy", 0, 0, DO_HMM_and_HMM_ObservationSequence_getCrossEntropy);
	praat_addAction2 (classHMM, 1, classHMM_ObservationSequence, 1, L"Get perplexity", 0, 0, DO_HMM_and_HMM_ObservationSequence_getPerplexity);


	praat_addAction2 (classHMM, 1, classHMM_ObservationSequence, 1, L"To HMM_StateSequence", 0, 0, DO_HMM_and_HMM_ObservationSequence_to_HMM_StateSequence);
	praat_addAction2 (classHMM, 2, classHMM_ObservationSequence, 1, L"Get cross-entropy", 0, 0, DO_HMM_and_HMM_and_HMM_ObservationSequence_getCrossEntropy);
	praat_addAction2 (classHMM, 1, classHMM_ObservationSequence, 1, L"To TableOfReal (bigrams)...", 0, 0, DO_HMM_and_HMM_ObservationSequence_to_TableOfReal);
	praat_addAction2 (classHMM, 1, classHMM_ObservationSequence, 0, L"Learn...", 0, 0, DO_HMM_and_HMM_ObservationSequence_learn);

	praat_addAction1 (classHMM_ObservationSequence, 0, L"To TableOfReal (bigrams)...", 0, 0, DO_HMM_ObservationSequence_to_TableOfReal);
	praat_addAction1 (classHMM_ObservationSequence, 0, L"To Strings", 0, 0, DO_HMM_ObservationSequence_to_Strings);
	praat_addAction1 (classHMM_StateSequence, 0, L"To TableOfReal (bigrams)...", 0, 0, DO_HMM_StateSequence_to_TableOfReal);
	praat_addAction1 (classHMM_StateSequence, 0, L"To Strings", 0, 0, DO_HMM_StateSequence_to_Strings);

	praat_addAction1 (classHMM_ObservationSequence, 0, L"To HMM...", 0, 1, DO_HMM_ObservationSequence_to_HMM);

	praat_addAction1 (classStrings, 0, L"To HMM_ObservationSequence", 0, praat_HIDDEN, DO_Strings_to_HMM_ObservationSequence);
	praat_addAction1 (classTableOfReal, 0, L"To GaussianMixture (row labels)...", L"To Covariance", praat_HIDDEN + praat_DEPTH_1, DO_TableOfReal_to_GaussianMixture_fromRowlabels);
	praat_addAction1 (classTableOfReal, 0, L"To GaussianMixture...", L"To Covariance", praat_HIDDEN + praat_DEPTH_1, DO_TableOfReal_to_GaussianMixture);
}

/* End of file praat_HMM_init.c */
