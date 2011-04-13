/* PairDistribution.cpp
 *
 * Copyright (C) 1997-2011 Paul Boersma
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
 * pb 2004/09/03 PairDistribution_Distributions_getFractionCorrect
 * pb 2007/03/29 PairDistribution_to_Table
 * pb 2011/03/13 C++
 */

#include "PairDistribution.h"

#include "oo_DESTROY.h"
#include "PairDistribution_def.h"
#include "oo_COPY.h"
#include "PairDistribution_def.h"
#include "oo_EQUAL.h"
#include "PairDistribution_def.h"
#include "oo_CAN_WRITE_AS_ENCODING.h"
#include "PairDistribution_def.h"
#include "oo_WRITE_TEXT.h"
#include "PairDistribution_def.h"
#include "oo_READ_TEXT.h"
#include "PairDistribution_def.h"
#include "oo_WRITE_BINARY.h"
#include "PairDistribution_def.h"
#include "oo_READ_BINARY.h"
#include "PairDistribution_def.h"
#include "oo_DESCRIPTION.h"
#include "PairDistribution_def.h"

class_methods (PairProbability, Data) {
	class_method_local (PairProbability, destroy)
	class_method_local (PairProbability, copy)
	class_method_local (PairProbability, equal)
	class_method_local (PairProbability, canWriteAsEncoding)
	class_method_local (PairProbability, writeText)
	class_method_local (PairProbability, readText)
	class_method_local (PairProbability, writeBinary)
	class_method_local (PairProbability, readBinary)
	class_method_local (PairProbability, description)
	class_methods_end
}

static void info (I) {
	iam (PairDistribution);
	inherited (PairDistribution) info (me);
	MelderInfo_writeLine2 (L"Number of pairs: ", Melder_integer (my pairs -> size));
}

class_methods (PairDistribution, Data) {
	class_method_local (PairDistribution, destroy)
	class_method_local (PairDistribution, copy)
	class_method_local (PairDistribution, equal)
	class_method_local (PairDistribution, canWriteAsEncoding)
	class_method_local (PairDistribution, writeText)
	class_method_local (PairDistribution, readText)
	class_method_local (PairDistribution, writeBinary)
	class_method_local (PairDistribution, readBinary)
	class_method_local (PairDistribution, description)
	class_method (info)
	class_methods_end
}

PairProbability PairProbability_create (const wchar_t *string1, const wchar_t *string2, double weight) {
	try {
		autoPairProbability me = Thing_new (PairProbability); therror
		my string1 = Melder_wcsdup_e (string1); therror
		my string2 = Melder_wcsdup_e (string2); therror
		my weight = weight;
		return me.transfer();
	} catch (...) {
		rethrowzero;
	}
}

PairDistribution PairDistribution_create () {
	try {
		autoPairDistribution me = Thing_new (PairDistribution); therror
		my pairs = Ordered_create (); therror
		return me.transfer();
	} catch (...) {
		rethrowzero;
	}
}

static void PairDistribution_checkSpecifiedPairNumber (PairDistribution me, long pairNumber) {
	if (pairNumber < 1)
		Melder_throw (me, ": the specified pair number is ", pairNumber, ", but should be at least 1.");
	if (pairNumber > my pairs -> size)
		Melder_throw (me, ": the specified pair number is ", pairNumber, ", but should be at most my number of pairs (", my pairs -> size, ").");	
}

const wchar * PairDistribution_getString1 (PairDistribution me, long pairNumber) {
	try {
		PairDistribution_checkSpecifiedPairNumber (me, pairNumber);
		PairProbability prob = static_cast <PairProbability> (my pairs -> item [pairNumber]);
		return prob -> string1;
	} catch (...) {
		rethrowmzero (me, ": string1 not retrieved.");
	}
}

const wchar * PairDistribution_getString2 (PairDistribution me, long pairNumber) {
	try {
		PairDistribution_checkSpecifiedPairNumber (me, pairNumber);
		PairProbability prob = static_cast <PairProbability> (my pairs -> item [pairNumber]);
		return prob -> string2;
	} catch (...) {
		rethrowmzero (me, ": string2 not retrieved.");
	}
}

double PairDistribution_getWeight (PairDistribution me, long pairNumber) {
	try {
		PairDistribution_checkSpecifiedPairNumber (me, pairNumber);
		PairProbability prob = static_cast <PairProbability> (my pairs -> item [pairNumber]);
		return prob -> weight;
	} catch (...) {
		rethrowmzero (me, ": weight not retrieved.");
	}
}

void PairDistribution_add (PairDistribution me, const wchar_t *string1, const wchar_t *string2, double weight) {
	try {
		PairProbability pair = PairProbability_create (string1, string2, weight); therror
		Collection_addItem (my pairs, pair); therror
	} catch (...) {
		rethrow;
	}
}

void PairDistribution_removeZeroWeights (PairDistribution me) {
	for (long ipair = my pairs -> size; ipair > 0; ipair --) {
		PairProbability prob = static_cast <PairProbability> (my pairs -> item [ipair]);
		if (prob -> weight <= 0.0) {
			Collection_removeItem (my pairs, ipair);
		}
	}
}

static double PairDistributions_getTotalWeight_checkPositive (PairDistribution me) throw (int) {
	double totalWeight = 0.0;
	for (long ipair = 1; ipair <= my pairs -> size; ipair ++) {
		PairProbability prob = static_cast <PairProbability> (my pairs -> item [ipair]);
		totalWeight += prob -> weight;
	}
	if (totalWeight <= 0.0) {
		Melder_throw (me, ": the total probability weight is ", Melder_half (totalWeight), " but should be greater than zero for this operation.");
	}
	return totalWeight;
}

void PairDistribution_to_Stringses (PairDistribution me, long nout, Strings *strings1_out, Strings *strings2_out) {
	try {
		*strings1_out = *strings2_out = NULL;
		long nin = my pairs -> size, iin;
		if (nin < 1)
			Melder_throw ("No candidates.");
		if (nout < 1)
			Melder_throw (L"Number of generated string pairs should be positive.");
		double total = PairDistributions_getTotalWeight_checkPositive (me);
		autoStrings strings1 = Thing_new (Strings);
		strings1 -> numberOfStrings = nout;
		strings1 -> strings = (wchar_t **) NUMpvector (1, nout); therror
		autoStrings strings2 = Thing_new (Strings);
		strings2 -> numberOfStrings = nout;
		strings2 -> strings = (wchar_t **) NUMpvector (1, nout); therror
		for (long iout = 1; iout <= nout; iout ++) {
			do {
				double rand = NUMrandomUniform (0, total), sum = 0.0;
				for (iin = 1; iin <= nin; iin ++) {
					PairProbability prob = static_cast <PairProbability> (my pairs -> item [iin]);
					sum += prob -> weight;
					if (rand <= sum) break;
				}
			} while (iin > nin);   /* Guard against rounding errors. */
			PairProbability prob = static_cast <PairProbability> (my pairs -> item [iin]);
			if (! prob -> string1 || ! prob -> string2)
				Melder_throw ("No string in probability pair ", iin, ".");
			strings1 -> strings [iout] = Melder_wcsdup_e (prob -> string1); therror
			strings2 -> strings [iout] = Melder_wcsdup_e (prob -> string2); therror
		}
		*strings1_out = strings1.transfer();
		*strings2_out = strings2.transfer();
	} catch (...) {
		rethrowm (me, ": generation of Stringses not performed.");
	}
}

void PairDistribution_peekPair (PairDistribution me, wchar_t **string1, wchar_t **string2) {
	try {
		*string1 = *string2 = NULL;
		double total = 0.0;
		long nin = my pairs -> size, iin;
		PairProbability prob;
		if (nin < 1) Melder_throw ("No candidates.");
		for (iin = 1; iin <= nin; iin ++) {
			prob = static_cast <PairProbability> (my pairs -> item [iin]);
			total += prob -> weight;
		}
		do {
			double rand = NUMrandomUniform (0, total), sum = 0.0;
			for (iin = 1; iin <= nin; iin ++) {
				prob = static_cast <PairProbability> (my pairs -> item [iin]);
				sum += prob -> weight;
				if (rand <= sum) break;
			}
		} while (iin > nin);   /* Guard against rounding errors. */
		prob = static_cast <PairProbability> (my pairs -> item [iin]);
		if (! prob -> string1 || ! prob -> string2) Melder_throw ("No string in probability pair ", iin, L".");
		*string1 = prob -> string1;
		*string2 = prob -> string2;
	} catch (...) {
		rethrowm (me, ": pair not peeked.");
	}
}

static int compare (PairProbability me, PairProbability thee) throw () {
	return wcscmp (my string1, thy string1);
}

static double PairDistribution_getFractionCorrect (PairDistribution me, int which) {
	try {
		double correct = 0.0;
		long pairmin = 1, ipair;
		autoPairDistribution thee = static_cast <PairDistribution> (Data_copy (me));
		NUMsort_p (thy pairs -> size, thy pairs -> item, (int (*) (const void *, const void *)) compare);
		double total = PairDistributions_getTotalWeight_checkPositive (thee.peek());
		do {
			long pairmax = pairmin;
			wchar_t *firstInput = ((PairProbability) thy pairs -> item [pairmin]) -> string1;
			for (ipair = pairmin + 1; ipair <= thy pairs -> size; ipair ++) {
				PairProbability prob = static_cast <PairProbability> (thy pairs -> item [ipair]);
				if (! wcsequ (prob -> string1, firstInput)) {
					pairmax = ipair - 1;
					break;
				}
			}
			if (ipair > thy pairs -> size) pairmax = thy pairs -> size;
			if (which == 0) {
				double pmax = 0.0;
				for (ipair = pairmin; ipair <= pairmax; ipair ++) {
					PairProbability prob = static_cast <PairProbability> (thy pairs -> item [ipair]);
					double p = prob -> weight / total;
					if (p > pmax) pmax = p;
				}
				correct += pmax;
			} else {
				double sum = 0.0, p2 = 0.0;
				for (ipair = pairmin; ipair <= pairmax; ipair ++) {
					PairProbability prob = static_cast <PairProbability> (thy pairs -> item [ipair]);
					double p = prob -> weight / total;
					sum += p;
					p2 += p * p;
				}
				correct += p2 / sum;
			}
			pairmin = pairmax + 1;
		} while (pairmin <= thy pairs -> size);
		return correct;
	} catch (...) {
		rethrowmzero (me, ": could not compute my fraction correct.");
	}
}

double PairDistribution_getFractionCorrect_maximumLikelihood (PairDistribution me) {
	return PairDistribution_getFractionCorrect (me, 0);
}

double PairDistribution_getFractionCorrect_probabilityMatching (PairDistribution me) {
	return PairDistribution_getFractionCorrect (me, 1);
}

double PairDistribution_Distributions_getFractionCorrect (PairDistribution me, Distributions dist, long column) {
	try {
		double correct = 0.0;
		long pairmin = 1;
		wchar_t string [1000];
		Distributions_checkSpecifiedColumnNumberWithinRange (dist, column);
		autoPairDistribution thee = static_cast <PairDistribution> (Data_copy (me)); therror
		NUMsort_p (thy pairs -> size, thy pairs -> item, (int (*) (const void *, const void *)) compare);
		double total = PairDistributions_getTotalWeight_checkPositive (thee.peek());
		do {
			long pairmax = pairmin, length, ipair;
			double sum = 0.0, sumDist = 0.0;
			wchar_t *firstInput = ((PairProbability) thy pairs -> item [pairmin]) -> string1;
			for (ipair = pairmin + 1; ipair <= thy pairs -> size; ipair ++) {
				PairProbability prob = static_cast <PairProbability> (thy pairs -> item [ipair]);
				if (! wcsequ (prob -> string1, firstInput)) {
					pairmax = ipair - 1;
					break;
				}
			}
			if (ipair > thy pairs -> size) pairmax = thy pairs -> size;
			for (ipair = pairmin; ipair <= pairmax; ipair ++) {
				PairProbability prob = static_cast <PairProbability> (thy pairs -> item [ipair]);
				double p = prob -> weight / total, pout = 0.0;
				swprintf (string, 1000, L"%ls \\-> %ls", prob -> string1, prob -> string2);
				for (long idist = 1; idist <= dist -> numberOfRows; idist ++) {
					if (wcsequ (string, dist -> rowLabels [idist])) {
						pout = dist -> data [idist] [column];
						break;
					}
				}
				sum += p * pout;
			}
			swprintf (string, 1000, L"%ls \\-> ", firstInput);
			length = wcslen (string);
			for (long idist = 1; idist <= dist -> numberOfRows; idist ++) {
				if (wcsnequ (string, dist -> rowLabels [idist], length)) {
					sumDist += dist -> data [idist] [column];
				}
			}
			if (sumDist != 0.0) correct += sum / sumDist;
			pairmin = pairmax + 1;
		} while (pairmin <= thy pairs -> size);
		return correct;
	} catch (...) {
		rethrowmzero (me, " & ", dist, ": could not compute our fraction correct.");
	}
}

Table PairDistribution_to_Table (PairDistribution me) {
	try {
		autoTable thee = Table_createWithColumnNames (my pairs -> size, L"string1 string2 weight");
		for (long ipair = 1; ipair <= my pairs -> size; ipair ++) {
			PairProbability prob = static_cast <PairProbability> (my pairs -> item [ipair]);
			Table_setStringValue (thee.peek(), ipair, 1, prob -> string1); therror
			Table_setStringValue (thee.peek(), ipair, 2, prob -> string2); therror
			Table_setNumericValue (thee.peek(), ipair, 3, prob -> weight); therror
		}
		return thee.transfer();
	} catch (...) {
		rethrowzero;
	}
}

/* End of file PairDistribution.c */
