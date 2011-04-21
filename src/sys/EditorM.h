/* EditorM.h
 *
 * Copyright (C) 1992-2010 Paul Boersma
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
 * pb 2010/12/07
 */

#include "UiFile.h"

#undef FORM
#undef REAL
#undef REAL_OR_UNDEFINED
#undef POSITIVE
#undef INTEGER
#undef NATURAL
#undef WORD
#undef SENTENCE
#undef COLOUR
#undef CHANNEL
#undef BOOLEAN
#undef LABEL
#undef TEXTFIELD
#undef RADIO
#undef RADIOBUTTON
#undef OPTIONMENU
#undef OPTION
#undef RADIOBUTTONS_ENUM
#undef OPTIONS_ENUM
#undef RADIO_ENUM
#undef OPTIONMENU_ENUM
#undef LIST
#undef OK
#undef SET_REAL
#undef SET_INTEGER
#undef SET_STRING
#undef DO
#undef END
#undef DIRECT
#undef FORM_WRITE
#undef DO_WRITE
#undef GET_REAL
#undef GET_INTEGER
#undef GET_STRING
#undef GET_FILE

#define REAL(label,def)		cmd->_dialog->addReal (label, def);
#define REAL_OR_UNDEFINED(label,def)  cmd->_dialog->addRealOrUndefined (label, def);
#define POSITIVE(label,def)	cmd->_dialog->addPositive (label, def);
#define INTEGER(label,def)	cmd->_dialog->addInteger (label, def);
#define NATURAL(label,def)	cmd->_dialog->addNatural (label, def);
#define WORD(label,def)		cmd->_dialog->addWord (label, def);
#define SENTENCE(label,def)	cmd->_dialog->addSentence (label, def);
#define COLOUR(label,def)	cmd->_dialog->addColour (label, def);
#define CHANNEL(label,def)	cmd->_dialog->addChannel (label, def);
#define BOOLEAN(label,def)	cmd->_dialog->addBoolean (label, def);
#define LABEL(name,label)	cmd->_dialog->addLabel (name, label);
#define TEXTFIELD(name,def)	cmd->_dialog->addText (name, def);
#define RADIO(label,def)	radio = cmd->_dialog->addRadio (label, def);
#define RADIOBUTTON(label)	radio->addRadio (label);
#define OPTIONMENU(label,def)	radio = cmd->_dialog->addOptionMenu (label, def);
#define OPTION(label)		radio->addOptionMenu (label);
#define RADIOBUTTONS_ENUM(labelProc,min,max) { int itext; for (itext = min; itext <= max; itext ++) RADIOBUTTON (labelProc) }
#define OPTIONS_ENUM(labelProc,min,max) { int itext; for (itext = min; itext <= max; itext ++) OPTION (labelProc) }
#define RADIO_ENUM(label,enum,def) \
	RADIO (label, enum##_##def - enum##_MIN + 1) \
	for (int ienum = enum##_MIN; ienum <= enum##_MAX; ienum ++) \
		OPTION (enum##_getText (ienum))
#define OPTIONMENU_ENUM(label,enum,def) \
	OPTIONMENU (label, enum##_##def - enum##_MIN + 1) \
	for (int ienum = enum##_MIN; ienum <= enum##_MAX; ienum ++) \
		OPTION (enum##_getText (ienum))
#define LIST(label,n,str,def)	cmd->_dialog->addList (label, n, str, def);
#define SET_REAL(name,value)	cmd->_dialog->setReal (name, value);
#define SET_INTEGER(name,value)	cmd->_dialog->setInteger (name, value);
#define SET_STRING(name,value)	cmd->_dialog->setString (name, value);
#define SET_ENUM(name,enum,value)  SET_STRING (name, enum##_getText (value))

#define DIALOG  cmd -> _dialog

#define EDITOR_ARGS  Editor *editor_me, EditorCommand *cmd, UiForm *sendingForm, const wchar_t *sendingString, Interpreter *interpreter
#define EDITOR_IAM(klas)  iam (klas); (void) me; (void) cmd; (void) sendingForm; (void) sendingString; (void) interpreter
#define EDITOR_FORM(title,helpTitle)  if (cmd -> _dialog == NULL) { UiForm::UiField *radio = NULL; (void) radio; \
	cmd -> _dialog = UiForm::createE (cmd, title, cmd -> _itemTitle, helpTitle);
#define EDITOR_OK  cmd->_dialog->finish (); } if (sendingForm == NULL && sendingString == NULL) {
#define EDITOR_DO  cmd->_dialog->do_ (false); } else if (sendingForm == NULL) { \
	if (! UiForm::parseStringE (cmd, sendingString, interpreter)) return 0; } else {
#define EDITOR_END  } return 1;

#define EDITOR_FORM_WRITE(title,helpTitle) \
	if (cmd -> _dialog == NULL) { \
		cmd -> _dialog = UiOutfile::createE (cmd, title, cmd -> _itemTitle, helpTitle); \
		} if (sendingForm == NULL && sendingString == NULL) { wchar_t defaultName [300]; defaultName [0] = '\0';
#define EDITOR_DO_WRITE \
	cmd -> _dialog -> do_ (defaultName); } else { MelderFile file; structMelderFile file2 = { 0 }; \
		if (sendingString == NULL) file = ((UiFile *)sendingForm)->getFile (); \
		else { if (! Melder_relativePathToFile (sendingString, & file2)) return 0; file = & file2; }

#define GET_REAL(name)  cmd->_dialog->getReal (name)
#define GET_INTEGER(name)  cmd->_dialog->getInteger (name)
#define GET_STRING(name)  cmd->_dialog->getString (name)
#ifdef __cplusplus
	#define GET_ENUM(enum,name)  (enum) enum##_getValue (GET_STRING (name))
#else
	#define GET_ENUM(enum,name)  enum##_getValue (GET_STRING (name))
#endif
#define GET_FILE  cmd->_dialog->getFile (cmd -> _dialog)

/* End of file EditorM.h */
