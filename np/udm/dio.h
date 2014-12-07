// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*																			*/
/* File:	  dio.h															*/
/*																			*/
/* Purpose:   header file for dio.c			                                                                */
/*																			*/
/* Author:	  Klaus Johannsen                                                                                               */
/*			  Institut fuer Computeranwendungen III                                                 */
/*			  Universitaet Stuttgart										*/
/*			  Pfaffenwaldring 27											*/
/*			  70550 Stuttgart												*/
/*			  email: ug@ica3.uni-stuttgart.de								*/
/*																			*/
/* History:   16.12.96 begin												*/
/*																			*/
/* Remarks:                                                                                                                             */
/*																			*/
/****************************************************************************/


/* RCS_ID
   $Header$
 */

/* switch */
#define __DIO_USE_IN_UG__

/****************************************************************************/
/*																			*/
/* auto include mechanism and other include files							*/
/*																			*/
/****************************************************************************/

#ifndef __DIO__
#define __DIO__

#include <cstdio>


/****************************************************************************/
/*																			*/
/* configuration of interface                                                                                           */
/*																			*/
/****************************************************************************/

#define DIO_VERSION                                     "DATA_IO_1.7"

#define __DTIO_USE_IN_UG__

/****************************************************************************/
/*																			*/
/* defines in the following order											*/
/*																			*/
/*		  compile time constants defining static data size (i.e. arrays)	*/
/*		  other constants													*/
/*		  macros															*/
/*																			*/
/****************************************************************************/

#ifdef __MGIO_USE_IN_UG__

        #include "gm.h"
        #define DIO_DIM                                         DIM

#else

        #define DIO_DIM                                         3

#endif

#define DIO_VDMAX                                               100
#define DIO_NAMELEN                                             128
#define DIO_IDENTLEN                                    4096

/* types of vector data */
#define DIO_SCALAR                                              0
#define DIO_VECTOR                                              1
#define DIO_MULTIPLE_SCALAR                             2

/****************************************************************************/
/*																			*/
/* data structures exported by the corresponding source file				*/
/*																			*/
/****************************************************************************/

struct dio_general {

  /* information about the file */
  int mode;                                                     /* macros see above						*/
  char version[DIO_NAMELEN];                    /* version of i/o						*/
  char mgfile[DIO_NAMELEN];                     /* corresponding multigrid file                 */
  int nparfiles;                                        /* number of processors					*/
  int me;                                                       /* id of my processor					*/
  double time;                                          /* time, -1.0 means no time specified!  */
  double dt;                                                    /* previous time-step                                   */
  double ndt;                                                   /* next time-step (suggestion)                  */
  int magic_cookie;                                     /* identification with mg-file			*/
  char ident[DIO_IDENTLEN];         /* identification string from input file*/

  /* information about data stored */
  int nVD;                                                                              /* nb of vector data				*/
  char VDname[DIO_VDMAX][DIO_NAMELEN];                  /* name of each vectordata desc		*/
  int VDncomp[DIO_VDMAX];                                               /* nb of comp of each vectordata	*/
  int VDtype[DIO_VDMAX];                                                /* types of vector data, see above	*/
  char VDcompNames[DIO_VDMAX][DIO_NAMELEN];             /* component names, used char-wise  */
  int ndata;                                                                            /* nb of doubles stored				*/
};

typedef struct dio_general DIO_GENERAL;

/****************************************************************************/
/*																			*/
/* definition of exported global variables									*/
/*																			*/
/****************************************************************************/

extern int datapathes_set;

/****************************************************************************/
/*																			*/
/* function declarations													*/
/*																			*/
/****************************************************************************/

/* read functions */
int             Read_OpenDTFile         (char *filename);
int             Read_DT_General         (DIO_GENERAL *dio_general);

/* misc functions */
#ifdef __DTIO_USE_IN_UG__
int             DTIO_dircreate          (char *filename, int rename);
int             DTIO_filetype           (char *filename);
#endif

/* write functions */
int             Write_OpenDTFile        (char *filename, int rename);
int             Write_DT_General        (DIO_GENERAL *dio_general);

/* general functions */
int     CloseDTFile                     (void);
int     DIO_Init                        (void);

#endif
