// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*																			*/
/* File:	  general.h														*/
/*																			*/
/* Purpose:   general header file                                                                       */
/*																			*/
/* Author:	  Stefan Lang                                                                           */
/*			  Institut fuer Computeranwendungen III                                                 */
/*			  Universitaet Stuttgart										*/
/*			  Pfaffenwaldring 27											*/
/*			  70550 Stuttgart												*/
/*			  email: ug@ica3.uni-stuttgart.de								*/
/*																			*/
/* History:   24.06.96 start: UG_VERSION string, RCSID macro                */
/*																			*/
/* Remarks:                                                                                                                             */
/*																			*/
/****************************************************************************/



/****************************************************************************/
/*																			*/
/* auto include mechanism and other include files							*/
/*																			*/
/****************************************************************************/

#ifndef __GENERAL__
#define __GENERAL__


/****************************************************************************/
/*																			*/
/* defines in the following order											*/
/*																			*/
/*		  compile time constants defining static data size (i.e. arrays)	*/
/*		  other constants													*/
/*		  macros															*/
/*																			*/
/****************************************************************************/

#define UG_VERSION "ug 3.8"

#define UG_RCS_STRING UGRCSSTRING(UG_VERSION,ARCH_VERSION,DIM,MODEL,GRAPE_SUPPORT,NETGEN_SUPPORT,DOM_MODULE,DEBUG_MODE)

#define UGRCSSTRING(ug_version,arch_version,dim,model,grape,netgen,dom_module,debug_mode)\
  UGRCSSTRINGAUX(ug_version,arch_version,dim,model,grape,netgen,dom_module,debug_mode)

#define UGRCSSTRINGAUX(ug_version,arch_version,dim,model,grape,netgen,dom_module,debug_mode)\
  "$" "State: UG_VERSION=" # ug_version " ARCH_VERSION=" # arch_version " DIM=" # dim\
  " GRAPE_SUPPORT=" # grape " MODEL=" # model\
  " NETGEN_SUPPORT=" # netgen " DOM_MODULE=" # dom_module\
  " DEBUG_MODE=" # debug_mode " $"

#define RCSID(header,module_rcs_string) RCSIDAUX(header,module_rcs_string)
#define RCSIDAUX(header,module_rcs_string) static char rcsid[] = header module_rcs_string;

#define RCSID1(header,module_rcs_string) RCSIDAUX1(header,module_rcs_string)
#define RCSIDAUX1(header,module_rcs_string) static char rcsid1[] = header module_rcs_string;


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

#endif
