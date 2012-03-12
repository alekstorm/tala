/* Strings.c
 *
 * Copyright (C) 1992-2007 Paul Boersma
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
 * pb 2003/07/02 corrected Strings_randomize so that the first element can sometimes go to the first place
 * pb 2004/03/21 Strings_createAsFileList now accepts spaces in directory names on Unix and Mac
 * pb 2004/04/20 the previous thing now done with backslashes rather than double quotes,
 *               because double quotes prevent the expansion of the wildcard asterisk
 * pb 2006/02/14 Strings_createAsDirectoryList for Windows
 * pb 2006/03/08 allow 1,000,000 file names in Strings_createAsFileList
 * pb 2006/09/19 Strings_createAsDirectoryList for Mac and Unix
 * pb 2006/10/04 return fewer errors in Strings_createAsFileList and Strings_createAsDirectoryList
 * pb 2006/10/28 erased MacOS 9 stuff
 * pb 2006/12/10 MelderInfo
 * pb 2007/01/24 Strings_createAsFileList: removed gigantic memory leak
 * pb 2007/01/24 Strings_createAsFileList: used stat instead of platform-specific struct dirent. entry
 * pb 2007/08/10 wchar_t
 * pb 2007/10/01 can write as encoding
 * pb 2007/10/01 corrected nativization
 * pb 2007/11/17 getVectorStr
 * pb 2007/12/10 Strings_createAsFileList precomposes characters
 */

//#define USE_STAT  1
#ifndef USE_STAT
	#if defined (_WIN32)
		#define USE_STAT  0
	#else
		#define USE_STAT  1
	#endif
#endif

#include "Strings.h"
#include "kar/longchar.h"
#if USE_STAT
	#include <sys/types.h>
	//#define __USE_BSD
	#include <sys/stat.h>
	#include <dirent.h>
#endif
#if defined (_WIN32)
	#include <windows.h>
#endif

long Strings::totalLength () {
	long totalLength = 0, i;
	for (i = 1; i <= _numberOfStrings; i ++) {
		totalLength += wcslen (_strings [i]);
	}
	return totalLength;
}

long Strings::maximumLength () {
	long maximumLength = 0, i;
	for (i = 1; i <= _numberOfStrings; i ++) {
		long length = wcslen (_strings [i]);
		if (length > maximumLength) {
			maximumLength = length;
		}
	}
	return maximumLength;
}

void Strings::info () {
	Data::info ();
	MelderInfo_writeLine2 (L"Number of strings: ", Melder_integer (_numberOfStrings));
	MelderInfo_writeLine3 (L"Total length: ", Melder_integer (totalLength ()), L" characters");
	MelderInfo_writeLine3 (L"Longest string: ", Melder_integer (maximumLength ()), L" characters");
}

const wchar * Strings::getVectorStr (long icol) {
	wchar_t *stringValue;
	if (icol < 1 || icol > _numberOfStrings) return L"";
	stringValue = _strings [icol];
	return stringValue == NULL ? L"" : stringValue;
}

Strings::Strings (const wchar_t *path, bool fileList) {
	#if USE_STAT
		/*
		 * Initialize.
		 */
		DIR *d = NULL;
		MelderString filePath = { 0 }, searchDirectory = { 0 }, left = { 0 }, right = { 0 };
		/*
		 * Parse the path.
		 * Example: in /Users/paul/sounds/h*.wav",
		 * the search directory is "/Users/paul/sounds",
		 * the left environment is "h", and the right environment is ".wav".
		 */
		MelderString_copy (& searchDirectory, path);
		wchar_t *asterisk = wcsrchr (searchDirectory. string, '*');
		if (asterisk != NULL) {
			*asterisk = '\0';
			searchDirectory. length = asterisk - searchDirectory. string;   // Probably superfluous, but correct.
			wchar_t *lastSlash = wcsrchr (searchDirectory. string, Melder_DIRECTORY_SEPARATOR);
			if (lastSlash != NULL) {
				*lastSlash = '\0';   // This fixes searchDirectory.
				searchDirectory. length = lastSlash - searchDirectory. string;   // Probably superfluous, but correct.
				MelderString_copy (& left, lastSlash + 1);
			} else {
				MelderString_copy (& left, searchDirectory. string);   /* Quickly save... */
				MelderString_empty (& searchDirectory);   /* ...before destruction. */
			}
			MelderString_copy (& right, asterisk + 1);
		}
		char buffer8 [1000];
		Melder_wcsTo8bitFileRepresentation_inline (searchDirectory. string, buffer8);
		d = opendir (buffer8 [0] ? buffer8 : ".");
		if (d == NULL) error3 (L"Cannot open directory ", searchDirectory. string, L".")
		//Melder_casual ("opened");
		_strings = (wchar_t**)NUMpvector (1, 1000000); cherror
		struct dirent *entry;
		while ((entry = readdir (d)) != NULL) {
			MelderString_copy (& filePath, searchDirectory. string [0] ? searchDirectory. string : L".");
			MelderString_appendCharacter (& filePath, Melder_DIRECTORY_SEPARATOR);
			wchar_t bufferW [1000];
			Melder_8bitFileRepresentationToWcs_inline (entry -> d_name, bufferW);
			MelderString_append (& filePath, bufferW);
			//Melder_casual ("read %s", filePath. string);
			Melder_wcsTo8bitFileRepresentation_inline (filePath. string, buffer8);
			struct stat stats;
			if (stat (buffer8, & stats) != 0) {
				error3 (L"Cannot look at file ", filePath. string, L".")
				//stats. st_mode = -1L;
			}
			//Melder_casual ("statted %s", filePath. string);
			//Melder_casual ("file %s mode %s", filePath. string, Melder_integer (stats. st_mode / 4096));
			if ((fileList && S_ISREG (stats. st_mode)) ||
				(!fileList && S_ISDIR (stats. st_mode)))
			{
				Melder_8bitFileRepresentationToWcs_inline (entry -> d_name, bufferW);
				unsigned long length = wcslen (bufferW);
				if (bufferW [0] != '.' &&
					(left. length == 0 || wcsnequ (bufferW, left. string, left. length)) &&
					(right. length == 0 || (length >= right. length && wcsequ (bufferW + (length - right. length), right. string))))
				{
					_strings [++ _numberOfStrings] = Melder_wcsdup_e (bufferW); cherror
				}
			}
		}
	#elif defined (_WIN32)
		HANDLE searchHandle;
		WIN32_FIND_DATAW findData;
		wchar_t searchPath [300];
		int len = wcslen (path), hasAsterisk = wcschr (path, '*') != NULL, endsInSeparator = len != 0 && path [len - 1] == '\\';
		_strings = NUMpvector (1, 1000000); cherror
		swprintf (searchPath, 300, L"%ls%ls%ls", path, hasAsterisk || endsInSeparator ? L"" : L"\\", hasAsterisk ? L"" : L"*");
		searchHandle = FindFirstFileW (searchPath, & findData);
		if (searchHandle != INVALID_HANDLE_VALUE) {
			do {
				if ((fileList &&
						(findData. dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
				 || (!fileList &&
						(findData. dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0))
				{
					_strings [++ _numberOfStrings] = Melder_wcsdup_e (findData. cFileName); cherror
				}
			} while (FindNextFileW (searchHandle, & findData));
			FindClose (searchHandle);
		}
	#endif
end:
	#if USE_STAT
		if (d) closedir (d);
		MelderString_free (& filePath);
		MelderString_free (& searchDirectory);
		MelderString_free (& left);
		MelderString_free (& right);
	#endif
}

Strings::Strings (MelderFile file) {
	MelderReadText text = MelderReadText_createFromFile (file); cherror
	{
		/*
		 * Count number of strings.
		 */
		long n = MelderReadText_getNumberOfLines (text);

		/*
		 * Create.
		 */
		if (n > 0) _strings = (wchar_t**)NUMpvector (1, n); cherror
		_numberOfStrings = n;

		/*
		 * Read strings.
		 */
		for (long i = 1; i <= n; i ++) {
			wchar_t *line = MelderReadText_readLine (text); cherror
			_strings [i] = Melder_wcsdup_e (line); cherror
		}
	}

end:
	MelderReadText_delete (text);
	iferror {
		Melder_error3 (L"(Strings_readFromRawTextFile:) File ", MelderFile_messageName (file), L" not read.");
	}
}

int Strings::writeToRawTextFile (MelderFile file) {
	MelderString buffer = { 0 };
	for (long i = 1; i <= _numberOfStrings; i ++) {
		MelderString_append2 (& buffer, _strings [i], L"\n");
	}
	MelderFile_writeText (file, buffer.string);
end:
	MelderString_free (& buffer);
	iferror return 0;
	return 1;
}

void Strings::randomize () {
	for (long i = 1; i < _numberOfStrings; i ++) {
		long other = NUMrandomInteger (i, _numberOfStrings);
		wchar_t *dummy = _strings [other];
		_strings [other] = _strings [i];
		_strings [i] = dummy;
	}
}

int Strings::genericize () {
	wchar_t *buffer = NULL;
	buffer = Melder_calloc_e (wchar_t, maximumLength () * 3 + 1); cherror
	for (long i = 1; i <= _numberOfStrings; i ++) {
		const wchar_t *p = (const wchar_t *) _strings [i];
		while (*p) {
			if (*p > 126) {   /* Backslashes are not converted, i.e. genericize^2 == genericize. */
				wchar_t *newString;
				Longchar_genericizeW (_strings [i], buffer);
				newString = Melder_wcsdup_e (buffer); cherror
				/*
				 * Replace string only if copying was OK.
				 */
				Melder_free (_strings [i]);
				_strings [i] = newString;
				break;
			}
			p ++;
		}
	}
end:
	Melder_free (buffer);
	iferror return 0;
	return 1;
}

int Strings::nativize () {
	wchar_t *buffer = NULL;
	buffer = Melder_calloc_e (wchar_t, maximumLength () + 1); cherror
	for (long i = 1; i <= _numberOfStrings; i ++) {
		Longchar_nativizeW (_strings [i], buffer, false);
		wchar_t *newString = Melder_wcsdup_e (buffer); cherror
		/*
		 * Replace string only if copying was OK.
		 */
		Melder_free (_strings [i]);
		_strings [i] = newString;
	}
end:
	Melder_free (buffer);
	iferror return 0;
	return 1;
}

void Strings::sort () {
	NUMsort_str (_numberOfStrings, _strings);
}

void Strings::remove (long position) {
	Melder_assert (position >= 1);
	Melder_assert (position <= _numberOfStrings);
	Melder_free (_strings [position]);
	for (long i = position; i < _numberOfStrings; i ++) {
		_strings [i] = _strings [i + 1];
	}
	_numberOfStrings --;
}

int Strings::replace (long position, const wchar_t *text) {
	Melder_assert (position >= 1);
	Melder_assert (position <= _numberOfStrings);
	if (Melder_wcsequ (_strings [position], text)) return 1;
	Melder_free (_strings [position]);
	_strings [position] = Melder_wcsdup_e (text); cherror
end:
	iferror return 0;
	return 1;
}

int Strings::insert (long position, const wchar_t *text) {
	if (position == 0) position = _numberOfStrings + 1;
	Melder_assert (position >= 1);
	Melder_assert (position <= _numberOfStrings + 1);
	for (long i = _numberOfStrings + 1; i > position; i --) {
		_strings [i] = _strings [i - 1];
	}
	_strings [position] = Melder_wcsdup_e (text); cherror
	_numberOfStrings ++;
end:
	iferror return 0;
	return 1;
}

/* End of file Strings.c */
