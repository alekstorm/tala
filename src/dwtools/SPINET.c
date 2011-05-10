/* SPINET.c
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
 djmw 20061212 Changed info to Melder_writeLine<x> format.
 djmw 20071012 Added: o_CAN_WRITE_AS_ENCODING.h
 djmw 20080122 float -> double
  djmw 20110304 Thing_new
*/

#include "SPINET.h"
#include "Sound_extensions.h"
#include "num/NUM2.h"

#include "sys/oo/oo_DESTROY.h"
#include "SPINET_def.h"
#include "sys/oo/oo_COPY.h"
#include "SPINET_def.h"
#include "sys/oo/oo_EQUAL.h"
#include "SPINET_def.h"
#include "sys/oo/oo_CAN_WRITE_AS_ENCODING.h"
#include "SPINET_def.h"
#include "sys/oo/oo_WRITE_TEXT.h"
#include "SPINET_def.h"
#include "sys/oo/oo_WRITE_BINARY.h"
#include "SPINET_def.h"
#include "sys/oo/oo_READ_TEXT.h"
#include "SPINET_def.h"
#include "sys/oo/oo_READ_BINARY.h"
#include "SPINET_def.h"
#include "sys/oo/oo_DESCRIPTION.h"
#include "SPINET_def.h"

static void info (I)
{
	iam (SPINET); double miny, maxy, mins, maxs;
	
	classData -> info (me);
 	if (! Sampled2_getWindowExtrema_d (me, my y, 1, my nx, 1, my ny, &miny, &maxy) ||
 		! Sampled2_getWindowExtrema_d (me, my s, 1, my nx, 1, my ny, &mins, &maxs)) return;
	MelderInfo_writeLine2 (L"Minimum power: ", Melder_double (miny));
	MelderInfo_writeLine2 (L"Maximum power: ", Melder_double (maxy));
	MelderInfo_writeLine2 (L"Minimum power rectified: ", Melder_double (mins));
	MelderInfo_writeLine2 (L"Maximum powerrectified: ", Melder_double (maxs));
}

class_methods (SPINET, Sampled2)
	class_method_local (SPINET, destroy)
	class_method_local (SPINET, equal)
	class_method_local (SPINET, canWriteAsEncoding)
	class_method_local (SPINET, copy)
	class_method_local (SPINET, readText)
	class_method_local (SPINET, readBinary)
	class_method_local (SPINET, writeText)
	class_method_local (SPINET, writeBinary)
	class_method_local (SPINET, description)
	class_method (info)
class_methods_end


SPINET SPINET_create (double tmin, double tmax, long nt, double dt, double t1,
	 double minimumFrequency, double maximumFrequency, long nFilters,
	 double excitationErbProportion, double inhibitionErbProportion)
{
	SPINET me = Thing_new (SPINET);
	double minErb = NUMhertzToErb (minimumFrequency);
	double maxErb = NUMhertzToErb (maximumFrequency);
	double dErb = (maxErb - minErb) / nFilters;
	if (! me || ! Sampled2_init (me, tmin, tmax, nt, dt, t1,
			minErb - dErb / 2, maxErb + dErb / 2, nFilters, dErb, minErb) ||
		! (my y = NUMdmatrix (1, nFilters, 1, nt)) ||
		! (my s = NUMdmatrix (1, nFilters, 1, nt))) { forget (me); return NULL; }
	my gamma = 4;
	my excitationErbProportion = excitationErbProportion;
	my inhibitionErbProportion = inhibitionErbProportion;
	return me;
}

/* End of file SPINET.c */
