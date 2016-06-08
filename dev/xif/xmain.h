// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*                                                                                                                                                      */
/* File:          xmain.h                                                                                                               */
/*                                                                                                                                                      */
/* Purpose:   global variables exported by xmain                                                        */
/*                                                                                                                                                      */
/* Author:        Peter Bastian                                                                                                 */
/*                        Interdisziplinaeres Zentrum fuer Wissenschaftliches Rechnen   */
/*                        Universitaet Heidelberg                                                                               */
/*                        Im Neuenheimer Feld 368                                                                               */
/*                        6900 Heidelberg                                                                                               */
/*                        internet: ug@ica3.uni-stuttgart.de                                    */
/*                                                                                                                                                      */
/* History:   17.02.94 begin, ug version 3.0                                                            */
/*                                                                                                                                                      */
/* Remarks:                                                                                                                             */
/*                                                                                                                                                      */
/****************************************************************************/



/****************************************************************************/
/*                                                                                                                                                      */
/* auto include mechanism and other include files                                                       */
/*                                                                                                                                                      */
/****************************************************************************/

#ifndef __XMAIN__
#define __XMAIN__

#include "namespace.h"

START_UG_NAMESPACE

/****************************************************************************/
/*                                                                                                                                                      */
/* defines in the following order                                                                                       */
/*                                                                                                                                                      */
/*                compile time constants defining static data size (i.e. arrays)        */
/*                other constants                                                                                                       */
/*                macros                                                                                                                        */
/*                                                                                                                                                      */
/****************************************************************************/

#define XUI             0x1
#define CUI             0x2
#define GUI             0x4
#define NUI             0x8
#define XGUI            0x5
#define CGUI            0x6
#define CNUI            0xa
#define PUI             0x10

#define CUITOGGLE               0x2

#define XUI_STRING      "x"
#define CUI_STRING      "c"
#define GUI_STRING      "g"
#define NUI_STRING      "n"
#define XGUI_STRING     "xg"
#define CGUI_STRING     "cg"
#define CNUI_STRING     "cn"

#define CUI_ON          (user_interface & CUI)
#define XUI_ON          (user_interface & XUI)
#define GUI_ON          (user_interface & GUI)
#define NUI_ON          (user_interface & NUI)
#define PUI_ON          (user_interface & PUI)

#define SET_CUI_ON      (user_interface |= CUI)
#define SET_XUI_ON      (user_interface |= XUI)
#define SET_GUI_ON      (user_interface |= GUI)
#define SET_NUI_ON      (user_interface |= NUI)

#define SET_CUI_OFF     (user_interface &= ~CUI)
#define SET_XUI_OFF     (user_interface &= ~XUI)
#define SET_GUI_OFF     (user_interface &= ~GUI)
#define SET_NUI_OFF     (user_interface &= ~NUI)

#define TOGGLE_CUI      {int tmp=cui; cui=CUI_ON; \
                         if (tmp) SET_CUI_ON;else SET_CUI_OFF;}

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

extern Display *display;                                        /* the display                                  */
extern int screen_num;                                          /* screen on display                    */
extern char *prog_name;                                         /* my own name                                  */
extern Screen *screen_ptr;                                      /* dont know for what                   */
extern unsigned int display_width;                      /* size of screen if needed     */
extern unsigned int display_height;
extern int if_argc;                                             /* command line args                    */
extern char **if_argv;
extern int user_interface;                                      /* user interface to open       */
extern int cui;                                                         /* toggle for cui               */


/****************************************************************************/
/*                                                                                                                                                      */
/* function declarations                                                                                                        */
/*                                                                                                                                                      */
/****************************************************************************/

END_UG_NAMESPACE

#endif
