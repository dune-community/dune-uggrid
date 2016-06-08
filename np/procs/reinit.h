// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*                                                                          */
/* File:      reinit.h                                                      */
/*                                                                          */
/* Purpose:   numproc interface to reinit problem classes                   */
/*                                                                          */
/* Author:    Klaus Johannsen                                               */
/*            IWR/TS                                                        */
/*            Universitaet Heidelberg                                       */
/*            Im Neuenheiner Feld 368                                       */
/*            69120 Heidelberg                                              */
/*            email: ug@ica3.uni-stuttgart.de                               */
/*                                                                          */
/* History:   August 16, 2000                                               */
/*                                                                          */
/* Remarks:                                                                 */
/*                                                                          */
/****************************************************************************/



/****************************************************************************/
/*                                                                          */
/* auto include mechanism and other include files                           */
/*                                                                          */
/****************************************************************************/

#ifndef __REINIT__
#define __REINIT__

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

#define REINIT_CLASS_NAME "reinit"

/****************************************************************************/
/*                                                                                                                                                      */
/* definition of exported data structures                                                                       */
/*                                                                                                                                                      */
/****************************************************************************/

#define PARAMETER_MAX               10

struct reinit_result {

  INT parameter_not_found;              /* 1 if parameter not found         */
  INT parameter_out_of_range;           /* 1 if parameter out of range      */
  INT parameter_nochange;                   /* 1 if no change performed         */
};
typedef struct reinit_result REINIT_RESULT;

struct np_reinit {

  NP_BASE base;                              /* derived from nonlinear assemble */

  /* things to be initialized for being executable */
  INT n;                                     /* number of parameters            */
  char name[NS_PREFIX NAMESIZE][PARAMETER_MAX];        /* name of parameters              */
  DOUBLE parameter[PARAMETER_MAX];           /* parameters                      */

  /* functions */
  INT (*GetProblemParameter)
    (struct np_reinit *,                 /* pointer to (derived) object     */
    char *,                              /* name of parameter               */
    DOUBLE *);                                       /* parameter                               */
  INT (*ReinitProblem)
    (struct np_reinit *,                     /* pointer to (derived) object     */
    char *,                              /* name of parameters              */
    DOUBLE,                                      /* parameters                          */
    REINIT_RESULT *);                    /* result                          */
};
typedef struct np_reinit NP_REINIT;

/****************************************************************************/
/*                                                                                                                                                      */
/* definition of exported functions                                                                                     */
/*                                                                                                                                                      */
/****************************************************************************/

INT REINIT_Display (NP_BASE *base);
INT InitReinit (void);


END_UGDIM_NAMESPACE

#endif
