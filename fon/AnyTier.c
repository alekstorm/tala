/* AnyTier.c
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
 * pb 2002/07/16 GPL
 * pb 2007/10/01 can write as encoding
 * pb 2008/03/27 binary rather than linear searches
 * pb 2008/03/31 corrected binary search (case n=1)
 * pb 2008/09/20 shiftX
 * pb 2008/09/23 scaleX
 */

#include "AnyTier.h"

#include "oo_DESTROY.h"
#include "AnyTier_def.h"
#include "oo_COPY.h"
#include "AnyTier_def.h"
#include "oo_EQUAL.h"
#include "AnyTier_def.h"
#include "oo_CAN_WRITE_AS_ENCODING.h"
#include "AnyTier_def.h"
#include "oo_WRITE_TEXT.h"
#include "AnyTier_def.h"
#include "oo_READ_TEXT.h"
#include "AnyTier_def.h"
#include "oo_WRITE_BINARY.h"
#include "AnyTier_def.h"
#include "oo_READ_BINARY.h"
#include "AnyTier_def.h"
#include "oo_DESCRIPTION.h"
#include "AnyTier_def.h"

class_methods (AnyPoint, Data) {
	class_method_local (AnyPoint, destroy)
	class_method_local (AnyPoint, copy)
	class_method_local (AnyPoint, equal)
	class_method_local (AnyPoint, canWriteAsEncoding)
	class_method_local (AnyPoint, writeText)
	class_method_local (AnyPoint, readText)
	class_method_local (AnyPoint, writeBinary)
	class_method_local (AnyPoint, readBinary)
	class_method_local (AnyPoint, description)
	class_methods_end
}

static void shiftX (I, double xfrom, double xto) {
	iam (AnyTier);
	inherited (AnyTier) shiftX (me, xfrom, xto);
	for (long i = 1; i <= my points -> size; i ++) {
		AnyPoint point = my points -> item [i];
		NUMshift (& point -> time, xfrom, xto);
	}
}

static void scaleX (I, double xminfrom, double xmaxfrom, double xminto, double xmaxto) {
	iam (AnyTier);
	inherited (AnyTier) scaleX (me, xminfrom, xmaxfrom, xminto, xmaxto);
	for (long i = 1; i <= my points -> size; i ++) {
		AnyPoint point = my points -> item [i];
		NUMscale (& point -> time, xminfrom, xmaxfrom, xminto, xmaxto);
	}
}

class_methods (AnyTier, Function) {
	class_method_local (AnyTier, destroy)
	class_method_local (AnyTier, copy)
	class_method_local (AnyTier, equal)
	class_method_local (AnyTier, canWriteAsEncoding)
	class_method_local (AnyTier, writeText)
	class_method_local (AnyTier, readText)
	class_method_local (AnyTier, writeBinary)
	class_method_local (AnyTier, readBinary)
	class_method_local (AnyTier, description)
	class_method (shiftX)
	class_method (scaleX)
	class_methods_end
}

long AnyTier_timeToLowIndex (I, double time) {
	iam (AnyTier);
	if (my points -> size == 0) return 0;   // undefined
	long ileft = 1, iright = my points -> size;
	AnyPoint *points = (AnyPoint *) my points -> item;
	double tleft = points [ileft] -> time;
	if (time < tleft) return 0;   // offleft
	double tright = points [iright] -> time;
	if (time >= tright) return iright;
	Melder_assert (time >= tleft && time < tright);
	Melder_assert (iright > ileft);
	while (iright > ileft + 1) {
		long imid = (ileft + iright) / 2;
		double tmid = points [imid] -> time;
		if (time < tmid) {
			iright = imid;
			tright = tmid;
		} else {
			ileft = imid;
			tleft = tmid;
		}
	}
	Melder_assert (iright == ileft + 1);
	Melder_assert (ileft >= 1);
	Melder_assert (iright <= my points -> size);
	Melder_assert (time >= points [ileft] -> time);
	Melder_assert (time <= points [iright] -> time);
	return ileft;
}

long AnyTier_timeToHighIndex (I, double time) {
	iam (AnyTier);
	if (my points -> size == 0) return 0;   // undefined; is this right?
	long ileft = 1, iright = my points -> size;
	AnyPoint *points = (AnyPoint *) my points -> item;
	double tleft = points [ileft] -> time;
	if (time <= tleft) return 1;
	double tright = points [iright] -> time;
	if (time > tright) return iright + 1;   // offright
	Melder_assert (time > tleft && time <= tright);
	Melder_assert (iright > ileft);
	while (iright > ileft + 1) {
		long imid = (ileft + iright) / 2;
		double tmid = points [imid] -> time;
		if (time <= tmid) {
			iright = imid;
			tright = tmid;
		} else {
			ileft = imid;
			tleft = tmid;
		}
	}
	Melder_assert (iright == ileft + 1);
	Melder_assert (ileft >= 1);
	Melder_assert (iright <= my points -> size);
	Melder_assert (time >= points [ileft] -> time);
	Melder_assert (time <= points [iright] -> time);
	return iright;
}

long AnyTier_getWindowPoints (I, double tmin, double tmax, long *imin, long *imax) {
	iam (AnyTier);
	if (my points -> size == 0) return 0;
	*imin = AnyTier_timeToHighIndex (me, tmin);
	*imax = AnyTier_timeToLowIndex (me, tmax);
	if (*imax < *imin) return 0;
	return *imax - *imin + 1;
}
	
long AnyTier_timeToNearestIndex (I, double time) {
	iam (AnyTier);
	if (my points -> size == 0) return 0;   // undefined
	long ileft = 1, iright = my points -> size;
	AnyPoint *points = (AnyPoint *) my points -> item;
	double tleft = points [ileft] -> time;
	if (time <= tleft) return 1;
	double tright = points [iright] -> time;
	if (time >= tright) return iright;
	Melder_assert (time > tleft && time < tright);
	Melder_assert (iright > ileft);
	while (iright > ileft + 1) {
		long imid = (ileft + iright) / 2;
		double tmid = points [imid] -> time;
		if (time < tmid) {
			iright = imid;
			tright = tmid;
		} else {
			ileft = imid;
			tleft = tmid;
		}
	}
	Melder_assert (iright == ileft + 1);
	Melder_assert (ileft >= 1);
	Melder_assert (iright <= my points -> size);
	Melder_assert (time >= points [ileft] -> time);
	Melder_assert (time <= points [iright] -> time);
	return time - tleft <= tright - time ? ileft : iright;
}

long AnyTier_hasPoint (I, double t) {
	iam (AnyTier);
	if (my points -> size == 0) return 0;   // point not found
	long ileft = 1, iright = my points -> size;
	AnyPoint *points = (AnyPoint *) my points -> item;
	double tleft = points [ileft] -> time;
	if (t < tleft) return 0;   // offleft
	double tright = points [iright] -> time;
	if (t > tright) return 0;   // offright
	if (t == tleft) return 1;
	if (t == tright) return iright;
	Melder_assert (t > tleft && t < tright);
	Melder_assert (iright > ileft);
	while (iright > ileft + 1) {
		long imid = (ileft + iright) / 2;
		double tmid = points [imid] -> time;
		if (t < tmid) {
			iright = imid;
			tright = tmid;
		} else if (t == tmid) {
			return imid;   // point found
		} else {
			ileft = imid;
			tleft = tmid;
		}
	}
	Melder_assert (iright == ileft + 1);
	Melder_assert (ileft >= 1);
	Melder_assert (iright <= my points -> size);
	Melder_assert (t > points [ileft] -> time);
	Melder_assert (t < points [iright] -> time);
	return 0;   /* Point not found. */
}

int AnyTier_addPoint (I, Any point) {
	iam (AnyTier);
	if (! point || ! Collection_addItem (my points, point)) return 0;
	return 1;
}

void AnyTier_removePoint (I, long i) {
	iam (AnyTier);
	if (i >= 1 && i <= my points -> size) Collection_removeItem (my points, i);
}

void AnyTier_removePointNear (I, double time) {
	iam (AnyTier);
	long ipoint = AnyTier_timeToNearestIndex (me, time);
	if (ipoint) Collection_removeItem (my points, ipoint);
}

void AnyTier_removePointsBetween (I, double tmin, double tmax) {
	iam (AnyTier);
	if (my points -> size == 0) return;
	long ileft = AnyTier_timeToHighIndex (me, tmin);
	long iright = AnyTier_timeToLowIndex (me, tmax);
	for (long i = iright; i >= ileft; i --)
		Collection_removeItem (my points, i);
}

PointProcess AnyTier_downto_PointProcess (I) {
	iam (AnyTier);
	long numberOfPoints = my points -> size;
	AnyPoint *points = (AnyPoint *) my points -> item;
	PointProcess thee = PointProcess_create (my xmin, my xmax, numberOfPoints);
	if (! thee) return NULL;
	/* OPTIMIZATION, bypassing PointProcess_addTime: */
	for (long i = 1; i <= numberOfPoints; i ++)
		thy t [i] = points [i] -> time;
	thy nt = numberOfPoints;
	return thee;
}

/* End of file AnyTier.c */
