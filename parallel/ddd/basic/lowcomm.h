// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*                                                                          */
/* File:      lowcomm.h                                                     */
/*                                                                          */
/* Purpose:   lowlevel communication layer                                  */
/*                                                                          */
/* Author:    Klaus Birken                                                  */
/*            Institut fuer Computeranwendungen III                         */
/*            Universitaet Stuttgart                                        */
/*            Pfaffenwaldring 27                                            */
/*            70569 Stuttgart                                               */
/*            internet: birken@ica3.uni-stuttgart.de                        */
/*                                                                          */
/* History:   960715 kb  begin                                              */
/*                                                                          */
/* Remarks:                                                                 */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/*                                                                          */
/* auto include mechanism and other include files                           */
/*                                                                          */
/****************************************************************************/

#ifndef __DDD_LOWCOMM_H__
#define __DDD_LOWCOMM_H__



/****************************************************************************/
/*                                                                          */
/* defines in the following order                                           */
/*                                                                          */
/*        compile time constants defining static data size (i.e. arrays)    */
/*        other constants                                                   */
/*        macros                                                            */
/*                                                                          */
/****************************************************************************/



/****************************************************************************/
/*                                                                          */
/* data structures exported by the corresponding source file                */
/*                                                                          */
/****************************************************************************/


typedef unsigned long ULONG;


typedef void *LC_MSGHANDLE;  /* handle for actual message (send OR recv side*/
typedef void *LC_MSGTYPE;    /* type of message (on send AND recv side) */
typedef int LC_MSGCOMP;      /* component of message (dto) */



/****************************************************************************/
/*                                                                          */
/* function declarations                                                    */
/*                                                                          */
/****************************************************************************/


/* lowcomm.c */
void  LowCommInit (void);
void  LowCommExit (void);

LC_MSGTYPE LC_NewMsgType (char *);
LC_MSGCOMP LC_NewMsgTable (LC_MSGTYPE, size_t);
LC_MSGCOMP LC_NewMsgChunk (LC_MSGTYPE);

void       LC_MsgSend (LC_MSGHANDLE);

int           LC_Connect (LC_MSGTYPE);
LC_MSGHANDLE *LC_Communicate (void);
void          LC_Cleanup (void);

LC_MSGHANDLE LC_NewSendMsg (LC_MSGTYPE, DDD_PROC);
ULONG    LC_GetTableLen (LC_MSGHANDLE, LC_MSGCOMP);
void *   LC_GetPtr (LC_MSGHANDLE, LC_MSGCOMP);
DDD_PROC LC_MsgGetProc (LC_MSGHANDLE);
void     LC_MsgPrepareSend (LC_MSGHANDLE);
void     LC_SetTableLen (LC_MSGHANDLE, LC_MSGCOMP, ULONG);
void     LC_SetTableSize (LC_MSGHANDLE, LC_MSGCOMP, ULONG);
void     LC_SetChunkSize (LC_MSGHANDLE, LC_MSGCOMP, size_t);




#endif
