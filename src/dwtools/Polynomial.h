#ifndef _Polynomial_h_
#define _Polynomial_h_
/* Polynomial.h
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
 djmw 20020813 GPL header
 djmw 20110306 Latest modification.
*/

#define FITTER_PARAMETER_FREE 0
#define FITTER_PARAMETER_FIXED 1

#ifndef _SimpleVector_h_
	#include "dwsys/SimpleVector.h"
#endif
#ifndef _Function_h_
	#include "fon/Function.h"
#endif
#ifndef _TableOfReal_h_
	#include "stat/TableOfReal.h"
#endif
#ifndef _Graphics_h_
	#include "sys/Graphics.h"
#endif
#ifndef _Minimizers_h_
	#include "Minimizers.h"
#endif
#ifndef _Spectrum_h_
	#include "fon/Spectrum.h"
#endif
#ifndef _RealTier_h_
	#include "fon/RealTier.h"
#endif
#ifndef _SSCP_h_
	#include "SSCP.h"
#endif

#ifdef __cplusplus
	extern "C" {
#endif

#define Spline_MAXIMUM_DEGREE 20

#define FunctionTerms_members Function_members	\
	long numberOfCoefficients;	\
	double *coefficients;
#define FunctionTerms_methods Function_methods	\
	double (*evaluate) (I, double x);	\
	void (*evaluate_z) (I, dcomplex *z, dcomplex *p); \
	void (*evaluateTerms) (I, double x, double terms[]); \
	void (*getExtrema) (I, double x1, double x2, double *xmin, double *ymin, \
			double *xmax, double *ymax);	\
	long (*getDegree) (I);
class_create (FunctionTerms, Function);

int FunctionTerms_init (I, double xmin, double xmax, long numberOfCoefficients);

int FunctionTerms_initFromString (I, double xmin, double xmax, wchar_t *s, int allowTrailingZeros);

FunctionTerms FunctionTerms_create (double xmin, double xmax, long numberOfCoefficients);

void FunctionTerms_setDomain (I, double xmin, double xmax);

int FunctionTerms_setCoefficient (I, long index, double value);

double FunctionTerms_evaluate (I, double x);

void FunctionTerms_evaluate_z (I, dcomplex *z, dcomplex *p);

void FunctionTerms_evaluateTerms (I, double x, double terms[]);

void FunctionTerms_getExtrema (I, double x1, double x2, double *xmin, double *ymin,
	double *xmax, double *ymax);

long FunctionTerms_getDegree (I);

double FunctionTerms_getMinimum (I, double x1, double x2);

double FunctionTerms_getXOfMinimum (I, double x1, double x2);

double FunctionTerms_getMaximum (I, double x1, double x2);

double FunctionTerms_getXOfMaximum (I, double x1, double x2);
/*
	Returns minimum and maximum function values (ymin, ymax) in
	interval [x1, x2] and their x-values (xmin, xmax).
	Precondition: [x1, x2] is a (sub)domain 
		my xmin <= x1 < x2 <= my xmax
*/

void FunctionTerms_draw (I, Graphics g, double xmin, double xmax, double ymin, double ymax, 
	int extrapolate, int garnish);
/*
	Extrapolate only for functions whose domain is extendable and that can be extrapolated.
	Polynomials can be extrapolated.
	LegendreSeries and ChebyshevSeries cannot be extrapolated.
*/
void FunctionTerms_drawBasisFunction (I, Graphics g, long index, double xmin, double xmax,
	double ymin, double ymax, int extrapolate, int garnish);

#define Polynomial_members FunctionTerms_members
#define Polynomial_methods FunctionTerms_methods
class_create (Polynomial, FunctionTerms);

#define Roots_members ComplexVector_members
#define Roots_methods ComplexVector_methods
class_create (Roots, ComplexVector);

Polynomial Polynomial_create (double xmin, double xmax, long degree);

Polynomial Polynomial_createFromString (double xmin, double xmax, wchar_t *s);

void Polynomial_scaleCoefficients_monic (Polynomial me);
/* Make coefficent of leading term 1.0 */

Polynomial Polynomial_scaleX (Polynomial me, double xmin, double xmax);
/* x' = (x-location) / scale */

void Polynomial_evaluate_z (Polynomial me, dcomplex *z, dcomplex *p);
/* Evaluate at complex z = x + iy */

	
double Polynomial_getArea (Polynomial me, double xmin, double xmax);

Polynomial Polynomial_getDerivative (Polynomial me);

Polynomial Polynomial_getPrimitive (Polynomial me);

void Polynomial_draw (I, Graphics g, double xmin, double xmax, double ymin, double ymax, int garnish);

double Polynomial_evaluate (I, double x);

void Polynomial_evaluateTerms (I, double x, double terms[]);

Polynomial Polynomials_multiply (Polynomial me, Polynomial thee);

int Polynomials_divide (Polynomial me, Polynomial thee, Polynomial *q, Polynomial *r);

#define LegendreSeries_members FunctionTerms_members
#define LegendreSeries_methods FunctionTerms_methods
class_create (LegendreSeries, FunctionTerms);

LegendreSeries LegendreSeries_create (double xmin, double xmax, long numberOfPolynomials);

LegendreSeries LegendreSeries_createFromString (double xmin, double xmax, wchar_t *s);

LegendreSeries LegendreSeries_getDerivative (LegendreSeries me);

Polynomial LegendreSeries_to_Polynomial (LegendreSeries me);

Roots Roots_create (long numberOfRoots);

void Roots_fixIntoUnitCircle (Roots me);

void Roots_sort (Roots me);
/* Sort to size of real part a+bi, a-bi*/

dcomplex Roots_evaluate_z (Roots me, dcomplex z);

Roots Polynomial_to_Roots_ev (Polynomial me);

long Roots_getNumberOfRoots (Roots me);

void Roots_draw (Roots me, Graphics g, double rmin, double rmax, double imin, double imax, 
	wchar_t *symbol, int fontSize, int garnish);
	
dcomplex Roots_getRoot (Roots me, long index);
int Roots_setRoot (Roots me, long index, double re, double im);

Spectrum Roots_to_Spectrum (Roots me, double nyquistFrequency,
	long numberOfFrequencies, double radius);
	
Roots Polynomial_to_Roots (Polynomial me);
/* Find roots of polynomial and polish them */

void Roots_and_Polynomial_polish (Roots me, Polynomial thee);

Polynomial Roots_to_Polynomial (Roots me);

Polynomial TableOfReal_to_Polynomial (I, long degree, long xcol,
	long ycol, long scol);

LegendreSeries TableOfReal_to_LegendreSeries (I, long numberOfPolynomials, 
	long xcol, long ycol, long scol);
	
Spectrum Polynomial_to_Spectrum (Polynomial me, double nyquistFrequency,
	long numberOfFrequencies, double radius);

/*
	A ChebyshevSeries p(x) on a domain [xmin,xmax] is defined as the
	following linear combination of Chebyshev polynomials T[k](x') of
	degree k-1 and domain [-1, 1]:
		p(x) = sum (k=1..numberOfCoefficients, c[k]*T[k](x')) - c[1] / 2, where
		x' = (2 * x - xmin - xmax) / (xmax - xmin)
	This is equivalent to:
		p(x) = c[1] /2 + sum (k=2..numberOfCoefficients, c[k]*T[k](x'))
*/	
#define ChebyshevSeries_members FunctionTerms_members
#define ChebyshevSeries_methods FunctionTerms_methods
class_create (ChebyshevSeries, FunctionTerms);
	
ChebyshevSeries ChebyshevSeries_create (double xmin, double xmax, long numberOfPolynomials);

ChebyshevSeries ChebyshevSeries_createFromString (double xmin, double xmax, wchar_t *s);

Polynomial ChebyshevSeries_to_Polynomial (ChebyshevSeries me);

#define Spline_members FunctionTerms_members	\
	long degree, numberOfKnots;	\
	double *knots;
#define Spline_methods FunctionTerms_methods \
	long (*getOrder) (I);
class_create (Spline, FunctionTerms);

int Spline_init (I, double xmin, double xmax, long degree, long numberOfCoefficients, long numberOfKnots);

long Spline_getOrder (I);

void Spline_drawKnots (I, Graphics g, double xmin, double xmax, double ymin, double ymax, int garnish);

Spline Spline_scaleX (I, double xmin, double xmax);
/* scale domain and knots to new domain */

#define MSpline_members Spline_members
#define MSpline_methods Spline_methods
class_create (MSpline, Spline);

MSpline MSpline_create (double xmin, double xmax, long degree, long numberOfInteriorKnots);

MSpline MSpline_createFromStrings (double xmin, double xmax, long degree, wchar_t *coef, wchar_t *interiorKnots);

#define ISpline_members Spline_members
#define ISpline_methods Spline_methods
class_create (ISpline, Spline);

ISpline ISpline_create (double xmin, double xmax, long degree, long numberOfInteriorKnots);
ISpline ISpline_createFromStrings (double xmin, double xmax, long degree, wchar_t *coef, wchar_t *interiorKnots);

/****************** fit **********************************************/

int FunctionTerms_and_RealTier_fit (I, thou, int *freezeCoefficients, double tol, int ic, Covariance *c);

Polynomial RealTier_to_Polynomial (I, long degree, double tol, int ic, Covariance *cvm);

LegendreSeries RealTier_to_LegendreSeries (I, long degree, double tol, int ic, Covariance *cvm);

ChebyshevSeries RealTier_to_ChebyshevSeries (I, long degree, double tol, int ic, Covariance *cvm);

#ifdef __cplusplus
	}
#endif

#endif /* _Polynomial_h_ */
