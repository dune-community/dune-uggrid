// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*                                                                                                                                                      */
/* File:          freebnd.c                                                                                                             */
/*                                                                                                                                                      */
/* Purpose:   moving free boundaries                                                                            */
/*                                                                                                                                                      */
/* Author:        Henrik Rentz-Reichert                                                                                 */
/*                        Institut fuer Computeranwendungen III                                                 */
/*                        Universitaet Stuttgart                                                                                */
/*                        Pfaffenwaldring 27                                                                                    */
/*                        70550 Stuttgart                                                                                               */
/*                        email: ug@ica3.uni-stuttgart.de                                                               */
/*                                                                                                                                                      */
/* History:   01.09.97 begin, ug version 3.7                                                            */
/*                                                                                                                                                      */
/* Remarks:                                                                                                                             */
/*                                                                                                                                                      */
/****************************************************************************/



/****************************************************************************/
/*                                                                                                                                                      */
/* auto include mechanism and other include files                                                       */
/*                                                                                                                                                      */
/****************************************************************************/

#ifndef __FREEBND__
#define __FREEBND__

#include "ugtypes.h"
#include "np.h"

#include "namespace.h"

START_UGDIM_NAMESPACE

/****************************************************************************/
/*                                                                                                                                                      */
/* defines in the following order                                                                                       */
/*                                                                                                                                                      */
/*                compile time constants defining static data size (i.e. arrays)        */
/*                other constants                                                                                                       */
/*                macros                                                                                                                        */
/*                                                                                                                                                      */
/****************************************************************************/



/****************************************************************************/
/*                                                                                                                                                      */
/* data structures exported by the corresponding source file                            */
/*                                                                                                                                                      */
/****************************************************************************/



/****************************************************************************/
/*                                                                                                                                                      */
/* definition of exported global variables                                                                      */
/*                                                                                                                                                      */
/****************************************************************************/



/****************************************************************************/
/*                                                                          */
/* function declarations                                                    */
/*                                                                          */
/****************************************************************************/

INT MoveFreeBoundary            (MULTIGRID *mg, INT level, const VECDATA_DESC *vd);
INT StoreMGgeom                         (const MULTIGRID *mg, const VECDATA_DESC *vd);
INT RestoreMGgeom                       (MULTIGRID *mg, const VECDATA_DESC *vd);
INT ComputeBoundaryVelocity (MULTIGRID *mg, INT fl, INT tl, const VECDATA_DESC *VD_p_0, const VECDATA_DESC *VD_p_m1, DOUBLE dt, const VECDATA_DESC *VD_vel);

END_UGDIM_NAMESPACE

#endif
