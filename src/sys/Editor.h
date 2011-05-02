#ifndef _Editor_h_
#define _Editor_h_
/* Editor.h
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

#ifndef _Collection_h_
	#include "Collection.h"
#endif
#ifndef _Gui_h_
	#include "Gui.h"
#endif
#ifndef _Graphics_h_
	#include "Graphics.h"
#endif
#include "Editor_enums.h"
#include "sys/EditorM.h"

#define Editor_HIDDEN  (1 << 14)

class Interpreter;
class UiForm;
class EditorMenu;
class Editor;

class EditorCommand {
  public:
	virtual ~EditorCommand ();

	Editor *_editor;
	EditorMenu *_menu;
	const wchar_t *_itemTitle;
	GuiObject _itemWidget;
	int (*_commandCallback) (Editor *editor, EditorCommand *cmd, UiForm *sendingForm, const wchar_t *sendingString, Interpreter *interpreter);
	const wchar_t *_script;
	UiForm *_dialog;
};

class EditorMenu {
  public:
	virtual ~EditorMenu ();

	EditorCommand * addCommand (const wchar_t *itemTitle, long flags,
		int (*commandCallback) (Editor *editor, EditorCommand *, UiForm *, const wchar_t *, Interpreter *));
	// virtual GuiObject getMenuWidget (); // FIXME why did this disappear?

	Editor *_editor;
	const wchar_t *_menuTitle;
	GuiObject _menuWidget;
	Ordered _commands;
};

class Editor {
  public:
	static void prefs ();

	Editor (GuiObject parent, int x, int y , int width, int height,
		const wchar_t *title, Any data);
	virtual ~Editor ();

	virtual bool hasMenuBar () { return true; }
	virtual bool canFullScreen () { return false; }
	virtual bool isEditable () { return true; }
	virtual bool isScriptable () { return true; }
	virtual const wchar_t * type () = 0;
	virtual void info ();
	virtual void nameChanged ();
	virtual void goAway ();
	virtual void dataChanged ();
	virtual void clipboardChanged (Any clipboard);
	virtual void save ();
	virtual void restore ();
	virtual void form_pictureWindow (EditorCommand *cmd);
	virtual void ok_pictureWindow (EditorCommand *cmd);
	virtual void do_pictureWindow (EditorCommand *cmd);
	virtual void form_pictureMargins (EditorCommand *cmd);
	virtual void ok_pictureMargins (EditorCommand *cmd);
	virtual void do_pictureMargins (EditorCommand *cmd);

	virtual EditorMenu * addMenu (const wchar_t *menuTitle, long flags);
	virtual EditorMenu * getMenu (const wchar_t *menuTitle);

	virtual void raise ();
	/* Raises and deiconizes the editor window. */

	virtual void changeData (Any data);
	/* Tell the Editor that the data has changed.
	   If 'data' is not NULL, this routine installs 'data' into the Editor's 'data' field.
	*/

	virtual void changeClipboard (Any data);
	/* Tell the Editor that a clipboard has changed. */

	virtual void setDestroyCallback (void (*cb) (Editor *editor, void *closure), void *closure);
	/* Makes the Editor notify client when user clicks "Close":	*/
	/* the Editor will destroy itself.				*/
	/* Use this callback to remove your references to "me".		*/

	virtual void setDataChangedCallback (void (*cb) (Editor *editor, void *closure, Any data), void *closure);
	/* Makes the Editor notify client (boss, creator, owner) when user changes data. */
	/* 'data' is the new data (if not NULL). */
	/* Most Editors will include the following line at several places, after 'data' or '*data' has changed:
		if (my dataChangedCallback)
			my dataChangedCallback (me, my dataChangedClosure, my data);
	*/

	virtual void broadcastChange ();
	/* A shortcut for the line above, with NULL for the 'data' argument, i.e. only '*data' has changed. */

	virtual void setPublishCallback (void (*cb) (Editor *editor, void *closure, Any publish), void *closure);
	virtual void setPublish2Callback (void (*cb) (Editor *editor, void *closure, Any publish1, Any publish2), void *closure);
	/*
		Makes the Editor notify client when user clicks a "Publish" button:
		the Editor should create some new Data ("publish").
		By registering this callback, the client takes responsibility for eventually removing "publish".
	*/

	virtual void save (const wchar_t *text);   /* For Undo. */

	virtual EditorCommand *getMenuCommand (const wchar_t *menuTitle, const wchar_t *itemTitle);
	virtual int doMenuCommand (const wchar_t *command, const wchar_t *arguments, Interpreter *interpreter);

	virtual void openPraatPicture ();
	virtual void closePraatPicture ();

	wchar_t *_name;
	GuiObject _parent, _shell, _dialog, _menuBar, _undoButton, _searchButton;
	Ordered _menus;
	Any _data, _previousData;   /* The data that can be displayed and edited. */
	wchar_t _undoText [100];
	Graphics _pictureGraphics;
	void (*_destroyCallback) (Editor *editor, void *closure);
	void *_destroyClosure;
	void (*_dataChangedCallback) (Editor *editor, void *closure, Any data);
	void *_dataChangedClosure;
	void (*_publishCallback) (Editor *editor, void *closure, Any publish);
	void *_publishClosure;
	void (*_publish2Callback) (Editor *editor, void *closure, Any publish1, Any publish2);
	void *_publish2Closure;

  protected:
	virtual void setMenuSensitive (const wchar_t *menu, int sensitive);

  private:
	static void gui_window_cb_goAway (void *editor);
	static int menu_cb_newScript (EDITOR_ARGS);
	static int menu_cb_openScript (EDITOR_ARGS);
	static int menu_cb_close (EDITOR_ARGS);
	static int menu_cb_undo (EDITOR_ARGS);
	static int menu_cb_settingsReport (EDITOR_ARGS);
	static int menu_cb_info (EDITOR_ARGS);

	void createMenus ();
};

#endif
/* End of file Editor.h */
