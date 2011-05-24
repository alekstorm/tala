/* melder.c
 *
 * Copyright (C) 1992-2010 Paul Boersma
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
 * pb 2002/03/07 GPL
 * pb 2002/03/13 Mach
 * pb 2002/12/11 MelderInfo
 * pb 2003/12/29 Melder_warning: added XMapRaised because delete response is UNMAP
 * pb 2004/04/06 motif_information drains text window only, i.e. no longer updates all windows
                 (which used to cause up to seven seconds of delay in a 1-second sound window)
 * pb 2004/10/24 info buffer can grow
 * pb 2004/11/28 author notification in Melder_fatal
 * pb 2005/03/04 number and string comparisons, including regular expressions
 * pb 2005/06/16 removed enums from number and string comparisons (ints give no compiler warnings)
 * pb 2005/07/19 Melder_stringMatchesCriterion: regard NULL criterion as empty string
 * pb 2007/05/24 more wchar_t
 * pb 2007/05/26 Melder_stringMatchesCriterionW
 * pb 2007/06/19 removed some
 * pb 2007/08/12 wchar_t in helpProc
 * pb 2007/12/02 enums
 * pb 2007/12/13 Melder_writeToConsole
 * pb 2007/12/18 Gui
 * sdk 2008/01/22 GTK
 * pb 2009/01/20 removed pause
 * fb 2010/02/26 GTK
 * pb 2010/06/22 GTK: correct hiding and showing again
 * pb 2010/07/29 removed GuiDialog_show
 * pb 2010/11/26 even Unix now has a GUI fatal window
 * pb 2010/12/30 messageFund
 */

#include <math.h>
#include <time.h>
#include <ctype.h>
#include <assert.h>
#include "melder.h"
#include "kar/longchar.h"
#include "dwsys/regularExp.h"
#ifdef _WIN32
	#include <windows.h>
#endif
#if defined (macintosh)
	#include "sys/macport_on.h"
	//#include <Sound.h>
	#include "sys/macport_off.h"
#endif

#include "sys/enums_getText.h"
#include "melder_enums.h"
#include "sys/enums_getValue.h"
#include "melder_enums.h"

/********** Exported variables. **********/

char Melder_buffer1 [30001], Melder_buffer2 [30001];
unsigned long Melder_systemVersion;

static void defaultHelp (const wchar_t *query) {
	Melder_error3 (L"Do not know how to find help on \"", query, L"\".");
	Melder_flushError (NULL);
}

static void defaultSearch (void) {
	Melder_flushError ("Do not know how to search.");
}

static void defaultWarning (wchar_t *message) {
	Melder_writeToConsole (L"Warning: ", true);
	Melder_writeToConsole (message, true);
	Melder_writeToConsole (L"\n", true);
}

static void defaultFatal (wchar_t *message) {
	Melder_writeToConsole (L"Fatal error: ", true);
	Melder_writeToConsole (message, true);
	Melder_writeToConsole (L"\n", true);
}

static int defaultPublish (void *anything) {
	(void) anything;
	return 0;   /* Nothing published. */
}

static int defaultRecord (double duration) {
	(void) duration;
	return 0;   /* Nothing recorded. */
}

static int defaultRecordFromFile (MelderFile file) {
	(void) file;
	return 0;   /* Nothing recorded. */
}

static void defaultPlay (void) {}

static void defaultPlayReverse (void) {}

static int defaultPublishPlayed (void) {
	return 0;   /* Nothing published. */
}

/********** Current message methods: initialize to default (batch) behaviour. **********/

static struct {
	void (*help) (const wchar_t *query);
	void (*search) (void);
	void (*warning) (wchar_t *message);
	void (*fatal) (wchar_t *message);
	int (*publish) (void *anything);
	int (*record) (double duration);
	int (*recordFromFile) (MelderFile fs);
	void (*play) (void);
	void (*playReverse) (void);
	int (*publishPlayed) (void);
}
	theMelder = {
		defaultHelp, defaultSearch,
		defaultWarning, defaultFatal,
		defaultPublish,
		defaultRecord, defaultRecordFromFile, defaultPlay, defaultPlayReverse, defaultPublishPlayed
	};

/********** CASUAL **********/

void Melder_casual (const char *format, ...) {
	va_list arg;
	va_start (arg, format);
	vsprintf (Melder_buffer1, format, arg);
	fprintf (stderr, "%s\n", Melder_buffer1);
	va_end (arg);
}

/********** PROGRESS **********/

static MelderString theProgressBuffer = { 0 };

int Melder_progress1 (double progress, const wchar_t *s1) {
	MelderString_empty (& theProgressBuffer);
	MelderString_append1 (& theProgressBuffer, s1);
	//return _Melder_progress (progress, theProgressBuffer.string); // FIXME
}
int Melder_progress2 (double progress, const wchar_t *s1, const wchar_t *s2) {
	MelderString_empty (& theProgressBuffer);
	MelderString_append2 (& theProgressBuffer, s1, s2);
	//return _Melder_progress (progress, theProgressBuffer.string);
}
int Melder_progress3 (double progress, const wchar_t *s1, const wchar_t *s2, const wchar_t *s3) {
	MelderString_empty (& theProgressBuffer);
	MelderString_append3 (& theProgressBuffer, s1, s2, s3);
	//return _Melder_progress (progress, theProgressBuffer.string);
}
int Melder_progress4 (double progress, const wchar_t *s1, const wchar_t *s2, const wchar_t *s3, const wchar_t *s4) {
	MelderString_empty (& theProgressBuffer);
	MelderString_append4 (& theProgressBuffer, s1, s2, s3, s4);
	//return _Melder_progress (progress, theProgressBuffer.string);
}
int Melder_progress5 (double progress, const wchar_t *s1, const wchar_t *s2, const wchar_t *s3, const wchar_t *s4, const wchar_t *s5) {
	MelderString_empty (& theProgressBuffer);
	MelderString_append5 (& theProgressBuffer, s1, s2, s3, s4, s5);
	//return _Melder_progress (progress, theProgressBuffer.string);
}
int Melder_progress6 (double progress, const wchar_t *s1, const wchar_t *s2, const wchar_t *s3, const wchar_t *s4, const wchar_t *s5, const wchar_t *s6) {
	MelderString_empty (& theProgressBuffer);
	MelderString_append6 (& theProgressBuffer, s1, s2, s3, s4, s5, s6);
	//return _Melder_progress (progress, theProgressBuffer.string);
}
int Melder_progress7 (double progress, const wchar_t *s1, const wchar_t *s2, const wchar_t *s3, const wchar_t *s4, const wchar_t *s5, const wchar_t *s6, const wchar_t *s7) {
	MelderString_empty (& theProgressBuffer);
	MelderString_append7 (& theProgressBuffer, s1, s2, s3, s4, s5, s6, s7);
	//return _Melder_progress (progress, theProgressBuffer.string);
}
int Melder_progress8 (double progress, const wchar_t *s1, const wchar_t *s2, const wchar_t *s3, const wchar_t *s4, const wchar_t *s5, const wchar_t *s6, const wchar_t *s7, const wchar_t *s8) {
	MelderString_empty (& theProgressBuffer);
	MelderString_append8 (& theProgressBuffer, s1, s2, s3, s4, s5, s6, s7, s8);
	//return _Melder_progress (progress, theProgressBuffer.string);
}
int Melder_progress9 (double progress, const wchar_t *s1, const wchar_t *s2, const wchar_t *s3, const wchar_t *s4, const wchar_t *s5, const wchar_t *s6, const wchar_t *s7, const wchar_t *s8, const wchar_t *s9) {
	MelderString_empty (& theProgressBuffer);
	MelderString_append9 (& theProgressBuffer, s1, s2, s3, s4, s5, s6, s7, s8, s9);
	//return _Melder_progress (progress, theProgressBuffer.string);
}

void * Melder_monitor1 (double progress, const wchar_t *s1) {
	MelderString_empty (& theProgressBuffer);
	MelderString_append1 (& theProgressBuffer, s1);
	//return _Melder_monitor (progress, theProgressBuffer.string); // FIXME
}
void * Melder_monitor2 (double progress, const wchar_t *s1, const wchar_t *s2) {
	MelderString_empty (& theProgressBuffer);
	MelderString_append2 (& theProgressBuffer, s1, s2);
	//return _Melder_monitor (progress, theProgressBuffer.string);
}
void * Melder_monitor3 (double progress, const wchar_t *s1, const wchar_t *s2, const wchar_t *s3) {
	MelderString_empty (& theProgressBuffer);
	MelderString_append3 (& theProgressBuffer, s1, s2, s3);
	//return _Melder_monitor (progress, theProgressBuffer.string);
}
void * Melder_monitor4 (double progress, const wchar_t *s1, const wchar_t *s2, const wchar_t *s3, const wchar_t *s4) {
	MelderString_empty (& theProgressBuffer);
	MelderString_append4 (& theProgressBuffer, s1, s2, s3, s4);
	//return _Melder_monitor (progress, theProgressBuffer.string);
}
void * Melder_monitor5 (double progress, const wchar_t *s1, const wchar_t *s2, const wchar_t *s3, const wchar_t *s4, const wchar_t *s5) {
	MelderString_empty (& theProgressBuffer);
	MelderString_append5 (& theProgressBuffer, s1, s2, s3, s4, s5);
	//return _Melder_monitor (progress, theProgressBuffer.string);
}
void * Melder_monitor6 (double progress, const wchar_t *s1, const wchar_t *s2, const wchar_t *s3, const wchar_t *s4, const wchar_t *s5, const wchar_t *s6) {
	MelderString_empty (& theProgressBuffer);
	MelderString_append6 (& theProgressBuffer, s1, s2, s3, s4, s5, s6);
	//return _Melder_monitor (progress, theProgressBuffer.string);
}
void * Melder_monitor7 (double progress, const wchar_t *s1, const wchar_t *s2, const wchar_t *s3, const wchar_t *s4, const wchar_t *s5, const wchar_t *s6, const wchar_t *s7) {
	MelderString_empty (& theProgressBuffer);
	MelderString_append7 (& theProgressBuffer, s1, s2, s3, s4, s5, s6, s7);
	//return _Melder_monitor (progress, theProgressBuffer.string);
}
void * Melder_monitor8 (double progress, const wchar_t *s1, const wchar_t *s2, const wchar_t *s3, const wchar_t *s4, const wchar_t *s5, const wchar_t *s6, const wchar_t *s7, const wchar_t *s8) {
	MelderString_empty (& theProgressBuffer);
	MelderString_append8 (& theProgressBuffer, s1, s2, s3, s4, s5, s6, s7, s8);
	//return _Melder_monitor (progress, theProgressBuffer.string);
}
void * Melder_monitor9 (double progress, const wchar_t *s1, const wchar_t *s2, const wchar_t *s3, const wchar_t *s4, const wchar_t *s5, const wchar_t *s6, const wchar_t *s7, const wchar_t *s8, const wchar_t *s9) {
	MelderString_empty (& theProgressBuffer);
	MelderString_append9 (& theProgressBuffer, s1, s2, s3, s4, s5, s6, s7, s8, s9);
	//return _Melder_monitor (progress, theProgressBuffer.string);
}

/********** NUMBER AND STRING COMPARISONS **********/

int Melder_numberMatchesCriterion (double value, int which_kMelder_number, double criterion) {
	return
		(which_kMelder_number == kMelder_number_EQUAL_TO && value == criterion) ||
		(which_kMelder_number == kMelder_number_NOT_EQUAL_TO && value != criterion) ||
		(which_kMelder_number == kMelder_number_LESS_THAN && value < criterion) ||
		(which_kMelder_number == kMelder_number_LESS_THAN_OR_EQUAL_TO && value <= criterion) ||
		(which_kMelder_number == kMelder_number_GREATER_THAN && value > criterion) ||
		(which_kMelder_number == kMelder_number_GREATER_THAN_OR_EQUAL_TO && value >= criterion);
}

int Melder_stringMatchesCriterion (const wchar_t *value, int which_kMelder_string, const wchar_t *criterion) {
	if (value == NULL) {
		value = L"";   /* Regard null strings as empty strings, as is usual in Praat. */
	}
	if (criterion == NULL) {
		criterion = L"";   /* Regard null strings as empty strings, as is usual in Praat. */
	}
	if (which_kMelder_string <= kMelder_string_NOT_EQUAL_TO) {
		int matchPositiveCriterion = wcsequ (value, criterion);
		return (which_kMelder_string == kMelder_string_EQUAL_TO) == matchPositiveCriterion;
	}
	if (which_kMelder_string <= kMelder_string_DOES_NOT_CONTAIN) {
		int matchPositiveCriterion = wcsstr (value, criterion) != NULL;
		return (which_kMelder_string == kMelder_string_CONTAINS) == matchPositiveCriterion;
	}
	if (which_kMelder_string <= kMelder_string_DOES_NOT_START_WITH) {
		int matchPositiveCriterion = wcsnequ (value, criterion, wcslen (criterion));
		return (which_kMelder_string == kMelder_string_STARTS_WITH) == matchPositiveCriterion;
	}
	if (which_kMelder_string <= kMelder_string_DOES_NOT_END_WITH) {
		int criterionLength = wcslen (criterion), valueLength = wcslen (value);
		int matchPositiveCriterion = criterionLength <= valueLength && wcsequ (value + valueLength - criterionLength, criterion);
		return (which_kMelder_string == kMelder_string_ENDS_WITH) == matchPositiveCriterion;
	}
	if (which_kMelder_string == kMelder_string_MATCH_REGEXP) {
		wchar_t *place = NULL, *errorMessage;
		regexp *compiled_regexp = CompileRE (criterion, & errorMessage, 0);
		if (compiled_regexp == NULL) return FALSE;   // BUG: what about removing errorMessage?
		if (ExecRE (compiled_regexp, NULL, value, NULL, 0, '\0', '\0', NULL, NULL, NULL))
			place = compiled_regexp -> startp [0];
		free (compiled_regexp);
		return place != NULL;
	}
	return 0;   /* Should not occur. */
}

void Melder_help (const wchar_t *query) {
	theMelder. help (query);
}

void Melder_search (void) {
	theMelder. search ();
}

/********** WARNING **********/

static int theWarningDepth = 0;
void Melder_warningOff (void) { theWarningDepth --; }
void Melder_warningOn (void) { theWarningDepth ++; }

static MelderString theWarningBuffer = { 0 };

void Melder_warning1 (const wchar_t *s1) {
	if (theWarningDepth < 0) return;
	MelderString_empty (& theWarningBuffer);
	MelderString_append1 (& theWarningBuffer, s1);
	theMelder. warning (theWarningBuffer.string);
}
void Melder_warning2 (const wchar_t *s1, const wchar_t *s2) {
	if (theWarningDepth < 0) return;
	MelderString_empty (& theWarningBuffer);
	MelderString_append2 (& theWarningBuffer, s1, s2);
	theMelder. warning (theWarningBuffer.string);
}
void Melder_warning3 (const wchar_t *s1, const wchar_t *s2, const wchar_t *s3) {
	if (theWarningDepth < 0) return;
	MelderString_empty (& theWarningBuffer);
	MelderString_append3 (& theWarningBuffer, s1, s2, s3);
	theMelder. warning (theWarningBuffer.string);
}
void Melder_warning4 (const wchar_t *s1, const wchar_t *s2, const wchar_t *s3, const wchar_t *s4) {
	if (theWarningDepth < 0) return;
	MelderString_empty (& theWarningBuffer);
	MelderString_append4 (& theWarningBuffer, s1, s2, s3, s4);
	theMelder. warning (theWarningBuffer.string);
}
void Melder_warning5 (const wchar_t *s1, const wchar_t *s2, const wchar_t *s3, const wchar_t *s4, const wchar_t *s5) {
	if (theWarningDepth < 0) return;
	MelderString_empty (& theWarningBuffer);
	MelderString_append5 (& theWarningBuffer, s1, s2, s3, s4, s5);
	theMelder. warning (theWarningBuffer.string);
}
void Melder_warning6 (const wchar_t *s1, const wchar_t *s2, const wchar_t *s3, const wchar_t *s4, const wchar_t *s5, const wchar_t *s6) {
	if (theWarningDepth < 0) return;
	MelderString_empty (& theWarningBuffer);
	MelderString_append6 (& theWarningBuffer, s1, s2, s3, s4, s5, s6);
	theMelder. warning (theWarningBuffer.string);
}
void Melder_warning7 (const wchar_t *s1, const wchar_t *s2, const wchar_t *s3, const wchar_t *s4, const wchar_t *s5, const wchar_t *s6, const wchar_t *s7) {
	if (theWarningDepth < 0) return;
	MelderString_empty (& theWarningBuffer);
	MelderString_append7 (& theWarningBuffer, s1, s2, s3, s4, s5, s6, s7);
	theMelder. warning (theWarningBuffer.string);
}
void Melder_warning8 (const wchar_t *s1, const wchar_t *s2, const wchar_t *s3, const wchar_t *s4, const wchar_t *s5, const wchar_t *s6, const wchar_t *s7, const wchar_t *s8) {
	if (theWarningDepth < 0) return;
	MelderString_empty (& theWarningBuffer);
	MelderString_append8 (& theWarningBuffer, s1, s2, s3, s4, s5, s6, s7, s8);
	theMelder. warning (theWarningBuffer.string);
}
void Melder_warning9 (const wchar_t *s1, const wchar_t *s2, const wchar_t *s3, const wchar_t *s4, const wchar_t *s5, const wchar_t *s6, const wchar_t *s7, const wchar_t *s8, const wchar_t *s9) {
	if (theWarningDepth < 0) return;
	MelderString_empty (& theWarningBuffer);
	MelderString_append9 (& theWarningBuffer, s1, s2, s3, s4, s5, s6, s7, s8, s9);
	theMelder. warning (theWarningBuffer.string);
}

void Melder_beep (void) {
	#ifdef macintosh
		SysBeep (0);
	#else
		fprintf (stderr, "\a");
	#endif
}

/*********** FATAL **********/

int Melder_fatal (const char *format, ...) {
	const char *lead = strstr (format, "Praat cannot start up") ? "" :
		"Praat will crash. Notify the author (paul.boersma@uva.nl) with the following information:\n";
	va_list arg;
	va_start (arg, format);
	strcpy (Melder_buffer1, lead);
	vsprintf (Melder_buffer1 + strlen (lead), format, arg);
	theMelder. fatal (Melder_peekUtf8ToWcs (Melder_buffer1));
	va_end (arg);
	abort ();
	return 0;   /* Make some compilers happy, some unhappy. */
}

int _Melder_assert (const char *condition, const char *fileName, int lineNumber) {
	return Melder_fatal ("Assertion failed in file \"%s\" at line %d:\n   %s\n",
		fileName, lineNumber, condition);
}

int Melder_publish (void *anything) {
	return theMelder. publish (anything);
}

int Melder_record (double duration) {
	return theMelder. record (duration);
}

int Melder_recordFromFile (MelderFile file) {
	return theMelder. recordFromFile (file);
}

void Melder_play (void) {
	theMelder. play ();
}

void Melder_playReverse (void) {
	theMelder. playReverse ();
}

int Melder_publishPlayed (void) {
	return theMelder. publishPlayed ();
}

/********** Procedures to override message methods (e.g., to enforce interactive behaviour). **********/

void Melder_setHelpProc (void (*help) (const wchar_t *query))
	{ theMelder. help = help ? help : defaultHelp; }

void Melder_setSearchProc (void (*search) (void))
	{ theMelder. search = search ? search : defaultSearch; }

void Melder_setWarningProc (void (*warning) (wchar_t *))
	{ theMelder. warning = warning ? warning : defaultWarning; }

void Melder_setFatalProc (void (*fatal) (wchar_t *))
	{ theMelder. fatal = fatal ? fatal : defaultFatal; }

void Melder_setPublishProc (int (*publish) (void *))
	{ theMelder. publish = publish ? publish : defaultPublish; }

void Melder_setRecordProc (int (*record) (double))
	{ theMelder. record = record ? record : defaultRecord; }

void Melder_setRecordFromFileProc (int (*recordFromFile) (MelderFile))
	{ theMelder. recordFromFile = recordFromFile ? recordFromFile : defaultRecordFromFile; }

void Melder_setPlayProc (void (*play) (void))
	{ theMelder. play = play ? play : defaultPlay; }

void Melder_setPlayReverseProc (void (*playReverse) (void))
	{ theMelder. playReverse = playReverse ? playReverse : defaultPlayReverse; }

void Melder_setPublishPlayedProc (int (*publishPlayed) (void))
	{ theMelder. publishPlayed = publishPlayed ? publishPlayed : defaultPublishPlayed; }

/* End of file melder.c */
