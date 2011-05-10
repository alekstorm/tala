#ifndef _ParamCurve_h_
#define _ParamCurve_h_
/* ParamCurve.h
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

/*
	Parametrized curve (x (t), y (t)):
		two functions (x and y) of one variable (the parameter t).
	Sampled parametrized curve (x [i], y [j]):
		x [i] = x (tx [i]) = x (tx1 + (i - 1) * dtx);
		y [i] = y (ty [i]) = y (ty1 + (i - 1) * dty);
*/

#include "Sound.h"

#ifdef __cplusplus
	extern "C" {
#endif

#define ParamCurve_members Function_members \
	Sound x, y;
#define ParamCurve_methods Function_methods
class_create (ParamCurve, Function);

int ParamCurve_init (I, Any x, Any y);

Any ParamCurve_create (Any x, Any y);
/*
	Return value:
		a newly created ParamCurve object,
		or NULL in case of failure.
	Failures:
		Out of memory.
		Domains do not overlap:
			x -> xmax <= y -> xmin || x -> xmin >= y -> xmax.
	Postconditions:
		(Result's domain is intersection of both domains:)
		result -> xmin = max (x -> xmin, y -> xmin);
		result -> xmax = min (x -> xmax, y -> xmax);
*/

void ParamCurve_swapXY (I);
/*
	Reflect around y = x.
	Postconditions:
		x == old y;
		y == old x;
*/

#ifdef __cplusplus
	}
#endif

/* End of file ParamCurve.h */
#endif

