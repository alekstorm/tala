#ifndef _UiFile_h_
#define _UiFile_h_
/* UiFile.h
 *
 * Copyright (C) 1992-2011 Alek Storm
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

#ifdef __cplusplus
class UiFile : public UiForm {
  public:
	UiFile (GuiObject parent, const wchar_t *title,
		int (*okCallback) (UiForm *sendingForm, const wchar_t *sendingString, Interpreter *interpreter, const wchar_t *invokingButtonTitle, bool modified, void *closure), void *okClosure,
		const wchar_t *invokingButtonTitle, const wchar_t *helpTitle);

	MelderFile getFile ();
	void hide (void);
	/*
	Hides the visible UiFile that was opened most recently.
	Normally, file selectors stay open until their okCallback has completed.
	However, the okCallback may initiate an event loop allowing the user
	to interact with the application, for instance in Melder_pause ().
	In order that the user does not have to hide the modal file selector
	manually (by clicking the Cancel button), the application can call
	UiFile_hide () before Melder_pause ().
	*/

	structMelderFile _file;
	void *_okClosure;
	int _shiftKeyPressed;
};

class UiInfile : public UiFile {
  public:
	UiInfile (GuiObject parent, const wchar_t *title,
		int (*okCallback) (UiForm *sendingForm, const wchar_t *sendingString, Interpreter *interpreter, const wchar_t *invokingButtonTitle, bool modified, void *closure), void *okClosure,
		const wchar_t *invokingButtonTitle, const wchar_t *helpTitle, bool allowMultipleFiles);

	void do_ ();

	bool _allowMultipleFiles;
};

class UiOutfile : public UiFile {
  public:
	static UiOutfile *createE (EditorCommand cmd, const wchar_t *title, const wchar_t *invokingButtonTitle, const wchar_t *helpTitle);

	UiOutfile (GuiObject parent, const wchar_t *title,
		int (*okCallback) (UiForm *sendingForm, const wchar_t *sendingString, Interpreter *interpreter, const wchar_t *invokingButtonTitle, bool modified, void *closure), void *okClosure,
		const wchar_t *invokingButtonTitle, const wchar_t *helpTitle);

	void do_ (const wchar_t *defaultName);

	int (*_allowExecutionHook) (void *closure);
	void *_allowExecutionClosure;   /* I am owner (see destroy). */
};
#else
typedef struct UiFile UiFile;
typedef struct UiInfile UiInfile;
typedef struct UiOutfile UiOutfile;
#endif
#endif
