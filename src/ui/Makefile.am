AM_CFLAGS = $(COMMON) -I @abs_top_srcdir@/src -std=gnu99
AM_CPPFLAGS = $(COMMON) -I @abs_top_srcdir@/src

SUBDIRS = init editors

noinst_LIBRARIES = libui.a
libui_a_SOURCES = \
	Formula.cpp \
	Graphics_altitude.cpp \
	Graphics_colour.cpp \
	Graphics.cpp \
	Graphics_grey.cpp \
	Graphics_image.cpp \
	Graphics_linesAndAreas.cpp \
	Graphics_mouse.cpp \
	GraphicsPostscript.cpp \
	Graphics_record.cpp \
	GraphicsScreen.cpp \
	Graphics_surface.cpp \
	Graphics_text.cpp \
	Graphics_utils.cpp \
	GuiButton.cpp \
	GuiCheckButton.cpp \
	Gui.cpp \
	GuiDialog.cpp \
	GuiDrawingArea.cpp \
	GuiFileSelect.cpp \
	GuiLabel.cpp \
	GuiList.cpp \
	GuiMenu.cpp \
	GuiObject.cpp \
	GuiRadioButton.cpp \
	GuiRowColumn.cpp \
	GuiText.cpp \
	GuiWindow.cpp \
	Interpreter.cpp \
	machine.cpp \
	Picture.cpp \
	praat_actions.cpp \
	praat.cpp \
	praat_logo.cpp \
	praat_menuCommands.cpp \
	praat_objectMenus.cpp \
	praat_picture.cpp \
	praat_script.cpp \
	praat_statistics.cpp \
	Preferences.cpp \
	Printer.cpp \
	sendpraat.cpp \
	sendsocket.cpp \
	site.cpp \
	UiField.cpp \
	UiFile.cpp \
	UiForm.cpp \
	UiPause.cpp
#libui_la_LIBADD = init/libinit.la editors/libeditors.la
