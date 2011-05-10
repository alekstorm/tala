/* OTMulti.c
 *
 * Copyright (C) 2005-2011 Paul Boersma
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
 * pb 2005/06/11 the very beginning of computational bidirectional multi-level OT
 * pb 2006/05/16 guarded against cells with many violations
 * pb 2006/05/17 draw disharmonies above tableau
 * pb 2007/05/19 decision strategies
 * pb 2007/08/12 wchar_t
 * pb 2007/10/01 leak and constraint plasticity
 * pb 2007/10/01 can write as encoding
 * pb 2007/11/14 drawTableau: corrected direction of arrows for positive satisfactions
 * pb 2008/04/14 OTMulti_getConstraintIndexFromName
 * pb 2009/03/18 modern enums
 * pb 2010/06/05 corrected colours
 * pb 2011/03/01 multiple update rules; more decision strategies
 */

#include "OTMulti.h"

#include "sys/oo/oo_DESTROY.h"
#include "OTMulti_def.h"
#include "sys/oo/oo_COPY.h"
#include "OTMulti_def.h"
#include "sys/oo/oo_EQUAL.h"
#include "OTMulti_def.h"
#include "sys/oo/oo_CAN_WRITE_AS_ENCODING.h"
#include "OTMulti_def.h"
#include "sys/oo/oo_WRITE_BINARY.h"
#include "OTMulti_def.h"
#include "sys/oo/oo_READ_BINARY.h"
#include "OTMulti_def.h"
#include "sys/oo/oo_DESCRIPTION.h"
#include "OTMulti_def.h"

static void classOTMulti_info (I)
{
	iam (OTMulti);
	classData -> info (me);
	long numberOfViolations = 0;
	for (long icand = 1; icand <= my numberOfCandidates; icand ++)
	{
		for (long icons = 1; icons <= my numberOfConstraints; icons ++)
		{
			numberOfViolations += my candidates [icand]. marks [icons];
		}
	}
	MelderInfo_writeLine2 (L"Decision strategy: ", kOTGrammar_decisionStrategy_getText (my decisionStrategy));
	MelderInfo_writeLine2 (L"Number of constraints: ", Melder_integer (my numberOfConstraints));
	MelderInfo_writeLine2 (L"Number of candidates: ", Melder_integer (my numberOfCandidates));
	MelderInfo_writeLine2 (L"Number of violation marks: ", Melder_integer (numberOfViolations));
}

static int writeText (I, MelderFile file)
{
	iam (OTMulti);
	MelderFile_write7 (file, L"\n<", kOTGrammar_decisionStrategy_getText (my decisionStrategy),
		L">\n", Melder_double (my leak), L" ! leak\n", Melder_integer (my numberOfConstraints), L" constraints");
	for (long icons = 1; icons <= my numberOfConstraints; icons ++)
	{
		OTConstraint constraint = & my constraints [icons];
		MelderFile_write1 (file, L"\n\t\"");
		for (const wchar_t *p = & constraint -> name [0]; *p; p ++)
		{
			if (*p =='\"') MelderFile_writeCharacter (file, '\"');   // Double any quotes within quotes.
			MelderFile_writeCharacter (file, *p);
		}
		MelderFile_write6 (file, L"\" ", Melder_double (constraint -> ranking),
			L" ", Melder_double (constraint -> disharmony), L" ", Melder_double (constraint -> plasticity));
	}
	MelderFile_write3 (file, L"\n\n", Melder_integer (my numberOfCandidates), L" candidates");
	for (long icand = 1; icand <= my numberOfCandidates; icand ++)
	{
		OTCandidate candidate = & my candidates [icand];
		MelderFile_write1 (file, L"\n\t\"");
		for (const wchar_t *p = & candidate -> string [0]; *p; p ++)
		{
			if (*p =='\"') MelderFile_writeCharacter (file, '\"');   // Double any quotes within quotes.
			MelderFile_writeCharacter (file, *p);
		}
		MelderFile_write1 (file, L"\"  ");
		for (long icons = 1; icons <= candidate -> numberOfConstraints; icons ++)
		{
			MelderFile_write2 (file, L" ", Melder_integer (candidate -> marks [icons]));
		}
	}
	return 1;
}

void OTMulti_checkIndex (OTMulti me)
{
	if (my index) return;
	my index = NUMlvector (1, my numberOfConstraints);
	for (long icons = 1; icons <= my numberOfConstraints; icons ++) my index [icons] = icons;
	OTMulti_sort (me);
}

static int readText (I, MelderReadText text)
{
	int localVersion = Thing_version;
	iam (OTMulti);
	if (! inherited (OTMulti) readText (me, text)) return 0;
	if (localVersion >= 1)
	{
		if ((my decisionStrategy = texgete1 (text, kOTGrammar_decisionStrategy_getValue)) < 0) return Melder_error1 (L"Trying to read decision strategy.");
	}
	if (localVersion >= 2)
	{
		my leak = texgetr8 (text); iferror return Melder_error1 (L"Trying to read leak.");
	}
	if ((my numberOfConstraints = texgeti4 (text)) < 1) return Melder_error1 (L"No constraints.");
	if (! (my constraints = NUMstructvector (OTConstraint, 1, my numberOfConstraints))) return 0;
	for (long icons = 1; icons <= my numberOfConstraints; icons ++)
	{
		OTConstraint constraint = & my constraints [icons];
		if ((constraint -> name = texgetw2 (text)) == NULL) return 0;
		constraint -> ranking = texgetr8 (text);
		constraint -> disharmony = texgetr8 (text);
		if (localVersion < 2)
		{
			constraint -> plasticity = 1.0;
		}
		else
		{
			constraint -> plasticity = texgetr8 (text); iferror return Melder_error3 (L"Trying to read plasticity of constraint ", Melder_integer (icons), L".");;
		}
	}
	if ((my numberOfCandidates = texgeti4 (text)) < 1) return Melder_error1 (L"No candidates.");
	if (! (my candidates = NUMstructvector (OTCandidate, 1, my numberOfCandidates))) return 0;
	for (long icand = 1; icand <= my numberOfCandidates; icand ++)
	{
		OTCandidate candidate = & my candidates [icand];
		if ((candidate -> string = texgetw2 (text)) == NULL) return 0;
		candidate -> numberOfConstraints = my numberOfConstraints;   /* Redundancy, needed for writing binary. */
		if ((candidate -> marks = NUMivector (1, candidate -> numberOfConstraints)) == NULL) return 0;
		for (long icons = 1; icons <= candidate -> numberOfConstraints; icons ++)
		{
			candidate -> marks [icons] = texgeti2 (text);
		}
	}
	OTMulti_checkIndex (me);
	return 1;
}

class_methods (OTMulti, Data)
	us -> version = 2;
	class_method_local (OTMulti, destroy)
	class_method_local (OTMulti, info)
	class_method_local (OTMulti, description)
	class_method_local (OTMulti, copy)
	class_method_local (OTMulti, equal)
	class_method_local (OTMulti, canWriteAsEncoding)
	class_method (writeText)
	class_method (readText)
	class_method_local (OTMulti, writeBinary)
	class_method_local (OTMulti, readBinary)
class_methods_end

long OTMulti_getConstraintIndexFromName (OTMulti me, const wchar_t *name)
{
	for (long icons = 1; icons <= my numberOfConstraints; icons ++)
	{
		if (Melder_wcsequ (my constraints [icons]. name, name))
		{
			return icons;
		}
	}
	return 0;
}

static OTMulti constraintCompare_grammar;

static int constraintCompare (const void *first, const void *second) {
	OTMulti me = constraintCompare_grammar;
	long icons = * (long *) first, jcons = * (long *) second;
	OTConstraint ci = & my constraints [icons], cj = & my constraints [jcons];
	/*
	 * Sort primarily by disharmony.
	 */
	if (ci -> disharmony > cj -> disharmony) return -1;
	if (ci -> disharmony < cj -> disharmony) return +1;
	/*
	 * Tied constraints are sorted alphabetically.
	 */
	return wcscmp (my constraints [icons]. name, my constraints [jcons]. name);
}

void OTMulti_sort (OTMulti me) {
	constraintCompare_grammar = me;
	qsort (& my index [1], my numberOfConstraints, sizeof (long), constraintCompare);
	for (long icons = 1; icons <= my numberOfConstraints; icons ++) {
		OTConstraint constraint = & my constraints [my index [icons]];
		constraint -> tiedToTheLeft = icons > 1 &&
			my constraints [my index [icons - 1]]. disharmony == constraint -> disharmony;
		constraint -> tiedToTheRight = icons < my numberOfConstraints &&
			my constraints [my index [icons + 1]]. disharmony == constraint -> disharmony;
	}
}

void OTMulti_newDisharmonies (OTMulti me, double evaluationNoise)
{
	for (long icons = 1; icons <= my numberOfConstraints; icons ++)
	{
		OTConstraint constraint = & my constraints [icons];
		constraint -> disharmony = constraint -> ranking + NUMrandomGauss (0, evaluationNoise);
	}
	OTMulti_sort (me);
}

int OTMulti_compareCandidates (OTMulti me, long icand1, long icand2)
{
	int *marks1 = my candidates [icand1]. marks;
	int *marks2 = my candidates [icand2]. marks;
	if (my decisionStrategy == kOTGrammar_decisionStrategy_OPTIMALITY_THEORY)
	{
		for (long icons = 1; icons <= my numberOfConstraints; icons ++)
		{
			int numberOfMarks1 = marks1 [my index [icons]];
			int numberOfMarks2 = marks2 [my index [icons]];
			/*
			 * Count tied constraints as one.
			 */
			while (my constraints [my index [icons]]. tiedToTheRight) {
				icons ++;
				numberOfMarks1 += marks1 [my index [icons]];
				numberOfMarks2 += marks2 [my index [icons]];
			}
			if (numberOfMarks1 < numberOfMarks2) return -1;   /* Candidate 1 is better than candidate 2. */
			if (numberOfMarks1 > numberOfMarks2) return +1;   /* Candidate 2 is better than candidate 1. */
		}
	}
	else if (my decisionStrategy == kOTGrammar_decisionStrategy_HARMONIC_GRAMMAR ||
		my decisionStrategy == kOTGrammar_decisionStrategy_MAXIMUM_ENTROPY)
	{
		double disharmony1 = 0.0, disharmony2 = 0.0;
		for (long icons = 1; icons <= my numberOfConstraints; icons ++)
		{
			disharmony1 += my constraints [icons]. disharmony * marks1 [icons];
			disharmony2 += my constraints [icons]. disharmony * marks2 [icons];
		}
		if (disharmony1 < disharmony2) return -1;   /* Candidate 1 is better than candidate 2. */
		if (disharmony1 > disharmony2) return +1;   /* Candidate 2 is better than candidate 1. */
	}
	else if (my decisionStrategy == kOTGrammar_decisionStrategy_LINEAR_OT)
	{
		double disharmony1 = 0.0, disharmony2 = 0.0;
		for (long icons = 1; icons <= my numberOfConstraints; icons ++)
		{
			if (my constraints [icons]. disharmony > 0.0)
			{
				disharmony1 += my constraints [icons]. disharmony * marks1 [icons];
				disharmony2 += my constraints [icons]. disharmony * marks2 [icons];
			}
		}
		if (disharmony1 < disharmony2) return -1;   /* Candidate 1 is better than candidate 2. */
		if (disharmony1 > disharmony2) return +1;   /* Candidate 2 is better than candidate 1. */
	}
	else if (my decisionStrategy == kOTGrammar_decisionStrategy_EXPONENTIAL_HG ||
		my decisionStrategy == kOTGrammar_decisionStrategy_EXPONENTIAL_MAXIMUM_ENTROPY)
	{
		double disharmony1 = 0.0, disharmony2 = 0.0;
		for (long icons = 1; icons <= my numberOfConstraints; icons ++)
		{
			disharmony1 += exp (my constraints [icons]. disharmony) * marks1 [icons];
			disharmony2 += exp (my constraints [icons]. disharmony) * marks2 [icons];
		}
		if (disharmony1 < disharmony2) return -1;   /* Candidate 1 is better than candidate 2. */
		if (disharmony1 > disharmony2) return +1;   /* Candidate 2 is better than candidate 1. */
	}
	else if (my decisionStrategy == kOTGrammar_decisionStrategy_POSITIVE_HG)
	{
		double disharmony1 = 0.0, disharmony2 = 0.0;
		for (long icons = 1; icons <= my numberOfConstraints; icons ++)
		{
			double constraintDisharmony = my constraints [icons]. disharmony > 1.0 ? my constraints [icons]. disharmony : 1.0;
			disharmony1 += constraintDisharmony * marks1 [icons];
			disharmony2 += constraintDisharmony * marks2 [icons];
		}
		if (disharmony1 < disharmony2) return -1;   /* Candidate 1 is better than candidate 2. */
		if (disharmony1 > disharmony2) return +1;   /* Candidate 2 is better than candidate 1. */
	}
	else
	{
		Melder_fatal ("Unimplemented decision strategy.");
	}
	return 0;   /* None of the comparisons found a difference between the two candidates. Hence, they are equally good. */
}

int OTMulti_candidateMatches (OTMulti me, long icand, const wchar_t *form1, const wchar_t *form2)
{
	const wchar_t *string = my candidates [icand]. string;
	return (form1 [0] == '\0' || wcsstr (string, form1)) && (form2 [0] == '\0' || wcsstr (string, form2));
}

static void _OTMulti_fillInHarmonies (OTMulti me, const wchar_t *form1, const wchar_t *form2) {
	if (my decisionStrategy == kOTGrammar_decisionStrategy_OPTIMALITY_THEORY) return;
	for (long icand = 1; icand <= my numberOfCandidates; icand ++) if (OTMulti_candidateMatches (me, icand, form1, form2)) {
		OTCandidate candidate = & my candidates [icand];
		int *marks = candidate -> marks;
		double disharmony = 0.0;
		if (my decisionStrategy == kOTGrammar_decisionStrategy_HARMONIC_GRAMMAR ||
			my decisionStrategy == kOTGrammar_decisionStrategy_MAXIMUM_ENTROPY)
		{
			for (long icons = 1; icons <= my numberOfConstraints; icons ++) {
				disharmony += my constraints [icons]. disharmony * marks [icons];
			}
		} else if (my decisionStrategy == kOTGrammar_decisionStrategy_EXPONENTIAL_HG ||
			my decisionStrategy == kOTGrammar_decisionStrategy_EXPONENTIAL_MAXIMUM_ENTROPY)
		{
			for (long icons = 1; icons <= my numberOfConstraints; icons ++) {
				disharmony += exp (my constraints [icons]. disharmony) * marks [icons];
			}
		} else if (my decisionStrategy == kOTGrammar_decisionStrategy_LINEAR_OT) {
			for (long icons = 1; icons <= my numberOfConstraints; icons ++) {
				if (my constraints [icons]. disharmony > 0.0) {
					disharmony += my constraints [icons]. disharmony * marks [icons];
				}
			}
		} else if (my decisionStrategy == kOTGrammar_decisionStrategy_POSITIVE_HG) {
			for (long icons = 1; icons <= my numberOfConstraints; icons ++) {
				double constraintDisharmony = my constraints [icons]. disharmony > 1.0 ? my constraints [icons]. disharmony : 1.0;
				disharmony += constraintDisharmony * marks [icons];
			}
		} else {
			Melder_fatal ("_OTMulti_fillInHarmonies: unimplemented decision strategy.");
		}
		candidate -> harmony = - disharmony;
	}
}

static void _OTMulti_fillInProbabilities (OTMulti me, const wchar_t *form1, const wchar_t *form2) {
	double maximumHarmony = my candidates [1]. harmony;
	for (long icand = 2; icand <= my numberOfCandidates; icand ++) if (OTMulti_candidateMatches (me, icand, form1, form2)) {
		OTCandidate candidate = & my candidates [icand];
		if (candidate -> harmony > maximumHarmony) {
			maximumHarmony = candidate -> harmony;
		}
	}
	for (long icand = 1; icand <= my numberOfCandidates; icand ++) if (OTMulti_candidateMatches (me, icand, form1, form2)) {
		OTCandidate candidate = & my candidates [icand];
		candidate -> probability = exp (candidate -> harmony - maximumHarmony);
		Melder_assert (candidate -> probability >= 0.0 && candidate -> probability <= 1.0);
	}
	double sumOfProbabilities = 0.0;
	for (long icand = 1; icand <= my numberOfCandidates; icand ++) if (OTMulti_candidateMatches (me, icand, form1, form2)) {
		OTCandidate candidate = & my candidates [icand];
		sumOfProbabilities += candidate -> probability;
	}
	Melder_assert (sumOfProbabilities > 0.0);   // Because at least one of them is 1.0.
	for (long icand = 1; icand <= my numberOfCandidates; icand ++) if (OTMulti_candidateMatches (me, icand, form1, form2)) {
		OTCandidate candidate = & my candidates [icand];
		candidate -> probability /= sumOfProbabilities;
	}
}

long OTMulti_getWinner (OTMulti me, const wchar_t *form1, const wchar_t *form2)
{
	long icand_best = 0;
	if (my decisionStrategy == kOTGrammar_decisionStrategy_MAXIMUM_ENTROPY ||
		my decisionStrategy == kOTGrammar_decisionStrategy_EXPONENTIAL_MAXIMUM_ENTROPY)
	{
		_OTMulti_fillInHarmonies (me, form1, form2);
		_OTMulti_fillInProbabilities (me, form1, form2);
		double cutOff = NUMrandomUniform (0.0, 1.0);
		double sumOfProbabilities = 0.0;
		for (long icand = 1; icand <= my numberOfCandidates; icand ++) if (OTMulti_candidateMatches (me, icand, form1, form2)) {
			sumOfProbabilities += my candidates [icand]. probability;
			if (sumOfProbabilities > cutOff) {
				icand_best = icand;
				break;
			}
		}
	} else {
		long numberOfBestCandidates = 0;
		for (long icand = 1; icand <= my numberOfCandidates; icand ++) if (OTMulti_candidateMatches (me, icand, form1, form2)) {
			if (icand_best == 0) {
				icand_best = icand;
				numberOfBestCandidates = 1;
			} else {
				int comparison = OTMulti_compareCandidates (me, icand, icand_best);
				if (comparison == -1) {
					icand_best = icand;   // the current candidate is the unique best candidate found so far
					numberOfBestCandidates = 1;
				} else if (comparison == 0) {
					numberOfBestCandidates += 1;   // the current candidate is equally good as the best found before
					/*
					 * Give all candidates that are equally good an equal chance to become the winner.
					 */
					if (NUMrandomUniform (0.0, numberOfBestCandidates) < 1.0) {
						icand_best = icand;
					}
				}
			}
		}
	}
	if (icand_best == 0) error5 (L"The forms ", form1, L" and ", form2, L" do not match any candidate.")
end:
	iferror return Melder_error1 (L"(OTMulti: Get winner (two):) Not performed.");
	return icand_best;
}

static int OTMulti_modifyRankings (OTMulti me, long iwinner, long iloser,
	enum kOTGrammar_rerankingStrategy updateRule,
	double plasticity, double relativePlasticityNoise)
{
	bool *grammarHasChanged = NULL;   // to be implemented
	bool warnIfStalled = false;   // to be implemented
	OTCandidate winner = & my candidates [iwinner], loser = & my candidates [iloser];
	double step = relativePlasticityNoise == 0.0 ? plasticity : NUMrandomGauss (plasticity, relativePlasticityNoise * plasticity);
	bool multiplyStepByNumberOfViolations =
		my decisionStrategy == kOTGrammar_decisionStrategy_HARMONIC_GRAMMAR ||
		my decisionStrategy == kOTGrammar_decisionStrategy_LINEAR_OT ||
		my decisionStrategy == kOTGrammar_decisionStrategy_MAXIMUM_ENTROPY ||
		my decisionStrategy == kOTGrammar_decisionStrategy_POSITIVE_HG ||
		my decisionStrategy == kOTGrammar_decisionStrategy_EXPONENTIAL_HG ||
		my decisionStrategy == kOTGrammar_decisionStrategy_EXPONENTIAL_MAXIMUM_ENTROPY;
	if (Melder_debug != 0)
	{
		/*
		 * Perhaps override the standard update rule.
		 */
		if (Melder_debug == 26) multiplyStepByNumberOfViolations = false;   // OT-GLA
		else if (Melder_debug == 27) multiplyStepByNumberOfViolations = true;   // HG-GLA
	}
	if (updateRule == kOTGrammar_rerankingStrategy_SYMMETRIC_ONE)
	{
		long icons = NUMrandomInteger (1, my numberOfConstraints);
		OTConstraint constraint = & my constraints [icons];
		double constraintStep = step * constraint -> plasticity;
		int winnerMarks = winner -> marks [icons];
		int loserMarks = loser -> marks [icons];
		if (loserMarks > winnerMarks)
		{
			if (multiplyStepByNumberOfViolations) constraintStep *= loserMarks - winnerMarks;
			constraint -> ranking -= constraintStep * (1.0 + constraint -> ranking * my leak);
			if (grammarHasChanged != NULL) *grammarHasChanged = true;
		}
		if (winnerMarks > loserMarks)
		{
			if (multiplyStepByNumberOfViolations) constraintStep *= winnerMarks - loserMarks;
			constraint -> ranking += constraintStep * (1.0 - constraint -> ranking * my leak);
			if (grammarHasChanged != NULL) *grammarHasChanged = true;
		}
	}
	else if (updateRule == kOTGrammar_rerankingStrategy_SYMMETRIC_ALL)
	{
		bool changed = false;
		for (long icons = 1; icons <= my numberOfConstraints; icons ++)
		{
			OTConstraint constraint = & my constraints [icons];
			double constraintStep = step * constraint -> plasticity;
			int winnerMarks = winner -> marks [icons];
			int loserMarks = loser -> marks [icons];
			if (loserMarks > winnerMarks)
			{
				if (multiplyStepByNumberOfViolations) constraintStep *= loserMarks - winnerMarks;
				constraint -> ranking -= constraintStep * (1.0 + constraint -> ranking * my leak);
				changed = true;
			}
			if (winnerMarks > loserMarks)
			{
				if (multiplyStepByNumberOfViolations) constraintStep *= winnerMarks - loserMarks;
				constraint -> ranking += constraintStep * (1.0 - constraint -> ranking * my leak);
				changed = true;
			}
		}
		if (changed && my decisionStrategy == kOTGrammar_decisionStrategy_EXPONENTIAL_HG)
		{
			double sumOfWeights = 0.0;
			for (long icons = 1; icons <= my numberOfConstraints; icons ++)
			{
				sumOfWeights += my constraints [icons]. ranking;
			}
			double averageWeight = sumOfWeights / my numberOfConstraints;
			for (long icons = 1; icons <= my numberOfConstraints; icons ++)
			{
				my constraints [icons]. ranking -= averageWeight;
			}
		}
		if (grammarHasChanged != NULL) *grammarHasChanged = changed;
	}
	else if (updateRule == kOTGrammar_rerankingStrategy_WEIGHTED_UNCANCELLED)
	{
		int winningConstraints = 0, losingConstraints = 0;
		for (long icons = 1; icons <= my numberOfConstraints; icons ++)
		{
			int winnerMarks = winner -> marks [icons];
			int loserMarks = loser -> marks [icons];
			if (loserMarks > winnerMarks) losingConstraints ++;
			if (winnerMarks > loserMarks) winningConstraints ++;
		}
		if (winningConstraints != 0) {
			for (long icons = 1; icons <= my numberOfConstraints; icons ++)
			{
				OTConstraint constraint = & my constraints [icons];
				double constraintStep = step * constraint -> plasticity;
				int winnerMarks = winner -> marks [icons];
				int loserMarks = loser -> marks [icons];
				if (loserMarks > winnerMarks)
				{
					if (multiplyStepByNumberOfViolations) constraintStep *= loserMarks - winnerMarks;
					constraint -> ranking -= constraintStep * (1.0 + constraint -> ranking * my leak) / losingConstraints;
					//constraint -> ranking -= constraintStep * (1.0 + constraint -> ranking * my leak) * winningConstraints;
					if (grammarHasChanged != NULL) *grammarHasChanged = true;
				}
				if (winnerMarks > loserMarks)
				{
					if (multiplyStepByNumberOfViolations) constraintStep *= winnerMarks - loserMarks;
					constraint -> ranking += constraintStep * (1.0 - constraint -> ranking * my leak) / winningConstraints;
					//constraint -> ranking += constraintStep * (1.0 - constraint -> ranking * my leak) * losingConstraints;
					if (grammarHasChanged != NULL) *grammarHasChanged = true;
				}
			}
		}
	}
	else if (updateRule == kOTGrammar_rerankingStrategy_WEIGHTED_ALL)
	{
		int winningConstraints = 0, losingConstraints = 0;
		for (long icons = 1; icons <= my numberOfConstraints; icons ++)
		{
			int winnerMarks = winner -> marks [icons];
			int loserMarks = loser -> marks [icons];
			if (loserMarks > 0) losingConstraints ++;
			if (winnerMarks > 0) winningConstraints ++;
		}
		if (winningConstraints != 0) for (long icons = 1; icons <= my numberOfConstraints; icons ++) 
		{
			OTConstraint constraint = & my constraints [icons];
			double constraintStep = step * constraint -> plasticity;
			int winnerMarks = winner -> marks [icons];
			int loserMarks = loser -> marks [icons];
			if (loserMarks > 0)
			{
				if (multiplyStepByNumberOfViolations) constraintStep *= loserMarks - winnerMarks;
				constraint -> ranking -= constraintStep * (1.0 + constraint -> ranking * my leak) / losingConstraints;
				if (grammarHasChanged != NULL) *grammarHasChanged = true;
			}
			if (winnerMarks > 0)
			{
				if (multiplyStepByNumberOfViolations) constraintStep *= winnerMarks - loserMarks;
				constraint -> ranking += constraintStep * (1.0 - constraint -> ranking * my leak) / winningConstraints;
				if (grammarHasChanged != NULL) *grammarHasChanged = true;
			}
		}
	}
	else if (updateRule == kOTGrammar_rerankingStrategy_EDCD || updateRule == kOTGrammar_rerankingStrategy_EDCD_WITH_VACATION)
	{
		/*
		 * Determine the crucial winner mark.
		 */
		double pivotRanking;
		bool equivalent = true;
		long icons = 1;
		for (; icons <= my numberOfConstraints; icons ++)
		{
			int winnerMarks = winner -> marks [my index [icons]];   // order is important, so indirect
			int loserMarks = loser -> marks [my index [icons]];
			if (loserMarks < winnerMarks) break;
			if (loserMarks > winnerMarks) equivalent = false;
		}
		if (icons > my numberOfConstraints)   // completed the loop?
		{
			if (warnIfStalled && ! equivalent)
				Melder_warning4 (L"Correct output is harmonically bounded (by having strict superset violations as compared to the learner's output)! EDCD stalls.\n"
					"Correct output: ", loser -> string, L"\nLearner's output: ", winner -> string);
			goto end;
		}
		/*
		 * Determine the stratum into which some constraints will be demoted.
		 */
		pivotRanking = my constraints [my index [icons]]. ranking;
		if (updateRule == kOTGrammar_rerankingStrategy_EDCD_WITH_VACATION)
		{
			long numberOfConstraintsToDemote = 0;
			for (icons = 1; icons <= my numberOfConstraints; icons ++)
			{
				int winnerMarks = winner -> marks [icons];
				int loserMarks = loser -> marks [icons];
				if (loserMarks > winnerMarks)
				{
					OTConstraint constraint = & my constraints [icons];
					if (constraint -> ranking >= pivotRanking)
					{
						numberOfConstraintsToDemote += 1;
					}
				}
			}
			if (numberOfConstraintsToDemote > 0)
			{
				for (icons = 1; icons <= my numberOfConstraints; icons ++)
				{
					OTConstraint constraint = & my constraints [icons];
					if (constraint -> ranking < pivotRanking)
					{
						constraint -> ranking -= numberOfConstraintsToDemote * step * constraint -> plasticity;
						if (grammarHasChanged != NULL) *grammarHasChanged = true;
					}
				}
			}
		}
		/*
		 * Demote all the uniquely violated constraints in the loser
		 * that have rankings not lower than the pivot.
		 */
		for (icons = 1; icons <= my numberOfConstraints; icons ++)
		{
			long numberOfConstraintsDemoted = 0;
			int winnerMarks = winner -> marks [my index [icons]];   // For the vacation version, the order is important, so indirect.
			int loserMarks = loser -> marks [my index [icons]];
			if (loserMarks > winnerMarks)
			{
				OTConstraint constraint = & my constraints [my index [icons]];
				double constraintStep = step * constraint -> plasticity;
				if (constraint -> ranking >= pivotRanking)
				{
					numberOfConstraintsDemoted += 1;
					constraint -> ranking = pivotRanking - numberOfConstraintsDemoted * constraintStep;   // This preserves the order of the demotees.
					if (grammarHasChanged != NULL) *grammarHasChanged = true;
				}
			}
		}
	} else if (updateRule == kOTGrammar_rerankingStrategy_DEMOTION_ONLY) {
		/*
		 * Determine the crucial loser mark.
		 */
		long crucialLoserMark;
		OTConstraint offendingConstraint;
		long icons = 1;
		for (; icons <= my numberOfConstraints; icons ++)
		{
			int winnerMarks = winner -> marks [my index [icons]];   /* Order is important, so indirect. */
			int loserMarks = loser -> marks [my index [icons]];
			if (my constraints [my index [icons]]. tiedToTheRight)
				error1 (L"Demotion-only learning cannot handle tied constraints.")
			if (loserMarks < winnerMarks)
				error1 (L"Demotion-only learning step: Loser wins! Should never happen.")
			if (loserMarks > winnerMarks) break;
		}
		if (icons > my numberOfConstraints)   /* Completed the loop? */
			error1 (L"(OTGrammar_step:) Loser equals correct candidate.")
		crucialLoserMark = icons;
		/*
		 * Demote the highest uniquely violated constraint in the loser.
		 */
		offendingConstraint = & my constraints [my index [crucialLoserMark]];
		double constraintStep = step * offendingConstraint -> plasticity;
		offendingConstraint -> ranking -= constraintStep;
		if (grammarHasChanged != NULL) *grammarHasChanged = true;
	} else { Melder_assert (updateRule == kOTGrammar_rerankingStrategy_WEIGHTED_ALL_UP_HIGHEST_DOWN);
		bool changed = false;
		long numberOfUp = 0;
		for (long icons = 1; icons <= my numberOfConstraints; icons ++)
		{
			int winnerMarks = winner -> marks [icons];
			int loserMarks = loser -> marks [icons];
			if (winnerMarks > loserMarks)
			{
				numberOfUp ++;
			}
		}
		if (numberOfUp > 0)
		{
			for (long icons = 1; icons <= my numberOfConstraints; icons ++)
			{
				OTConstraint constraint = & my constraints [icons];
				double constraintStep = step * constraint -> plasticity;
				int winnerMarks = winner -> marks [icons];
				int loserMarks = loser -> marks [icons];
				if (winnerMarks > loserMarks)
				{
					if (multiplyStepByNumberOfViolations) constraintStep *= winnerMarks - loserMarks;
					constraint -> ranking += constraintStep * (1.0 - constraint -> ranking * my leak) / numberOfUp;
				}
			}
			long crucialLoserMark, winnerMarks = 0, loserMarks = 0;
			OTConstraint offendingConstraint;
			long icons = 1;
			for (; icons <= my numberOfConstraints; icons ++)
			{
				winnerMarks = winner -> marks [my index [icons]];   /* Order is important, so indirect. */
				loserMarks = loser -> marks [my index [icons]];
				if (my constraints [my index [icons]]. tiedToTheRight)
					error1 (L"Demotion-only learning cannot handle tied constraints.")
				if (loserMarks < winnerMarks)
					error1 (L"Demotion-only learning step: Loser wins! Should never happen.")
				if (loserMarks > winnerMarks) break;
			}
			if (icons > my numberOfConstraints)   /* Completed the loop? */
				error1 (L"(OTGrammar_step:) Loser equals correct candidate.")
			crucialLoserMark = icons;
			/*
			 * Demote the highest uniquely violated constraint in the loser.
			 */
			offendingConstraint = & my constraints [my index [crucialLoserMark]];
			double constraintStep = step * offendingConstraint -> plasticity;
			if (multiplyStepByNumberOfViolations) constraintStep *= winnerMarks - loserMarks;
			offendingConstraint -> ranking -= /*numberOfUp **/ constraintStep * (1.0 - offendingConstraint -> ranking * my leak);
		}
		if (grammarHasChanged != NULL) *grammarHasChanged = changed;
	}
end:
	iferror return 0;
	return 1;
}

int OTMulti_learnOne (OTMulti me, const wchar_t *form1, const wchar_t *form2,
	enum kOTGrammar_rerankingStrategy updateRule, int direction, double plasticity, double relativePlasticityNoise)
{
	long iloser = OTMulti_getWinner (me, form1, form2); cherror
	if (direction & OTMulti_LEARN_FORWARD)
	{
		long iwinner = OTMulti_getWinner (me, form1, L""); cherror
		OTMulti_modifyRankings (me, iwinner, iloser, updateRule, plasticity, relativePlasticityNoise); cherror
	}
	if (direction & OTMulti_LEARN_BACKWARD)
	{
		long iwinner = OTMulti_getWinner (me, form2, L""); cherror
		OTMulti_modifyRankings (me, iwinner, iloser, updateRule, plasticity, relativePlasticityNoise); cherror
	}
end:
	iferror return 0;
	return 1;
}

Table OTMulti_createHistory (OTMulti me, long storeHistoryEvery, long numberOfData)
{
	Table thee = NULL;
//start:
	long numberOfSamplingPoints = numberOfData / storeHistoryEvery;   /* E.g. 0, 20, 40, ... */
	thee = Table_createWithoutColumnNames (1 + numberOfSamplingPoints, 3 + my numberOfConstraints); cherror
	Table_setColumnLabel (thee, 1, L"Datum"); cherror
	Table_setColumnLabel (thee, 2, L"Form1"); cherror
	Table_setColumnLabel (thee, 3, L"Form2"); cherror
	for (long icons = 1; icons <= my numberOfConstraints; icons ++) {
		Table_setColumnLabel (thee, 3 + icons, my constraints [icons]. name); cherror
	}
	Table_setNumericValue (thee, 1, 1, 0); cherror
	Table_setStringValue (thee, 1, 2, L"(initial)"); cherror
	Table_setStringValue (thee, 1, 3, L"(initial)"); cherror
	for (long icons = 1; icons <= my numberOfConstraints; icons ++) {
		Table_setNumericValue (thee, 1, 3 + icons, my constraints [icons]. ranking); cherror
	}
end:
	iferror {
		Melder_error1 (L"OTMulti history not created.");
		forget (thee);
	}
	return thee;
}

#if 0
static Table OTMulti_createHistory (OTMulti me, long storeHistoryEvery, long numberOfData)
{
	Table thee = NULL;
	try {
		long numberOfSamplingPoints = numberOfData / storeHistoryEvery;   /* E.g. 0, 20, 40, ... */
		thee = Table_createWithoutColumnNames (1 + numberOfSamplingPoints, 3 + my numberOfConstraints);
		Table_setColumnLabel (thee, 1, L"Datum");
		Table_setColumnLabel (thee, 2, L"Form1");
		Table_setColumnLabel (thee, 3, L"Form2");
		for (long icons = 1; icons <= my numberOfConstraints; icons ++) {
			Table_setColumnLabel (thee, 3 + icons, my constraints [icons]. name);
		}
		Table_setNumericValue (thee, 1, 1, 0);
		Table_setStringValue (thee, 1, 2, L"(initial)");
		Table_setStringValue (thee, 1, 3, L"(initial)");
		for (long icons = 1; icons <= my numberOfConstraints; icons ++) {
			Table_setNumericValue (thee, 1, 3 + icons, my constraints [icons]. ranking);
		}
	} catch (...) {
		forget (thee);
		throwcat (L"OTMulti history not created.");
	}
	return thee;
}
#endif

int OTMulti_updateHistory (OTMulti me, Table thee, long storeHistoryEvery, long idatum, const wchar_t *form1, const wchar_t *form2)
{
//start:
	if (idatum % storeHistoryEvery == 0) {
		long irow = 1 + idatum / storeHistoryEvery;
		Table_setNumericValue (thee, irow, 1, idatum); cherror
		Table_setStringValue (thee, irow, 2, form1); cherror
		Table_setStringValue (thee, irow, 3, form2); cherror
		for (long icons = 1; icons <= my numberOfConstraints; icons ++) {
			Table_setNumericValue (thee, irow, 3 + icons, my constraints [icons]. ranking); cherror
		}
	}
end:
	iferror return 0;
	return 1;
}

void OTMulti_reset (OTMulti me, double ranking) {
	for (long icons = 1; icons <= my numberOfConstraints; icons ++)
	{
		OTConstraint constraint = & my constraints [icons];
		constraint -> disharmony = constraint -> ranking = ranking;
	}
	OTMulti_sort (me);
}

int OTMulti_setRanking (OTMulti me, long constraint, double ranking, double disharmony) {
	if (constraint < 1 || constraint > my numberOfConstraints)
		return Melder_error ("(OTGrammar_setRanking): No constraint %ld.", constraint);
	my constraints [constraint]. ranking = ranking;
	my constraints [constraint]. disharmony = disharmony;
	OTMulti_sort (me);
	return 1;
}

int OTMulti_setConstraintPlasticity (OTMulti me, long constraint, double plasticity) {
	if (constraint < 1 || constraint > my numberOfConstraints)
		return Melder_error ("(OTMulti_setConstraintPlasticity): No constraint %ld.", constraint);
	my constraints [constraint]. plasticity = plasticity;
	return 1;
}

int OTMulti_removeConstraint (OTMulti me, const wchar_t *constraintName) {
	long removed = 0;

	if (my numberOfConstraints <= 1)
		return Melder_error1 (L"Cannot remove last constraint.");

	/*
	 * Look for the constraint to be removed.
	 */
	for (long icons = 1; icons <= my numberOfConstraints; icons ++) {
		OTConstraint constraint = & my constraints [icons];
		if (wcsequ (constraint -> name, constraintName)) {
			removed = icons;
			break;
		}
	}
	if (removed == 0)
		return Melder_error3 (L"No constraint \"", constraintName, L"\".");
	/*
	 * Remove the constraint while reusing the memory space.
	 */
	my numberOfConstraints -= 1;
	/*
	 * Shift constraints.
	 */
	Melder_free (my constraints [removed]. name);
	for (long icons = removed; icons <= my numberOfConstraints; icons ++) {
		my constraints [icons] = my constraints [icons + 1];
	}
	/*
	 * Shift tableau rows.
	 */
	for (long icand = 1; icand <= my numberOfCandidates; icand ++) {
		OTCandidate candidate = & my candidates [icand];
		candidate -> numberOfConstraints -= 1;
		for (long icons = removed; icons <= my numberOfConstraints; icons ++) {
			candidate -> marks [icons] = candidate -> marks [icons + 1];
		}
	}
	/*
	 * Rebuild index.
	 */
	for (long icons = 1; icons <= my numberOfConstraints; icons ++) my index [icons] = icons;
	OTMulti_sort (me);
	return 1;
}

int OTMulti_generateOptimalForm (OTMulti me, const wchar_t *form1, const wchar_t *form2, wchar_t *optimalForm, double evaluationNoise) {
	OTMulti_newDisharmonies (me, evaluationNoise);
	long winner = OTMulti_getWinner (me, form1, form2);
	if (! winner) error1 (L"No winner")
	wcscpy (optimalForm, my candidates [winner]. string);
end:
	iferror return Melder_error1 (L"(OTMulti_generateOptimalForm:) Not performed.");
	return 1;
}

Strings OTMulti_Strings_generateOptimalForms (OTMulti me, Strings thee, double evaluationNoise) {
	try {
		autoStrings outputs = Thing_new (Strings);
		long n = thy numberOfStrings;
		outputs -> numberOfStrings = n;
		outputs -> strings = NUMwvector (1, n); therror
		for (long i = 1; i <= n; i ++) {
			wchar_t output [100];
			OTMulti_generateOptimalForm (me, thy strings [i], L"", output, evaluationNoise); therror
			outputs -> strings [i] = Melder_wcsdup_e (output); therror
		}
		return outputs.transfer();
	} catch (...) {
		rethrowmzero (me, " & ", thee, ": optimal forms not generated.");
	}
}

Strings OTMulti_generateOptimalForms (OTMulti me, const wchar_t *form1, const wchar_t *form2, long numberOfTrials, double evaluationNoise) {
	try {
		autoStrings outputs = Thing_new (Strings);
		outputs -> numberOfStrings = numberOfTrials;
		outputs -> strings = NUMwvector (1, numberOfTrials); therror
		for (long i = 1; i <= numberOfTrials; i ++) {
			wchar_t output [100];
			OTMulti_generateOptimalForm (me, form1, form2, output, evaluationNoise); therror
			outputs -> strings [i] = Melder_wcsdup_e (output); therror
		}
		return outputs.transfer();
	} catch (...) {
		rethrowmzero (me, ": optimal forms not generated.");
	}
}

Distributions OTMulti_to_Distribution (OTMulti me, const wchar_t *form1, const wchar_t *form2,
	long numberOfTrials, double evaluationNoise)
{
	Distributions thee = NULL;
	long *index = NULL;
//start:
	long totalNumberOfOutputs = 0, iout = 0;
	/*
	 * Count the total number of outputs.
	 */
	for (long icand = 1; icand <= my numberOfCandidates; icand ++)
	{
		if (OTMulti_candidateMatches (me, icand, form1, form2))
		{
			totalNumberOfOutputs ++;
		}
	}
	/*
	 * Create the distribution. One row for every output form.
	 */
	thee = Distributions_create (totalNumberOfOutputs, 1); cherror
	index = NUMlvector (1, my numberOfCandidates); cherror
	/*
	 * Set the row labels to the output strings.
	 */
	iout = 0;
	for (long icand = 1; icand <= my numberOfCandidates; icand ++)
	{
		if (OTMulti_candidateMatches (me, icand, form1, form2))
		{
			thy rowLabels [++ iout] = Melder_wcsdup_e (my candidates [icand]. string); cherror
			index [icand] = iout;
		}
	}
	/*
	 * Compute a number of outputs and store the results.
	 */
	for (long itrial = 1; itrial <= numberOfTrials; itrial ++)
	{
		OTMulti_newDisharmonies (me, evaluationNoise);
		long iwinner = OTMulti_getWinner (me, form1, form2);
		thy data [index [iwinner]] [1] += 1;
	}
end:
	NUMlvector_free (index, 1);
	iferror forget (thee);
	return thee;
}

/* End of file OTMulti.c */
