#ifndef _HyperPage_h_
#define _HyperPage_h_
/* HyperPage.h
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
 * pb 2011/03/02
 */

#ifndef _Editor_h_
	#include "Editor.h"
#endif
#ifndef _Collection_h_
	#include "Collection.h"
#endif
#ifndef _Graphics_h_
	#include "Graphics.h"
#endif

#define HyperLink_members Data_members \
	double x1DC, x2DC, y1DC, y2DC;
#define HyperLink_methods Data_methods
class_create (HyperLink, Data);

#define HyperPage_ADD_BORDER  1
#define HyperPage_USE_ENTRY_HINT  2

HyperLink HyperLink_create (const wchar_t *name, double x1, double x2, double y1, double y2);

class HyperPage : public Editor {
  public:
	static void prefs (void);

	static int _hasHistory, _isOrdered;

	HyperPage (GuiObject parent, const wchar_t *title, Any data);
	virtual ~HyperPage ();

	virtual const wchar_t * type () { return L"HyperPage"; }
	virtual bool isEditable () { return false; }
	virtual void clear ();
	virtual int any (const wchar_t *text, int font, int size, int style, double minFooterDistance,
		double x, double secondIndent, double topSpacing, double bottomSpacing, unsigned long method);
	virtual int pageTitle (const wchar_t *title);
	virtual int intro (const wchar_t *text);
	virtual int entry (const wchar_t *title);
	virtual int paragraph (const wchar_t *text);
	virtual int listItem (const wchar_t *text);
	virtual int listItem1 (const wchar_t *text);
	virtual int listItem2 (const wchar_t *text);
	virtual int listItem3 (const wchar_t *text);
	virtual int listTag (const wchar_t *text);
	virtual int listTag1 (const wchar_t *text);
	virtual int listTag2 (const wchar_t *text);
	virtual int listTag3 (const wchar_t *text);
	virtual int definition (const wchar_t *text);
	virtual int definition1 (const wchar_t *text);
	virtual int definition2 (const wchar_t *text);
	virtual int definition3 (const wchar_t *text);
	virtual int code (const wchar_t *text);
	virtual int code1 (const wchar_t *text);
	virtual int code2 (const wchar_t *text);
	virtual int code3 (const wchar_t *text);
	virtual int code4 (const wchar_t *text);
	virtual int code5 (const wchar_t *text);
	virtual int prototype (const wchar_t *text);
	virtual int formula (const wchar_t *formula);
	virtual int picture (double width_inches, double height_inches, void (*draw) (Graphics g));
	virtual int script (double width_inches, double height_inches, const wchar_t *script);
	virtual void setEntryHint (const wchar_t *entry);
	virtual void initSheetOfPaper ();
	virtual void updateVerticalScrollBar ();
	virtual void draw ();
	virtual void initScreen ();
	virtual void saveHistory (const wchar_t *title);
	virtual int goToPage (const wchar_t *title);
	virtual int goToPage_i (long ipage);
	virtual void defaultHeaders (EditorCommand *cmd);
	virtual void setFontSize (int fontSize);
	virtual int do_forth ();
	virtual void createMenus ();
	virtual long getCurrentPageNumber ();
	virtual long getNumberOfPages ();
	virtual void dataChanged ();
	virtual int do_back ();

	GuiObject _drawingArea, _verticalScrollBar;
	Graphics _g, _ps;
	double _x, _y, _rightMargin, _previousBottomSpacing;
	long _pageNumber;
	Collection _links;
	int _printing, _top, _mirror;
	wchar_t *_insideHeader, *_middleHeader, *_outsideHeader;
	wchar_t *_insideFooter, *_middleFooter, *_outsideFooter;
	int _font, _fontSize;
	wchar_t *_entryHint; double _entryPosition;
	struct { wchar_t *page; int top; } _history [20];
	int _historyPointer;
	wchar_t *_currentPageTitle;
	GuiObject _fontSizeButton_10, _fontSizeButton_12, _fontSizeButton_14, _fontSizeButton_18, _fontSizeButton_24;
	GuiObject _holder;
	void *_praatApplication, *_praatObjects, *_praatPicture;
	bool _scriptErrorHasBeenNotified;
	structMelderDir _rootDirectory;

  protected:
	virtual void updateSizeMenu ();
	virtual void createVerticalScrollBar (GuiObject parent);
	virtual void createChildren ();

  private:
	static void gui_drawingarea_cb_resize (void *void_me, GuiDrawingAreaResizeEvent event);
	#if gtk
	static void gui_cb_verticalScroll (GtkRange *rng, gpointer void_me);
	#else
	static void gui_cb_verticalScroll (GUI_ARGS);
	#endif
	static void gui_drawingarea_cb_expose (void *void_me, GuiDrawingAreaExposeEvent event);
	static void gui_drawingarea_cb_click (void *void_me, GuiDrawingAreaClickEvent event);
	static int menu_cb_postScriptSettings (EDITOR_ARGS);
	#ifdef macintosh
	static int menu_cb_pageSetup (EDITOR_ARGS);
	#endif
	static int menu_cb_print (EDITOR_ARGS);
	static int menu_cb_font (EDITOR_ARGS);
	static int menu_cb_10 (EDITOR_ARGS);
	static int menu_cb_12 (EDITOR_ARGS);
	static int menu_cb_14 (EDITOR_ARGS);
	static int menu_cb_18 (EDITOR_ARGS);
	static int menu_cb_24 (EDITOR_ARGS);
	static int menu_cb_fontSize (EDITOR_ARGS);
	static int menu_cb_searchForPage (EDITOR_ARGS);
	static int menu_cb_pageUp (EDITOR_ARGS);
	static int menu_cb_pageDown (EDITOR_ARGS);
	static int menu_cb_back (EDITOR_ARGS);
	static void gui_button_cb_back (void *void_me, GuiButtonEvent event);
	static int menu_cb_forth (EDITOR_ARGS);
	static void gui_button_cb_forth (void *void_me, GuiButtonEvent event);
	static void gui_button_cb_previousPage (void *void_me, GuiButtonEvent event);
	static void gui_button_cb_nextPage (void *void_me, GuiButtonEvent event);
};

/* End of file HyperPage.h */
#endif
