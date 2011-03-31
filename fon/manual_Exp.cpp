/* manual_Exp.c
 *
 * Copyright (C) 2001-2011 Paul Boersma
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

#include "ManPagesM.h"

extern "C" void manual_Exp_init (ManPages me);
extern "C" void manual_Exp_init (ManPages me) {

MAN_BEGIN (L"ExperimentMFC", L"ppgb", 20051205)
INTRO (L"One of the @@types of objects@ in Praat, "
	"for running a Multiple Forced Choice listening experiment.")
LIST_ITEM (L"@@ExperimentMFC 1. When to use Praat")
LIST_ITEM (L"@@ExperimentMFC 2. The first example")
LIST_ITEM (L"@@ExperimentMFC 2.1. The experiment file")
LIST_ITEM (L"@@ExperimentMFC 2.2. The stimuli")
LIST_ITEM (L"@@ExperimentMFC 2.3. The carrier phrase")
LIST_ITEM (L"@@ExperimentMFC 2.4. Breaks")
LIST_ITEM (L"@@ExperimentMFC 2.5. Randomization strategies")
LIST_ITEM (L"@@ExperimentMFC 2.6. Instructions")
LIST_ITEM (L"@@ExperimentMFC 2.7. Response categories")
LIST_ITEM (L"@@ExperimentMFC 2.8. Goodness judgments")
LIST_ITEM (L"@@ExperimentMFC 2.9. How an experiment proceeds")
LIST_ITEM (L"@@ExperimentMFC 3. More examples")
LIST_ITEM (L"@@ExperimentMFC 3.1. A simple discrimination experiment")
LIST_ITEM (L"@@ExperimentMFC 3.2. An AXB discrimination experiment")
LIST_ITEM (L"@@ExperimentMFC 3.3. A 4I-oddity experiment")
LIST_ITEM (L"@@ExperimentMFC 3.4. Variable inter-stimulus intervals")
LIST_ITEM (L"@@ExperimentMFC 4. Special buttons")
LIST_ITEM (L"@@ExperimentMFC 4.1. The replay button")
LIST_ITEM (L"@@ExperimentMFC 4.2. The OK button")
LIST_ITEM (L"@@ExperimentMFC 4.3. The oops button")
LIST_ITEM (L"@@ExperimentMFC 5. Stimulus-dependent texts")
LIST_ITEM (L"@@ExperimentMFC 5.1. The stimulus-dependent run text")
LIST_ITEM (L"@@ExperimentMFC 5.2. Stimulus-dependent response buttons")
LIST_ITEM (L"@@ExperimentMFC 6. Responses are sounds")
LIST_ITEM (L"@@ExperimentMFC 7. Running multiple experiments")
MAN_END

MAN_BEGIN (L"ExperimentMFC 1. When to use Praat", L"ppgb", 20110303)
NORMAL (L"With Praat's ExperimentMFC, you can do simple experiments on identification and discrimination. "
	"`Simple' means that for identification, the subject hears a sound and has to click on one of a set of "
	"labelled rectangles (optionally, you can have the subject give a goodness-of-fit judgment). "
	"For discrimination, you can have simple same-different choices, or more intricate things like AXB, 4I-oddity, and so on.")
NORMAL (L"The advantage of using Praat's ExperimentMFC for this is that it is free, it works on Windows, Unix, and Macintosh, "
	"and the whole experiment (experiment file plus sound files) is portable across computers "
	"(you can run it from a CD, for instance). Because of the limited possibilities, "
	"it is also quite easy to set up the experiment. Just read the description below.")
NORMAL (L"If you require more from your experiment design, you can use Praat's @@Demo window@; "
	"with that less simple method you could for instance let the stimulus depend on the subject's previous responses. "
	"Alternatively, you could use a dedicated program like Presentation or E-prime instead of Praat; "
	"with these programs, you can also measure reaction times more accurately.")
MAN_END

MAN_BEGIN (L"ExperimentMFC 2. The first example", L"ppgb", 20051205)
INTRO (L"The following pages give an example of an experiment definition, "
	"and explain the main features of an identification task.")
LIST_ITEM (L"@@ExperimentMFC 2.1. The experiment file")
LIST_ITEM (L"@@ExperimentMFC 2.2. The stimuli")
LIST_ITEM (L"@@ExperimentMFC 2.3. The carrier phrase")
LIST_ITEM (L"@@ExperimentMFC 2.4. Breaks")
LIST_ITEM (L"@@ExperimentMFC 2.5. Randomization strategies")
LIST_ITEM (L"@@ExperimentMFC 2.6. Instructions")
LIST_ITEM (L"@@ExperimentMFC 2.7. Response categories")
LIST_ITEM (L"@@ExperimentMFC 2.8. Goodness judgments")
LIST_ITEM (L"@@ExperimentMFC 2.9. How an experiment proceeds")
MAN_END

MAN_BEGIN (L"ExperimentMFC 2.1. The experiment file", L"ppgb", 20081123)
INTRO (L"An experiment is defined in a simple text file, which we call an %%experiment file%. "
	"The following is an example of such an experiment file. The first two lines have to be typed "
	"exactly as in this example, the rest depends on your stimuli, on your response categories, "
	"and on the way the experiment is to be presented to the listener. "
	"The order of the elements in this file cannot be changed, and nothing can be skipped.")
CODE (L"\"ooTextFile\"")
CODE (L"\"ExperimentMFC 5\"")
CODE (L"stimuliAreSounds? <yes>")
CODE (L"stimulusFileNameHead = \"Sounds/\"")
CODE (L"stimulusFileNameTail = \".wav\"")
CODE (L"stimulusCarrierBefore = \"weSayTheWord\"")
CODE (L"stimulusCarrierAfter = \"again\"")
CODE (L"stimulusInitialSilenceDuration = 0.5 seconds")
CODE (L"stimulusMedialSilenceDuration = 0")
CODE (L"numberOfDifferentStimuli = 4")
CODE1 (L"\"heed\"  \"\"")
CODE1 (L"\"hid\"   \"\"")
CODE1 (L"\"hood\"  \"\"")
CODE1 (L"\"hud\"   \"\"")
CODE (L"numberOfReplicationsPerStimulus = 3")
CODE (L"breakAfterEvery = 0")
CODE (L"randomize = <PermuteBalancedNoDoublets>")
CODE (L"startText = \"This is a listening experiment.")
CODE (L"After hearing a sound, choose the vowel that is most similar to what you heard.")
CODE (L"")
CODE (L"Click to start.\"")
CODE (L"runText = \"Choose the vowel that you heard.\"")
CODE (L"pauseText = \"You can have a short break if you like. Click to proceed.\"")
CODE (L"endText = \"The experiment has finished.\"")
CODE (L"maximumNumberOfReplays = 0")
CODE (L"replayButton = 0 0 0 0 \"\" \"\"")
CODE (L"okButton = 0 0 0 0 \"\" \"\"")
CODE (L"oopsButton = 0 0 0 0 \"\" \"\"")
CODE (L"responsesAreSounds? <no> \"\" \"\" \"\" \"\" 0 0")
CODE (L"numberOfDifferentResponses = 5")
CODE1 (L"0.2 0.3 0.7 0.8 \"h I d\" 40 \"\" \"i\"")
CODE1 (L"0.3 0.4 0.5 0.6 \"h E d\" 40 \"\" \"e\"")
CODE1 (L"0.4 0.5 0.3 0.4 \"h A d\" 40 \"\" \"a\"")
CODE1 (L"0.5 0.6 0.5 0.6 \"h O d\" 40 \"\" \"o\"")
CODE1 (L"0.6 0.7 0.7 0.8 \"h U d\" 40 \"\" \"u\"")
CODE (L"numberOfGoodnessCategories = 5")
CODE1 (L"0.25 0.35 0.10 0.20 \"1 (poor)\"")
CODE1 (L"0.35 0.45 0.10 0.20 \"2\"")
CODE1 (L"0.45 0.55 0.10 0.20 \"3\"")
CODE1 (L"0.55 0.65 0.10 0.20 \"4\"")
CODE1 (L"0.65 0.75 0.10 0.20 \"5 (good)\"")
NORMAL (L"This experiment will play 4 different stimuli to the listener, each 3 times. "
	"Thus, the listener is confronted with 12 trials.")
MAN_END

MAN_BEGIN (L"ExperimentMFC 2.2. The stimuli", L"ppgb", 20080330)
INTRO (L"You can see that the @@ExperimentMFC 2.1. The experiment file|example experiment@ "
	"contains four different stimuli: %heed, %hid, %hood, and %hud. "
	"These are the %names of the four stimuli.")
NORMAL (L"Because in the example experiment stimuli are sounds, "
	"each of the four stimuli should be in a sound file. "
	"The names of these sound files must be identical to the names "
	"of the stimuli, bracketed with %stimulusFileNamehead and %stimulusFileNameTail. "
	"Hence, the stimuli are expected in the following four files:")
LIST_ITEM (L"Sounds/heed.wav")
LIST_ITEM (L"Sounds/hid.wav")
LIST_ITEM (L"Sounds/hood.wav")
LIST_ITEM (L"Sounds/hud.wav")
NORMAL (L"You need not use WAV files. You can also use AIFF files, "
	"in which case %stimulusFileNameTail would probably be \".aiff\", or any other "
	"type of sound file that Praat supports. But all sound files must have the same number of channels "
	"(i.e. all mono or all stereo) and the same sampling frequency.")
NORMAL (L"In this example, the experiment will look for the sound files in the directory #Sounds, "
	"which has to be in the same directory as your experiment file. "
	"In other words, \"Sounds/heed.wav\" is a %%relative file path%.")
NORMAL (L"Instead of a relative path, you can also supply a %%full file path%. "
	"Such a path depends on your computer and on your operating system. "
	"For instance, if you have a Windows computer and the stimuli are in the directory ##D:\\bsCorpus\\bsAutumn\\bsSpeaker23#, "
	"you can write")
CODE (L"fileNameHead = \"D:\\bsCorpus\\bsAutumn\\bsSpeaker23\\bs\"")
NORMAL (L"If you have a Macintosh (OS X) or Unix computer and the stimuli are in ##/Users/mietta/Sounds/Dutch#, you write")
CODE (L"fileNameHead = \"/Users/mietta/Sounds/Dutch/\"")
NORMAL (L"But relative file paths will usually be preferred: they are more %portable. "
	"The advantage of using relative file paths is that you can move your whole experiment (experiment file plus sounds) "
	"from one computer to another without changing the experiment file, "
	"as long as you put the experiment file in the same directory as where you put the directory #Sounds. "
	"Or you can put the whole experiment on a CD and run the experiment directly from the CD. "
	"Since Praat supports the forward slash \"/\" as a directory separator on all computers, "
	"you can run the exact same experiment on Macintosh, Windows and Unix computers, "
	"independent of the type of computer where you have created your experiment.")
MAN_END

MAN_BEGIN (L"ExperimentMFC 2.3. The carrier phrase", L"ppgb", 20051205)
NORMAL (L"The stimuli can be embedded in a %%carrier phrase%. "
	"In the @@ExperimentMFC 2.1. The experiment file|example experiment@, we see that the stimulus "
	"will be inserted between the sounds in the files ##weSayTheWord.wav# "
	"and ##again.wav#, both of which are expected to be in the directory #Sounds. "
	"If you do not want a carrier phrase, you do")
CODE (L"stimulusCarrierBefore = \"\"")
CODE (L"stimulusCarrierAfter = \"\"")
NORMAL (L"If you want only an introductory phrase before the stimulus, and nothing after the stimulus, "
	"you do something like")
CODE (L"stimulusCarrierBefore = \"listenTo\"")
CODE (L"stimulusCarrierAfter = \"\"")
NORMAL (L"and of course you supply the file ##listenTo.wav# in the directory #Sounds.")
NORMAL (L"If you want to have a short silence before every stimulus (and before the carrier phrase), "
	"you supply a non-zero %stimulusInitialSilenceDuration, as in the example.")
NORMAL (L"Since the carrier phrase is concatenated with the stimulus before it is played, it should have the same "
	"sampling frequency as the stimulus files.")
MAN_END

MAN_BEGIN (L"ExperimentMFC 2.4. Breaks", L"ppgb", 20051205)
NORMAL (L"A new stimulus will arrive as soon as the listener makes her choice. To allow her some breathing "
	"time, you can insert a break after every so many trials. In the example, %breakAfterEvery is 0, "
	"because there are only 12 trials. A typical experiment has perhaps 180 trials, and you may want to "
	"insert a break after every 40 trials. In that case, you do")
CODE (L"breakAfterEvery = 40")
MAN_END

MAN_BEGIN (L"ExperimentMFC 2.5. Randomization strategies", L"ppgb", 20051205)
NORMAL (L"The 3 times 4 stimuli will have to be played in a certain order. For the least random order, you say")
CODE (L"randomize = <CyclicNonRandom>")
NORMAL (L"In this case, the stimuli will be played in the order in which they were specified in the file, 3 times:")
FORMULA (L"heed hid hood hud heed hid hood hud heed hid hood hud")
NORMAL (L"The most likely case in which you would want to use this randomization strategy, is if you have, say, 120 "
	"different stimuli and you want to play them only once (%numberOfReplicationsPerStimulus = 1) in a fixed order.")
NORMAL (L"The other extreme, the most random order, is")
CODE (L"randomize = <WithReplacement>")
NORMAL (L"In this case, a stimulus will be chosen at random 12 times without memory, for instance")
FORMULA (L"hid hood hood heed hid hood hud hud hid hood heed hid")
NORMAL (L"The order will probably be different for each listener. "
	"In this example, %hood and %hid occur four times each, %heed and %hud only twice each. "
	"This strategy is too random for most experiments. Usually, you will want to have the same number "
	"of replications of each stimulus. The most random way to do this is")
CODE (L"randomize = <PermuteAll>")
NORMAL (L"In this case, all stimuli will be played exactly 3 times, for instance")
FORMULA (L"heed hood hud hud hid heed heed hud hood hid hid hood")
NORMAL (L"Quite often, you will want a less random order, namely one in which the 12 trials are divided into "
	"3 blocks of 4 stimuli. Within each block, all 4 different stimuli occur in a random order:")
CODE (L"randomize = <PermuteBalanced>")
NORMAL (L"In this case, each stimulus occurs exactly once within each block:")
FORMULA (L"heed hood hud hid hood hud hid heed heed hud hood hid")
NORMAL (L"This strategy ensures a certain spreading of the stimuli over the sequence of 12 trials. "
	"As we see here, it is still possible that the same stimulus (%heed) occurs twice in a row, "
	"namely as the last stimulus of the second block and the first stimulus of the third. "
	"If you want to prevent that situation, you use")
CODE (L"randomize = <PermuteBalancedNoDoublets>")
NORMAL (L"This will ensure that the same stimulus is never applied twice in a row:")
FORMULA (L"heed hood hud hid hood hud hid heed hud heed hood hid")
NORMAL (L"This randomization strategy is used in our example, and advised for most listening experiments "
	"in which you want to minimize effects of stimulus order.")
NORMAL (L"The randomization procedure does not interfere in any way with the breaks. The order is determined "
	"before any breaks are inserted.")
MAN_END

MAN_BEGIN (L"ExperimentMFC 2.6. Instructions", L"ppgb", 20081123)
NORMAL (L"Before the experiment begins, the listener will see the %startText in the centre of the screen. "
	"During each trial, she will see the %runText at the top of the screen. "
	"During breaks, she will see the %pauseText in the centre of the screen. "
	"After all the trials have been performed, she will see the %endText. "
	"As you can see in the example, all these texts can consist of multiple lines.")
MAN_END

MAN_BEGIN (L"ExperimentMFC 2.7. Response categories", L"ppgb", 20110109)
NORMAL (L"Every trial comes with the same set of response categories. "
	"The @@ExperimentMFC 2.1. The experiment file|example experiment@ has five of them. "
	"For each response category, you supply the area of the screen where a rectangle will be drawn. "
	"The whole screen measures from 0.0 (left) to 1.0 (right) and from 0.0 (bottom) to 1.0 (top). "
	"Thus, \"0.2 0.3 0.7 0.8\" means that a rectangle will be drawn somewhere in the top left quadrant "
	"of the screen. You also supply the text that will be drawn in this rectangle, for instance the text \"h I d\". "
	"After this you supply the font size for this text, for instance 40.")
NORMAL (L"The second text that you supply for every response is a response key on the keyboard. "
	"In the above example this is \"\", i.e. the subject cannot press a key as a response. "
	"If you want the user to be able to press the \"m\" key instead of clicking in the \"h I d\" rectangle, "
	"the line in the experiment file would be:")
CODE1 (L"0.2 0.3 0.7 0.8 \"h I d\" 40 \"m\" \"i\"")
NORMAL (L"The third text that you supply for each rectangle is the response category as it will be reported by Praat to you when the user clicks it, "
	"e.g. the text \"i\". If you want Praat to ignore mouse clicks on this rectangle, specify an empty response "
	"category, i.e. \"\".")
NORMAL (L"The border of the rectangles will be maroon, the background of the screen will be light grey. "
	"The colour of clickable rectangles will be yellow, that of non-clickable rectangles (those with "
	"empty category specifications) light grey.")
NORMAL (L"You can have a picture instead of a text on a response button, by using \\bsFI:")
CODE1 (L"0.2 0.3 0.7 0.8 \"\\bsFIpictures/hello.jpg\" 40 \"m\" \"i\"")
NORMAL (L"In this example, the picture ##hello.jpg# from the subdirectory #pictures "
	"(i.e. a subdirectory of the directory where your experiment file is) "
	"will be drawn into the rectangle [0.2, 0.3] \\xx [0.7, 0.8]. "
	"This currently (January 2011) works only on the Mac.")
MAN_END

MAN_BEGIN (L"ExperimentMFC 2.8. Goodness judgments", L"ppgb", 20051205)
NORMAL (L"If %numberOfGoodnessCategories is not 0, some more rectangles will be drawn, "
	"as in the @@ExperimentMFC 2.1. The experiment file|example experiment@. "
	"You specify again the locations of these rectangles (in the example, they touch each other), "
	"and the texts on them. Praat will record the number of the button when the listener clicks on it. "
	"Thus, if she clicks on the button labelled \"1 (poor)\", Praat will record a goodness judgment of 1, "
	"because this is the first button in the list. If she clicks on \"5 (good)\", Praat will record a "
	"goodness judgment of 5.")
MAN_END

MAN_BEGIN (L"ExperimentMFC 2.9. How an experiment proceeds", L"ppgb", 20110317)
NORMAL (L"A text file with an ExperimentMFC object can be read into Praat with @@Read from file...@ "
	"(it is not a script but a data file, so do not try to read it with ##Open Praat script...#). "
	"You can then choose #Run. After the experiment finishes, you can close the experiment window "
	"and choose ##Extract results#. The resulting ResultsMFC object contains for each trial the stimulus "
	"name (e.g. \"hood\"), the response category (e.g. \"u\"), and the goodness judgment (e.g. 4). "
	"You will want to save this ResultsMFC object to a text file with @@Save as text file...@. "
	"You may want to call these text files by the names of the subjects, e.g. ##ts.ResultsMFC# "
	"and ##mj.ResultsMFC#. Once you have collected the results of all your subjects, you can read "
	"all the results files into Praat with @@Read from file...@, then select all the resulting "
	"ResultsMFC objects (which will have automatically been named #ts, #mj, and so on), then choose "
	"##Collect to table#. This will result in a table whose first column contains the names of the subjects, "
	"the second column contains the stimulus names, the third column contains the responses, "
	"and the last column contains the approximate reaction times (measured from the start of the stimulus sound, i.e. after the initial silence duration). "
	"If there are goodness judgments, these will go into the fourth column. The table can be saved "
	"as a table file (with ##Save as tab-separated file...#), which can be read by programs like Excel and SPSS.")
MAN_END

MAN_BEGIN (L"ExperimentMFC 3. More examples", L"ppgb", 20051205)
INTRO (L"You can do many more kinds of experiments than simple identification experiments.")
LIST_ITEM (L"@@ExperimentMFC 3.1. A simple discrimination experiment")
LIST_ITEM (L"@@ExperimentMFC 3.2. An AXB discrimination experiment")
LIST_ITEM (L"@@ExperimentMFC 3.3. A 4I-oddity experiment")
LIST_ITEM (L"@@ExperimentMFC 3.4. Variable inter-stimulus intervals")
MAN_END

MAN_BEGIN (L"ExperimentMFC 3.1. A simple discrimination experiment", L"ppgb", 20070926)
NORMAL (L"The @@ExperimentMFC 2.1. The experiment file|example experiment@ was an %identification experiment: "
	"the subject had identify a single sound as one element of a set of categories. "
	"Phoneticians will often do %discrimination experiments, which are experiments in which "
	"a stimulus consists of multiple sub-stimuli played in sequence, and the subject has to judge the similarity "
	"between these sub-stimuli.")
NORMAL (L"The simplest discrimination task has only two sub-stimuli, and the subject has to say whether these are "
	"the %same or %different. Suppose you have vowel-like sounds along an F1 continuum with seven steps, "
	"say 300, 320, 340, 360, 380, 400, and 420 Hertz, and you are interested in knowing how well the listeners "
	"can distinguish these. As your stimuli, you create pairs of these sounds, separated by 0.8 seconds of silence. "
	"It is important to include stimuli in which the sounds are identical, e.g. stimuli in which both sounds have an F1 "
	"of 340 Hz (see the literature on signal detection theory). Since sounds that are very different acoustically "
	"will always be heard as different, you do not include pairs in which the distance is larger than 60 Hz. "
	"The experiment file will look like this:")
CODE (L"\"ooTextFile\"")
CODE (L"\"ExperimentMFC 5\"")
CODE (L"stimuli are sounds? <yes>")
CODE (L"\"stimuli/\"  \".wav\"")
CODE (L"carrier phrase \"\"  \"\"")
CODE (L"initial silence duration 0.5 seconds")
CODE (L"medial silence duration 0.8 seconds  ! inter-stimulus interval")
CODE (L"37 different stimuli")
CODE1 (L"\"300,300\"  \"\"  \"300,320\"  \"\"  \"300,340\"  \"\"  \"300,360\"  \"\"")
CODE1 (L"\"320,300\"  \"\"  \"320,320\"  \"\"  \"320,340\"  \"\"  \"320,360\"  \"\"  \"320,380\"  \"\"")
CODE1 (L"\"340,300\"  \"\"  \"340,320\"  \"\"  \"340,340\"  \"\"  \"340,360\"  \"\"  \"340,380\"  \"\"  \"340,400\"  \"\"")
CODE1 (L"\"360,300\"  \"\"  \"360,320\"  \"\"  \"360,340\"  \"\"  \"360,360\"  \"\"  \"360,380\"  \"\"  \"360,400\"  \"\"  \"360,420\"  \"\"")
CODE1 (L"\"380,320\"  \"\"  \"380,340\"  \"\"  \"380,360\"  \"\"  \"380,380\"  \"\"  \"380,400\"  \"\"  \"380,420\"  \"\"")
CODE1 (L"\"400,340\"  \"\"  \"400,360\"  \"\"  \"400,380\"  \"\"  \"400,400\"  \"\"  \"400,420\"  \"\"")
CODE1 (L"\"420,360\"  \"\"  \"420,380\"  \"\"  \"420,400\"  \"\"  \"420,420\"")
CODE (L"10 replications per stimulus")
CODE (L"break after every 50 stimuli")
CODE (L"<PermuteBalancedNoDoublets>")
CODE (L"\"Click to start.\"")
CODE (L"\"Say whether these sounds were the same or different.\"")
CODE (L"\"You can have a short break if you like. Click to proceed.\"")
CODE (L"\"The experiment has finished. Call the experimenter.\"")
CODE (L"0 replays")
CODE (L"replay button 0 0 0 0 \"\" \"\"")
CODE (L"ok button 0 0 0 0 \"\" \"\"")
CODE (L"oops button 0 0 0 0 \"\" \"\"")
CODE (L"responses are sounds? <no> \"\" \"\" \"\" \"\" 0 0")
CODE (L"2 response categories")
CODE1 (L"0.1 0.4 0.35 0.65 \"same\" 24 \"\" \"same\"")
CODE1 (L"0.6 0.9 0.35 0.65 \"different\" 24 \"\" \"different\"")
CODE (L"0 goodness categories")
NORMAL (L"In this example, the subject will have to click 370 times. After every 50 times, she will have the "
	"opportunity to sip her tea. A 0.5-seconds silence is played before every stimulus, so that the listener "
	"will not hear the stimulus immediately after her mouse click.")
NORMAL (L"The experimenter does not have to create the stimulus pairs as sound files. "
	"You can specify multiple sound files by separating them with commas. Thus, \"320,300\" means that "
	"Praat will play the files ##320.wav# and ##300.wav#. These two substimili will be separated here by a silence "
	"of 0.8 seconds, called the %%inter-stimulus interval% (or %stimulusMedialSilenceDuration).")
NORMAL (L"Note that the text in this file is rather different from the previous example. "
	"It does not matter whether you write \"numberOfDifferentStimuli\", or \"different stimuli\", or anything else; "
	"Praat ignores these texts as long as they do not contain numbers, quoted strings, or things between <>.")
MAN_END

MAN_BEGIN (L"ExperimentMFC 3.2. An AXB discrimination experiment", L"ppgb", 20070926)
INTRO (L"In the AXB task, the subject will hear three stimuli in sequence, and has to say "
	"whether the second (X) is more similar to the first (A) or to the second (B). "
	"An experiment file could look like follows:")
CODE (L"\"ooTextFile\"")
CODE (L"\"ExperimentMFC 5\"")
CODE (L"stimuliAreSounds? <yes>")
CODE (L"\"stimuli/\"  \".wav\"")
CODE (L"carrier \"\"  \"\"")
CODE (L"initial silence 0.5")
CODE (L"inter-stimulus interval 0.3")
CODE (L"100 stimuli")
CODE1 (L"\"300,300,320\"  \"\"  \"300,320,340\"  \"\"  \"300,340,340\"  \"\"  \"300,340,360\"  \"\"")
CODE1 (L"...")
CODE1 (L"(and 96 more triplets of substimuli)")
CODE1 (L"...")
CODE (L"4 replications")
CODE (L"break every 50")
CODE (L"<PermuteBalancedNoDoublets>")
CODE (L"\"Click to start.\"")
CODE (L"\"Say whether the second sound is more similar to the first or to the third.\"")
CODE (L"\"You can have a short break if you like. Click to proceed.\"")
CODE (L"\"The experiment has finished.\"")
CODE (L"0 replays")
CODE (L"replay button 0 0 0 0 \"\" \"\"")
CODE (L"ok button 0 0 0 0 \"\" \"\"")
CODE (L"oops button 0 0 0 0 \"\" \"\"")
CODE (L"responses are sounds? <no> \"\" \"\" \"\" \"\" 0 0")
CODE (L"3 response categories")
CODE1 (L"0.1 0.3 0.4 0.6 \"first\" 30 \"\" \"A\"")
CODE1 (L"0.4 0.6 0.4 0.6 \"second\" 30 \"\" \"\"")
CODE1 (L"0.7 0.9 0.4 0.6 \"third\" 30 \"\" \"B\"")
CODE (L"0 goodness categories")
NORMAL (L"In this example, the subject has to click 400 times. She sees three buttons, "
	"labelled %first, %second, and %third, but the second button (the one with the empty response category) "
	"is not clickable: it has a light grey rather than a yellow interior and cannot be chosen by the subject. "
	"In your ResultsMFC object, you will only see %A and %B responses.")
MAN_END

MAN_BEGIN (L"ExperimentMFC 3.3. A 4I-oddity experiment", L"ppgb", 20070926)
NORMAL (L"In the four-items-oddity task, the subject will hear four stimuli in sequence, and has to say "
	"whether the second or the third is the odd one out. The other three substimuli are identical. "
	"An experiment file could look as follows:")
CODE (L"\"ooTextFile\"")
CODE (L"\"ExperimentMFC 5\"")
CODE (L"stimuliAreSounds? <yes>")
CODE (L"stimulusFileNameHead = \"stimuli/\"")
CODE (L"stimulusFileNameTail = \".wav\"")
CODE (L"stimulusCarrierBefore = \"\"")
CODE (L"stimulusCarrierAfter = \"\"")
CODE (L"stimulusInitialSilenceDuration = 0.5 seconds")
CODE (L"stimulusMedialSilenceDuration = 0.3 seconds")
CODE (L"numberOfDifferentStimuli = 60")
CODE1 (L"\"300,300,320,300\"  \"\"  \"300,320,300,300\"  \"\"")
CODE1 (L"\"300,300,340,300\"  \"\"  \"300,340,300,300\"  \"\"")
CODE1 (L"\"300,300,360,300\"  \"\"  \"300,360,300,300\"  \"\"")
CODE1 (L"\"320,320,300,320\"  \"\"  \"320,300,320,320\"  \"\"")
CODE1 (L"\"320,320,340,320\"  \"\"  \"320,340,320,320\"  \"\"")
CODE1 (L"\"320,320,360,320\"  \"\"  \"320,360,320,320\"  \"\"")
CODE1 (L"\"320,320,380,320\"  \"\"  \"320,380,320,320\"  \"\"")
CODE1 (L"\"340,340,300,340\"  \"\"  \"340,300,340,340\"  \"\"")
CODE1 (L"\"340,340,320,340\"  \"\"  \"340,320,340,340\"  \"\"")
CODE1 (L"\"340,340,360,340\"  \"\"  \"340,360,340,340\"  \"\"")
CODE1 (L"\"340,340,380,340\"  \"\"  \"340,380,340,340\"  \"\"")
CODE1 (L"\"340,340,400,340\"  \"\"  \"340,400,340,340\"  \"\"")
CODE1 (L"\"360,360,300,360\"  \"\"  \"360,300,360,360\"  \"\"")
CODE1 (L"\"360,360,320,360\"  \"\"  \"360,320,360,360\"  \"\"")
CODE1 (L"\"360,360,340,360\"  \"\"  \"360,340,360,360\"  \"\"")
CODE1 (L"\"360,360,380,360\"  \"\"  \"360,380,360,360\"  \"\"")
CODE1 (L"\"360,360,400,360\"  \"\"  \"360,400,360,360\"  \"\"")
CODE1 (L"\"360,360,420,360\"  \"\"  \"360,420,360,360\"  \"\"")
CODE1 (L"\"380,380,320,380\"  \"\"  \"380,320,380,380\"  \"\"")
CODE1 (L"\"380,380,340,380\"  \"\"  \"380,340,380,380\"  \"\"")
CODE1 (L"\"380,380,360,380\"  \"\"  \"380,360,380,380\"  \"\"")
CODE1 (L"\"380,380,400,380\"  \"\"  \"380,400,380,380\"  \"\"")
CODE1 (L"\"380,380,420,380\"  \"\"  \"380,420,380,380\"  \"\"")
CODE1 (L"\"400,400,340,400\"  \"\"  \"400,340,400,400\"  \"\"")
CODE1 (L"\"400,400,360,400\"  \"\"  \"400,360,400,400\"  \"\"")
CODE1 (L"\"400,400,380,400\"  \"\"  \"400,380,400,400\"  \"\"")
CODE1 (L"\"400,400,420,400\"  \"\"  \"400,420,400,400\"  \"\"")
CODE1 (L"\"420,420,360,420\"  \"\"  \"420,360,420,420\"  \"\"")
CODE1 (L"\"420,420,380,420\"  \"\"  \"420,380,420,420\"  \"\"")
CODE1 (L"\"420,420,400,420\"  \"\"  \"420,400,420,420\"  \"\"")
CODE (L"numberOfReplicationsPerStimulus = 5")
CODE (L"breakAfterEvery = 40")
CODE (L"randomize = <PermuteBalancedNoDoublets>")
CODE (L"startText = \"Click to start.\"")
CODE (L"runText = \"Say whether the second or the third sound is different from the rest.\"")
CODE (L"pauseText = \"You can have a short break if you like. Click to proceed.\"")
CODE (L"endText = \"The experiment has finished.\"")
CODE (L"maximumNumberOfReplays = 0")
CODE (L"replayButton = 0 0 0 0 \"\" \"\"")
CODE (L"okButton = 0 0 0 0 \"\" \"\"")
CODE (L"oopsButton = 0 0 0 0 \"\" \"\"")
CODE (L"responsesAreSounds? <no>")
CODE (L"responseFileNameHead = \"\"")
CODE (L"responseFileNameTail = \"\"")
CODE (L"responseCarrierBefore = \"\"")
CODE (L"responseCarrierAfter = \"\"")
CODE (L"responseInitialSilenceDuration = 0")
CODE (L"responseMedialSilenceDuration = 0")
CODE (L"numberOfResponseCategories = 4")
CODE1 (L"0.04 0.24 0.4 0.6 \"first\" 30 \"\" \"\"")
CODE1 (L"0.28 0.48 0.4 0.6 \"second\" 30 \"\" \"2\"")
CODE1 (L"0.52 0.72 0.4 0.6 \"third\" 30 \"\" \"3\"")
CODE1 (L"0.76 0.96 0.4 0.6 \"fourth\" 30 \"\" \"\"")
CODE (L"numberOfGoodnessCategories = 0")
NORMAL (L"In this example, the subject has to click 300 times. She sees four buttons, "
	"but the first and fourth buttons cannot be chosen. "
	"In your ResultsMFC object, you will only see the responses %2 and %3.")
MAN_END

MAN_BEGIN (L"ExperimentMFC 3.4. Variable inter-stimulus intervals", L"ppgb", 20070225)
NORMAL (L"Praat only supports a fixed inter-stimulus interval, but sometimes you may want to test "
	"discrimination as a function of the inter-stimulus interval itself. You can achieve this by "
	"supplying an %interStimulusInterval of 0 and using sound files with various silences:")
CODE1 (L"\"300,silence0.5,320\"  \"300,silence1.5,320\"  \"300,silence4.5,320\"")
NORMAL (L"In this example, you have to supply the sound files ##silence0.5.wav# and so on. You can "
	"create them with the help of @@Create Sound from formula...@ (supply a %formula of 0).")
MAN_END

MAN_BEGIN (L"ExperimentMFC 4. Special buttons", L"ppgb", 20051205)
INTRO (L"You can include up to three special buttons on the screen that the participant sees. "
	"It is probably inadvisable to use all three at the same time.")
LIST_ITEM (L"@@ExperimentMFC 4.1. The replay button")
LIST_ITEM (L"@@ExperimentMFC 4.2. The OK button")
LIST_ITEM (L"@@ExperimentMFC 4.3. The oops button")
MAN_END

MAN_BEGIN (L"ExperimentMFC 4.1. The replay button", L"ppgb", 20051205)
INTRO (L"The @@ExperimentMFC 2.1. The experiment file|example experiment@ contained the following lines:")
CODE (L"maximumNumberOfReplays = 0")
CODE (L"replayButton = 0 0 0 0 \"\" \"\"")
NORMAL (L"This means that that experiment did not have a replay button. "
	"To add a replay button along the lower edge of the screen, you do something like")
CODE (L"maximumNumberOfReplays = 1000")
CODE (L"replayButton = 0.3 0.7 0.01 0.07 \"Click here to play the last sound again\" \"\"")
NORMAL (L"If you supply a right edge (here 0.7) that is greater than the left edge (here 0.3), "
	"Praat will know that you want to show a replay button.")
NORMAL (L"When the participant clicks this button, Praat will play the current stimulus again. "
	"In this example, the button will be visible until the partipant has clicked it 1000 times.")
NORMAL (L"To assign a keyboard shortcut to the replay button, do something like")
CODE (L"maximumNumberOfReplays = 1000")
CODE (L"replayButton = 0.1 0.9 0.01 0.07 \"Click here or press the space bar to play the last sound again\" \" \"")
MAN_END

MAN_BEGIN (L"ExperimentMFC 4.2. The OK button", L"ppgb", 20051205)
INTRO (L"The @@ExperimentMFC 2.1. The experiment file|example experiment@ contained the following lines:")
CODE (L"okButton = 0 0 0 0 \"\" \"\"")
NORMAL (L"This means that that experiment did not have an OK button. "
	"To add an OK button in the lower right corner of the screen, you do something likw")
CODE (L"okButton = 0.8 0.95 0.05 0.15 \"OK\" \"\"")
NORMAL (L"If you supply a right edge (here 0.95) that is greater than the left edge (here 0.8), "
	"Praat will know that you want to show an OK button.")
NORMAL (L"The behaviour of the experiment changes appreciably if you include an OK button. "
	"If you do not include an OK button, Praat will present the next stimulus as soon as the participant "
	"has clicked a response category (and a goodness category, if there are such). "
	"If you do include an OK button, it will become visible to the participant as soon as she has chosen "
	"a response category (and a goodness category, if there are such). "
	"The participant can then click the OK button, but she can also choose to click the response "
	"(and goodness buttons) a bit more first.")
NORMAL (L"The OK button seems to be useful only if there is also a replay button, "
	"or if the response categories are sounds (see @@ExperimentMFC 6. Responses are sounds@).")
NORMAL (L"To assign a keyboard shortcut (here, the space bar) to the OK button, do something like")
CODE (L"okButton = 0.8 0.95 0.05 0.15 \"OK\" \" \"")
MAN_END

MAN_BEGIN (L"ExperimentMFC 4.3. The oops button", L"ppgb", 20051205)
INTRO (L"The @@ExperimentMFC 2.1. The experiment file|example experiment@ contained the following lines:")
CODE (L"oopsButton = 0 0 0 0 \"\" \"\"")
NORMAL (L"This means that that experiment did not have an oops button. "
	"To add an oops button in the lower left corner of the screen, you do something likw")
CODE (L"oopsButton = 0.05 0.2 0.05 0.15 \"oops\" \"\"")
NORMAL (L"If you supply a right edge (here 0.2) that is greater than the left edge (here 0.05), "
	"Praat will know that you want to show an oops button.")
NORMAL (L"If you include an oops button, it will become visible to the participant for every stimulus except the first, "
	"and it will also be visible on the pause (break) screens and on the final screen.")
NORMAL (L"If the participant clicks the oops button, Praat will forget everything the participant did "
	"with the current stimulus and the previous stimulus. The experiment will continue with playing "
	"the previous stimulus again and waiting for the participant's choice.")
MAN_END

MAN_BEGIN (L"ExperimentMFC 5. Stimulus-dependent texts", L"ppgb", 20051205)
INTRO (L"In the @@ExperimentMFC 2.1. The experiment file|example experiment@, the text at the top of the screen "
	"stayed the same throughout the experiment, and so did the texts on the response buttons. "
	"There are ways to have these texts depend on the stimulus at hand.")
LIST_ITEM (L"@@ExperimentMFC 5.1. The stimulus-dependent run text")
LIST_ITEM (L"@@ExperimentMFC 5.2. Stimulus-dependent response buttons")
MAN_END

MAN_BEGIN (L"ExperimentMFC 5.1. The stimulus-dependent run text", L"ppgb", 20051205)
INTRO (L"The @@ExperimentMFC 2.1. The experiment file|example experiment@ contained the following lines:")
CODE (L"numberOfDifferentStimuli = 4")
CODE1 (L"\"heed\"  \"\"")
CODE1 (L"\"hid\"   \"\"")
CODE1 (L"\"hood\"  \"\"")
CODE1 (L"\"hud\"   \"\"")
CODE (L"...")
CODE (L"...")
CODE (L"runText = \"Choose the vowel that you heard.\"")
NORMAL (L"For every stimulus, the same `run text' was written at the top of the screen. "
	"But suppose you want to make that text dependent on the stimulus. You would do:")
CODE1 (L"\"heed\"  \"Choose the vowel you heard.\"")
CODE1 (L"\"hid\"   \"Click the vowel you heard.\"")
CODE1 (L"\"hood\"  \"Select the vowel you heard.\"")
CODE1 (L"\"hud\"   \"What's the vowel you heard?\"")
CODE (L"...")
CODE (L"...")
CODE (L"runText = \"\"")
NORMAL (L"In this case, each stimulus comes with its own text. The %runText will only show up for stimuli "
	"for which you do not supply a separate text.")
NORMAL (L"This feature is useful mainly in cases where the responses are sounds but the stimulus is not "
	"(see @@ExperimentMFC 6. Responses are sounds@) or if you want to cause some lexical priming.")
MAN_END

MAN_BEGIN (L"ExperimentMFC 5.2. Stimulus-dependent response buttons", L"ppgb", 20070926)
INTRO (L"The @@ExperimentMFC 2.1. The experiment file|example experiment@ contained the following lines:")
CODE (L"numberOfDifferentStimuli = 4")
CODE1 (L"\"heed\"  \"\"")
CODE1 (L"\"hid\"   \"\"")
CODE1 (L"\"hood\"  \"\"")
CODE1 (L"\"hud\"   \"\"")
CODE (L"...")
CODE (L"...")
CODE (L"numberOfDifferentResponses = 5")
CODE1 (L"0.2 0.3 0.7 0.8 \"h I d\" 40 \"\" \"i\"")
CODE1 (L"0.3 0.4 0.5 0.6 \"h E d\" 40 \"\" \"e\"")
CODE1 (L"0.4 0.5 0.3 0.4 \"h A d\" 40 \"\" \"a\"")
CODE1 (L"0.5 0.6 0.5 0.6 \"h O d\" 40 \"\" \"o\"")
CODE1 (L"0.6 0.7 0.7 0.8 \"h U d\" 40 \"\" \"u\"")
NORMAL (L"For every stimulus, the buttons contained the same visible texts, such as \"h I d\" and \"h A d\".")
NORMAL (L"But suppose you have an experiment about the perception of voicing in plosives. "
	"The first stimulus starts with an ambiguous /b/ or /p/, and you want the participant "
	"to choose between \"bath\" and \"path\". The next stimulus starts with an ambiguous /d/ or /t/, "
	"and you want the participant to choose between \"dart\" and \"tart\". "
	"You would go about like this:")
CODE (L"numberOfDifferentStimuli = 6")
CODE1 (L"\"bpath1\"  \"|bath|path\"")
CODE1 (L"\"bpath2\"  \"|bath|path\"")
CODE1 (L"\"bpath3\"  \"|bath|path\"")
CODE1 (L"\"dtart1\"  \"|dart|tart\"")
CODE1 (L"\"dtart2\"  \"|dart|tart\"")
CODE1 (L"\"dtart3\"  \"|dart|tart\"")
CODE (L"...")
CODE (L"...")
CODE (L"numberOfDifferentResponses = 2")
CODE1 (L"0.2 0.4 0.7 0.8 \"\" 40 \"\" \"left\"")
CODE1 (L"0.6 0.8 0.7 0.8 \"\" 40 \"\" \"right\"")
NORMAL (L"In this case, the two response buttons show either \"path\" and \"path\", or \"dart\" and \"tart\".")
NORMAL (L"In the ResultsMFC (see @@ExperimentMFC 2.9. How an experiment proceeds@), "
	"the stimuli will be recorded as \"bpath1|bath|path\" and the like, not just as \"bpath1\". "
	"Praat does this in order to be able to cope with balanced designs such as")
CODE1 (L"\"bpath1\"  \"|bath|path\"")
CODE1 (L"\"bpath1\"  \"|path|bath\"")
NORMAL (L"In other words, the button ordering is considered part of the stimulus.")
NORMAL (L"This trick can be combined with a stimulus-dependent run text:")
CODE (L"numberOfDifferentStimuli = 32")
CODE1 (L"\"bpath1\"  \"Throw a...|bath|path\"")
CODE1 (L"\"bpath1\"  \"Walk a...|bath|path\"")
CODE1 (L"\"bpath2\"  \"Walk a...|bath|path\"")
CODE1 (L"\"dtart1\"  \"Throw a...|dart|tart\"")
CODE1 (L"\"dtart1\"  \"Carry a...|dart|tart\"")
CODE (L"...")
CODE (L"runText = \"\"")
CODE (L"...")
CODE (L"numberOfDifferentResponses = 2")
CODE1 (L"0.2 0.4 0.7 0.8 \"\" 40 \"\" \"left\"")
CODE1 (L"0.6 0.8 0.7 0.8 \"\" 40 \"\" \"right\"")
MAN_END

MAN_BEGIN (L"ExperimentMFC 6. Responses are sounds", L"ppgb", 20070926)
INTRO (L"In the @@ExperimentMFC 2.1. The experiment file|example experiment@, "
	"the stimuli were sounds, and the responses were categories whose labels appeared on buttons. "
	"Sometimes you want it the other way around.")
NORMAL (L"An example is the %%/i/ prototype% task: the top of the screen just says \"Please choose the best %ee\", "
	"and no stimulus sound is played. Instead, the participant can click repeatedly on an array of 40 buttons, "
	"each of which contains a different [i]-like sound. That is, if the participant clicks on a response button, "
	"an [i]-like sound is played, and every response button has its own sound.")
NORMAL (L"Such a task can be regarded as reversing the task of the example experiment, "
	"in which the stimulus was a sound and the reponse was a phonological category. "
	"In the /i/ prototype task, the stimulus is a phonological category, and the response is a sound.")
NORMAL (L"This is what the experiment file could look like:")
CODE (L"\"ooTextFile\"")
CODE (L"\"ExperimentMFC 5\"")
CODE (L"stimuliAreSounds? <no> \"\" \"\" \"\" \"\" 0 0")
CODE (L"numberOfDifferentStimuli = 2")
CODE1 (L"\"i\"  \"Choose the best \\% \\% ee\\% .\"")
CODE1 (L"\"I\"  \"Choose the best \\% \\% i\\% .\"")
CODE (L"numberOfReplicationsPerStimulus = 1")
CODE (L"breakAfterEvery = 1")
CODE (L"randomize = <CyclicNonRandom>")
CODE (L"startText = \"Click to start.\"")
CODE (L"runText = \"\"")
CODE (L"pauseText = \"You can have a short break if you like. Click to proceed.\"")
CODE (L"endText = \"The experiment has finished.\"")
CODE (L"maximumNumberOfReplays = 0")
CODE (L"replayButton = 0 0 0 0 \"\" \"\"")
CODE (L"okButton = 0.8 0.95 0.45 0.55 \"OK\" \"\"")
CODE (L"oopsButton = 0 0 0 0 \"\" \"\"")
CODE (L"responsesAreSounds? <yes>")
CODE (L"responseFileNameHead = \"Sounds/\"")
CODE (L"responseFileNameTail = \".wav\"")
CODE (L"responseCarrierBefore = \"\"")
CODE (L"responseCarrierAfter = \"\"")
CODE (L"responseInitialSilenceDuration = 0.3")
CODE (L"responseMedialSilenceDuration = 0")
CODE (L"numberOfDifferentResponses = 16")
CODE1 (L"0.2 0.3 0.7 0.8 \"\" 10 \"\" \"i11\"")
CODE1 (L"0.3 0.4 0.7 0.8 \"\" 10 \"\" \"i12\"")
CODE1 (L"0.4 0.5 0.7 0.8 \"\" 10 \"\" \"i13\"")
CODE1 (L"0.5 0.6 0.7 0.8 \"\" 10 \"\" \"i14\"")
CODE1 (L"0.2 0.3 0.6 0.7 \"\" 10 \"\" \"i21\"")
CODE1 (L"0.3 0.4 0.6 0.7 \"\" 10 \"\" \"i22\"")
CODE1 (L"0.4 0.5 0.6 0.7 \"\" 10 \"\" \"i23\"")
CODE1 (L"0.5 0.6 0.6 0.7 \"\" 10 \"\" \"i24\"")
CODE1 (L"0.2 0.3 0.5 0.6 \"\" 10 \"\" \"i31\"")
CODE1 (L"0.3 0.4 0.5 0.6 \"\" 10 \"\" \"i32\"")
CODE1 (L"0.4 0.5 0.5 0.6 \"\" 10 \"\" \"i33\"")
CODE1 (L"0.5 0.6 0.5 0.6 \"\" 10 \"\" \"i34\"")
CODE1 (L"0.2 0.3 0.4 0.5 \"\" 10 \"\" \"i41\"")
CODE1 (L"0.3 0.4 0.4 0.5 \"\" 10 \"\" \"i42\"")
CODE1 (L"0.4 0.5 0.4 0.5 \"\" 10 \"\" \"i43\"")
CODE1 (L"0.5 0.6 0.4 0.5 \"\" 10 \"\" \"i44\"")
CODE (L"numberOfGoodnessCategories = 5")
CODE1 (L"0.25 0.35 0.10 0.20 \"1 (poor)\"")
CODE1 (L"0.35 0.45 0.10 0.20 \"2\"")
CODE1 (L"0.45 0.55 0.10 0.20 \"3\"")
CODE1 (L"0.55 0.65 0.10 0.20 \"4\"")
CODE1 (L"0.65 0.75 0.10 0.20 \"5 (good)\"")
NORMAL (L"The participant will see 16 squares on the screen. First she will have to find the best /i/, "
	"then the best /\\ic/. The sound files ##Sounds/i11.wav# and so on must exist and have the same sampling frequency. "
	"A silence of 0.3 seconds is played just before each response sound.")
MAN_END

MAN_BEGIN (L"ExperimentMFC 7. Running multiple experiments", L"ppgb", 20100518)
INTRO (L"In all the earlier examples, either the set of stimulus sounds or the set of response sounds stayed "
	"the same throughout the experiment. If you want more than one set of stimuli, or more than one set of responses, "
	"you can run several experiments after each other, simply by selecting more than one experiment, then clicking #Run.")
NORMAL (L"You can put all these ExperimentMFC objects in one text file. The following example contains two experiments. "
	"The second line has to contain the text \"Collection\", followed by the number of experiments:")
CODE (L"\"ooTextFile\"")
CODE (L"\"Collection\" 2")
CODE (L"")
CODE (L"\"ExperimentMFC 5\" \"i\"")
CODE (L"stimuliAreSounds? <no> \"\" \"\" \"\" \"\" 0 0")
CODE (L"numberOfDifferentStimuli = 1")
CODE1 (L"\"i\"  \"Choose the best \\% \\% ee\\% .\"")
CODE (L"numberOfReplicationsPerStimulus = 1")
CODE (L"breakAfterEvery = 0")
CODE (L"randomize = <CyclicNonRandom>")
CODE (L"startText = \"You are going to choose the best \\% \\% ee\\% . Click to start.\"")
CODE (L"runText = \"\"")
CODE (L"pauseText = \"\"")
CODE (L"endText = \"Thank you for choosing the best \\% \\% ee\\% . Click to proceed.\"")
CODE (L"maximumNumberOfReplays = 0")
CODE (L"replayButton = 0 0 0 0 \"\" \"\"")
CODE (L"okButton = 0.8 0.95 0.45 0.55 \"OK\" \"\"")
CODE (L"oopsButton = 0 0 0 0 \"\" \"\"")
CODE (L"responsesAreSounds? <yes>")
CODE (L"responseFileNameHead = \"Sounds/\"")
CODE (L"responseFileNameTail = \".wav\"")
CODE (L"responseCarrierBefore = \"\"")
CODE (L"responseCarrierAfter = \"\"")
CODE (L"responseInitialSilenceDuration = 0.3")
CODE (L"responseMedialSilenceDuration = 0")
CODE (L"numberOfDifferentResponses = 6")
CODE1 (L"0.2 0.3 0.7 0.8 \"\" 10 \"\" \"i1\"")
CODE1 (L"0.3 0.4 0.7 0.8 \"\" 10 \"\" \"i2\"")
CODE1 (L"0.4 0.5 0.7 0.8 \"\" 10 \"\" \"i3\"")
CODE1 (L"0.5 0.6 0.7 0.8 \"\" 10 \"\" \"i4\"")
CODE1 (L"0.6 0.7 0.7 0.8 \"\" 10 \"\" \"i5\"")
CODE1 (L"0.7 0.8 0.7 0.8 \"\" 10 \"\" \"i6\"")
CODE (L"numberOfGoodnessCategories = 0")
CODE (L"")
CODE (L"\"ExperimentMFC 5\" \"u\"")
CODE (L"stimuliAreSounds? <no> \"\" \"\" \"\" \"\" 0 0")
CODE (L"numberOfDifferentStimuli = 1")
CODE1 (L"\"u\"  \"Choose the best \\% \\% oo\\% .\"")
CODE (L"numberOfReplicationsPerStimulus = 1")
CODE (L"breakAfterEvery = 0")
CODE (L"randomize = <CyclicNonRandom>")
CODE (L"startText = \"You are going to choose the best \\% \\% oo\\% . Click to start.\"")
CODE (L"runText = \"\"")
CODE (L"pauseText = \"\"")
CODE (L"endText = \"All the experiments have finished. You can call the experimenter.\"")
CODE (L"maximumNumberOfReplays = 0")
CODE (L"replayButton = 0 0 0 0 \"\" \"\"")
CODE (L"okButton = 0.8 0.95 0.45 0.55 \"OK\" \"\"")
CODE (L"oopsButton = 0 0 0 0 \"\" \"\"")
CODE (L"responsesAreSounds? <yes>")
CODE (L"responseFileNameHead = \"Sounds/\"")
CODE (L"responseFileNameTail = \".wav\"")
CODE (L"responseCarrierBefore = \"\"")
CODE (L"responseCarrierAfter = \"\"")
CODE (L"responseInitialSilenceDuration = 0.3")
CODE (L"responseMedialSilenceDuration = 0")
CODE (L"numberOfDifferentResponses = 6")
CODE1 (L"0.2 0.3 0.7 0.8 \"\" 10 \"\" \"u1\"")
CODE1 (L"0.3 0.4 0.7 0.8 \"\" 10 \"\" \"u2\"")
CODE1 (L"0.4 0.5 0.7 0.8 \"\" 10 \"\" \"u3\"")
CODE1 (L"0.5 0.6 0.7 0.8 \"\" 10 \"\" \"u4\"")
CODE1 (L"0.6 0.7 0.7 0.8 \"\" 10 \"\" \"u5\"")
CODE1 (L"0.7 0.8 0.7 0.8 \"\" 10 \"\" \"u6\"")
CODE (L"numberOfGoodnessCategories = 0")
NORMAL (L"In this example, the participant first has to choose the best /i/ from among six [i]-like sounds, "
	"which are in the sound files ##i1.wav# through ##i6.wav#. After that, she has to choose the best /u/ "
	"from among six [u]-like sounds, which are in the sound files ##u1.wav# through ##u6.wav#. "
	"The percent signs in \\% \\% ee\\%  mean that %ee will be italicized.")
NORMAL (L"If you read this file with ##Read from file...#, you will see two ExperimentMFC objects, "
	"named #i and #u. They both stand selected. You then click #Run, and after the participant finishes, "
	"you select both ExperimentMFC objects again (probably they still stand selected), and click ##Extract results#. "
	"You will then get two #ResultMFC objects.")
MAN_END
}

/* End of file manual_Exp.c */
