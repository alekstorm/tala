/* Graphics_enums.h
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
 * pb 2007/12/09
 */

enums_begin (kGraphics_font, 0)
	enums_add (kGraphics_font, 0, HELVETICA, L"Helvetica")
	enums_add (kGraphics_font, 1, TIMES, L"Times")
	enums_add (kGraphics_font, 2, COURIER, L"Courier")
	enums_add (kGraphics_font, 3, PALATINO, L"Palatino")
enums_end (kGraphics_font, 3, TIMES)

enums_begin (kGraphics_horizontalAlignment, 0)
	enums_add (kGraphics_horizontalAlignment, 0, LEFT, L"left")
	enums_add (kGraphics_horizontalAlignment, 1, CENTRE, L"centre")
	enums_alt (kGraphics_horizontalAlignment,    CENTRE, L"center")
	enums_add (kGraphics_horizontalAlignment, 2, RIGHT, L"right")
	/* For reading old preferences files: */
	enums_alt (kGraphics_horizontalAlignment, LEFT, L"0")
	enums_alt (kGraphics_horizontalAlignment, CENTRE, L"1")
	enums_alt (kGraphics_horizontalAlignment, RIGHT, L"2")
enums_end (kGraphics_horizontalAlignment, 2, CENTRE)

enums_begin (kGraphicsPostscript_spots, 0)
	enums_add (kGraphicsPostscript_spots, 0, FINE, L"finest")
	enums_add (kGraphicsPostscript_spots, 1, PHOTOCOPYABLE, L"photocopyable")
	/* For reading old preferences files: */
	enums_alt (kGraphicsPostscript_spots, FINE, L"0")
	enums_alt (kGraphicsPostscript_spots, PHOTOCOPYABLE, L"1")
enums_end (kGraphicsPostscript_spots, 1, FINE)

enums_begin (kGraphicsPostscript_paperSize, 0)
	enums_add (kGraphicsPostscript_paperSize, 0, A4, L"A4")
	enums_add (kGraphicsPostscript_paperSize, 1, A3, L"A3")
	enums_add (kGraphicsPostscript_paperSize, 2, US_LETTER, L"US Letter")
	/* For reading old preferences files: */
	enums_alt (kGraphicsPostscript_paperSize, A4, L"0")
	enums_alt (kGraphicsPostscript_paperSize, A3, L"1")
	enums_alt (kGraphicsPostscript_paperSize, US_LETTER, L"2")
enums_end (kGraphicsPostscript_paperSize, 2, A4)

enums_begin (kGraphicsPostscript_orientation, 0)
	enums_add (kGraphicsPostscript_orientation, 0, PORTRAIT, L"portrait")
	enums_add (kGraphicsPostscript_orientation, 1, LANDSCAPE, L"landscape")
	/* For reading old preferences files: */
	enums_alt (kGraphicsPostscript_orientation, PORTRAIT, L"0")
	enums_alt (kGraphicsPostscript_orientation, LANDSCAPE, L"1")
enums_end (kGraphicsPostscript_orientation, 1, PORTRAIT)

enums_begin (kGraphicsPostscript_fontChoiceStrategy, 0)
	enums_add (kGraphicsPostscript_fontChoiceStrategy, 0, AUTOMATIC, L"automatic")
	enums_add (kGraphicsPostscript_fontChoiceStrategy, 1, LINOTYPE, L"Linotype")
	enums_add (kGraphicsPostscript_fontChoiceStrategy, 2, MONOTYPE, L"Monotype")
	enums_add (kGraphicsPostscript_fontChoiceStrategy, 3, PS_MONOTYPE, L"PS Monotype")
	/* For reading old preferences files: */
	enums_alt (kGraphicsPostscript_fontChoiceStrategy, AUTOMATIC, L"0")
	enums_alt (kGraphicsPostscript_fontChoiceStrategy, LINOTYPE, L"1")
	enums_alt (kGraphicsPostscript_fontChoiceStrategy, MONOTYPE, L"2")
	enums_alt (kGraphicsPostscript_fontChoiceStrategy, PS_MONOTYPE, L"3")
enums_end (kGraphicsPostscript_fontChoiceStrategy, 3, AUTOMATIC)

/* End of file Graphics_enums.h */
