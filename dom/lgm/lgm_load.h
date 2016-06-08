// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*																			*/
/* File:	  lgm_domain.h													*/
/*																			*/
/* Purpose:   header file for lgm_domain                                                                        */
/*																			*/
/* Author:	  Klaus Johannsen                                                                                               */
/*			  Institut fuer Computeranwendungen III                                                 */
/*			  Universitaet Stuttgart										*/
/*			  Pfaffenwaldring 27											*/
/*			  70550 Stuttgart												*/
/*			  email: klaus@ica3.uni-stuttgart.de							*/
/*																			*/
/* History:   07.09.96 begin												*/
/*																			*/
/* Remarks:                                                                                                                             */
/*																			*/
/****************************************************************************/



/****************************************************************************/
/*																			*/
/* auto include mechanism and other include files							*/
/*																			*/
/****************************************************************************/

#ifndef __LGM_LOAD__
#define __LGM_LOAD__

#include "heaps.h"
#include "domain.h"
#include "lgm_domain.h"

#include "namespace.h"

START_UGDIM_NAMESPACE


/****************************************************************************/
/*																			*/
/* defines in the following order											*/
/*																			*/
/*		  compile time constants defining static data size (i.e. arrays)	*/
/*		  other constants													*/
/*		  macros															*/
/*																			*/
/****************************************************************************/

/****************************************************************************/
/*																			*/
/*	defines for basic configuration											*/
/*																			*/
/****************************************************************************/
#define  ACCEL_WITH_HASH        1
#define  FACTOR_FOR_HASHSIZE    20
/****************************************************************************/
/*																			*/
/* data structures exported by the corresponding source file				*/
/*																			*/
/****************************************************************************/

/****************************************************************************/
/*																			*/
/* definition of exported global variables									*/
/*																			*/
/****************************************************************************/


/****************************************************************************/
/*																			*/
/* function declarations													*/
/*																			*/
/****************************************************************************/

LGM_DOMAIN *LGM_LoadDomain      (const char *filename, const char *name, HEAP *theHeap, INT DomainVarID, INT MarkKey);
INT                     LGM_LoadMesh    (const char *filename, HEAP *theHeap, MESH *theMesh, LGM_DOMAIN *theDomain, INT MarkKey);
INT                     InitLGMLoad             (void);


END_UGDIM_NAMESPACE

#endif
