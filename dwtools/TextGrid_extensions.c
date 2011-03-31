/* TextGrid_extensions.c
 *
 * Copyright (C) 1993-2007 David Weenink
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
 djmw 20020702 GPL header
 djmw 20020702 +TextGrid_extendTime
 djmw 20051215 Corrected a bug in TextGrid_readFromTIMITLabelFile that caused a crash when the first number in
 	a file was not 0 (in that case an empty interval was added as the first element in the tier).
 djmw 20060517 Added (TextTier|IntervalTier|TextGrid)_changeLabels.
 djmw 20060712 TextGrid_readFromTIMITLabelFile: don't set first boundary to zero for .wrd files.
 djmw 20060921 Added IntervalTier_removeBoundary_equalLabels, IntervalTier_removeBoundary_minimumDuration
 djmw 20061113 Reassign item in list after a deletion.
 djmw 20061116 Added IntervalTier_removeInterval to correct a bug in IntervalTier_removeBoundary...
 djmw 20071008 Removed two unused variables.
 djmw 20071202 Melder_warning<n>
*/

#include <ctype.h>
#include "TextGrid_extensions.h"
#include "NUM2.h"
#include "praat.h"

struct TIMIT_key {const char *timitLabel, *ipaLabel;} TIMIT_toIpaTable[] =
{	{"",""},
	/* Vowels */
	{"iy", "i"},			/* beet: bcl b IY tcl t */
	{"ih", "\\ic"}, 		/* bit: bcl b IH tcl t */
	{"eh", "\\ep"}, 		/* bet: bcl b EH tcl t */
	{"ey", "e"},  			/* bait: bcl b EY tcl t */
	{"ae", "\\ae"},  		/* bat: bcl b AE tcl t */
	{"aa", "\\as"}, 		/* bott: bcl b AA tcl t */
	{"aw", "a\\hs"},  		/* bout: bcl b AW tcl t */
	{"ay", "a\\ic"},  		/* bite: bcl b AY tcl t */
	{"ah", "\\vt"}, 		/* but: bcl b AH tcl t */
	{"ao", "\\ct"},  		/* bought: bcl b AO tcl t */
	{"oy", "\\ct\\ic"},  	/* boy: bcl b OY */
	{"ow", "o"}, 			/* boat: bcl b OW tcl t */
	{"uh", "\\hs"}, 		/* book: bcl b UH tcl t */
	{"uw", "u"},  			/* boot: bcl b UW tcl t */
	/* fronted allophone of uw (alveolair contexts) */
	{"ux", "\\u\""}, 		/* toot: tcl t UX tcl t */
	{"er", "\\er\\hr"},  	/* bird: bcl b ER dcl d */
	{"ax", "\\sw"}, 		/* about: AX bcl b aw tcl t */
	{"ix", "\\i-"}, 		/* debit: dcl d eh bcl b IX tcl t */
	{"axr", "\\sr"}, 		/* butter: bcl ah dx AXR */
	/* devoiced schwa, very short */
	{"ax-h", "\\sw\\ov"}, 	/* suspect: s AX-H s pcl p eh kcl k tcl t */
	/* Semivowels and glides */
	{"l", "l"},				/* lay:	L ey */
	{"r", "r"},				/* ray:	R ey */
	{"w", "w"},				/* way:	w ey */
	{"y", "j"},				/* yacht: Y aa tcl t */
	{"hh","h" },			/* hay: HH ey*/
	/* voiced allophone of h */
	{"hv", "\\hh"},			/* ahead: ax HV eh dcl d */
	{"el", "l\\|v"},		/* bottle: bcl b aa tcl t EL */
	/* Nasals */
	{"m", "m"},				/* mom:	M aa M */
	{"n", "n"},				/* noon: N uw N*/
	{"ng", "\\ng"},			/* sing: s ih NG */
	{"em", "m\\|v"},		/* bottom: b aa tcl t EM */
	{"en", "n\\|v"},		/* button:	b ah q EN */
	{"eng", "\\ng\\|v"},	/* washington: w aa sh ENG tcl t ax n */
	/* nasal flap */
	{"nx", "n^\\fh"},		/* winner: wih NX axr */
	/* Fricatives */
	{"s", "s"},				/* sea: S iy */
	{"sh", "\\sh"},			/* she: SH iy */
	{"z", "z"},				/* zone: Z ow n */
	{"zh", "\\zh"},			/* azure: ae ZH er */
	{"f", "f"},				/* fin: F ih n */
	{"th", "\\te"},			/* thin: TH ih n */
	{"v", "v"},				/* van: v ae n */
	{"dh", "\\dh"},			/* then: DH en */
	/* Affricates */
	{"jh", "d\\zh"},		/* joke: DCL JH ow kcl k */
	{"ch", "t\\sh"},		/* choke TCL CH ow kcl k */
	/* Stops */
	{"b", "b"},				/* bee: BCL B iy */
	{"d", "d"},				/* day: DCL D ey */
	{"g", "g"},				/* gay: GCL G ey */
	{"p", "p"},				/* pea: PCL P iy */
	{"t", "t"},				/* tea: TCL T iy */
	{"k", "k"},				/* key: KCL K iy */
	/* flap */
	{"dx", "\\fh"},			/* muddy: m ah DX iy & dirty: dcl d er DX iy */
	/* glottal stop */
	{"q", "?"},
	/* Others */
	{"pau", ""},	/* pause */
	{"epi", ""},	/* epenthetic silence */
	{"h#", ""}, 	/* marks start and end piece of sentence */
	/* the following markers only occur in the dictionary */
	{"1", "1"},		/* primary stress marker */
	{"2", "2"}		/* secondary stress marker */
};

#define TIMIT_NLABELS (sizeof TIMIT_toIpaTable / sizeof TIMIT_toIpaTable[1] - 1)
static char * TIMIT_DELIMITER = "h\\# ";

static const char *timitLabelToIpaLabel (const char timitLabel[])
{
	for (unsigned int i=1; i <= TIMIT_NLABELS; i++)
		if (!strcmp (TIMIT_toIpaTable[i].timitLabel, timitLabel))
			return TIMIT_toIpaTable[i].ipaLabel;
	return timitLabel;
}

static int isTimitPhoneticLabel (const char label[])
{
	for (unsigned int i=1; i <= TIMIT_NLABELS; i++)
		if (! strcmp (TIMIT_toIpaTable[i].timitLabel, label)) return 1;
	return 0;
}

static int isTimitWord (const char label[])
{
	const char *p = label;
	for (; *p; p++) if (isupper (*p) && *p != '\'') return 0;
	return 1;
}

Any TextGrid_TIMITLabelFileRecognizer (int nread, const char *header, MelderFile file)
{
	long it[5]; int length, phnFile = 0; char hkruis[3] = "h#", label1[512], label2[512];
	if (nread < 12 ||
		sscanf (header,"%ld%ld%s%n\n", &it[1], &it[2], label1, &length) != 3 ||
		it[1] < 0 || it[2] <= it[1] ||
		sscanf (&header[length],"%ld%ld%s\n", &it[3], &it[4], label2) != 3 ||
		it[3] < it[2] || it[4] <= it[3]) return NULL;
	if (! strcmp (label1, hkruis))
	{
		if (isTimitPhoneticLabel (label2)) phnFile = 1;
		else if (! isTimitWord (label2)) return NULL;
	}
	else if (! isTimitWord (label1) || ! isTimitWord (label2)) return NULL;
	return TextGrid_readFromTIMITLabelFile (file, phnFile);
}

static int IntervalTier_add (IntervalTier me, double xmin, double xmax, const wchar_t *label)
{
	TextInterval interval, new;
	long i = IntervalTier_timeToIndex (me, xmin); /* xmin is in interval i */
	double xmaxi; /* the  time at the right of interval i */

	new = TextInterval_create (xmin, xmax, label);
	if (i < 1 || new == NULL) return 0;
	interval = my intervals -> item[i];
	if (xmax > (xmaxi = interval -> xmax))
	{
		forget (new);
		return 0; /* we don't know what to do */
	}
	if (xmin == interval -> xmin)
	{
		if (xmax == interval -> xmax) /* interval already present */
		{
			forget (new);
			return TextInterval_setText (interval, label);
		}
		/* split interval */
		interval -> xmin = xmax;
		return Collection_addItem (my intervals, new);
	}
	interval -> xmax = xmin;
	if (! Collection_addItem (my intervals, new)) return 0;
	/* extra interval when xmax's are not the same */
	if (xmax < xmaxi && (!(new = TextInterval_create (xmax, xmaxi, interval -> text)) ||
		! Collection_addItem (my intervals, new))) return 0;
	return 1;
}

TextGrid TextGrid_readFromTIMITLabelFile (MelderFile file, int phnFile)
{
	TextGrid me = NULL;
	IntervalTier timit, ipa;
	TextInterval interval;
	long i, linesRead = 0, it1, it2, ni;
	char line[200], label[200];
	double tmax = 3600; /* initial value one hour should be long enough */
	double dt = 1.0 / 16000; /* TIMIT samplingFrequency is 16000 Hz */
	double xmax = dt, xmin;
	FILE *f = Melder_fopen (file, "r");

	if (! f) return Melder_errorp1 (L"Cannot open file.");
	/*
		Ending time will only be known after all labels have been read.
		Start with a sufficiently long duration (one hour) and correct this later.
	*/
	if (! (me = TextGrid_create (0, 3600, L"wrd", NULL))) goto cleanup;
	timit = my tiers -> item[1];
	while (fgets (line, 199, f))
	{
		char *labelstring;
		linesRead++;
		if (sscanf (line,"%ld%ld%s", &it1, &it2, label) != 3)
		{
			(void) Melder_error1 (L"Incorrect number of items.");
			goto cleanup;
		}
		if (it1 < 0 || it2 <= it1)
		{
			(void) Melder_error2 (L"Incorrect time at line ", Melder_integer (linesRead));
			goto cleanup;
		}
		xmin = it1 * dt;
		xmax = it2 * dt;
		ni = timit -> intervals -> size - 1;
		if (ni < 1)
		{
			ni = 1;
			/* Some files do not start with a first line "0 <number2> h#".
			   Instead they start with "<number1> <number2> h#", where number1 > 0.
			   We override number1 with 0. */

			if (xmin > 0 && phnFile) xmin = 0;
		}
		interval = timit -> intervals -> item[ni];
		if (xmin < interval -> xmax && linesRead > 1)
		{
			xmin = interval -> xmax;
			Melder_warning5 (L"File \"", MelderFile_messageName (file), L"\": Start time set to previous end "
				"time for label at line ", Melder_integer (linesRead), L".");
		}
		/* standard: new TextInterval */
		labelstring = (strncmp (label, "h#", 2) ? label : TIMIT_DELIMITER);
		if (! IntervalTier_add (timit, xmin, xmax, Melder_peekUtf8ToWcs (labelstring))) goto cleanup;
	}
	/*
		Now correct the end times, based on last read interval.
		(end time was set to large value!)
	*/
	if (timit -> intervals -> size < 2)
	{
		(void) Melder_error1 (L"Empty TextGrid");
		goto cleanup;
	}
	Collection_removeItem (timit -> intervals, timit -> intervals -> size);
	interval = timit -> intervals -> item[timit -> intervals -> size];
	tmax = interval -> xmax;
	timit -> xmax = tmax;
	my xmax = xmax;
	if (phnFile) /* Create tier 2 with IPA symbols */
	{
		ipa = Data_copy (timit);
		if (ipa == NULL || ! Collection_addItem (my tiers, ipa)) goto cleanup;
		for (i = 1; i <= ipa -> intervals -> size; i++)
		{
			interval = timit -> intervals -> item[i];

			if (! TextInterval_setText (ipa -> intervals -> item[i],
				Melder_peekUtf8ToWcs (timitLabelToIpaLabel (Melder_peekWcsToUtf8 (interval -> text))))) goto cleanup;
		}
		Thing_setName (ipa, L"ipa");
		Thing_setName (timit, L"phn");
	}
cleanup:
	fclose (f);
	if (! Melder_hasError()) return me;
	forget (me);
	return Melder_errorp3 (L"Reading from file \"", MelderFile_name (file), L"\" not performed.");
}

TextGrid TextGrids_merge (TextGrid grid1, TextGrid grid2)
{
	TextGrid me, thee = NULL;
	double extra_time_end, extra_time_start;
	int i, at_end = 0, at_start = 1;

	me = Data_copy (grid1);
	if (me == NULL) return NULL;
	thee = Data_copy (grid2);
	if (thee == NULL) goto end;

	/*
		The new TextGrid will have the domain
		[min(grid1->xmin, grid2->xmin), max(grid1->xmax, grid2->xmax)]
	*/

	extra_time_end = fabs (thy xmax - my xmax);
	extra_time_start = fabs (thy xmin - my xmin);

	if ((my xmin > thy xmin &&
		! TextGrid_extendTime (me, extra_time_start, at_start)) ||
		(my xmax < thy xmax &&
		! TextGrid_extendTime (me, extra_time_end, at_end))) goto end;

	if ((thy xmin > my xmin &&
		! TextGrid_extendTime (thee, extra_time_start, at_start)) ||
		(thy xmax < my xmax &&
		! TextGrid_extendTime (thee, extra_time_end, at_end))) goto end;

	for (i = 1; i <= thy tiers -> size; i++)
	{
		Function tier = Data_copy (thy tiers -> item [i]);
		if (tier == NULL || ! Collection_addItem (my tiers, tier)) goto end;
	}
end:
	forget (thee);
	if (Melder_hasError ()) forget (me);
	return me;
}

int TextGrid_extendTime (TextGrid me, double extra_time, int position)
{
	TextGrid thee;
	long i, ntier = my tiers -> size;
	double xmax = my xmax, xmin = my xmin;
	int at_end = position == 0;

	if (extra_time == 0) return 1;
	extra_time = fabs (extra_time); /* Just in case... */
	thee = Data_copy (me);
	if (thee == NULL) return 0;

	if (at_end) xmax += extra_time;
	else xmin -= extra_time;

	for (i = 1; i <= ntier; i++)
	{
		Function anyTier = my tiers -> item [i];
		double tmin = anyTier -> xmin, tmax = anyTier -> xmax;

		if (at_end)
		{
			anyTier -> xmax = xmax;
			tmin = tmax;
			tmax = xmax;
		}
		else
		{
			anyTier -> xmin = xmin;
			tmax = tmin;
			tmin = xmin;
		}
		if (anyTier -> methods == (Function_Table) classIntervalTier)
		{
			IntervalTier tier = (IntervalTier) anyTier;

			TextInterval interval = TextInterval_create (tmin, tmax, L"");
			if (interval == NULL ||
				! Collection_addItem (tier -> intervals, interval))
			{
				/*
					Restore original TextGrid and quit
				*/
				TextGrid tmp = me;
				me = thee;
				forget (tmp);
				return 0;
			}
		}
	}

	my xmin = xmin;
	my xmax = xmax;
	forget (thee);
	return 1;
}

int TextGrid_setTierName (TextGrid me, long itier, wchar_t *newName)
{
	long ntiers = my tiers -> size;

	if (itier < 1 || itier > ntiers) return
		Melder_error5 (L"TextGrid_renameTier: Tier number (", Melder_integer (itier), L") should not be "
		"larger than the number of tiers (", Melder_integer (ntiers), L").");
	Thing_setName (my tiers -> item [itier], newName);
	return 1;
}

static void IntervalTier_removeInterval (IntervalTier me, long index, int extend_option)
{
	long size_pre = my intervals -> size;

	/* There always must be at least one interval */
	if (size_pre == 1 || index > size_pre || index < 1) return;

	TextInterval ti = my intervals -> item[index];
	double xmin = ti -> xmin;
	double xmax = ti -> xmax;
	Collection_removeItem (my intervals, index);
	if (index == 1) // Change xmin of the new first interval.
	{
		ti = my intervals -> item[index];
		ti -> xmin = xmin;
	}
	else if (index == size_pre) // Change xmax of the new last interval.
	{
		ti = my intervals -> item[my intervals -> size];
		ti -> xmax = xmax;
	}
	else
	{
		if (extend_option == 0) // extend earlier interval to the right
		{
			ti = my intervals -> item[index -1];
			ti -> xmax = xmax;
		}
		else // extend next interval to the left
		{
			ti = my intervals -> item[index];
			ti -> xmin = xmin;
		}
	}
}

void IntervalTier_removeBoundary_minimumDuration (IntervalTier me, wchar_t *label, double minimumDuration)
{
	long i = 1;

	while (i <= my intervals -> size)
	{
		TextInterval ti = my intervals -> item[i];
		if ((label == NULL || (ti -> text != NULL && wcsequ (ti -> text, label))) &&
			ti -> xmax - ti -> xmin < minimumDuration)
		{
			IntervalTier_removeInterval (me, i, 0);
		}
		else
		{
			i++;
		}
	}
}

void IntervalTier_removeBoundary_equalLabels (IntervalTier me, wchar_t *label)
{
	long i = 1;

	while (i < my intervals -> size)
	{
		TextInterval ti = my intervals -> item[i], tip1 = my intervals -> item[i+1];
		if ((label == NULL || (ti -> text != NULL && wcsequ (ti -> text, label))) &&
			(Melder_wcscmp (ti -> text, tip1 -> text) == 0))
		{

			IntervalTier_removeInterval (me, i, 1);
		}
		else
		{
			i++;
		}
	}
}

int IntervalTier_changeLabels (I, long from, long to, wchar_t *search, wchar_t *replace, int use_regexp, long *nmatches, long *nstringmatches)
{
	iam (IntervalTier);
	wchar_t **labels, **newlabels = NULL;
	long i, nlabels, maximumNumberOfReplaces = 0;

	if (from == 0) from = 1;
	if (to == 0) to = my intervals -> size;
	if (from > to || from < 1 || to > my intervals -> size) return 0;
	if (use_regexp && wcslen (search) == 0) return Melder_error1 (L"TextTier_changeLabels: The regex search string may not be empty.\n"
		"You may search for an empty string with the expression \"^$\"");

	nlabels = to - from + 1;
	labels = (wchar_t **) NUMpvector (1, nlabels);
	if (labels == NULL) return 0;

	for (i = from; i <= to; i++)
	{
		TextInterval interval = my intervals -> item[i];
		labels[i - from + 1] = interval -> text;   // Shallow copy.
	}

	newlabels = strs_replace (labels, 1, nlabels, search, replace, maximumNumberOfReplaces, nmatches, nstringmatches, use_regexp);

	if (newlabels == NULL) goto end;

	for (i = from; i <= to; i++)
	{
		TextInterval interval = my intervals -> item[i];
		Melder_free (interval -> text);
		interval -> text = newlabels[i - from + 1];   // Shallow copy.
	}

end:
	NUMpvector_free (newlabels, 1);
	NUMpvector_free (labels, 1);
	return ! Melder_hasError ();
}

int TextTier_changeLabels (I, long from, long to, wchar_t *search, wchar_t *replace, int use_regexp, long *nmatches, long *nstringmatches)
{
	iam (TextTier);
	wchar_t **marks, **newmarks = NULL;
	long i, nmarks, maximumNumberOfReplaces = 0;

	if (from == 0) from = 1;
	if (to == 0) to = my points -> size;
	if (from > to || from < 1 || to > my points -> size) return 0;
	if (use_regexp && wcslen (search) == 0) return Melder_error1 (L"TextTier_changeLabels: The regex search string may not be empty.\n"
		"You may search for an empty string with the expression \"^$\"");

	nmarks = to - from + 1;
	marks = (wchar_t **) NUMpvector (1, nmarks);
	if (marks == NULL) return 0;

	for (i = from; i <= to; i++)
	{
		TextPoint point = my points -> item[i];
		marks[i - from + 1] = point -> mark;   // Shallow copy.
	}

	newmarks = strs_replace (marks, 1, nmarks, search, replace, maximumNumberOfReplaces, nmatches, nstringmatches, use_regexp);

	if (newmarks == NULL) goto end;

	for (i = from; i <= to; i++)
	{
		TextPoint point = my points -> item[i];
		Melder_free (point -> mark);
		point -> mark = newmarks[i - from + 1];   // Shallow copy.
	}

end:
	NUMpvector_free (newmarks, 1);
	NUMpvector_free (marks, 1);
	return ! Melder_hasError ();
}

int TextGrid_changeLabels (TextGrid me, int tier, long from, long to, wchar_t *search, wchar_t *replace, int use_regexp, long *nmatches, long *nstringmatches)
{
	Data anyTier;
	int status;
	long ntiers = my tiers -> size;

	if (tier < 1 || tier > ntiers) return Melder_error
		("TextGrid_changeLabels: The tier number (%d) should not be "
		"larger than the number of tiers (%d).", tier, ntiers);
	if (use_regexp && wcslen (search) == 0) return Melder_error1 (L"TextGrid_changeLabels: The regex search string may not be empty.\n"
		"You may search for an empty string with the expression \"^$\"");
	anyTier = my tiers -> item [tier];
	if (anyTier -> methods == (Data_Table) classIntervalTier)
	{
		status = IntervalTier_changeLabels (anyTier, from, to, search, replace, use_regexp, nmatches, nstringmatches);
	}
	else
	{
		status = TextTier_changeLabels (anyTier, from, to, search, replace, use_regexp, nmatches, nstringmatches);
	}
	return status;
}

/* End of file TextGrid_extensions.c */
