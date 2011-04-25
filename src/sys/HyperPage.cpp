/* HyperPage.c
 *
 * Copyright (C) 1996-2010 Paul Boersma
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
 * pb 2003/09/15 better positioning of buttons
 * pb 2004/11/23 made vertical spacing dependent on font size
 * pb 2005/05/07 embedded scripts (for pictures)
 * pb 2005/07/19 moved "<1" and "1>" buttons to the top, removed horizontal scroll bar and page number
 * pb 2006/10/20 embedded scripts allow links
 * pb 2007/06/10 wchar_t
 * pb 2007/11/30 erased Graphics_printf
 * pb 2008/03/21 new Editor API
 * pb 2008/11/24 prevented crash by Melder_malloc (replaced with Melder_calloc)
 * pb 2009/03/17 split up structPraat
 * pb 2010/05/14 GTK
 */

#include <ctype.h>
#include "HyperPage.h"
#include "Printer.h"
#include "Preferences.h"
#include "machine.h"

#include "praat.h"
#include "EditorM.h"

#define PAGE_HEIGHT  320.0
#define SCREEN_HEIGHT  15.0
#define PAPER_TOP  12.0
#define TOP_MARGIN  0.8
#define PAPER_BOTTOM  (13.0 - (double) thePrinter. paperHeight / thePrinter. resolution)
#define BOTTOM_MARGIN  0.5
static double resolution;

static enum kGraphics_font prefs_font;
static int prefs_fontSize;

/********** class HyperLink **********/

class_methods (HyperLink, Data)
class_methods_end

HyperLink HyperLink_create (const wchar_t *name, double x1DC, double x2DC, double y1DC, double y2DC) {
	HyperLink me = Thing_new (HyperLink);
	if (! me) return NULL;
	Thing_setName (me, name);
	my x1DC = x1DC, my x2DC = x2DC, my y1DC = y1DC, my y2DC = y2DC;
	return me;
}

/********** class HyperPage **********/

static void gui_drawingarea_cb_resize (void *void_me, GuiDrawingAreaResizeEvent event) {
	HyperPage *editor= (HyperPage *)void_me;
	if (editor->_g == NULL) return;
	Graphics_setWsViewport (editor->_g, 0, event -> width, 0, event -> height);
	Graphics_setWsWindow (editor->_g, 0.0, editor->_rightMargin = event -> width / resolution,
		PAGE_HEIGHT - event -> height / resolution, PAGE_HEIGHT);
	Graphics_updateWs (editor->_g);
	editor->updateVerticalScrollBar ();
}

#if gtk
static void gui_cb_verticalScroll (GtkRange *rng, gpointer void_me) {
	HyperPage *editor= (HyperPage *)void_me;
	double value = gtk_range_get_value (GTK_RANGE (rng));
	if (value != editor->_top) {
		editor->_top = value;
		Graphics_clearWs (editor->_g);
		editor->initScreen ();
		editor->draw ();   /* Do not wait for expose event. */
		editor->updateVerticalScrollBar ();
	}
}
#else
static void gui_cb_verticalScroll (GUI_ARGS) {
	int value, sliderSize, incr, pincr;
	#if gtk
		double value = gtk_range_get_value (GTK_RANGE (w));
	#elif motif
		XmScrollBarGetValues (w, & value, & sliderSize, & incr, & pincr);
	#endif
	if (value != editor->_top) {
		editor->_top = value;
		Graphics_clearWs (editor->_g);
		editor->initScreen ();
		editor->draw ();   /* Do not wait for expose event. */
		editor->updateVerticalScrollBar ();
	}
}
#endif

HyperPage::HyperPage (GuiObject parent, const wchar_t *title, Any data)
	: Editor (parent, 0, 0, 6 * resolution + 30, 800, title, data) {
	createMenus ();
	resolution = Gui_getResolution (parent);
	#if motif
		Melder_assert (XtWindow (_drawingArea));
	#endif
	_g = Graphics_create_xmdrawingarea (_drawingArea);
	Graphics_setAtSignIsLink (_g, TRUE);
	Graphics_setDollarSignIsCode (_g, TRUE);
	Graphics_setFont (_g, kGraphics_font_TIMES);
	if (prefs_font != kGraphics_font_TIMES && prefs_font != kGraphics_font_HELVETICA)
		prefs_font = kGraphics_font_TIMES;   // Ensure Unicode compatibility.
	_font = prefs_font;
	setFontSize (prefs_fontSize);	

	struct structGuiDrawingAreaResizeEvent event = { _drawingArea, 0 };
	event. width = GuiObject_getWidth (_drawingArea);
	event. height = GuiObject_getHeight (_drawingArea);
	gui_drawingarea_cb_resize (this, & event);

	#if gtk
		g_signal_connect (G_OBJECT (_verticalScrollBar), "value-changed", G_CALLBACK (gui_cb_verticalScroll), this);
	#elif motif
		XtAddCallback (_verticalScrollBar, XmNvalueChangedCallback, gui_cb_verticalScroll, (XtPointer) this);
		XtAddCallback (_verticalScrollBar, XmNdragCallback, gui_cb_verticalScroll, (XtPointer) this);
	#endif
	updateVerticalScrollBar ();   /* Scroll to the top (_top == 0). */
}

HyperPage::~HyperPage() {
	forget (_links);
	Melder_free (_entryHint);
	forget (_g);
	for (int i = 0; i < 20; i ++) Melder_free (_history [i]. page);
	Melder_free (_currentPageTitle);
	if (_praatApplication != NULL) {
		for (int iobject = ((PraatObjects) _praatObjects) -> n; iobject >= 1; iobject --) {
			Melder_free (((PraatObjects) _praatObjects) -> list [iobject]. name);
			forget (((PraatObjects) _praatObjects) -> list [iobject]. object);
		}
		Melder_free (_praatApplication);
		Melder_free (_praatObjects);
		Melder_free (_praatPicture);
	}
}

void HyperPage::prefs (void) {
	Preferences_addEnum (L"HyperPage.font", & prefs_font, kGraphics_font, DEFAULT);
	Preferences_addInt (L"HyperPage.fontSize", & prefs_fontSize, 12);
}

void HyperPage::saveHistory (const wchar_t *title) {
	if (! title) return;

	/*
	 * The page title will be saved at the top. Go there.
	 */
	while (_historyPointer < 19 && _history [_historyPointer]. page)
		_historyPointer ++;

	/*
	 * If the page title to be saved is already at the top, ignore it.
	 */	
	if (_history [_historyPointer]. page) {
		if (wcsequ (_history [_historyPointer]. page, title)) return;
	} else if (_historyPointer > 0 && wcsequ (_history [_historyPointer - 1]. page, title)) {
		_historyPointer --;
		return;
	}

	/*
	 * If the history buffer is full, shift it.
	 */
	if (_historyPointer == 19 && _history [_historyPointer]. page) {
		int i;
		Melder_free (_history [0]. page);
		for (i = 0; i < 19; i ++) _history [i] = _history [i + 1];
	}

	/*
	 * Add the page title to the top of the history list.
	 */
	_history [_historyPointer]. page = Melder_wcsdup_f (title);
}

/********************************************************************************
 *
 * Before drawing or printing.
 *
 */

void HyperPage::initScreen () {
	_y = PAGE_HEIGHT + _top / 5.0;
	_x = 0;
	_previousBottomSpacing = 0.0;
	forget (_links);
	_links = Collection_create (classHyperLink, 100);
}

void HyperPage::initSheetOfPaper () {
	int reflect = _mirror && (_pageNumber & 1) == 0;
	wchar_t *leftHeader = reflect ? _outsideHeader : _insideHeader;
	wchar_t *rightHeader = reflect ? _insideHeader : _outsideHeader;
	wchar_t *leftFooter = reflect ? _outsideFooter : _insideFooter;
	wchar_t *rightFooter = reflect ? _insideFooter : _outsideFooter;

	_y = PAPER_TOP - TOP_MARGIN;
	_x = 0;
	_previousBottomSpacing = 0.0;
	Graphics_setFont (_ps, kGraphics_font_TIMES);
	Graphics_setFontSize (_ps, 12);
	Graphics_setFontStyle (_ps, Graphics_ITALIC);
	if (leftHeader) {
		Graphics_setTextAlignment (_ps, Graphics_LEFT, Graphics_TOP);
		Graphics_text (_ps, 0.7, PAPER_TOP, leftHeader);
	}
	if (_middleHeader) {
		Graphics_setTextAlignment (_ps, Graphics_CENTRE, Graphics_TOP);
		Graphics_text (_ps, 0.7 + 3, PAPER_TOP, _middleHeader);
	}
	if (rightHeader) {
		Graphics_setTextAlignment (_ps, Graphics_RIGHT, Graphics_TOP);
		Graphics_text (_ps, 0.7 + 6, PAPER_TOP, rightHeader);
	}
	if (leftFooter) {
		Graphics_setTextAlignment (_ps, Graphics_LEFT, Graphics_BOTTOM);
		Graphics_text (_ps, 0.7, PAPER_BOTTOM, leftFooter);
	}
	if (_middleFooter) {
		Graphics_setTextAlignment (_ps, Graphics_CENTRE, Graphics_BOTTOM);
		Graphics_text (_ps, 0.7 + 3, PAPER_BOTTOM, _middleFooter);
	}
	if (rightFooter) {
		Graphics_setTextAlignment (_ps, Graphics_RIGHT, Graphics_BOTTOM);
		Graphics_text (_ps, 0.7 + 6, PAPER_BOTTOM, rightFooter);
	}
	Graphics_setFontStyle (_ps, Graphics_NORMAL);
	if (_pageNumber)
		Graphics_text1 (_ps, 0.7 + ( reflect ? 0 : 6 ), PAPER_BOTTOM, Melder_integer (_pageNumber));
	Graphics_setTextAlignment (_ps, Graphics_LEFT, Graphics_BOTTOM);
}

int HyperPage::any (const wchar_t *text, int font, int size, int style, double minFooterDistance,
	double x, double secondIndent, double topSpacing, double bottomSpacing, unsigned long method)
{
	double heightGuess;

	if (_rightMargin == 0) return 0;
	// Melder_assert (_rightMargin != 0);

	heightGuess = size * (1.2/72) * ((long) size * wcslen (text) / (int) (_rightMargin * 150));

	if (! _printing) {
		Graphics_Link *paragraphLinks;
		int numberOfParagraphLinks, ilink;
		if (_entryHint && (method & HyperPage_USE_ENTRY_HINT) && wcsequ (text, _entryHint)) {
			_entryPosition = _y;
		}
		_y -= ( _previousBottomSpacing > topSpacing ? _previousBottomSpacing : topSpacing ) * size / 12.0;
		_y -= size * (1.2/72);
		_x = x;

		if (/* _y > PAGE_HEIGHT + 2.0 + heightGuess || */ _y < PAGE_HEIGHT - SCREEN_HEIGHT) {
			_y -= heightGuess;
		} else {
			Graphics_setFont (_g, (kGraphics_font)font);
			Graphics_setFontSize (_g, size);
			Graphics_setWrapWidth (_g, _rightMargin - x - 0.1);
			Graphics_setSecondIndent (_g, secondIndent);
			Graphics_setFontStyle (_g, style);
			Graphics_text (_g, _x, _y, text);
			numberOfParagraphLinks = Graphics_getLinks (& paragraphLinks);
			if (_links) for (ilink = 1; ilink <= numberOfParagraphLinks; ilink ++) {
				HyperLink link = HyperLink_create (paragraphLinks [ilink]. name,
					paragraphLinks [ilink]. x1, paragraphLinks [ilink]. x2,
					paragraphLinks [ilink]. y1, paragraphLinks [ilink]. y2);
				Collection_addItem (_links, link);
			}
			if (method & HyperPage_ADD_BORDER) {
				Graphics_setLineWidth (_g, 2);
				Graphics_line (_g, 0.0, _y, _rightMargin, _y);
				Graphics_setLineWidth (_g, 1);
			}
			/*
			 * The text may have wrapped.
			 * Ask the Graphics manager by how much, and update our text position accordingly.
			 */
			_y = Graphics_inqTextY (_g);
		}
	} else {
		Graphics_setFont (_ps, (kGraphics_font)font);
		Graphics_setFontSize (_ps, size);
		_y -= _y == PAPER_TOP - TOP_MARGIN ? 0 : ( _previousBottomSpacing > topSpacing ? _previousBottomSpacing : topSpacing ) * size / 12.0;
		_y -= size * (1.2/72);
		if (_y < PAPER_BOTTOM + BOTTOM_MARGIN + minFooterDistance + size * (1.2/72) * (wcslen (text) / (6.0 * 10))) {
			Graphics_nextSheetOfPaper (_ps);
			if (_pageNumber) _pageNumber ++;
			initSheetOfPaper ();
			Graphics_setFont (_ps, (kGraphics_font)font);
			Graphics_setFontSize (_ps, size);
			_y -= size * (1.2/72);
		}
		_x = 0.7 + x;
		Graphics_setWrapWidth (_ps, 6.0 - x);
		Graphics_setSecondIndent (_ps, secondIndent);
		Graphics_setFontStyle (_ps, style);
		Graphics_text (_ps, _x, _y, text);
		if (method & HyperPage_ADD_BORDER) {
			Graphics_setLineWidth (_ps, 3);
			/*Graphics_line (_ps, 0.7, _y, 6.7, _y);*/
			Graphics_line (_ps, 0.7, _y + size * (1.2/72) + 0.07, 6.7, _y + size * (1.2/72) + 0.07);
			Graphics_setLineWidth (_ps, 1);
		}
		_y = Graphics_inqTextY (_ps);
	}
	_previousBottomSpacing = bottomSpacing;
	return 1;
}

int HyperPage::pageTitle (const wchar_t *title) {
	return any (title, _font, _fontSize * 2, 0,
		2.0, 0.0, 0.0, _printing ? 0.4/2 : 0.2/2, 0.3/2, HyperPage_ADD_BORDER);
}
int HyperPage::intro (const wchar_t *text) {
	return any (text, _font, _fontSize, 0, 0.0, 0.03, 0.0, 0.1, 0.1, 0);
}
int HyperPage::entry (const wchar_t *title) {
	return any (title, _font, _fontSize * 1.4, Graphics_BOLD, 0.5, 0.0, 0.0, 0.25/1.4, 0.1/1.4, HyperPage_USE_ENTRY_HINT);
}
int HyperPage::paragraph (const wchar_t *text) {
	return any (text, _font, _fontSize, 0, 0.0, 0.03, 0.0, 0.1, 0.1, 0);
}
int HyperPage::listItem (const wchar_t *text) {
	return any (text, _font, _fontSize, 0, 0.0, 0.30, 0.2, 0.0, 0.0, 0);
}
int HyperPage::listItem1 (const wchar_t *text) {
	return any (text, _font, _fontSize, 0, 0.0, 0.57, 0.2, 0.0, 0.0, 0);
}
int HyperPage::listItem2 (const wchar_t *text) {
	return any (text, _font, _fontSize, 0, 0.0, 0.84, 0.2, 0.0, 0.0, 0);
}
int HyperPage::listItem3 (const wchar_t *text) {
	return any (text, _font, _fontSize, 0, 0.0, 1.11, 0.2, 0.0, 0.0, 0);
}
int HyperPage::listTag (const wchar_t *text) {
	return any (text, _font, _fontSize, 0, 0.2, 0.03, 0.0, 0.1, 0.03, 0);
}
int HyperPage::listTag1 (const wchar_t *text) {
	return any (text, _font, _fontSize, 0, 0.2, 0.50, 0.0, 0.05, 0.03, 0);
}
int HyperPage::listTag2 (const wchar_t *text) {
	return any (text, _font, _fontSize, 0, 0.2, 0.97, 0.0, 0.03, 0.03, 0);
}
int HyperPage::listTag3 (const wchar_t *text) {
	return any (text, _font, _fontSize, 0, 0.2, 1.44, 0.0, 0.03, 0.03, 0);
}
int HyperPage::definition (const wchar_t *text) {
	return any (text, _font, _fontSize, 0, 0.0, 0.5, 0.0, 0.03, 0.1, 0);
}
int HyperPage::definition1 (const wchar_t *text) {
	return any (text, _font, _fontSize, 0, 0.0, 0.97, 0.0, 0.03, 0.05, 0);
}
int HyperPage::definition2 (const wchar_t *text) {
	return any (text, _font, _fontSize, 0, 0.0, 1.44, 0.0, 0.03, 0.03, 0);
}
int HyperPage::definition3 (const wchar_t *text) {
	return any (text, _font, _fontSize, 0, 0.0, 1.93, 0.0, 0.03, 0.03, 0);
}
int HyperPage::code (const wchar_t *text) {
	return any (text, kGraphics_font_COURIER, _fontSize * 0.86, 0, 0.0, 0.3, 0.5, 0.0, 0.0, 0);
}
int HyperPage::code1 (const wchar_t *text) {
	return any (text, kGraphics_font_COURIER, _fontSize * 0.86, 0, 0.0, 0.6, 0.5, 0.0, 0.0, 0);
}
int HyperPage::code2 (const wchar_t *text) {
	return any (text, kGraphics_font_COURIER, _fontSize * 0.86, 0, 0.0, 0.9, 0.5, 0.0, 0.0, 0);
}
int HyperPage::code3 (const wchar_t *text) {
	return any (text, kGraphics_font_COURIER, _fontSize * 0.86, 0, 0.0, 1.2, 0.5, 0.0, 0.0, 0);
}
int HyperPage::code4 (const wchar_t *text) {
	return any (text, kGraphics_font_COURIER, _fontSize * 0.86, 0, 0.0, 1.5, 0.5, 0.0, 0.0, 0);
}
int HyperPage::code5 (const wchar_t *text) {
	return any (text, kGraphics_font_COURIER, _fontSize * 0.86, 0, 0.0, 1.8, 0.5, 0.0, 0.0, 0);
}
int HyperPage::prototype (const wchar_t *text) {
	return any (text, _font, _fontSize, 0, 0.0, 0.03, 0.5, 0.0, 0.0, 0);
}
int HyperPage::formula (const wchar_t *formula) {
	double topSpacing = 0.2, bottomSpacing = 0.2, minFooterDistance = 0.0;
	int font = _font, size = _fontSize;
	if (! _printing) {
		_y -= ( _previousBottomSpacing > topSpacing ? _previousBottomSpacing : topSpacing ) * size / 12.0;
		_y -= size * (1.2/72);
		if (_y > PAGE_HEIGHT + 2.0 || _y < PAGE_HEIGHT - SCREEN_HEIGHT) {
		} else {
			Graphics_setFont (_g, (kGraphics_font)font);
			Graphics_setFontStyle (_g, 0);
			Graphics_setFontSize (_g, size);
			Graphics_setWrapWidth (_g, 0);
			Graphics_setTextAlignment (_g, Graphics_CENTRE, Graphics_BOTTOM);
			Graphics_text (_g, _rightMargin / 2, _y, formula);
			Graphics_setTextAlignment (_g, Graphics_LEFT, Graphics_BOTTOM);
		}
	} else {
		Graphics_setFont (_ps, (kGraphics_font)font);
		Graphics_setFontStyle (_ps, 0);
		Graphics_setFontSize (_ps, size);
		_y -= _y == PAPER_TOP - TOP_MARGIN ? 0 : ( _previousBottomSpacing > topSpacing ? _previousBottomSpacing : topSpacing ) * size / 12.0;
		_y -= size * (1.2/72);
		if (_y < PAPER_BOTTOM + BOTTOM_MARGIN + minFooterDistance) {
			Graphics_nextSheetOfPaper (_ps);
			if (_pageNumber) _pageNumber ++;
			initSheetOfPaper ();
			Graphics_setFont (_ps, (kGraphics_font)font);
			Graphics_setFontSize (_ps, size);
			_y -= size * (1.2/72);
		}
		Graphics_setWrapWidth (_ps, 0);
		Graphics_setTextAlignment (_ps, Graphics_CENTRE, Graphics_BOTTOM);
		Graphics_text (_ps, 3.7, _y, formula);
		Graphics_setTextAlignment (_ps, Graphics_LEFT, Graphics_BOTTOM);
	}
	_previousBottomSpacing = bottomSpacing;
	return 1;
}

int HyperPage::picture (double width_inches, double height_inches, void (*draw) (Graphics g)) {
	double topSpacing = 0.1, bottomSpacing = 0.1, minFooterDistance = 0.0;
	int font = _font, size = _fontSize;
	width_inches *= width_inches < 0.0 ? -1.0 : size / 12.0;
	height_inches *= height_inches < 0.0 ? -1.0 : size / 12.0;
	if (! _printing) {
		_y -= ( _previousBottomSpacing > topSpacing ? _previousBottomSpacing : topSpacing ) * size / 12.0;
		if (_y > PAGE_HEIGHT + height_inches || _y < PAGE_HEIGHT - SCREEN_HEIGHT) {
			_y -= height_inches;
		} else {
			_y -= height_inches;
			Graphics_setFont (_g, (kGraphics_font)font);
			Graphics_setFontStyle (_g, 0);
			Graphics_setFontSize (_g, size);
			_x = width_inches > _rightMargin ? 0 : 0.5 * (_rightMargin - width_inches);
			Graphics_setWrapWidth (_g, 0);
			Graphics_setViewport (_g, _x, _x + width_inches, _y, _y + height_inches);
			draw (_g);
			Graphics_setViewport (_g, 0, 1, 0, 1);
			Graphics_setWindow (_g, 0, 1, 0, 1);
			Graphics_setTextAlignment (_g, Graphics_LEFT, Graphics_BOTTOM);
		}
	} else {
		Graphics_setFont (_ps, (kGraphics_font)font);
		Graphics_setFontStyle (_ps, 0);
		Graphics_setFontSize (_ps, size);
		_y -= _y == PAPER_TOP - TOP_MARGIN ? 0 : ( _previousBottomSpacing > topSpacing ? _previousBottomSpacing : topSpacing ) * size / 12.0;
		_y -= height_inches;
		if (_y < PAPER_BOTTOM + BOTTOM_MARGIN + minFooterDistance) {
			Graphics_nextSheetOfPaper (_ps);
			if (_pageNumber) _pageNumber ++;
			initSheetOfPaper ();
			Graphics_setFont (_ps, (kGraphics_font)font);
			Graphics_setFontSize (_ps, size);
			_y -= height_inches;
		}
		_x = 3.7 - 0.5 * width_inches;
		if (_x < 0) _x = 0;
		Graphics_setWrapWidth (_ps, 0);
		Graphics_setViewport (_ps, _x, _x + width_inches, _y, _y + height_inches);
		draw (_ps);
		Graphics_setViewport (_ps, 0, 1, 0, 1);
		Graphics_setWindow (_ps, 0, 1, 0, 1);
		Graphics_setTextAlignment (_ps, Graphics_LEFT, Graphics_BOTTOM);
	}
	_previousBottomSpacing = bottomSpacing;
	return 1;
}

int HyperPage::script (double width_inches, double height_inches, const wchar_t *script) {
	wchar_t *text = Melder_wcsdup_f (script);
	Interpreter *interpreter = new Interpreter (NULL);
	double topSpacing = 0.1, bottomSpacing = 0.1, minFooterDistance = 0.0;
	int font = _font, size = _fontSize;
	double true_width_inches = width_inches * ( width_inches < 0.0 ? -1.0 : size / 12.0 );
	double true_height_inches = height_inches * ( height_inches < 0.0 ? -1.0 : size / 12.0 );
	if (! _printing) {
		_y -= ( _previousBottomSpacing > topSpacing ? _previousBottomSpacing : topSpacing ) * size / 12.0;
		if (_y > PAGE_HEIGHT + true_height_inches || _y < PAGE_HEIGHT - SCREEN_HEIGHT) {
			_y -= true_height_inches;
		} else {
			_y -= true_height_inches;
			Graphics_setFont (_g, (kGraphics_font)font);
			Graphics_setFontStyle (_g, 0);
			Graphics_setFontSize (_g, size);
			_x = true_width_inches > _rightMargin ? 0 : 0.5 * (_rightMargin - true_width_inches);
			Graphics_setWrapWidth (_g, 0);
			long x1DCold, x2DCold, y1DCold, y2DCold;
			Graphics_inqWsViewport (_g, & x1DCold, & x2DCold, & y1DCold, & y2DCold);
			double x1NDCold, x2NDCold, y1NDCold, y2NDCold;
			Graphics_inqWsWindow (_g, & x1NDCold, & x2NDCold, & y1NDCold, & y2NDCold);
			{
				if (_praatApplication == NULL) _praatApplication = Melder_calloc_f (structPraatApplication, 1);
				if (_praatObjects == NULL) _praatObjects = Melder_calloc_f (structPraatObjects, 1);
				if (_praatPicture == NULL) _praatPicture = Melder_calloc_f (structPraatPicture, 1);
				theCurrentPraatApplication = (structPraatApplication*)_praatApplication;
				theCurrentPraatApplication -> batch = true;   // prevent creation of editor windows
				theCurrentPraatApplication -> topShell = theForegroundPraatApplication. topShell;   // needed for UiForm_create () in dialogs
				theCurrentPraatObjects = (structPraatObjects*)_praatObjects;
				theCurrentPraatPicture = (structPraatPicture*)_praatPicture;
				theCurrentPraatPicture -> graphics = _g;   // has to draw into HyperPage rather than Picture window
				theCurrentPraatPicture -> font = font;
				theCurrentPraatPicture -> fontSize = size;
				theCurrentPraatPicture -> lineType = Graphics_DRAWN;
				theCurrentPraatPicture -> colour = Graphics_BLACK;
				theCurrentPraatPicture -> lineWidth = 1.0;
				theCurrentPraatPicture -> arrowSize = 1.0;
				theCurrentPraatPicture -> x1NDC = _x;
				theCurrentPraatPicture -> x2NDC = _x + true_width_inches;
				theCurrentPraatPicture -> y1NDC = _y;
				theCurrentPraatPicture -> y2NDC = _y + true_height_inches;

				Graphics_setViewport (_g, theCurrentPraatPicture -> x1NDC, theCurrentPraatPicture -> x2NDC, theCurrentPraatPicture -> y1NDC, theCurrentPraatPicture -> y2NDC);
				Graphics_setWindow (_g, 0.0, 1.0, 0.0, 1.0);
				long x1DC, y1DC, x2DC, y2DC;
				Graphics_WCtoDC (_g, 0.0, 0.0, & x1DC, & y2DC);
				Graphics_WCtoDC (_g, 1.0, 1.0, & x2DC, & y1DC);
				Graphics_resetWsViewport (_g, x1DC, x2DC, y1DC, y2DC);
				Graphics_setWsWindow (_g, 0, width_inches, 0, height_inches);
				theCurrentPraatPicture -> x1NDC = 0;
				theCurrentPraatPicture -> x2NDC = width_inches;
				theCurrentPraatPicture -> y1NDC = 0;
				theCurrentPraatPicture -> y2NDC = height_inches;
				Graphics_setViewport (_g, theCurrentPraatPicture -> x1NDC, theCurrentPraatPicture -> x2NDC, theCurrentPraatPicture -> y1NDC, theCurrentPraatPicture -> y2NDC);			

				Melder_progressOff ();
				Melder_warningOff ();
				structMelderDir saveDir = { { 0 } };
				Melder_getDefaultDir (& saveDir);
				if (! MelderDir_isNull (& _rootDirectory)) Melder_setDefaultDir (& _rootDirectory);
				interpreter->run (text);
				Melder_setDefaultDir (& saveDir);
				Melder_warningOn ();
				Melder_progressOn ();
				Graphics_setLineType (_g, Graphics_DRAWN);
				Graphics_setLineWidth (_g, 1.0);
				Graphics_setArrowSize (_g, 1.0);
				Graphics_setColour (_g, Graphics_BLACK);
				iferror {
					if (_scriptErrorHasBeenNotified) {
						Melder_clearError ();
					} else {
						Melder_flushError (NULL);
						_scriptErrorHasBeenNotified = true;
					}
				}
				/*Graphics_Link *paragraphLinks;
				long numberOfParagraphLinks = Graphics_getLinks (& paragraphLinks);
				if (_links) for (long ilink = 1; ilink <= numberOfParagraphLinks; ilink ++) {
					HyperLink link = HyperLink_create (paragraphLinks [ilink]. name,
						paragraphLinks [ilink]. x1, paragraphLinks [ilink]. x2,
						paragraphLinks [ilink]. y1, paragraphLinks [ilink]. y2);
					Collection_addItem (_links, link);
				}*/
				theCurrentPraatApplication = & theForegroundPraatApplication;
				theCurrentPraatObjects = & theForegroundPraatObjects;
				theCurrentPraatPicture = & theForegroundPraatPicture;
			}
			Graphics_resetWsViewport (_g, x1DCold, x2DCold, y1DCold, y2DCold);
			Graphics_setWsWindow (_g, x1NDCold, x2NDCold, y1NDCold, y2NDCold);
			Graphics_setViewport (_g, 0, 1, 0, 1);
			Graphics_setWindow (_g, 0, 1, 0, 1);
			Graphics_setTextAlignment (_g, Graphics_LEFT, Graphics_BOTTOM);
		}
	} else {
		Graphics_setFont (_ps, (kGraphics_font)font);
		Graphics_setFontStyle (_ps, 0);
		Graphics_setFontSize (_ps, size);
		_y -= _y == PAPER_TOP - TOP_MARGIN ? 0 : ( _previousBottomSpacing > topSpacing ? _previousBottomSpacing : topSpacing ) * size / 12.0;
		_y -= true_height_inches;
		if (_y < PAPER_BOTTOM + BOTTOM_MARGIN + minFooterDistance) {
			Graphics_nextSheetOfPaper (_ps);
			if (_pageNumber) _pageNumber ++;
			initSheetOfPaper ();
			Graphics_setFont (_ps, (kGraphics_font)font);
			Graphics_setFontSize (_ps, size);
			_y -= true_height_inches;
		}
		_x = 3.7 - 0.5 * true_width_inches;
		if (_x < 0) _x = 0;
		Graphics_setWrapWidth (_ps, 0);
		long x1DCold, x2DCold, y1DCold, y2DCold;
		Graphics_inqWsViewport (_ps, & x1DCold, & x2DCold, & y1DCold, & y2DCold);
		double x1NDCold, x2NDCold, y1NDCold, y2NDCold;
		Graphics_inqWsWindow (_ps, & x1NDCold, & x2NDCold, & y1NDCold, & y2NDCold);
		{
			if (_praatApplication == NULL) _praatApplication = Melder_calloc_f (structPraatApplication, 1);
			if (_praatObjects == NULL) _praatObjects = Melder_calloc_f (structPraatObjects, 1);
			if (_praatPicture == NULL) _praatPicture = Melder_calloc_f (structPraatPicture, 1);
			theCurrentPraatApplication = (structPraatApplication*)_praatApplication;
			theCurrentPraatApplication -> batch = true;
			theCurrentPraatObjects = (structPraatObjects*)_praatObjects;
			theCurrentPraatPicture = (structPraatPicture*)_praatPicture;
			theCurrentPraatPicture -> graphics = _ps;
			theCurrentPraatPicture -> font = font;
			theCurrentPraatPicture -> fontSize = size;
			theCurrentPraatPicture -> lineType = Graphics_DRAWN;
			theCurrentPraatPicture -> colour = Graphics_BLACK;
			theCurrentPraatPicture -> lineWidth = 1.0;
			theCurrentPraatPicture -> arrowSize = 1.0;
			theCurrentPraatPicture -> x1NDC = _x;
			theCurrentPraatPicture -> x2NDC = _x + true_width_inches;
			theCurrentPraatPicture -> y1NDC = _y;
			theCurrentPraatPicture -> y2NDC = _y + true_height_inches;

			Graphics_setViewport (_ps, theCurrentPraatPicture -> x1NDC, theCurrentPraatPicture -> x2NDC, theCurrentPraatPicture -> y1NDC, theCurrentPraatPicture -> y2NDC);
			Graphics_setWindow (_ps, 0.0, 1.0, 0.0, 1.0);
			long x1DC, y1DC, x2DC, y2DC;
			Graphics_WCtoDC (_ps, 0.0, 0.0, & x1DC, & y2DC);
			Graphics_WCtoDC (_ps, 1.0, 1.0, & x2DC, & y1DC);
			long shift = (long) (Graphics_getResolution (_ps) * true_height_inches) + (y1DCold - y2DCold);
			Graphics_resetWsViewport (_ps, x1DC, x2DC, y1DC + shift, y2DC + shift);
			Graphics_setWsWindow (_ps, 0, width_inches, 0, height_inches);
			theCurrentPraatPicture -> x1NDC = 0;
			theCurrentPraatPicture -> x2NDC = width_inches;
			theCurrentPraatPicture -> y1NDC = 0;
			theCurrentPraatPicture -> y2NDC = height_inches;
			Graphics_setViewport (_ps, theCurrentPraatPicture -> x1NDC, theCurrentPraatPicture -> x2NDC, theCurrentPraatPicture -> y1NDC, theCurrentPraatPicture -> y2NDC);

			Melder_progressOff ();
			Melder_warningOff ();
			structMelderDir saveDir = { { 0 } };
			Melder_getDefaultDir (& saveDir);
			if (! MelderDir_isNull (& _rootDirectory)) Melder_setDefaultDir (& _rootDirectory);
			interpreter->run (text);
			Melder_setDefaultDir (& saveDir);
			Melder_warningOn ();
			Melder_progressOn ();
			iferror Melder_clearError ();
			Graphics_setLineType (_ps, Graphics_DRAWN);
			Graphics_setLineWidth (_ps, 1.0);
			Graphics_setArrowSize (_ps, 1.0);
			Graphics_setColour (_ps, Graphics_BLACK);
			theCurrentPraatApplication = & theForegroundPraatApplication;
			theCurrentPraatObjects = & theForegroundPraatObjects;
			theCurrentPraatPicture = & theForegroundPraatPicture;
		}
		Graphics_resetWsViewport (_ps, x1DCold, x2DCold, y1DCold, y2DCold);
		Graphics_setWsWindow (_ps, x1NDCold, x2NDCold, y1NDCold, y2NDCold);
		Graphics_setViewport (_ps, 0, 1, 0, 1);
		Graphics_setWindow (_ps, 0, 1, 0, 1);
		Graphics_setTextAlignment (_ps, Graphics_LEFT, Graphics_BOTTOM);
	}
	_previousBottomSpacing = bottomSpacing;
	forget (interpreter);
	Melder_free (text);
	return 1;
}

void HyperPage::draw () {}

static void print (void *void_me, Graphics graphics) {
	HyperPage *editor= (HyperPage *)void_me;
	editor->_ps = graphics;
	Graphics_setDollarSignIsCode (graphics, TRUE);
	Graphics_setAtSignIsLink (graphics, TRUE);
	editor->_printing = TRUE;
	editor->initSheetOfPaper ();
	editor->draw ();
	editor->_printing = FALSE;
}

static void gui_drawingarea_cb_expose (void *void_me, GuiDrawingAreaExposeEvent event) {
	(void) event;
	HyperPage *editor= (HyperPage *)void_me;
	if (editor->_g == NULL) return;   // Could be the case in the very beginning.
	Graphics_clearWs (editor->_g);
	editor->initScreen ();
	editor->draw ();
	if (editor->_entryHint && editor->_entryPosition) {
		Melder_free (editor->_entryHint);
		editor->_top = 5.0 * (PAGE_HEIGHT - editor->_entryPosition);
		if (editor->_top < 0) editor->_top = 0;
		Graphics_clearWs (editor->_g);
		editor->initScreen ();
		editor->draw ();
		editor->updateVerticalScrollBar ();
	}
}

static void gui_drawingarea_cb_click (void *void_me, GuiDrawingAreaClickEvent event) {
	HyperPage *editor= (HyperPage *)void_me;
	if (editor->_g == NULL) return;   // Could be the case in the very beginning.
	if (gtk && event -> type != BUTTON_PRESS) return;
	if (! editor->_links) return;
	for (long ilink = 1; ilink <= editor->_links -> size; ilink ++) {
		HyperLink link = (structHyperLink*)editor->_links -> item [ilink];
		if (event -> y > link -> y2DC && event -> y < link -> y1DC && event -> x > link -> x1DC && event -> x < link -> x2DC) {
			editor->saveHistory (editor->_currentPageTitle);
			if (! editor->goToPage (link -> name)) {
				/* Allow for a returned 0 just to mean: 'do not jump'. */
				if (Melder_hasError ()) Melder_flushError (NULL);
			}
			return;
		}
	}
}

static int menu_cb_postScriptSettings (EDITOR_ARGS) {
	Printer_postScriptSettings ();
	return 1;
}

#ifdef macintosh
static int menu_cb_pageSetup (EDITOR_ARGS) {
	Printer_pageSetup ();
	return 1;
}
#endif

static int menu_cb_print (EDITOR_ARGS) {
	HyperPage *editor = (HyperPage *)editor_me;
	EDITOR_FORM (L"Print", 0)
		SENTENCE (L"Left or inside header", L"")
		SENTENCE (L"Middle header", L"")
		LABEL (L"", L"Right or outside header:")
		TEXTFIELD (L"Right or outside header", L"")
		SENTENCE (L"Left or inside footer", L"")
		SENTENCE (L"Middle footer", L"")
		SENTENCE (L"Right or outside footer", L"")
		BOOLEAN (L"Mirror even/odd headers", TRUE)
		INTEGER (L"First page number", L"0 (= no page numbers)")
	EDITOR_OK
		editor->defaultHeaders (cmd);
		if (editor->_pageNumber) SET_INTEGER (L"First page number", editor->_pageNumber + 1)
	EDITOR_DO
		editor->_insideHeader = GET_STRING (L"Left or inside header");
		editor->_middleHeader = GET_STRING (L"Middle header");
		editor->_outsideHeader = GET_STRING (L"Right or outside header");
		editor->_insideFooter = GET_STRING (L"Left or inside footer");
		editor->_middleFooter = GET_STRING (L"Middle footer");
		editor->_outsideFooter = GET_STRING (L"Right or outside footer");
		editor->_mirror = GET_INTEGER (L"Mirror even/odd headers");
		editor->_pageNumber = GET_INTEGER (L"First page number");
		Printer_print (print, editor);
	EDITOR_END
}

static int menu_cb_font (EDITOR_ARGS) {
	HyperPage *editor = (HyperPage *)editor_me;
	EDITOR_FORM (L"Font", 0)
		RADIO (L"Font", 1)
			RADIOBUTTON (L"Times")
			RADIOBUTTON (L"Helvetica")
	EDITOR_OK
		SET_INTEGER (L"Font", editor->_font == kGraphics_font_TIMES ? 1 :
				editor->_font == kGraphics_font_HELVETICA ? 2 : editor->_font == kGraphics_font_PALATINO ? 3 : 1);
	EDITOR_DO
		int font = GET_INTEGER (L"Font");
		prefs_font = (kGraphics_font)(editor->_font = font == 1 ? kGraphics_font_TIMES : kGraphics_font_HELVETICA);
		if (editor->_g) Graphics_updateWs (editor->_g);
	EDITOR_END
}

void HyperPage::updateSizeMenu () {
	GuiMenuItem_check (_fontSizeButton_10, _fontSize == 10);
	GuiMenuItem_check (_fontSizeButton_12, _fontSize == 12);
	GuiMenuItem_check (_fontSizeButton_14, _fontSize == 14);
	GuiMenuItem_check (_fontSizeButton_18, _fontSize == 18);
	GuiMenuItem_check (_fontSizeButton_24, _fontSize == 24);
}
void HyperPage::setFontSize (int fontSize) {
	prefs_fontSize = _fontSize = fontSize;
	if (_g) Graphics_updateWs (_g);
	updateSizeMenu ();
}

static int menu_cb_10 (EDITOR_ARGS) { ((HyperPage *)editor_me)->setFontSize (10); return 1; }
static int menu_cb_12 (EDITOR_ARGS) { ((HyperPage *)editor_me)->setFontSize (12); return 1; }
static int menu_cb_14 (EDITOR_ARGS) { ((HyperPage *)editor_me)->setFontSize (14); return 1; }
static int menu_cb_18 (EDITOR_ARGS) { ((HyperPage *)editor_me)->setFontSize (18); return 1; }
static int menu_cb_24 (EDITOR_ARGS) { ((HyperPage *)editor_me)->setFontSize (24); return 1; }

static int menu_cb_fontSize (EDITOR_ARGS) {
	HyperPage *editor = (HyperPage *)editor_me;
	EDITOR_FORM (L"Font size", 0)
		NATURAL (L"Font size (points)", L"12")
	EDITOR_OK
		SET_INTEGER (L"Font size", editor->_fontSize)
	EDITOR_DO
		editor->setFontSize (GET_INTEGER (L"Font size"));
	EDITOR_END
}

static int menu_cb_searchForPage (EDITOR_ARGS) {
	HyperPage *editor = (HyperPage *)editor_me;
	EDITOR_FORM (L"Search for page", 0)
		TEXTFIELD (L"Page", L"a")
	EDITOR_OK
	EDITOR_DO
		if (! editor->goToPage (GET_STRING (L"Page"))) return 0;
	EDITOR_END
}

/********************************************************************************
 *
 * The vertical scroll bar controls and/or mirrors
 * the position of the viewable area within the page.
 * A page can be PAGE_HEIGHT inches high, so '_top' (and the scroll-bar 'value')
 * may take on values between 0 and PAGE_HEIGHT * 5 (fifth inches).
 * Hence, the 'minimum' is 0.
 * The viewable area shows a certain number of fifth inches;
 * hence the 'sliderSize' is height / resolution * 5,
 * and the 'maximum' is PAGE_HEIGHT * 5.
 * The 'increment' is 1, so the arrows move the page by one fifth of an inch.
 * The 'pageIncrement' is sliderSize - 1.
 */

void HyperPage::createVerticalScrollBar (GuiObject parent) {
	#if gtk
		int maximumScrollBarValue = (int) (PAGE_HEIGHT * 5);
		GtkObject *adj = gtk_adjustment_new (1, 1, maximumScrollBarValue, 1, 1, maximumScrollBarValue - 1);
		_verticalScrollBar = gtk_vscrollbar_new (GTK_ADJUSTMENT (adj));
		GuiObject_show (_verticalScrollBar);
		gtk_box_pack_end (GTK_BOX (parent), _verticalScrollBar, false, false, 3);
	#elif motif
		// TODO: Kan dit niet een algemele gui klasse worden?
		_verticalScrollBar = XtVaCreateManagedWidget ("verticalScrollBar",
			xmScrollBarWidgetClass, parent, XmNorientation, XmVERTICAL,
			XmNrightAttachment, XmATTACH_FORM,
			XmNtopAttachment, XmATTACH_FORM,
				XmNtopOffset, Machine_getMenuBarHeight () + Machine_getTextHeight () + 12,
			XmNbottomAttachment, XmATTACH_FORM, XmNbottomOffset, Machine_getScrollBarWidth (),
			XmNwidth, Machine_getScrollBarWidth (),
			XmNminimum, 0, XmNmaximum, (int) (PAGE_HEIGHT * 5),
			XmNsliderSize, 25, XmNvalue, 0,
			XmNincrement, 1, XmNpageIncrement, 24,
			NULL);
	#endif
}

void HyperPage::updateVerticalScrollBar ()
/* We cannot call this immediately after creation. */
/* This has to be called after changing '_topParagraph'. */
{
	Dimension width, height;
	int sliderSize;
	#if motif
		XtVaGetValues (_drawingArea, XmNwidth, & width, XmNheight, & height, NULL);
	#endif
	sliderSize = 25 /*height / resolution * 5*/;   /* Don't change slider unless you clip value! */
	#if gtk
		GtkAdjustment *adj = gtk_range_get_adjustment (GTK_RANGE (_verticalScrollBar));
		adj -> page_size = sliderSize;
		//gtk_adjustment_set_value (adj, value);
		gtk_adjustment_changed (adj);
		gtk_range_set_increments (GTK_RANGE (_verticalScrollBar), 1, sliderSize - 1);
	#elif motif
		XmScrollBarSetValues (_verticalScrollBar, _top, sliderSize, 1, sliderSize - 1, False);
	#endif
	_history [_historyPointer]. top = 0/*_top*/;
}

static int menu_cb_pageUp (EDITOR_ARGS) {
	HyperPage *editor = (HyperPage *)editor_me;
	int value, sliderSize, incr, pincr;
	if (! editor->_verticalScrollBar) return 0;
	#if	gtk
		value = gtk_range_get_value (GTK_RANGE (editor->_verticalScrollBar));
		sliderSize = 1;
		pincr = PAGE_HEIGHT * 5 - 1;
	#elif motif
		XmScrollBarGetValues (editor->_verticalScrollBar, & value, & sliderSize, & incr, & pincr);
	#endif
	value -= pincr;
	if (value < 0) value = 0;
	if (value != editor->_top) {
		editor->_top = value;
		Graphics_clearWs (editor->_g);
		editor->initScreen ();
		editor->draw ();   /* Do not wait for expose event. */
		editor->updateVerticalScrollBar ();
	}
	return 1;
}

static int menu_cb_pageDown (EDITOR_ARGS) {
	HyperPage *editor = (HyperPage *)editor_me;
	int value, sliderSize, incr, pincr;
	if (! editor->_verticalScrollBar) return 0;
	#if	gtk
		value = gtk_range_get_value (GTK_RANGE (editor->_verticalScrollBar));
		sliderSize = 1;
		pincr = PAGE_HEIGHT * 5 - 1;
	#elif motif
		XmScrollBarGetValues (editor->_verticalScrollBar, & value, & sliderSize, & incr, & pincr);
	#endif
	value += pincr;
	if (value > (int) (PAGE_HEIGHT * 5) - sliderSize) value = (int) (PAGE_HEIGHT * 5) - sliderSize;
	if (value != editor->_top) {
		editor->_top = value;
		Graphics_clearWs (editor->_g);
		editor->initScreen ();
		editor->draw ();   /* Do not wait for expose event. */
		editor->updateVerticalScrollBar ();
	}
	return 1;
}

int HyperPage::do_back () {
	if (_historyPointer <= 0) return 1;
	wchar_t *page = Melder_wcsdup_f (_history [-- _historyPointer]. page);   /* Temporary, because pointer will be moved. */
	int top = _history [_historyPointer]. top;
	if (goToPage (page)) {
		_top = top;
		clear ();
		updateVerticalScrollBar ();
	} else {
		Melder_free (page);
		return 0;
	}
	Melder_free (page);
	return 1;
}

static int menu_cb_back (EDITOR_ARGS) {
	HyperPage *editor = (HyperPage *)editor_me;
	if (! editor->do_back ()) return 0;
	return 1;
}

static void gui_button_cb_back (void *void_me, GuiButtonEvent event) {
	(void) event;
	HyperPage *editor= (HyperPage *)void_me;
	if (! editor->do_back ()) Melder_flushError (NULL);
}

int HyperPage::do_forth () {
	wchar_t *page;
	int top;
	if (_historyPointer >= 19 || ! _history [_historyPointer + 1]. page) return 1;
	page = Melder_wcsdup_f (_history [++ _historyPointer]. page);
	top = _history [_historyPointer]. top;
	if (goToPage (page)) {
		_top = top;
		clear ();
		updateVerticalScrollBar ();
	} else {
		Melder_free (page);
		return 0;
	}
	Melder_free (page);
	return 1;
}

static int menu_cb_forth (EDITOR_ARGS) {
	HyperPage *editor = (HyperPage *)editor_me;
	if (! editor->do_forth ()) return 0;
	return 1;
}

static void gui_button_cb_forth (void *void_me, GuiButtonEvent event) {
	(void) event;
	HyperPage *editor= (HyperPage *)void_me;
	if (! editor->do_forth ()) Melder_flushError (NULL);
}

void HyperPage::createMenus () {
	EditorMenu *menu = getMenu (L"File");
	menu->addCommand (L"PostScript settings...", 0, menu_cb_postScriptSettings);
	#ifdef macintosh
		menu->addCommand (L"Page setup...", 0, menu_cb_pageSetup);
	#endif
	menu->addCommand (L"Print page...", 'P', menu_cb_print);
	menu->addCommand (L"-- close --", 0, NULL);

	if (_hasHistory) {
		menu = addMenu (L"Go to", 0);
		menu->addCommand (L"Search for page...", 0, menu_cb_searchForPage);
		menu->addCommand (L"Back", GuiMenu_OPTION | GuiMenu_LEFT_ARROW, menu_cb_back);
		menu->addCommand (L"Forward", GuiMenu_OPTION | GuiMenu_RIGHT_ARROW, menu_cb_forth);
		menu->addCommand (L"-- page --", 0, NULL);
		menu->addCommand (L"Page up", GuiMenu_PAGE_UP, menu_cb_pageUp);
		menu->addCommand (L"Page down", GuiMenu_PAGE_DOWN, menu_cb_pageDown);
	}

	menu = addMenu (L"Font", 0);
	menu->addCommand (L"Font size...", 0, menu_cb_fontSize);
	_fontSizeButton_10 = menu->addCommand (L"10", GuiMenu_CHECKBUTTON, menu_cb_10) -> _itemWidget;
	_fontSizeButton_12 = menu->addCommand (L"12", GuiMenu_CHECKBUTTON, menu_cb_12) -> _itemWidget;
	_fontSizeButton_14 = menu->addCommand (L"14", GuiMenu_CHECKBUTTON, menu_cb_14) -> _itemWidget;
	_fontSizeButton_18 = menu->addCommand (L"18", GuiMenu_CHECKBUTTON, menu_cb_18) -> _itemWidget;
	_fontSizeButton_24 = menu->addCommand (L"24", GuiMenu_CHECKBUTTON, menu_cb_24) -> _itemWidget;
	menu->addCommand (L"-- font --", 0, NULL);
	menu->addCommand (L"Font...", 0, menu_cb_font);
}

static void gui_button_cb_previousPage (void *void_me, GuiButtonEvent event) {
	(void) event;
	HyperPage *editor= (HyperPage *)void_me;
	editor->goToPage_i (editor->getCurrentPageNumber () > 1 ?
		editor->getCurrentPageNumber () - 1 : editor->getNumberOfPages ());
}

static void gui_button_cb_nextPage (void *void_me, GuiButtonEvent event) {
	(void) event;
	HyperPage *editor= (HyperPage *)void_me;
	editor->goToPage_i (editor->getCurrentPageNumber () < editor->getNumberOfPages () ?
		editor->getCurrentPageNumber () + 1 : 1);
}

void HyperPage::createChildren () {
	int height = Machine_getTextHeight ();
	int y = Machine_getMenuBarHeight () + 4;

	#if gtk
		_holder = gtk_hbox_new (FALSE, 0);
		gtk_box_pack_start (GTK_BOX (_dialog), _holder, false, false, 0);
		GuiObject_show (_holder);
	#elif motif
		_holder = _dialog;
	#endif

	/***** Create navigation buttons. *****/

	if (_hasHistory) {
		GuiButton_createShown (_holder, 4, 48, y, y + height,
			L"<", gui_button_cb_back, this, 0);
		GuiButton_createShown (_holder, 54, 98, y, y + height,
			L">", gui_button_cb_forth, this, 0);
	}
	if (_isOrdered) {
		GuiButton_createShown (_holder, 174, 218, y, y + height,
			L"< 1", gui_button_cb_previousPage, this, 0);
		GuiButton_createShown (_holder, 224, 268, y, y + height,
			L"1 >", gui_button_cb_nextPage, this, 0);
	}
	#if gtk
		GuiObject scrollBox = gtk_hbox_new (false, 0);
		gtk_box_pack_end (GTK_BOX (_dialog), scrollBox, true, true, 0);
		_drawingArea = GuiDrawingArea_create (GTK_WIDGET (scrollBox), 0, 600, 0, 800,
			gui_drawingarea_cb_expose, gui_drawingarea_cb_click, NULL, gui_drawingarea_cb_resize, this, GuiDrawingArea_BORDER);
		gtk_widget_set_double_buffered (_drawingArea, FALSE);
		gtk_box_pack_start (GTK_BOX (scrollBox), _drawingArea, true, true, 0);
		createVerticalScrollBar (scrollBox);
		GuiObject_show (_drawingArea);
		GuiObject_show (scrollBox);
	#elif motif
		/***** Create scroll bar. *****/

		createVerticalScrollBar (_dialog);

		/***** Create drawing area. *****/
		_drawingArea = GuiDrawingArea_createShown (_dialog, 0, - Machine_getScrollBarWidth (), y + height + 8, - Machine_getScrollBarWidth (),
			gui_drawingarea_cb_expose, gui_drawingarea_cb_click, NULL, gui_drawingarea_cb_resize, me, GuiDrawingArea_BORDER);
	#endif
}

void HyperPage::clear () {
	Graphics_updateWs (_g);
	forget (_links);
}

void HyperPage::dataChanged () {
	int oldError = Melder_hasError ();
	goToPage (_currentPageTitle);
	if (Melder_hasError () && ! oldError) Melder_flushError (NULL);
	clear ();
	updateVerticalScrollBar ();
}
long HyperPage::getNumberOfPages () {
	return 0;
}
long HyperPage::getCurrentPageNumber () {
	return 0;
}
void HyperPage::defaultHeaders (EditorCommand *cmd) {
	(void) cmd;
}

int HyperPage::_hasHistory = FALSE;
int HyperPage::_isOrdered = FALSE;

// FIXME weird inherited methods?
int HyperPage::goToPage (const wchar_t *title) {
	clear (); return 0;
	/*switch (goToPage (title)) {
		case -1: return 0;
		case 0: clear (); return 0;
	}
	saveHistory (title);   // Last chance: clear() will destroy "title" !!!
	Melder_free (_currentPageTitle);
	_currentPageTitle = Melder_wcsdup_f (title);
	_top = 0;
	clear ();
	updateVerticalScrollBar ();   // Scroll to the top (_top == 0).
	return 1;*/
}

int HyperPage::goToPage_i (long i) {
	clear (); return 0;
	/*if (! goToPage_i (i)) { clear (); return 0; }
	_top = 0;
	clear ();
	updateVerticalScrollBar ();   // Scroll to the top (_top == 0).
	return 1;*/
}

void HyperPage::setEntryHint (const wchar_t *hint) {
	Melder_free (_entryHint);
	_entryHint = Melder_wcsdup_f (hint);
}

/* End of file HyperPage.c */
