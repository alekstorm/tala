/* FFNet_Eigen.c
 *
 * Copyright (C) 1994-2008 David Weenink
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
 djmw 20020712 GPL header
 djmw 20071202 Melder_warning<n>
*/

#include "FFNet_Eigen.h"
#include "Graphics.h"
#include "NUM2.h"

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

/* End of file FFNet_Eigen.c */
