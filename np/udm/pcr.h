// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*                                                                                                                                                      */
/* File:          pcr.h                                                                                                                 */
/*                                                                                                                                                      */
/* Purpose:   defines for priniting of convergence rates                                        */
/*                                                                                                                                                      */
/* Author:        Klaus Johannsen/Henrik Rentz-Reichert                                                 */
/*                        Institut fuer Computeranwendungen III                                                 */
/*                        Universitaet Stuttgart                                                                                */
/*                        Pfaffenwaldring 27                                                                                    */
/*                        70569 Stuttgart                                                                                               */
/*                        email: ug@ica3.uni-stuttgart.de                                                               */
/*                                                                                                                                                      */
/* History:   25.03.95 begin, ug version 3.0                                                            */
/*                        09.12.95 transition to new descriptor formats (HRR)                   */
/*                        December 2, 1996 redesign of numerics                                                 */
/*                                                                                                                                                      */
/* Remarks:                                                                                                                             */
/*                                                                                                                                                      */
/****************************************************************************/



/****************************************************************************/
/*                                                                                                                                                      */
/* auto include mechanism and other include files                                                       */
/*                                                                                                                                                      */
/****************************************************************************/

#ifndef __PCR__
#define __PCR__

#include "ugtypes.h"
#include "gm.h"
#include "algebra.h"
#include "ugenv.h"
#include "udm.h"
#include "numproc.h"
#include "scan.h"

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

/* DisplayModes for PrintConvergenceRate */
enum DisplayMode {PCR_NO_DISPLAY,
                  PCR_RED_DISPLAY,
                  PCR_FULL_DISPLAY };

/* printmode PrintConvergenceRate */
enum PrintMode {PCR_CRATE,
                PCR_AVERAGE,
                PCR_INTERN,
                PCR_CRATE_SD,
                PCR_AVERAGE_SD,
                PCR_INTERN_SD};

/****************************************************************************/
/*                                                                                                                                                      */
/* function declarations                                                                                                        */
/*                                                                                                                                                      */
/****************************************************************************/

INT                     GetStrINTinRange                        (const char *str, INT min, INT max, INT *value);
INT                     GetStrDOUBLEinRange                     (const char *str, DOUBLE min, DOUBLE max, DOUBLE *value);
INT             PreparePCR                                      (VECDATA_DESC *Vsym, INT DispMode, const char *text, INT *ID);
INT         PrepareEPCR                 (EVECDATA_DESC *Vsym, INT DispMode, const char *text, INT *ID);
INT             PostPCR                                         (INT ID, const char *path);
INT             DoPCR                                           (INT ID, VEC_SCALAR Defect, INT PrintMode);

END_UGDIM_NAMESPACE

#endif
