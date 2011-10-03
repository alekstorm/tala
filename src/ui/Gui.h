#ifndef _Gui_h_
#define _Gui_h_
/* Gui.h
 *
 * Copyright (C) 1993-2011 Paul Boersma
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
 * 2011/03/02
 */

#include "sys/Collection.h"

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <cairo/cairo.h>

#ifdef __cplusplus
	extern "C" {
#endif

#define GUI_ARGS  GuiObject w, XtPointer void_me, XtPointer call

#define GUI_IAM(klas)  (void) w; (void) void_me; (void) call; iam (klas);

#define Gui_LEFT_DIALOG_SPACING  20
#define Gui_RIGHT_DIALOG_SPACING  20
#define Gui_TOP_DIALOG_SPACING  14
#define Gui_BOTTOM_DIALOG_SPACING  20
#define Gui_HORIZONTAL_DIALOG_SPACING  12
#define Gui_VERTICAL_DIALOG_SPACING_SAME  12
#define Gui_VERTICAL_DIALOG_SPACING_DIFFERENT  20
#define Gui_TEXTFIELD_HEIGHT  Machine_getTextHeight ()
#define Gui_LABEL_HEIGHT  16
#define Gui_RADIOBUTTON_HEIGHT  18
#define Gui_RADIOBUTTON_SPACING  8
#define Gui_CHECKBUTTON_HEIGHT  20
#define Gui_LABEL_SPACING  8
#define Gui_OPTIONMENU_HEIGHT  20
#define Gui_PUSHBUTTON_HEIGHT  20
#define Gui_OK_BUTTON_WIDTH  69
#define Gui_CANCEL_BUTTON_WIDTH  69
#define Gui_APPLY_BUTTON_WIDTH  69

#define Gui_AUTOMATIC  -32768
#define Gui_HOMOGENEOUS  1

typedef GMainContext *AppContext;
typedef void *XtPointer;
typedef gint Dimension;
typedef gboolean Boolean;
#define True 1
#define False 0
typedef GtkWidget *GuiObject;

/* Button layout and state: */
#define GuiMenu_INSENSITIVE  (1 << 8)
#define GuiMenu_CHECKBUTTON  (1 << 9)
#define GuiMenu_TOGGLE_ON  (1 << 10)
#define GuiMenu_ATTRACTIVE  (1 << 11)
#define GuiMenu_RADIO_FIRST  (1 << 12)
#define GuiMenu_RADIO_NEXT  (1 << 13)
#define GuiMenu_BUTTON_STATE_MASK  (GuiMenu_INSENSITIVE|GuiMenu_CHECKBUTTON|GuiMenu_TOGGLE_ON|GuiMenu_ATTRACTIVE|GuiMenu_RADIO_FIRST|GuiMenu_RADIO_NEXT)

/* Accelerators: */
#define GuiMenu_OPTION  (1 << 21)
#define GuiMenu_SHIFT  (1 << 22)
#define GuiMenu_COMMAND  (1 << 23)
#define GuiMenu_LEFT_ARROW  1
#define GuiMenu_RIGHT_ARROW  2
#define GuiMenu_UP_ARROW  3
#define GuiMenu_DOWN_ARROW  4
#define GuiMenu_PAUSE  5
#define GuiMenu_DELETE  6
#define GuiMenu_INSERT  7
#define GuiMenu_BACKSPACE  8
#define GuiMenu_TAB  9
#define GuiMenu_LINEFEED  10
#define GuiMenu_HOME  11
#define GuiMenu_END  12
#define GuiMenu_ENTER  13
#define GuiMenu_PAGE_UP  14
#define GuiMenu_PAGE_DOWN  15
#define GuiMenu_ESCAPE  16
#define GuiMenu_F1  17
#define GuiMenu_F2  18
#define GuiMenu_F3  19
#define GuiMenu_F4  20
#define GuiMenu_F5  21
#define GuiMenu_F6  22
#define GuiMenu_F7  23
#define GuiMenu_F8  24
#define GuiMenu_F9  25
#define GuiMenu_F10  26
#define GuiMenu_F11  27
#define GuiMenu_F12  28

GuiObject Gui_addMenuBar (GuiObject form);
int Gui_getResolution (GuiObject widget);

/* GuiButton creation flags: */
#define GuiButton_DEFAULT  1
#define GuiButton_CANCEL  2
#define GuiButton_INSENSITIVE  4
#define GuiButton_ATTRACTIVE  8
typedef struct structGuiButtonEvent {
	GuiObject button;
	bool shiftKeyPressed, commandKeyPressed, optionKeyPressed, extraControlKeyPressed;
} *GuiButtonEvent;
GuiObject GuiButton_create (GuiObject parent, int left, int right, int top, int bottom,
	const wchar_t *buttonText, void (*clickedCallback) (void *boss, GuiButtonEvent event), void *boss, unsigned long flags);
GuiObject GuiButton_createShown (GuiObject parent, int left, int right, int top, int bottom,
	const wchar_t *buttonText, void (*clickedCallback) (void *boss, GuiButtonEvent event), void *boss, unsigned long flags);
void GuiButton_setString (GuiObject widget, const wchar_t *text);   // rarely used

/* GuiCheckButton creation flags: */
#define GuiCheckButton_SET  1
#define GuiCheckButton_INSENSITIVE  2
typedef struct structGuiCheckButtonEvent {
	GuiObject toggle;
} *GuiCheckButtonEvent;
GuiObject GuiCheckButton_create (GuiObject parent, int left, int right, int top, int bottom,
	const wchar_t *buttonText, void (*valueChangedCallback) (void *boss, GuiCheckButtonEvent event), void *valueChangedBoss, unsigned long flags);
GuiObject GuiCheckButton_createShown (GuiObject parent, int left, int right, int top, int bottom,
	const wchar_t *buttonText, void (*valueChangedCallback) (void *boss, GuiCheckButtonEvent event), void *valueChangedBoss, unsigned long flags);
bool GuiCheckButton_getValue (GuiObject widget);
void GuiCheckButton_setValue (GuiObject widget, bool value);

GuiObject GuiColumn_createShown (GuiObject parent, unsigned long flags);
GuiObject GuiRow_createShown (GuiObject parent, unsigned long flags);

/* GuiDialog creation flags: */
#define GuiDialog_MODAL  1
GuiObject GuiDialog_create (GuiObject parent, int x, int y, int width, int height,
	const wchar_t *title, void (*goAwayCallback) (void *goAwayBoss), void *goAwayBoss, unsigned long flags);
GuiObject GuiDialog_getButtonArea (GuiObject widget);

SortedSetOfString GuiFileSelect_getInfileNames (GuiObject parent, const wchar_t *title, bool allowMultipleFiles);
wchar_t * GuiFileSelect_getOutfileName (GuiObject parent, const wchar_t *title, const wchar_t *defaultName);
wchar_t * GuiFileSelect_getDirectoryName (GuiObject parent, const wchar_t *title);

/* GuiDrawingArea creation flags: */
#define GuiDrawingArea_BORDER  1
enum mouse_events { MOTION_NOTIFY = 1, BUTTON_PRESS, BUTTON_RELEASE };
typedef struct structGuiDrawingAreaExposeEvent {
	GuiObject widget;
	int x, y, width, height;
} *GuiDrawingAreaExposeEvent;
typedef struct structGuiDrawingAreaClickEvent {
	GuiObject widget;
	int x, y;
	bool shiftKeyPressed, commandKeyPressed, optionKeyPressed, extraControlKeyPressed;
	int button;
	enum mouse_events type;
} *GuiDrawingAreaClickEvent;
typedef struct structGuiDrawingAreaKeyEvent {
	GuiObject widget;
	wchar_t key;
	bool shiftKeyPressed, commandKeyPressed, optionKeyPressed, extraControlKeyPressed;
} *GuiDrawingAreaKeyEvent;
typedef struct structGuiDrawingAreaResizeEvent {
	GuiObject widget;
	int width, height;
} *GuiDrawingAreaResizeEvent;
GuiObject GuiDrawingArea_create (GuiObject parent, int left, int right, int top, int bottom,
	void (*exposeCallback) (void *boss, GuiDrawingAreaExposeEvent event),
	void (*clickCallback) (void *boss, GuiDrawingAreaClickEvent event),
	void (*keyCallback) (void *boss, GuiDrawingAreaKeyEvent event),
	void (*resizeCallback) (void *boss, GuiDrawingAreaResizeEvent event), void *boss,
	unsigned long flags);
GuiObject GuiDrawingArea_createShown (GuiObject parent, int left, int right, int top, int bottom,
	void (*exposeCallback) (void *boss, GuiDrawingAreaExposeEvent event),
	void (*clickCallback) (void *boss, GuiDrawingAreaClickEvent event),
	void (*keyCallback) (void *boss, GuiDrawingAreaKeyEvent event),
	void (*resizeCallback) (void *boss, GuiDrawingAreaResizeEvent event), void *boss,
	unsigned long flags);
void GuiDrawingArea_setExposeCallback (GuiObject widget, void (*callback) (void *boss, GuiDrawingAreaExposeEvent event), void *boss);
void GuiDrawingArea_setClickCallback (GuiObject widget, void (*callback) (void *boss, GuiDrawingAreaClickEvent event), void *boss);

/* GuiLabel creation flags: */
#define GuiLabel_CENTRE  1
#define GuiLabel_RIGHT  2
GuiObject GuiLabel_create (GuiObject parent, int left, int right, int top, int bottom,
	const wchar_t *labelText, unsigned long flags);
GuiObject GuiLabel_createShown (GuiObject parent, int left, int right, int top, int bottom,
	const wchar_t *labelText, unsigned long flags);
void GuiLabel_setString (GuiObject widget, const wchar_t *text);

typedef struct structGuiListEvent {
	GuiObject list;
} *GuiListEvent;
GuiObject GuiList_create (GuiObject parent, int left, int right, int top, int bottom, bool allowMultipleSelection, const wchar_t *header);
GuiObject GuiList_createShown (GuiObject parent, int left, int right, int top, int bottom, bool allowMultipleSelection, const wchar_t *header);
void GuiList_deleteAllItems (GuiObject me);
void GuiList_deleteItem (GuiObject me, long position);
void GuiList_deselectAllItems (GuiObject me);
void GuiList_deselectItem (GuiObject me, long position);
long GuiList_getBottomPosition (GuiObject me);
long GuiList_getNumberOfItems (GuiObject me);
long * GuiList_getSelectedPositions (GuiObject me, long *numberOfSelected);
long GuiList_getTopPosition (GuiObject me);
void GuiList_insertItem (GuiObject me, const wchar_t *itemText, long position);
void GuiList_replaceItem (GuiObject me, const wchar_t *itemText, long position);
void GuiList_setTopPosition (GuiObject me, long topPosition);
void GuiList_selectItem (GuiObject me, long position);
void GuiList_setSelectionChangedCallback (GuiObject me, void (*callback) (void *boss, GuiListEvent event), void *boss);
void GuiList_setDoubleClickCallback (GuiObject me, void (*callback) (void *boss, GuiListEvent event), void *boss);

GuiObject GuiMenuBar_addMenu (GuiObject bar, const wchar_t *title, long flags);
GuiObject GuiMenuBar_addMenu2 (GuiObject bar, const wchar_t *title, long flags, GuiObject *menuTitle);
GuiObject GuiMenuBar_addMenu3 (GuiObject parent, const wchar_t *title, long flags, GuiObject *button);

/* Flags is a combination of the above defines. */

GuiObject GuiMenu_addItem (GuiObject menu, const wchar_t *title, long flags,
	void (*commandCallback) (GuiObject, XtPointer, XtPointer), const void *closure);
/* Flags is a combination of the above defines. */
GuiObject GuiMenu_addSeparator (GuiObject menu);
void GuiMenuItem_check (GuiObject menuItem, bool check);

/* GuiRadioButton creation flags: */
#define GuiRadioButton_SET  1
#define GuiRadioButton_INSENSITIVE  2
typedef struct structGuiRadioButtonEvent {
	GuiObject toggle;
} *GuiRadioButtonEvent;
GuiObject GuiRadioButton_create (GuiObject parent, int left, int right, int top, int bottom,
	const wchar_t *buttonText, void (*valueChangedCallback) (void *boss, GuiRadioButtonEvent event), void *valueChangedBoss, unsigned long flags);
GuiObject GuiRadioButton_createShown (GuiObject parent, int left, int right, int top, int bottom,
	const wchar_t *buttonText, void (*valueChangedCallback) (void *boss, GuiRadioButtonEvent event), void *valueChangedBoss, unsigned long flags);
bool GuiRadioButton_getValue (GuiObject widget);
void GuiRadioButton_setValue (GuiObject widget, bool value);

void * GuiRadioButton_getGroup (GuiObject widget);
void GuiRadioButton_setGroup (GuiObject widget, void *group);

typedef struct structGuiTextEvent {
	GuiObject text;
} *GuiTextEvent;

/* GuiText creation flags: */
#define GuiText_SCROLLED  1
#define GuiText_MULTILINE  2
#define GuiText_WORDWRAP  4
#define GuiText_NONEDITABLE  8
GuiObject GuiText_create (GuiObject parent, int left, int right, int top, int bottom, unsigned long flags);
GuiObject GuiText_createShown (GuiObject parent, int left, int right, int top, int bottom, unsigned long flags);
void GuiText_copy (GuiObject widget);
void GuiText_cut (GuiObject widget);
wchar_t * GuiText_getSelection (GuiObject widget);
wchar_t * GuiText_getString (GuiObject widget);
wchar_t * GuiText_getStringAndSelectionPosition (GuiObject widget, long *first, long *last);
void GuiText_paste (GuiObject widget);
void GuiText_redo (GuiObject widget);
void GuiText_remove (GuiObject widget);
void GuiText_replace (GuiObject widget, long from_pos, long to_pos, const wchar_t *value);
void GuiText_scrollToSelection (GuiObject widget);
void GuiText_setChangeCallback (GuiObject widget, void (*changeCallback) (void *boss, GuiTextEvent event), void *changeBoss);
void GuiText_setFontSize (GuiObject widget, int size);
void GuiText_setRedoItem (GuiObject widget, GuiObject item);
void GuiText_setSelection (GuiObject widget, long first, long last);
void GuiText_setString (GuiObject widget, const wchar_t *text);
void GuiText_setUndoItem (GuiObject widget, GuiObject item);
void GuiText_undo (GuiObject widget);
void GuiText_updateChangeCountAfterSave (GuiObject widget);

/* GuiWindow creation flags: */
#define GuiWindow_FULLSCREEN  1
GuiObject GuiWindow_create (GuiObject parentOfShell, int x, int y, int width, int height,
	const wchar_t *title, void (*goAwayCallback) (void *goAwayBoss), void *goAwayBoss, unsigned long flags);
	// returns a Form widget that has a new Shell parent.
void GuiWindow_setTitle (GuiObject shell, const wchar_t *title);
int GuiWindow_setDirty (GuiObject shell, int dirty);
/*
	Purpose: like on MacOSX you get this little dot in the red close button,
		and the window proxy icon dims.
	Return value:
		TRUE if the system supports this feature, FALSE if not;
		the point of this is that you can use a different user feedback strategy, like appending
		the text "(modified)" to the window title, if this feature is not supported.
*/
void GuiWindow_setFile (GuiObject shell, MelderFile file);
/*
	Purpose: set the window title, and (on MacOS X) the window proxy icon and the window path menu.
*/
void GuiWindow_drain (GuiObject me);
/*
	Purpose: drain the double graphics buffer.
*/

void GuiObject_destroy (GuiObject me);
long GuiObject_getHeight (GuiObject me);
long GuiObject_getWidth (GuiObject me);
long GuiObject_getX (GuiObject me);
long GuiObject_getY (GuiObject me);
void GuiObject_hide (GuiObject me);
void GuiObject_move (GuiObject me, long x, long y);
GuiObject GuiObject_parent (GuiObject w);
void GuiObject_setSensitive (GuiObject me, bool sensitive);
void GuiObject_show (GuiObject me);
void GuiObject_size (GuiObject me, long width, long height);

/********** EVENTS **********/

void Gui_setOpenDocumentCallback (int (*openDocumentCallback) (MelderFile file));
void Gui_setQuitApplicationCallback (int (*quitApplicationCallback) (void));

#ifdef __cplusplus
	}
#endif

/* End of file Gui.h */
#endif
