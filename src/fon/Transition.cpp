/* Transition.c
 *
 * Copyright (C) 1997-2007 Paul Boersma
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
 * pb 2003/06/19 Eigen
 * pb 2006/12/10 MelderInfo
 * pb 2007/08/12 wchar_t
 * pb 2007/10/01 can write as encoding
 */

#include "Transition.h"
#include "num/NUM2.h"
#include "dwsys/Eigen.h"

#include "sys/oo/oo_DESTROY.h"
#include "Transition_def.h"
#include "sys/oo/oo_COPY.h"
#include "Transition_def.h"
#include "sys/oo/oo_EQUAL.h"
#include "Transition_def.h"
#include "sys/oo/oo_CAN_WRITE_AS_ENCODING.h"
#include "Transition_def.h"
#include "sys/oo/oo_WRITE_BINARY.h"
#include "Transition_def.h"
#include "sys/oo/oo_READ_TEXT.h"
#include "Transition_def.h"
#include "sys/oo/oo_READ_BINARY.h"
#include "Transition_def.h"
#include "sys/oo/oo_DESCRIPTION.h"
#include "Transition_def.h"

static int writeText (I, MelderFile file) {
	iam (Transition);
	texputi4 (file, my numberOfStates, L"numberOfStates", 0,0,0,0,0);
	MelderFile_write1 (file, L"\nstateLabels []: ");
	if (my numberOfStates < 1) MelderFile_write1 (file, L"(empty)");
	MelderFile_write1 (file, L"\n");
	for (long i = 1; i <= my numberOfStates; i ++) {
		MelderFile_write1 (file, L"\"");
		if (my stateLabels [i] != NULL) MelderFile_write1 (file, my stateLabels [i]);
		MelderFile_write1 (file, L"\"\t");
	}
	for (long i = 1; i <= my numberOfStates; i ++) {
		MelderFile_write3 (file, L"\nstate [", Melder_integer (i), L"]:");
		for (long j = 1; j <= my numberOfStates; j ++) {
			MelderFile_write2 (file, L"\t", Melder_double (my data [i] [j]));
		}
	}
	return 1;
}

static void info (I) {
	iam (Transition);
	classData -> info (me);
	MelderInfo_writeLine2 (L"Number of states: ", Melder_integer (my numberOfStates));
}

class_methods (Transition, Data) {
	class_method_local (Transition, destroy)
	class_method_local (Transition, description)
	class_method_local (Transition, copy)
	class_method_local (Transition, equal)
	class_method_local (Transition, canWriteAsEncoding)
	class_method (writeText)
	class_method_local (Transition, readText)
	class_method_local (Transition, writeBinary)
	class_method_local (Transition, readBinary)
	class_method (info)
	class_methods_end
}

int Transition_init (I, long numberOfStates) {
	iam (Transition);
	if (numberOfStates < 1)
		return Melder_error1 (L"(Transition_init:) Cannot create empty matrix.");
	my numberOfStates = numberOfStates;
	if (! (my stateLabels = (wchar_t **)NUMvector (sizeof (char *), 1, numberOfStates))) return 0;
	if (! (my data = NUMdmatrix (1, my numberOfStates, 1, my numberOfStates))) return 0;
	return 1;
}

Transition Transition_create (long numberOfStates) {
	Transition me = Thing_new (Transition);
	if (! me || ! Transition_init (me, numberOfStates)) forget (me);
	return me;
}

static void Transition_transpose (Transition me) {
	long i, j;
	for (i = 1; i < my numberOfStates; i ++) {
		for (j = i + 1; j <= my numberOfStates; j ++) {
			double temp = my data [i] [j];
			my data [i] [j] = my data [j] [i];
			my data [j] [i] = temp;
		}
	}
}

int Transition_eigen (Transition me, Matrix *eigenvectors, Matrix *eigenvalues) {
	long i, j;
	Eigen eigen = Thing_new (Eigen);
	*eigenvectors = NULL, *eigenvalues = NULL;
	Transition_transpose (me);
	if (! Eigen_initFromSymmetricMatrix (eigen, my data, my numberOfStates)) {
		Transition_transpose (me);
		goto end;
	}
	Transition_transpose (me);
	*eigenvectors = Matrix_createSimple (my numberOfStates, my numberOfStates);
	*eigenvalues = Matrix_createSimple (my numberOfStates, 1);
	for (i = 1; i <= my numberOfStates; i ++) {
		(*eigenvalues) -> z [i] [1] = eigen -> eigenvalues [i];
		for (j = 1; j <= my numberOfStates; j ++)
			(*eigenvectors) -> z [i] [j] = eigen -> eigenvectors [j] [i];
	}
end:
	forget (eigen);
	if (Melder_hasError ()) {
		_Thing_forget ((Thing *) eigenvectors); *eigenvectors = NULL;
		_Thing_forget ((Thing *) eigenvalues); *eigenvalues = NULL;
		return 0;
	}
	return 1;
}

Transition Transition_power (Transition me, long power) {
	long ipow;
	Transition thee = NULL, him = NULL;
	thee = (structTransition *)Data_copy (me);
	him = (structTransition *)Data_copy (me);
	if (! thee || ! him) goto end;
	for (ipow = 2; ipow <= power; ipow ++) {
		long irow, icol;
		Transition tmp;
		tmp = him; him = thee; thee = tmp;
		for (irow = 1; irow <= my numberOfStates; irow ++) for (icol = 1; icol <= my numberOfStates; icol ++) {
			long i;
			thy data [irow] [icol] = 0.0;
			for (i = 1; i <= my numberOfStates; i ++)
				thy data [irow] [icol] += his data [irow] [i] * my data [i] [icol];
		}
	}
end:
	forget (him);
	if (Melder_hasError ()) forget (thee);
	return thee;
}

Matrix Transition_to_Matrix (Transition me) {
	Matrix thee = Matrix_createSimple (my numberOfStates, my numberOfStates);
	long i, j;
	if (! thee) return NULL;
	for (i = 1; i <= my numberOfStates; i ++) for (j = 1; j <= my numberOfStates; j ++)
		thy z [i] [j] = my data [i] [j];
	return thee;
}

Transition Matrix_to_Transition (Matrix me) {
	Transition thee = NULL;
	long i, j;

	/*
	 * Preconditions: matrix matching.
	 */
	if (my nx != my ny)
		error1 (L"Matrix should be square.")

	if (! (thee = Transition_create (my nx))) goto end;
	for (i = 1; i <= my nx; i ++) for (j = 1; j <= my nx; j ++)
		thy data [i] [j] = my z [i] [j];

end:
	if (Melder_hasError ()) {
		forget (thee);
		return (structTransition *)Melder_errorp ("(Matrix_to_Transition:) Not performed.");
	}
	return thee;
}

/* End of file Transition.c */
