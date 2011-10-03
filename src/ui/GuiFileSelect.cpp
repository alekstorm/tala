/* GuiFileSelect.c
 *
 * Copyright (C) 2010 Paul Boersma
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
 * pb 2010/07/26 split off from UiFile.c
 * pb 2010/11/27 GuiFileSelect_getDirectoryName ()
 */

#include "Gui.h"
#ifdef _WIN32
	#include <Shlobj.h>
#endif

SortedSetOfString GuiFileSelect_getInfileNames (GuiObject parent, const wchar_t *title, bool allowMultipleFiles) {
	SortedSetOfString me = SortedSetOfString_create (); cherror
	{
		(void) parent;
		GuiObject dialog = gtk_file_chooser_dialog_new (Melder_peekWcsToUtf8 (title), NULL, GTK_FILE_CHOOSER_ACTION_OPEN,
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
		gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER (dialog), allowMultipleFiles);
		if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
			GSList *infileNames_list = gtk_file_chooser_get_filenames (GTK_FILE_CHOOSER (dialog));
			for (GSList *element = infileNames_list; element != NULL; element = g_slist_next (element)) {
				char *infileName_utf8 = (char*)element -> data;
				SortedSetOfString_add (me, Melder_peekUtf8ToWcs (infileName_utf8)); cherror
				g_free (infileName_utf8);
			}
			g_slist_free (infileNames_list);
		}
		gtk_widget_destroy (dialog);
	}
end:
	iferror forget (me);
	return me;
}

wchar_t * GuiFileSelect_getOutfileName (GuiObject parent, const wchar_t *title, const wchar_t *defaultName) {
	wchar_t *outfileName = NULL;
	(void) parent;
	static structMelderFile file;
	GuiObject dialog = gtk_file_chooser_dialog_new (Melder_peekWcsToUtf8 (title), NULL, GTK_FILE_CHOOSER_ACTION_SAVE,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT, NULL);
	gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog), TRUE);
	if (file. path [0] != '\0') {
		gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (dialog), Melder_peekWcsToUtf8 (file. path));
	}
	gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dialog), Melder_peekWcsToUtf8 (defaultName));
	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
		char *outfileName_utf8 = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
		outfileName = Melder_utf8ToWcs_e (outfileName_utf8);
		g_free (outfileName_utf8);
		Melder_pathToFile (outfileName, & file);
	}
	gtk_widget_destroy (dialog);
	return outfileName;
}

wchar_t * GuiFileSelect_getDirectoryName (GuiObject parent, const wchar_t *title) {
	wchar_t *directoryName = NULL;
	(void) parent;
	static structMelderFile file;
	GuiObject dialog = gtk_file_chooser_dialog_new (Melder_peekWcsToUtf8 (title), NULL, GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, "Choose", GTK_RESPONSE_ACCEPT, NULL);
	if (file. path [0] != '\0') {
		gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (dialog), Melder_peekWcsToUtf8 (file. path));
	}
	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
		char *directoryName_utf8 = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
		directoryName = Melder_utf8ToWcs_e (directoryName_utf8);
		g_free (directoryName_utf8);
		Melder_pathToFile (directoryName, & file);
	}
	gtk_widget_destroy (dialog);
	return directoryName;
}

/* End of file GuiFileSelect.c */
