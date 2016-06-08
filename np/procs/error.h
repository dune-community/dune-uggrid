// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*                                                                                                                                                      */
/* File:          error.h                                                                                                       */
/*                                                                                                                                                      */
/* Purpose:   header for indicator.c                                                                            */
/*                                                                                                                                                      */
/* Author:        Christian Wieners                                                                             */
/*                        Institut fuer Computeranwendungen III                                                 */
/*                        Universitaet Stuttgart                                                                                */
/*                        Pfaffenwaldring 27                                                                                    */
/*                        70550 Stuttgart                                                                                               */
/*                        email: ug@ica3.uni-stuttgart.de                                                       */
/*                                                                                                                                                      */
/* History:   Sep 4, 1996, ug version 3.4                                                               */
/*                                                                                                                                                      */
/* Remarks:                                                                                                                             */
/*                                                                                                                                                      */
/****************************************************************************/



/****************************************************************************/
/*                                                                                                                                                      */
/* auto include mechanism and other include files                                                       */
/*                                                                                                                                                      */
/****************************************************************************/

#ifndef __ERROR__
#define __ERROR__

#include "np.h"
#include "ts.h"

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

#define ERROR_CLASS_NAME "error"

/****************************************************************************/
/*                                                                                                                                                      */
/* data structures exported by the corresponding source file                            */
/*                                                                                                                                                      */
/****************************************************************************/

/* a data type for returning the status of the computation                  */
typedef struct {
  INT error_code;                           /* error code                       */
  INT nel;                                  /* number of surface elements       */
  INT refine;                               /* nb. el. marked for refinement    */
  INT coarse;                               /* nb. el. marked for coarsening    */
  DOUBLE step;                              /* new timestepsize                 */
  DOUBLE error;                             /* error estimate                   */
} ERESULT;

struct np_error {

  NP_BASE base;                              /* inherits base class             */

  /* data (optinal, necessary for calling the generic execute routine)    */
  VECDATA_DESC *x;                       /* solution                        */
  VECDATA_DESC *o;                       /* old solution                    */
  NP_T_SOLVER *ts;                       /* reference to timesolver         */

  /* functions */
  INT (*PreProcess)
    (struct np_error *,                      /* pointer to (derived) object     */
    INT,                                         /* level                           */
    INT *);                                      /* result                          */
  INT (*Error)
    (struct np_error *,                      /* pointer to (derived) object     */
    INT,                                         /* level                           */
    VECDATA_DESC *,                              /* solution vector                 */
    ERESULT *);                                  /* result                          */
  INT (*TimeError)
    (struct np_error *,                      /* pointer to (derived) object     */
    INT,                                         /* level                           */
    DOUBLE,                                      /* time                            */
    DOUBLE *,                                    /* time step                       */
    VECDATA_DESC *,                              /* solution vector                 */
    VECDATA_DESC *,                              /* old solution vector             */
    NP_T_SOLVER *,                               /* reference to timesolver         */
    ERESULT *);                                  /* result                          */
  INT (*PostProcess)
    (struct np_error *,                      /* pointer to (derived) object     */
    INT,                                         /* level                           */
    INT *);                                      /* result                          */
};
typedef struct np_error NP_ERROR;

typedef INT (*PreProcessErrorProcPtr)                                       \
  (NP_ERROR *, INT, INT *);
typedef INT (*ErrorProcPtr)                                                 \
  (NP_ERROR *, INT, VECDATA_DESC *, VECDATA_DESC *, ERESULT *);
typedef INT (*TimeErrorProcPtr)                                             \
  (NP_ERROR *, INT, DOUBLE, DOUBLE *, VECDATA_DESC *, VECDATA_DESC *,        \
  NP_T_SOLVER *, ERESULT *);
typedef INT (*PostProcessErrorProcPtr)                                      \
  (NP_ERROR *, INT, INT *);

/****************************************************************************/
/*                                                                                                                                                      */
/* definition of exported global variables                                                                      */
/*                                                                                                                                                      */
/****************************************************************************/

/****************************************************************************/
/*                                                                                                                                                      */
/* function declarations                                                                                                        */
/*                                                                                                                                                      */
/****************************************************************************/

INT SurfaceIndicator (MULTIGRID *theMG, VECDATA_DESC *theVD,
                      DOUBLE refine, DOUBLE coarse, INT project,
                      INT from, INT to, INT clear, ERESULT *eresult);

/* generic init function for Error num procs */
INT NPErrorInit (NP_ERROR *theNP, INT argc , char **argv);

/* generic display function for Error num procs */
INT NPErrorDisplay (NP_ERROR *theNP);

/* generic execute function for Error num procs */
INT NPErrorExecute (NP_BASE *theNP, INT argc , char **argv);

INT     InitError (void);

END_UGDIM_NAMESPACE

#endif
