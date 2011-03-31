/* manual_BSS.c
 *
 * Copyright (C) 2010-2011 David Weenink
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
	djmw 20101227 Initial version
*/

#include "ManPagesM.h"

LINK_C void manual_BSS (ManPages me);
LINK_C void manual_BSS (ManPages me)
{
MAN_BEGIN (L"CrossCorrelationTable", L"djmw", 20110105)
INTRO (L"One of the types of objects in Praat. A CrossCorrelationTable represents the cross-correlations between "
	"a number of signals. Cell [%i,%j] of a CrossCorrelationTable contains the cross-correlation between the %i-th "
	"and the %j-th signal. For example, the CrossCorrelationTable of an %n-channel sound is a %n\\xx%n table where "
	"the number in cell [%i,%j] is the cross-correlation of channel %i with channel %j (for a particular lag time %\\ta).")
NORMAL (L"A CrossCorrelationTable has a square matrix whose cells contain the cross-correlations between "
	"the signals and a centroid vector with the average value of each signal.")
ENTRY (L"Remarks")
NORMAL (L"Sometimes in the statistical literature, the cross-correlation between signals is also called "
	"\"covariance\". However, the only thing a @@Covariance@ has in common with a CrossCorrelationTable is that "
	"both are symmetric matrices. The differences between a CrossCorrelationTable and a Covariance are:")
TAG (L"1. a Covariance matrix is always positive-definite; for a cross-correlation table this is only guaranteed if "
	"the lag time %\\ta = 0.")
TAG (L"2. The elements %%c__ij_% in a Covariance always satisfy |%%c__ij_%/\\Vr(%%c__ii_%\\.c%%c__jj_%)| \\<_ 1; this is "
	"generally not the case for cross-correlations.")
MAN_END

MAN_BEGIN (L"CrossCorrelationTables", L"djmw", 20101227)
INTRO (L"One of the types of objects in Praat. A CrossCorrelationTables represents a collection of @@CrossCorrelationTable@ objects.")
MAN_END

MAN_BEGIN (L"CrossCorrelationTables: Create test set...", L"djmw", 20110212)
INTRO (L"Create a collection of @@CrossCorrelationTable@s that are all derived from different diagonal matrices by the same transformation matrix.")
ENTRY (L"Settings")
SCRIPT (5.4, Manual_SETTINGS_WINDOW_HEIGHT (4), L""
	Manual_DRAW_SETTINGS_WINDOW ("CrossCorrelationTables: Create test set", 4)
	Manual_DRAW_SETTINGS_WINDOW_FIELD ("Matrix dimension", "5")
	Manual_DRAW_SETTINGS_WINDOW_FIELD ("Number of matrices", "20")
	Manual_DRAW_SETTINGS_WINDOW_BOOLEAN("First is positive-definite",1)
	Manual_DRAW_SETTINGS_WINDOW_FIELD ("Sigma", "0.02")
)
TAG (L"##Matrix dimension")
DEFINITION (L"determines the size of the square matrix with cross-correlations.")
TAG (L"##Number of matrices")
DEFINITION (L"determines the number of matrices that have to be generated.")
TAG (L"##First is positive-definite")
DEFINITION (L"guarantees that the first matrix of the series is positive definite.")
TAG (L"##Sigma")
DEFINITION (L"the standard deviation of the noise that is added to each transformation matrix element. A value "
	"of zero makes all the cross-correlation matrices jointly diagonalizable. A value greater than zero "
	"makes each transformation matrix a little different and the collection not jointly "
	"diagonalizable anymore.")
ENTRY (L"Algorithm")
NORMAL (L"All the CrossCorrelationTable matrices are generated as #V\\'p\\.c#D__%k_\\.c #V, where #D__%k_ is a diagonal matrix "
	"with entries randomly choosen from the [-1,1] interval. The matrix #V is a \"random\" orthogonal matrix "
	"obtained from the singular value decomposition of a matrix #M = #U\\.c#D\\.c#V\\'p, where the cells of the "
	"matrix #M are random Gaussian numbers with mean 0 and standard deviation 1.")
NORMAL (L"If the first matrix has to be positive definite, the numbers on the diagonal of #D__1_ are randomly "
	"chosen from the [0.1,1] interval.")
MAN_END

MAN_BEGIN (L"Sound: To CrossCorrelationTable...", L"djmw", 20110212)
INTRO (L"A command that creates a @@CrossCorrelationTable@ form every selected @@Sound@ object.")
ENTRY (L"Settings")
SCRIPT (5.4, Manual_SETTINGS_WINDOW_HEIGHT (2), L""
	Manual_DRAW_SETTINGS_WINDOW ("Sound: To CrossCorrelationTable", 2)
	Manual_DRAW_SETTINGS_WINDOW_RANGE("Time range", "0.0", "10.0")
	Manual_DRAW_SETTINGS_WINDOW_FIELD ("Lag time", "0.0")
)
TAG (L"##Time range (s)#,")
DEFINITION (L"determines the time range over which the table is calculated.")
TAG (L"##Lag time (s)#,")
DEFINITION (L"determines the lag time.")
ENTRY (L"Algorithm")
NORMAL (L"The cross-correlation between channel %i and channel %j for lag time \\ta is defined as the "
	"discretized #integral")
FORMULA (L"cross-corr (%c__%i_, %c__%j_) [%\\ta] \\=3 \\su__%t_ %c__%i_[%t] %c__%j_[%t+%\\ta] %%\\Det%,")
NORMAL (L"where %t and %t+%\\ta are discrete times and %%\\Det% is the @@sampling period@. ")
MAN_END

MAN_BEGIN (L"Sound: To Sound (blind source separation)...", L"djmw", 20110212)
INTRO (L"Analyze the selected multi-channel sound into its independent components by an iterative method.")
NORMAL (L"The method to find the independent components tries to simultaneously diagonalize a number of "
	"@@CrossCorrelationTable@s that are calculated from the multi-channel sound at different lag times.")
ENTRY (L"Settings")
SCRIPT (5.4, Manual_SETTINGS_WINDOW_HEIGHT (6), L""
	Manual_DRAW_SETTINGS_WINDOW ("Sound: To Sound (blind source separation)", 6)
	Manual_DRAW_SETTINGS_WINDOW_RANGE("Time range (s)", "0.0", "10.0")
	Manual_DRAW_SETTINGS_WINDOW_FIELD ("Number of cross-correlations", "20")
	Manual_DRAW_SETTINGS_WINDOW_FIELD ("Lag times", "0.002")
	Manual_DRAW_SETTINGS_WINDOW_FIELD ("Maximum number of iterations", "100")
	Manual_DRAW_SETTINGS_WINDOW_FIELD ("Tolerance", "0.001")
	Manual_DRAW_SETTINGS_WINDOW_OPTIONMENU("Diagonalization method", "ffdiag")
)
TAG (L"##Time range (s)")
DEFINITION (L"defines the time range over which the ##CrossCorrelationTable#s of the sound will be calculated.")
TAG (L"##Number of cross-correlations")
DEFINITION (L"defines the number of ##CrossCorrelationTable#s to be calculated.")
TAG (L"##Lag times")
DEFINITION (L"defines the lag time %\\ta__0_ for the ##CrossCorrelationTable#s. These tables "
	"are calculated at lag times %\\ta__k_=(%k - 1)%\\ta__0_, where %k runs from 1 to %%numberOfCrosscorrelations%.")
TAG (L"##Maximum number of iterations")
DEFINITION (L"defines a stopping criterion for the iteration. The iteration will stops when this number is reached.")
TAG (L"##Tolerance")
DEFINITION (L"defines another stopping criterion that depends on the method used.")
TAG (L"##Diagonalization method")
DEFINITION (L"defines the method to determine the independent components.")
ENTRY (L"Algorithm")
NORMAL (L"This method tries to decompose the sound according to the %%instantaneous% mixing model")
FORMULA (L"#Y=#A\\.c#X.")
NORMAL (L"In this model #Y is a matrix with the selected multi-channel sound, #A is a so-called "
	"%%mixing matrix% and #X is a matrix with the independent components. "
	"Essentially the model says that each channel in the multi-channel sound is a linear combination of the "
	"independent sound components in #X. "
	"If we would know the mixing matrix #A we could easily solve the model above for #X by standard means. "
	"However, if we don't know #A and we don't know #X, the decomposition of #Y is underdetermined as there "
	"are an infinite number of possibilities. ")
NORMAL (L"One approach to solve the equation above is to make assumptions about the statistical properties "
	"of the components in the matrix #X: it turns out that a sufficient assumption is to assume that the "
	"components in #X at each time instant are %%statistically independent%. This is not an unrealistic "
	"assumption in many cases, although in practice it need not be exactly the case. Another assumption is "
	"that the mixing matrix is constant." )
NORMAL (L"The theory says that statistically independent signals are not correlated (although the reverse "
	"is not always true: signals that are not correlated don't have to be statistically independent). "
	"The methods implemented here all follow this lead as follows. If we calculate the @@CrossCorrelationTable@ "
	"for the left and the right side signals of the equation above, then, "
	"for the multi-channel sound #Y this will result in a cross-correlation matrix #C. For the right side we "
	"obtain #A\\.c#D\\.c#A\\'p, where #D is a diagonal matrix because all the cross-correlations between "
	"different independent components are zero by definition. This results in the following identity: ")
FORMULA (L"#C(\\ta)=#A\\.c#D(\\ta)\\.c#A\\'p, for all values of the lag time \\ta.")
NORMAL (L"This equation says that, given the model, the cross-correlation matrix can be diagonalized for "
	"all values of the lag time %%by the same transformation matrix% #A.")
NORMAL (L"If we calculate the cross-correlation matrices for a number of different lag times, say 20, we "
	"then have to obtain the matrix #A that diagonalizes them all. Unfortunately there is no closed form solution "
	"that diagonalizes more than two matrices at the same time and we have to resort to iterative "
	"algorithms for joint diagonalization. ")
NORMAL (L"Two of these algorithms are the ##qdiag# method as described in @@Vollgraf & Obermayer (2006)@ "
	"and the ##ffdiag# method as described in @@Ziehe et al. (2004)@. ")
NORMAL (L"Unfortunately the convergence criteria of these two algorithms cannot easily be compared as "
	"the criterion for the ##ffdiag# algorithm is the relative change of the square root of the sum of the "
	"squared off-diagonal "
	"elements of the transformed cross-correlation matrices and the criterion for ##qdiag# is the largest "
	"change in the eigenvectors norm during an iteration.")
MAN_END

MAN_BEGIN (L"Blind source separation", L"djmw", 20110113)
INTRO (L"Blind source separation (BSS) is a technique for estimating individual source components from their mixtures "
	"at multiple sensors. It is called %blind because we don't use any other information besides the mixtures. ")
NORMAL (L"For example, imagine a room with a number of persons present and a number of microphones for recording. "
	"When one or more persons are speaking at the same time, each microphone registers a different %mixture of individual speaker's audio signals. It is the task of BSS to untangle these mixtures into their sources, i.e. the individual speaker's audio signals. "
	"In general, this is a difficult problem because of several complicating factors. ")
LIST_ITEM (L"\\bu Different locations of speakers and microphones in the room: the individual speaker's audio signals do not reach all microphones at the same time. ")
LIST_ITEM (L"\\bu Room acoustics: the signal that reaches a microphone is composed of the signal that %directly travels to the microphone and parts that come from room reverberations and echos. ")
LIST_ITEM (L"\\bu Varying distances to microphones: one ore more speakers might be moving. This makes the mixing time dependent.")
NORMAL (L"If the number of sources is %larger than the number of sensors we speak of an %overdetermined problem. If the number of sensors and the number of sources are %equal we speak of a %determined problem. The more difficult problem is the %underdetermined one where the number of sensors is %less than the number of sources. ")
ENTRY (L"Typology of mixtures")
NORMAL (L"In general two different types of mixtures are considered in the literature: %%instantaneous "
	"mixtures% and %%convolutive mixtures%. ")
TAG (L"%%Instantaneous mixtures%")
DEFINITION (L"where the mixing is instantaneous, corresponds to the model #Y=#A\\.c#X. In this model #Y is a matrix with the recorded microphone sounds, #A is a so-called "
	"%%mixing matrix% and #X is a matrix with the independent source signals. "
	"Essentially the model says that the signal that each microphone records is a (possibly different) linear combination of the %same source signals.  "
	"If we would know the mixing matrix #A we could easily solve the model above for #X by standard means. "
	"However, in general we don't know #A and #X and there are an infinite number of possible decompositions for #Y. The problem is however solvable by making some (mild) assumptions about #A and #X. ")
TAG (L"%%Convolutive mixtures%")
DEFINITION (L"are mixtures where the mixing is of convolutive nature, i.e. the model is ")
FORMULA (L"%%y__i_ (n)% = \\Si__%j_^^%d^\\Si__%\\ta_^^M__%ij_-1^ %%h__ij_(\\ta)x__j_(n-\\ta) + N__i_(n)%, for %i=1..m.")
DEFINITION (L"Here %%y__i_ (n) is the %n-th sample of the %i-th microphone signal, %m is the number of microphones, %%h__ij_(\\ta)% is the multi-input multi-output linear filter with the source-microphone impulse responses that characterize the propagation of the sound in the room and %%N__i_% is a noise source. This model is typically much harder to solve than the previous one because of the %%h__ij_(\\ta)% filter term that can have thousands of coefficients. For example, the typical @@reverberation time@ of a room is approximately 0.3 s which corresponds to 2400 samples, i.e. filter coefficients, for an 8 kHz sampled sound.")
ENTRY (L"Solving the blind source separation for instantaneous mixtures")
NORMAL (L"Various techniques exist for solving the blind source separation problem for %instantaneous mixtures. Very popular ones make make use of second order statistics (SOS) by trying to "
	"simultaneously diagonalize a large number of cross-correlation matrices. Other techniques like independent component analysis use higher order statistics (HOS) to find the independent components, i.e. the sources.")
NORMAL (L"Given the decomposition problem #Y=#A\\.c#X, we can see that the solution is determined "
	"only upto a permutation and a scaling of the components. This is called the %%indeterminancy "
	"problem% of BSS. This can be seen as follows: given a permutation matrix #P, i.e. a matrix which "
	"contains only zeros except for one 1 in every row and column, and a diagonal scaling matrix #D, any "
	"scaling and permutation of the independent components #X__%n_=(#D\\.c#P)\\.c#X can be compensated "
	"by the reversed scaling of the mixing matrix #A__%n_=#A\\.c(#D\\.c#P)^^-1^ because #A\\.c(#D\\.c#P)^^-1^\\.c(#D\\.c#P)\\.c#X = #A\\.c#X = #Y. ")
ENTRY (L"Solving the blind source separation for convolutive mixtures")
NORMAL (L"Solutions for %convolutive mixture problems are much harder to achieve. "
	"One normally starts by transforming the problem to the frequency domain where the "
	"convolution is turned into a multiplication. The problem then translates into a separate "
	"%%instantaneous% mixing problem for %%each% frequency in the frequency domain. It is here that "
	"the indeterminacy problem hits us because it is not clear beforehand how to combine the "
	"independent components of each frequency bin.")
MAN_END

MAN_BEGIN (L"reverberation time", L"djmw", 20110107)
NORMAL (L"Reverberation is the persistence of sound in a room after the sound source has silenced. ")
NORMAL (L"The %%reverberation time% is normally defined as the time required for the persistence of a direct sound to decay by 60 dB after the direct sound has silenced. Sometimes this dB level is indicated with a subscript and the reverberation time is given by the symbol %T__60_. "
	"The reverberation time depends mainly on a room's volume and area and on the absorption at the walls. Generally absorption is frequency dependent and therefore the reverberation time of a room varies with frequency. ")
MAN_END

MAN_BEGIN (L"Vollgraf & Obermayer (2006)", L"djmw", 20110105)
NORMAL (L"Roland Vollgraf & Klaus Obermayer (2006): \"Quadratic optimization for simultaneous matrix "
	"diagonalization.\" %%IEEE Transactions On Signal Processing% #54: 3270\\--3278.")
MAN_END

MAN_BEGIN (L"Ziehe et al. (2004)", L"djmw", 20110105)
NORMAL (L"Andreas Ziehe, Pavel Laskov, Guido Nolte & Klaus-Robert M\\u\"ller (2004): \"A fast algorithm for joint "
	"diagonalization with non-orthogonal transformations and its application to blind source separation\", "
	"%%Journal of Machine Learning Research% #5: 777\\--800.")
MAN_END

}

/* End of file manual_BSS.c */

