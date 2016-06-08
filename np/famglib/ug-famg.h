// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*																			*/
/* File:      ug-famg.h														*/
/*																			*/
/* Purpose:   ug - famg interface											*/
/*																			*/
/* Author:    Christian Wrobel												*/
/*			  IWR technische Simulation										*/
/*			  Universitaet Heidelberg										*/
/*			  Im Neuenheimer Feld 368										*/
/*			  69120 Heidelberg												*/
/*			  internet: christian@ica3.uni-stuttgart.de						*/
/*																			*/
/*																			*/
/* History:   November 98 begin												*/
/*																			*/
/* Remarks:																	*/
/*																			*/
/****************************************************************************/

#ifndef __UG_FAMG__
#define __UG_FAMG__

#include "amgtransfer.h"


/****************************************************************************/
/*																			*/
/* data structures exported by the corresponding source file				*/
/*																			*/
/****************************************************************************/

typedef struct
{
  NP_ITER iter;

  INT heap;
  INT n1;
  INT n2;
  INT gamma;
  INT cgnodes;
#ifdef ModelP
  INT cgminnodespe;
#endif
  INT cglevels;
  DOUBLE coarsening;
  DOUBLE strong;
  INT adaptive;
  INT maxit;
  DOUBLE alimit;
  DOUBLE rlimit;
  DOUBLE divlimit;
  DOUBLE reduction;
  INT famg_mark_key;
} NP_FAMG_ITER;

typedef struct
{
  NP_AMG_TRANSFER amg_trans;

  INT famg_mark_key;
  INT coarsegridsolver;
  INT coarsegridagglo;

  VECDATA_DESC *smooth_globsol;         /* for the fine node smoother */
  VECDATA_DESC *smooth_sol;                     /* for the fine node smoother */
  VECDATA_DESC *smooth_def;                     /* for the fine node smoother */
  VECDATA_DESC *tv;                             /* test vector */
  VECDATA_DESC *tvT;                            /* test vector */
  MATDATA_DESC *ConsMat;                        /* (partly) consistent matrix */
  MATDATA_DESC *D;                              /* approximation of the inverse of the diagonal*/
  int ConsMatTempAllocated;
} NP_FAMG_TRANSFER;

/****************************************************************************/
/*                                                                          */
/* Functions                                                                */
/*                                                                          */
/****************************************************************************/

INT FAMGRestrictDefect (NP_TRANSFER *theNP, INT level,
                        VECDATA_DESC *to, VECDATA_DESC *from,
                        MATDATA_DESC *A, VEC_SCALAR damp,
                        INT *result);

INT FAMGInterpolateCorrection (NP_TRANSFER *theNP, INT level,
                               VECDATA_DESC *to, VECDATA_DESC *from,
                               MATDATA_DESC *A, VEC_SCALAR damp,
                               INT *result);
INT InitFAMG (void);

#endif
