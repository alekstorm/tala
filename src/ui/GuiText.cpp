/* GuiText.c
 *
 * Copyright (C) 1993-2010 Paul Boersma
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
 * pb 2003/12/30 this file separated from motifEmulator.c
 * pb 2004/01/01 Mac: MLTE
 * pb 2004/03/11 Mac: tried to make compatible with MacOS X 10.1.x
 * pb 2005/09/01 Mac: GuiText_undo and GuiText_redo
 * pb 2006/10/29 Mac: erased MacOS 9 stuff
 * pb 2006/11/10 comments
 * pb 2007/02/15 Mac: GuiText_updateChangeCountAfterSave
 * pb 2007/05/30 GuiText_getStringW
 * pb 2007/05/31 Mac: CreateEditUnicodeTextControl
 * pb 2007/06/01 Mac: erased TextEdit stuff as well as changeCount
 * pb 2007/06/11 GuiText_getSelectionW, GuiText_replaceW
 * pb 2007/06/12 let command-key combinations pass through
 * pb 2007/12/15 erased ASCII versions
 * pb 2007/12/25 Gui
 * sdk 2007/12/27 first GTK version
 * pb 2008/10/05 better implicit selection (namely, none)
 * fb 2010/02/23 GTK
 * fb 2010/02/26 GTK & GuiText_set(Undo|Redo)Item() & history for GTK
 * fb 2010/03/02 history: merge same events together
 * pb 2010/03/11 support Unicode values above 0xFFFF
 * pb 2010/05/14 GTK changedCallback
 * pb 2010/05/30 GTK selections
 * pb 2010/11/28 removed Motif
 */

#include "GuiP.h"
#undef iam
#define iam(x)  x me = (x) void_me
#if win || mac
	#define iam_text \
		Melder_assert (widget -> widgetClass == xmTextWidgetClass); \
		GuiText me = (structGuiText*)widget -> userData
#else
	#define iam_text \
		GuiText me = (structGuiText*)_GuiObject_getUserData (widget)
#endif

#if !mac
	typedef gchar * history_data;

	typedef struct _history_entry_s history_entry;
	struct _history_entry_s {
		history_entry *prev, *next;
		long first, last;
		history_data text;
		bool type_del : 1;
	};
#endif

typedef struct structGuiText {
	GuiObject widget;
	void (*changeCallback) (void *boss, GuiTextEvent event);
	void *changeBoss;
	#if mac
		TXNObject macMlteObject;
		TXNFrameID macMlteFrameId;
	#else
		history_entry *prev, *next;
		GuiObject undo_item, redo_item;
		bool history_change : 1;
	#endif
	#if win || mac
		bool editable;
	#endif
} *GuiText;

#ifndef UNIX

#if mac
	#define isTextControl(w)  ((w) -> isControl != 0)
	#define isMLTE(w)  ((w) -> macMlteObject != NULL)
#endif

/*
 * (1) KEYBOARD FOCUS
 *
 * (1.1) In Motif, the native GUI system handles all that we want:
 * every window with text widgets has one text focus widget,
 * which will receive global text focus when the window is activated.
 * The global text focus is visible to the user.
 * The focus changes whenever the user clicks in a text widget that does not have focus.
 *
 * (1.2) In Windows, the native GUI system handles almost all of the above.
 * The exception is that windows have no own text focus widgets;
 * there is only a single global text focus. This means that when the user
 * clicks on a non-active window, none of the text widgets in this window
 * will automatically receive text focus. Yet, the user expects automatic text focus behaviour
 * (click a window, then type immediately) in text edit windows (like Praat's script editor)
 * and in windows that have a single text widget (like Praat's TextGrid editor).
 * For this reason, the WM_COMMAND message handler in Gui.c intercepts the EN_SETFOCUS notification.
 * This handler calls _GuiText_handleFocusReception (), which records
 * the new text focus widget in its window. When a window is activated,
 * we set the global focus explicitly to this window's own text focus widget,
 * by calling _GuiText_setTheTextFocus ().
 *
 * (1.3) On Macintosh, we have to handle all of this explicitly.
 *
 * (1.4) On Win and Mac, we implement a feature not available in Motif:
 * the use of Command-X, Command-C, and Command-V to cut, copy, and paste in text widgets,
 * even in dialogs, where there are no menus for which these three commands could be keyboard shortcuts.
 * For this reason, _GuiText_handleFocusReception () also stores the global text focus,
 * so that the keyboard shortcut handler in Gui.c knows what widget to cut from, copy from, or paste into.
 * (It is true that Windows itself stores the global text focus, but this is not always a text widget;
 *  we want it to always be a text widget, e.g. in the TextGrid editor it is always the text widget,
 *  never the drawing area, that receives key strokes. In Motif, we will have to program this text
 *  preference explicitly; see the discussion in FunctionEditor.c.)
 */

void _GuiText_handleFocusReception (GuiObject widget) {
	/*
	 * On Windows, this is called:
	 * 1. on a user click in a text widget: WM_COMMAND -> EN_SETFOCUS;
	 * 2. on window activation: _GuiText_setTheTextFocus () -> SetFocus -> WM_COMMAND -> EN_SETFOCUS;
	 * 3. on a user click in a push button or toggle button, which would otherwise draw the
	 *    focus away from the text widgets: WM_COMMAND -> _GuiText_setTheTextFocus ().
	 *
	 * On Macintosh, this is called:
	 * 1. on a user click in a text widget: handleControlClick & handleTextEditClick -> _GuiText_setTheTextFocus ();
	 * 2. on window activation: handleActivateEvent -> _GuiText_setTheTextFocus ().
	 */
	widget -> shell -> textFocus = widget;   /* see (1.2) */
	theGui.textFocus = widget;   /* see (1.4) */
}

void _GuiText_handleFocusLoss (GuiObject widget) {
	/*
	 * me is going out of sight;
	 * it must stop having global focus.
	 */
	/*
	 * On Windows, this is called:
	 * 1. on window deactivation
	 * 2. on window closure
	 * 3. on text unmanaging
	 * 4. on window unmanaging
	 *
	 * On Macintosh, this is called:
	 * 1. on window deactivation
	 * 2. on window closure
	 * 3. on text unmanaging
	 * 4. on window unmanaging
	 */
	if (widget == theGui.textFocus)
		theGui.textFocus = NULL;
}

#if mac
void _GuiMac_clearTheTextFocus (void) {
	if (theGui.textFocus) {
		GuiText textFocus = theGui.textFocus -> userData;
		_GuiMac_clipOnParent (theGui.textFocus);
		if (isTextControl (theGui.textFocus)) {
			ClearKeyboardFocus (theGui.textFocus -> macWindow);
		} else if (isMLTE (textFocus)) {
			TXNFocus (textFocus -> macMlteObject, 0);
			TXNActivate (textFocus -> macMlteObject, textFocus -> macMlteFrameId, 0);
		}
		GuiMac_clipOff ();
		_GuiText_handleFocusLoss (theGui.textFocus);
	}
}
#endif

void _GuiText_setTheTextFocus (GuiObject widget) {
	if (widget == NULL || theGui.textFocus == widget
		|| ! widget -> managed) return;   /* Perhaps not-yet-managed. Test: open Praat's DataEditor with a Sound, then type. */
	gtk_widget_grab_focus (widget);
}

/*
 * CHANGE NOTIFICATION
 */
void _GuiText_handleValueChanged (GuiObject widget) {
	iam_text;
	if (my changeCallback) {
		struct structGuiTextEvent event = { widget };
		my changeCallback (my changeBoss, & event);
	}
}

/*
 * EVENT HANDLING
 */

#if mac
	int _GuiMacText_tryToHandleReturnKey (EventHandlerCallRef eventHandlerCallRef, EventRef eventRef, GuiObject widget, EventRecord *event) {
		if (widget && widget -> activateCallback) {
			widget -> activateCallback (widget, widget -> activateClosure, (XtPointer) event);
				return 1;
		}
		return 0;   /* Not handled. */
	}
	int _GuiMacText_tryToHandleClipboardShortcut (EventHandlerCallRef eventHandlerCallRef, EventRef eventRef, GuiObject widget, unsigned char charCode, EventRecord *event) {
		if (widget) {
			iam_text;
			if (isTextControl (widget)) {
				if (charCode == 'X' || charCode == 'C' || charCode == 'V') {
					if (! my editable && (charCode == 'X' || charCode == 'V')) return 0;
					CallNextEventHandler (eventHandlerCallRef, eventRef);
					_GuiText_handleValueChanged (widget);
					return 1;
				}
			} else if (isMLTE (me)) {
				if (charCode == 'X' && my editable) {
					if (event -> what != autoKey) GuiText_cut (widget);
					return 1;
				}
				if (charCode == 'C') {
					if (event -> what != autoKey) GuiText_copy (widget);
					return 1;
				}
				if (charCode == 'V' && my editable) {
					GuiText_paste (widget);
					return 1;
				}
			}
		}
		return 0;   /* Not handled. */
	}
	int _GuiMacText_tryToHandleKey (EventHandlerCallRef eventHandlerCallRef, EventRef eventRef, GuiObject widget, unsigned char keyCode, unsigned char charCode, EventRecord *event) {
		(void) keyCode;
		if (widget) {
			iam_text;
			if (my editable) {
				_GuiMac_clipOnParent (widget);
				//Melder_casual ("char code %d", charCode);
				if (isTextControl (widget)) {
					CallNextEventHandler (eventHandlerCallRef, eventRef);
				} else if (isMLTE (me)) {
					//static long key = 0; Melder_casual ("key %ld", ++key);
					//TXNKeyDown (my macMlteObject, event);   // Tends never to be called.
					CallNextEventHandler (eventHandlerCallRef, eventRef);
				}
				GuiMac_clipOff ();
				if (charCode > 31 || charCode < 28) {   // arrows do not change the value of the text
					_GuiText_handleValueChanged (widget);
				}
				return 1;
			}
		}
		return 0;   /* Not handled. */
	}
	void _GuiMacText_handleClick (GuiObject widget, EventRecord *event) {
		iam_text;
		_GuiText_setTheTextFocus (widget);
		_GuiMac_clipOnParent (widget);
		if (isTextControl (widget)) {
			HandleControlClick (widget -> nat.control.handle, event -> where, event -> modifiers, NULL);
		} else if (isMLTE (me)) {
			LocalToGlobal (& event -> where);
			TXNClick (my macMlteObject, event);   /* Handles text selection and scrolling. */
			GlobalToLocal (& event -> where);
		}
		GuiMac_clipOff ();
	}
#endif

/*
 * LAYOUT
 */
#if mac
	void _GuiMacText_move (GuiObject widget) {
		iam_text;
		if (isTextControl (widget)) {
			_GuiMac_clipOnParent (widget);
			MoveControl (widget -> nat.control.handle, widget -> rect.left + 3, widget -> rect.top + 3);
			_GuiMac_clipOffValid (widget);
		} else if (isMLTE (me)) {
			TXNSetFrameBounds (my macMlteObject, widget -> rect. top, widget -> rect. left,
				widget -> rect. bottom, widget -> rect. right, my macMlteFrameId);
		}
	}
	void _GuiMacText_shellResize (GuiObject widget) {
		iam_text;
		/*
		 * Shell erasure, and therefore text erasure, has been handled by caller.
		 * Reshowing will be handled by caller.
		 */
		if (isTextControl (widget)) {
			MoveControl (widget -> nat.control.handle, widget -> rect.left + 3, widget -> rect.top + 3);
			SizeControl (widget -> nat.control.handle, widget -> width - 6, widget -> height - 6);
			/*
			 * Control reshowing will explicitly be handled by caller.
			 */
		} else if (isMLTE (me)) {
			TXNSetFrameBounds (my macMlteObject, widget -> rect. top, widget -> rect. left,
				widget -> rect. bottom, widget -> rect. right, my macMlteFrameId);
		}
	}
	void _GuiMacText_resize (GuiObject widget) {
		iam_text;
		if (isTextControl (widget)) {
			SizeControl (widget -> nat.control.handle, widget -> width - 6, widget -> height - 6);
			/*
			 * Container widgets will have been invalidated.
			 * So in order not to make the control flash, we validate it.
			 */
			_Gui_validateWidget (widget);
		} else if (isMLTE (me)) {
			TXNSetFrameBounds (my macMlteObject, widget -> rect. top, widget -> rect. left,
				widget -> rect. bottom, widget -> rect. right, my macMlteFrameId);
		}
	}
#endif

void _GuiText_unmanage (GuiObject widget) {
	#if win
		_GuiText_handleFocusLoss (widget);
		_GuiNativeControl_hide (widget);
	#elif mac
		iam_text;
		/*
		 * Just _GuiText_handleFocusLoss () is not enough,
		 * because that can leave a visible blinking cursor.
		 */
		if (isTextControl (widget)) {
			if (widget == theGui.textFocus) _GuiMac_clearTheTextFocus ();   /* Remove visible blinking cursor. */
			_GuiNativeControl_hide (widget);
		} else if (isMLTE (me)) {
		}
	#endif
	/*
	 * The caller will set the unmanage flag to zero, and remanage the parent.
	 */
}

/*
 * VISIBILITY
 */

#if mac
	void _GuiMacText_update (GuiObject widget) {
		iam_text;
		_GuiMac_clipOnParent (widget);
		if (isTextControl (widget)) {
			Draw1Control (widget -> nat.control.handle);
		} else if (isMLTE (me)) {
			TXNDraw (my macMlteObject, NULL);
		}
		GuiMac_clipOff ();
	}
#endif

void _GuiWinMacText_destroy (GuiObject widget) {
	if (widget == theGui.textFocus)
		theGui.textFocus = NULL;   // remove dangling reference
	if (widget == widget -> shell -> textFocus)
		widget -> shell -> textFocus = NULL;   // remove dangling reference
	iam_text;
	#if win
		DestroyWindow (widget -> window);
	#elif mac
		if (isTextControl (widget)) {
			_GuiMac_clipOnParent (widget);
			DisposeControl (widget -> nat.control.handle);
			GuiMac_clipOff ();
		} else if (isMLTE (me)) {
			TXNDeleteObject (my macMlteObject);
		}
	#endif
	Melder_free (me);   // NOTE: my widget is not destroyed here
}

void _GuiWinMacText_map (GuiObject widget) {
	iam_text;
	#if win
		ShowWindow (widget -> window, SW_SHOW);
	#elif mac
		if (isTextControl (widget)) {
			_GuiNativeControl_show (widget);
		} else if (isMLTE (me)) {
		}
	#endif
}
#endif


#if mac || win

static long NativeText_getLength (GuiObject widget) {
	#if win
		return Edit_GetTextLength (widget -> window);
	#elif mac
		iam_text;
		if (isTextControl (widget)) {
			Size size;
			CFStringRef cfString;
			GetControlData (widget -> nat.control.handle, kControlEntireControl, kControlEditTextCFStringTag, sizeof (CFStringRef), & cfString, NULL);
			size = CFStringGetLength (cfString);
			CFRelease (cfString);
			return size;
		} else if (isMLTE (me)) {
			#if 1
				/*
				 * From the reference page of TXNDataSize:
				 * "If you are using Unicode and you want to know the number of characters,
				 * divide the returned ByteCount value by sizeof(UniChar) or 2,
				 * since MLTE uses the 16-bit Unicode Transformation Format (UTF-16)."
				 */
				return TXNDataSize (my macMlteObject) / sizeof (UniChar);
			#else
				long length = 0, dataSize = TXNDataSize (my macMlteObject);
				ItemCount numberOfRuns;
				TXNCountRunsInRange (my macMlteObject, 0, dataSize, & numberOfRuns);
				for (long irun = 0; irun < numberOfRuns; irun ++) {
					unsigned long left, right;
					TXNDataType dataType;
					TXNGetIndexedRunInfoFromRange (my macMlteObject, irun, 0, dataSize,
						& left, & right, & dataType, 0, NULL);
					if (dataType == kTXNTextData || dataType == kTXNUnicodeTextData) {
						Handle han;
						TXNGetDataEncoded (my macMlteObject, left, right, & han, kTXNUnicodeTextData);
						if (han) {
							long size = GetHandleSize (han) / 2;
							length += size;
							DisposeHandle (han);
						}
					}
				}
				return length;
			#endif
		}
		return 0;   // Should not occur.
	#endif
}

static void NativeText_getText (GuiObject widget, wchar_t *buffer, long length) {
	#if win
		GetWindowText (widget -> window, buffer, length + 1);
	#elif mac
		iam_text;
		if (isTextControl (widget)) {
			CFStringRef cfString;
			GetControlData (widget -> nat.control.handle, kControlEntireControl, kControlEditTextCFStringTag, sizeof (CFStringRef), & cfString, NULL);
			UniChar *macText = Melder_malloc_f (UniChar, length + 1);
			CFRange range = { 0, length };
			CFStringGetCharacters (cfString, range, macText);
			CFRelease (cfString);
			long j = 0;
			for (long i = 0; i < length; i ++) {
				unsigned long kar = macText [i];
				if (kar < 0xD800 || kar > 0xDFFF) {
					buffer [j ++] = kar;
				} else {
					Melder_assert (kar >= 0xD800 && kar <= 0xDBFF);
					unsigned long kar1 = macText [++ i];
					Melder_assert (kar1 >= 0xDC00 && kar1 <= 0xDFFF);
					buffer [j ++] = 0x10000 + ((kar & 0x3FF) << 10) + (kar1 & 0x3FF);
				}
			}
			buffer [j] = '\0';
			Melder_free (macText);
		} else if (isMLTE (me)) {
			#if 1
				Handle han;
				TXNGetDataEncoded (my macMlteObject, 0, length, & han, kTXNUnicodeTextData);
				long j = 0;
				for (long i = 0; i < length; i ++) {
					unsigned long kar = ((UniChar *) *han) [i];
					if (kar < 0xD800 || kar > 0xDFFF) {
						buffer [j ++] = kar;
					} else {
						Melder_assert (kar >= 0xD800 && kar <= 0xDBFF);
						unsigned long kar1 = ((UniChar *) *han) [++ i];
						Melder_assert (kar1 >= 0xDC00 && kar1 <= 0xDFFF);
						buffer [j ++] = 0x10000 + ((kar & 0x3FF) << 10) + (kar1 & 0x3FF);
					}
				}
				buffer [j] = '\0';
				DisposeHandle (han);
			#else
				long dataSize = TXNDataSize (my macMlteObject);
				ItemCount numberOfRuns;
				TXNCountRunsInRange (my macMlteObject, 0, dataSize, & numberOfRuns);
				for (long irun = 0; irun < numberOfRuns; irun ++) {
					unsigned long left, right;
					TXNDataType dataType;
					TXNGetIndexedRunInfoFromRange (my macMlteObject, irun, 0, dataSize,
						& left, & right, & dataType, 0, NULL);
					if (dataType == kTXNTextData || dataType == kTXNUnicodeTextData) {
						Handle han;
						TXNGetDataEncoded (my macMlteObject, left, right, & han, kTXNUnicodeTextData);
						if (han) {
							long size = GetHandleSize (han) / 2;
							wcsncpy (buffer, (wchar_t *) *han, size);
							buffer += size;
							DisposeHandle (han);
						}
					}
				}
				buffer [0] = '\0';
				return;
			#endif
		}
	#endif
	buffer [length] = '\0';   // superfluous?
}

/*
 * SELECTION
 */

static int NativeText_getSelectionRange (GuiObject widget, long *out_left, long *out_right) {
	unsigned long left, right;
	#ifndef unix
	Melder_assert (MEMBER (widget, Text));
	#endif
	#if win
		SendMessage (widget -> window, EM_GETSEL, (WPARAM) & left, (LPARAM) & right);   // 32-bit (R&N: 579)
	#elif mac
		iam_text;
		if (isTextControl (widget)) {
			ControlEditTextSelectionRec rec;
			GetControlData (widget -> nat.control.handle, kControlEntireControl, kControlEditTextSelectionTag, sizeof (rec), & rec, NULL);
			left = rec.selStart;
			right = rec. selEnd;
		} else if (isMLTE (me)) {
			TXNGetSelection (my macMlteObject, & left, & right);
		}
	#endif
	if (out_left) *out_left = left;
	if (out_right) *out_right = right;
	return right > left;
}

/*
 * PACKAGE
 */

void _GuiText_init (void) {
	#if mac
		//short font;
		TXNMacOSPreferredFontDescription defaults = { 0 };
		//GetFNum ("\006Monaco", & font);
		//defaults. fontID = font;
		defaults. pointSize = 0x000B0000;
		defaults. fontStyle = kTXNDefaultFontStyle;
		defaults. encoding  = /*kTXNMacOSEncoding*/ kTXNSystemDefaultEncoding;
		TXNInitTextension (& defaults, 1, 0);
	#endif
}

void _GuiText_exit (void) {
	#if mac
		TXNTerminateTextension (); 
	#endif
}

#endif

#if !mac
	/*
	 * Undo/Redo history functions
	 */

	static void _GuiText_delete(GuiObject widget, int from_pos, int to_pos) {
		if (G_OBJECT_TYPE (G_OBJECT (widget)) == GTK_TYPE_ENTRY) {
			gtk_editable_delete_text (GTK_EDITABLE (widget), from_pos, to_pos);
		} else {
			GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget));
			GtkTextIter from_it, to_it;
			gtk_text_buffer_get_iter_at_offset(buffer, &from_it, from_pos);
			gtk_text_buffer_get_iter_at_offset(buffer, &to_it, to_pos);
			gtk_text_buffer_delete_interactive(buffer, &from_it, &to_it,
				gtk_text_view_get_editable(GTK_TEXT_VIEW(widget)));
			gtk_text_buffer_place_cursor(buffer, &to_it);
		}
	}

	static void _GuiText_insert(GuiObject widget, int from_pos, int to_pos, const history_data text) {
		if (G_OBJECT_TYPE (G_OBJECT (widget)) == GTK_TYPE_ENTRY) {
			gint from_pos_gint = from_pos;
			gtk_editable_insert_text (GTK_EDITABLE (widget), text, to_pos - from_pos, &from_pos_gint);
		} else {
			GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget));
			GtkTextIter it;
			gtk_text_buffer_get_iter_at_offset(buffer, &it, from_pos);
			gtk_text_buffer_insert_interactive(buffer, &it, text, to_pos - from_pos,
				gtk_text_view_get_editable(GTK_TEXT_VIEW(widget)));
			gtk_text_buffer_get_iter_at_offset(buffer, &it, to_pos);
			gtk_text_buffer_place_cursor(buffer, &it);
		}
	}

	/* Tests the previous elements of the history for mergability with the one to insert given via parameters.
	 * If successful, it returns a pointer to the last valid entry in the merged history list, otherwise
	 * returns NULL.
	 * Specifically the function expands the previous insert/delete event(s)
	 *  - with the current, if current also is an insert/delete event and the ranges of previous and current event match
	 *  - with the previous delete and current insert event, in case the ranges of both event-pairs respectively match
	 */
	static history_entry * history_addAndMerge(void *void_me, history_data text_new, long first, long last, bool deleted) {
		iam(GuiText);
		history_entry *he = NULL;
		
		if (!(my prev))
			return NULL;
		
		if (my prev->type_del == deleted) {
			// extend the last history event by this one
			if (my prev->first == last) {
				// most common for backspace key presses
				he = my prev;
				text_new = (gchar*)realloc(text_new, sizeof(*text_new) * (he->last - first + 1));
				memcpy(text_new + last - first, he->text, sizeof(*text_new) * (he->last - he->first + 1));
				free(he->text);
				he->text = text_new;
				he->first = first;
				
			} else if (my prev->last == first) {
				// most common for ordinary text insertion
				he = my prev;
				he->text = (gchar*)realloc(he->text, sizeof(*he->text) * (last - he->first + 1));
				memcpy(he->text + he->last - he->first, text_new, sizeof(*he->text) * (last - first + 1));
				free(text_new);
				he->last = last;
				
			} else if (deleted && my prev->first == first) {
				// most common for delete key presses
				he = my prev;
				he->text = (gchar*)realloc(he->text, sizeof(*he->text) * (last - first + he->last - he->first + 1));
				memcpy(he->text + he->last - he->first, text_new, sizeof(*he->text) * (last - first + 1));
				free(text_new);
				he->last = last + he->last - he->first;
			}
		} else {
			// prev->type_del != deleted, no simple expansion possible, check for double expansion
			if (!deleted && my prev->prev && my prev->prev->prev) {
				history_entry *del_one = my prev;
				history_entry *ins_mult = del_one->prev;
				history_entry *del_mult = ins_mult->prev;
				long from1 = del_mult->first, to1 = del_mult->last;
				long from2 = ins_mult->first, to2 = ins_mult->last;
				long from3 = del_one->first, to3 = del_one->last;
				if (from3 == first && to3 == last && from2 == from1 && to2 == to1 && to1 == first &&
						!ins_mult->type_del && del_mult->type_del) {
					// most common for overwriting text
					/* So the layout is as follows:
					 *
					 *        del_mult                  ins_mult               del_one        current (parameters)
					 * [del, from1, to1, "uvw"] [ins, from1, to1, "abc"] [del, to1, to3, "x"] [ins, to1, to3, "d"]
					 *     n >= 1 characters          n characters           1 character          1 character
					 *
					 * So merge those four events into two events by expanding del_mult by del_one and ins_mult by current */
					del_mult->text = (gchar*)realloc(del_mult->text, sizeof(*del_mult->text) * (to3 - from1 + 1));
					ins_mult->text = (gchar*)realloc(ins_mult->text, sizeof(*ins_mult->text) * (to3 - from1 + 1));
					memcpy(del_mult->text + to1 - from1, del_one->text, sizeof(*del_mult->text) * (to3 - to1 + 1));
					memcpy(ins_mult->text + to1 - from1, text_new     , sizeof(*del_mult->text) * (to3 - to1 + 1));
					del_mult->last = to3;
					ins_mult->last = to3;
					free(del_one->text);
					free(del_one);
					free(text_new);
					my prev = he = ins_mult;
				}
			}
		}
		
		return he;
	}

	/* Inserts a new history action, thereby removing any remaining 'redo' steps;
	 *   text_new  a newly allocated string that will be freed by a history function
	 *             (history_add or history_clear)
	 */
	static void history_add(void *void_me, history_data text_new, long first, long last, bool deleted) {
		iam(GuiText);
		
		// delete all newer entries; from here on there is no 'Redo' until the next 'Undo' is performed
		history_entry *old_hnext = my next, *hnext;
		while (old_hnext) {
			hnext = old_hnext->next;
			free(old_hnext->text);
			free(old_hnext);
			old_hnext = hnext;
		}
		my next = NULL;
		
		history_entry *he = history_addAndMerge(void_me, text_new, first, last, deleted);
		if (he == NULL) {
			he = (history_entry*)malloc(sizeof(history_entry));
			he->first = first;
			he->last = last;
			he->type_del = deleted;
			he->text = text_new;
			
			he->prev = my prev;
			he->next = NULL;
			if (my prev)
				my prev->next = he;
		}
		my prev = he;
		he->next = NULL;
		
		if (my undo_item) GuiObject_setSensitive(my undo_item, TRUE);
		if (my redo_item) GuiObject_setSensitive(my redo_item, FALSE);
	}

	static bool history_has_undo(void *void_me) {
		iam(GuiText);
		return my prev != NULL;
	}

	static bool history_has_redo(void *void_me) {
		iam(GuiText);
		return my next != NULL;
	}

	static void history_do(void *void_me, bool undo) {
		iam(GuiText);
		history_entry *he = undo ? my prev : my next;
		if (he == NULL) // TODO: this function should not be called in that case
			return;
		
		my history_change = 1;
		if (undo ^ he->type_del) {
			_GuiText_delete(my widget, he->first, he->last);
		} else {
			_GuiText_insert(my widget, he->first, he->last, he->text);
		}
		my history_change = 0;
		
		if (undo) {
			my next = my prev;
			my prev = my prev->prev;
		} else {
			my prev = my next;
			my next = my next->next;
		}
		
		if (my undo_item) GuiObject_setSensitive(my undo_item, history_has_undo(me));
		if (my redo_item) GuiObject_setSensitive(my redo_item, history_has_redo(me));
	}

	static void history_clear(void *void_me) {
		iam(GuiText);
		history_entry *h1, *h2;
		
		h1 = my prev;
		while (h1) {
			h2 = h1->prev;
			free(h1->text);
			free(h1);
			h1 = h2;
		}
		my prev = NULL;
		
		h1 = my next;
		while (h1) {
			h2 = h1->next;
			free(h1->text);
			free(h1);
			h1 = h2;
		}
		my next = NULL;
		
		if (my undo_item) GuiObject_setSensitive(my undo_item, FALSE);
		if (my redo_item) GuiObject_setSensitive(my redo_item, FALSE);
	}
#endif

/*
 * CALLBACKS
 */

static void _GuiGtkEntry_history_delete_cb (GtkEditable *ed, gint from, gint to, gpointer void_me) {
	iam (GuiText);
	if (my history_change) return;
	history_add (me, gtk_editable_get_chars (GTK_EDITABLE (ed), from, to), from, to, 1);
}

static void _GuiGtkEntry_history_insert_cb (GtkEditable *ed, gchar *utf8_text, gint len, gint *from, gpointer void_me) {
	(void) ed;
	iam (GuiText);
	if (my history_change) return;
	gchar *text = (gchar*)malloc (sizeof (gchar) * (len + 1));
	strcpy (text, utf8_text);
	history_add (me, text, *from, *from + len, 0);
}
	
static void _GuiGtkTextBuf_history_delete_cb (GtkTextBuffer *buffer, GtkTextIter *from, GtkTextIter *to, gpointer void_me) {
	iam (GuiText);
	if (my history_change) return;
	int from_pos = gtk_text_iter_get_offset (from);
	int to_pos = gtk_text_iter_get_offset (to);
	history_add (me, gtk_text_buffer_get_text (buffer, from, to, FALSE), from_pos, to_pos, 1);
}
	
static void _GuiGtkTextBuf_history_insert_cb (GtkTextBuffer *buffer, GtkTextIter *from, gchar *utf8_text, gint len, gpointer void_me) {
	(void) buffer;
	iam (GuiText);
	if (my history_change) return;
	int from_pos = gtk_text_iter_get_offset (from);
	gchar *text = (gchar*)malloc (sizeof (gchar) * (len + 1));
	strcpy (text, utf8_text);
	history_add (me, text, from_pos, from_pos + len, 0);
}
	
static void _GuiGtkText_valueChangedCallback (GuiObject widget, gpointer void_me) {
	iam (GuiText);
	Melder_assert (me != NULL);
	if (my changeCallback != NULL) {
		struct structGuiTextEvent event = { widget };
		my changeCallback (my changeBoss, & event);
	}
}
	
static void _GuiGtkText_destroyCallback (GuiObject widget, gpointer void_me) {
	(void) widget;
	iam (GuiText);
	if (my undo_item) g_object_unref (my undo_item);
	if (my redo_item) g_object_unref (my redo_item);
	my undo_item = NULL;
	my redo_item = NULL;
	history_clear (me);
	Melder_free (me);
}

GuiObject GuiText_create (GuiObject parent, int left, int right, int top, int bottom, unsigned long flags) {
	GuiText me = Melder_calloc_f (struct structGuiText, 1);
	if (flags & GuiText_SCROLLED) {
		GtkWrapMode ww;
		GuiObject scrolled = gtk_scrolled_window_new (NULL, NULL);
		gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
		my widget = gtk_text_view_new ();
		gtk_container_add (GTK_CONTAINER (scrolled), my widget);
		gtk_widget_show (scrolled);
		gtk_text_view_set_editable (GTK_TEXT_VIEW (my widget), (flags & GuiText_NONEDITABLE) == 0);
		if ((flags & GuiText_WORDWRAP) != 0) 
			ww = GTK_WRAP_WORD_CHAR;
		else
			ww = GTK_WRAP_NONE;
		gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (my widget), ww);
		GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (my widget));
		g_signal_connect (G_OBJECT (buffer), "delete-range", G_CALLBACK (_GuiGtkTextBuf_history_delete_cb), me);
		g_signal_connect (G_OBJECT (buffer), "insert-text", G_CALLBACK (_GuiGtkTextBuf_history_insert_cb), me);
		g_signal_connect (G_OBJECT (buffer), "changed", G_CALLBACK (_GuiGtkText_valueChangedCallback), me);
		gtk_container_add (GTK_CONTAINER (parent), scrolled);
	} else {
		my widget = gtk_entry_new ();
		gtk_editable_set_editable (GTK_EDITABLE (my widget), (flags & GuiText_NONEDITABLE) == 0);
		g_signal_connect (G_OBJECT (my widget), "delete-text", G_CALLBACK (_GuiGtkEntry_history_delete_cb), me);
		g_signal_connect (G_OBJECT (my widget), "insert-text", G_CALLBACK (_GuiGtkEntry_history_insert_cb), me);
		g_signal_connect (GTK_EDITABLE (my widget), "changed", G_CALLBACK (_GuiGtkText_valueChangedCallback), me);
		//GTK_WIDGET_UNSET_FLAGS (my widget, GTK_CAN_DEFAULT);
		if (GTK_IS_BOX (parent)) {
			gtk_container_add (GTK_CONTAINER (parent), my widget);
		}
		gtk_entry_set_activates_default (GTK_ENTRY (my widget), true);
	}
	_GuiObject_setUserData (my widget, me);
	_GuiObject_position (my widget, left, right, top, bottom);
	my prev = NULL;
	my next = NULL;
	my history_change = 0;
	my undo_item = NULL;
	my redo_item = NULL;
	g_signal_connect (G_OBJECT (my widget), "destroy", G_CALLBACK (_GuiGtkText_destroyCallback), me);

	return my widget;
}

GuiObject GuiText_createShown (GuiObject parent, int left, int right, int top, int bottom, unsigned long flags) {
	GuiObject me = GuiText_create (parent, left, right, top, bottom, flags);
	GuiObject_show (me);
	return me;
}

void GuiText_copy (GuiObject widget) {
	if (G_OBJECT_TYPE (G_OBJECT (widget)) == GTK_TYPE_ENTRY) {
		gtk_editable_copy_clipboard (GTK_EDITABLE (widget));
	} else if (G_OBJECT_TYPE (G_OBJECT (widget)) == GTK_TYPE_TEXT_VIEW) {
		GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget));
		GtkClipboard *cb = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
		gtk_text_buffer_copy_clipboard(buffer, cb);
	}
}

void GuiText_cut (GuiObject widget) {
	if (G_OBJECT_TYPE (G_OBJECT (widget)) == GTK_TYPE_ENTRY) {
		gtk_editable_cut_clipboard (GTK_EDITABLE (widget));
	} else if (G_OBJECT_TYPE (G_OBJECT (widget)) == GTK_TYPE_TEXT_VIEW) {
		GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget));
		GtkClipboard *cb = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
		gtk_text_buffer_cut_clipboard(buffer, cb, gtk_text_view_get_editable(GTK_TEXT_VIEW(widget)));
	}
}

wchar_t * GuiText_getSelection (GuiObject widget) {
	// first = gtk_text_iter_get_offset (& start);
	// last = gtk_text_iter_get_offset (& end);
	if (G_OBJECT_TYPE (G_OBJECT (widget)) == GTK_TYPE_ENTRY) {
		gint start, end;
		gtk_editable_get_selection_bounds (GTK_EDITABLE (widget), & start, & end); 
		if (end > start) {   // at least one character selected?
			gchar *text = gtk_editable_get_chars (GTK_EDITABLE (widget), start, end);
			wchar_t *result = Melder_utf8ToWcs_e (text);
			g_free (text);
			return result;
		}
	} else if (G_OBJECT_TYPE (G_OBJECT (widget)) == GTK_TYPE_TEXT_VIEW) {
		GtkTextBuffer *textBuffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (widget));
		if (gtk_text_buffer_get_has_selection (textBuffer)) {   // at least one character selected?
			GtkTextIter start, end;
			gtk_text_buffer_get_selection_bounds (textBuffer, & start, & end);
			gchar *text = gtk_text_buffer_get_text (textBuffer, & start, & end, TRUE);
			wchar_t *result = Melder_utf8ToWcs_e (text);
			g_free (text);
			return result;
		}
	}
	return NULL;   // zero characters selected
}

wchar_t * GuiText_getString (GuiObject widget) {
	long first, last;
	return GuiText_getStringAndSelectionPosition (widget, & first, & last);
}

wchar_t * GuiText_getStringAndSelectionPosition (GuiObject widget, long *first, long *last) {
	if (G_OBJECT_TYPE (G_OBJECT (widget)) == GTK_TYPE_ENTRY) {
		gint first_gint, last_gint;
		gtk_editable_get_selection_bounds (GTK_EDITABLE (widget), & first_gint, & last_gint);
		*first = first_gint;
		*last = last_gint;
		return Melder_utf8ToWcs_e (gtk_entry_get_text (GTK_ENTRY (widget)));
	} else if (G_OBJECT_TYPE (G_OBJECT (widget)) == GTK_TYPE_TEXT_VIEW) {
		GtkTextBuffer *textBuffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (widget));
		GtkTextIter start, end;
		gtk_text_buffer_get_start_iter (textBuffer, & start);
		gtk_text_buffer_get_end_iter (textBuffer, & end);
		gchar *text = gtk_text_buffer_get_text (textBuffer, & start, & end, TRUE); // TODO: Hidden chars ook maar doen he?
		wchar_t *result = Melder_utf8ToWcs_e (text);
		g_free (text);
		gtk_text_buffer_get_selection_bounds (textBuffer, & start, & end);
		*first = gtk_text_iter_get_offset (& start);
		*last = gtk_text_iter_get_offset (& end);
		return result;
	}
	return NULL;
}

void GuiText_paste (GuiObject widget) {
	if (G_OBJECT_TYPE (G_OBJECT (widget)) == GTK_TYPE_ENTRY) {
		gtk_editable_paste_clipboard (GTK_EDITABLE (widget));
	} else if (G_OBJECT_TYPE (G_OBJECT (widget)) == GTK_TYPE_TEXT_VIEW) {
		GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget));
		GtkClipboard *cb = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
		gtk_text_buffer_paste_clipboard(buffer, cb, NULL, gtk_text_view_get_editable(GTK_TEXT_VIEW(widget)));
	}
}

void GuiText_redo (GuiObject widget) {
	iam_text;
	history_do(me, 0);
}

void GuiText_remove (GuiObject widget) {
	if (G_OBJECT_TYPE (G_OBJECT (widget)) == GTK_TYPE_ENTRY) {
		gtk_editable_delete_selection (GTK_EDITABLE (widget));
	} else if (G_OBJECT_TYPE (G_OBJECT (widget)) == GTK_TYPE_TEXT_VIEW) {
		GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget));
		gtk_text_buffer_delete_selection(buffer, TRUE, gtk_text_view_get_editable(GTK_TEXT_VIEW(widget)));
	}
}

void GuiText_replace (GuiObject widget, long from_pos, long to_pos, const wchar_t *text) {
	gchar *new_ = Melder_peekWcsToUtf8 (text);
	if (G_OBJECT_TYPE (G_OBJECT (widget)) == GTK_TYPE_ENTRY) {
		gtk_editable_delete_text (GTK_EDITABLE (widget), from_pos, to_pos);
		gint from_pos_gint = from_pos;
		gtk_editable_insert_text (GTK_EDITABLE (widget), new_, g_utf8_strlen (new_, -1), & from_pos_gint);
	} else if (G_OBJECT_TYPE (G_OBJECT (widget)) == GTK_TYPE_TEXT_VIEW) {
		GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW(widget));
		GtkTextIter from_it, to_it;
		gtk_text_buffer_get_iter_at_offset (buffer, & from_it, from_pos);
		gtk_text_buffer_get_iter_at_offset (buffer, & to_it, to_pos);
		gtk_text_buffer_delete_interactive (buffer, & from_it, & to_it,
			gtk_text_view_get_editable (GTK_TEXT_VIEW (widget)));
		gtk_text_buffer_insert_interactive (buffer, & from_it, new_, g_utf8_strlen (new_, -1),
			gtk_text_view_get_editable (GTK_TEXT_VIEW (widget)));
	}
}

void GuiText_scrollToSelection (GuiObject widget) {
	GtkTextBuffer *textBuffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (widget));
	GtkTextIter start, end;
	gtk_text_buffer_get_selection_bounds (textBuffer, & start, & end);
	//GtkTextMark *mark = gtk_text_buffer_create_mark (textBuffer, NULL, & start, true);
	gtk_text_view_scroll_to_iter (GTK_TEXT_VIEW (widget), & start, 0.1, false, 0.0, 0.0); 
	//gtk_text_view_scroll_to_mark (GTK_TEXT_VIEW (widget), mark, 0.1, false, 0.0, 0.0); 
}

void GuiText_setChangeCallback (GuiObject widget, void (*changeCallback) (void *boss, GuiTextEvent event), void *changeBoss) {
	iam_text;
	my changeCallback = changeCallback;
	my changeBoss = changeBoss;
}

void GuiText_setFontSize (GuiObject widget, int size) {
	GtkRcStyle *modStyle = gtk_widget_get_modifier_style (widget);
	PangoFontDescription *fontDesc = modStyle -> font_desc != NULL ? modStyle->font_desc : pango_font_description_copy (widget -> style -> font_desc);
	pango_font_description_set_absolute_size (fontDesc, size * PANGO_SCALE);
	modStyle -> font_desc = fontDesc;
	gtk_widget_modify_style (widget, modStyle);
}

void GuiText_setRedoItem (GuiObject widget, GuiObject item) {
	iam_text;
	if (my redo_item)
		g_object_unref (my redo_item);
	my redo_item = item;
	if (my redo_item) {
		g_object_ref (my redo_item);
		GuiObject_setSensitive (my redo_item, history_has_redo (me));
	}
}

void GuiText_setSelection (GuiObject widget, long first, long last) {
	if (widget != NULL) {
		if (G_OBJECT_TYPE (G_OBJECT (widget)) == GTK_TYPE_ENTRY) {
			gtk_editable_select_region (GTK_EDITABLE (widget), first, last);
		} else if (G_OBJECT_TYPE (G_OBJECT (widget)) == GTK_TYPE_TEXT_VIEW) {
			GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (widget));
			GtkTextIter from_it, to_it;
			gtk_text_buffer_get_iter_at_offset (buffer, & from_it, first);
			gtk_text_buffer_get_iter_at_offset (buffer, & to_it, last);
			gtk_text_buffer_select_range (buffer, & from_it, & to_it);
		}
	}
}

void GuiText_setString (GuiObject widget, const wchar_t *text) {
	if (G_OBJECT_TYPE (widget) == GTK_TYPE_ENTRY) {
		gtk_entry_set_text (GTK_ENTRY (widget), Melder_peekWcsToUtf8 (text));
	} else if (G_OBJECT_TYPE (widget) == GTK_TYPE_TEXT_VIEW) {
		GtkTextBuffer *textBuffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (widget));
		gchar *textUtf8 = Melder_peekWcsToUtf8 (text);
		//gtk_text_buffer_set_text (textBuffer, textUtf8, strlen (textUtf8));   // length in bytes!
		GtkTextIter start, end;
		gtk_text_buffer_get_start_iter (textBuffer, & start);
		gtk_text_buffer_get_end_iter (textBuffer, & end);
		gtk_text_buffer_delete_interactive (textBuffer, & start, & end, gtk_text_view_get_editable (GTK_TEXT_VIEW (widget)));
		gtk_text_buffer_insert_interactive (textBuffer, & start, textUtf8, strlen (textUtf8), gtk_text_view_get_editable (GTK_TEXT_VIEW (widget)));
	}
}

void GuiText_setUndoItem (GuiObject widget, GuiObject item) {
	iam_text;
	if (my undo_item)
		g_object_unref (my undo_item);
	my undo_item = item;
	if (my undo_item) {
		g_object_ref (my undo_item);
		GuiObject_setSensitive(my undo_item, history_has_undo(me));
	}
}

void GuiText_undo (GuiObject widget) {
	iam_text;
	history_do (me, 1);
}

/* End of file GuiText.c */
