// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*																			*/
/* File:      famg_uginterface.h											*/
/*																			*/
/* Purpose:   famg - ug interface											*/
/*																			*/
/* Author:    Christian Wagner												*/
/*			  Institut fuer Computeranwendungen  III						*/
/*			  Universitaet Stuttgart										*/
/*			  Pfaffenwaldring 27											*/
/*			  70569 Stuttgart												*/
/*			  internet: chris@ica3.uni-stuttgart.de							*/
/*																			*/
/*																			*/
/* History:   November 97 begin, Stuttgart									*/
/*			  August 98 integration into ug (Christian Wrobel)				*/
/*																			*/
/* Remarks:																	*/
/*																			*/
/****************************************************************************/

#ifndef __FAMG_INTERFACE__
#define __FAMG_INTERFACE__

/* RCS_ID
   $Header$
 */

//#include "gm.h"        /* for ug data structure               */
#include "famg_algebra.h"


#define FAMG_RHS      0
#define FAMG_UNKNOWN  1
#define FAMG_DEFECT   2
#define FAMG_TVA      3
#define FAMG_TVB      4
#define FAMG_NVECTORS 5

#ifdef USE_UG_DS
struct FAMG_Interface
{
  FAMGMatrixAlg* matrix;
  FAMGGridVector* gridvector;
  FAMGVector* vector[FAMG_NVECTORS];
};
#else
struct FAMG_Interface
{
  int n;
  int nl;
  int nv;
  int *start;
  int *index;
  double *entry;
  double *vector[FAMG_NVECTORS];
  void **extra;
};
#endif

struct FAMGParameter_ug
{
  int heap;
  int nv;
  int gamma;
  int n1;
  int n2;
  double ilut;
  double cgilut;
  int cgnodes;
  double mincoarse;
  int conloops;
  int type;
  int stv;
  double tol;
  double sigma;
  double omegar;
  double omegal;
  double error1;
  double error2;
  int maxit;
  double alimit;
  double rlimit;
  double divlimit;
  double reduction;
  char solver[10];
  char presmoother[10];
  char postsmoother[10];
  char cgsmoother[10];
};

#ifdef UG_DRAW
struct FAMG_IndexBitField
{
  unsigned type : 1;
  unsigned id : 31;
};

struct FAMG_MatrixPtr
{
  struct FAMG_IndexBitField *index;
  double *entry;
  double **adjoined;
  int nc;
};

typedef struct FAMG_MatrixPtr FAMG_MatrixPtr;

struct FAMG_TransferBitField
{
  unsigned f1 : 1;
  unsigned f0 : 31;
};


struct FAMG_TransferEntry
{
  double data;
  struct FAMG_TransferBitField id;
  struct FAMG_TransferEntry *next;
};

typedef struct FAMG_TransferEntry FAMG_TransferEntry;

FAMG_MatrixPtr FAMG_GetMatrixPtr(int level,int i);
FAMGVector* FAMG_GetVector(int level, int i);
int FAMG_GetMaxLevel();
FAMG_TransferEntry* FAMG_GetTransferEntry(int level,int i);
#endif


int FAMGConstructParameter(struct FAMGParameter_ug *in_parameter);
void FAMGDeconstructParameter();
int FAMGConstruct(FAMGGridVector *gridvector, FAMGMatrixAlg *matrix, FAMGVector *vectors[FAMG_NVECTORS]);
int FAMGConstructSimple(FAMGMatrixAlg *matrix, FAMGVector *tvA, FAMGVector *tvB);
int FAMGSolve(FAMGVector *rhs, FAMGVector *defect, FAMGVector *unknown);
void FAMGDeconstruct();
void FAMGDeconstructSimple();

int FAMGSolveSystem(struct FAMG_Interface*);
int FAMG_GetNF(int level);

// for debugging
class FAMGSystem;
FAMGSystem *FAMG_GetSystem();

#endif