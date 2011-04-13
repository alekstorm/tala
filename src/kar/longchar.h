#ifndef _longchar_h_
#define _longchar_h_
/* longchar.h
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
 * pb 2002/03/07 GPL
 * pb 2002/03/11 nativize & genericize are each other's inverse and their second application is identity
 * pb 2004/12/02 differentiate between X11 and Windows encodings
 * pb 2005/03/08 added "PostScript" encodings (mostly Mac and Praat-IPA)
 * pb 2006/11/17 Unicode
 * pb 2006/12/05 first wchar support
 * pb 2007/08/08 more wchar_t support
 * pb 2007/08/16 removed New Century Schoolbook
 * pb 2008/04/23 updated info
 */
#include <wchar.h>

#ifdef __cplusplus
	extern "C" {
#endif

/********** NON-ASCII CHARACTERS **********/

/* System-independent representation of some non-ASCII symbols.
	These symbols are represented by a backslash (\) plus two ASCII symbols.
	In 1992, we needed this because Praat is multilingual.
	Nowadays, we have Unicode, which is a system-independent representation as well.
	Still, backslash sequences are useful if your computer does not have a fast input method for the symbols
	you want to use, which is likely to happen with phonetic characters.

	- For the characters of the following languages, we supply backslash sequences as well as size and PostScript information:
	  English, Dutch, German, French, Spanish, Portuguese, Italian,
	  Danish, Swedish, Norwegian, Welsh, Luxemburgian, Frisian.
	- For the characters of the following languages, we supply backslash sequences (PostScript will not work yet):
	  Hungarian, Polish, Czech, Rumanian, Icelandic, Serbocroat, Turkish, Greek, Hebrew.
	- By supporting Unicode, we also support other alphabets, such as Arabic, Chinese, Cyrillic, and Devanagari;
	  since we have no backslash support for these, you need a specialized input method to enter them into Praat.
*/

/* Alphabets. */

#define Longchar_ROMAN  0
#define Longchar_SYMBOL  1
#define Longchar_PHONETIC  2
#define Longchar_DINGBATS  3
#define Longchar_RIGHT_TO_LEFT  4

/********** Conversion of Roman native and generic string encodings. **********/

wchar_t * Longchar_nativizeW (const wchar_t *generic, wchar_t *native, int educateQuotes);

char * Longchar_genericize (const char *native, char *generic);
wchar_t *Longchar_genericizeW (const wchar_t *native, wchar_t *generic);
/*
	Function:
		Copies the string 'native' to the string 'generic',
		interpreting non-ASCII native characters as belonging to the Roman alphabet.
	Returns 'generic' as a convenience.
	'generic' can become at most three times as long as 'native'.
	Usage:
		translating user input into a generic string.
*/

typedef struct structLongchar_Info {
	unsigned char first, second;   /* First and second character of two-byte encoding. */
		/* For ASCII characters, 'second' is a space. */
	unsigned char alphabet;   /* Roman, Symbol, Phonetic, or Dingbats. */
	struct {
		const char *name;   /* The PostScript name, starting with a slash. */
		/* The widths in thousands of the height. */
		short times, timesBold, timesItalic, timesBoldItalic;   /* Times. */
		short helvetica, helveticaBold;   /* Helvetica. */
		short palatino, palatinoBold, palatinoItalic, palatinoBoldItalic;   /* Palatino. */
		/* Courier width always 600. */
	}
		ps;   /* PostScript properties. */
	unsigned short xwinEncoding;   /* The one-byte encoding for X11 (ISO8859-1 for Roman). */
	unsigned short winEncoding;   /* The one-byte encoding for Windows (ISO8859-1 for Roman; SILDoulosIPA 1993). */
	unsigned short macEncoding;   /* The one-byte encoding for Macintosh (Mac for Roman; SILDoulosIPA 1993). */
	unsigned short psEncoding;   /* The one-byte encoding for PostScript (Mac-Praat, TeX-xipa-Praat). */
	unsigned long unicode;   /* The four-byte encoding for Unicode. */
	unsigned short unicodeDecomposition [6];   /* Diacritics decoupled from the base character. */
}
	*Longchar_Info;

Longchar_Info Longchar_getInfo (char kar1, char kar2);
Longchar_Info Longchar_getInfoFromNative (wchar_t kar);
/* If no info found, these two routines return the info for a space. */

/*
	How should we represent the dumb ASCII double quote (")?
	The dumb quote is converted to a left or right double quote,
	on Macintosh, PostScript, and Windows
	(ISO8859-1 has no left and right double quotes).
	You can enforce the way your double quotes look by
	using one of these generic symbols:
		- \"l for a left double quote
		- \"r for a right double quote
		- \"" for a straight double quote
	There is no translation for single quotes, because some languages
	use the right single quote as an apostrophe in unpredictable positions.
	You can get a left single quote by typing "`",
	which looks like a grave accent on Macintosh;
	you will get a right single quote or apostrophe by typing a "'",
	which looks like a straight quote on Macintosh.
	Thus, the string typed as "`hallo'" will give you left and right quotes,
	even on Macintosh. (Reading this note in Xwindows may feel somewhat funny.)
*/

#ifdef __cplusplus
	}
#endif

/* End of file longchar.h */
#endif
