// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/** \defgroup graphics Graphical User Interface
 * \ingroup ug
 */
/*! \file graphics.h
 * \ingroup graphics
 */

/****************************************************************************/
/*                                                                          */
/* File:      graphics.h                                                    */
/*                                                                          */
/* Purpose:   implements a simple but portable graphical user interface     */
/*                                                                          */
/* Author:    Stefan Lang, Klaus Birken                                     */
/*            Institut fuer Computeranwendungen III                         */
/*            Universitaet Stuttgart                                        */
/*            Pfaffenwaldring 27                                            */
/*            70569 Stuttgart                                               */
/*            email: ug@ica3.uni-stuttgart.de                               */
/*                                                                          */
/* History:   971216 begin                                                  */
/*                                                                          */
/* Remarks:                                                                 */
/*                                                                          */
/****************************************************************************/




/****************************************************************************/
/*                                                                                                                                                      */
/* auto include mechanism and other include files                                                       */
/*                                                                                                                                                      */
/****************************************************************************/

#ifndef __GRAPHICS_H__
#define __GRAPHICS_H__

#include "ugtypes.h"

#include "namespace.h"

START_UGDIM_NAMESPACE

/****************************************************************************/
/*                                                                          */
/*      constants                                                           */
/*                                                                          */
/****************************************************************************/


/****************************************************************************/
/*                                                                          */
/*      macros                                                                                                                                  */
/*                                                                                                                                                      */
/****************************************************************************/


/****************************************************************************/
/*                                                                                                                                                      */
/*      data types                                                                                                                              */
/*                                                                                                                                                      */
/****************************************************************************/



/****************************************************************************/
/*                                                                                                                                                      */
/* function exported by this module                                                                     */
/*                                                                                                                                                      */
/****************************************************************************/

/* initialization and clean up */
INT               InitGraphics                          (void);
INT           ExitGraphics              (void);

END_UGDIM_NAMESPACE

#endif
