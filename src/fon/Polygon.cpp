/* Polygon.c
 *
 * Copyright (C) 1992-2009 Paul Boersma
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
 * pb 2004/01/27 paint with colours
 * pb 2004/06/15 allow reversed axes
 * pb 2007/06/21 tex
 * pb 2007/10/01 canWriteAsEncoding
 * pb 2008/01/19 double
 * pb 2009/12/14 RGB colours
 */

#include "Polygon.h"

#include "sys/oo/oo_DESTROY.h"
#include "Polygon_def.h"
#include "sys/oo/oo_COPY.h"
#include "Polygon_def.h"
#include "sys/oo/oo_EQUAL.h"
#include "Polygon_def.h"
#include "sys/oo/oo_CAN_WRITE_AS_ENCODING.h"
#include "Polygon_def.h"
#include "sys/oo/oo_WRITE_BINARY.h"
#include "Polygon_def.h"
#include "sys/oo/oo_READ_BINARY.h"
#include "Polygon_def.h"
#include "sys/oo/oo_DESCRIPTION.h"
#include "Polygon_def.h"

static void info (I) {
	iam (Polygon);
	classData -> info (me);
	MelderInfo_writeLine2 (L"Number of points: ", Melder_integer (my numberOfPoints));
	MelderInfo_writeLine2 (L"Perimeter: ", Melder_single (Polygon_perimeter (me)));
}
  
static int writeText (I, MelderFile file) {
	iam (Polygon);
	texputi4 (file, my numberOfPoints, L"numberOfPoints", 0,0,0,0,0);
	for (long i = 1; i <= my numberOfPoints; i ++) {
		texputr4 (file, my x [i], L"x [", Melder_integer (i), L"]", 0,0,0);
		texputr4 (file, my y [i], L"y [", Melder_integer (i), L"]", 0,0,0);
	}
	return 1;
}

static int readText (I, MelderReadText text) {
	iam (Polygon);
	my numberOfPoints = texgeti4 (text);
	if (my numberOfPoints < 1)
		return Melder_error3 (L"Cannot read a Polygon with only ", Melder_integer (my numberOfPoints), L" points.");
	if (! (my x = NUMdvector (1, my numberOfPoints)) || ! (my y = NUMdvector (1, my numberOfPoints)))
		return 0;
	for (long i = 1; i <= my numberOfPoints; i ++) {
		my x [i] = texgetr4 (text);
		my y [i] = texgetr4 (text);
	}
	return 1;
}

class_methods (Polygon, Data) {
	us -> version = 1;
	class_method_local (Polygon, destroy)
	class_method (info)
	class_method_local (Polygon, description)
	class_method_local (Polygon, copy)
	class_method_local (Polygon, equal)
	class_method_local (Polygon, canWriteAsEncoding)
	class_method (writeText)
	class_method (readText)
	class_method_local (Polygon, writeBinary)
	class_method_local (Polygon, readBinary)
	class_methods_end
}

Polygon Polygon_create (long numberOfPoints) {
	Polygon me = Thing_new (Polygon);
	if (! me) return NULL;
	my numberOfPoints = numberOfPoints;
	if (! (my x = NUMdvector (1, numberOfPoints)) || ! (my y = NUMdvector (1, numberOfPoints)))
		{ forget (me); return (structPolygon *)Melder_errorp ("Polygon not created."); }
	return me;
}

void Polygon_randomize (I) {
	iam (Polygon);
	long i, j;
	double xdum, ydum;
	for (i = 1; i <= my numberOfPoints; i ++) {
		j = NUMrandomInteger (i, my numberOfPoints);
		xdum = my x [i];
		ydum = my y [i];
		my x [i] = my x [j];
		my y [i] = my y [j];
		my x [j] = xdum;
		my y [j] = ydum;
	}
}

double Polygon_perimeter (I) {
	iam (Polygon);
	double dx, dy;
	double result = sqrt (( dx = my x [1] - my x [my numberOfPoints], dx * dx ) +
					( dy = my y [1] - my y [my numberOfPoints], dy * dy ));
	long i;
	for (i = 1; i <= my numberOfPoints - 1; i ++)
		result += sqrt (( dx = my x [i] - my x [i + 1], dx * dx ) + ( dy = my y [i] - my y [i + 1], dy * dy ));
	return result;
}

static void computeDistanceTable (Polygon me, int **table) {
	for (long i = 1; i <= my numberOfPoints - 1; i ++)
		for (long j = i + 1; j <= my numberOfPoints; j ++) {
			double dx, dy;
			table [i] [j] = table [j] [i] =
				sqrt (( dx = my x [i] - my x [j], dx * dx ) + ( dy = my y [i] - my y [j], dy * dy ));
					/* Round to zero. */
		}
}

static long computeTotalDistance (int **distance, int path [], int numberOfCities) {
	long result = 0;
	for (long i = 1; i <= numberOfCities; i ++)
		result += distance [path [i - 1]] [path [i]];
	return result;
}

static void shuffle (int path [], int numberOfCities) {
	for (long i = 1; i <= numberOfCities; i ++) {
		int j = NUMrandomInteger (i, numberOfCities);
		int help = path [i];
		path [i] = path [j];
		path [j] = help;
	}
	path [0] = path [numberOfCities];
}
  
static int tryExchange (int **distance, int *path, int numberOfCities, long *totalDistance) {
	int result = 0;
	int b1 = path [0];
	int b2nr = 1;
	while (b2nr < numberOfCities - 1) {
		int b2 = path [b2nr];
		int distance_b1_b2 = distance [b1] [b2];
		int d2nr = b2nr + 2;
		int d1 = path [d2nr - 1];
		int cont = 1;
		while (d2nr <= numberOfCities && cont) {
			int d2 = path [d2nr];
			int gain = distance_b1_b2 + distance [d1] [d2] - distance [b1] [d1] - distance [b2] [d2];
			if (gain > 0) {
				int below = b2nr, above = d2nr - 1;
				cont = 0;
				do {
					int help = path [below];
					path [below ++] = path [above];
					path [above --] = help;
				} while (below < above);
				*totalDistance -= gain;
			}
			d1 = d2;
			d2nr ++;
		}
		if (cont) { b1 = b2; b2nr ++; } else result = 1;
	}
	return result;
}

static int tryAdoption (int **distance, int *path, int numberOfCities, long *totalDistance)
{
	int *help = NUMivector (0, numberOfCities);
	int i, maximumGainLeft, result = 0;

	/* Compute maximum distance between two successive cities. */

	int city1 = path [0], city2 = path [1];
	int maximumDistance = distance [city1] [city2];
	for (i = 2; i <= numberOfCities; i ++) {
		city1 = city2;
		city2 = path [i];
		if (distance [city1] [city2] > maximumDistance)
			maximumDistance = distance [city1] [city2];
	}
	maximumGainLeft = maximumDistance;
	for (i = 1; i <= numberOfCities; i ++) {
		int cont = 1, b1, b2, distance_b1_b2, d1nr = 3, cc, e1nrMax = 6;
		int numberOfCitiesMinus1 = numberOfCities - 1, j;
		for (j = 0; j <= numberOfCitiesMinus1; j ++) path [j] = path [j + 1];
		path [numberOfCities] = path [0];
		b1 = path [0];
		b2 = path [1];
		distance_b1_b2 = distance [b1] [b2];
		cc = path [2];
		while (d1nr < numberOfCitiesMinus1 && cont) {
			int d1 = path [d1nr];
			int gain1 = distance_b1_b2 + distance [d1] [cc] - distance [d1] [b2];
			if (gain1 + maximumGainLeft > 0) {
				int e1nr = d1nr + 1;
				int dn = path [d1nr];
				if (e1nrMax > numberOfCitiesMinus1) e1nrMax = numberOfCitiesMinus1;
				while (e1nr < e1nrMax && cont) {
					int e1 = path [e1nr];
					int gain = gain1 + distance [dn] [e1] - distance [dn] [b1] - distance [cc] [e1];
					if (gain > 0) {
						int nAdoption = e1nr - d1nr;
						int dnnr = e1nr - 1;
						cont = 0;
						*totalDistance -= gain;
						for (j = 0; j <= dnnr - 1; j ++) help [j] = path [j + 1];
						for (j = 1; j <= nAdoption; j ++) path [j] = help [dnnr - j];
						for (j = 0; j <= d1nr - 2; j ++) path [nAdoption + j + 1] = help [j];
					}
					dn = e1;
					e1nr ++;
				}
			}
			e1nrMax ++;
			cc = d1;
			d1nr ++;
		}
		result |= ! cont;
	}
	NUMivector_free (help, 0);
	return result;
}

void Polygon_salesperson (I, long numberOfIterations)
{
	iam (Polygon);
	int numberOfCities, i, *path, *shortestPath, **distance;
	long numberOfShortest = 1, totalDistance, shortestDistance, iteration;
	Polygon help;

	numberOfCities = my numberOfPoints;
	if (! (distance = NUMimatrix (1, numberOfCities, 1, numberOfCities))) return;
	computeDistanceTable (me, distance);
	if (! (path = NUMivector (0, numberOfCities))) return;
	for (i = 1; i <= numberOfCities; i ++)
		path [i] = i;
	path [0] = numberOfCities;   /* Close path. */
	if (! (shortestPath = NUMivector_copy (path, 0, numberOfCities))) return;
	for (iteration = 1; iteration <= numberOfIterations; iteration ++)
	{
		if (iteration > 1) shuffle (path, numberOfCities);
		totalDistance = computeTotalDistance (distance, path, numberOfCities);
		if (iteration == 1) shortestDistance = totalDistance;
		do
		{
			do {} while (tryExchange (distance, path, numberOfCities, & totalDistance));
		}
			while (tryAdoption (distance, path, numberOfCities, & totalDistance));
		if (totalDistance < shortestDistance)   /* New shortest path. */
		{
			numberOfShortest = 1;
			for (i = 0; i <= numberOfCities; i ++) shortestPath [i] = path [i];
			shortestDistance = totalDistance;
		}
		else if (totalDistance == shortestDistance)   /* Shortest path confirmed. */
			numberOfShortest ++;
	}
	if (numberOfIterations > 1)
		Melder_casual ("Polygon_salesperson: "
			"found %ld times the same shortest path.", numberOfShortest);

	/* Change me: I will follow the shortest path found. */

	help = (structPolygon *)Data_copy (me);
	for (i = 1; i <= numberOfCities; i ++)
	{
		my x [i] = help -> x [shortestPath [i]];
		my y [i] = help -> y [shortestPath [i]];
	}
	forget (help);

	NUMimatrix_free (distance, 1, 1);
	NUMivector_free (path, 0);
	NUMivector_free (shortestPath, 0);
}

/* End of file Polygon.c */
