/* ParamCurve.c
 *
 * Copyright (C) 1992-2008 Paul Boersma
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
 * pb 1997/04/27
 * pb 2002/07/16 GPL
 * pb 2008/01/19 double
 */

#include "ParamCurve.h"

static void destroy (I) {
	iam (ParamCurve);
	forget (my x);
	forget (my y);
	inherited (ParamCurve) destroy (me);
}

static void info (I) {
	iam (ParamCurve);
	long i;
	double xmin = 1e300, xmax = -1e300, ymin = 1e300, ymax = -1e300;
	for (i = 1; i <= my x -> nx; i ++) {
		double value = my x -> z [1] [i];
		if (value < xmin) xmin = value;
		if (value > xmax) xmax = value;
	}
	for (i = 1; i <= my y -> nx; i ++) {
		double value = my y -> z [1] [i];
		if (value < ymin) ymin = value;
		if (value > ymax) ymax = value;
	}
	classData -> info (me);
	MelderInfo_writeLine1 (L"Domain:");
	MelderInfo_writeLine2 (L"   tmin: ", Melder_double (my xmin));
	MelderInfo_writeLine2 (L"   tmax: ", Melder_double (my xmax));
	MelderInfo_writeLine1 (L"x sampling:");
	MelderInfo_writeLine2 (L"   Number of values of t in x: ", Melder_double (my x -> nx));
	MelderInfo_writeLine5 (L"   t step in x: ", Melder_double (my x -> dx), L" (sampling rate ", Melder_double (1.0 / my x -> dx), L")");
	MelderInfo_writeLine2 (L"   First t in x: ", Melder_double (my x -> x1));
	MelderInfo_writeLine1 (L"x values:");
	MelderInfo_writeLine2 (L"   Minimum x: ", Melder_double (xmin));
	MelderInfo_writeLine2 (L"   Maximum x: ", Melder_double (xmax));
	MelderInfo_writeLine1 (L"y sampling:");
	MelderInfo_writeLine2 (L"   Number of values of t in y: ", Melder_double (my y -> nx));
	MelderInfo_writeLine5 (L"   t step in y: ", Melder_double (my y -> dx), L" (sampling rate ", Melder_double (1.0 / my y -> dx), L")");
	MelderInfo_writeLine2 (L"   First t in y: ", Melder_double (my y -> x1));
	MelderInfo_writeLine1 (L"y values:");
	MelderInfo_writeLine2 (L"   Minimum y: ", Melder_double (ymin));
	MelderInfo_writeLine2 (L"   Maximum y: ", Melder_double (ymax));
}

static int copy (I, thou) {
	iam (ParamCurve); thouart (ParamCurve);
	thy x = NULL;
	thy y = NULL;
	if (! inherited (ParamCurve) copy (me, thee) ||
		! (thy x = (structSound *)Data_copy (my x)) ||
		! (thy y = (structSound *)Data_copy (my y)))
		return 0;
	return 1;
}

static bool equal (I, thou) {
	iam (ParamCurve); thouart (ParamCurve);
	return inherited (ParamCurve) equal (me, thee) && Data_equal (my x, thy x) && Data_equal (my y, thy y);
}

static int writeText (I, MelderFile file) {
	iam (ParamCurve);
	return Data_writeText (my x, file) && Data_writeText (my y, file);
}

static int readText (I, MelderReadText text) {
	iam (ParamCurve);
	return (my x = Thing_new (Sound)) != NULL && (my y = Thing_new (Sound)) != NULL &&
		Data_readText (my x, text) && Data_readText (my y, text);
}

static int writeBinary (I, FILE *f) {
	iam (ParamCurve);
	return Data_writeBinary (my x, f) && Data_writeBinary (my y, f);
}

static int readBinary (I, FILE *f) {
	iam (ParamCurve);
	return (my x = Thing_new (Sound)) != NULL && (my y = Thing_new (Sound)) != NULL &&
		Data_readBinary (my x, f) && Data_readBinary (my y, f);
}

class_methods (ParamCurve, Function)
	class_method (destroy)
	class_method (info)
	class_method (copy)
	class_method (equal)
	class_method (writeText)
	class_method (readText)
	class_method (writeBinary)
	class_method (readBinary)
class_methods_end

int ParamCurve_init (I, Any void_x, Any void_y) {
	iam (ParamCurve);
	Sound x = (structSound *)void_x, y = (structSound *)void_y;
	if (x -> xmax <= y -> xmin || x -> xmin >= y -> xmax)
		return Melder_error1 (L"Domains do not overlap.");
	if (! (my x = (structSound *)Data_copy (x)) || ! (my y = (structSound *)Data_copy (y)))
		return 0;
	my xmin = x -> xmin > y -> xmin ? x -> xmin : y -> xmin;
	my xmax = x -> xmax < y -> xmax ? x -> xmax : y -> xmax; 
	return 1;
}

Any ParamCurve_create (Any x, Any y) {
	ParamCurve me = Thing_new (ParamCurve);
	if (! me || ! ParamCurve_init (me, x, y)) { forget (me); return (structSound *)Melder_errorp ("ParamCurve not created."); }
	return me;
}

void ParamCurve_swapXY (I) {
	iam (ParamCurve);
	Sound help = my x;
	my x = my y;
	my y = help;
}

/* End of file ParamCurve.c */
