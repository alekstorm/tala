/* melder_enums.h
 *
 * Copyright (C) 2007 Paul Boersma
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
 * pb 2007/12/07
 */

enums_begin (kMelder_number, 1)
	enums_add (kMelder_number, 1, EQUAL_TO, L"equal to")
	enums_add (kMelder_number, 2, NOT_EQUAL_TO, L"not equal to")
	enums_add (kMelder_number, 3, LESS_THAN, L"less than")
	enums_add (kMelder_number, 4, LESS_THAN_OR_EQUAL_TO, L"less than or equal to")
	enums_add (kMelder_number, 5, GREATER_THAN, L"greater than")
	enums_add (kMelder_number, 6, GREATER_THAN_OR_EQUAL_TO, L"greater than or equal to")
	/* For reading old preferences files: */
	enums_alt (kMelder_number, EQUAL_TO, L"1")
	enums_alt (kMelder_number, NOT_EQUAL_TO, L"2")
	enums_alt (kMelder_number, LESS_THAN, L"3")
	enums_alt (kMelder_number, LESS_THAN_OR_EQUAL_TO, L"4")
	enums_alt (kMelder_number, GREATER_THAN, L"5")
	enums_alt (kMelder_number, GREATER_THAN_OR_EQUAL_TO, L"6")
enums_end (kMelder_number, 6, EQUAL_TO)

enums_begin (kMelder_string, 1)
	enums_add (kMelder_string, 1, EQUAL_TO, L"is equal to")
	enums_add (kMelder_string, 2, NOT_EQUAL_TO, L"is not equal to")
	enums_add (kMelder_string, 3, CONTAINS, L"contains")
	enums_add (kMelder_string, 4, DOES_NOT_CONTAIN, L"does not contain")
	enums_add (kMelder_string, 5, STARTS_WITH, L"starts with")
	enums_add (kMelder_string, 6, DOES_NOT_START_WITH, L"does not start with")
	enums_add (kMelder_string, 7, ENDS_WITH, L"ends with")
	enums_add (kMelder_string, 8, DOES_NOT_END_WITH, L"does not end with")
	enums_add (kMelder_string, 9, MATCH_REGEXP, L"matches (regex)")
	/* For reading old preferences files: */
	enums_alt (kMelder_string, EQUAL_TO, L"1")
	enums_alt (kMelder_string, NOT_EQUAL_TO, L"2")
	enums_alt (kMelder_string, CONTAINS, L"3")
	enums_alt (kMelder_string, DOES_NOT_CONTAIN, L"4")
	enums_alt (kMelder_string, STARTS_WITH, L"5")
	enums_alt (kMelder_string, DOES_NOT_START_WITH, L"6")
	enums_alt (kMelder_string, ENDS_WITH, L"7")
	enums_alt (kMelder_string, DOES_NOT_END_WITH, L"8")
	enums_alt (kMelder_string, MATCH_REGEXP, L"9")
enums_end (kMelder_string, 9, EQUAL_TO)

enums_begin (kMelder_textInputEncoding, 1)
	enums_add (kMelder_textInputEncoding, 1, UTF8, L"UTF-8")
	enums_add (kMelder_textInputEncoding, 2, UTF8_THEN_ISO_LATIN1, L"try UTF-8, then ISO Latin-1")
	enums_add (kMelder_textInputEncoding, 3, ISO_LATIN1, L"ISO Latin-1")
	enums_add (kMelder_textInputEncoding, 4, UTF8_THEN_WINDOWS_LATIN1, L"try UTF-8, then Windows Latin-1")
	enums_add (kMelder_textInputEncoding, 5, WINDOWS_LATIN1, L"Windows Latin-1")
	enums_add (kMelder_textInputEncoding, 6, UTF8_THEN_MACROMAN, L"try UTF-8, then MacRoman")
	enums_add (kMelder_textInputEncoding, 7, MACROMAN, L"MacRoman")
	/* For reading old preferences files: */
	enums_alt (kMelder_textInputEncoding, UTF8, L"1")
	enums_alt (kMelder_textInputEncoding, UTF8_THEN_ISO_LATIN1, L"2")
	enums_alt (kMelder_textInputEncoding, ISO_LATIN1, L"3")
	enums_alt (kMelder_textInputEncoding, UTF8_THEN_WINDOWS_LATIN1, L"4")
	enums_alt (kMelder_textInputEncoding, WINDOWS_LATIN1, L"5")
	enums_alt (kMelder_textInputEncoding, UTF8_THEN_MACROMAN, L"6")
	enums_alt (kMelder_textInputEncoding, MACROMAN, L"7")
#if defined (macintosh)
enums_end (kMelder_textInputEncoding, 7, UTF8_THEN_MACROMAN)
#elif defined (_WIN32)
enums_end (kMelder_textInputEncoding, 7, UTF8_THEN_WINDOWS_LATIN1)
#else
enums_end (kMelder_textInputEncoding, 7, UTF8_THEN_ISO_LATIN1)
#endif

enums_begin (kMelder_textOutputEncoding, 1)
	enums_add (kMelder_textOutputEncoding, 1, UTF8, L"UTF-8")
	enums_add (kMelder_textOutputEncoding, 2, UTF16, L"UTF-16")
	enums_add (kMelder_textOutputEncoding, 3, ASCII_THEN_UTF16, L"try ASCII, then UTF-16")
	enums_add (kMelder_textOutputEncoding, 4, ISO_LATIN1_THEN_UTF16, L"try ISO Latin-1, then UTF-16")
	/* For reading old preferences files: */
	enums_alt (kMelder_textOutputEncoding, UTF8, L"1")
	enums_alt (kMelder_textOutputEncoding, UTF16, L"2")
	enums_alt (kMelder_textOutputEncoding, ASCII_THEN_UTF16, L"3")
	enums_alt (kMelder_textOutputEncoding, ISO_LATIN1_THEN_UTF16, L"4")
enums_end (kMelder_textOutputEncoding, 4, ASCII_THEN_UTF16)

enums_begin (kMelder_asynchronicityLevel, 0)
	enums_add (kMelder_asynchronicityLevel, 0, SYNCHRONOUS, L"synchronous (nothing)")
	enums_add (kMelder_asynchronicityLevel, 1, CALLING_BACK, L"calling back (view running cursor)")
	enums_add (kMelder_asynchronicityLevel, 2, INTERRUPTABLE, L"interruptable (Escape key stops playing)")
	enums_add (kMelder_asynchronicityLevel, 3, ASYNCHRONOUS, L"asynchronous (anything)")
	/* For reading old preferences files: */
	enums_alt (kMelder_asynchronicityLevel, SYNCHRONOUS, L"0")
	enums_alt (kMelder_asynchronicityLevel, CALLING_BACK, L"1")
	enums_alt (kMelder_asynchronicityLevel, INTERRUPTABLE, L"2")
	enums_alt (kMelder_asynchronicityLevel, ASYNCHRONOUS, L"3")
enums_end (kMelder_asynchronicityLevel, 3, ASYNCHRONOUS)

/* End of file melder_enums.h */
