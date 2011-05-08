/* KNN.c
 *
 * Copyright (C) 2008 Ola So"der, 2010 Paul Boersma
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
 * os 20080529 initial release
 * pb 20100606 removed some array-creations-on-the-stack
 */

#include "KNN.h"
#include "KNN_threads.h"
#include "OlaP.h"

#include "sys/oo/oo_DESTROY.h"
#include "KNN_def.h"
#include "sys/oo/oo_COPY.h"
#include "KNN_def.h"
#include "sys/oo/oo_EQUAL.h"
#include "KNN_def.h"
#include "sys/oo/oo_WRITE_TEXT.h"
#include "KNN_def.h"
#include "sys/oo/oo_WRITE_BINARY.h"
#include "KNN_def.h"
#include "sys/oo/oo_READ_TEXT.h"
#include "KNN_def.h"
#include "sys/oo/oo_READ_BINARY.h"
#include "KNN_def.h"
#include "sys/oo/oo_DESCRIPTION.h"
#include "KNN_def.h"


/////////////////////////////////////////////////////////////////////////////////////////////
// Praat specifics                                                                         //
/////////////////////////////////////////////////////////////////////////////////////////////

static void info (I)
{
    iam (KNN);
    classData -> info (me);
    MelderInfo_writeLine2 (L"Size of instancebase: ", Melder_integer (my nInstances));
}

class_methods (KNN, Data)
class_method_local (KNN, destroy)
class_method_local (KNN, copy)
class_method_local (KNN, equal)
class_method_local (KNN, writeText)
class_method_local (KNN, writeBinary)
class_method_local (KNN, readText)
class_method_local (KNN, readBinary)
class_method_local (KNN, description)
class_method (info)
class_methods_end

/////////////////////////////////////////////////////////////////////////////////////////////
// Creation                                                                                //
/////////////////////////////////////////////////////////////////////////////////////////////

KNN KNN_create ()   
{
    KNN me = Thing_new (KNN);
    if (!me)
    {
        return(NULL);
    }
    else
    {
        my nInstances = 0;
        return(me);
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Learning                                                                                //
/////////////////////////////////////////////////////////////////////////////////////////////

int KNN_learn
(
    ///////////////////////////////
    // Parameters                //
    ///////////////////////////////

    KNN me,         // the classifier to be trained
                    //
    Pattern p,      // source pattern
                    //
    Categories c,   // target categories
                    //
    int method,     // method <- REPLACE or APPEND
                    //
    int ordering    // ordering <- SHUFFLE?
)

{
    if (c->size == p->ny)           // the number of input vectors must
    {                               // equal the number of categories
        switch (method)
        {
        case kOla_REPLACE:          // in REPLACE mode simply
                                    // dispose of the current
            if (my nInstances > 0)  // instance base and store
            {                       // the new one
                forget(my input);
                forget(my output);
            }

            my input = Data_copy(p);
            my output = Data_copy(c);
            my nInstances = c->size;

            break;

        case kOla_APPEND:                   // in APPEND mode a new
                                            // instance base is formed
                                            // by merging the new and
                                            // the old
                                            //
            if (p->nx == (my input)->nx)    // the number of features
                                            // of the old and new
                                            // instance base must
                                            // match otherwise merging
                                            // wont be possible
            {

                Matrix tinput = Matrix_appendRows(my input, p);
                Categories toutput = Collections_merge(my output, c);

                forget(my input);
                forget(my output);

                my input = (Pattern) tinput;
                my output = toutput;
                my nInstances += p->ny;
            }
            else                                    // fail
            {
                return(kOla_DIMENSIONALITY_MISMATCH);
            }
            break;
        }
        if (ordering == kOla_SHUFFLE)               // shuffle the instance base
        {
            KNN_shuffleInstances(me);
        }
    }
    else                                            // fail
    {
        return(kOla_PATTERN_CATEGORIES_MISMATCH);
    }

    return(kOla_SUCCESS);                           // success

}

/////////////////////////////////////////////////////////////////////////////////////////////
// Classification - To Categories                                                          //
/////////////////////////////////////////////////////////////////////////////////////////////

typedef struct
{
        KNN me;
        Pattern ps;
        long * output;
        FeatureWeights fws;
        long k;
        int dist;
        long istart;
        long istop;

} KNN_input_ToCategories_t;

Categories KNN_classifyToCategories
(
    ///////////////////////////////
    // Parameters                //
    ///////////////////////////////

    KNN me,             // the classifier being used
                        //
    Pattern ps,         // the pattern to classify
                        //
    FeatureWeights fws, // feature weights
                        //
    long k,             // the number of sought after neighbours
                        //
    int dist            // distance weighting
)

{
    int nthreads = KNN_getNumberOfCPUs();
    long *outputindices = NUMlvector (0, ps->ny);
    long chunksize =  ps->ny / nthreads;

    Melder_assert(nthreads > 0); 
    Melder_assert(k > 0 && k <= my nInstances);

    if(chunksize < 1)
    {
        chunksize = 1;
        nthreads = ps->ny;
    }

    long istart = 1;
    long istop = chunksize;

    Categories output = Categories_create();
    KNN_input_ToCategories_t ** input = (KNN_input_ToCategories_t **) malloc(nthreads * sizeof(KNN_input_ToCategories_t *));

    if(!input)
        return(NULL);

    for(int i = 0; i < nthreads; ++i)
    {
        input[i] = (KNN_input_ToCategories_t *) malloc(sizeof(KNN_input_ToCategories_t));
        if(!input[i])
        {
            while(input[i--])
                free(input[i]);

            free(input);
            return(NULL);
        }
    }

    for(int i = 0; i < nthreads; ++i)
    {  
        input[i]->me = me;
        input[i]->ps = ps;
        input[i]->output = outputindices;
        input[i]->fws = fws;
        input[i]->k = k;
        input[i]->dist = dist;
        input[i]->istart = istart;

        if(istop + chunksize > ps->ny)
        {  
            input[i]->istop = ps->ny; 
            break;
        }
        else
        {  
            input[i]->istop = istop;
            istart = istop + 1;
            istop += chunksize;
        }
    }
 
    enum KNN_thread_status * error = (enum KNN_thread_status *) KNN_threadDistribution(KNN_classifyToCategoriesAux, (void **) input, nthreads);
    //void *error = KNN_classifyToCategoriesAux (input [0]);
    for(int i = 0; i < nthreads; ++i)
        free(input[i]);
  
    free(input);
    if(error)           // Something went very wrong, you ought to inform the user!
    {
        free(error);
        return(NULL);
    }

    if(output)
    {
        Categories_init(output, 0);
        for (long i = 1; i <= ps->ny; ++i)
            Collection_addItem(output, Data_copy((my output)->item[outputindices[i]]));
    }
	NUMlvector_free (outputindices, 0);
    return(output);
}

void * KNN_classifyToCategoriesAux
(
    ///////////////////////////////
    // Parameters                //
    ///////////////////////////////

    void * input
   
)

{
    Melder_assert(((KNN_input_ToCategories_t *) input)->istart > 0 &&  
                  ((KNN_input_ToCategories_t *) input)->istop > 0 &&
                  ((KNN_input_ToCategories_t *) input)->istart <= ((KNN_input_ToCategories_t *) input)->ps->ny &&
                  ((KNN_input_ToCategories_t *) input)->istop <= ((KNN_input_ToCategories_t *) input)->ps->ny &&
                  ((KNN_input_ToCategories_t *) input)->istart <= ((KNN_input_ToCategories_t *) input)->istop);
    
    long ncollected;
    long ncategories;

    long *indices = NUMlvector (0, ((KNN_input_ToCategories_t *) input)->k);
    long *freqindices = NUMlvector (0, ((KNN_input_ToCategories_t *) input)->k);

    double *distances = NUMdvector (0, ((KNN_input_ToCategories_t *) input)->k);
    double *freqs = NUMdvector (0, ((KNN_input_ToCategories_t *) input)->k);
 
    for (long y = ((KNN_input_ToCategories_t *) input)->istart; y <= ((KNN_input_ToCategories_t *) input)->istop; ++y)
    {
        /////////////////////////////////////////
        // Localizing the k nearest neighbours //
        /////////////////////////////////////////

        ncollected = KNN_kNeighbours
        (
            ((KNN_input_ToCategories_t *) input)->ps, 
            ((KNN_input_ToCategories_t *) input)->me->input, 
            ((KNN_input_ToCategories_t *) input)->fws, y, 
            ((KNN_input_ToCategories_t *) input)->k, indices, distances
        );

        /////////////////////////////////////////////////
        // Computing frequencies and average distances //
        /////////////////////////////////////////////////

        ncategories = KNN_kIndicesToFrequenciesAndDistances
        (
            ((KNN_input_ToCategories_t *) input)->me->output, 
            ((KNN_input_ToCategories_t *) input)->k, 
            indices, distances, freqs, freqindices
        );

        ////////////////////////
        // Distance weighting //
        ////////////////////////

        switch(((KNN_input_ToCategories_t *) input)->dist)
        {
            case kOla_DISTANCE_WEIGHTED_VOTING:
                for (long c = 0; c < ncategories; ++c)
                    freqs[c] *= 1 / OlaMAX(distances[c], kOla_MINFLOAT);
                break;

            case kOla_SQUARED_DISTANCE_WEIGHTED_VOTING:
                for (long c = 0; c < ncategories; ++c)
                    freqs[c] *= 1 / OlaMAX(OlaSQUARE(distances[c]), kOla_MINFLOAT);
        }

        KNN_normalizeFloatArray(freqs, ncategories);
        ((KNN_input_ToCategories_t *) input)->output[y] = freqindices[KNN_max(freqs, ncategories)];
    }

	NUMlvector_free (indices, 0);
	NUMlvector_free (freqindices, 0);
	NUMdvector_free (distances, 0);
	NUMdvector_free (freqs, 0);
    return(NULL);

}

////////////////////////////////////////////////////////////////////////////////////////////
// Classification - To TableOfReal                                                         //
/////////////////////////////////////////////////////////////////////////////////////////////

typedef struct
{
        KNN me;
        Pattern ps;
        Categories uniqueCategories;
        TableOfReal output;
        FeatureWeights fws;
        long k;
        int dist;
        long istart;
        long istop;

} KNN_input_ToTableOfReal_t;

TableOfReal KNN_classifyToTableOfReal
(
    ///////////////////////////////
    // Parameters                //
    ///////////////////////////////

    KNN me,             // the classifier being used
                        //
    Pattern ps,         // source Pattern
                        //
    FeatureWeights fws, // feature weights
                        //
    long k,             // the number of sought after neighbours
                        //
    int dist            // distance weighting
)

{
    int nthreads = KNN_getNumberOfCPUs();
    long chunksize =  ps->ny / nthreads;
    Categories uniqueCategories = Categories_selectUniqueItems(my output, 1);
    long ncategories = Categories_getSize(uniqueCategories);
   
    Melder_assert(nthreads > 0);
    Melder_assert(ncategories > 0);
    Melder_assert(k > 0 && k <= my nInstances);
 
    if(!ncategories)
        return(NULL);

    if(chunksize < 1)
    {
        chunksize = 1;
        nthreads = ps->ny;
    }

    long istart = 1;
    long istop = chunksize;

    KNN_input_ToTableOfReal_t ** input = (KNN_input_ToTableOfReal_t **) malloc(nthreads * sizeof(KNN_input_ToTableOfReal_t *));
    
    if(!input)
        return(NULL);

    TableOfReal output = TableOfReal_create(ps->ny, ncategories);

    for (long i = 1; i <= ncategories; ++i)
        TableOfReal_setColumnLabel(output, i,  SimpleString_c(uniqueCategories->item[i]));

    for(int i = 0; i < nthreads; ++i)
    {
        input[i] = (KNN_input_ToTableOfReal_t *) malloc(sizeof(KNN_input_ToTableOfReal_t));
        if(!input[i])
        {
            while(input[i--])
                free(input[i]);

            free(input);
            return(NULL);
        }
    }

    for(int i = 0; i < nthreads; ++i)
    {  
        input[i]->me = me;
        input[i]->ps = ps;
        input[i]->output = output;
        input[i]->uniqueCategories = uniqueCategories;
        input[i]->fws = fws;
        input[i]->k = k;
        input[i]->dist = dist;
        input[i]->istart = istart;

        if(istop + chunksize > ps->ny)
        {  
            input[i]->istop = ps->ny; 
            break;
        }
        else
        {  
            input[i]->istop = istop;
            istart = istop + 1;
            istop += chunksize;
        }
    }
 
    enum KNN_thread_status * error = (enum KNN_thread_status *) KNN_threadDistribution(KNN_classifyToTableOfRealAux, (void **) input, nthreads);
    for(int i = 0; i < nthreads; ++i)
        free(input[i]);
  
    free(input);
    if(error)           // Something went very wrong, you ought to inform the user!
    {
        free(error);
        return(NULL);
    }
    return(output);
}


void * KNN_classifyToTableOfRealAux
(
    ///////////////////////////////
    // Parameters                //
    ///////////////////////////////

    void * input

)

{
    long ncategories = Categories_getSize (((KNN_input_ToTableOfReal_t *) input)->uniqueCategories);
    long *indices = NUMlvector (0, ((KNN_input_ToTableOfReal_t *) input)->k);
    double *distances = NUMdvector (0, ((KNN_input_ToTableOfReal_t *) input)->k);

    for (long y = ((KNN_input_ToTableOfReal_t *) input)->istart; y <= ((KNN_input_ToTableOfReal_t *) input)->istop; ++y)
    {
        KNN_kNeighbours(((KNN_input_ToTableOfReal_t *) input)->ps, 
                        ((KNN_input_ToTableOfReal_t *) input)->me->input, 
                        ((KNN_input_ToTableOfReal_t *) input)->fws, y, 
                        ((KNN_input_ToTableOfReal_t *) input)->k, indices, distances);

        for(long i = 0; i < ((KNN_input_ToTableOfReal_t *) input)->k; ++i)
        {
            for(long j = 1; j <= ncategories; ++j)
                if(FRIENDS(((KNN_input_ToTableOfReal_t *) input)->me->output->item[indices[i]], ((KNN_input_ToTableOfReal_t *) input)->uniqueCategories->item[j])) 
                    ++((KNN_input_ToTableOfReal_t *) input)->output->data[y][j];
        }
    }
 
    switch (((KNN_input_ToTableOfReal_t *) input)->dist)
    {
        case kOla_DISTANCE_WEIGHTED_VOTING:
            for (long y = ((KNN_input_ToTableOfReal_t *) input)->istart; y <= ((KNN_input_ToTableOfReal_t *) input)->istop; ++y)
            {
                double sum = 0;
                for(long c = 1; c <= ncategories; ++c)
                {
                    ((KNN_input_ToTableOfReal_t *) input)->output->data[y][c] *= 1 / OlaMAX(distances[c], kOla_MINFLOAT);
                    sum += ((KNN_input_ToTableOfReal_t *) input)->output->data[y][c];
                } 
                
                for(long c = 1; c <= ncategories; ++c)
                    ((KNN_input_ToTableOfReal_t *) input)->output->data[y][c] /= sum;

            }
            break;

        case kOla_SQUARED_DISTANCE_WEIGHTED_VOTING:
            for (long y = ((KNN_input_ToTableOfReal_t *) input)->istart; y <= ((KNN_input_ToTableOfReal_t *) input)->istop; ++y)
            {
                double sum = 0;
                for(long c = 1; c <= ncategories; ++c)
                {
                    ((KNN_input_ToTableOfReal_t *) input)->output->data[y][c] *= 1 / OlaMAX(OlaSQUARE(distances[c]), kOla_MINFLOAT);
                    sum += ((KNN_input_ToTableOfReal_t *) input)->output->data[y][c];
                } 

                for(long c = 1; c <= ncategories; ++c)
                    ((KNN_input_ToTableOfReal_t *) input)->output->data[y][c] /= sum;
            }
            break;

        case kOla_FLAT_VOTING: 
            for (long y = ((KNN_input_ToTableOfReal_t *) input)->istart; y <= ((KNN_input_ToTableOfReal_t *) input)->istop; ++y)
            {
                double sum = 0;
                for(long c = 1; c <= ncategories; ++c)
                    sum += ((KNN_input_ToTableOfReal_t *) input)->output->data[y][c];

                for(long c = 1; c <= ncategories; ++c)
                    ((KNN_input_ToTableOfReal_t *) input)->output->data[y][c] /= sum;
            }

    }
	NUMlvector_free (indices, 0);
	NUMdvector_free (distances, 0);
    return(NULL);
}

//////////////////////////////////////////////////////////////////////////////////////////////
// Classification - Folding                                                                //
/////////////////////////////////////////////////////////////////////////////////////////////

Categories KNN_classifyFold
(
    ///////////////////////////////
    // Parameters                //
    ///////////////////////////////

    KNN me,             // the classifier being used
                        //
    Pattern ps,         // source Pattern
                        //
    FeatureWeights fws, // feature weights
                        //
    long k,             // the number of sought after neighbours
                        //
    int dist,           // distance weighting
                        //
    long begin,         // fold start, inclusive [...
                        //
    long end            // fold end, inclusive ...]
                        //
)

{
    Melder_assert(k > 0 && k <= ps->ny);
    Melder_assert(end > 0 && end <= ps->ny);
    Melder_assert(begin > 0 && begin <= ps->ny);

    if (begin > end) 
        OlaSWAP(long, begin, end);         

    if (k > my nInstances - (end - begin))
        k = my nInstances - (end - begin);

    long ncollected;
    long ncategories;
    long *indices = NUMlvector (0, k);
    long *freqindices = NUMlvector (0, k);
    double *distances = NUMdvector (0, k);
    double *freqs = NUMdvector (0, k);
    long *outputindices = NUMlvector (0, ps->ny);
    long noutputindices = 0;

    for (long y = begin; y <= end; ++y)
    {
        /////////////////////////////////////////
        // Localizing the k nearest neighbours //
        /////////////////////////////////////////

        ncollected = KNN_kNeighboursSkipRange(ps, my input, fws, y, k, indices, distances, begin, end);

        /////////////////////////////////////////////////
        // Computing frequencies and average distances //
        /////////////////////////////////////////////////

        ncategories = KNN_kIndicesToFrequenciesAndDistances(my output, k, indices, distances, freqs, freqindices);

        ////////////////////////
        // Distance weighting //
        ////////////////////////

        switch (dist)
        {
            case kOla_DISTANCE_WEIGHTED_VOTING:
                for (long c = 0; c < ncategories; c++)
                    freqs[c] *= 1 / OlaMAX(distances[c], kOla_MINFLOAT);
                break;

            case kOla_SQUARED_DISTANCE_WEIGHTED_VOTING:
                for (long c = 0; c < ncategories; c++)
                    freqs[c] *= 1 / OlaMAX(OlaSQUARE(distances[c]), kOla_MINFLOAT);
        }

        KNN_normalizeFloatArray(freqs, ncategories);
        outputindices[noutputindices++] = freqindices[KNN_max(freqs, ncategories)];
    }

    Categories output = Categories_create();
    if (output)
    {
        Categories_init(output, 0);
        for (long o = 0; o < noutputindices; o++)
        {
            Collection_addItem(output, Data_copy((my output)->item[outputindices[o]]));
        }
        return(output);
    }
	NUMlvector_free (indices, 0);
	NUMlvector_free (freqindices, 0);
	NUMdvector_free (distances, 0);
	NUMdvector_free (freqs, 0);
	NUMlvector_free (outputindices, 0);
    return(NULL);

}

/////////////////////////////////////////////////////////////////////////////////////////////
// Evaluation                                                                              //
/////////////////////////////////////////////////////////////////////////////////////////////

double KNN_evaluate
(
    ///////////////////////////////
    // Parameters                //
    ///////////////////////////////

    KNN me,             // the classifier being used
                        //
    FeatureWeights fws, // feature weights
                        //
    long k,             // the number of sought after neighbours
                        //
    int dist,           // distance weighting
                        //
    int mode            // kOla_TEN_FOLD_CROSS_VALIDATION / kOla_LEAVE_ONE_OUT
                        //
)

{
    double correct = 0;
    long adder;

    switch(mode)
    {
        case kOla_TEN_FOLD_CROSS_VALIDATION:
            adder = my nInstances / 10;
            break;

        case kOla_LEAVE_ONE_OUT:
            if(my nInstances > 1)
                adder = 1;
            else
                adder = 0;
            break;

        default:
            adder = 0;
    }
    
    if(adder ==  0)
        return(-1);

    for (long begin = 1; begin <= my nInstances; begin += adder)
    {
        Categories c = KNN_classifyFold(me, my input, fws, k, dist, begin, OlaMIN(begin + adder - 1, my nInstances));
        for (long y = 1; y <= c->size; y++)
            if (FRIENDS(c->item[y], (my output)->item[begin + y - 1])) 
                ++correct;

        forget(c);
    }

    correct /= (double) my nInstances;

    return(correct);

}

/////////////////////////////////////////////////////////////////////////////////////////////
// Evaluation using a separate test set                                                    //
/////////////////////////////////////////////////////////////////////////////////////////////

double KNN_evaluateWithTestSet
(
    ///////////////////////////////
    // Parameters                //
    ///////////////////////////////

    KNN me,             // the classifier being used
                        //
    Pattern p,          // The vectors of the test set
                        //
    Categories c,       // The categories of the test set
                        //
    FeatureWeights fws, // feature weights
                        //
    long k,             // the number of sought after neighbours
                        //
    int dist            // distance weighting
                        //
)

{
    double correct = 0;
    Categories t = KNN_classifyToCategories(me, p, fws, k, dist);

    if (t)
    {
        for (long y = 1; y <= t->size; y++)
            if (FRIENDS(t->item[y], c->item[y])) correct++;
        forget(t);
        return(correct / c->size);
    }
    else
    {
        return(0);
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Model search                                                                            //
/////////////////////////////////////////////////////////////////////////////////////////////

double KNN_modelSearch
(
    ///////////////////////////////
    // Parameters                //
    ///////////////////////////////

    KNN me,             // the classifier being used
                        //
    FeatureWeights fws, // feature weights
                        //
    long * k,           // valid long *, to hold the output value of k
                        //
    int * dist,         // valid int *, to hold the output value dist_weight
                        //
    int mode,           // evaluation mode
                        //
    double rate,        // learning rate
                        //
    long nseeds         // the number of seeds to be used
                        //
)

{
    int dists[] = 
    {
        kOla_SQUARED_DISTANCE_WEIGHTED_VOTING, 
        kOla_DISTANCE_WEIGHTED_VOTING, 
        kOla_FLAT_VOTING
    };

    long max = *k;
    double range = (double) max / 2;
    double pivot = range;

    double dpivot = 1;
    double drange = 1;
    double drate = rate / range;

    typedef struct structsoil
    {
        double performance;
        long dist;
        long k;
    } soil;

    soil best = {0, lround(dpivot), lround(dpivot)};
    soil *field = NUMstructvector (soil, 0, nseeds - 1);

    while (range > 0)
    {
        for (long n = 0; n < nseeds; n++)
        {
            field[n].k = lround(NUMrandomUniform(OlaMAX(pivot - range, 1), OlaMIN(pivot + range, max)));
            field[n].dist = lround(NUMrandomUniform(OlaMAX(dpivot - drange, 0), OlaMIN(dpivot + drange, 2)));
            field[n].performance = KNN_evaluate(me, fws, field[n].k, dists[field[n].dist], mode);
        }

        long maxindex = 0;
        for (long n = 1; n < nseeds; n++)
            if (field[n].performance > field[maxindex].performance) maxindex = n;

        if (field[maxindex].performance > best.performance)
        {
            pivot = field[maxindex].k;
            dpivot = field[maxindex].dist;

            best.performance = field[maxindex].performance;
            best.dist = field[maxindex].dist;
            best.k = field[maxindex].k;
        }

        range -= rate;
        drange -= drate;
    }

    *k = best.k;
    *dist = dists[best.dist];

	NUMstructvector_free (soil, field, 0);
    return(best.performance);

}

/////////////////////////////////////////////////////////////////////////////////////////////
// Euclidean distance                                                                      //
/////////////////////////////////////////////////////////////////////////////////////////////

double KNN_distanceEuclidean
(
    Pattern ps,         // Pattern 1
                        //
    Pattern pt,         // Pattern 2
                        //
    FeatureWeights fws, // Feature weights
                        //
    long rows,          // Vector index of pattern 1
                        //
    long rowt           // Vector index of pattern 2
)

{
    double distance = 0;
    for (long x = 1; x <= ps->nx; ++x)
        distance += OlaSQUARE((ps->z[rows][x] - pt->z[rowt][x]) * fws->fweights->data[1][x]);

    return(sqrt(distance));
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Manhattan distance                                                                      //
/////////////////////////////////////////////////////////////////////////////////////////////

double KNN_distanceManhattan
(
    Pattern ps,     // Pattern 1
                    //
    Pattern pt,     // Pattern 2
                    //
    long rows,      // Vector index of pattern 1
                    //
    long rowt       // Vector index of pattern 2
                    //
)

{
    double distance = 0;
    for (long x = 1; x <= ps->nx; ++x)
        distance += fabs(ps->z[rows][x] - pt->z[rowt][x]);

    return(distance);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Find longest distance                                                                   //
/////////////////////////////////////////////////////////////////////////////////////////////

long KNN_max
(
    double * distances,     // an array of distances containing ...
                            //
    long ndistances         // ndistances distances
                            //
)

{
    long maxndx = 0;

    for(long maxc = 1; maxc < ndistances; ++maxc)
        if (distances[maxc] > distances[maxndx]) 
            maxndx = maxc;

    return(maxndx);
}


////////////////////////////////////////////////////////////////////////////////////////////
// Locate k neighbours, skip one + disposal of distance                                    //
/////////////////////////////////////////////////////////////////////////////////////////////

long KNN_kNeighboursSkip
(
    ///////////////////////////////
    // Parameters                //
    ///////////////////////////////

    Pattern j,          // source pattern
                        //
    Pattern p,          // target pattern (where neighbours are sought for)
                        //
    FeatureWeights fws, // feature weights
                        //
    long jy,            // source instance index
                        //
    long k,             // the number of sought after neighbours
                        //
    long * indices,     // memory space to contain the indices of
                        // the k neighbours
                        //
    long skipper        // the index of the instance to be skipped
                        //
)

{
    long maxi;
    long dc = 0;
    long py = 1;

    double *distances = NUMdvector (0, k - 1);

    Melder_assert(jy > 0 && jy <= j->ny);
    Melder_assert(k > 0 && k <= p->ny);
    Melder_assert(skipper <= p->ny);

    while (dc < k && py <= p->ny)
    {
        if ((py != jy) && (py != skipper))
        {

            distances[dc] = KNN_distanceEuclidean(j, p, fws, jy, py);
            indices[dc] = py;
            ++dc;
        }
        ++py;
    }

    maxi = KNN_max(distances, k);
    while (py <= p->ny)
    {
        if ((py != jy) && (py != skipper))
        {

            double d = KNN_distanceEuclidean(j, p, fws, jy, py);
            if (d < distances[maxi])
            {
                distances[maxi] = d;
                indices[maxi] = py;
                maxi = KNN_max(distances, k);
            }
        }
        ++py;
    }
    
	NUMdvector_free (distances, 0);
    return(OlaMIN(k, dc));

}

//////////////////////////////////////////////////////////////////////////////////
// Locate the k nearest neighbours, exclude instances within the range defined  //
// by [begin ... end]                                                           //
//////////////////////////////////////////////////////////////////////////////////

long KNN_kNeighboursSkipRange
(
    ///////////////////////////////
    // Parameters                //
    ///////////////////////////////

    Pattern j,          // source-pattern (where the unknown is located)
                        //
    Pattern p,          // target pattern (where neighbours are sought for)
                        //
    FeatureWeights fws, // feature weights
                        //
    long jy,            // the index of the unknown instance in the source pattern
                        //
    long k,             // the number of sought after neighbours
                        //
    long * indices,     // a pointer to a memory-space big enough for k longs
                        // representing indices to the k neighbours in the
                        // target pattern
                        //
    double * distances, // a pointer to a memory-space big enough for k
                        // doubles representing the distances to the k
                        // neighbours
                        //
    long begin,         // an index indicating the first instance in the
                        // target pattern to be excluded from the search
                        //
    long end            // an index indicating the last instance in the
                        // range of excluded instances in the target
                        // pattern
)

{
    ///////////////////////////////
    // Private variables         //
    ///////////////////////////////

    long maxi;                      // index indicating the most distant neighbour
                                    // among the k nearest
                                    //
    long dc = 0;                    // fetch counter
                                    //
    long py = 0;                    //

    Melder_assert(jy > 0 && jy <= j->ny);
    Melder_assert(k > 0 && k <= p->ny);
    Melder_assert(end > 0 && end <= j->ny);
    Melder_assert(begin > 0 && begin <= j->ny);

    while (dc < k && (end + py) % p->ny + 1 != begin)   // the first k neighbours are the nearest found
    {                                                   // sofar

        if ((end + py) % p->ny + 1 != jy)               // no instance is its own neighbour
        {
            distances[dc] = KNN_distanceEuclidean(j, p, fws, jy, (end + py) % p->ny + 1);
            indices[dc] = (end + py) % p->ny + 1;
            ++dc;
        }
        ++py;
    }

    maxi = KNN_max(distances, k);                       // accept only those instances less distant
    while ((end + py) % p->ny + 1 != begin)             // than the least near one found this far
    {
        if ((end + py) % p->ny + 1 != jy)
        {
            double d = KNN_distanceEuclidean(j, p, fws, jy, (end + py) % p->ny + 1);
            if (d < distances[maxi])
            {
                distances[maxi] = d;
                indices[maxi] = (end + py) % p->ny + 1;
                maxi = KNN_max(distances, k);
            }
        }
        ++py;
    }

    return(OlaMIN(k, dc)); // return the number of found neighbours

}

/////////////////////////////////////////////////////////////////////////////////////////////
// Locate k neighbours                                                                     //
/////////////////////////////////////////////////////////////////////////////////////////////

long KNN_kNeighbours
(
    ///////////////////////////////
    // Parameters                //
    ///////////////////////////////

    Pattern j,          // source-pattern (where the unknown is located)
                        //
    Pattern p,          // target pattern (where neighbours are sought for)
                        //
    FeatureWeights fws, // feature weights
                        //
    long jy,            // the index of the unknown instance in the source pattern
                        //
    long k,             // the number of sought after neighbours
                        //
    long * indices,     // a pointer to a memory-space big enough for k longs
                        // representing indices to the k neighbours in the
                        // target pattern
    double * distances  // a pointer to a memory-space big enough for k
                        // doubles representing the distances to the k
                        // neighbours
                        //
)   

{
    long maxi;
    long dc = 0;
    long py = 1;

    Melder_assert(jy > 0 && jy <= j->ny);
    Melder_assert(k > 0 && k <= p->ny);
    Melder_assert(indices);
    Melder_assert(distances);

    while (dc < k && py <= p->ny)
    {
        if (py != jy)
        {
            distances[dc] = KNN_distanceEuclidean(j, p, fws, jy, py);
            indices[dc] = py;
            ++dc;
        }
        ++py;
    }

    maxi = KNN_max(distances, k);
    while (py <= p->ny)
    {
        if (py != jy)
        {
            double d = KNN_distanceEuclidean(j, p, fws, jy, py);
            if (d < distances[maxi])
            {
                distances[maxi] = d;
                indices[maxi] = py;
                maxi = KNN_max(distances, k);
            }
        }
        ++py;
    }

    long ret = OlaMIN(k, dc);
    if (ret < 1)
    {
        indices[0] = jy;
        return(0);
    }
    else
        return(ret);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Locating k (nearest) friends                                                            //
/////////////////////////////////////////////////////////////////////////////////////////////

long KNN_kFriends
(
    ///////////////////////////////
    // Parameters                //
    ///////////////////////////////

    Pattern j,          // source-pattern
                        //
    Pattern p,          // target pattern (where friends are sought for)
                        //
    Categories c,       // categories
                        //
    long jy,            // the index of the source instance
                        //  
    long k,             // the number of sought after friends
                        //
    long * indices      // a pointer to a memory-space big enough for k longs
                        // representing indices to the k friends in the
                        // target pattern
)

{
    long maxi;
    long dc = 0;
    long py = 1;
    double *distances = NUMdvector (0, k - 1);

    Melder_assert(jy <= j->ny  && k <= p->ny && k > 0);
    Melder_assert(indices);

    while (dc < k && py < p->ny)
    {
        if (jy != py && FRIENDS(c->item[jy], c->item[py]))
        {
            distances[dc] = KNN_distanceManhattan(j, p, jy, py);
            indices[dc] = py;
            dc++;
        }
        ++py;
    }

    maxi = KNN_max(distances, k);
    while (py <= p->ny)
    {
        if (jy != py && FRIENDS(c->item[jy],c->item[py]))
        {
            double d = KNN_distanceManhattan(j, p, jy, py);
            if (d < distances[maxi])
            {
                distances[maxi] = d;
                indices[maxi] = py;
                maxi = KNN_max(distances, k);
            }
        }
        ++py;
    }

	NUMdvector_free (distances, 0);
    return(OlaMIN(k,dc));

}

/////////////////////////////////////////////////////////////////////////////////////////////
// Computing the distance to the nearest enemy                                             //
/////////////////////////////////////////////////////////////////////////////////////////////

double KNN_nearestEnemy
(
    ///////////////////////////////
    // Parameters                //
    ///////////////////////////////

    Pattern j,      // source-pattern
                    //
    Pattern p,      // target pattern (where friends are sought for)
                    //
    Categories c,   // categories
                    //
    long jy         // the index of the source instance
                    //
)

{
    double distance = KNN_distanceManhattan(j, p, jy, 1);
    Melder_assert(jy > 0 && jy <= j->ny );

    for (long y = 2; y <= p->ny; y++)
    {
        if (ENEMIES(c->item[jy], c->item[y]))
        {
            double newdist = KNN_distanceManhattan(j, p, jy, y);
            if (newdist > distance) distance = newdist;
        }
    }

    return(distance);

}


/////////////////////////////////////////////////////////////////////////////////////////////
// Computing the number of friends among k neighbours                                      //
/////////////////////////////////////////////////////////////////////////////////////////////

long KNN_friendsAmongkNeighbours
(
    ///////////////////////////////
    // Parameters                //
    ///////////////////////////////

    Pattern j,      // source-pattern
                    //
    Pattern p,      // target pattern (where friends are sought for)
                    //
    Categories c,   // categories
                    //
    long jy,        // the index of the source instance
                    //
    long k          // k (!)
                    //
)

{
    double *distances = NUMdvector (0, k - 1);
    long *indices = NUMlvector (0, k - 1);
    long friends = 0;

    Melder_assert(jy > 0 && jy <= j->ny  && k <= p->ny && k > 0);

    FeatureWeights fws = FeatureWeights_create(p->nx);
    long ncollected = KNN_kNeighbours(j, p, fws, jy, k, indices, distances);
    forget(fws);

    while (ncollected--)
        if (FRIENDS(c->item[jy], c->item[indices[ncollected]])) friends++;
    
	NUMdvector_free (distances, 0);
	NUMlvector_free (indices, 0);
    return(friends);

}

/////////////////////////////////////////////////////////////////////////////////////////////
// Locating k unique (nearest) enemies                                                     //
/////////////////////////////////////////////////////////////////////////////////////////////

long KNN_kUniqueEnemies
(
    ///////////////////////////////
    // Parameters                //
    ///////////////////////////////

    Pattern j,      // source-pattern
                    //
    Pattern p,      // target pattern (where friends are sought for)
                    //
    Categories c,   // categories
                    //
    long jy,        // the index of the source instance
                    //
    long k,         // k (!)
                    //
    long * indices  // a memory space to hold the indices of the
                    // located enemies
                    //
)

{
    long maxi;
    long dc = 0;
    long py = 1;
    double *distances = NUMdvector (0, k - 1);

    Melder_assert (jy <= j->ny);
	Melder_assert (k <= p->ny);
	Melder_assert (k > 0);
    Melder_assert (indices != NULL);

    while (dc < k && py <= p->ny)
    {
        if (ENEMIES(c->item[jy], c->item[py]))
        {
            int hasfriend = 0;
            for (long sc = 0; sc < dc; ++sc)
                if (FRIENDS(c->item[py], c->item[indices[sc]])) hasfriend = 1;

            if (!hasfriend)
            {
                distances[dc] = KNN_distanceManhattan(j, p, jy, py);
                indices[dc] = py;
                ++dc;
            }
        }
        ++py;
    }

    maxi = KNN_max(distances, k);
    while (py <= p->ny)
    {
        if (ENEMIES(c->item[jy], c->item[py]))
        {
            int hasfriend = 0;
            for (long sc = 0; sc < dc; ++sc)
                if (FRIENDS(c->item[py], c->item[indices[sc]])) hasfriend = 1;

            if (!hasfriend)
            {
                double d = KNN_distanceManhattan(j, p, jy, py);
                if (d < distances[maxi] && FRIENDS(c->item[jy],c->item[py]))
                {
                    distances[maxi] = d;
                    indices[maxi] = py;
                    maxi = KNN_max(distances, k);
                }
            }
        }
        ++py;
    }
	NUMdvector_free (distances, 0);
    return(OlaMIN(k,dc));
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Compute dissimilarity matrix                                                            //
/////////////////////////////////////////////////////////////////////////////////////////////

Dissimilarity KNN_patternToDissimilarity
(
    ///////////////////////////////
    // Parameters                //
    ///////////////////////////////

    Pattern p,          // Pattern
                        //
    FeatureWeights fws  // Feature weights
                        //
)

{
    Dissimilarity output = Dissimilarity_create(p->ny);
    Melder_assert(output);
    if (output)
    {
        for (long y = 1; y <= p->ny; ++y)
            for (long x = 1; x <= p->ny; ++x)
                output->data[y][x] = KNN_distanceEuclidean(p, p, fws, y, x);
        
        return(output);
    }
    else 
        return(NULL);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Compute frequencies                                                                     //
/////////////////////////////////////////////////////////////////////////////////////////////

long KNN_kIndicesToFrequenciesAndDistances
(
    ///////////////////////////////
    // Parameters                //
    ///////////////////////////////

    Categories c,       // Source categories
                        //
    long k,             // k (!)
                        //
    long * indices,     // In: indices
                        //
    double * distances, // Out: distances
                        //
    double * freqs,     // Out: and frequencies (double, sic!)
                        //
    long *freqindices   // Out: and indices -> freqs.
)

{
    long ncategories = 0;
    
    Melder_assert(k <= c->size && k > 0);
    Melder_assert(distances && indices && freqs  && freqindices);

    for (long y = 0; y < k; ++y)
    {
        int hasfriend = 0;
        long friend = 0;

        while (friend < ncategories)
        {
            if (FRIENDS(c->item[indices[y]], c->item[freqindices[friend]]))
            {
                hasfriend = 1;
                break;
            }
            ++friend;
        }

        if (!hasfriend)
        {
            freqindices[ncategories] = indices[y];
            freqs[ncategories] = 1;
            distances[ncategories] = distances[y];
            ncategories++;
        }
        else
        {
            ++freqs[friend];
            distances[friend] += (distances[y] - distances[friend]) / (ncategories + 1);
        }
    }
    return(ncategories);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Normalize array                                                                         //
/////////////////////////////////////////////////////////////////////////////////////////////

void KNN_normalizeFloatArray
(
    ///////////////////////////////
    // Parameters                //
    ///////////////////////////////

    double * array,     // Array to be normalized
                        //
    long n              // The number of elements
                        // in the array
)

{
    long c = 0;
    double sum = 0;

    while(c < n)
        sum += array[c++];

    while(c--)
        array[c] /= sum;
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Remove instance                                                                         //
/////////////////////////////////////////////////////////////////////////////////////////////

void KNN_removeInstance
(
    ///////////////////////////////
    // Parameters                //
    ///////////////////////////////

    KNN me,     // Classifier
                //
    long y      // Index of the instance to be purged
                //
)

{
    if (y == 1 && my nInstances == 1)
    {
        my nInstances = 0;
        forget(my input);
        forget(my output);
        return;
    }

    Melder_assert(y > 0 && y <= my nInstances);
    if (y > my nInstances || y < 1) 
        return;                         // safety belt

    Pattern new = Pattern_create(my nInstances - 1, (my input)->nx);
    Melder_assert(new);
    if (new)
    {
        long yt = 1;
        for (long cy = 1; cy <= my nInstances; ++cy)
            if (cy != y)
            {
                for (long cx = 1; cx <= (my input)->nx; ++cx)
                    new->z[yt][cx] = (my input)->z[cy][cx];
                ++yt;
            }

        forget(my input);
        my input = new;
        Collection_removeItem(my output, y);
        my nInstances--;
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Shuffle instances                                                                       //
/////////////////////////////////////////////////////////////////////////////////////////////

void KNN_shuffleInstances
(
    ///////////////////////////////
    // Parameters                //
    ///////////////////////////////

    KNN me      // Classifier whose instance
                // base is to be shuffled
)

{
    if (my nInstances < 2) 
        return;                 // It takes atleast two to tango

    Pattern new_input = Pattern_create(my nInstances, (my input)->nx);
    Categories new_output = Categories_create();
    Melder_assert(new_input && new_output);
    if (new_input && new_output)
    {
        long y = 1;
        Categories_init(new_output, 0);
        while (my nInstances)
        {
            long pick = (long) lround(NUMrandomUniform(1, my nInstances));
            Collection_addItem(new_output, Data_copy((my output)->item[pick]));

            for (long x = 1;x <= (my input)->nx; ++x)
                new_input->z[y][x] = (my input)->z[pick][x];

            KNN_removeInstance(me, pick);
            ++y;
        }

        forget(my input);
        forget(my output);

        my input = new_input;
        my output = new_output;
        my nInstances  = new_output->size;
    }
    else
    {
        forget(new_input);
        forget(new_output);
    }
}


/////////////////////////////////////////////////////////////////////////////////////////////
// KNN to Permutation (experimental)                                                       //
/////////////////////////////////////////////////////////////////////////////////////////////
Permutation KNN_SA_ToPermutation
(
    ///////////////////////////////
    // Parameters                //
    ///////////////////////////////

    KNN me,             // the classifier being used
                        //
    long tries,         //
                        //
    long iterations,    //
                        //
    double step_size,   //
                        //
    double boltzmann_c, //
                        //
    double temp_start,  //
                        //
    double damping_f,   //
                        //
    double temp_stop    //
                        //
)

{
    gsl_rng * r;
    const gsl_rng_type * T;

    KNN_SA_t * istruct = KNN_SA_t_create(my input); 
    Permutation result = Permutation_create(my nInstances);

    gsl_siman_params_t params = {tries, iterations, step_size, boltzmann_c, temp_start, damping_f, temp_stop};

    gsl_rng_env_setup();
    T = gsl_rng_default;
    r = gsl_rng_alloc(T);

    gsl_siman_solve(r, 
                    istruct, 
                    KNN_SA_t_energy, 
                    KNN_SA_t_step, 
                    KNN_SA_t_metric, 
                    NULL,                // KNN_SA_t_print, 
                    KNN_SA_t_copy, 
                    KNN_SA_t_copy_construct, 
                    KNN_SA_t_destroy, 
                    0, 
                    params);

    for(unsigned long i = 1; i <= my nInstances; ++i)
        result->p[i] = istruct->indices[i];

    KNN_SA_t_destroy(istruct);

    return(result);
}


double KNN_SA_t_energy
(
    ///////////////////////////////
    // Parameters                //
    ///////////////////////////////

    void * istruct
             
)

{
    if(((KNN_SA_t *) istruct)->p->ny < 2) 
        return(0);  

    double eCost = 0;
    for(long i = 1; i <= ((KNN_SA_t *) istruct)->p->ny; ++i)
    {
        /* fast and sloppy version */ 
       
        double jDist = 0;
        double kDist = 0;

        long j = i - 1 > 0 ? i - 1 : ((KNN_SA_t *) istruct)->p->ny;
        long k = i + 1 <= ((KNN_SA_t *) istruct)->p->ny ? i + 1 : 1;

        for (long x = 1; x <= ((KNN_SA_t *) istruct)->p->nx; ++x)
        {
            jDist +=    OlaSQUARE(((KNN_SA_t *) istruct)->p->z[((KNN_SA_t *) istruct)->indices[i]][x] - 
                        ((KNN_SA_t *) istruct)->p->z[((KNN_SA_t *) istruct)->indices[j]][x]);
            
            kDist +=    OlaSQUARE(((KNN_SA_t *) istruct)->p->z[((KNN_SA_t *) istruct)->indices[i]][x] - 
                        ((KNN_SA_t *) istruct)->p->z[((KNN_SA_t *) istruct)->indices[k]][x]);
        }   
        
        eCost += ((sqrt(jDist) + sqrt(kDist)) / 2 - eCost) / i;

    }

    return(eCost);
}

double KNN_SA_t_metric
(
    void * istruct1,
    void * istruct2
)

{
    double result = 0;

    for(long i = ((KNN_SA_t *) istruct1)->p->ny; i >= 1; --i)
        if(((KNN_SA_t *) istruct1)->indices[i] !=  ((KNN_SA_t *) istruct2)->indices[i]) 
            ++result;

    return(result);
}

void KNN_SA_t_print
(
    void * istruct
)

{
    Melder_casual("\n");
    for(long i = 1; i <= ((KNN_SA_t *)istruct)->p->ny; ++i)
        Melder_casual("%ld,", ((KNN_SA_t *)istruct)->indices[i]);
    Melder_casual("\n");
}

void KNN_SA_t_step
(
    const gsl_rng * r,
    void * istruct,
    double step_size
)

{
    long i1 = lround((((KNN_SA_t *) istruct)->p->ny - 1) * gsl_rng_uniform(r)) + 1;
    long i2 = (i1 + lround(step_size * gsl_rng_uniform(r))) % ((KNN_SA_t *) istruct)->p->ny + 1;

    if(i1 == i2)
        return;

    if(i1 > i2)
        OlaSWAP(long, i1, i2);

    long partitions[i2 - i1 + 1];

    KNN_SA_partition(((KNN_SA_t *) istruct)->p, i1, i2, partitions);

    for(long r, l = 1, stop = i2 - i1 + 1; l < stop; ++l)
    {
        while(l < stop && partitions[l] == 1)
            ++l;

        r = l + 1;

        while(r <= stop && partitions[r] == 2)
            ++r;

        if(r == stop)
            break;
        
        OlaSWAP(long, ((KNN_SA_t *) istruct)->indices[i1], ((KNN_SA_t *) istruct)->indices[i2]); 
    }
}

void KNN_SA_t_copy
(
    void * istruct_src,
    void * istruct_dest
)

{
    ((KNN_SA_t *) istruct_dest)->p = ((KNN_SA_t *) istruct_src)->p;

    for(long i = 1; i <= ((KNN_SA_t *) istruct_dest)->p->ny; ++i)
        ((KNN_SA_t *) istruct_dest)->indices[i] = ((KNN_SA_t *) istruct_src)->indices[i];
}


void * KNN_SA_t_copy_construct
(
    void * istruct
)

{
    KNN_SA_t * result = malloc(sizeof(KNN_SA_t));

    result->p = ((KNN_SA_t *) istruct)->p;
    result->indices = malloc(sizeof(long) * (result->p->ny + 1));

    for(long i = 1; i <= result->p->ny; ++i)
        result->indices[i] = ((KNN_SA_t *) istruct)->indices[i];

    return((void *) result);
}

KNN_SA_t * KNN_SA_t_create
(
    Pattern p
)

{
    KNN_SA_t * result = malloc(sizeof(KNN_SA_t));

    result->p = p;
    result->indices = malloc(sizeof(long) * (p->ny + 1));

    for(long i = 1; i <= p->ny; ++i)
        result->indices[i] = i;

    return(result);
}

void KNN_SA_t_destroy
(
    void * istruct
)

{
    free(((KNN_SA_t *) istruct)->indices);
    free((KNN_SA_t *) istruct);
}

void KNN_SA_partition
(
    ///////////////////////////////
    // Parameters                //
    ///////////////////////////////

    Pattern p,              // 
                            //
    long i1,                // i1 < i2
                            //
    long i2,                //
                            //
    long * result           // [0] anv. ej
                            //
)

{
    long c1 = (long) lround(NUMrandomUniform(i1, i2));
    long c2 = (long) lround(NUMrandomUniform(i1, i2));

    double *p1 = NUMdvector (1, p->nx); 
    double *p2 = NUMdvector (1, p->nx);

    for(long x = 1; x <= p->nx; ++x)
    {
        p1[x] = p->z[c1][x];
        p2[x] = p->z[c2][x];
    }

    for(short converging = 1; converging; )
    {
        double d1, d2;
        converging = 0;

        for(long i = i1, j = 1; i <= i2; ++i)
        {
            d1 = 0;
            d2 = 0;
        
            for (long x = 1; x <= p->nx; ++x)
            {
                d1 += OlaSQUARE(p->z[i][x] - p1[x]);
                d2 += OlaSQUARE(p->z[i][x] - p2[x]);
            }

            d1 = sqrt(d1);
            d2 = sqrt(d2);

            if(d1 < d2)
            {
                if(result[j] != 1)
                {
                    converging = 1;
                    result[j] = 1;
                }
            }
            else
            { 
                if(result[j] != 2)
                {
                    converging = 1;
                    result[j] = 2;
                }
            }
            ++j;
        }

        for(long x = 1; x <= p->nx; ++x)
        {
            p1[x] = 0;
            p2[x] = 0;
        }

        for(long i = i1, j = 1, j1 = 1, j2 = 1; i <= i2; ++i)
        {
            if(result[j] == 1)
            {
                for(long x = 1; x <= p->nx; ++x)
                    p1[x] += (p->z[i][x] - p1[x]) / j1;
                ++j1;
            }
            else
            {
                for(long x = 1; x <= p->nx; ++x)
                    p2[x] += (p->z[i][x] - p2[x]) / j2;
                ++j2;
            }
            ++j;
        }
    }
    NUMdvector_free (p1, 1);
    NUMdvector_free (p2, 1);
}


/* End of file KNN.c */
