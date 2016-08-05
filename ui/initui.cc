// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*																			*/
/* File:	  initui.c														*/
/*																			*/
/* Purpose:   init file to init user interface								*/
/*																			*/
/* Author:	  Peter Bastian/Klaus Johannsen                                                                 */
/*			  Institut fuer Computeranwendungen III                                                 */
/*			  Universitaet Stuttgart										*/
/*			  Pfaffenwaldring 27											*/
/*			  70569 Stuttgart												*/
/*			  email: ug@ica3.uni-stuttgart.de				                        */
/*																			*/
/* History:   29.01.92 begin, ug version 2.0								*/
/*																			*/
/* Remarks:                                                                                                                             */
/*																			*/
/****************************************************************************/

/****************************************************************************/
/*																			*/
/* include files															*/
/*			  system include files											*/
/*			  application include files                                                                     */
/*																			*/
/****************************************************************************/

#include <config.h>

/* ANSI-C includes */
#include <cstdio>
#include <cstring>

/* low module */
#include "misc.h"
#include "defaults.h"
#include "general.h"

/* user interface module */
#include "cmdline.h"
#include "ugstruct.h"
#include "commands.h"

/* own header */
#include "initui.h"

USING_UG_NAMESPACES

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
/* data structures used in this source file (exported data structures are	*/
/*		  in the corresponding include file!)								*/
/*																			*/
/****************************************************************************/



/****************************************************************************/
/*																			*/
/* definition of exported global variables									*/
/*																			*/
/****************************************************************************/



/****************************************************************************/
/*																			*/
/* definition of variables global to this source file only (static!)		*/
/*																			*/
/****************************************************************************/


/****************************************************************************/
/*																			*/
/* forward declarations of functions used before they are defined			*/
/*																			*/
/****************************************************************************/


/****************************************************************************/
/*																			*/
/* Function:  InitUi														*/
/*																			*/
/* Purpose:   Init ui module												*/
/*																			*/
/* Input:	  int argc, char **argv: of main-call							*/
/*																			*/
/* Output: INT 0: ok														*/
/*			   1: init failed												*/
/*																			*/
/****************************************************************************/

INT NS_DIM_PREFIX InitUi (INT argc, char **argv)
{
  INT err;

  /* init cmdline */
  if ((err=InitCmdline())!=0)
  {
    SetHiWrd(err,__LINE__);
    return (err);
  }

  /* init commands of ug's command line interface */
  if ((err=InitCommands())!=0)
  {
    SetHiWrd(err,__LINE__);
    return (err);
  }

  /* return to application */
  return(0);
}


INT NS_DIM_PREFIX ExitUi()
{
  return 0;
}
