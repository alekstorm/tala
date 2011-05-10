/* praat_FFNet_init.c
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
 djmw 20020408 GPL
 djmw 20020408 added FFNet_help
 djmw 20030701 Removed non-GPL minimizations
 djmw 20040526 Removed bug in FFNet_drawCostHistory interface.
 djmw 20041123 Latest modification
 djmw 20061218 Changed to Melder_information<x> format.
 djmw 20080902 Melder_error<1...>
 djmw 20071011 REQUIRE requires L"ui/editors/AmplitudeTierEditor.h".
 djmw 20071024 Use MelderString_append in FFNet_createNameFromTopology
 djmw 20100511 FFNet query outputs
*/

#include "ui/praat.h"

#include "FFNet/FFNet_Matrix.h"
#include "FFNet/FFNet_Activation_Categories.h"
#include "FFNet/FFNet_Pattern_Activation.h"
#include "FFNet/FFNet_Pattern_Categories.h"
#include "dwtools/Discriminant.h"
#include "dwtools/PCA.h"
#include "dwtools/Minimizers.h"
#include "dwtools/Matrix_extensions.h"
#include "num/NUM2.h"

#include <math.h>

void TableOfReal_drawAsSquares (I, Graphics graphics, long rowmin, long rowmax,
	long colmin, long colmax, int garnish);

/* Routines to be removed sometime in the future:
20040422, 2.4.04: FFNet_drawWeightsToLayer  use FFNet_drawWeights
20040422, 2.4.04: FFNet_weightsToMatrix use FFNet_extractWeights
*/


static wchar_t *QUERY_BUTTON   = L"Query -";
static wchar_t *DRAW_BUTTON     = L"Draw -";
static wchar_t *MODIFY_BUTTON  = L"Modify -";
static wchar_t *EXTRACT_BUTTON = L"Extract -";

/**************** New FFNet ***************************/

static void FFNet_create_addCommonFields_inputOutput (UiForm *dia)
{
    NATURAL (L"Number of inputs", L"4")
    NATURAL (L"Number of outputs", L"3")
}

static int FFNet_create_checkCommonFields_inputOutput (UiForm *dia, long *numberOfInputs, long *numberOfOutputs)
{
	*numberOfInputs = GET_INTEGER (L"Number of inputs");
	*numberOfOutputs = GET_INTEGER (L"Number of outputs");
	return 1;
}

static void FFNet_create_addCommonFields_hidden (UiForm *dia)
{
    INTEGER (L"Number of units in hidden layer 1", L"0")
    INTEGER (L"Number of units in hidden layer 2", L"0")
}

static int FFNet_create_checkCommonFields_hidden (UiForm *dia, 	long *numberOfHidden1, long *numberOfHidden2)
{
	*numberOfHidden1 = GET_INTEGER (L"Number of units in hidden layer 1");
	*numberOfHidden2 = GET_INTEGER (L"Number of units in hidden layer 2");
	if (*numberOfHidden1 < 0 || *numberOfHidden2 < 0)
	{
		return Melder_error1 (L"The number of units in a hidden layer must be greater than or equal to 0.");
	}
	return 1;
}

static void FFNet_create_addCommonFields (UiForm *dia)
{
	FFNet_create_addCommonFields_inputOutput (dia);
    FFNet_create_addCommonFields_hidden (dia);
}

static int FFNet_create_checkCommonFields (UiForm *dia, long *numberOfInputs, long *numberOfOutputs,
	long *numberOfHidden1, long *numberOfHidden2)
{
	return FFNet_create_checkCommonFields_inputOutput (dia, numberOfInputs, numberOfOutputs) &&
		FFNet_create_checkCommonFields_hidden (dia, numberOfHidden1, numberOfHidden2);
}

FORM (FFNet_create, L"Create FFNet", L"Create FFNet...")
	WORD (L"Name", L"4-3")
	FFNet_create_addCommonFields (dia);
    OK
DO
	FFNet thee;
	long numberOfInputs, numberOfOutputs, numberOfHidden1, numberOfHidden2;
	if (! FFNet_create_checkCommonFields (dia, &numberOfInputs, &numberOfOutputs,
		&numberOfHidden1, &numberOfHidden2)) return 0;
	thee = FFNet_create (numberOfInputs, numberOfHidden1, numberOfHidden2, numberOfOutputs, 0);
	if (thee == NULL)
	{
		return Melder_error1 (L"There was not enough memory to create the FFNet.\n"
			" Please reduce some of the numbers.");
	}
    if (! praat_new1 (thee, GET_STRING (L"Name"))) return 0;
END

FORM (FFNet_create_linearOutputs, L"Create FFNet", L"Create FFNet (linear outputs)...")
	WORD (L"Name", L"4-3L")
	FFNet_create_addCommonFields (dia);
    OK
DO
	FFNet thee;
	long numberOfInputs, numberOfOutputs, numberOfHidden1, numberOfHidden2;
	if (! FFNet_create_checkCommonFields (dia, &numberOfInputs, &numberOfOutputs,
		&numberOfHidden1, &numberOfHidden2)) return 0;
	thee = FFNet_create (numberOfInputs, numberOfHidden1, numberOfHidden2, numberOfOutputs, 1);
	if (thee == NULL)
	{
		return Melder_error1 (L"There was not enough memory to create the FFNet.\nPlease reduce some of the numbers.");
	}
    if (! praat_new1 (thee, GET_STRING (L"Name"))) return 0;
END

FORM (FFNet_createIrisExample, L"Create iris example", L"Create iris example...")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"For the feedforward neural net we need to know the:")
	FFNet_create_addCommonFields_hidden (dia);
	OK
DO
	long numberOfHidden1, numberOfHidden2;
	if (! FFNet_create_checkCommonFields_hidden (dia, &numberOfHidden1, &numberOfHidden2)) return 0;
	NEW (FFNet_createIrisExample (numberOfHidden1, numberOfHidden2))
END

DIRECT (FFNet_getNumberOfInputs)
	FFNet  me = (structFFNet *)ONLY(classFFNet);
	Melder_information2 (Melder_integer (my nUnitsInLayer[0]), L" units");
END

DIRECT (FFNet_getNumberOfOutputs)
	FFNet  me = (structFFNet *)ONLY(classFFNet);
	Melder_information2 (Melder_integer (my nUnitsInLayer[my nLayers]), L" units");
END

FORM (FFNet_getNumberOfHiddenUnits, L"FFNet: Get number of hidden units", L"FFNet: Get number of hidden units...")
	NATURAL (L"Hidden layer number", L"1")
	OK
DO
	FFNet  me = (structFFNet *)ONLY(classFFNet);
	long layerNumber = GET_INTEGER (L"Hidden layer number");
	long numberOfUnits = 0;

	if (layerNumber > 0 && layerNumber <= my nLayers - 1) numberOfUnits = my nUnitsInLayer[layerNumber];
	Melder_information2 (Melder_integer (numberOfUnits), L" units");
END

FORM (FFNet_getCategoryOfOutputUnit, L"FFNet: Get category of output unit", L"ui/editors/AmplitudeTierEditor.h")
	NATURAL (L"Output unit", L"1")
	OK
DO
	FFNet me = (structFFNet *)ONLY_OBJECT;
	Categories c = my outputCategories;
	long unit = GET_INTEGER (L"Output unit");
	if (unit > c -> size) return Melder_error3 (L"Output unit cannot be larger than ", Melder_integer (c -> size), L".");
	SimpleString ss = (structSimpleString *)c -> item[unit];
	Melder_information1 (ss -> string);
END

FORM (FFNet_getOutputUnitOfCategory, L"FFNet: Get output unit of category", L"ui/editors/AmplitudeTierEditor.h")
	SENTENCE (L"Category", L"u")
	OK
DO
	FFNet me = (structFFNet *)ONLY_OBJECT;
	Categories c = my outputCategories;
	wchar_t *category = GET_STRING (L"Category");
	long index = 0;
	for (long i = 1; i <= c -> size; i++)
	{
		SimpleString s = (structSimpleString *)c -> item[i];
		if (Melder_wcsequ (s -> string, category)) { index = i; break;};
	}
	Melder_information1 (Melder_integer (index));
END

FORM (FFNet_getNumberOfHiddenWeights, L"FFNet: Get number of hidden weights", L"FFNet: Get number of hidden weights...")
	NATURAL (L"Hidden layer number", L"1")
	OK
DO
	FFNet  me = (structFFNet *)ONLY(classFFNet);
	long layerNumber = GET_INTEGER (L"Hidden layer number");
	long numberOfWeights = 0;
	if (layerNumber > 0 && layerNumber <= my nLayers - 1)
	{
		numberOfWeights = my nUnitsInLayer[layerNumber] * (my nUnitsInLayer[layerNumber - 1]+1);
	}
	Melder_information2 (Melder_integer (numberOfWeights), L" weights (including biases)");
END

DIRECT (FFNet_getNumberOfOutputWeights)
	FFNet  me = (structFFNet *)ONLY(classFFNet);
	Melder_information2 (Melder_integer (my nUnitsInLayer[my nLayers] * (my nUnitsInLayer[my nLayers - 1]+1)), L" weights");
END

/**************** New Pattern ***************************/

FORM (Pattern_create, L"Create Pattern", 0)
	WORD (L"Name", L"1x1")
    NATURAL (L"Dimension of a pattern", L"1")
    NATURAL (L"Number of patterns", L"1")
    OK
DO
	if (! praat_new1 (Pattern_create (GET_INTEGER (L"Number of patterns"),
		GET_INTEGER (L"Dimension of a pattern")), GET_STRING (L"Name"))) return 0;
END

/**************** New Categories ***************************/

FORM (Categories_create, L"Create Categories", L"ui/editors/AmplitudeTierEditor.h")
	WORD (L"Name", L"empty")
	OK
DO
	if (! praat_new1 (Categories_create (), GET_STRING (L"Name"))) return 0;
END

DIRECT (FFNet_help)
	Melder_help (L"Feedforward neural networks");
END

DIRECT (FFNet_getMinimum)
	Melder_information1 (Melder_double (FFNet_getMinimum ((structFFNet *)ONLY_OBJECT)));
END

FORM (FFNet_reset, L"FFNet: Reset", L"FFNet: Reset...")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Warning: this command destroys all previous learning.")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"New weights will be randomly chosen from the interval [-range, +range].")
    POSITIVE (L"Range", L"0.1")
    OK
DO
    WHERE (SELECTED)
    {
    	FFNet_reset ((structFFNet *)OBJECT, GET_REAL (L"Range"));
		praat_dataChanged (OBJECT);
	}
END

FORM (FFNet_selectBiasesInLayer, L"FFNet: Select biases", L"FFNet: Select biases...")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"WARNING: This command induces very specific behaviour ")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"during a following learning phase.")
	NATURAL (L"Layer number", L"1")
	OK
DO
	WHERE (SELECTED)
	{
		FFNet_selectBiasesInLayer ((structFFNet *)OBJECT, GET_INTEGER (L"Layer number"));
		praat_dataChanged (OBJECT);
	}
END

DIRECT (FFNet_selectAllWeights)
	WHERE (SELECTED)
	{
    	FFNet_selectAllWeights ((structFFNet *)OBJECT);
		praat_dataChanged (OBJECT);
	}
END

void SimpleString_draw (SimpleString me, Any g, double xWC, double yWC)
{
    Graphics_text (g, xWC, yWC, my string);
}

void Categories_drawItem (Categories me, Graphics g, long position, 
	double xWC, double yWC)
{
	if (position < 1 || position > my size) return;
	SimpleString_draw ((structSimpleString *)my item[position], g, xWC, yWC);
}

void FFNet_drawTopology (FFNet me, Graphics g)
{
    long i, maxNumOfUnits = my nUnitsInLayer[0];
    int dxIsFixed = 1;
    double radius, dx, dy = 1.0 / (my nLayers + 1);

    for (i = 1; i <= my nLayers; i++)
	{
		if (my nUnitsInLayer[i] > maxNumOfUnits) maxNumOfUnits = my nUnitsInLayer[i];
	}
    dx = 1.0 / maxNumOfUnits;
    radius = dx / 10;
    Graphics_setInner (g);
    Graphics_setWindow (g, 0.0, 1.0, 0.0, 1.0); 
    for (i = 0; i <= my nLayers; i++)
    {
		long j, k;
		double dx2 = dx, x2WC, y2WC = dy / 2 + i * dy;
		double x2 = (maxNumOfUnits - my nUnitsInLayer[i] + 1) * dx2 / 2;
		/* draw the units */
		if (! dxIsFixed)
		{
			dx2 = 1.0 / my nUnitsInLayer[i];
			x2 = dx2 / 2;
		}
		if (i == 0)
		{
	    	Graphics_setTextAlignment (g, Graphics_CENTRE, Graphics_TOP);
	    	for (x2WC = x2, j = 1; j <= my nInputs; j++)
	    	{
				Graphics_arrow (g, x2WC, y2WC - radius - dy / 4, x2WC, y2WC - radius);
				x2WC += dx2;
	    	}
		}  
		Graphics_setColour (g, Graphics_RED);
		for (x2WC = x2, j = 1; j <= my nUnitsInLayer[i]; j++)
		{
	   	 	Graphics_circle (g, x2WC, y2WC, radius);
			if (i > 0) Graphics_fillCircle (g, x2WC, y2WC, radius);
			x2WC += dx2;
		}
		Graphics_setColour (g, Graphics_BLACK);
		if (i > 0)
		{
	    	double dx1 = dx;
			double x1 = (maxNumOfUnits - my nUnitsInLayer[i - 1] + 1) * dx1 / 2;
	   		double y1WC = y2WC - dy;
	    	if (! dxIsFixed)
			{
				dx1 = 1.0 / my nUnitsInLayer[i - 1];
				x1 = dx1 / 2;
			}
	    	x2WC = x2;	
	    	for (j = 1; j <= my nUnitsInLayer[i]; j++)
	    	{
				double x1WC = x1;
				for (k = 1; k <= my nUnitsInLayer[i - 1]; k++)
				{
		    		double xd = x2WC - x1WC;
		    		double cosa = xd / sqrt (xd * xd + dy * dy);
		    		double sina = dy / sqrt (xd * xd + dy * dy);
		    		Graphics_line (g, x1WC + radius * cosa, y1WC + radius * sina, x2WC - radius * cosa, y2WC - radius * sina);
		    		x1WC += dx1;
				}
				x2WC += dx2;
	    	}
		}
		if (i == my nLayers)
		{
	    	x2WC = x2;
	    	Graphics_setTextAlignment (g, Graphics_CENTRE, Graphics_BOTTOM);
	    	for (j = 1; j <= my nOutputs; j++)
	    	{
				Graphics_arrow (g, x2WC, y2WC + radius, x2WC, y2WC + radius + dy / 4);
				if (my outputCategories) Categories_drawItem (my outputCategories, g, j, x2WC, y2WC + radius + dy / 4); 
				x2WC += dx2;
	    	}
		}
    }
    Graphics_unsetInner (g);
}

void FFNet_drawActivation (FFNet me, Graphics g)
{
    long i, j, node = 1, maxNumOfUnits = my nUnitsInLayer[0];
    int dxIsFixed = 1;
	Graphics_Colour colour = Graphics_inqColour (g);
    double r1, dx, dy = 1.0 / (my nLayers + 1);

    Graphics_setInner (g);
    Graphics_setWindow (g, 0.0, 1.0, 0.0, 1.0); 
    for (i = 1; i <= my nLayers; i++)
	{
		if (my nUnitsInLayer[i] > maxNumOfUnits) maxNumOfUnits = my nUnitsInLayer[i];
	}
    dx = 1.0 / maxNumOfUnits;
    r1 = dx / 2; /* May touch when neighbouring activities are both 1 (very rare). */
    for (i = 0; i <= my nLayers; i++, node++)
    {
		double dx2 = dx, x2WC, y2WC = dy / 2 + i * dy;
		double x2 = (maxNumOfUnits - my nUnitsInLayer[i] + 1) * dx2 / 2;
		if (! dxIsFixed)
		{
			dx2 = 1.0 / my nUnitsInLayer[i];
			x2 = dx2 / 2;
		}
		x2WC = x2;
		for (j = 1; j <= my nUnitsInLayer[i]; j++, node++)
		{
	    	double activity = my activity[node];
	    	double radius = r1 * (fabs (activity) < 0.05 ? 0.05 : fabs (activity));
	    	/*Graphics_setColour (g, activity < 0 ? Graphics_BLACK : Graphics_RED);*/
	    	Graphics_circle (g, x2WC, y2WC, radius);
	    	if (activity < 0) Graphics_fillCircle (g, x2WC, y2WC, radius);
	    	x2WC += dx2;
		}
    }
    Graphics_setColour (g, colour);
    Graphics_unsetInner (g);
}

void Matrix_drawAsSquares (I, Any g, double xmin, double xmax, double ymin, double ymax, int garnish);

/* This routine is deprecated since praat-4.2.4 20040422 and will be removed in the future. */
/* Deprecated: the strengths of the weights that connect to the nodes in later 'layer' */
/* are drawn with boxes. The area of each box corresponds to the strength. */ 
/* Black boxes have negative strength? */
void FFNet_drawWeightsToLayer (FFNet me, Graphics g, int layer, int scaling, int garnish)
{
    Matrix weights;
    
    if (layer < 1 || layer > my nLayers || 
    	! (weights = FFNet_weightsToMatrix (me, layer, 0))) return;
    Matrix_scale (weights, scaling);
    Matrix_drawAsSquares (weights, g, 0, 0, 0, 0, 0);
    if (garnish)
    {
    	double x1WC, x2WC, y1WC, y2WC; wchar_t text[30];
		Graphics_inqWindow (g, & x1WC, & x2WC, & y1WC, & y2WC);
		swprintf (text, 30, L"Units in layer %ld ->", layer);
		Graphics_textBottom (g, 0, text);
		if (layer == 1) wcscpy (text, L"Input units ->");
		else swprintf (text, 30, L"Units in layer %ld ->", layer-1);
		Graphics_textLeft (g, 0, text);
		/* how do I find out the current settings ??? */
		Graphics_setTextAlignment (g, Graphics_RIGHT, Graphics_HALF);
		Graphics_setInner (g);
		Graphics_text (g, 0.5, weights->ny, L"bias");
		Graphics_unsetInner (g);
    }
    forget (weights);
}

void FFNet_drawWeights (FFNet me, Graphics g, long layer, int garnish)
{
    TableOfReal thee = FFNet_extractWeights (me, layer);
    
	if (thee == NULL) return;
	TableOfReal_drawAsSquares (thee, g, 1, thy numberOfRows, 1, thy numberOfColumns, garnish);

    forget (thee);
}

void Minimizer_drawHistory (I, Any graphics, long iFrom, long iTo, double hmin, 
    double hmax, int garnish)
{
    iam (Minimizer); 
	double *history;
    long i, itmin, itmax;
	
    if (my history == NULL ||
		(history = NUMdvector (1, my iteration)) == NULL) return;
    for (i = 1; i <= my iteration; i++)
	{
		history[i] = my history[i];
	}
    if (iTo <= iFrom)
	{
		iFrom = 1; iTo = my iteration;
	}
    itmin = iFrom; itmax = iTo;
    if (itmin < 1) itmin = 1; 
    if (itmax > my iteration) itmax = my iteration;
    if (hmax <= hmin)
	{
		NUMdvector_extrema (history, itmin, itmax, & hmin, & hmax);
	}
    if (hmax <= hmin)
	{
		hmin -= 0.5 * fabs (hmin);
		hmax += 0.5 * fabs (hmax);
	}
    Graphics_setInner (graphics);
    Graphics_setWindow (graphics, iFrom, iTo, hmin, hmax);
    Graphics_function (graphics, history, itmin, itmax, itmin, itmax);
    NUMdvector_free (history, 1);
    Graphics_unsetInner (graphics);
    if (garnish)
    {
    	Graphics_drawInnerBox (graphics);    
   		Graphics_textBottom (graphics, 1, L"Number of iterations");
		Graphics_marksBottom (graphics, 2, 1, 1, 0);
    	Graphics_marksLeft (graphics, 2, 1, 1, 0);
    }
}

/* draw cost vs epochs */
void FFNet_drawCostHistory (FFNet me, Graphics g, long iFrom, long iTo, 
    double costMin, double costMax, int garnish)
{
    if (my minimizer) Minimizer_drawHistory (my minimizer, g, iFrom, iTo,
		costMin, costMax, 0);
	if (garnish)
	{
		Graphics_drawInnerBox (g);
		Graphics_textLeft (g, 1, my costFunctionType == FFNet_COST_MSE ?
			L"Minimum squared error" : L"Minimum cross entropy");
		Graphics_marksLeft (g, 2, 1, 1, 0);
		Graphics_textBottom (g, 1, L"Number of epochs");
		Graphics_marksBottom (g, 2, 1, 1, 0);
	}
}

DIRECT (FFNet_drawTopology)
    EVERY_DRAW (FFNet_drawTopology ((structFFNet *)OBJECT, GRAPHICS))
END

FORM (FFNet_drawWeightsToLayer, L"FFNet: Draw weights to layer", 0)
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Warning: Disapproved. Use \"Draw weights..\" instead.")
    NATURAL (L"Layer number", L"1")
	RADIO (L"Scale", 1)
	RADIOBUTTON (L"by maximum of all weights to layer")
	RADIOBUTTON (L"by maximum weight from 'from-unit'")
	RADIOBUTTON (L"by maximum weight to 'to-unit'")
    BOOLEAN (L"Garnish", 1)
    OK
DO
    EVERY_DRAW (FFNet_drawWeightsToLayer ((structFFNet *)OBJECT, GRAPHICS,
		GET_INTEGER (L"Layer number"),
    	GET_INTEGER (L"Scale"), GET_INTEGER (L"Garnish")))
END

FORM (FFNet_drawWeights, L"FFNet: Draw weights", L"FFNet: Draw weights...")
    NATURAL (L"Layer number", L"1")
    BOOLEAN (L"Garnish", 1)
    OK
DO
    EVERY_DRAW (FFNet_drawWeights ((structFFNet *)OBJECT, GRAPHICS,
		GET_INTEGER (L"Layer number"), GET_INTEGER (L"Garnish")))
END

FORM (FFNet_drawCostHistory, L"FFNet: Draw cost history", L"FFNet: Draw cost history...")
    INTEGER (L"left Iteration_range", L"0")
    INTEGER (L"right Iteration_range", L"0")
    REAL (L"left Cost_range", L"0.0")
    REAL (L"right Cost_range", L"0.0")
    BOOLEAN (L"Garnish", 1)
    OK
DO
    EVERY_DRAW (FFNet_drawCostHistory ((structFFNet *)OBJECT, GRAPHICS,
	GET_INTEGER (L"left Iteration_range"), GET_INTEGER (L"right Iteration_range"),
	GET_REAL (L"left Cost_range"), GET_REAL (L"right Cost_range"), GET_INTEGER (L"Garnish")))
END

FORM (FFNet_extractWeights, L"FFNet: Extract weights", L"FFNet: Extract weights...")
    NATURAL (L"Layer number", L"1")
    OK
DO
    EVERY_TO (FFNet_extractWeights ((structFFNet *)OBJECT, GET_INTEGER (L"Layer number")))
END

FORM (FFNet_weightsToMatrix, L"FFNet: Weights to Matrix ", 0)
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Warning: Use \"Extract weights..\" instead.")
    NATURAL (L"Layer number", L"1")
    OK
DO
    EVERY_TO (FFNet_weightsToMatrix ((structFFNet *)OBJECT, GET_INTEGER (L"Layer number"), 0))
END

/******************* FFNet && Activation *************************************/

FORM (FFNet_Activation_to_Categories, L"FFNet & Activation: To Categories", 0)
	RADIO (L"Kind of labeling", 1)
	RADIOBUTTON (L"Winner-takes-all")
	RADIOBUTTON (L"Stochastic")
	OK
DO
	wchar_t name [200];
	praat_name2 (name, classFFNet, classActivation);
	if (! praat_new1 (FFNet_Activation_to_Categories ((structFFNet *)ONLY (classFFNet), (structActivation *)ONLY (classActivation),
		GET_INTEGER (L"Kind of labeling")), name)) return 0;
END

/******************* FFNet && Eigen ******************************************/

void FFNet_Eigen_drawIntersection (FFNet me, Eigen eigen, Graphics g, long pcx, long pcy,
    double xmin, double xmax, double ymin, double ymax)
{
    long i, ix = labs (pcx), iy = labs (pcy); double x1, x2, y1, y2;
    long numberOfEigenvalues = eigen -> numberOfEigenvalues;
    long dimension = eigen -> dimension;
    
    if (ix > numberOfEigenvalues ||
    	iy > numberOfEigenvalues || my nInputs != dimension) return;
    Melder_assert (ix > 0 && iy > 0);
    if (xmax <= xmin || ymax <= ymin) Graphics_inqWindow (g, & x1, & x2, & y1, & y2);
    if (xmax <= xmin) { xmin = x1; xmax = x2; }
    if (ymax <= ymin) { ymin = y1; ymax = y2; }
    Graphics_setInner (g);
    Graphics_setWindow (g, xmin, xmax, ymin, ymax);
    for (i=1; i <= my nUnitsInLayer[1]; i++)
    {
		long j, unitOffset = my nInputs + 1;
		double c1 = 0.0, c2 = 0.0, bias = my w[ my wLast[unitOffset+i] ];
		double x[6], y[6], xs[3], ys[3]; int ns = 0;
		for (j=1; j <= my nInputs; j++)
		{
	    	c1 += my w[ my wFirst[unitOffset+i] + j - 1 ] * eigen->eigenvectors[ix][j];
	    	c2 += my w[ my wFirst[unitOffset+i] + j - 1 ] * eigen->eigenvectors[iy][j];
		}
		x[1] = x[2] = x[5] = xmin; x[3] = x[4] = xmax;
		y[1] = y[4] = y[5] = ymin; y[2] = y[3] = ymax;
		for (j=1; j <= 4; j++)
		{
	    	double p1 = c1 * x[j  ] + c2 * y[j  ] + bias;
	    	double p2 = c1 * x[j+1] + c2 * y[j+1] + bias;
	    	double r = fabs (p1) / ( fabs (p1) + fabs (p2));
	    	if (p1*p2 > 0 || r == 0.0) continue;
	    	if (++ns > 2) break;
	    	xs[ns] = x[j] + ( x[j+1] - x[j]) * r;
	    	ys[ns] = y[j] + ( y[j+1] - y[j]) * r;
		}
		if (ns < 2) Melder_casual ("Intersection for unit %ld outside range", i);
		else Graphics_line (g, xs[1], ys[1], xs[2], ys[2]);
    }
    Graphics_unsetInner (g);
}

/*
	Draw the intersection line of the decision hyperplane 'w.e-b' of the weights of unit i 
	from layer j with the plane spanned by eigenvectors pcx and pcy.
*/
void FFNet_Eigen_drawDecisionPlaneInEigenspace (FFNet me, thou, Graphics g, long unit,
	long layer, long pcx, long pcy, double xmin, double xmax, double ymin, double ymax)
{
	thouart (Eigen);
    long i, iw, node;
	double ni, xi[3], yi[3]; /* Intersections */
	double x1, x2, y1, y2;
	double bias, we1, we2;
    
	if (layer < 1 || layer > my nLayers) return;
	if (unit < 1 || unit > my nUnitsInLayer[layer]) return;
    if (pcx > thy numberOfEigenvalues || pcy > thy numberOfEigenvalues) return;
	if (my nUnitsInLayer[layer-1] != thy dimension) return;
    
    Graphics_inqWindow (g, & x1, & x2, & y1, & y2);
    if (xmax <= xmin)
	{
		xmin = x1; xmax = x2;
	}
    if (ymax <= ymin)
	{
		ymin = y1; ymax = y2; 
	}
    Graphics_setInner (g);
    Graphics_setWindow (g, xmin, xmax, ymin, ymax);
	
	node = FFNet_getNodeNumberFromUnitNumber (me, unit, layer);
	if (node < 1) return;
	
	/*
		Suppose p1 and p2 are the two points in the eigenplane, spanned by the eigenvectors
		e1 and e2, where the neural net decision hyperplane intersects these eigenvectors.
		Their coordinates in the eigenplane will be (x0*e1, 0) and (0,y0*e2).
		At the same time, the two points are part of the decision hyperplane of the
		chosen unit. The hyperplane equation is:
			w.e+bias = 0,
		where 'w' is the weight vector, 'e' is the input vector and 'b' is the bias.
		This results in two equations for the unknown x0 and y0:
			w.(x0*e1)+bias = 0
			w.(y0*e2)+bias = 0
		This suggests the solution for x0 and y0:
		
			x0 = -bias / (w.e1)
			y0 = -bias / (w.e2)
		
		If w.e1 != 0 && w.e2 != 0
		 	p1 = (x0, 0) and p2 = (0, y0)	
		If w.e1 == 0 && w.e2 != 0
			The line is parallel to e1 and intersects e2 at y0.
		If w.e2 == 0 && w.e1 != 0
			The line is parallel to e2 and intersects e1 at x0.
		If w.e1 == 0 && w.e2 == 0
			Both planes are parallel, no intersection.
	*/
	
	we1 = 0; we2 = 0; iw = my wFirst[node] - 1;
	
	for (i = 1; i <= my nUnitsInLayer[layer-1]; i++)
	{
		we1 += my w[iw + i] * thy eigenvectors[pcx][i];
		we2 += my w[iw + i] * thy eigenvectors[pcy][i];
	}
	
	bias = my w[my wLast[node]];
	x1 = xmin; x2 = xmax;
	y1 = ymin; y2 = ymax;
	if (we1 != 0)
	{
		x1 = -bias / we1; y1 = 0;
	}
	if (we2 != 0)
	{
		x2 = 0; y2 = -bias / we2;
	}
	if (we1 == 0 && we2 == 0)
	{
		Melder_warning5 (L"We cannot draw the intersection of the neural net decision plane\n"
			"for unit ", Melder_integer (unit), L" in layer ", Melder_integer (layer), 
			L" with the plane spanned by the eigenvectors because \nboth planes are parallel.");
		return;
	}
	ni = NUMgetIntersectionsWithRectangle (x1, y1, x2, y2, xmin, ymin, xmax, ymax, xi, yi);
	if (ni == 2) Graphics_line (g, xi[1], yi[1], xi[2], yi[2]);
	else Melder_warning1 (L"There were no intersections in the drawing area.\n"
		"Please enlarge the drawing area.");
    Graphics_unsetInner (g);
}

FORM (FFNet_Eigen_drawIntersection, L"FFnet & Eigen: Draw intersection", 0)
    INTEGER (L"X-component", L"1")
    INTEGER (L"Y-component", L"2")
    REAL (L"xmin", L"0.0")
    REAL (L"xmax", L"0.0")
    REAL (L"ymin", L"0.0")
    REAL (L"ymax", L"0.0")
    OK
DO
    long pcx = GET_INTEGER (L"X-component"), pcy = GET_INTEGER (L"Y-component");
    REQUIRE (pcx != 0 && pcy != 0, L"X and Y component must differ from 0.")
    praat_picture_open ();
    FFNet_Eigen_drawIntersection ((structFFNet *)ONLY (classFFNet), (structEigen *)ONLY (classEigen),
    	GRAPHICS, pcx, pcy, GET_REAL (L"xmin"), GET_REAL (L"xmax"),
    	GET_REAL (L"ymin"), GET_REAL (L"ymax"));
    praat_picture_close ();
END

FORM (FFNet_PCA_drawDecisionPlaneInEigenspace, L"FFNet & PCA: Draw decision plane", L"ui/editors/AmplitudeTierEditor.h")
	NATURAL (L"Unit number", L"1")
	NATURAL (L"Layer number", L"1")
    NATURAL (L"Horizontal eigenvector number", L"1")
    NATURAL (L"Vertical eigenvector number", L"2")
	REAL (L"left Horizontal range", L"0.0")
	REAL (L"right Horizontal range", L"0.0")
	REAL (L"left Vertical range", L"0.0")
	REAL (L"right Vertical range", L"0.0")
	OK
DO
    praat_picture_open ();
	FFNet_Eigen_drawDecisionPlaneInEigenspace((structFFNet *)ONLY (classFFNet), (structPCA *)ONLY (classPCA),
		GRAPHICS, GET_INTEGER (L"Unit number"), GET_INTEGER (L"Layer number"),
		GET_INTEGER (L"Horizontal eigenvector number"),
		GET_INTEGER (L"Vertical eigenvector number"), GET_REAL (L"left Horizontal range"),
		GET_REAL (L"right Horizontal range"), GET_REAL (L"left Vertical range"),
		GET_REAL (L"right Vertical range"));
    praat_picture_close ();

END

/************************* FFNet && Categories **********************************/

DIRECT (FFNet_Categories_to_Activation)
	NEW (FFNet_Categories_to_Activation ((structFFNet *)ONLY (classFFNet), (structCategories *)ONLY (classCategories)))
END

/************************* FFNet && Matrix **********************************/

FORM (FFNet_weightsFromMatrix, L"Replace weights by values from Matrix", 0)
    NATURAL (L"Layer", L"1")
    OK
DO
    NEW (FFNet_weightsFromMatrix ((structFFNet *)ONLY (classFFNet), (structMatrix *)ONLY (classMatrix),
    	GET_INTEGER (L"Layer")));
END

/************************* FFNet && Pattern **********************************/

void FFNet_Pattern_drawActivation (FFNet me, Pattern pattern, Graphics g, long index)
{
    if (index < 1 || index > pattern->ny) return;
    FFNet_propagate (me, pattern->z[index], NULL);
    FFNet_drawActivation (me, g);
}

FORM (FFNet_Pattern_drawActivation, L"Draw an activation", 0)
	NATURAL (L"Pattern (row) number", L"1");
	OK
DO
	EVERY_DRAW (FFNet_Pattern_drawActivation ((structFFNet *)ONLY (classFFNet), (structPattern *)ONLY (classPattern),
		GRAPHICS, GET_INTEGER (L"Pattern")))
END

FORM (FFNet_Pattern_to_Activation, L"To activations in layer", 0)
	NATURAL (L"Layer", L"1")
	OK
DO
	wchar_t name [200];
	praat_name2 (name, classFFNet, classPattern);
	if (! praat_new1 (FFNet_Pattern_to_Activation ((structFFNet *)ONLY (classFFNet),
		(structPattern *)ONLY (classPattern), GET_INTEGER (L"Layer")), name)) return 0;
END

DIRECT (hint_FFNet_and_Pattern_classify)
	Melder_information1 (L"You can use the FFNet as a classifier by selecting a\n"
		"FFNet and a Pattern together and choosing \"To Categories...\".");
END

DIRECT (hint_FFNet_and_Pattern_and_Categories_learn)
	Melder_information1 (L"You can teach a FFNet to classify by selecting a\n"
		"FFNet, a Pattern and a Categories together and choosing \"Learn...\".");
END

FORM (FFNet_Pattern_to_Categories, L"FFNet & Pattern: To Categories", L"FFNet & Pattern: To Categories...")
	RADIO (L"Determine output category as", 1)
	RADIOBUTTON (L"Winner-takes-all")
	RADIOBUTTON (L"Stochastic")
	OK
DO
	wchar_t name [200];
	praat_name2 (name, classFFNet, classPattern);
	if (! praat_new1 (FFNet_Pattern_to_Categories ((structFFNet *)ONLY (classFFNet),
		(structPattern *)ONLY (classPattern), GET_INTEGER (L"Determine output category as")), name)) return 0;
END

/*********** FFNet Pattern Activation **********************************/

FORM (FFNet_Pattern_Activation_getCosts_total, L"FFNet & Pattern & Activation: Get total costs", L"FFNet & Pattern & Activation: Get total costs...")
	RADIO (L"Cost function", 1)
	RADIOBUTTON (L"Minimum-squared-error")
	RADIOBUTTON (L"Minimum-cross-entropy")
	OK
DO
	Melder_information1 (Melder_double (FFNet_Pattern_Activation_getCosts_total ((structFFNet *)ONLY (classFFNet),
		(structPattern *)ONLY (classPattern), (structActivation *)ONLY (classActivation), GET_INTEGER (L"Cost function"))));
END

FORM (FFNet_Pattern_Activation_getCosts_average, L"FFNet & Pattern & Activation: Get average costs", L"FFNet & Pattern & Activation: Get average costs...")
	RADIO (L"Cost function", 1)
	RADIOBUTTON (L"Minimum-squared-error")
	RADIOBUTTON (L"Minimum-cross-entropy")
	OK
DO
	Melder_information1 (Melder_double (FFNet_Pattern_Activation_getCosts_average ((structFFNet *)ONLY (classFFNet),
		(structPattern *)ONLY (classPattern), (structActivation *)ONLY (classActivation), GET_INTEGER (L"Cost function"))));
END

FORM (FFNet_Pattern_Activation_learnSD, L"FFNet & Pattern & Activation: Learn slow", 0)
    NATURAL (L"Maximum number of epochs", L"100")
    POSITIVE (L"Tolerance of minimizer", L"1e-7")
    LABEL (L"Specifics", L"Specific for this minimization")
    POSITIVE (L"Learning rate", L"0.1")
    REAL (L"Momentum", L"0.9")
	RADIO (L"Cost function", 1)
	RADIOBUTTON (L"Minimum-squared-error")
	RADIOBUTTON (L"Minimum-cross-entropy")
    OK
DO
	struct structSteepestDescentMinimizer_parameters_decl p;
    p.eta = GET_REAL (L"Learning rate");
    p.momentum = GET_REAL (L"Momentum");
    return FFNet_Pattern_Activation_learnSD ((structFFNet *)ONLY (classFFNet), (structPattern *)ONLY (classPattern),
    	(structActivation *)ONLY (classActivation), GET_INTEGER (L"Maximum number of epochs"),
		GET_REAL (L"Tolerance of minimizer"), & p, GET_INTEGER (L"Cost function"));
END

FORM (FFNet_Pattern_Activation_learnSM, L"FFNet & Pattern & Activation: Learn", 0)
    NATURAL (L"Maximum number of epochs", L"100")
    POSITIVE (L"Tolerance of minimizer", L"1e-7")
	RADIO (L"Cost function", 1)
	RADIOBUTTON (L"Minimum-squared-error")
	RADIOBUTTON (L"Minimum-cross-entropy")
    OK
DO
    return FFNet_Pattern_Activation_learnSM ((structFFNet *)ONLY (classFFNet),
		(structPattern *)ONLY (classPattern), (structActivation *)ONLY (classActivation),
		GET_INTEGER (L"Maximum number of epochs"),
		GET_REAL (L"Tolerance of minimizer"), NULL,
		GET_INTEGER (L"Cost function"));
END

/*********** FFNet Pattern Categories **********************************/

FORM (FFNet_Pattern_Categories_getCosts_total, L"FFNet & Pattern & Categories: Get total costs", L"FFNet & Pattern & Categories: Get total costs...")
	RADIO (L"Cost function", 1)
	RADIOBUTTON (L"Minimum-squared-error")
	RADIOBUTTON (L"Minimum-cross-entropy")
	OK
DO
	Melder_information1 (Melder_double (FFNet_Pattern_Categories_getCosts_total ((structFFNet *)ONLY (classFFNet),
		(structPattern *)ONLY (classPattern), (structCategories *)ONLY (classCategories), GET_INTEGER (L"Cost function"))));
END

FORM (FFNet_Pattern_Categories_getCosts_average, L"FFNet & Pattern & Categories: Get average costs", L"FFNet & Pattern & Categories: Get average costs...")
	RADIO (L"Cost function", 1)
	RADIOBUTTON (L"Minimum-squared-error")
	RADIOBUTTON (L"Minimum-cross-entropy")
	OK
DO
	Melder_information1 (Melder_double (FFNet_Pattern_Categories_getCosts_average ((structFFNet *)ONLY (classFFNet),
		(structPattern *)ONLY (classPattern), (structCategories *)ONLY (classCategories), GET_INTEGER (L"Cost function"))));
END

FORM (Pattern_Categories_to_FFNet, L"Pattern & Categories: To FFNet", L"Pattern & Categories: To FFNet...")
    INTEGER (L"Number of units in hidden layer 1", L"0")
    INTEGER (L"Number of units in hidden layer 2", L"0")
    OK
DO
	Pattern p = (structPattern *)ONLY (classPattern);
	Categories uniq = NULL, l = (structCategories *)ONLY (classCategories);
	long numberOfOutputs;
	FFNet ffnet = NULL;
	long nHidden1 = GET_INTEGER (L"Number of units in hidden layer 1");
	long nHidden2 = GET_INTEGER (L"Number of units in hidden layer 2");
	MelderString ffnetName = { 0 };

	if (nHidden1 < 1) nHidden1 = 0;
 	if (nHidden2 < 1) nHidden2 = 0;
	uniq = Categories_selectUniqueItems (l, 1);
	if (uniq == NULL) return Melder_error1 (L"There is not enough memory to create the output categories.\n"
			"Please try again with less categories.");
	numberOfOutputs = uniq -> size;
	if (numberOfOutputs < 1)
	{
		forget (uniq);
		return Melder_error1 (L"There are not enough categories in the Categories.\n"
			"Please try again with more categories in the Categories.");
	}

    ffnet = FFNet_create (p -> nx, nHidden1, nHidden2, numberOfOutputs, 0);
	if (ffnet == NULL)
	{
		forget (uniq);
		return Melder_error1 (L"There was not enough memory to create the FFNet.\n"
			"Please try again with less hidden nodes in the hidden layer(s).");
	}
	if (! FFNet_setOutputCategories (ffnet, uniq))
	{
		forget (uniq); forget (ffnet);
		return 0;
	}
	FFNet_createNameFromTopology (ffnet, &ffnetName);
	int status = praat_new1 (ffnet, ffnetName.string);
	MelderString_free (&ffnetName);
	return status;
END

FORM (FFNet_Pattern_Categories_learnSM, L"FFNet & Pattern & Categories: Learn", L"FFNet & Pattern & Categories: Learn...")
    NATURAL (L"Maximum number of epochs", L"100")
    POSITIVE (L"Tolerance of minimizer", L"1e-7")
	RADIO (L"Cost function", 1)
	RADIOBUTTON (L"Minimum-squared-error")
	RADIOBUTTON (L"Minimum-cross-entropy")
    OK
DO
	if (! FFNet_Pattern_Categories_learnSM ((structFFNet *)ONLY (classFFNet),
		(structPattern *)ONLY (classPattern), (structCategories *)ONLY (classCategories),
		GET_INTEGER (L"Maximum number of epochs"),
		GET_REAL (L"Tolerance of minimizer"), NULL,
		GET_INTEGER (L"Cost function"))) return 0;
END

FORM (FFNet_Pattern_Categories_learnSD, L"FFNet & Pattern & Categories: Learn slow", L"FFNet & Pattern & Categories: Learn slow...")
    NATURAL (L"Maximum number of epochs", L"100")
    POSITIVE (L"Tolerance of minimizer", L"1e-7")
    LABEL (L"Specifics", L"Specific for this minimization")
    POSITIVE (L"Learning rate", L"0.1")
    REAL (L"Momentum", L"0.9")
	RADIO (L"Cost function", 1)
	RADIOBUTTON (L"Minimum-squared-error")
	RADIOBUTTON (L"Minimum-cross-entropy")
    OK
DO
	struct structSteepestDescentMinimizer_parameters_decl p;
	p.eta = GET_REAL (L"Learning rate");
    p.momentum = GET_REAL (L"Momentum");
    return FFNet_Pattern_Categories_learnSD ((structFFNet *)ONLY (classFFNet),
		(structPattern *)ONLY (classPattern), (structCategories *)ONLY (classCategories),
		GET_INTEGER (L"Maximum number of epochs"),
		GET_REAL (L"Tolerance of minimizer"), &p,
		GET_INTEGER (L"Cost function"));
END

#ifdef __cplusplus
	extern "C" {
#endif
void praat_uvafon_FFNet_init (void);
#ifdef __cplusplus
	}
#endif

void praat_uvafon_FFNet_init (void)
{
	Thing_recognizeClassesByName (classFFNet, NULL);

    praat_addMenuCommand (L"Objects", L"New", L"Neural nets", 0, 0, 0);
	praat_addMenuCommand (L"Objects", L"New", L"Feedforward neural networks", 0, 1, DO_FFNet_help);
    praat_addMenuCommand (L"Objects", L"New", L"-- FFNet --", 0, 1, 0);
    praat_addMenuCommand (L"Objects", L"New", L"Create iris example...", 0, 1, DO_FFNet_createIrisExample);
    praat_addMenuCommand (L"Objects", L"New", L"Create FFNet...", 0, 1, DO_FFNet_create);
	praat_addMenuCommand (L"Objects", L"New", L"Advanced", 0, 1, 0);
    praat_addMenuCommand (L"Objects", L"New", L"Create Pattern...", 0, 2, DO_Pattern_create);
    praat_addMenuCommand (L"Objects", L"New", L"Create Categories...", 0, 2, DO_Categories_create);
	praat_addMenuCommand (L"Objects", L"New", L"Create FFNet (linear outputs)...", 0, 2, DO_FFNet_create_linearOutputs);

	praat_addAction1 (classFFNet, 0, L"FFNet help", 0, 0, DO_FFNet_help);
	praat_addAction1 (classFFNet, 0, DRAW_BUTTON, 0, 0, 0);
	praat_addAction1 (classFFNet, 0, L"Draw topology", 0, 1, DO_FFNet_drawTopology);
	praat_addAction1 (classFFNet, 0, L"Draw weights...", 0, 1, DO_FFNet_drawWeights);
	praat_addAction1 (classFFNet, 0, L"Draw weights to layer...", 0,  praat_DEPTH_1 | praat_HIDDEN, DO_FFNet_drawWeightsToLayer);
	praat_addAction1 (classFFNet, 0, L"Draw cost history...", 0, 1, DO_FFNet_drawCostHistory);
	praat_addAction1 (classFFNet, 0, QUERY_BUTTON, 0, 0, 0);
		praat_addAction1 (classFFNet, 0, L"Query structure", 0, 1, 0);
		praat_addAction1 (classFFNet, 1, L"Get number of outputs", 0, 2, DO_FFNet_getNumberOfOutputs);
		praat_addAction1 (classFFNet, 1, L"Get number of hidden units...", 0, 2, DO_FFNet_getNumberOfHiddenUnits);
		praat_addAction1 (classFFNet, 1, L"Get number of inputs", 0, 2, DO_FFNet_getNumberOfInputs);
		praat_addAction1 (classFFNet, 1, L"Get number of hidden weights...", 0, 2, DO_FFNet_getNumberOfHiddenWeights);
		praat_addAction1 (classFFNet, 1, L"Get number of output weights", 0, 2, DO_FFNet_getNumberOfOutputWeights);
		praat_addAction1 (classFFNet, 1, L"Get category of output unit...", 0, 2, DO_FFNet_getCategoryOfOutputUnit);
		praat_addAction1 (classFFNet, 1, L"Get output unit of category...", 0, 2, DO_FFNet_getOutputUnitOfCategory);
		praat_addAction1 (classFFNet, 0, L"-- FFNet weights --", 0, 1, 0);
		praat_addAction1 (classFFNet, 1, L"Get minimum", 0, 1, DO_FFNet_getMinimum);
	praat_addAction1 (classFFNet, 0, MODIFY_BUTTON, 0, 0, 0);
	praat_addAction1 (classFFNet, 1, L"Reset...", 0, 1, DO_FFNet_reset);
	praat_addAction1 (classFFNet, 0, L"Select biases...", 0, 1, DO_FFNet_selectBiasesInLayer);
	praat_addAction1 (classFFNet, 0, L"Select all weights", 0, 1, DO_FFNet_selectAllWeights);
	praat_addAction1 (classFFNet, 0, EXTRACT_BUTTON, 0, 0, 0);
	praat_addAction1 (classFFNet, 0, L"Extract weights...", 0, 1, DO_FFNet_extractWeights);
	praat_addAction1 (classFFNet, 0, L"Weights to Matrix...", 0, praat_DEPTH_1 | praat_HIDDEN, DO_FFNet_weightsToMatrix);
	praat_addAction1 (classFFNet, 0, L"& Pattern: Classify?", 0, 0, DO_hint_FFNet_and_Pattern_classify);
	praat_addAction1 (classFFNet, 0, L"& Pattern & Categories: Learn?", 0, 0, DO_hint_FFNet_and_Pattern_and_Categories_learn);

	praat_addAction2 (classFFNet, 1, classActivation, 1, L"Analyse", 0, 0, 0);
	praat_addAction2 (classFFNet, 1, classActivation, 1, L"To Categories...", 0, 0, DO_FFNet_Activation_to_Categories);

	praat_addAction2 (classFFNet, 1, classEigen, 1, L"Draw", 0, 0, 0);
	praat_addAction2 (classFFNet, 1, classEigen, 1, L"Draw hyperplane intersections", 0, 0, DO_FFNet_Eigen_drawIntersection);

	praat_addAction2 (classFFNet, 1, classCategories, 1, L"Analyse", 0, 0, 0);
	praat_addAction2 (classFFNet, 1, classCategories, 1, L"To Activation", 0, 0, DO_FFNet_Categories_to_Activation);

	praat_addAction2 (classFFNet, 1, classMatrix, 1, L"Modify", 0, 0, 0);
	praat_addAction2 (classFFNet, 1, classMatrix, 1, L"Weights from Matrix...", 0, 0, DO_FFNet_weightsFromMatrix);

	praat_addAction2 (classFFNet, 1, classPattern, 1, L"Draw", 0, 0, 0);
	praat_addAction2 (classFFNet, 1, classPattern, 1, L"Draw activation...", 0, 0, DO_FFNet_Pattern_drawActivation);
	praat_addAction2 (classFFNet, 1, classPattern, 1, L"Analyse", 0, 0, 0);
	praat_addAction2 (classFFNet, 1, classPattern, 1, L"To Categories...", 0, 0, DO_FFNet_Pattern_to_Categories);
	praat_addAction2 (classFFNet, 1, classPattern, 1, L"To Activation...", 0, 0, DO_FFNet_Pattern_to_Activation);

	praat_addAction2 (classFFNet, 1, classPCA, 1, L"Draw decision plane...", 0, 0, DO_FFNet_PCA_drawDecisionPlaneInEigenspace);

	praat_addAction2 (classPattern, 1, classCategories, 1, L"To FFNet...", 0, 0, DO_Pattern_Categories_to_FFNet);

	praat_addAction3 (classFFNet, 1, classPattern, 1, classActivation, 1, L"Get total costs...", 0, 0, DO_FFNet_Pattern_Activation_getCosts_total);
	praat_addAction3 (classFFNet, 1, classPattern, 1, classActivation, 1, L"Get average costs...", 0, 0, DO_FFNet_Pattern_Activation_getCosts_average);
	praat_addAction3 (classFFNet, 1, classPattern, 1, classActivation, 1, L"Learn", 0, 0, 0);
	praat_addAction3 (classFFNet, 1, classPattern, 1, classActivation, 1, L"Learn...", 0, 0, DO_FFNet_Pattern_Activation_learnSM);
	praat_addAction3 (classFFNet, 1, classPattern, 1, classActivation, 1, L"Learn slow...", 0, 0, DO_FFNet_Pattern_Activation_learnSD);

	praat_addAction3 (classFFNet, 1, classPattern, 1, classCategories, 1, L"Get total costs...", 0, 0, DO_FFNet_Pattern_Categories_getCosts_total);
	praat_addAction3 (classFFNet, 1, classPattern, 1, classCategories, 1, L"Get average costs...", 0, 0, DO_FFNet_Pattern_Categories_getCosts_average);
	praat_addAction3 (classFFNet, 1, classPattern, 1, classCategories, 1, L"Learn", 0, 0, 0);
	praat_addAction3 (classFFNet, 1, classPattern, 1, classCategories, 1, L"Learn...", 0, 0, DO_FFNet_Pattern_Categories_learnSM);
	praat_addAction3 (classFFNet, 1, classPattern, 1, classCategories, 1, L"Learn slow...", 0, 0, DO_FFNet_Pattern_Categories_learnSD);
}
