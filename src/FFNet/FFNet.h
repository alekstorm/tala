#ifndef _FFNet_h_
#define _FFNet_h_
/* FFNet.h
 *
 * Copyright (C) 1997-2011 David Weenink
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
 djmw 19970515
 djmw 20020712 GPL header.
 djmw 20031025 Added FFNet_getMinimum.
 djmw 20040420 FFNet_create and Pattern&Categories: To FFNet changes.
 djmw 20040422 FFNet_extractWeights added.
 djmw 20040505 FFNet_getNodeNumberFromUnitNumber added.
 djmw 20071024 Latest modification.
 djmw 20080121 float -> double
*/

#ifndef _Data_h_
	#include "sys/Data.h"
#endif
#ifndef _Categories_h_
	#include "dwtools/Categories.h"
#endif
#ifndef _Minimizers_h_
	#include "dwtools/Minimizers.h"
#endif
#ifndef _TableOfReal_h_
	#include "stat/TableOfReal.h"
#endif

#ifdef __cplusplus
	extern "C" {
#endif

#include "FFNet_def.h"
#define FFNet_methods Data_methods
oo_CLASS_CREATE (FFNet, Data);

/* Parameters:
 *  long nLayers	    : the #layers in the net (exclusive the inputs)
 *    nUnitsInLayer	    : array[0..nLayers] the #units in each layer
 *						  nUnitsInLayer[0] : #inputs
 *						  nUnitsInLayer[nLayers] :#outputs
 *    nWeights		    : the total #weights in the net (inclusive bias)
 *  double *w		    : array[1..nWeights] with the connection strengths
 *    *activity		    : array[1..nNodes] with activities
 *  outputLabels	    : labels belonging to the outputs
 *  BOOKKEEPING:
 *  long nNodes		    : total #nodes: bias modelled as unit with constant activity)
 *    *isbias		    : array[1..nNodes] set 1 if node is bias else 0
 *    *nodeFirst	    : array[1..nNodes] first node connected to this unit
 *    *nodeLast:	    : array[1..nNodes] last node connected to this unit
 *    *wFirst		    : array[1..nNodes] first index in *w for this unit
 *    *wLast		    : array[1..nNodes] last (inclusive the bias)
 *  LEARNING:
 *  int *wSelected		: array[1..nWeights] weights selected for minimization
 *  double *deriv	    : array[1..nNodes] derivative of nonlinearity at node
 *    *error		    : array[1..nNodes] the error at node
 *    *dw		    	: array[1..nWeights] total derivative for weights
 *    *dwi		   		: array[1..nWeights] derivative per pattern
 * long dimension		: dimension of minimizer space (<= my nWeights)
 * long nPatterns	    : the #patterns to be learned
 * double **inputPattern: matrix[1..nPatterns][1..nInputs]
 * double **targetActivation: matrix[1..nPatterns][1..nOutputs]
 * double accumulatedCost : accumulated costs of testing/training with patterns
 *
 * A network consists of nLayers layers. Layer numbering is from 0...nLayers.
 * Layer 0 is the input layer,  the highest numbered layer is the output layer
 *   (nLayers <= 4) 
 * Each layer consists of a number of units. The biases of all the units in a layer 
 * are modelled with connections to an extra unit in the lower layer (with constant 
 * activity 1.0). Nodes refers to 'units' + 'bias units'.
 * The variable 'nNodes' is the total number of nodes (inclusive bias nodes). 
 * E.g. the topology (2,3,4), i.e., 2 inputs, 3 units in the first layer
 * and 4 units in the second layer (outputs) is modelled
 * with (2+1)+ (3+1)+ (4) = 11 nodes. 
 * The numbering of the weights is as follows (indices 1..nWeights):
 * E.g., topology (I,H,O) (I inputs, H hidden units and O output units)
 * There are a total of H* (I+1) + O* (H+1) weights in this net.
 * w[1] - w[I]                        : I (1)->H (1), I (2)->H (1) ... I (I)->H (1)
 *   w[I+1]                           :   bias->H (1)
 * w[ (I+1)+1] - w[2 (I+1)-1]           : I (1)->H (2), I (2)->H (2) ... I (I)->H (2)
 *   w[2 (I+1)]                        :   bias->H (2)
 *   ...
 * w[ (H-1) (I+1)+1] - w[H (I+1)-1]      : I (1)->H (H), I (2)->H (H) ... I (I)->H (H)
 *   w[H (I+1)]                        :   bias->H (H)
 *      M = H (I+1)
 * w[M+1] - w[M+H]                    : H (1)->O (1), H (2)->O (1) ... H (H)->O (1)
 *   w[M+H+1]                         :   bias->O (1)
 * w[M+ (H+1)+1] - w (M+2 (H+1)-1)       : H (1)->O (2), H (2)->O (2) ... H (H)->O (2)
 *   w[M+2 (H+1)]                      :   bias->O (1)
 *   ...
 * w[M+ (O-1)* (H+1)+1] - w[M+O (H+1)-1] : H (1)->O (O), H (2)->O (O) ... H (H)->O (O)
 *   w[m+o (h+1)-1]                    :   bias->O (O)
 *
 * Internals:
 *
 * A number of auxiliary arrays for efficient calculations have been setup.
 * For a node k we need to know:
 * 1. isbias[1..nNodes]        : usage: if (isbias[k]) ...
 *      true if node k is a bias node. There are nLayers bias nodes
 * 2. nodeFirst[1..nNodes]       : usage is j=nodeFirst[k]; 
 *      j is the first node that is connected to k .
 * 3. nodeLast[1..nNodes]        : usage is j=nodeLast[k] 
 *      j is the  last node that is connected to k (bias included).
 * For the calculation of the errors,  during learning,  in unit k we need to
 * know which weights from the preceeding layer connect to it.
 * 4. wFirst[1..nNodes] : usage j=wFirst[k]
 *      w[j] is first weight to node k.
 * 5. wLast[1..nNodes]  : usage j=wLast[k]
 *      w[j] is last weight to node k.
 */

/* FFNet::copy
 *  copy everything except minimizer, patterns and inputs.
 */

int FFNet_init (FFNet me, long numberOfInputs, long nodesInLayer1, long nodesInLayer2, 
	long numberOfOutputs, int outputsAreLinear	);

FFNet FFNet_create (long numberOfInputs, long numberInLayer1, long numberInLayer2, 
	long numberOfOutputs, int outputsAreLinear	);
	
void FFNet_createNameFromTopology (FFNet me, MelderString *name);	
/* Create names as <inputs>-<outputs>, <inputs>-<hidden>-<outputs>, 
	<inputs>-<hidden1>-<hidden2>-<outputs> for 1, 2 or 3 layer networks.
*/

#define FFNet_COST_MSE 1
    
void FFNet_setCostFunction (FFNet me, int type);

#define FFNet_NONLIN_SIGMOID 1

void FFNet_setNonLinearity (FFNet me, int type);

int FFNet_setOutputCategories (FFNet me, Categories thee);

void FFNet_reset (FFNet me, double wrange);
/* reset the neural net:
 *   initialize all (selected) weights and biases with random numbers from the
 *   interval (-wrange, wrange).
 *   forget links with minimizer.
 */
 
void FFNet_propagateToLayer (FFNet me, const double input[], double activity[], long layer);
/* propagate the input through the net to layer and calculate the activities */ 

void FFNet_propagate (FFNet me, const double input[], double output[]);
/* step (1) feed forward input from "input layer" to "output layer"
 * if output != NULL the output activity is copied into output.
 * postcondition: my activities defined
 */

double FFNet_computeError (FFNet me, const double target[]);
/* step (2) calculate error on output nodes w.r.t. desired output */
/* step (3) backpropagate this error to previous nodes */
/* precondition: step (1) */

void FFNet_computeDerivative (FFNet me);
/* step (4) compute derivative in my dwi */
/* Precondition: step (3) */

long FFNet_getWinningUnit (FFNet me, int labeling);
/* labeling = 1 : winner-takes-all */
/* labeling = 2 : stochastic */

void FFNet_selectAllWeights (FFNet me);
void FFNet_selectBiasesInLayer (FFNet me, long layer);

long FFNet_dimensionOfSearchSpace (FFNet me);
/* count the selected weights */

long FFNet_getNumberOfWeights (FFNet me);
/* return my nWeights */

void FFNet_weightConnectsUnits (FFNet me, long index, long *fromUnit, long *toUnit, long *layer);
/*
 * w[index] connects unit fromUnit in "layer-1" with unit toUnit in "layer".
 *  fromUnit returns 0 then w[index] is bias.
 */
 
long FFNet_getNodeNumberFromUnitNumber (FFNet me, long unit, long layer);
 
void FFNet_nodeToUnitInLayer (FFNet me, long node, long *unit, long *layer);
/* translate node index to unit "unit" in layer "layer" */

long FFNet_getNumberOfLayers (FFNet me);

long FFNet_getNumberOfUnits (FFNet me);

long FFNet_getNumberOfHiddenLayers (FFNet me);

long FFNet_getNumberOfUnitsInLayer (FFNet me, int layer);

double FFNet_getMinimum (FFNet me);

Collection FFNet_createIrisExample (long numberOfHidden1, long numberOfHidden2);

TableOfReal FFNet_extractWeights (FFNet me, long layer);
FFNet FFNet_and_TabelOfReal_to_FFNet (FFNet me, TableOfReal him, long layer);

#ifdef __cplusplus
	}
#endif

#endif /* _FFNet_h_ */
