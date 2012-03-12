/* Data.c
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
 * pb 2002/03/07 GPL
 * pb 2003/07/02 Data_copy returns NULL if argument is NULL
 * pb 2003/09/14 old ClassTextFile formant readable across systems
 * pb 2004/10/16 C++ compatible struct tags
 * pb 2007/06/21 wchar_t
 * pb 2007/07/21 Data_canWriteAsEncoding
 * pb 2008/01/18 guarded against some crashes (-> Data me = NULL)
 * pb 2008/07/20 wchar_t
 * pb 2008/08/13 prevented overriding of header in file recognition
 */

#include "Collection.h"

structMelderDir Data_directoryBeingRead = { { 0 } };

int Data::copyTo (Data *to) {
	(void) to;
	if (! canCopy()) return (int)Melder_errorp3 (L"(Data_copy:) Class ", className(), L" cannot be copied.");
	return 1;
}

int Data::writeText (MelderFile openFile) {
	(void) openFile;
	return 1;
}

int Data::readText (MelderReadText text) {
	(void) text;
	return 1;
}

int Data::writeBinary (FILE *f) {
	(void) f;
	return 1;
}

int Data::readBinary (FILE *f) {
	(void) f;
	return 1;
}

int Data::writeCache (CACHE *f) {
	(void) f;
	return 1;
}

int Data::readCache (CACHE *f) {
	(void) f;
	return 1;
}

int Data::writeLisp (FILE *f) {
	(void) f;
	return 1;
}

int Data::readLisp (FILE *f) {
	(void) f;
	return 1;
}

Data *Data::copy () {
	Data *other = (Data *)Thing::newFromClassName(className());
	if (! copyTo(other)) {
		forget (other);
		return (Data *)Melder_errorp3 (L"(Data_copy:) Object of class ", className(), L" not copied.");
	}
	other->setName(name);
	return other;
}

bool Data::equal (Data *other) {
	return className() == other->className();
}

bool Data::canCopy () {
	return true;
}

bool Data::canWriteText () {
	/*return our writeText != classData -> writeText;*/ // FIXME
	return false;
}

int Data::createTextFile (MelderFile file, bool verbose) {
	MelderFile_create (file, L"TEXT", 0, L"txt"); cherror
	#if defined (_WIN32)
		file -> requiresCRLF = true;
	#endif
	file -> verbose = verbose;
	file -> outputEncoding = Melder_getOutputEncoding ();
	if (file -> outputEncoding == kMelder_textOutputEncoding_ASCII_THEN_UTF16)
		file -> outputEncoding = canWriteAsEncoding (kMelder_textOutputEncoding_ASCII) ? kMelder_textOutputEncoding_ASCII : kMelder_textOutputEncoding_UTF16;
	else if (file -> outputEncoding == kMelder_textOutputEncoding_ISO_LATIN1_THEN_UTF16)
		file -> outputEncoding = canWriteAsEncoding (kMelder_textOutputEncoding_ISO_LATIN1) ? kMelder_textOutputEncoding_ISO_LATIN1 : kMelder_textOutputEncoding_UTF16;
	if (file -> outputEncoding == kMelder_textOutputEncoding_UTF16)
		binputu2 (0xfeff, file -> filePointer);
end:
	iferror return 0;
	return 1;
}

int Data::writeToTextFile (MelderFile file, bool verbose) {
	if (! canWriteText ()) error3 (L"(Data_writeToTextFile:) Objects of class ", className(), L" cannot be written to a text file.")
	createTextFile (file, verbose); cherror
	#ifndef _WIN32
		flockfile (file -> filePointer);   // BUG
	#endif
	MelderFile_write2 (file, L"File type = \"sys/oo/ooTextFile\"\nObject class = \"", className());
	if (version > 0) MelderFile_write2 (file, L" ", Melder_integer (Thing::version));
	MelderFile_write1 (file, L"\"\n");
	writeText (file); cherror
	MelderFile_writeCharacter (file, '\n');
end:
	#ifndef _WIN32
		if (file -> filePointer) funlockfile (file -> filePointer);
	#endif
	MelderFile_close (file);
	iferror return Melder_error5 (L"Cannot write ", className(), L"to file ", MelderFile_messageName (file), L".");
	MelderFile_setMacTypeAndCreator (file, 'TEXT', 0);
	return 1;
}

bool Data::canWriteBinary () {
	/*return our writeBinary != classData -> writeBinary;*/ // FIXME
	return false;
}

int Data::writeToBinaryFile (MelderFile file) {
	if (! canWriteBinary ()) error3 (L"(Data_writeToBinaryFile:) Objects of class ", className(), L" cannot be written to a generic binary file.")
	MelderFile_create (file, 0, 0, 0); cherror
	if (fprintf (file -> filePointer, "sys/oo/ooBinaryFile") < 0)
		error1 (L"Cannot write first bytes of file.")
	binputw1 (Thing::version > 0 ? Melder_wcscat3 (className(), L" ", Melder_integer (Thing::version)) : className(), file -> filePointer);
	writeBinary (file -> filePointer);
end:
	MelderFile_close (file);
	iferror return Melder_error3 (L"(Data_writeToBinaryFile:) Cannot write file ", MelderFile_messageName (file), L".");
	MelderFile_setMacTypeAndCreator (file, 'BINA', 0);
	return 1;
}

bool Data::canWriteLisp () {
	/*return our writeLisp != classData -> writeLisp; */ // FIXME
	return false;
}

int Data::writeLispToConsole () {
	if (! canWriteLisp ()) return (int)Melder_error3 (L"(Data_writeLispToConsole:) Class ", className(), L" cannot be written as LISP.");
	wprintf (L"Write as LISP sequence to console: class %ls,  name \"%ls\".\n", className(), name ? name : L"<none>");
	return writeLisp (stdout);
}

int Data::writeToLispFile (MelderFile file) {
	FILE *f;
	if (! canWriteLisp ()) return Melder_error3 (L"(Data_writeToLispFile:) Class ", className(), L" cannot be written as LISP.");
	if ((f = Melder_fopen (file, "w")) == NULL) return 0;
	if (fprintf (f, "%sLispFile\n", Melder_peekWcsToUtf8 (className())) == EOF || ! writeLisp (f)) {
		fclose (f);
		return Melder_error3 (L"(Data_writeToLispFile:) Error while writing file ",
			MelderFile_messageName (file), L". Disk probably full.");
	}
	fclose (f);
	MelderFile_setMacTypeAndCreator (file, 'TEXT', 0);
	return 1;
}

bool Data::canReadText () {
	/*return our readText != classData -> readText;*/ // FIXME
	return false;
}

Data *Data::readFromTextFile (MelderFile file) {
	Data *data = NULL;
	wchar_t *klas = NULL;
	MelderReadText text = NULL;
	text = MelderReadText_createFromFile (file); cherror
	{
		wchar_t *line = MelderReadText_readLine (text); cherror
		if (line == NULL) error1 (L"No lines.")
		{
			wchar_t *end = wcsstr (line, L"sys/oo/ooTextFile");   /* oo format? */
			if (end) {
				klas = texgetw2 (text); cherror
				data = (Data *)Thing::newFromClassName (klas); cherror
			} else {
				end = wcsstr (line, L"TextFile");
				if (end == NULL) error1 (L"Not an old-type text file; should not occur.")
				*end = '\0';
				data = (Data *)Thing::newFromClassName (line); cherror
				Thing::version = -1;   /* Old version: override version number, which was set to 0 by newFromClassName. */
			}
			MelderFile_getParentDir (file, & Data_directoryBeingRead);
			data->readText (text); cherror
		}
	}
end:
	Melder_free (klas);
	MelderReadText_delete (text);
	iferror forget (data);
	return data;
}

bool Data::canReadBinary () {
	/*return our readBinary != classData -> readBinary;*/ // FIXME
	return false;
}

Data *Data::readFromBinaryFile (MelderFile file) {
	Data *data = NULL;
	int n;
	FILE *f;
	char line [200], *end;
	if ((f = Melder_fopen (file, "rb")) == NULL) return NULL;
	n = fread (line, 1, 199, f); line [n] = '\0';
	end = strstr (line, "sys/oo/ooBinaryFile");
	if (end) {
		char *klas;
		fseek (f, strlen ("sys/oo/ooBinaryFile"), 0);
		klas = bingets1 (f);
		if (! klas || ! (data = (Data *)Thing::newFromClassNameA (klas))) { fclose (f); return 0; }
		Melder_free (klas);
	} else {
		end = strstr (line, "BinaryFile");
		if (! end || ! (*end = '\0', data = (Data *)Thing::newFromClassNameA (line))) {
			fclose (f);
			return (Data *)Melder_errorp3 (L"(Data_readFromBinaryFile:) File ", MelderFile_messageName (file),
				L" is not a Data binary file.");
		}
		Thing::version = -1;   /* Old version: override version number, which was set to 0 by newFromClassName. */
		rewind (f);
		fread (line, 1, end - line + strlen ("BinaryFile"), f);
	}
	MelderFile_getParentDir (file, & Data_directoryBeingRead);
	if (! data->readBinary (f)) forget (data);
	fclose (f);
	return data;
}

bool Data::canReadLisp () {
	/*return our readLisp != classData -> readLisp;*/ // FIXME
	return false;
}

Data *Data::readFromLispFile (MelderFile file) {
	Data *data = NULL;
	FILE *f;
	char line [200], *end;
	if ((f = Melder_fopen (file, "r")) == NULL) return NULL;
	fgets (line, 199, f);
	end = strstr (line, "LispFile");
	if (! end || ! (*end = '\0', data = (Data *)Thing::newFromClassNameA (line))) {
		fclose (f);
		return (Data *)Melder_errorp3 (L"(Data_readFromLispFile:) File ", MelderFile_messageName (file),
			L" is not a Data LISP file.");
	}
	MelderFile_getParentDir (file, & Data_directoryBeingRead);
	if (! data->readLisp (f)) forget (data);
	fclose (f);
	return data;
}

/* Generic reading. */

static int numFileTypeRecognizers = 0;
static Data * (*fileTypeRecognizers [100]) (int nread, const char *header, MelderFile fs);
void Data::recognizeFileType (Data * (*recognizer) (int nread, const char *header, MelderFile fs)) {
	Melder_assert (numFileTypeRecognizers < 100);
	fileTypeRecognizers [++ numFileTypeRecognizers] = recognizer;
}

Data * Data::readFromFile (MelderFile file) {
	int nread, i;
	char header [513];
	FILE *f = Melder_fopen (file, "rb");
	if (! f) return NULL;
	nread = fread (& header [0], 1, 512, f);
	fclose (f);
	header [nread] = 0;

	/***** 1. Is this file a text file as defined in Data.c? *****/

	if (nread > 11) {
		char *p = strstr (header, "TextFile");
		if (p != NULL && p - header < nread - 8 && p - header < 40)
			return Data::readFromTextFile (file);
	}
	if (nread > 22) {
		char headerCopy [101];
		memcpy (headerCopy, header, 100);
		headerCopy [100] = '\0';
		for (i = 0; i < 100; i ++)
			if (headerCopy [i] == '\0') headerCopy [i] = '\001';
		char *p = strstr (headerCopy, "T\001e\001x\001t\001F\001i\001l\001e");
		if (p != NULL && p - headerCopy < nread - 15 && p - headerCopy < 80)
			return Data::readFromTextFile (file);
	}

	/***** 2. Is this file a binary file as defined in Data.c? *****/

	if (nread > 13) {
		char *p = strstr (header, "BinaryFile");
		if (p != NULL && p - header < nread - 10 && p - header < 40)
			return Data::readFromBinaryFile (file);
	}

	/***** 3. Is this file a LISP file as defined in Data.c? *****/

	if (nread > 11) {
		char *p = strstr (header, "LispFile");
		if (p != NULL && p - header < nread - 8 && p - header < 40)
			return Data::readFromLispFile (file);
	}

	/***** 4. Is this file of a type for which a recognizer has been installed? *****/

	MelderFile_getParentDir (file, & Data_directoryBeingRead);
	for (i = 1; i <= numFileTypeRecognizers; i ++) {
		Data *object = (Data*)fileTypeRecognizers [i] (nread, header, file);
		if (Melder_hasError ()) return NULL;
		if (object) return object;
	}

	/***** 5. Is this a common text file? *****/

	for (i = 0; i < nread; i ++)
		if (header [i] < 32 || header [i] > 126)   /* Not ASCII? */
			break;
	if (i >= nread) return Data::readFromTextFile (file);

	return (Data *)Melder_errorp3 (L"(Data_readFromFile:) File ", MelderFile_messageName (file), L" not recognized.");
}

/* Recursive routines for working with struct members. */

int Data::Description::countMembers (Data::Description *structDescription) {
	Data::Description *desc;
	int count = 0;
	for (desc = structDescription; desc -> _name; desc ++)
		count ++;
	if (structDescription [0]. _type == inheritwa) {
		Data::Description *parentDescription = * (Data::Description **) structDescription [0]. _tagType;
		if (parentDescription)
			return count + countMembers(parentDescription);
	}
	return count;
}

Data::Description *Data::Description::findMatch (Data::Description *structDescription, const wchar_t *name) {
	Data::Description *desc;
	for (desc = structDescription; desc -> _name; desc ++)
		if (wcsequ (name, desc -> _name)) return desc;
	if (structDescription [0]. _type == inheritwa) {
		Data::Description *parentDescription = * (Data::Description **) structDescription [0]. _tagType;
		if (parentDescription)
			return findMatch (parentDescription, name);
	}
	return NULL;   /* Not found. */
}

Data::Description * Data::Description::findNumberUse (Data::Description *structDescription, const wchar_t *string) {
	Data::Description *desc;
	for (desc = structDescription; desc -> _name; desc ++) {
		if (desc -> _max1 && wcsequ (desc -> _max1, string)) return desc;
		if (desc -> _max2 && wcsequ (desc -> _max2, string)) return desc;
	}
	if (structDescription [0]. _type == inheritwa) {
		Data::Description *parentDescription = * (Data::Description **) structDescription [0]. _tagType;
		if (parentDescription)
			return findNumberUse (parentDescription, string);
	}
	return NULL;
}

/* Retrieving data from object + description. */

long Data::Description::integer (void *address, Data::Description *description) {
	switch (description -> _type) {
		case bytewa: return * (signed char *) ((char *) address + description -> _offset);
		case shortwa: return * (short *) ((char *) address + description -> _offset);
		case intwa: return * (int *) ((char *) address + description -> _offset);
		case longwa: return * (long *) ((char *) address + description -> _offset);
		case ubytewa: return * (unsigned char *) ((char *) address + description -> _offset);
		case ushortwa: return * (unsigned short *) ((char *) address + description -> _offset);
		case uintwa: return * (unsigned int *) ((char *) address + description -> _offset);
		case ulongwa: return * (unsigned long *) ((char *) address + description -> _offset);
		case boolwa: return * (bool *) ((char *) address + description -> _offset);
		case charwa: return * (char *) ((char *) address + description -> _offset);
		case collectionwa: return (* (Collection **) ((char *) address + description -> _offset)) -> size();
		case objectwa: return (* (Collection **) ((char *) address + description -> _offset)) -> size();
		default: return 0;
	}
}

int Data::Description::evaluateInteger (void *structAddress, Data::Description *structDescription,
	const wchar_t *formula, long *result)
{
	if (formula == NULL) {   /* This was a VECTOR_FROM array. */
		*result = 1;
		return 1;
	}
	if (wcsnequ (formula, L"my ", 3)) {
		wchar_t buffer [100], *minus1, *psize;
		Data::Description *sizeDescription;
		wcscpy (buffer, formula + 3);   /* Skip leading "my ". */
		if ((minus1 = wcsstr (buffer, L" - 1")) != NULL)
			*minus1 = '\0';   /* Strip trailing " - 1", but remember. */
		if ((psize = wcsstr (buffer, L" -> size")) != NULL)
			*psize = '\0';   /* Strip trailing " -> size". */
		if (! (sizeDescription = findMatch (structDescription, buffer))) {
			*result = 0;
			return 0 /*Melder_error ("Cannot find member \"%ls\".", buffer)*/;
		};
		*result = Data::Description::integer (structAddress, sizeDescription);
		if (minus1) *result -= 1;
	} else {
		*result = wcstol (formula, NULL, 10);
	}
	return 1;
}

/* End of file Data.c */
