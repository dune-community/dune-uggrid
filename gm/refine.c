// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*																			*/
/* File:	  refine.c														*/
/*																			*/
/* Purpose:   unstructured grid refinement using a general element concept	*/
/*			  (dimension independent for 2/3D)								*/
/*																			*/
/* Author:	  Stefan Lang                                                                           */
/*			  Institut fuer Computeranwendungen III                                                 */
/*			  Universitaet Stuttgart										*/
/*			  Pfaffenwaldring 27											*/
/*			  70569 Stuttgart												*/
/*			  email: ug@ica3.uni-stuttgart.de								*/
/*																			*/
/* History:   08.08.95 begin serial algorithm, ug version 3.0				*/
/*																			*/
/* Remarks:   - level 0 grid consists of red elements only					*/
/*			  - the only restriction in the element hierarchie is that              */
/*				green or yellow elements might not have sons of class           */
/*				green or red												*/
/*			  - the rule set for refinement consists of regular (red)		*/
/*				and irregular rules; regular rules create red elements		*/
/*				while irregular rules result in green elements				*/
/*				( green elements are needed for the closure of the grid,	*/
/*				yellow elements, which are from copy rules, save			*/
/*                              the numerical properties of the solver and are handsome for */
/*				the discretisation											*/
/*			  - if the rule set for the red rules is not complete for build-*/
/*				up a consistent red refined region the FIFO might be used       */
/*				for some (hopefully not too much) iterations to find a          */
/*				consistent one												*/
/*			  - in 2D: exists a complete rule set for grids of triangles    */
/*					   and quadrilaterals exclusivly						*/
/*			  - in 3D: exists a complete rule set for tetrahedrons			*/
/*					   and we assume after some analysation a complete set  */
/*					   of rules described by an algorithm for hexahedrons	*/
/*			  - for mixed element types in arbitrary dimension no               */
/*				rule set for the closure exists								*/
/*			  - BEFORE refinement we assume a situation where the error		*/
/*				estimator has detected and marked the leaf elements for         */
/*				further refinement											*/
/*			  - AFTER refinement all elements are refined by a rule in way	*/
/*				that no hanging nodes remain (this is the default mode)		*/
/*				or with hanging nodes (in the hanging node mode)			*/
/*				if you use inconsistent red refinement, you need to tell        */
/*                              the algorithm explicitly to use the FIFO					*/
/*																			*/
/****************************************************************************/

/****************************************************************************/
/*																			*/
/* include files															*/
/*			  system include files											*/
/*			  application include files                                                                     */
/*																			*/
/****************************************************************************/

/* standard C library */
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

/* low module */
#include "compiler.h"
#include "debug.h"
#include "heaps.h"
#include "misc.h"
#include "general.h"

/* dev module */
#include "devices.h"

/* gm module */
#include "algebra.h"
#include "evm.h"
#include "gm.h"
#include "GenerateRules.h"
#include "refine.h"
#include "rm.h"
#include "switch.h"
#include "ugm.h"

/* paralllel modules */
#ifdef ModelP
#include "ppif.h"
#include "ddd.h"
#include "parallel.h"
#include "identify.h"
#endif

/****************************************************************************/
/*																			*/
/* defines in the following order											*/
/*																			*/
/*		  compile time constants defining static data size (i.e. arrays)	*/
/*		  other constants													*/
/*		  macros															*/
/*																			*/
/****************************************************************************/

#define MINVNCLASS      2           /* determines copies, dep. on discr. !  */

/* defines for side matching of elements 8 bits:                            */
/* _ _ _ _ (4 bits for corner of one element) _ _ _ _ (4 bits for the other)*/

#define LINEPOINTS              51                      /* 0011 0011 */
#define TRIPOINTS               119                     /* 0111 0111 */
#define QUADPOINTS              255                     /* 1111 1111 */

#define MAX_GREEN_SONS          32              /* maximal number of sons for green refinement */

/* TODO: delete next line */
/* #define GET_PATTERN(e)				((SIDEPATTERN(e)<<EDGEPATTERN_LEN) | EDGEPATTERN(e)) */
#define EDGE_IN_PATTERN(p,i)            (((p)[(i)]) & 0x1)
#define SIDE_IN_PATTERN(e,p,i)          (((p)[EDGES_OF_ELEM(e)+(i)]) & 0x1)
#define EDGE_IN_PAT(p,i)                        (((p)>>(i)) & 0x1)
#define SIDE_IN_PAT(p,i)                        (((p)>>(i)) & 0x1)

#define MARK_BISECT_EDGE(r,i)           (r->pattern[i]==1)

#define REF_TYPE_CHANGES(e)             ((REFINE(e)!=MARK(e)) || (REFINECLASS(e)!=MARKCLASS(e)))
#define IS_TO_REFINE(e)                 (MARK(theElement)!=NO_REFINEMENT)

#define NEWGREEN(e)                             (TAG(e)==HEXAHEDRON || TAG(e)== PRISM || TAG(e)==PYRAMID)

/* TODO: delete special debug */
static ELEMENT *debugelem=NULL;
/*
   #define PRINTELEMID(id) \
                if (ID(theElement)==id && id!=10120) { \
                        debugelem=theElement; \
                        UserWriteF("refine.c:line=%d\n",__LINE__);\
                        ListElement(NULL,theElement,0,0,1,0); \
                } \
                else if ((id==-1 || ID(theElement)==10120) && debugelem!=NULL) {\
                        UserWriteF("refine.c:line=%d\n",__LINE__); \
                        ListElement(NULL,debugelem,0,0,1,0); \
                } \
                else if (id==-2 && debugelem!=NULL) {\
                        if (ID(theElement)==8899) {\
                                UserWriteF("refine.c:line=%d\n",__LINE__); \
                                UserWriteF("ERRORID=%d\n",ID(theElement)); \
                                ListElement(NULL,debugelem,0,0,1,0); \
                        }\
                }
 */

#define PRINTELEMID(id)

#define REFINE_ELEMENT_LIST(d,e,s) \
  IFDEBUG(gm,d) \
  if (e!=NULL) \
    UserWriteF( s " ID=%d TAG=%d BE=%d ECLASS=%d REFINECLASS=%d" \
                " MARKCLASS=%d REFINE=%d MARK=%d COARSE=%d" \
                " USED=%d NSONS=%d\n", ID(e),\
                TAG(e),(OBJT(e)==BEOBJ),ECLASS(e),REFINECLASS(e),MARKCLASS(e), \
                REFINE(e),MARK(e),COARSEN(e), \
                USED(e),NSONS(e)); \
  ENDDEBUG

/****************************************************************************/
/*																			*/
/* data structures used in this source file (exported data structures are	*/
/*		  in the corresponding include file!)								*/
/*																			*/
/****************************************************************************/

typedef NODE *ELEMENTCONTEXT[MAX_CORNERS_OF_ELEM+MAX_NEW_CORNERS_DIM];


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

static int rFlag=GM_REFINE_TRULY_LOCAL; /* type of refine                   */
static int hFlag=0;                                             /* refine with hanging nodes?		*/
static int fifoFlag=0;                                  /* use fifo? 0=no 1=yes				*/
static int first;                                               /* fifo loop counter                            */
static ELEMENT *fifo_first=NULL;                /* first element in fifo work list	*/
static ELEMENT *fifo_last=NULL;                 /* last element in fifo	work list	*/
static ELEMENT *fifo_insertfirst=NULL;  /* first element in fifo insertlist */
static ELEMENT *fifo_insertlast=NULL;   /* last element in fifo insertlist	*/
static INT No_Green_Update;                             /* counter for green refinements	*/
/* need not to be updated			*/
static INT Green_Marks;                                 /* green refined element counter	*/

/* determine number of edge from reduced (i.e. restricted to one side) edgepattern */
/* if there are two edges marked for bisection, if not deliver -1. If the edge-    */
/* is not reduced (i.e. marked edges lying on more than one side) deliver -2       */
static INT TriSectionEdge[64][2] = {  {-1,-1},{-1,-1},{-1,-1},{ 1, 0},{-1,-1},{ 0, 2},{ 2, 1},{-1,-1},
                                      {-1,-1},{ 3, 0},{-2,-2},{-2,-2},{ 2, 3},{-2,-2},{-2,-2},{-2,-2},
                                      {-1,-1},{ 0, 4},{ 4, 1},{-2,-2},{-2,-2},{-2,-2},{-2,-2},{-2,-2},
                                      { 4, 3},{-1,-1},{-2,-2},{-2,-2},{-2,-2},{-2,-2},{-2,-2},{-2,-2},
                                      {-1,-1},{-2,-2},{ 1, 5},{-2,-2},{ 5, 2},{-2,-2},{-2,-2},{-2,-2},
                                      { 3, 5},{-2,-2},{-2,-2},{-2,-2},{-1,-1},{-2,-2},{-2,-2},{-2,-2},
                                      { 5, 4},{-2,-2},{-1,-1},{-2,-2},{-2,-2},{-2,-2},{-2,-2},{-2,-2},
                                      {-2,-2},{-2,-2},{-2,-2},{-2,-2},{-2,-2},{-2,-2},{-2,-2},{-2,-2}  };

/* the indices of the edges of each side */
static INT CondensedEdgeOfSide[4] = {0x07,0x32,0x2C,0x19};

/* ptr to Get_Sons_of_ElementSideProc */
static Get_Sons_of_ElementSideProcPtr Get_Sons_of_ElementSideProc;

/* RCS string */
static char RCS_ID("$Header$",UG_RCS_STRING);

/****************************************************************************/
/*																			*/
/* forward declarations of functions used before they are defined			*/
/*																			*/
/****************************************************************************/

#ifdef ModelPTest
/****************************************************************************/
/*																			*/
/* Function:  MakeRefMarkandMarkClassConsistent								*/
/*																			*/
/* Purpose:	  exchange the MARK and MARKCLASS flags between elements on     */
/*			  the vertical boundary of one level.							*/
/*																			*/
/* Input:	  int level: level for which to make flags consistent                   */
/*																			*/
/* Output:	  void                                                                                                                  */
/*																			*/
/****************************************************************************/

int GetMarkandMarkClass (OBJECT obj, void *data)
{}

int PutMarkandMarkClass (OBJECT obj, void *data)
{}

void MakeRefMarkandMarkClassConsistent (int level)
{
  INTERFACE id;

  /* get id for vertical interface downwards */
  id = 1;
  /* exchange marks */
  DDD_IFExchange(id,INT,GetMarkandMarkClass, PutMarkandMarkClass);
}
#endif


/****************************************************************************/
/*																			*/
/* Function:  DropMarks                                                                                                         */
/*																			*/
/* Purpose:   drop marks from leafelements to first regular, and reset		*/
/*			  marks on all elements above (important for restrict marks)    */
/*																			*/
/* Param:	  MULTIGRID *theMG												*/
/*																			*/
/* return:	  INT GM_OK: ok                                                                                                 */
/*			  INT GM_ERROR: error											*/
/*																			*/
/****************************************************************************/

static INT DropMarks (MULTIGRID *theMG)
{
  INT k, Mark;
  GRID *theGrid;
  ELEMENT *theElement, *FatherElement;

  return(GM_OK);

  for (k=TOPLEVEL(theMG); k>0; k--)
  {
    theGrid = GRID_ON_LEVEL(theMG,k);
    for (theElement=FIRSTELEMENT(theGrid); theElement!=NULL; theElement=SUCCE(theElement))
      if ((MARKCLASS(theElement) == RED_CLASS) && (ECLASS(theElement) != RED_CLASS))
      {
        Mark = MARK(theElement);
        /* TODO: marks must be changed if element type changes */
        if (TAG(theElement)!=HEXAHEDRON && TAG(EFATHER(theElement))==HEXAHEDRON) Mark = HEXA_RED;
        if (TAG(theElement)!=PYRAMID && TAG(EFATHER(theElement))==PYRAMID) Mark = PYR_RED;
        FatherElement = theElement;

        SETMARK(FatherElement,NO_REFINEMENT);
        SETMARKCLASS(FatherElement,NO_CLASS);
        FatherElement = EFATHER(FatherElement);

        SETMARK(FatherElement,Mark);
        SETMARKCLASS(FatherElement,RED_CLASS);

                                #ifdef ModelPTest
        MakeRefMarkandMarkClassConsistent(k);
                                #endif
      }
  }
  return(GM_OK);
}

#ifdef ModelPTest
/****************************************************************************/
/*																			*/
/* Function:  ExchangePatternOfMasterAndSlaves                                                          */
/*																			*/
/* Purpose:	  exchange the PATTERN between elements on horizontal           */
/*			  boundary of one level.										*/
/*																			*/
/* Input:	  int level: level for which to make flags consistent                   */
/*																			*/
/* Output:	  void                                                                                                                  */
/*																			*/
/****************************************************************************/

int GetEdgePatternOfElement (OBJECT obj, void *data)
{}

int PutEdgePatternOfElement (OBJECT obj, void *data)
{}

/* TODO: perhaps it is better to exchange the PATTERN to check that they are */
/*       consistent then use the name ExchangePatternOfMasterToSlaves        */
void SendPatternFromMasterToSlaves(int level)
{
  int id;

  /* get interface id for horizontal interface */
  id = 1;
  DDD_IFExchange(id,INT,GetEdgePatternOfElement, PutEdgePatternOfElement);
}
#endif


/****************************************************************************/
/*																			*/
/* Function:  CloseGrid                                                                                                         */
/*																			*/
/* Purpose:   compute closure for next level. A closure can only be         */
/*			  determined if the rule set for the used elements is complete. */
/*                        This means that for all side and edge patterns possible for   */
/*			  an element type exists a rule which closes the element.       */
/*	          In this case a FIFO for computing the closure is not needed   */
/*	          any more and the closure can be computed in one step.			*/
/*																			*/
/* Param:	  GRID *theGrid: pointer to grid structure						*/
/*																			*/
/* return:	  INT >0: elements will be refined								*/
/*			  INT 0: no elements will be refined							*/
/*			  INT -1: an error occured                                                              */
/*																			*/
/****************************************************************************/

static int CloseGrid (GRID *theGrid)
{
  ELEMENT *theElement, *NbElement, *firstElement;
  EDGE *MyEdge, *NbEdge;
  INT i,j,k,cnt,n;
  INT Mark, MyEdgePattern, MySidePattern, MyEdgeNum, MyPattern, NewPattern;
  INT NbEdgePattern, NbSidePattern, NbEdgeNum, NbSideMask;
  SHORT *myPattern;

  /* reset USED flag of elements and PATTERN and ADDPATTERN flag on the edges */
  for (theElement=FIRSTELEMENT(theGrid); theElement!=NULL; theElement=SUCCE(theElement))
  {
    /* TODO: delete special debug */ PRINTELEMID(11668)

    SETUSED(theElement,0);
    for (j=0; j<EDGES_OF_ELEM(theElement); j++)
    {
      MyEdge=GetEdge(CORNER(theElement,CORNER_OF_EDGE(theElement,j,0)),CORNER(theElement,CORNER_OF_EDGE(theElement,j,1)));
      ASSERT(MyEdge != NULL);
      SETPATTERN(MyEdge,0);
      /* This is needed in RestrictMarks() */
      SETADDPATTERN(MyEdge,1);
    }
  }

  /* reset EDGE/SIDEPATTERN in elements, set SIDEPATTERN in elements for quadrilateral sides and set PATTERN on the edges */
  for (theElement=FIRSTELEMENT(theGrid); theElement!=NULL; theElement=SUCCE(theElement))
  {
    /* TODO: delete special debug */ PRINTELEMID(11668)

    if (MARKCLASS(theElement)==RED_CLASS)
    {
      Mark = MARK(theElement);
      myPattern = MARK2PATTERN(theElement,Mark);

      for (i=0; i<EDGES_OF_ELEM(theElement); i++)
        /* TODO: delete this (old code) */
        /* if (myPattern[i]) */
        if (EDGE_IN_PATTERN(myPattern,i))
        {
          MyEdge=GetEdge(CORNER(theElement,CORNER_OF_EDGE(theElement,i,0)),CORNER(theElement,CORNER_OF_EDGE(theElement,i,1)));
          if (MyEdge != NULL)
            SETPATTERN(MyEdge,1);
          else
            UserWriteF("CloseGrid(): ERROR edge i=%d of element e=%x not found!",i,theElement);
        }

      SETSIDEPATTERN(theElement,0);

      /* for 2D we are finished */
      if (DIM == 2) continue;

      for (i=0; i<SIDES_OF_ELEM(theElement); i++)
      {
        if (CORNERS_OF_SIDE(theElement,i)==4)
        {
          /* set SIDEPATTERN if side has node */
          if(SIDE_IN_PATTERN(theElement,myPattern,i))
            SETSIDEPATTERN(theElement,SIDEPATTERN(theElement) | 1<<i);
        }
      }
    }
    else
    {
      SETSIDEPATTERN(theElement,0);
      SETMARKCLASS(theElement,NO_CLASS);
    }
    /* TODO: delete this */
    /* SETEDGEPATTERN(theElement,0);*/
  }

  firstElement = FIRSTELEMENT(theGrid);

  if (fifoFlag) {
    fifo_first=fifo_last=fifo_insertfirst=fifo_insertlast=NULL;
    first = 1;
    n = 0;
    UserWriteF("Using FIFO: loop 0\n");
  }

  /* here starts the fifo loop */
FIFOSTART:

  /* set pattern (edge and side) on the elements */
  for (theElement=firstElement; theElement!=NULL; theElement=SUCCE(theElement))
  {
    /* TODO: delete special debug */ PRINTELEMID(11668)

    /* make edgepattern consistent with pattern of edges */
    SETUSED(theElement,1);
    MyEdgePattern = 0;
    for (i=EDGES_OF_ELEM(theElement)-1; i>=0; i--)
    {
      MyEdge=GetEdge(CORNER(theElement,CORNER_OF_EDGE(theElement,i,0)),CORNER(theElement,CORNER_OF_EDGE(theElement,i,1)));
      MyEdgePattern = (MyEdgePattern<<1) | PATTERN(MyEdge);
    }

    /* for 2D we are finished */
    if (DIM == 2) continue;

    /* TODO: change this for red refinement of pyramids */
    if (DIM==3 && TAG(theElement)==PYRAMID) continue;

    /* make SIDEPATTERN consistent with neighbors	*/
    /* TODO: compute this in a separate function?	*/
    for (i=0; i<SIDES_OF_ELEM(theElement); i++)
    {
      NbElement = NBELEM(theElement,i);
      if (NbElement == NULL) continue;
      /* TODO: is this or only USED ok? */
      if (!USED(NbElement)) continue;
      /* now edgepattern from theElement and NbElement are in final state */

      /* search neighbors side */
      for (j=0; j<SIDES_OF_ELEM(NbElement); j++)
        if (NBELEM(NbElement,j) == theElement)
          break;
      ASSERT(j<SIDES_OF_ELEM(NbElement));

      /* side is triangle or quadrilateral */
      switch (CORNERS_OF_SIDE(theElement,i))
      {
      case 3 :
        if (TAG(theElement) == PYRAMID ||
            TAG(theElement) == PRISM )
          continue;

        /* because SIDEPATTERN is set to zero, I choose TriSectionEdge��[0] */
        MyEdgeNum = TriSectionEdge[MyEdgePattern&CondensedEdgeOfSide[i]][0];
        if (MyEdgeNum == -2)
          RETURN(-1);

        if (MyEdgeNum == -1) continue;

        switch (TAG(NbElement)) {
        case TETRAHEDRON :
          NbEdgePattern = 0;
          for (k=0; k<EDGES_OF_ELEM(NbElement); k++) {
            NbEdge=GetEdge(CORNER(NbElement,CORNER_OF_EDGE(NbElement,k,0)),CORNER(NbElement,CORNER_OF_EDGE(NbElement,k,1)));
            ASSERT(NbEdge!=NULL);
            NbEdgePattern = NbEdgePattern | (PATTERN(NbEdge)<<k);
          }

          NbEdgeNum = TriSectionEdge[NbEdgePattern&CondensedEdgeOfSide[j]][0];
          if (NbEdgeNum == -2 || NbEdgeNum == -1)
            RETURN(-1);

          if (!( CORNER(theElement,CORNER_OF_EDGE(theElement,MyEdgeNum,0)) ==
                 CORNER(NbElement,CORNER_OF_EDGE(NbElement,NbEdgeNum,0)) &&
                 CORNER(theElement,CORNER_OF_EDGE(theElement,MyEdgeNum,1)) ==
                 CORNER(NbElement,CORNER_OF_EDGE(NbElement,NbEdgeNum,1))      ) &&
              !( CORNER(theElement,CORNER_OF_EDGE(theElement,MyEdgeNum,0)) ==
                 CORNER(NbElement,CORNER_OF_EDGE(NbElement,NbEdgeNum,1)) &&
                 CORNER(theElement,CORNER_OF_EDGE(theElement,MyEdgeNum,1)) ==
                 CORNER(NbElement,CORNER_OF_EDGE(NbElement,NbEdgeNum,0))      )        )
          {
            NbSidePattern = SIDEPATTERN(NbElement);
            NbSideMask = (1<<j);
            if ( NbSidePattern & NbSideMask )
              NbSidePattern &= ~NbSideMask;
            else
              NbSidePattern |= NbSideMask;
            SETSIDEPATTERN(NbElement,NbSidePattern);
          }
          break;
        case PYRAMID :
        case PRISM :
        {
          NODE *edgenode=NULL;
          INT trisectionedge=-1;

          for (k=0; k<CORNERS_OF_SIDE(NbElement,j); k++) {
            INT edge;

            edge = EDGE_OF_SIDE(theElement,j,k);
            NbEdge=GetEdge(CORNER(NbElement,CORNER_OF_EDGE(NbElement,edge,0)),CORNER(NbElement,CORNER_OF_EDGE(NbElement,edge,1)));
            ASSERT(NbEdge!=NULL);

            if (PATTERN(NbEdge) && (edge>trisectionedge))
              trisectionedge = edge;
          }
          assert(trisectionedge != -1);

          if (MyEdgeNum != trisectionedge) {
            SETSIDEPATTERN(NbElement,SIDEPATTERN(NbElement)|(1<<j));
          }

          break;
        }
        default :
          ASSERT(0);
          break;
        }
        break;

      case 4 :
        /* if side of one of the neighboring elements has a sidenode, then both need a sidenode */
        NbSidePattern = SIDEPATTERN(NbElement);
        if (SIDE_IN_PAT(SIDEPATTERN(theElement),i))
        {
          SETSIDEPATTERN(NbElement,SIDEPATTERN(NbElement) | (1<<j));
        }
        else if (SIDE_IN_PAT(SIDEPATTERN(NbElement),j))
        {
          SETSIDEPATTERN(theElement,SIDEPATTERN(theElement) | (1<<i));
        }
        break;
      default :
        UserWriteF("CloseGrid(): ERROR: CORNER_OF_SIDE(e=%x,s=%d)=%d !\n",theElement,i,CORNERS_OF_SIDE(theElement,i));
        RETURN(-1);
      }
    }
  }

        #ifdef ModelPTest
  /* send the PATTERN flag from master to slave elements */
  SendPatternFromMasterToSlaves(GLEVEL(theGrid));
        #endif

  /* set refinement rules from edge- and sidepattern */
  cnt = 0;
  for (theElement=firstElement; theElement!=NULL; theElement=SUCCE(theElement))
  {
    /* TODO: delete special debug */ PRINTELEMID(11668)

    MyEdgePattern = 0;
    for (i=EDGES_OF_ELEM(theElement)-1; i>=0; i--)
    {
      MyEdge=GetEdge(CORNER(theElement,CORNER_OF_EDGE(theElement,i,0)),CORNER(theElement,CORNER_OF_EDGE(theElement,i,1)));
      MyEdgePattern = (MyEdgePattern<<1) | PATTERN(MyEdge);
    }
    MySidePattern = SIDEPATTERN(theElement);
    MyPattern = MySidePattern<<EDGES_OF_ELEM(theElement) | MyEdgePattern;

    Mark = PATTERN2MARK(theElement,MyPattern);

    if (fifoFlag) {
      if (Mark == -1 && MARKCLASS(theElement)==RED_CLASS) {
        /* there is no rule for this pattern, switch to red */
        Mark = RED;
      }
      else
        ASSERT(Mark != -1);
    }
    else if (hFlag==0 && MARKCLASS(theElement)!=RED_CLASS) {
      Mark = NO_REFINEMENT;
    }
    else {
      ASSERT(Mark != -1);

      /* switch green class red class? */
      if (MARKCLASS(theElement)!=RED_CLASS && SWITCHCLASS(CLASS_OF_RULE(MARK2RULEADR(theElement,Mark)))) {
        IFDEBUG(gm,1)
        UserWriteF("   Switching MARKCLASS=%d for MARK=%d of EID=%d to RED_CLASS\n",
                   MARKCLASS(theElement),Mark,ID(theElement));
        ENDDEBUG
        SETMARKCLASS(theElement,RED_CLASS);
      }
    }

    IFDEBUG(gm,1)
    UserWriteF("    ID=%d TAG=%d MyPattern=%d Mark=%d MyEdgePattern=%d MySidePattern=%d\n",ID(theElement),TAG(theElement),MyPattern,Mark,MyEdgePattern,MySidePattern);
    ENDDEBUG

                #ifdef __THREEDIM__
    if (TAG(theElement)==TETRAHEDRON && MARKCLASS(theElement) == RED_CLASS) {
      Mark = (*theFullRefRule)(theElement);
      assert( Mark==FULL_REFRULE_0_5 ||
              Mark==FULL_REFRULE_1_3 ||
              Mark==FULL_REFRULE_2_4);
    }
                #endif

    NewPattern = MARK2PAT(theElement,Mark);
    IFDEBUG(gm,1)
    UserWriteF("   MyPattern=%d NewPattern=%d Mark=%d\n",MyPattern,NewPattern,Mark);
    ENDDEBUG


    if (fifoFlag) {
      if (MARKCLASS(theElement)==RED_CLASS && MyPattern != NewPattern) {
                                #ifdef __TWODIM__
        for (j=0; j<EDGES_OF_ELEM(theElement); j++) {
          if (EDGE_IN_PAT(MyPattern,j)==0 && EDGE_IN_PAT(NewPattern,j)) {
            MyEdge=GetEdge(CORNER(theElement,CORNER_OF_EDGE(theElement,j,0)),CORNER(theElement,CORNER_OF_EDGE(theElement,j,1)));
            if (MyEdge != NULL)
              SETPATTERN(MyEdge,1);
            else
              UserWriteF("CloseGrid(): ERROR edge i=%d of element e=%x not found!",i,theElement);

            /* boundary case */
            if (SIDE_ON_BND(theElement,j)) continue;

            /* add the element sharing this edge to fifo_queue */
            NbElement = NBELEM(theElement,j);

            if (NbElement==NULL) continue;

            PRINTDEBUG(gm,1,("   ADDING to FIFO: NBID=%d\n",ID(NbElement)))

            /* unlink element from element list */
            if (PREDE(NbElement) != NULL)
              SUCCE(PREDE(NbElement)) = SUCCE(NbElement);
            if (SUCCE(NbElement) != NULL)
              PREDE(SUCCE(NbElement)) = PREDE(NbElement);
            if (FIRSTELEMENT(theGrid) == NbElement) FIRSTELEMENT(theGrid) = SUCCE(NbElement);

            SUCCE(NbElement) = PREDE(NbElement) = NULL;
            /* insert into fifo */
            if (fifo_insertfirst == NULL) {
              fifo_insertfirst = fifo_insertlast = NbElement;
            }
            else {
              SUCCE(fifo_insertlast) = NbElement;
              PREDE(NbElement) = fifo_insertlast;
              fifo_insertlast = NbElement;
            }
          }
          if (EDGE_IN_PAT(MyPattern,j) && EDGE_IN_PAT(NewPattern,j)==0) {
            UserWriteF("CloseGrid(): ERROR EID=%d in fifo MyPattern=%d has edge=%d refined but NewPattern=%d NOT!\n",ID(theElement),MyPattern,j,NewPattern);
            RETURN(-1);
          }
        }
                                #endif
                                #ifdef __THREEDIM__
        UserWriteF("CloseGrid(): ERROR fifo for 3D NOT implemented!\n");
        RETURN(-1);
                                #endif
      }
    }

    if (Mark)
      cnt++;
    SETMARK(theElement,Mark);
  }

  if (fifoFlag) {
    /* insert fifo work list into elementlist */
    for (theElement=fifo_last; theElement!=NULL; theElement=PREDE(theElement)) {
      SUCCE(theElement) = FIRSTELEMENT(theGrid);
      PREDE(FIRSTELEMENT(theGrid)) = theElement;
      FIRSTELEMENT(theGrid) = theElement;
    }
    PREDE(FIRSTELEMENT(theGrid)) = NULL;

    if (fifo_insertfirst!=NULL) {
      /* append fifo insert list to fifo work list */
      firstElement = fifo_first = fifo_insertfirst;
      fifo_last = fifo_insertlast;

      IFDEBUG(gm,2)
      UserWriteF(" FIFO Queue:");
      for (theElement=fifo_first; theElement!=NULL; theElement=SUCCE(theElement))
        UserWriteF(" %d\n", ID(theElement));
      ENDDEBUG

        fifo_insertfirst = fifo_insertlast = NULL;
      first = 0;
      n++;
      UserWriteF(" loop %d",n);
      goto FIFOSTART;
    }
  }


        #ifdef FIFO
        #ifdef ModelP
  do {
    /* exchange FIFO flag and PATTERN from slaves to master */
    IF_FIF0AndPat_S2M(GLEVEL(grid));

    /* add all master elements of horizontal interface to FIFO */
    for (theElement=firstElement; theElement!=NULL; theElement=SUCCE(theElement)) {
      if (IS_HOR_MASTER(theElement) && FIFO(theElement)) {
        AddToFIFIO(theElement);
        SETFIFO(theElement,0);
      }
    }

    /* check condition for termination of pattern adaptation */
    AllFIFOsEmpty = CheckGlobalFIFOStatus(fifo);
  }
  while (fifo==NULL && AllFIFOsEmpty==1)
        #endif /* ModelP */
}         /* end if (fifoFlag) */
        #endif


  /* set additional pattern on the edges */
  for (theElement=FIRSTELEMENT(theGrid); theElement!=NULL; theElement=SUCCE(theElement))
  {
    /* TODO: delete special debug */ PRINTELEMID(11668)

    if (MARKCLASS(theElement)!=RED_CLASS) continue;
    for (j=0; j<EDGES_OF_ELEM(theElement); j++)
    {
      /* no green elements for this edge if there is no edge node */
      if (!NODE_OF_RULE(theElement,MARK(theElement),j))
        continue;

      MyEdge=GetEdge(CORNER(theElement,CORNER_OF_EDGE(theElement,j,0)),CORNER(theElement,CORNER_OF_EDGE(theElement,j,1)));
      ASSERT(MyEdge != NULL);

      /* ADDPATTERN is now set to 0 for all edges of red elements */
      SETADDPATTERN(MyEdge,0);
    }
  }

  /* build a green covering around the red elements */
  for (theElement=FIRSTELEMENT(theGrid); theElement!=NULL; theElement=SUCCE(theElement))
  {
    /* TODO: delete special debug */ PRINTELEMID(11668)

    if (MARKCLASS(theElement)==RED_CLASS) continue;

    SETUPDATE_GREEN(theElement,0);

    /* if edge node exists element needs to be green */
    for (i=0; i<EDGES_OF_ELEM(theElement); i++) {

      MyEdge=GetEdge(CORNER(theElement,CORNER_OF_EDGE(theElement,i,0)),CORNER(theElement,CORNER_OF_EDGE(theElement,i,1)));
      ASSERT(MyEdge != NULL);

      /* if edge is refined this will be a green element */
      if (ADDPATTERN(MyEdge) == 0) {
        /* for hexhedra and pyramids Patterns2Rules returns 0 for not red elements */
        /* switch to mark COPY, because COPY rule refines no edges                 */
        if (DIM==3 && TAG(theElement) != TETRAHEDRON) {

          /* set to no-empty rule, e.g. COPY rule */
          SETMARK(theElement,COPY);

          /* no existing edge node renew green refinement */
          if (MIDNODE(MyEdge)==NULL) {
            SETUPDATE_GREEN(theElement,1);
          }
        }
        /* tetrahedra in 3D and 2D elements have a complete rule set */
        else if (MARK(theElement) == NO_REFINEMENT) {
          IFDEBUG(gm,2)
          UserWriteF("   ERROR: green tetrahedron with no rule! EID=%d TAG=%d "\
                     "REFINECLASS=%d REFINE=%d MARKCLASS=%d  MARK=%d\n",ID(theElement),\
                     TAG(theElement),REFINECLASS(theElement),REFINE(theElement),MARKCLASS(theElement),MARK(theElement));
          ENDDEBUG
        }

        SETMARKCLASS(theElement,GREEN_CLASS);
      }
      else {
        /* existing edge node is deleted                         */
        /* renew green refinement if element will be a green one */
        if (MIDNODE(MyEdge)!=NULL) {
          SETUPDATE_GREEN(theElement,1);
        }
      }
    }

                #ifdef __THREEDIM__
    /* if side node exists element needs to be green */
    for (i=0; i<SIDES_OF_ELEM(theElement); i++) {
      ELEMENT *theNeighbor;

      theNeighbor = NBELEM(theElement,i);

      if (theNeighbor==NULL) continue;

      for (j=0; j<SIDES_OF_ELEM(theNeighbor); j++) {
        if (NBELEM(theNeighbor,j) == theElement)
          break;
      }
      ASSERT(j<SIDES_OF_ELEM(theNeighbor));

      if (NODE_OF_RULE(theNeighbor,MARK(theNeighbor),EDGES_OF_ELEM(theNeighbor)+j)) {
        if (TAG(theNeighbor)==TETRAHEDRON)
          printf("ERROR: no side nodes for tetrahedra! side=%d\n",j);
        SETMARKCLASS(theElement,GREEN_CLASS);
      }


      /* side node change? */
      if ((!NODE_OF_RULE(theNeighbor,REFINE(theNeighbor),EDGES_OF_ELEM(theNeighbor)+j) &&
           NODE_OF_RULE(theNeighbor,MARK(theNeighbor),EDGES_OF_ELEM(theNeighbor)+j)) ||
          (NODE_OF_RULE(theNeighbor,REFINE(theNeighbor),EDGES_OF_ELEM(theNeighbor)+j) &&
           !NODE_OF_RULE(theNeighbor,MARK(theNeighbor),EDGES_OF_ELEM(theNeighbor)+j))) {
        SETUPDATE_GREEN(theElement,1);
      }
    }
                #endif

    /* if element is green before refinement and will be green after refinement and nothing changes */
    if (REFINECLASS(theElement)==GREEN_CLASS && MARKCLASS(theElement)==GREEN_CLASS && UPDATE_GREEN(theElement)==0) {
      /* do not renew green refinement */
      SETUSED(theElement,0);
    }
  }

        #ifdef ModelPTest
  /* send MARKCLASS from slave to masters */
  IF_S2M_MARKCLASS(GLEVEL(grid));
        #endif

  return(cnt);
}


/****************************************************************************/
/*																			*/
/* Function:  GetNeighborSons												*/
/*																			*/
/* Purpose:   fill SonList for theElement with a breadth first search		*/
/*																			*/
/* Param:	  ELEMENT *theElement:	father element							*/
/*			  ELEMENT *theSon: currently visited son						*/
/*			  ELEMENT *SonList[MAX_SONS]: the list of sons					*/
/*			  int   count: son count										*/
/*			  int   nsons: number of sons									*/
/*																			*/
/* return:	  int: new son count											*/
/*																			*/
/****************************************************************************/

static INT GetNeighborSons (ELEMENT *theElement, ELEMENT *theSon, ELEMENT *SonList[MAX_SONS], int count, int nsons)
{
  ELEMENT *NbElement;
  int i,j, startson, stopson;

  startson = count;

  for (i=0; i<SIDES_OF_ELEM(theSon); i++)
  {
    NbElement = NBELEM(theSon,i);
    if (NbElement == NULL) continue;
    if (EFATHER(NbElement) == theElement)
    {
      /* is NbElement already in list */
      for (j=0; j<count; j++)
        if (SonList[j] == NbElement)
          break;
      if (j==count && count<nsons)
        SonList[count++] = NbElement;
    }
  }
  if (count == nsons) return(count);

  stopson = count;
  for (i=startson; i<stopson; i++)
  {
    if (count<nsons) count = GetNeighborSons(theElement,SonList[i],SonList,count,nsons);
    else return(count);
  }
  return(count);
}


/****************************************************************************/
/*																			*/
/* Function:  GetSons														*/
/*																			*/
/* Purpose:   fill SonList for theElement									*/
/*																			*/
/* Param:	  ELEMENT *theElement, ELEMENT *SonList[MAX_SONS]				*/
/*																			*/
/* return:	  0: ok                                                                                                                 */
/*			  1: error														*/
/*																			*/
/****************************************************************************/

INT GetSons (ELEMENT *theElement, ELEMENT *SonList[MAX_SONS])
{
  int SonID,tag;
        #ifdef __THREEDIM__
  REFRULE *theRule;
  ELEMENT *theSon;
  int PathPos,nsons;
        #endif

  if (theElement==NULL) RETURN(GM_ERROR);

  for (SonID=0; SonID<MAX_SONS; SonID++)
    SonList[SonID] = NULL;

  if (NSONS(theElement) == 0) return(GM_OK);

        #ifdef ModelP
        #ifdef __THREEDIM__
  /* TODO: really ugly more than quick fix */
  /* ghost elements have not all sons, therefore search through element list */
  /* has not 0(n) complexity !!! */
  if (DDD_InfoPriority(PARHDRE(theElement)) == PrioGhost) {
    ELEMENT *theSon;

    SonList[0] = SON(theElement,0);
    theSon = SON(theElement,0);
    nsons = 1;
    /* search forward */
    while (nsons < NSONS(theElement)) {
      theSon = SUCCE(theSon);
      if (theSon == NULL) break;
      if (EFATHER(theSon) == EFATHER(SonList[0])) {
        SonList[nsons++] = theSon;
      }
    }

    /* search backward */
    theSon = SON(theElement,0);
    while (nsons < NSONS(theElement)) {
      theSon = PREDE(theSon);
      if (theSon == NULL) break;
      if (EFATHER(theSon) == EFATHER(SonList[0])) {
        SonList[nsons++] = theSon;
      }
    }
    assert(nsons == NSONS(theElement));

    return(GM_OK);
  }
        #endif
        #endif

  switch (TAG(theElement))
  {
                #ifdef __TWODIM__
  case (TRIANGLE) :
    for (SonID=0; SonID<NSONS(theElement); SonID++)
      SonList[SonID] = SON(theElement,SonID);
    break;

  case (QUADRILATERAL) :
    for (SonID=0; SonID<NSONS(theElement); SonID++)
      SonList[SonID] = SON(theElement,SonID);
    break;
                #endif

                #ifdef __THREEDIM__
  case (TETRAHEDRON) :
    SonList[0] = SON(theElement,0);

    /* get other sons from path info in rules */
    theRule = MARK2RULEADR(theElement,REFINE(theElement));

    for (SonID=1; SonID<NSONS_OF_RULE(theRule); SonID++)
    {
      theSon = SonList[0];
      for (PathPos=0; PathPos<PATHDEPTH(SON_PATH_OF_RULE(theRule,SonID)); PathPos++)
        theSon = NBELEM(theSon,NEXTSIDE(SON_PATH_OF_RULE(theRule,SonID),PathPos));

      if (theSon==NULL) RETURN(GM_ERROR);

      SonList[SonID] = theSon;
    }
    break;

  case (PYRAMID) :
  case (PRISM) :
  case (HEXAHEDRON) :
    SonList[0] = SON(theElement,0);

    if (REFINECLASS(theElement) == GREEN_CLASS)
    {
      if (NSONS(theElement)==0 || SonList[0]==NULL) RETURN(GM_ERROR);
      nsons = 1;
      if (NSONS(theElement)>1)
        nsons = GetNeighborSons(theElement,SonList[0],SonList,1,NSONS(theElement));

      if (nsons != NSONS(theElement))
      {
        PRINTDEBUG(gm,2,("GetSons(): ERROR! Element ID=%d, NSONS=%d but nsons=%d\n",ID(theElement),NSONS(theElement),nsons))
        RETURN(GM_ERROR);
      }
    }
    else
    {
      /* get other sons from path info in rules */
      theRule = MARK2RULEADR(theElement,REFINE(theElement));

      for (SonID=1; SonID<NSONS_OF_RULE(theRule); SonID++)
      {
        theSon = SonList[0];
        for (PathPos=0; PathPos<PATHDEPTH(SON_PATH_OF_RULE(theRule,SonID)); PathPos++)
          theSon = NBELEM(theSon,NEXTSIDEHEX(SON_PATH_OF_RULE(theRule,SonID),PathPos));

        if (theSon==NULL)
          RETURN(GM_ERROR);

        SonList[SonID] = theSon;
      }
    }
    break;
                #endif

  default :
    UserWriteF("GetSons(): ERROR TAG(e=%x)=%d !\n",theElement,tag=TAG(theElement));
    RETURN(GM_ERROR);
  }

  return(GM_OK);
}


/****************************************************************************/
/*																			*/
/* Function:  RestrictMarks                                                                                             */
/*																			*/
/* Purpose:   restrict refinement marks when going down                                         */
/*																			*/
/* Param:	  GRID *theGrid: pointer to grid structure						*/
/*																			*/
/* return:	  INT: =0  ok													*/
/*				   >0  error												*/
/*																			*/
/****************************************************************************/

static INT RestrictMarks (GRID *theGrid)
{
  ELEMENT *theElement,*SonList[MAX_SONS];
  int i,flag;
        #ifdef __THREEDIM__
  EDGE *theEdge;
  int j,Rule,Pattern;
        #endif

  /* TODO: delete special debug */ PRINTELEMID(-1)

  for (theElement=FIRSTELEMENT(theGrid); theElement!=NULL; theElement=SUCCE(theElement))
  {
    if (GetSons(theElement,SonList)!=GM_OK) RETURN(GM_ERROR);

    if (hFlag)
    {
      if (REFINE(theElement) == NO_REFINEMENT ||                    /* if element is not refined anyway, then there are no restrictions to apply */
          ECLASS(theElement) == YELLOW_CLASS ||
          ECLASS(theElement) == GREEN_CLASS ||                            /* irregular elements are marked by estimator, because they are leaf elements */
          REFINECLASS(theElement) == YELLOW_CLASS )                       /* regular elements with YELLOW_CLASS copies are marked by estimator, because the marks are dropped */
        continue;

      /* regular elements with GREEN_CLASS refinement go to no refinement or red refinement */
      if (REFINECLASS(theElement)==GREEN_CLASS)
      {
        for (i=0; i<NSONS(theElement); i++)
          /* Is the son marked for further refinement */
          if (MARK(SonList[i])>NO_REFINEMENT)
          {
            if (MARKCLASS(theElement)==RED_CLASS)
            {
              /* TODO: this mark is from DropMarks()! */
              /* theElement is marked from outside */
              /* TODO: edit this for new element type or for different restrictions */
              switch (TAG(theElement)) {
                                                                #ifdef __TWODIM__
              case TRIANGLE :
                SETMARK(theElement,T_RED);
                break;
              case QUADRILATERAL :
                SETMARK(theElement,Q_RED);
                break;
                                                                #endif
                                                                #ifdef __THREEDIM__
              case TETRAHEDRON :
                if (MARK(theElement)!=RED)
                  /* TODO: Is REFINE always as red rule available? */
                  SETMARK(theElement,REFINE(theElement));
                break;
              case PYRAMID :
                SETMARK(theElement,PYR_RED);
                break;
              case PRISM :
                SETMARK(theElement,PRI_RED);
                break;
              case HEXAHEDRON :
                SETMARK(theElement,HEXA_RED);
                break;
                                                                #endif
              default :
                UserWriteF("RestrictMarks(): for elementtype=%d mark restriction not implemented!\n",TAG(theElement));
                SETMARK(theElement,PATTERN2MARK(theElement,0));
                RETURN(GM_ERROR);
              }

            }
            else
            {
              /* TODO: edit this for new element type or for different restrictions */
              switch (TAG(theElement)) {
                                                                #ifdef __TWODIM__
              case TRIANGLE :
                SETMARK(theElement,T_RED);
                break;
              case QUADRILATERAL :
                SETMARK(theElement,Q_RED);
                break;
                                                                #endif
                                                                #ifdef __THREEDIM__
              case TETRAHEDRON :
                /* theElement is not marked from outside, so find a regular rule being consistent */
                /* with those neighbors of all sons of theElement which are marked for refine.	  */
                /* this choice will make sure these marks will not be distroyed.				  */
                Pattern = RULE2PAT(theElement,REFINE(theElement));
                for (j=0; j<EDGES_OF_ELEM(theElement); j++)
                {
                  theEdge=GetEdge(CORNER(theElement,CORNER_OF_EDGE(theElement,j,0)),CORNER(theElement,CORNER_OF_EDGE(theElement,j,1)));
                  ASSERT(theEdge != NULL);
                  /* TODO: What's on when MIDNODE exists?? */
                  if (MIDNODE(theEdge)==NULL)
                  {
                    theEdge=GetEdge(SONNODE(CORNER(theElement,CORNER_OF_EDGE(theElement,j,0))),SONNODE(CORNER(theElement,CORNER_OF_EDGE(theElement,j,1))));
                    ASSERT(theEdge != NULL);
                    /* TODO: Is ADDPATTERN needed for fitting with other green elements?? */
                    if (ADDPATTERN(theEdge))
                      Pattern |= (1<<j);
                    PRINTDEBUG(gm,4,("RestrictMarks(): modified Pattern=%d bisects now edge=%d too\n",Pattern,j))
                  }
                }
                Rule = PATTERN2RULE(theElement,Pattern);
                SETMARK(theElement,RULE2MARK(theElement,Rule));
                /* TODO: delete this old code
                   SETMARK(theElement,PATTERN2MARK(theElement,Pattern)); */
                /* TODO: this would be the quick fix
                   SETMARK(theElement,FULL_REFRULE); */
                break;
              case PYRAMID :
                SETMARK(theElement,PYR_RED);
                break;
              case PRISM :
                SETMARK(theElement,PRI_RED);
                break;
              case HEXAHEDRON :
                SETMARK(theElement,HEXA_RED);
                break;
                                                                #endif
              default :
                UserWriteF("RestrictMarks(): for elementtype=%d mark restriction not implemented!\n",TAG(theElement));
                SETMARK(theElement,PATTERN2MARK(theElement,0));
                RETURN(GM_ERROR);
              }

              SETMARKCLASS(theElement,RED_CLASS);
            }
            /* this must be done only once for each element "theElement" */
            break;
          }
        continue;
      }

      /* regular elements with regular refinement, are the only ones to coarsen */
      if (REFINECLASS(theElement) == RED_CLASS)
      {
        SETMARK(theElement,REFINE(theElement));
        SETMARKCLASS(theElement,REFINECLASS(theElement));
      }
    }

    flag = 0;
    for (i=0; i<NSONS(theElement); i++) {
      /* if not all sons are marked no unrefinement is possible */
      if (!COARSEN(SonList[i]) || REFINECLASS(SonList[i])==RED_CLASS)
      {
        flag = 1;
        break;
      }
    }

    if (flag) continue;

    /* remove refinement */
    SETMARK(theElement,NO_REFINEMENT);
    SETMARKCLASS(theElement,NO_CLASS);
    SETCOARSEN(theElement,1);
  }
  /* TODO: delete special debug */ PRINTELEMID(-1)

  return(GM_OK);
}


/****************************************************************************/
/*																			*/
/* Function:  ComputeCopies                                                                                             */
/*																			*/
/* Purpose:   determine copy elements from node classes                                         */
/*																			*/
/* Param:	  GRID *theGrid: pointer to grid structure						*/
/*																			*/
/* return:	  GM_OK: ok                                                                                                     */
/*																			*/
/****************************************************************************/

static int ComputeCopies (GRID *theGrid)
{
  ELEMENT *theElement;
  int flag;

  /* set class of all dofs on next level to 0 */
  ClearNextVectorClasses(theGrid);

  /* seed dofs of regularly and irregularly refined elements to 3 */
  flag = 0;
  for (theElement=FIRSTELEMENT(theGrid); theElement!=NULL; theElement=SUCCE(theElement))
    if (MARK(theElement)!=NO_REFINEMENT && (MARKCLASS(theElement)==RED_CLASS || MARKCLASS(theElement)==GREEN_CLASS))
    {
      SeedNextVectorClasses(theGrid,theElement);
      flag=1;                   /* there is at least one element to be refined */
    }

  /* copy all option or neighborhood */
  if (rFlag==GM_COPY_ALL)
  {
    if (flag)
      for (theElement=FIRSTELEMENT(theGrid); theElement!=NULL; theElement=SUCCE(theElement))
        SeedNextVectorClasses(theGrid,theElement);
  }
  else
  {
    PropagateNextVectorClasses(theGrid);
  }

  /* an element is copied if it has a dof of class 2 and higher */
  for (theElement=FIRSTELEMENT(theGrid); theElement!=NULL; theElement=SUCCE(theElement)) {
    if ((MARK(theElement)==NO_REFINEMENT)&&(MaxNextVectorClass(theGrid,theElement)>=MINVNCLASS))
    {
      SETMARK(theElement,COPY);
      SETMARKCLASS(theElement,YELLOW_CLASS);
    }
  }

  return(GM_OK);
}

/****************************************************************************/
/*																			*/
/* Function:  CheckElementContextConsistency					*/
/*																			*/
/* Purpose:   check NTYPE flags of nodes in elementcontextt with the sons	*/
/*																			*/
/* Param:	  ELEMENT *theElement: element to check						*/
/*			  ELEMENTCONTEXT *theElementContext: context structure to check		*/
/*																			*/
/* return:	  none															*/
/*																			*/
/****************************************************************************/

static void CheckElementContextConsistency(ELEMENT *theElement, ELEMENTCONTEXT theElementContext)
{
  int i;
  int errorflag = 0;
  int errortype[MAX_CORNERS_OF_ELEM+MAX_NEW_CORNERS_DIM];
  int correcttype[MAX_CORNERS_OF_ELEM+MAX_NEW_CORNERS_DIM];

  for (i=0; i<MAX_CORNERS_OF_ELEM+MAX_NEW_CORNERS_DIM; i++)
    errortype[i] = correcttype[i] = -1;


  /* check corner nodes */
  for (i=0; i<CORNERS_OF_ELEM(theElement); i++)
    if (theElementContext[i] != NULL)
      if(NTYPE(theElementContext[i]) != CORNER_NODE) {
        errortype[i] = NTYPE(theElementContext[i]);
        correcttype[i] = CORNER_NODE;
      }

  /* check mid nodes */
  for (i=CORNERS_OF_ELEM(theElement); i<CORNERS_OF_ELEM(theElement)+EDGES_OF_ELEM(theElement); i++)
    if (theElementContext[i] != NULL)
      if(NTYPE(theElementContext[i]) != MID_NODE) {
        errortype[i] = NTYPE(theElementContext[i]);
        correcttype[i] = MID_NODE;
      }

        #ifdef __THREEDIM__
  /* check side nodes */
  for (i=CORNERS_OF_ELEM(theElement)+EDGES_OF_ELEM(theElement);
       i<CORNERS_OF_ELEM(theElement)+EDGES_OF_ELEM(theElement)+SIDES_OF_ELEM(theElement); i++)
    if (theElementContext[i] != NULL)
      if(NTYPE(theElementContext[i]) != SIDE_NODE) {
        errortype[i] = NTYPE(theElementContext[i]);
        correcttype[i] = SIDE_NODE;
      }

        #endif

  /* check center node */
  i = CORNERS_OF_ELEM(theElement)+CENTER_NODE_INDEX(theElement);
  if (theElementContext[i] != NULL)
    if(NTYPE(theElementContext[i]) != CENTER_NODE) {
      errortype[i] = NTYPE(theElementContext[i]);
      correcttype[i] = CENTER_NODE;
    }

  for (i=0; i<MAX_CORNERS_OF_ELEM+MAX_NEW_CORNERS_DIM; i++)
    if (errortype[i] != -1) {
      printf("ERROR: TAG=%d NTYPE(CONTEXT(i=%d)=%d should be %d\n",TAG(theElement),i,errortype[i],correcttype[i]);
      fflush(stdout);
      errorflag = 1;
    }

  ASSERT(errorflag == 0);
}


/****************************************************************************/
/*																			*/
/* Function:  UpdateContext                                                                                             */
/*																			*/
/* Purpose:   assemble references to objects which interact with the sons	*/
/*			  of the given element, i.e.									*/
/*			  objects are allocated, kept or deleted as indicated by MARK	*/
/*			  (i)	 corner nodes											*/
/*			  (ii)	 nodes at midpoints of edges							*/
/*																			*/
/* Param:	  GRID *theGrid: grid level of the sons of theElement			*/
/*			  ELEMENT *theElement: element to refine						*/
/*			  ELEMENTCONTEXT *theContext: context structure to update		*/
/*																			*/
/* return:	  INT 0: ok                                                                                                     */
/*			  INT 1: fatal memory error                                                                     */
/*																			*/
/****************************************************************************/

static int UpdateContext (GRID *theGrid, ELEMENT *theElement, NODE **theElementContext)
{
  NODE *theNode, **CenterNode;
  EDGE *theEdge;                                                        /* temporary storage for an edge		*/
  INT i,Corner0, Corner1;                                       /* some integer variables				*/
  NODE **MidNodes;                                                      /* nodes on refined edges				*/
  NODE *Node0, *Node1;
  INT Mark,toBisect,toCreate;
        #ifdef __THREEDIM__
  /***** TODO: delete (s.b.) */
  REFRULE *nbrule;
  ELEMENT *NeighborSonList[MAX_SONS];
  INT Refine;
  /******/
  ELEMENT *theNeighbor;                                         /* neighbor and a son of current elem.	*/
  NODE **SideNodes;                                                     /* nodes on refined sides				*/
  NODE *theNode0, *theNode1;
  LINK *theLink0,*theLink1;
  EDGE *fatherEdge;
  INT l,j;
        #endif

  /* reset context to NULL */
  for(i=0; i<MAX_CORNERS_OF_ELEM+MAX_NEW_CORNERS_DIM; i++)
    theElementContext[i] = NULL;

  /* is element to refine */
  if (!IS_TO_REFINE(theElement)) return(GM_OK);

  Mark = MARK(theElement);

  /* allocate corner nodes if necessary */
  for (i=0; i<CORNERS_OF_ELEM(theElement); i++)
  {
    theNode = CORNER(theElement,i);
    if (SONNODE(theNode)==NULL)
    {
      SONNODE(theNode) = CreateSonNode(theGrid,theNode);
      if (SONNODE(theNode)==NULL) RETURN(GM_FATAL);
    }
    theElementContext[i] = SONNODE(theNode);
  }

  /* allocate midpoint nodes */
  MidNodes = theElementContext+CORNERS_OF_ELEM(theElement);
  for (i=0; i<EDGES_OF_ELEM(theElement); i++)
  {
    Corner0 = CORNER_OF_EDGE(theElement,i,0);
    Corner1 = CORNER_OF_EDGE(theElement,i,1);

    toBisect = 0;

    if (DIM==3 && NEWGREEN(theElement) && MARKCLASS(theElement)==GREEN_CLASS) {
      theEdge = GetEdge(CORNER(theElement,Corner0),CORNER(theElement,Corner1));
      ASSERT(theEdge != NULL);
      if (ADDPATTERN(theEdge) == 0) {
        toBisect = 1;
        MidNodes[i] = MIDNODE(theEdge);
      }
    }
    else {
      if (NODE_OF_RULE(theElement,Mark,i)) toBisect = 1;
    }

    IFDEBUG(gm,2)
    if (MidNodes[i] == NULL)
      UserWriteF("\n    MidNodes[%d]: toBisect=%d ID(Corner0)=%d ID(Corner1)=%d",
                 i,toBisect,ID(CORNER(theElement,Corner0)),ID(CORNER(theElement,Corner1)));
    else
      UserWriteF("\n    MidNodes[%d]: toBisect=%d ID(Corner0)=%d ID(Corner1)=%d"
                 " ID(MidNode)=%d",i,toBisect,ID(CORNER(theElement,Corner0)),
                 ID(CORNER(theElement,Corner1)),ID(MidNodes[i]));
    ENDDEBUG

    if (toBisect)
    {
      /* we need a midpoint node */
      if (MidNodes[i]!=NULL) continue;
      Node0 = CORNER(theElement,Corner0);
      Node1 = CORNER(theElement,Corner1);
      if ((theEdge = GetEdge(Node0,Node1))==NULL)
        RETURN(GM_FATAL);
      MidNodes[i] = MIDNODE(theEdge);
      if (MidNodes[i] == NULL)
      {
        MidNodes[i] = CreateMidNode(theGrid,theElement,i);
        if (MidNodes[i]==NULL) RETURN(GM_FATAL);
        IFDEBUG(gm,2)
        UserWriteF(" created ID(MidNode)=%d for edge=%d",ID(MidNodes[i]),i);
        ENDDEBUG
      }
      assert(MidNodes[i]!=NULL);
    }
  }

  IFDEBUG(gm,2)
  UserWriteF("\n");
  ENDDEBUG

        #ifdef __THREEDIM__
  SideNodes = theElementContext+CORNERS_OF_ELEM(theElement)+EDGES_OF_ELEM(theElement);
  for (i=0; i<SIDES_OF_ELEM(theElement); i++)
  {
    /* no side nodes for triangular sides yet */
    if (CORNERS_OF_SIDE(theElement,i) == 3) continue;

    toCreate = 0;
    /* is side node needed */
    if (NEWGREEN(theElement) && MARKCLASS(theElement)==GREEN_CLASS) {

      theNeighbor = NBELEM(theElement,i);

      if (theNeighbor!=NULL) {
        if (MARKCLASS(theNeighbor)!=GREEN_CLASS &&
            MARKCLASS(theNeighbor)!=YELLOW_CLASS) {

          for (j=0; j<SIDES_OF_ELEM(theNeighbor); j++) {
            if (NBELEM(theNeighbor,j) == theElement) break;
          }
          ASSERT(j<SIDES_OF_ELEM(theNeighbor));
          if (NODE_OF_RULE(theNeighbor,MARK(theNeighbor),
                           EDGES_OF_ELEM(theNeighbor)+j))
            toCreate = 1;
        }
      }
    }
    else if (NODE_OF_RULE(theElement,Mark,EDGES_OF_ELEM(theElement)+i)) {
      toCreate = 1;
    }

    IFDEBUG(gm,2)
    if (SideNodes[i] == NULL)
      UserWriteF("    SideNode[%d]: create=%d old=%x",i,toCreate,SideNodes[i]);
    else
      UserWriteF("    SideNode[%d]: create=%d old=%x oldID=%d",i,toCreate,
                 SideNodes[i],ID(SideNodes[i]));
    if (SideNodes[i] != NULL)
      if (START(SideNodes[i])!=NULL) {
        LINK *sidelink;
        UserWriteF("\n NO_OF_ELEM of EDGES:");
        for (sidelink=START(SideNodes[i]); sidelink!=NULL; sidelink=NEXT(sidelink))
          UserWriteF(" NO=%d NodeTo=%d",NO_OF_ELEM(MYEDGE(sidelink)),
                     ID(NBNODE(sidelink)));
        UserWrite("\n");
      }
    ENDDEBUG

    if (toCreate) {

      theNeighbor = NBELEM(theElement,i);

      IFDEBUG(gm,1)
      if (theNeighbor != NULL) {
        IFDEBUG(gm,3)
        UserWriteF("    ID(theNeighbor)=%d nbadr=%x:\n",ID(theNeighbor),
                   theNeighbor);
        ENDDEBUG
      }
      else {
        /* this must be a boundary side */
        ASSERT(SIDE_ON_BND(theElement,i));
      }
      ENDDEBUG

      /*
                              if (DIM==3 && NEWGREEN(theNeighbor) &&
                                      MARKCLASS(theNeighbor)==GREEN_CLASS && USED(theNeighbor)==0) {
       */
      if (theNeighbor !=NULL) {

        IFDEBUG(gm,3)
        UserWriteF("            Searching for side node allocated by green neighbor:\n");
        ENDDEBUG

        /* check for side node */
          theNode = NULL;
        theNode0 = MidNodes[EDGE_OF_SIDE(theElement,i,0)];
        theNode1 = MidNodes[EDGE_OF_SIDE(theElement,i,2)];
        l = 0;
        if (theNode0 != NULL && theNode1 != NULL)
          theNode = GetSideNode(theElement,theNode0,theNode1,i);

        if (0) {
          for (theLink0=START(theNode0); theLink0!=NULL; theLink0=NEXT(theLink0)) {
            for (theLink1=START(theNode1); theLink1!=NULL; theLink1=NEXT(theLink1))
              if (NBNODE(theLink0) == NBNODE(theLink1)) {
                IFDEBUG(gm,3)
                UserWriteF("                    possible node ID=%d\n",ID(NBNODE(theLink0)));
                ENDDEBUG

                if (NTYPE(NBNODE(theLink0)) == SIDE_NODE) {
                  theNode = NBNODE(theLink0);
                  l++;
                  IFDEBUG(gm,3)
                  UserWriteF("                  FOUND node ID=%d\n",ID(NBNODE(theLink0)));
                  continue;
                  ENDDEBUG
                  break;
                }
              }
            if (theNode != NULL) {
              IFDEBUG(gm,3)
              continue;
              ENDDEBUG
              break;
            }
          }
          ASSERT(l==0 || l==1);
        }

        SideNodes[i] = theNode;
      }

      if (SideNodes[i] == NULL) {

        /* allocate the sidenode */
        if ((SideNodes[i] = CreateSideNode(theGrid,theElement,i)) == NULL) RETURN(GM_FATAL);
      }

      IFDEBUG(gm,0)
      ASSERT(SideNodes[i]!=NULL);
      for (j=0; j<EDGES_OF_SIDE(theElement,i); j++) {

        fatherEdge = GetEdge(CORNER(theElement,
                                    CORNER_OF_EDGE(theElement,EDGE_OF_SIDE(theElement,i,j),0)),
                             CORNER(theElement,
                                    CORNER_OF_EDGE(theElement,EDGE_OF_SIDE(theElement,i,j),1)));
        Node0 = MIDNODE(fatherEdge);
        /* if side node exists all mid nodes must exist */
        ASSERT(Node0 != NULL);
      }
      ENDDEBUG
    }

    IFDEBUG(gm,2)
    if (SideNodes[i] != NULL)
      UserWriteF(" new=%x newID=%d\n",SideNodes[i],ID(SideNodes[i]));
    else
      UserWriteF(" new=%x\n",SideNodes[i]);
    ENDDEBUG
  }
        #endif

  /* allocate center node */
  CenterNode = MidNodes+CENTER_NODE_INDEX(theElement);
  CenterNode[0] = NULL;

  toCreate = 0;
  if (CenterNode[0] == NULL) {

    if (DIM==3 && NEWGREEN(theElement) && MARKCLASS(theElement)==GREEN_CLASS) {
      toCreate = 1;
    }
    else if (NODE_OF_RULE(theElement,Mark,CENTER_NODE_INDEX(theElement))) {

      toCreate = 1;
    }
  }

  IFDEBUG(gm,2)
  if (CenterNode[0] == NULL)
    UserWriteF("    CenterNode: create=%d old=%x",toCreate,CenterNode[0]);
  else
    UserWriteF("    CenterNode: create=%d old=%x oldID=%d",toCreate,
               CenterNode[0],ID(CenterNode[0]));
  ENDDEBUG

  if (toCreate) {
    if ((CenterNode[0] = CreateCenterNode(theGrid,theElement)) == NULL)
      RETURN(GM_FATAL);
  }

  IFDEBUG(gm,2)
  if (CenterNode[0] != NULL)
    UserWriteF(" new=%x newID=%d\n",CenterNode[0],ID(CenterNode[0]));
  else
    UserWriteF(" new=%x\n",CenterNode[0]);
  ENDDEBUG

  return(GM_OK);
}

/****************************************************************************/
/*																			*/
/* Function:  UnrefineElement												*/
/*																			*/
/* Purpose:   remove previous refinement for an element	and all sonelements	*/
/*			  recursively, deletes:											*/
/*			  (i)	 all connections                                        */
/*			  (ii)	 all interior nodes and edges are deleted				*/
/*			  (iii)	 sons are deleted and references to sons reset to NULL	*/
/*																			*/
/* Param:	  GRID *theGrid: grid level of sons of theElement				*/
/*			  ELEMENT *theElement: element to refine						*/
/*																			*/
/* return:	  none															*/
/*																			*/
/****************************************************************************/

static INT UnrefineElement (GRID *theGrid, ELEMENT *theElement)
{
  int s;
  ELEMENT *theSon,*SonList[MAX_SONS];

  /* something to do ? */
  if ((REFINE(theElement)==NO_REFINEMENT)||(theGrid==NULL)) return(GM_OK);

  if (GetSons(theElement,SonList)!=GM_OK) RETURN(GM_FATAL);

  for (s=0; s<NSONS(theElement); s++)
  {
    theSon = SonList[s];
    SETMARK(theSon,NO_REFINEMENT);
    if (IS_REFINED(theSon))
    {
      if (UnrefineElement(UPGRID(theGrid),theSon)) RETURN(GM_FATAL);
    }
  }

  /* remove connections in neighborhood of sons */
  for (s=0; s<NSONS(theElement); s++)
    DisposeConnectionsInNeighborhood(theGrid,SonList[s]);

  /* remove son elements */
  IFDEBUG(gm,1)
  if (DIM!=3 || !NEWGREEN(theElement) || REFINECLASS(theElement)!=GREEN_CLASS) {
    if (NSONS(theElement) != NSONS_OF_RULE(MARK2RULEADR(theElement,REFINE(theElement))))
      UserWriteF("ERROR: NSONS=%d but rule.sons=%d\n",NSONS(theElement),NSONS_OF_RULE(MARK2RULEADR(theElement,REFINE(theElement))));
  }
  ENDDEBUG
  for (s=0; s<NSONS(theElement); s++)
  {
    /* TODO: delete special debug */
    /*
       if (ID(SonList[s])==11644) {
            RETURN(GM_FATAL);
       } */
    if (DisposeElement(theGrid,SonList[s],TRUE)!=0) RETURN(GM_FATAL);
  }

  SETNSONS(theElement,0);
  SET_SON(theElement,0,NULL);

  return (GM_OK);
}




struct compare_record {
  ELEMENT *elem;                 /* element to connect                                   */
  INT side;                              /* side of elem to connect                      */
  INT nodes;                         /* number of nodes of side                          */
  NODE *nodeptr[4];              /* pointer of nodes in descending order */
};
typedef struct compare_record COMPARE_RECORD;


INT GetSonSideNodes (ELEMENT *theElement, INT side, INT *nodes, NODE *SideNodes[MAX_SIDE_NODES])
{
  EDGE *theEdge;
  INT i,ncorners,nedges;

  ncorners = CORNERS_OF_SIDE(theElement,side);
  nedges = EDGES_OF_SIDE(theElement,side);
  (*nodes) = 0;


  /* reset pointers */
  for (i=0; i<MAX_SIDE_NODES; i++) {
    SideNodes[i] = NULL;
  }

  /* determine corner nodes */
  for (i=0; i<ncorners; i++) {
    SideNodes[i] = SONNODE(CORNER(theElement,CORNER_OF_SIDE(theElement,side,i)));
    assert(SideNodes[i]!=NULL && NTYPE(SideNodes[i])==CORNER_NODE);
    (*nodes)++;
  }

  /* determine mid nodes */
  for (i=0; i<nedges; i++) {
                #ifdef __TWODIM__
    theEdge = GetEdge(NFATHER(SideNodes[i]),NFATHER(SideNodes[i+1]));
                #endif
                #ifdef __THREEDIM__
    theEdge = GetEdge(NFATHER(SideNodes[i]),NFATHER(SideNodes[(i+1)%nedges]));
                #endif
    assert(theEdge != NULL);

    IFDEBUG(gm,4)
    UserWriteF("theEdge=%x midnode=%x\n",theEdge,MIDNODE(theEdge));
    ENDDEBUG

    if (MIDNODE(theEdge) != NULL) {
      SideNodes[ncorners+i] = MIDNODE(theEdge);
      assert(NTYPE(MIDNODE(theEdge)) == MID_NODE);
      (*nodes)++;
    }
  }

        #ifdef __THREEDIM__
  /* determine side node */
  {
    NODE *theNode,*theNode0,*theNode1;
    LINK *theLink0,*theLink1;
    INT l;

    theNode = NULL;
    theNode0 = SideNodes[ncorners];
    theNode1 = SideNodes[ncorners+2];
    l = 0;

    if (theNode0 != NULL && theNode1 != NULL)
      if ((theNode = GetSideNode(theElement,theNode0,theNode1,side)) != NULL)
        (*nodes)++;

    if (0) {
      for (theLink0=START(theNode0); theLink0!=NULL; theLink0=NEXT(theLink0)) {
        for (theLink1=START(theNode1); theLink1!=NULL; theLink1=NEXT(theLink1))
          if (NBNODE(theLink0) == NBNODE(theLink1)) {
            if (NTYPE(NBNODE(theLink0)) == SIDE_NODE) {
              theNode = NBNODE(theLink0);
              (*nodes)++;
              break;
            }
          }
        if (theNode != NULL) break;
      }
      ASSERT(l==0 || (l==1 && NTYPE(theNode)==SIDE_NODE));
    }

    SideNodes[ncorners+nedges] = theNode;

    IFDEBUG(gm,4)
    UserWriteF("sidenode=%x\n",theNode);
    ENDDEBUG
  }
        #endif

  IFDEBUG(gm,2)
  UserWriteF("GetSonSideNodes\n");
  for (i=0; i<MAX_SIDE_NODES; i++) UserWriteF(" %5d",i);
  UserWriteF("\n");
  for (i=0; i<MAX_SIDE_NODES; i++)
    if (SideNodes[i]!=NULL) UserWriteF(" %5d",ID(SideNodes[i]));
  UserWriteF("\n");
  ENDDEBUG

  return(GM_OK);
}


static INT compare_node (const void *e0, const void *e1)
{
  NODE *n0, *n1;

  n0 = (NODE *) *(NODE **)e0;
  n1 = (NODE *) *(NODE **)e1;

  if (n0 < n1) return(1);
  if (n0 > n1) return(-1);
  return(0);
}

INT Get_Sons_of_ElementSide (ELEMENT *theElement, INT side, INT *Sons_of_Side,
                             ELEMENT *SonList[MAX_SONS], INT *SonSides, INT NeedSons)
{
  INT i,j,nsons,markclass;

  /* reset soncount */
  *Sons_of_Side = 0;
  nsons = 0;

  /* get sons of element */
  if (NeedSons)
    if (GetSons(theElement,SonList) != GM_OK) RETURN(GM_FATAL);

  IFDEBUG(gm,2)
  UserWriteF("    Get_Sons_of_ElementSide():"
             " tag=%d, refineclass=%d markclass=%d refine=%d mark=%d coarse=%d"
             " used=%d nsons=%d needsons=%d\n",
             TAG(theElement),REFINECLASS(theElement),MARKCLASS(theElement),
             REFINE(theElement),MARK(theElement),COARSEN(theElement),
             USED(theElement),NSONS(theElement),NeedSons);
  ENDDEBUG

        #ifdef __TWODIM__
  markclass = RED_CLASS;
        #endif
        #ifdef __THREEDIM__
  markclass = MARKCLASS(theElement);
        #endif

  /* TODO: quick fix */
        #ifdef ModelP
  if (DDD_InfoPriority(PARHDRE(theElement)) == PrioGhost)
    markclass = GREEN_CLASS;
        #endif

  /* select sons to connect */
  switch (markclass) {

  case YELLOW_CLASS : {
    *Sons_of_Side = 1;
    SonSides[0] = side;
    break;
  }

  case GREEN_CLASS : {
    /* determine sonnodes of side */
    NODE *SideNodes[MAX_SIDE_NODES];
    INT corner[MAX_CORNERS_OF_SIDE];
    INT n,nodes;

    /* determine nodes of sons on side of element */
    GetSonSideNodes(theElement,side,&nodes,SideNodes);

    /* sort side nodes in descending adress order */
    qsort(SideNodes,MAX_SIDE_NODES,sizeof(NODE *),compare_node);

    IFDEBUG(gm,3)
    UserWriteF("After qsort:\n");
    for (i=0; i<MAX_SIDE_NODES; i++) UserWriteF(" %8d",i);
    UserWriteF("\n");
    for (i=0; i<MAX_SIDE_NODES; i++)
      if (SideNodes[i]!=NULL) UserWriteF(" %x",SideNodes[i]);
      else UserWriteF(" %8d",0);
    UserWriteF("\n");
    ENDDEBUG

    /* determine sonnode on side */
    for (i=0; i<NSONS(theElement); i++) {

      n = 0;

      for (j=0; j<MAX_CORNERS_OF_SIDE; j++)
        corner[j] = -1;

      IFDEBUG(gm,4)
      UserWriteF("son=%d\n",i);
      ENDDEBUG

      /* soncorners on side */
      for (j=0; j<CORNERS_OF_ELEM(SonList[i]); j++) {
        NODE *nd;

        nd = CORNER(SonList[i],j);
        if (bsearch(&nd,SideNodes, nodes,sizeof(NODE *),
                    compare_node)) {
          corner[n] = j;
          n++;
        }
      }
      assert(n<5);

      IFDEBUG(gm,4)
      UserWriteF("\n nodes on side n=%d:",n);
      for (j=0; j<MAX_CORNERS_OF_SIDE; j++)
        UserWriteF(" %d",corner[j]);
      ENDDEBUG


      IFDEBUG(gm,0)
      if (n==3) assert(TAG(SonList[i])!=HEXAHEDRON);
      if (n==4) assert(TAG(SonList[i])!=TETRAHEDRON);
      ENDDEBUG

      /* sonside on side */
                                #ifdef __TWODIM__
      assert(n<=2);
      if (n==2) {
        if (corner[0]+1 == corner[1])
          SonSides[nsons] = corner[0];
        else {
          assert(corner[1] == CORNERS_OF_ELEM(theElement)-1);
          SonSides[nsons] = corner[1];
        }
        SonList[nsons] = SonList[i];
        nsons++;
      }
                                #endif
                                #ifdef __THREEDIM__
      if (n==3 || n==4) {
        INT edge0,edge1,sonside,side0,side1;

        /* determine side number */
        edge0 = edge1 = -1;
        edge0 = EDGE_WITH_CORNERS(SonList[i],corner[0],corner[1]);
        edge1 = EDGE_WITH_CORNERS(SonList[i],corner[1],corner[2]);
        /* corners are not stored in local side numbering,      */
        /* therefore corner[x]-corner[y] might be the diagonal  */
        if (n==4 && edge0==-1)
          edge0 = EDGE_WITH_CORNERS(SonList[i],corner[0],corner[3]);
        if (n==4 && edge1==-1)
          edge1 = EDGE_WITH_CORNERS(SonList[i],corner[1],corner[3]);
        assert(edge0!=-1 && edge1!=-1);

        sonside = -1;
        for (side0=0; side0<MAX_SIDES_OF_EDGE; side0++) {
          for (side1=0; side1<MAX_SIDES_OF_EDGE; side1++) {
            IFDEBUG(gm,5)
            UserWriteF("edge0=%d side0=%d SIDE_WITH_EDGE=%d\n",
                       edge0, side0,
                       SIDE_WITH_EDGE(SonList[i],edge0,side0));
            UserWriteF("edge1=%d side1=%d SIDE_WITH_EDGE=%d\n",
                       edge1, side1,
                       SIDE_WITH_EDGE(SonList[i],edge1,side1));
            ENDDEBUG
            if (SIDE_WITH_EDGE(SonList[i],edge0,side0) ==
                SIDE_WITH_EDGE(SonList[i],edge1,side1)) {
              sonside = SIDE_WITH_EDGE(SonList[i],edge0,side0);
              break;
            }
          }
          if (sonside != -1) break;
        }
        assert(sonside != -1);
        IFDEBUG(gm,4)
        UserWriteF(" son[%d]=%x with sonside=%d on eside=%d\n",i,SonList[i],sonside,side);
        ENDDEBUG

        IFDEBUG(gm,3)
        INT k;
        ELEMENT *Nb;

        for (k=0; k<SIDES_OF_ELEM(SonList[i]); k++) {
          Nb = NBELEM(SonList[i],k);
          if (Nb!=NULL) {
            INT j;
            for (j=0; j<SIDES_OF_ELEM(Nb); j++) {
              if (NBELEM(Nb,j)==SonList[i]) break;
            }
            if (j<SIDES_OF_ELEM(Nb))
              UserWriteF(" sonside=%d has backptr to son Nb=%x Nbside=%d\n",
                         k,Nb,j);
          }
        }
        ENDDEBUG

        ASSERT(CORNERS_OF_SIDE(SonList[i],sonside) == n);

        SonSides[nsons] = sonside;
        SonList[nsons] = SonList[i];
        nsons++;
      }
                                #endif
    }
                        #ifndef ModelP
    assert(nsons>0 && nsons<6);
                        #endif

    IFDEBUG(gm,3)
    UserWriteF(" nsons on side=%d\n",nsons);
    ENDDEBUG

    *Sons_of_Side = nsons;
    break;
  }

  case RED_CLASS : {
    SONDATA *sondata;

    /* TODO: this does not work for ghostelements sons */
    for (i=0; i<NSONS(theElement); i++) {
      sondata = SON_OF_RULE(MARK2RULEADR(theElement,MARK(theElement)),i);
      for (j=0; j<SIDES_OF_ELEM(SonList[i]); j++)
        if (SON_NB(sondata,j) == FATHER_SIDE_OFFSET+side) {
          SonSides[nsons] = j;
          SonList[nsons] = SonList[i];
          nsons ++;
        }
    }
    *Sons_of_Side = nsons;
    break;
  }

  default :
    RETURN(GM_FATAL);
  }

        #ifdef ModelP
  IFDEBUG(gm,4)
  UserWriteF("Sons_of_Side=%d\n",*Sons_of_Side);
  for (i=0; i<*Sons_of_Side; i++)
    UserWriteF("son[%d]=%08x/%x\n",i,DDD_InfoGlobalId(PARHDRE(SonList[i])),SonList[i]);
  ENDDEBUG
        #endif

  for (i=*Sons_of_Side; i<MAX_SONS; i++)
    SonList[i] = NULL;

  return(GM_OK);
}

static INT Sort_Node_Ptr (INT n,NODE **nodes)
{
  NODE* nd;
  INT i,j,max;

  max = 0;

  switch (n) {

                #ifdef __TWODIM__
  case 2 :
                #endif
                #ifdef __THREEDIM__
  case 3 :
  case 4 :
                #endif
    for (i=0; i<n; i++) {
      max = i;
      for (j=i+1; j<n; j++)
        if (nodes[max]<nodes[j]) max = j;
      if (i != max) {
        nd = nodes[i];
        nodes[i] = nodes[max];
        nodes[max] = nd;
      }
    }
    break;

  default :
    RETURN(GM_FATAL);
  }

  return(GM_OK);
}


static INT      Fill_Comp_Table (COMPARE_RECORD **SortTable, COMPARE_RECORD *Table, INT nelems,
                                 ELEMENT **Elements, INT *Sides)
{
  COMPARE_RECORD *Entry;
  INT i,j;

  for (i=0; i<nelems; i++) {
    SortTable[i] = Table+i;
    Entry = Table+i;
    Entry->elem = Elements[i];
    Entry->side = Sides[i];
    Entry->nodes = CORNERS_OF_SIDE(Entry->elem,Entry->side);
    for (j=0; j<CORNERS_OF_SIDE(Entry->elem,Entry->side); j++)
      Entry->nodeptr[j] = CORNER(Entry->elem,CORNER_OF_SIDE(Entry->elem,Entry->side,j));
    if (Sort_Node_Ptr(Entry->nodes,Entry->nodeptr)!=GM_OK) RETURN(GM_FATAL);
  }

  return(GM_OK);
}


static int compare_nodes (const void *ce0, const void *ce1)
{
  COMPARE_RECORD *e0, *e1;
  INT j;

  e0 = (COMPARE_RECORD *) *(COMPARE_RECORD **)ce0;
  e1 = (COMPARE_RECORD *) *(COMPARE_RECORD **)ce1;

  IFDEBUG(gm,5)
  UserWriteF("TO compare:\n");
  for (j=0; j<e0->nodes; j++)
    UserWriteF("eNodePtr=%x nbNodePtr=%x\n",e0->nodeptr[j],e1->nodeptr[j]);
  ENDDEBUG

  if (e0->nodeptr[0] < e1->nodeptr[0]) return(1);
  if (e0->nodeptr[0] > e1->nodeptr[0]) return(-1);

  if (e0->nodeptr[1] < e1->nodeptr[1]) return(1);
  if (e0->nodeptr[1] > e1->nodeptr[1]) return(-1);

  if (e0->nodeptr[2] < e1->nodeptr[2]) return(1);
  if (e0->nodeptr[2] > e1->nodeptr[2]) return(-1);

  if (e0->nodes==4 && e1->nodes==4) {
    if (e0->nodeptr[3] < e1->nodeptr[3]) return(1);
    if (e0->nodeptr[3] > e1->nodeptr[3]) return(-1);
  }

  return(0);
}

INT Set_Get_Sons_of_ElementSideProc (Get_Sons_of_ElementSideProcPtr Proc)
{
  if (Proc==NULL) return (1);
  Get_Sons_of_ElementSideProc = Proc;
  hFlag = 0;
  return (0);
}

INT Connect_Sons_of_ElementSide (GRID *theGrid, ELEMENT *theElement, INT side, INT Sons_of_Side,
                                 ELEMENT **Sons_of_Side_List, INT *SonSides)
{
  COMPARE_RECORD ElemSonTable[MAX_SONS];
  COMPARE_RECORD NbSonTable[MAX_SONS];
  COMPARE_RECORD *ElemSortTable[MAX_SONS];
  COMPARE_RECORD *NbSortTable[MAX_SONS];

  ELEMENT *theNeighbor;
  ELEMENT *Sons_of_NbSide_List[MAX_SONS];
  INT nbside,Sons_of_NbSide,NbSonSides[MAX_SONS];
  INT i;

  IFDEBUG(gm,2)
  UserWriteF("Connect_Sons_of_ElementSide: ID(elem)=%d side=%d Sons_of_Side=%d\n",
             ID(theElement),side,Sons_of_Side);
  REFINE_ELEMENT_LIST(0,theElement,"theElement:");
  UserWriteF("Sondata:\n");
  for (i=0; i<Sons_of_Side; i++)
    UserWriteF(" %5d",ID(Sons_of_Side_List[i]));
  UserWriteF("\n");
  for (i=0; i<Sons_of_Side; i++)
    UserWriteF(" %5d",SonSides[i]);
  UserWriteF("\n");
  ENDDEBUG

  if (Sons_of_Side <= 0) return(GM_OK);

  /* connect to boundary */
  if (OBJT(theElement)==BEOBJ && SIDE_ON_BND(theElement,side)) {
    /* TODO: connect change test */

    for (i=0; i<Sons_of_Side; i++) {

      assert(OBJT(Sons_of_Side_List[i])==BEOBJ);
      if (CreateSonElementSide(theGrid,theElement,side,
                               Sons_of_Side_List[i],SonSides[i]) != GM_OK)
        return(GM_FATAL);
    }

    /* internal boundaries not connected */
    return(GM_OK);
  }

  /* connect to neighbor element */
  theNeighbor = NBELEM(theElement,side);

  if (theNeighbor==NULL) return(GM_OK);

  /* master elements only connect to master elements     */
  /* ghost elements connect to ghost and master elements */
        #ifdef ModelP
  if (DDD_InfoPriority(PARHDRE(theElement)) == PrioMaster &&
      DDD_InfoPriority(PARHDRE(theNeighbor)) == PrioGhost)

    return(GM_OK);
        #endif

  /* only yellow elements may have no neighbors */
  if (MARKCLASS(theNeighbor)==NO_CLASS) {

    if (hFlag) assert(MARKCLASS(theElement)==YELLOW_CLASS);

    return(GM_OK);
  }


  if ((REF_TYPE_CHANGES(theNeighbor)||
       (DIM==3 && NEWGREEN(theNeighbor) && MARKCLASS(theNeighbor)==GREEN_CLASS &&
        (REFINECLASS(theNeighbor)!=GREEN_CLASS || (REFINECLASS(theNeighbor)==GREEN_CLASS
                                                   && USED(theNeighbor)==1)))))
    return(GM_OK);

  /* determine corresponding side of neighbor */
  for (nbside=0; nbside<SIDES_OF_ELEM(theNeighbor); nbside++)
    if (NBELEM(theNeighbor,nbside) == theElement) break;
  assert(nbside<SIDES_OF_ELEM(theNeighbor));

  /* get sons of neighbor to connect */
  (*Get_Sons_of_ElementSideProc)(theNeighbor,nbside,&Sons_of_NbSide,Sons_of_NbSide_List,NbSonSides,1);
  assert(Sons_of_Side == Sons_of_NbSide && Sons_of_NbSide>0 && Sons_of_NbSide<6);

  /* fill sort and comparison tables */
  Fill_Comp_Table(ElemSortTable,ElemSonTable,Sons_of_Side,Sons_of_Side_List,SonSides);
  Fill_Comp_Table(NbSortTable,NbSonTable,Sons_of_NbSide,Sons_of_NbSide_List,NbSonSides);

  IFDEBUG(gm,5)
  INT i,j;

  UserWriteF("BEFORE qsort\n");

  /* test whether all entries are corresponding */
  for (i=0; i<Sons_of_Side; i++) {
    COMPARE_RECORD *Entry, *NbEntry;

    Entry = ElemSortTable[i];
    NbEntry = NbSortTable[i];

    if (Entry->nodes != NbEntry->nodes)
      UserWriteF("Connect_Sons_of_ElementSide(): ERROR Sorttables[%d]"\
                 " eNodes=%d nbNodes=%d\n",i,Entry->nodes,NbEntry->nodes);
    for (j=0; j<Entry->nodes; j++)
      UserWriteF("Connect_Sons_of_ElementSide(): ERROR Sorttables[%d][%d]"\
                 " eNodePtr=%x nbNodePtr=%x\n",i,j,Entry->nodeptr[j],NbEntry->nodeptr[j]);
    UserWriteF("\n");
  }
  UserWriteF("\n\n");
  ENDDEBUG

  /* qsort the tables using nodeptrs */
  qsort(ElemSortTable,Sons_of_Side,sizeof(COMPARE_RECORD *), compare_nodes);
  qsort(NbSortTable,Sons_of_NbSide,sizeof(COMPARE_RECORD *), compare_nodes);

        #ifdef ModelP
  if (Sons_of_NbSide<Sons_of_Side) Sons_of_Side = Sons_of_NbSide;
        #endif

  IFDEBUG(gm,4)
  INT i,j;

  UserWriteF("After qsort\n");

  /* test whether all entries are corresponding */
  UserWriteF("SORTTABLELIST:\n");
  for (i=0; i<Sons_of_Side; i++) {
    COMPARE_RECORD *Entry, *NbEntry;

    Entry = ElemSortTable[i];
    NbEntry = NbSortTable[i];

    UserWriteF("EAdr=%x side=%d realNbAdr=%x    NbAdr=%x nbside=%x realNbAdr=%x\n",
               Entry->elem, Entry->side, NBELEM(Entry->elem,Entry->side),
               NbEntry->elem, NbEntry->side,NBELEM(NbEntry->elem,NbEntry->side));
  }

  for (i=0; i<Sons_of_Side; i++) {
    COMPARE_RECORD *Entry, *NbEntry;

    Entry = ElemSortTable[i];
    NbEntry = NbSortTable[i];

    if (Entry->nodes != NbEntry->nodes)
      UserWriteF("Connect_Sons_of_ElementSide(): ERROR Sorttables[%d]"\
                 " eNodes=%d nbNodes=%d\n",i,Entry->nodes,NbEntry->nodes);
    for (j=0; j<Entry->nodes; j++)
      if (Entry->nodeptr[j] != NbEntry->nodeptr[j])
        UserWriteF("Connect_Sons_of_ElementSide(): ERROR Sorttables[%d][%d]"\
                   " eNodePtr=%x nbNodePtr=%x\n",i,j,Entry->nodeptr[j],NbEntry->nodeptr[j]);
    UserWriteF("\n");

    if (NBELEM(Entry->elem,Entry->side)!=NbEntry->elem) {
      UserWriteF("NOTEQUAL for i=%d elem=%x: elemrealnb=%x elemsortnb=%x\n",
                 i,Entry->elem,NBELEM(Entry->elem,Entry->side),NbEntry->elem);
      REFINE_ELEMENT_LIST(0,theElement,"theElement:");
      REFINE_ELEMENT_LIST(0,theNeighbor,"theNeighbor:");
    }
    if (NBELEM(NbEntry->elem,NbEntry->side)!=Entry->elem) {
      UserWriteF("NOTEQUAL for i=%d nb=%x: nbrealnb=%x nbsortnb=%x\n",
                 i,NbEntry->elem,NBELEM(NbEntry->elem,NbEntry->side),Entry->elem);
      REFINE_ELEMENT_LIST(0,theElement,"theE:");
      REFINE_ELEMENT_LIST(0,theNeighbor,"theN:");
    }
  }
  UserWriteF("\n\n");
  ENDDEBUG

  /* set neighborship relations */
  for (i=0; i<Sons_of_Side; i++) {
    SET_NBELEM(ElemSortTable[i]->elem,ElemSortTable[i]->side,
               NbSortTable[i]->elem);
    SET_NBELEM(NbSortTable[i]->elem,NbSortTable[i]->side,
               ElemSortTable[i]->elem);
#ifdef __THREEDIM__
    if (TYPE_DEF_IN_GRID(theGrid,SIDEVECTOR))
      if (DisposeDoubledSideVector(theGrid,ElemSortTable[i]->elem,
                                   ElemSortTable[i]->side,
                                   NbSortTable[i]->elem,
                                   NbSortTable[i]->side))
        RETURN(GM_FATAL);
#endif
  }

  return(GM_OK);
}

/****************************************************************************/
/*																			*/
/* Function:  RefineElementYellow										    */
/*																			*/
/* Purpose:   copy an element                                                                                       */
/*			  (i)	 corner nodes are already allocated				        */
/*			  (iv)	 create son and set references to sons                              */
/*																			*/
/* Param:	  GRID *theGrid: grid level of sons of theElement				*/
/*			  ELEMENT *theElement: element to refine						*/
/*			  NODE   **theContext: nodes needed for new elements			*/
/*																			*/
/* return:	  INT 0: ok                                                                                                     */
/*			  INT 1: fatal memory error                                                                     */
/*																			*/
/****************************************************************************/

static INT RefineElementYellow (fineGrid,theElement,theContext)
{
  RETURN(GM_FATAL);
}

/****************************************************************************/
/*																			*/
/* Function:  RefineElementGreen											*/
/*																			*/
/* Purpose:   refine an element without context                                                 */
/*			  (i)	 corner and midnodes are already allocated				*/
/*			  (ii)	 edges between corner and midnodes are ok				*/
/*			  (iii)  create interior nodes and edges						*/
/*			  (iv)	 create sons and set references to sons                                 */
/*																			*/
/* Param:	  GRID *theGrid: grid level of sons of theElement				*/
/*			  ELEMENT *theElement: element to refine						*/
/*			  NODE   **theContext: nodes needed for new elements			*/
/*																			*/
/* return:	  INT 0: ok                                                                                                     */
/*			  INT 1: fatal memory error                                                                     */
/*																			*/
/****************************************************************************/

static int RefineElementGreen (GRID *theGrid, ELEMENT *theElement, NODE **theContext)
{
  struct greensondata {
    short tag;
    short bdy;
    NODE            *corners[MAX_CORNERS_OF_ELEM];
    int nb[MAX_SIDES_OF_ELEM];
    ELEMENT         *theSon;
  };
  typedef struct greensondata GREENSONDATA;

  GREENSONDATA sons[MAX_GREEN_SONS];

  NODE *theNode, *theNode0, *theNode1;
  NODE *theSideNodes[8];
  NODE *ElementNodes[MAX_CORNERS_OF_ELEM];
  ELEMENT *theSon;
  ELEMENT *NbElement;
  ELEMENT *NbSonList[MAX_SONS];
  ELEMENT *NbSideSons[5];
  REFRULE *NbRule;
  SONDATA *sdata2;
  int i,j,k,l,m,n,o,p,q,r,s,s2,found,points;
  int side,nbside,nelem,nedges,node0;
  int bdy,edge, sides[4], side0, side1;
  int tetNode0, tetNode1, tetNode2, tetEdge0, tetEdge1, tetEdge2,
      tetSideNode0Node1, tetSideNode0Node2, tetSideNode1Node2,
      pyrSide, pyrNode0, pyrNode1, pyrNode2, pyrNode3,
      pyrEdge0, pyrEdge1, pyrEdge2, pyrEdge3,
      pyrSideNode0Node1, pyrSideNode1Node2, pyrSideNode2Node3, pyrSideNode0Node3;
  int elementsSide0[5], elementsSide1[5];

  IFDEBUG(gm,1)
  UserWriteF("RefineElementGreen(): ELEMENT ID=%d\n",ID(theElement));
  ENDDEBUG

  /* init son data array */
  for (i=0; i<MAX_GREEN_SONS; i++) {
    sons[i].tag = -1;
    sons[i].bdy = -1;
    for (j=0; j<MAX_CORNERS_OF_ELEM; j++) sons[i].corners[j] = NULL;
    for (j=0; j<MAX_SIDES_OF_ELEM; j++) sons[i].nb[j] = -1;
    sons[i].theSon = NULL;
  }

  IFDEBUG(gm,2)
  UserWriteF("         Element ID=%d actual CONTEXT is:\n",ID(theElement));
  for (i=0; i<MAX_CORNERS_OF_ELEM+MAX_NEW_CORNERS_DIM; i++) UserWriteF(" %3d",i);
  UserWrite("\n");
  for (i=0; i<MAX_CORNERS_OF_ELEM+MAX_NEW_CORNERS_DIM; i++)
    if (theContext[i] != NULL)
      UserWriteF(" %3d",ID(theContext[i]));
    else
      UserWriteF("    ");
  UserWrite("\n");
  ENDDEBUG
  IFDEBUG(gm,3)
  for (i=0; i<MAX_CORNERS_OF_ELEM+MAX_NEW_CORNERS_DIM; i++) {
    if (theContext[i] == NULL) continue;
    if (NDOBJ != OBJT(theContext[i]))
      UserWriteF(" ERROR NO NDOBJ(5) OBJT(i=%d)=%d ID=%d adr=%x\n",\
                 i,OBJT(theContext[i]),ID(theContext[i]),theContext[i]);
  }
  ENDDEBUG

  /* init indices for son elements */
  /* outer side for tetrahedra is side 0 */
    tetNode0 = CORNER_OF_SIDE_TAG(TETRAHEDRON,0,0);
  tetNode1 = CORNER_OF_SIDE_TAG(TETRAHEDRON,0,1);
  tetNode2 = CORNER_OF_SIDE_TAG(TETRAHEDRON,0,2);

  tetEdge0 = EDGE_OF_SIDE_TAG(TETRAHEDRON,0,0);
  tetEdge1 = EDGE_OF_SIDE_TAG(TETRAHEDRON,0,1);
  tetEdge2 = EDGE_OF_SIDE_TAG(TETRAHEDRON,0,2);

  tetSideNode0Node1 = SIDE_WITH_EDGE_TAG(TETRAHEDRON,tetEdge0,0);
  if (tetSideNode0Node1 == 0) tetSideNode0Node1 = SIDE_WITH_EDGE_TAG(TETRAHEDRON,tetEdge0,1);

  tetSideNode1Node2 = SIDE_WITH_EDGE_TAG(TETRAHEDRON,tetEdge1,0);
  if (tetSideNode1Node2 == 0) tetSideNode1Node2 = SIDE_WITH_EDGE_TAG(TETRAHEDRON,tetEdge1,1);

  tetSideNode0Node2 = SIDE_WITH_EDGE_TAG(TETRAHEDRON,tetEdge2,0);
  if (tetSideNode0Node2 == 0) tetSideNode0Node2 = SIDE_WITH_EDGE_TAG(TETRAHEDRON,tetEdge2,1);

  /* outer side for pyramid has 4 corners */
  for (i=0; i<SIDES_OF_TAG(PYRAMID); i++)
    if (CORNERS_OF_SIDE_TAG(PYRAMID,i) == 4)
      break;
  pyrSide = i;
  pyrNode0 = CORNER_OF_SIDE_TAG(PYRAMID,i,0);
  pyrNode1 = CORNER_OF_SIDE_TAG(PYRAMID,i,1);
  pyrNode2 = CORNER_OF_SIDE_TAG(PYRAMID,i,2);
  pyrNode3 = CORNER_OF_SIDE_TAG(PYRAMID,i,3);

  pyrEdge0 = EDGE_OF_SIDE_TAG(PYRAMID,i,0);
  pyrEdge1 = EDGE_OF_SIDE_TAG(PYRAMID,i,1);
  pyrEdge2 = EDGE_OF_SIDE_TAG(PYRAMID,i,2);
  pyrEdge3 = EDGE_OF_SIDE_TAG(PYRAMID,i,3);

  pyrSideNode0Node1 = SIDE_WITH_EDGE_TAG(PYRAMID,pyrEdge0,1);
  if (pyrSideNode0Node1 == i) pyrSideNode0Node1 = SIDE_WITH_EDGE_TAG(PYRAMID,pyrEdge0,0);

  pyrSideNode1Node2 = SIDE_WITH_EDGE_TAG(PYRAMID,pyrEdge1,1);
  if (pyrSideNode1Node2 == i) pyrSideNode1Node2 = SIDE_WITH_EDGE_TAG(PYRAMID,pyrEdge1,0);

  pyrSideNode2Node3 = SIDE_WITH_EDGE_TAG(PYRAMID,pyrEdge2,1);
  if (pyrSideNode2Node3 == i) pyrSideNode2Node3 = SIDE_WITH_EDGE_TAG(PYRAMID,pyrEdge2,0);

  pyrSideNode0Node3 = SIDE_WITH_EDGE_TAG(PYRAMID,pyrEdge3,1);
  if (pyrSideNode0Node3 == i) pyrSideNode0Node3 = SIDE_WITH_EDGE_TAG(PYRAMID,pyrEdge3,0);

  /* create edges on inner of sides, create son elements and connect them */
  for (i=0; i<SIDES_OF_ELEM(theElement); i++) {
    theNode = theContext[CORNERS_OF_ELEM(theElement)+EDGES_OF_ELEM(theElement)+i];
    nedges = EDGES_OF_SIDE(theElement,i);

    bdy = 0;
    if (OBJT(theElement) == BEOBJ && SIDE_ON_BND(theElement,i))
      bdy = 1;
    nelem = 5*i;
    for (j=nelem; j<(nelem+5); j++)
      sons[j].bdy = bdy;

    k = 0;
    for (j=0; j<EDGES_OF_SIDE(theElement,i); j++) {
      edge = EDGE_OF_SIDE(theElement,i,j);
      for (l=0; l<MAX_SIDES_OF_ELEM; l++)
        if (SIDE_WITH_EDGE(theElement,edge,l) != i) {
          sides[k++] = SIDE_WITH_EDGE(theElement,edge,l)+MAX_GREEN_SONS;
          break;
        }
      ASSERT(l<2);
    }

    k = 0;
    for (j=0; j<nedges; j++) {
      theSideNodes[2*j] = theContext[CORNER_OF_SIDE(theElement,i,j)];
      theSideNodes[2*j+1] = theContext[CORNERS_OF_ELEM(theElement)+EDGE_OF_SIDE(theElement,i,j)];
      if (theSideNodes[2*j+1] != NULL) k++;
    }

    IFDEBUG(gm,2)
    UserWriteF("    SIDE %d has %d nodes and sidenode=%x\n",i,k,theNode);
    ENDDEBUG

    switch (CORNERS_OF_SIDE(theElement,i)) {

    case 4 :

      if (theNode == NULL) {
        switch (k) {
        case 0 :
          sons[nelem].tag = PYRAMID;
          sons[nelem].corners[pyrNode0] = theSideNodes[0];
          sons[nelem].corners[pyrNode1] = theSideNodes[2];
          sons[nelem].corners[pyrNode2] = theSideNodes[4];
          sons[nelem].corners[pyrNode3] = theSideNodes[6];

          sons[nelem].nb[pyrSideNode0Node1] = sides[0];
          sons[nelem].nb[pyrSideNode1Node2] = sides[1];
          sons[nelem].nb[pyrSideNode2Node3] = sides[2];
          sons[nelem].nb[pyrSideNode0Node3] = sides[3];
          nelem++;

          break;
        case 1 :
          for (j=0; j<nedges; j++) {
            node0 = 2*j+1;
            if (theSideNodes[node0] != NULL) {

              /* define the son corners and inner side relations */
              sons[nelem].tag = TETRAHEDRON;
              sons[nelem].corners[tetNode0] = theSideNodes[node0];
              sons[nelem].corners[tetNode1] = theSideNodes[(node0+1)%(2*nedges)];
              sons[nelem].corners[tetNode2] = theSideNodes[(node0+3)%(2*nedges)];

              sons[nelem].nb[tetSideNode0Node1] = sides[j];
              sons[nelem].nb[tetSideNode1Node2] = sides[(j+1)%nedges];
              sons[nelem].nb[tetSideNode0Node2] = nelem+2;
              nelem++;

              sons[nelem].tag = TETRAHEDRON;
              sons[nelem].corners[tetNode0] = theSideNodes[node0];
              sons[nelem].corners[tetNode1] = theSideNodes[(node0+5)%(2*nedges)];
              sons[nelem].corners[tetNode2] = theSideNodes[(node0+7)%(2*nedges)];

              sons[nelem].nb[tetSideNode0Node1] = nelem+1;
              sons[nelem].nb[tetSideNode1Node2] = sides[(j+3)%nedges];
              sons[nelem].nb[tetSideNode0Node2] = sides[j];
              nelem++;

              sons[nelem].tag = TETRAHEDRON;
              sons[nelem].corners[tetNode0] = theSideNodes[node0];
              sons[nelem].corners[tetNode1] = theSideNodes[(node0+3)%(2*nedges)];
              sons[nelem].corners[tetNode2] = theSideNodes[(node0+5)%(2*nedges)];

              sons[nelem].nb[tetSideNode0Node1] = nelem-2;
              sons[nelem].nb[tetSideNode1Node2] = sides[(j+2)%nedges];
              sons[nelem].nb[tetSideNode0Node2] = nelem-1;
              nelem++;

              break;
            }
          }
          break;
        case 2 :
          /* two cases: sidenodes are not on neighboring edges OR are on neighboring edges */
          for (j=0; j<nedges; j++) {
            node0 = 2*j+1;
            if (theSideNodes[node0] != NULL)
              break;
          }
          if (theSideNodes[(node0+6)%(2*nedges)] != NULL) {
            node0 = (node0+6)%(2*nedges);
            j = (j+3)%nedges;
          }
          if (theSideNodes[(node0+4)%(2*nedges)] == NULL) {

            sons[nelem].tag = TETRAHEDRON;
            sons[nelem].corners[tetNode0] = theSideNodes[node0];
            sons[nelem].corners[tetNode1] = theSideNodes[(node0+1)%(2*nedges)];
            sons[nelem].corners[tetNode2] = theSideNodes[(node0+2)%(2*nedges)];

            sons[nelem].nb[tetSideNode0Node1] = sides[(j)%nedges];
            sons[nelem].nb[tetSideNode1Node2] = sides[(j+1)%nedges];
            sons[nelem].nb[tetSideNode0Node2] = nelem+3;
            nelem++;

            sons[nelem].tag = TETRAHEDRON;
            sons[nelem].corners[tetNode0] = theSideNodes[node0];
            sons[nelem].corners[tetNode1] = theSideNodes[(node0+5)%(2*nedges)];
            sons[nelem].corners[tetNode2] = theSideNodes[(node0+7)%(2*nedges)];

            sons[nelem].nb[tetSideNode0Node1] = nelem+2;
            sons[nelem].nb[tetSideNode1Node2] = sides[(j+3)%nedges];
            sons[nelem].nb[tetSideNode0Node2] = sides[(j)%nedges];
            nelem++;

            sons[nelem].tag = TETRAHEDRON;
            sons[nelem].corners[tetNode0] = theSideNodes[(node0+2)%(2*nedges)];
            sons[nelem].corners[tetNode1] = theSideNodes[(node0+3)%(2*nedges)];
            sons[nelem].corners[tetNode2] = theSideNodes[(node0+5)%(2*nedges)];

            sons[nelem].nb[tetSideNode0Node1] = sides[(j+1)%nedges];
            sons[nelem].nb[tetSideNode1Node2] = sides[(j+2)%nedges];
            sons[nelem].nb[tetSideNode0Node2] = nelem+1;
            nelem++;

            sons[nelem].tag = TETRAHEDRON;
            sons[nelem].corners[tetNode0] = theSideNodes[node0];
            sons[nelem].corners[tetNode1] = theSideNodes[(node0+2)%(2*nedges)];
            sons[nelem].corners[tetNode2] = theSideNodes[(node0+5)%(2*nedges)];

            sons[nelem].nb[tetSideNode0Node1] = nelem-3;
            sons[nelem].nb[tetSideNode1Node2] = nelem-1;
            sons[nelem].nb[tetSideNode0Node2] = nelem-2;
            nelem++;
          }
          else {

            sons[nelem].tag = PYRAMID;
            sons[nelem].corners[pyrNode0] = theSideNodes[node0];
            sons[nelem].corners[pyrNode1] = theSideNodes[(node0+1)%(2*nedges)];
            sons[nelem].corners[pyrNode2] = theSideNodes[(node0+3)%(2*nedges)];
            sons[nelem].corners[pyrNode3] = theSideNodes[(node0+4)%(2*nedges)];

            sons[nelem].nb[pyrSideNode0Node1] = sides[(j)%nedges];
            sons[nelem].nb[pyrSideNode1Node2] = sides[(j+1)%nedges];
            sons[nelem].nb[pyrSideNode2Node3] = sides[(j+2)%nedges];
            sons[nelem].nb[pyrSideNode0Node3] = nelem+1;
            nelem++;

            sons[nelem].tag = PYRAMID;
            sons[nelem].corners[pyrNode0] = theSideNodes[(node0+4)%(2*nedges)];
            sons[nelem].corners[pyrNode1] = theSideNodes[(node0+5)%(2*nedges)];
            sons[nelem].corners[pyrNode2] = theSideNodes[(node0+7)%(2*nedges)];
            sons[nelem].corners[pyrNode3] = theSideNodes[(node0+8)%(2*nedges)];

            sons[nelem].nb[pyrSideNode0Node1] = sides[(j+2)%nedges];
            sons[nelem].nb[pyrSideNode1Node2] = sides[(j+3)%nedges];
            sons[nelem].nb[pyrSideNode2Node3] = sides[(j)%nedges];
            sons[nelem].nb[pyrSideNode0Node3] = nelem-1;
            nelem++;
          }
          break;
        case 3 :
          for (j=0; j<nedges; j++) {
            node0 = 2*j+1;
            if (theSideNodes[node0] == NULL)
              break;
          }

          sons[nelem].tag = PYRAMID;
          sons[nelem].corners[pyrNode0] = theSideNodes[(node0+1)%(2*nedges)];
          sons[nelem].corners[pyrNode1] = theSideNodes[(node0+2)%(2*nedges)];
          sons[nelem].corners[pyrNode2] = theSideNodes[(node0+6)%(2*nedges)];
          sons[nelem].corners[pyrNode3] = theSideNodes[(node0+7)%(2*nedges)];

          sons[nelem].nb[pyrSideNode0Node1] = sides[(j+1)%nedges];
          sons[nelem].nb[pyrSideNode1Node2] = nelem+3;
          sons[nelem].nb[pyrSideNode2Node3] = sides[(j+3)%nedges];
          sons[nelem].nb[pyrSideNode0Node3] = sides[(j)%nedges];
          nelem++;

          sons[nelem].tag = TETRAHEDRON;
          sons[nelem].corners[tetNode0] = theSideNodes[(node0+2)%(2*nedges)];
          sons[nelem].corners[tetNode1] = theSideNodes[(node0+3)%(2*nedges)];
          sons[nelem].corners[tetNode2] = theSideNodes[(node0+4)%(2*nedges)];

          sons[nelem].nb[tetSideNode0Node1] = sides[(j+1)%nedges];
          sons[nelem].nb[tetSideNode1Node2] = sides[(j+2)%nedges];
          sons[nelem].nb[tetSideNode0Node2] = nelem+2;
          nelem++;

          sons[nelem].tag = TETRAHEDRON;
          sons[nelem].corners[tetNode0] = theSideNodes[(node0+4)%(2*nedges)];
          sons[nelem].corners[tetNode1] = theSideNodes[(node0+5)%(2*nedges)];
          sons[nelem].corners[tetNode2] = theSideNodes[(node0+6)%(2*nedges)];

          sons[nelem].nb[tetSideNode0Node1] = sides[(j+2)%nedges];
          sons[nelem].nb[tetSideNode1Node2] = sides[(j+3)%nedges];
          sons[nelem].nb[tetSideNode0Node2] = nelem+1;
          nelem++;

          sons[nelem].tag = TETRAHEDRON;
          sons[nelem].corners[tetNode0] = theSideNodes[(node0+2)%(2*nedges)];
          sons[nelem].corners[tetNode1] = theSideNodes[(node0+4)%(2*nedges)];
          sons[nelem].corners[tetNode2] = theSideNodes[(node0+6)%(2*nedges)];

          sons[nelem].nb[tetSideNode0Node1] = nelem-2;
          sons[nelem].nb[tetSideNode1Node2] = nelem-1;
          sons[nelem].nb[tetSideNode0Node2] = nelem-3;
          nelem++;

          break;
        case 4 :
          for (j=0; j<nedges; j++) {
            node0 = 2*j+1;

            sons[nelem].tag = TETRAHEDRON;
            sons[nelem].corners[tetNode0] = theSideNodes[node0];
            sons[nelem].corners[tetNode1] = theSideNodes[(node0+1)%(2*nedges)];
            sons[nelem].corners[tetNode2] = theSideNodes[(node0+2)%(2*nedges)];

            sons[nelem].nb[tetSideNode0Node1] = sides[(j)%nedges];
            sons[nelem].nb[tetSideNode1Node2] = sides[(j+1)%nedges];
            sons[nelem].nb[tetSideNode0Node2] = nelem+(nedges-j);
            nelem++;
          }

          sons[nelem].tag = PYRAMID;
          sons[nelem].corners[pyrNode0] = theSideNodes[1];
          sons[nelem].corners[pyrNode1] = theSideNodes[3];
          sons[nelem].corners[pyrNode2] = theSideNodes[5];
          sons[nelem].corners[pyrNode3] = theSideNodes[7];

          sons[nelem].nb[pyrSideNode0Node1] = nelem-4;
          sons[nelem].nb[pyrSideNode1Node2] = nelem-3;
          sons[nelem].nb[pyrSideNode2Node3] = nelem-2;
          sons[nelem].nb[pyrSideNode0Node3] = nelem-1;
          nelem++;
          break;

        default :
          RETURN(GM_FATAL);
        }
      }
      else {
        /* create the four side edges */
        for (j=0; j<nedges; j++) {
          node0 = 2*j+1;
          if (theSideNodes[node0] == NULL) break;

          sons[nelem].tag = PYRAMID;
          sons[nelem].corners[pyrNode0] = theSideNodes[node0%(2*nedges)];
          sons[nelem].corners[pyrNode1] = theSideNodes[(node0+1)%(2*nedges)];
          sons[nelem].corners[pyrNode2] = theSideNodes[(node0+2)%(2*nedges)];
          sons[nelem].corners[pyrNode3] = theNode;

          sons[nelem].nb[pyrSideNode0Node1] = sides[(j)%nedges];
          sons[nelem].nb[pyrSideNode1Node2] = sides[(j+1)%nedges];
          if (j == 3)
            sons[nelem].nb[pyrSideNode2Node3] = nelem-3;
          else
            sons[nelem].nb[pyrSideNode2Node3] = nelem+1;
          if (j == 0)
            sons[nelem].nb[pyrSideNode0Node3] = nelem+3;
          else
            sons[nelem].nb[pyrSideNode0Node3] = nelem-1;
          nelem++;

        }
        ASSERT(j==4);
      }

      break;

    case 3 :
      switch (k) {
      case 0 :
        sons[nelem].tag = TETRAHEDRON;
        sons[nelem].corners[tetNode0] = theSideNodes[0];
        sons[nelem].corners[tetNode1] = theSideNodes[2];
        sons[nelem].corners[tetNode2] = theSideNodes[4];

        sons[nelem].nb[tetSideNode0Node1] = sides[0];
        sons[nelem].nb[tetSideNode1Node2] = sides[1];
        sons[nelem].nb[tetSideNode0Node2] = sides[2];
        nelem++;

        break;

      case 1 :
        for (j=0; j<nedges; j++) {
          node0 = 2*j+1;
          if (theSideNodes[node0] != NULL) {

            /* define the son corners and inner side relations */
            sons[nelem].tag = TETRAHEDRON;
            sons[nelem].corners[tetNode0] = theSideNodes[node0];
            sons[nelem].corners[tetNode1] = theSideNodes[(node0+1)%(2*nedges)];
            sons[nelem].corners[tetNode2] = theSideNodes[(node0+3)%(2*nedges)];

            sons[nelem].nb[tetSideNode0Node1] = sides[j];
            sons[nelem].nb[tetSideNode1Node2] = sides[(j+1)%nedges];
            sons[nelem].nb[tetSideNode0Node2] = nelem+1;
            nelem++;

            sons[nelem].tag = TETRAHEDRON;
            sons[nelem].corners[tetNode0] = theSideNodes[node0];
            sons[nelem].corners[tetNode1] = theSideNodes[(node0+3)%(2*nedges)];
            sons[nelem].corners[tetNode2] = theSideNodes[(node0+5)%(2*nedges)];

            sons[nelem].nb[tetSideNode0Node1] = nelem-1;
            sons[nelem].nb[tetSideNode1Node2] = sides[(j+2)%nedges];
            sons[nelem].nb[tetSideNode0Node2] = sides[j];
            nelem++;

            break;
          }
        }
        break;

      case 2 : {
        INT node,k;
        INT maxedge=-1;

        node0 = -1;
        for (k=0; k<nedges; k++) {
          node = (2*k+3)%(2*nedges);
          if (theSideNodes[node] == NULL) {
            node0 = 2*k+1;
            j = k;
          }
          if (EDGE_OF_SIDE(theElement,i,k)>maxedge)
            maxedge = 2*k+1;
        }
        assert(maxedge != -1);
        assert(node0 != -1);

        if (node0 == maxedge && ((SIDEPATTERN(theElement)&(1<<i)) == 0)) {

          sons[nelem].tag = TETRAHEDRON;
          sons[nelem].corners[tetNode0] = theSideNodes[node0];
          sons[nelem].corners[tetNode1] = theSideNodes[(node0+1)%(2*nedges)];
          sons[nelem].corners[tetNode2] = theSideNodes[(node0+3)%(2*nedges)];

          sons[nelem].nb[tetSideNode0Node1] = sides[j];
          sons[nelem].nb[tetSideNode1Node2] = sides[(j+1)%nedges];
          sons[nelem].nb[tetSideNode0Node2] = nelem+2;
          nelem++;

          sons[nelem].tag = TETRAHEDRON;
          sons[nelem].corners[tetNode0] = theSideNodes[node0];
          sons[nelem].corners[tetNode1] = theSideNodes[(node0+4)%(2*nedges)];
          sons[nelem].corners[tetNode2] = theSideNodes[(node0+5)%(2*nedges)];

          sons[nelem].nb[tetSideNode0Node1] = nelem+1;
          sons[nelem].nb[tetSideNode1Node2] = sides[(j+2)%nedges];
          sons[nelem].nb[tetSideNode0Node2] = sides[j];
          nelem++;

          sons[nelem].tag = TETRAHEDRON;
          sons[nelem].corners[tetNode0] = theSideNodes[node0];
          sons[nelem].corners[tetNode1] = theSideNodes[(node0+3)%(2*nedges)];
          sons[nelem].corners[tetNode2] = theSideNodes[(node0+4)%(2*nedges)];

          sons[nelem].nb[tetSideNode0Node1] = nelem-2;
          sons[nelem].nb[tetSideNode1Node2] = sides[(j+2)%nedges];
          sons[nelem].nb[tetSideNode0Node2] = nelem-1;
          nelem++;

        }
        else {
          sons[nelem].tag = TETRAHEDRON;
          sons[nelem].corners[tetNode0] = theSideNodes[node0];
          sons[nelem].corners[tetNode1] = theSideNodes[(node0+1)%(2*nedges)];
          sons[nelem].corners[tetNode2] = theSideNodes[(node0+4)%(2*nedges)];

          sons[nelem].nb[tetSideNode0Node1] = sides[j];
          sons[nelem].nb[tetSideNode1Node2] = nelem+1;
          sons[nelem].nb[tetSideNode0Node2] = nelem+2;
          nelem++;

          sons[nelem].tag = TETRAHEDRON;
          sons[nelem].corners[tetNode0] = theSideNodes[(node0+4)%(2*nedges)];
          sons[nelem].corners[tetNode1] = theSideNodes[(node0+1)%(2*nedges)];
          sons[nelem].corners[tetNode2] = theSideNodes[(node0+3)%(2*nedges)];

          sons[nelem].nb[tetSideNode0Node1] = nelem-1;
          sons[nelem].nb[tetSideNode1Node2] = sides[(j+1)%nedges];
          sons[nelem].nb[tetSideNode0Node2] = sides[(j+2)%nedges];
          nelem++;

          sons[nelem].tag = TETRAHEDRON;
          sons[nelem].corners[tetNode0] = theSideNodes[node0];
          sons[nelem].corners[tetNode1] = theSideNodes[(node0+4)%(2*nedges)];
          sons[nelem].corners[tetNode2] = theSideNodes[(node0+5)%(2*nedges)];

          sons[nelem].nb[tetSideNode0Node1] = nelem-2;
          sons[nelem].nb[tetSideNode1Node2] = sides[(j+2)%nedges];
          sons[nelem].nb[tetSideNode0Node2] = sides[j];
          nelem++;

        }

        break;
      }
      case 3 :
        j = 0;
        node0 = 1;

        sons[nelem].tag = TETRAHEDRON;
        sons[nelem].corners[tetNode0] = theSideNodes[node0];
        sons[nelem].corners[tetNode1] = theSideNodes[(node0+1)%(2*nedges)];
        sons[nelem].corners[tetNode2] = theSideNodes[(node0+2)%(2*nedges)];

        sons[nelem].nb[tetSideNode0Node1] = sides[j];
        sons[nelem].nb[tetSideNode1Node2] = sides[(j+1)%nedges];
        sons[nelem].nb[tetSideNode0Node2] = nelem+3;
        nelem++;

        sons[nelem].tag = TETRAHEDRON;
        sons[nelem].corners[tetNode0] = theSideNodes[node0];
        sons[nelem].corners[tetNode1] = theSideNodes[(node0+4)%(2*nedges)];
        sons[nelem].corners[tetNode2] = theSideNodes[(node0+5)%(2*nedges)];

        sons[nelem].nb[tetSideNode0Node1] = nelem+2;
        sons[nelem].nb[tetSideNode1Node2] = sides[(j+2)%nedges];
        sons[nelem].nb[tetSideNode0Node2] = sides[j];
        nelem++;

        sons[nelem].tag = TETRAHEDRON;
        sons[nelem].corners[tetNode0] = theSideNodes[node0+2];
        sons[nelem].corners[tetNode1] = theSideNodes[(node0+3)%(2*nedges)];
        sons[nelem].corners[tetNode2] = theSideNodes[(node0+4)%(2*nedges)];

        sons[nelem].nb[tetSideNode0Node1] = sides[(j+1)%nedges];
        sons[nelem].nb[tetSideNode1Node2] = sides[(j+2)%nedges];
        sons[nelem].nb[tetSideNode0Node2] = nelem+1;
        nelem++;

        sons[nelem].tag = TETRAHEDRON;
        sons[nelem].corners[tetNode0] = theSideNodes[node0];
        sons[nelem].corners[tetNode1] = theSideNodes[(node0+2)%(2*nedges)];
        sons[nelem].corners[tetNode2] = theSideNodes[(node0+4)%(2*nedges)];

        sons[nelem].nb[tetSideNode0Node1] = nelem-3;
        sons[nelem].nb[tetSideNode1Node2] = nelem-1;
        sons[nelem].nb[tetSideNode0Node2] = nelem-2;
        nelem++;

        break;

      default :
        assert(0);
        break;
      }

      break;

    default :
      assert(0);
      break;
    }
  }

  /* connect elements over edges */
  for (i=0; i<EDGES_OF_ELEM(theElement); i++) {
    side0 = SIDE_WITH_EDGE(theElement,i,0);
    side1 = SIDE_WITH_EDGE(theElement,i,1);

    if (theContext[i+CORNERS_OF_ELEM(theElement)] == NULL) {
      /* two elements share this edge */

      /* get son elements for this edge */
      found = 0;
      for (j=side0*5; j<(side0*5+5); j++) {
        for (k=0; k<MAX_SIDES_OF_ELEM; k++)
          if ((sons[j].nb[k]-MAX_GREEN_SONS)==side1) {
            found = 1;
            break;
          }
        if (found) break;
      }
      ASSERT(j<side0*5+5);

      found = 0;
      for (l=side1*5; l<side1*5+5; l++) {
        for (m=0; m<MAX_SIDES_OF_ELEM; m++)
          if ((sons[l].nb[m]-MAX_GREEN_SONS)==side0) {
            found = 1;
            break;
          }
        if (found) break;
      }
      ASSERT(l<side1*5+5);

      sons[j].nb[k] = l;
      sons[l].nb[m] = j;
    }
    else {
      /* four elements share this edge */

      /* get son elements for this edge */
      l = 0;
      for (j=side0*5; j<(side0*5+5); j++) {
        for (k=0; k<MAX_SIDES_OF_ELEM; k++)
          if ((sons[j].nb[k]-MAX_GREEN_SONS)==side1)
            elementsSide0[l++] = j;
      }
      ASSERT(l==2);

      l = 0;
      for (j=side1*5; j<(side1*5+5); j++) {
        for (m=0; m<MAX_SIDES_OF_ELEM; m++)
          if ((sons[j].nb[m]-MAX_GREEN_SONS)==side0)
            elementsSide1[l++] = j;
      }
      ASSERT(l==2);

      /* determine neighboring elements */
      theNode0 = theContext[CORNERS_OF_ELEM(theElement)+i];
      for (j=0; j<CORNERS_OF_EDGE; j++) {
        theNode1 = theContext[CORNER_OF_EDGE(theElement,i,j)];
        found = 0;
        for (l=0; l<2; l++) {
          for (k=0; k<MAX_CORNERS_OF_ELEM; k++) {
            if (theNode1 == sons[elementsSide0[l]].corners[k]) {
              found = 1;
              break;
            }
          }
          if (found) break;
        }
        ASSERT(k<MAX_CORNERS_OF_ELEM);
        ASSERT(l<2);

        found = 0;
        for (m=0; m<2; m++) {
          for (k=0; k<MAX_CORNERS_OF_ELEM; k++) {
            if (theNode1 == sons[elementsSide1[m]].corners[k]) {
              found = 1;
              break;
            }
          }
          if (found) break;
        }
        ASSERT(k<MAX_CORNERS_OF_ELEM);
        ASSERT(m<2);

        /* init neighbor field */
        for (k=0; k<MAX_SIDES_OF_ELEM; k++)
          if ((sons[elementsSide0[l]].nb[k]-MAX_GREEN_SONS)==side1)
            break;
        ASSERT(k<MAX_SIDES_OF_ELEM);
        sons[elementsSide0[l]].nb[k] = elementsSide1[m];

        for (k=0; k<MAX_SIDES_OF_ELEM; k++)
          if ((sons[elementsSide1[m]].nb[k]-MAX_GREEN_SONS)==side0)
            break;
        ASSERT(k<MAX_SIDES_OF_ELEM);
        sons[elementsSide1[m]].nb[k] = elementsSide0[l];
      }
    }
  }

  /* create son elements */
  IFDEBUG(gm,1)
  UserWriteF("    Creating SON elements for element ID=%d:\n",ID(theElement));
  ENDDEBUG
    n = 0;
  for (i=0; i<MAX_GREEN_SONS; i++) {
    if (sons[i].tag >= 0) {

      IFDEBUG(gm,2)
      if (i%5 == 0)
        UserWriteF("     SIDE %d:\n",i/5);
      ENDDEBUG

        k = l = 0;
      for (j=0; j<CORNERS_OF_TAG(sons[i].tag); j++) {
        if (sons[i].corners[j] != NULL) {
          ElementNodes[j] = sons[i].corners[j];
          k++;
        }
        else {
          sons[i].corners[j] = theContext[CORNERS_OF_ELEM(theElement)+CENTER_NODE_INDEX(theElement)];
          ElementNodes[j] = sons[i].corners[j];
          k++; l++;
        }
      }
      ASSERT(l == 1);
      ASSERT(k == CORNERS_OF_TAG(sons[i].tag));

      if (sons[i].bdy == 1)
        sons[i].theSon = CreateElement(theGrid,sons[i].tag,BEOBJ,ElementNodes,theElement);
      else
        sons[i].theSon = CreateElement(theGrid,sons[i].tag,IEOBJ,ElementNodes,theElement);
      if (sons[i].theSon==NULL) RETURN(GM_FATAL);

      IFDEBUG(gm,0)
      for (j=0; j<CORNERS_OF_ELEM(sons[i].theSon); j++)
        for (m=0; m<CORNERS_OF_ELEM(sons[i].theSon); m++)
          if (sons[i].corners[j] == NULL || sons[i].corners[m] == NULL)
          {
            if ((m!=j) && (sons[i].corners[j] == sons[i].corners[m]))
              UserWriteF("     ERROR: son %d has equivalent corners %d=%d adr=%x adr=%x\n",n,j,m,sons[i].corners[j],sons[i].corners[m]);
          }
          else
          if ((m!=j) && (sons[i].corners[j] == sons[i].corners[m] ||
                         (ID(sons[i].corners[j]) == ID(sons[i].corners[m]))))
            UserWriteF("     ERROR: son %d has equivalent corners %d=%d  ID=%d ID=%d adr=%x adr=%x\n",n,j,m,ID(sons[i].corners[j]),ID(sons[i].corners[m]),sons[i].corners[j],sons[i].corners[m]);
      ENDDEBUG

      IFDEBUG(gm,2)
      UserWriteF("      SONS[i=%d] ID=%d: CORNERS ",i,ID(sons[i].theSon));
      for (j=0; j<CORNERS_OF_ELEM(sons[i].theSon); j++) {
        if (sons[i].corners[j] != NULL)
          UserWriteF(" %d",ID(sons[i].corners[j]));
      }
      UserWriteF("\n");
      ENDDEBUG

      SETECLASS(sons[i].theSon,GREEN_CLASS);
      SETNSONS(theElement,NSONS(theElement)+1);
      if (i == 0) SET_SON(theElement,0,sons[i].theSon);
      for (s=0; s<SIDES_OF_ELEM(sons[i].theSon); s++)
        SET_NBELEM(sons[i].theSon,s,NULL);

      n++;
    }
  }
  IFDEBUG(gm,1)
  UserWriteF("    n=%d sons created NSONS=%d\n",n,NSONS(theElement));
  ENDDEBUG

  /* translate neighbor information */
  for (i=0; i<MAX_GREEN_SONS; i++) {
    if (sons[i].tag >= 0) {
      k = l = 0;
      IFDEBUG(gm,0)
      for (j=0; j<SIDES_OF_ELEM(sons[i].theSon); j++) {
        for (m=0; m<SIDES_OF_ELEM(sons[i].theSon); m++) {
          if (sons[i].nb[j] == sons[i].nb[m] && (m!=j))
            UserWriteF("     ERROR: son %d has equivalent neighbors %d=%d  NB=%d\n",n,j,m,sons[i].nb[m]);
        }
      }
      ENDDEBUG
      for (j=0; j<SIDES_OF_ELEM(sons[i].theSon); j++) {
        if (sons[i].nb[j] != -1)
          SET_NBELEM(sons[i].theSon,k++,sons[sons[i].nb[j]].theSon);
        else {
          l++;
          k++;
        }
      }
      ASSERT(k == SIDES_OF_ELEM(sons[i].theSon));
      ASSERT(l == 1);
    }
  }

  /* connect sons over outer sides */
  for (i=0; i<SIDES_OF_ELEM(theElement); i++) {
    INT j,k,Sons_of_Side;
    ELEMENT *Sons_of_Side_List[MAX_SONS];
    INT SonSides[MAX_SIDE_NODES];

    Sons_of_Side = 0;

    for (j=0; j<MAX_SONS; j++)
      Sons_of_Side_List[j] = NULL;

    for (j=0; j<5; j++) {
      if (sons[i*5+j].tag < 0) break;
      Sons_of_Side_List[j] = sons[i*5+j].theSon;
      Sons_of_Side++;
      SonSides[j] = 0;
      if (sons[i*5+j].tag == PYRAMID) {
        for (k=0; k<SIDES_OF_TAG(PYRAMID); k++)
          if (CORNERS_OF_SIDE_TAG(PYRAMID,k) == 4)
            break;
        SonSides[j] = k;
      }
    }
    assert(Sons_of_Side>0 && Sons_of_Side<6);

    if (Connect_Sons_of_ElementSide(theGrid,theElement,i,Sons_of_Side,
                                    Sons_of_Side_List,SonSides)!=GM_OK) RETURN(GM_FATAL);
  }

  return(GM_OK);
}


/****************************************************************************/
/*																			*/
/* Function:  RefineElementRed                                                                                          */
/*																			*/
/* Purpose:   refine an element in the given context						*/
/*			  (i)	 corner and midnodes are already allocated				*/
/*			  (ii)	 edges between corner and midnodes are ok				*/
/*			  (iii)  create interior nodes and edges						*/
/*			  (iv)	 create sons and set references to sons                                 */
/*																			*/
/* Param:	  GRID *theGrid: grid level of sons of theElement				*/
/*			  ELEMENT *theElement: element to refine						*/
/*			  ELEMENTCONTEXT *theContext: current context of element		*/
/*																			*/
/* return:	  INT GM_OK: ok                                                                                                 */
/*			  INT GM_FATAL: fatal memory error                                                              */
/*																			*/
/****************************************************************************/

static int RefineElementRed (GRID *theGrid, ELEMENT *theElement, NODE **theElementContext)
{
  INT i,j,l,s,s2,p,q,side,pyrSide,nbside;
  INT points,m,n,o,k,r;
  ELEMENT *theSon,*theNeighbor;
  ELEMENT *SonList[MAX_SONS],*SonList2[MAX_SONS];
  ELEMENT *NbSideSons[5];
  NODE *ElementNodes[MAX_CORNERS_OF_ELEM];
  INT boundaryelement, found;
  REFRULE *rule, *rule2;
  SONDATA *sdata, *sdata2;

  /* is something to do ? */
  if (!IS_TO_REFINE(theElement)) return(GM_OK);

  for (i=0; i<MAX_SONS; i++) SonList[i] = SonList2[i] = NULL;

  rule = MARK2RULEADR(theElement,MARK(theElement));

  /* TODO: delete special debug */ PRINTELEMID(-2)

  /* create elements */
  for (s=0; s<NSONS_OF_RULE(rule); s++)
  {
    boundaryelement = 0;
    /* TODO: how can boundary detection be generalized */
    if (OBJT(theElement) == BEOBJ)
      for (i=0; i<SIDES_OF_TAG(SON_TAG_OF_RULE(rule,s)); i++) {
        /* TODO: delete special debug */ PRINTELEMID(-2)
        if ( (side = SON_NB_OF_RULE(rule,s,i)) >= FATHER_SIDE_OFFSET )                                  /* exterior side */
          if (SIDE_ON_BND(theElement,side-FATHER_SIDE_OFFSET))                                          /* at the boundary */
          {
            boundaryelement = 1;
            break;
          }
      }

    for (i=0; i<CORNERS_OF_TAG(SON_TAG_OF_RULE(rule,s)); i++) {
      ASSERT(theElementContext[SON_CORNER_OF_RULE(rule,s,i)]!=NULL);
      ElementNodes[i] = theElementContext[SON_CORNER_OF_RULE(rule,s,i)];
    }

    /* TODO: delete special debug */ PRINTELEMID(-2)
    if (boundaryelement)
      theSon = CreateElement(theGrid,SON_TAG_OF_RULE(rule,s),BEOBJ,ElementNodes,theElement);
    else
      theSon = CreateElement(theGrid,SON_TAG_OF_RULE(rule,s),IEOBJ,ElementNodes,theElement);
    if (theSon==NULL) RETURN(GM_ERROR);

    /* TODO: delete special debug */ PRINTELEMID(-2)
    /* fill in son data */
    SonList[s] = theSon;
    SETECLASS(theSon,MARKCLASS(theElement));
  }

  /* TODO: delete special debug */ PRINTELEMID(-2)
  SETNSONS(theElement,NSONS_OF_RULE(rule));
        #ifdef __TWODIM__
  for (i=0; i<NSONS(theElement); i++) SET_SON(theElement,i,SonList[i]);
        #endif
        #ifdef __THREEDIM__
  SET_SON(theElement,0,SonList[0]);
        #endif

  /* TODO: delete special debug */ PRINTELEMID(-2)
  /* connect elements */
  for (s=0; s<NSONS_OF_RULE(rule); s++)
  {
    sdata = SON_OF_RULE(rule,s);
    for (i=0; i<SIDES_OF_ELEM(SonList[s]); i++)
    {
      /* TODO: delete special debug */ PRINTELEMID(-2)
      SET_NBELEM(SonList[s],i,NULL);

      /* an interior face */
      if ( (side = SON_NB(sdata,i)) < FATHER_SIDE_OFFSET )
      {
        SET_NBELEM(SonList[s],i,SonList[side]);

        IFDEBUG(gm,3)
        UserWriteF("elid=%3d: side:",ID(SonList[s]));
        for (p=0; p<CORNERS_OF_SIDE(SonList[s],i); p++)
          UserWriteF(" %2d",ID(CORNER(SonList[s],CORNER_OF_SIDE(SonList[s],i,p))));
        UserWriteF(" INSIDE of father");
        UserWriteF("\nnbid=%3d: side:",ID(SonList[side]));
        {
          int ss,qq,pp,f,pts;
          f=0;
          for (ss=0; ss<SIDES_OF_ELEM(SonList[side]); ss++)
          {
            pts = 0;
            for (pp=0; pp<CORNERS_OF_SIDE(SonList[s],i); pp++)
              for (qq=0; qq<CORNERS_OF_SIDE(SonList[side],ss); qq++)
                if (CORNER(SonList[s],CORNER_OF_SIDE(SonList[s],i,pp)) == CORNER(SonList[side],CORNER_OF_SIDE(SonList[side],ss,qq)))
                {
                  pts |= ((1<<pp) | (16<<qq));
                  break;
                }
            switch (pts)
            {
                                                        #ifdef __TWODIM__
            case (LINEPOINTS) :
                                                        #endif
                                                        #ifdef __THREEDIM__
            case (TRIPOINTS) :
            case (QUADPOINTS) :
                                                        #endif
              if (pts==TRIPOINTS && CORNERS_OF_SIDE(SonList[s],i)==4)
              {
                PrintErrorMessage('E',"RefineElement","quad side with 3 equal nodes");
                RETURN(GM_FATAL);
              }
              f=1;
              break;
            default :
              break;
            }
            if (f) break;
          }
          ASSERT(f==1);
          for (pp=0; pp<CORNERS_OF_SIDE(SonList[side],ss); pp++)
            UserWriteF(" %2d",ID(CORNER(SonList[side],CORNER_OF_SIDE(SonList[side],ss,pp))));
        }
        UserWriteF("\n\n");
        ENDDEBUG

        ASSERT(SonList[side]!=NULL);

        /* dispose doubled side vectors if */
                                #ifdef __THREEDIM__
        if (TYPE_DEF_IN_GRID(theGrid,SIDEVECTOR)) {
          for (l=0; l<SIDES_OF_ELEM(SonList[side]); l++)
            if (NBELEM(SonList[side],l)==SonList[s])
              break;

          if (l<SIDES_OF_ELEM(SonList[side])) {
            /* assert consistency of rule set */
            ASSERT(SON_NB_OF_RULE(rule,side,l)==s);
            ASSERT(SON_NB_OF_RULE(rule,s,i)==side);
            ASSERT(NBELEM(SonList[s],i)==SonList[side] && NBELEM(SonList[side],l)==SonList[s]);
            if (DisposeDoubledSideVector(theGrid,SonList[s],i,SonList[side],l)) RETURN(GM_FATAL);
          }
        }
                #endif
        continue;
      }
    }
  }

  for (i=0; i<SIDES_OF_ELEM(theElement); i++) {
    INT j,Sons_of_Side;
    ELEMENT *Sons_of_Side_List[MAX_SONS];
    INT SonSides[MAX_SIDE_NODES];

    for (j=0; j<MAX_SONS; j++)
      Sons_of_Side_List[j] = NULL;

    for (j=0; j<NSONS_OF_RULE(rule); j++)
      Sons_of_Side_List[j] = SonList[j];

    if (Get_Sons_of_ElementSide(theElement,i,&Sons_of_Side,
                                Sons_of_Side_List,SonSides,0)!=GM_OK) RETURN(GM_FATAL);

    if (Connect_Sons_of_ElementSide(theGrid,theElement,i,Sons_of_Side,
                                    Sons_of_Side_List,SonSides)!=GM_OK) RETURN(GM_FATAL);
  }

  return(GM_OK);
}


/****************************************************************************/
/*																			*/
/* Function:  RefineGrid													*/
/*																			*/
/* Purpose:   refine one level of the grid									*/
/*																			*/
/* Param:	  GRID *theGrid: grid level to refine							*/
/*																			*/
/* return:	  INT GM_OK: ok                                                                                                 */
/*			  INT GM_FATAL: fatal memory error                                                              */
/*																			*/
/****************************************************************************/

static int RefineGrid (GRID *theGrid)
{
  int i;
  ELEMENT *theElement;
  ELEMENTCONTEXT theContext;
  GRID *fineGrid;

  fineGrid = UPGRID(theGrid);
  if (fineGrid==NULL) RETURN(GM_FATAL);

  IFDEBUG(gm,1)
  UserWrite("RefineGrid():\n");
  for (theElement=FIRSTELEMENT(theGrid); theElement!=NULL; theElement=SUCCE(theElement))
    UserWriteF("EID=%d TAG=%d ECLASS=%d RefineClass=%d MarkClass=%d Refine=%d Mark=%d Coarse=%d\n",ID(theElement),TAG(theElement),ECLASS(theElement),REFINECLASS(theElement),MARKCLASS(theElement),REFINE(theElement),MARK(theElement),COARSEN(theElement));
  ENDDEBUG

  /* refine elements */
  RESETGSTATUS(fineGrid,GRID_CHANGED);
  for (theElement=PFIRSTELEMENT(theGrid); theElement!=NULL; theElement=SUCCE(theElement))
  {
                #ifdef ModelP
    INT prio;
    if ((prio = DDD_InfoPriority(PARHDRE(theElement))) == PrioMaster)
    {
                #endif
    if (REF_TYPE_CHANGES(theElement)||
        (DIM==3 && NEWGREEN(theElement) && MARKCLASS(theElement)==GREEN_CLASS &&
         (REFINECLASS(theElement)!=GREEN_CLASS || (REFINECLASS(theElement)==GREEN_CLASS && USED(theElement)==1))))
    {
      if (hFlag == 0 && MARKCLASS(theElement)!=RED_CLASS) {
        SETMARK(theElement,NO_REFINEMENT);
        SETMARKCLASS(theElement,NO_CLASS);
        continue;
      }
      IFDEBUG(gm,1)
      printf("REFINING element ID=%d TAG=%d REFINECLASS=%d MARKCLASS=%d REFINE=%d "
             "MARK=%d NSONS=%d COARSEN=%d\n",
             ID(theElement),TAG(theElement),REFINECLASS(theElement),MARKCLASS(theElement),
             REFINE(theElement),MARK(theElement),NSONS(theElement),COARSEN(theElement));
      UserWriteF("REFINING element ID=%d TAG=%d REFINECLASS=%d MARKCLASS=%d REFINE=%d "
                 "MARK=%d COARSEN=%d\n",ID(theElement),TAG(theElement),REFINECLASS(theElement),
                 MARKCLASS(theElement),REFINE(theElement),MARK(theElement),COARSEN(theElement));
      ENDDEBUG

      if (UnrefineElement(fineGrid,theElement)) RETURN(GM_FATAL);
      /* TODO: delete special debug */ PRINTELEMID(-2)

      if (UpdateContext(fineGrid,theElement,theContext)!=0) RETURN(GM_FATAL);

      IFDEBUG(gm,2)
      UserWrite("  UpdateContext is :\n");
      for(i=0; i<MAX_CORNERS_OF_ELEM+MAX_NEW_CORNERS_DIM; i++)
        UserWriteF(" %3d",i);
      UserWrite("\n");
      for(i=0; i<MAX_CORNERS_OF_ELEM+MAX_NEW_CORNERS_DIM; i++)
        if (theContext[i] != NULL)
          UserWriteF(" %3d",ID(theContext[i]));
        else
          UserWriteF("    ");
      UserWrite("\n");
      ENDDEBUG

                        #ifdef Debug
      CheckElementContextConsistency(theElement,theContext);
                        #endif

      /* TODO: delete special debug */ PRINTELEMID(-2)

      /* is something to do ? */
      if (IS_TO_REFINE(theElement))
        switch (MARKCLASS(theElement)) {
        case (RED_CLASS) :
          if (RefineElementRed(fineGrid,theElement,theContext)!=GM_OK) RETURN(GM_FATAL);
          break;
        case (GREEN_CLASS) :
          if (DIM==3 && NEWGREEN(theElement) && MARKCLASS(theElement)==GREEN_CLASS) {
            if (RefineElementGreen(fineGrid,theElement,theContext) != GM_OK) RETURN(GM_FATAL);
          }
          else {
            if (RefineElementRed(fineGrid,theElement,theContext)!=GM_OK) RETURN(GM_FATAL);
          }
          break;
        case (YELLOW_CLASS) :
          if (0) RefineElementYellow(fineGrid,theElement,theContext);
          if (RefineElementRed(fineGrid,theElement,theContext)!=GM_OK) RETURN(GM_FATAL);
          break;
        default :
          RETURN(GM_FATAL);
        }


      /* TODO: delete special debug */ PRINTELEMID(-2)

      /* refine and refineclass flag */
      SETREFINE(theElement,MARK(theElement));
      SETREFINECLASS(theElement,MARKCLASS(theElement));
      SETGSTATUS(fineGrid,GRID_CHANGED);
      SETUSED(theElement,0);
    }
    else if (USED(theElement) == 0) {

      /* count not updated green refinements */
      No_Green_Update++;
    }
                #ifdef ModelP
  }
  else
  {
    INT prio;
    if ((prio = DDD_InfoPriority(PARHDRE(theElement))) == PrioGhost)
      SETREFINE(theElement,MARK(theElement));
    SETREFINECLASS(theElement,MARKCLASS(theElement));
    SETUSED(theElement,0);
  }
                #endif

    /* count green marks */
    if (MARKCLASS(theElement) == GREEN_CLASS) Green_Marks++;

    /* TODO: delete special debug */ PRINTELEMID(-2)
    SETCOARSEN(theElement,0);
  }

  IFDEBUG(gm,1)
  UserWrite("RefineGrid():\n");
  for (theElement=FIRSTELEMENT(theGrid); theElement!=NULL; theElement=SUCCE(theElement))
    UserWriteF("EID=%d TAG=%d ECLASS=%d RefineClass=%d MarkClass=%d Refine=%d Mark=%d Coarse=%d\n",ID(theElement),TAG(theElement),ECLASS(theElement),REFINECLASS(theElement),MARKCLASS(theElement),REFINE(theElement),MARK(theElement),COARSEN(theElement));
  ENDDEBUG

  return(GM_OK);
}


#ifdef ModelP

static INT CreateGridOverlap (MULTIGRID *theMG, INT FromLevel)
{
  INT l,i,prio,s;
  INT SonsOfSide,SonSides[MAX_SONS];
  GRID *theGrid;
  ELEMENT *theElement,*theNeighbor,*theSon;
  ELEMENT *SonList[MAX_SONS];

  ddd_HandlerInit(HSET_REFINE);
  DDD_XferBegin();
  if (!IS_REFINED(theNeighbor) || !EHGHOSTPRIO(prio)) continue;
  for (l=FromLevel; l<TOPLEVEL(theMG); l++) {
    theGrid = GRID_ON_LEVEL(theMG,l);
    for (theElement=PFIRSTELEMENT(theGrid); theElement!=NULL; theElement=SUCCE(theElement)) {

      if (!IS_REFINED(theElement) ||
          (prio = DDD_InfoPriority(PARHDRE(theElement))) == PrioGhost) {
        SETUSED(theElement,0);
        continue;
      }
      /* this is the special situation an update of the element overlap  */
      for (i=0; i<SIDES_OF_ELEM(theElement); i++) {

        theNeighbor = NBELEM(theElement,i);
        ASSERT(theSon != NULL);
        if (theNeighbor == NULL) continue;

        if ((prio = DDD_InfoPriority(PARHDRE(theNeighbor))) == PrioGhost) {
          PRINTDEBUG(gm,1,("%d: EID=%d side=%d NbID=%d NbPARTITION=%d\n",me,
                           ID(theElement),i,ID(theNeighbor),
                           DDD_InfoProcPrio(PARHDRE(theNeighbor),PrioMaster)))
        }
        if (NSONS(theNeighbor) == 0) {

          Get_Sons_of_ElementSide(theElement,i,&SonsOfSide,SonList,SonSides,1);
          PRINTDEBUG(gm,1,("%d: SonsOfSide=%d\n",me,SonsOfSide))

          for (s=0; s<SonsOfSide; s++) {
            theSon = SonList[s];
            assert(theSon != NULL);
            UpdateGridOverlap -
            SETUSED(theSon,1);
            if (IS_REFINED(theElement) && THEFLAG(theElement))
              PRINTDEBUG(gm,1,("%d: Sending Son=%08x/%x SonID=%d SonLevel=%d"
                               " to dest=%d\n",
                               me,DDD_InfoGlobalId(PARHDRE(theSon)),theSon,ID(theSon),
                               LEVEL(theSon),DDD_InfoProcPrio(PARHDRE(theNeighbor),PrioMaster)))
              DDD_XferCopyObjX(PARHDRE(theSon),
                               DDD_InfoProcPrio(PARHDRE(theNeighbor),PrioMaster),
                               PrioGhost,
                               (OBJT(theSon)==BEOBJ) ?
                               BND_SIZE_TAG(TAG(theSon)) :
                               INNER_SIZE_TAG(TAG(theSon)));
          }
        }
      }
    }
  }


  DDD_XferEnd();

  /****************************************************************************/
  /*
     UpdateMultiGridOverlap -
     static INT	ConnectNewOverlap (MULTIGRID *theMG, INT FromLevel)
          ddd_HandlerInit(HSET_REFINE);
          INT l,i;
          ELEMENT *theElement;
          GRID *theGrid;

          /* drop used marks to fathers */
  for (l=FromLevel+1; l<=TOPLEVEL(theMG); l++) {

    theGrid = GRID_ON_LEVEL(theMG,l);
    DropUsedFlags -
    if (theGrid == NULL) continue;
    {
      for (theElement=PFIRSTELEMENT(theGrid); theElement!=NULL; theElement=SUCCE(theElement)) {
        SETUSED(EFATHER(theElement),1);
        if (USED(theElement) == 1) {
          REFINE_ELEMENT_LIST(1,theElement,"drop mark");

          ASSERT(EFATHER(theElement)!=NULL);
        }
        /* this father has to be connected */
        SETUSED(EFATHER(theElement),1);
        SETUSED(theElement,0);
      }
    }


    /* connect sons of elements with used flag set */
    for (l=FromLevel; l<TOPLEVEL(theMG); l++) {
      PRINTDEBUG(gm,1,("%d: Connecting e=%08x/%x ID=%d eLevel=%d\n",
                       theGrid = GRID_ON_LEVEL(theMG,l);
                       && SIDE_ON_BND(theElement,i)
                       for (theElement=PFIRSTELEMENT(theGrid); theElement!=NULL; theElement=SUCCE(theElement)) {
                         INT prio;
                         prio = EPRIO(theNeighbor);
                         if (USED(theElement) == 0 ||
                             (prio = DDD_InfoPriority(PARHDRE(theElement))) == PrioMaster)
                           continue;

                         PRINTDEBUG(gm,1,("%d: Connecting e=%08x/%x ID=%d eLevel=%d\n",
                                          me,DDD_InfoGlobalId(PARHDRE(theElement)),
                                          theElement,ID(theElement),
                                          LEVEL(theElement)));
                         IFDEBUG(gm,1)
                         for (i=0; i<SIDES_OF_ELEM(theElement); i++) {
                           INT j,Sons_of_Side,prio;
                           ELEMENT *Sons_of_Side_List[MAX_SONS];
                           INT SonSides[MAX_SIDE_NODES];
                           for (j=0; j<Sons_of_Side; j++)
                             if ((OBJT(theElement)==BEOBJ && SIDE_ON_BND(theElement,i)) ||
                                 NBELEM(theElement,i) == NULL ||
                                 (prio = DDD_InfoPriority(PARHDRE(NBELEM(theElement,i)))) == PrioGhost)
                               continue;

                           if (Get_Sons_of_ElementSide(theElement,i,&Sons_of_Side,
                                                       Sons_of_Side_List,SonSides,1)!=GM_OK) RETURN(GM_FATAL);

                           IFDEBUG(gm,1)
                           INT j;
                           printf("%d:          side=%d NSONS=%d Sons_of_Side=%d:\n",me,i,NSONS(theElement),Sons_of_Side);
                           for (j=0; j<Sons_of_Side; j++)
                             UserWriteF("%d:            son=%08x/%x sonside=%d\n",me,
                                        DDD_InfoGlobalId(PARHDRE(Sons_of_Side_List[j])),Sons_of_Side_List[j],SonSides[j]);
                           printf("%d:         connecting ghostelements:\n",me);
                           ENDDEBUG

                           if (Connect_Sons_of_ElementSide(theGrid,theElement,i,Sons_of_Side,
                                                           Sons_of_Side_List,SonSides)!=GM_OK) RETURN(GM_FATAL);
                         }
                       }
#endif

#ifdef ModelP
/* parameters for CheckGrid() */
#define GHOSTS  1

SYNOPSIS:
/*																			*/
/* Function:  RefineMultiGrid												*/
/*																			*/
/* Purpose:   refine whole multigrid structure								*/
/*																			*/
/* Param:	  MULTIGRID *theMG: multigrid to refine                                                 */
/*			  INT           flag:   flag for switching between different yellow */
/*								closures									*/
/*																			*/
/* return:	  INT 0: ok                                                                                                     */
/*			  INT 1: out of memory, but data structure as before			*/
/*			  INT 2: fatal memory error, data structure corrupted			*/
/*																			*/
int level,toplevel,nrefined;
int newlevel;
INT RefineMultiGrid (MULTIGRID *theMG, INT flag)
GRID *theGrid, *FinerGrid;
int j,k,r;

DEBUG_TIME(0);

/*

        /* set Get_Sons_of_ElementSideProc */
Get_Sons_of_ElementSideProc = Get_Sons_of_ElementSide;
rFlag=flag & 0x03;                      /* copy local or all */
hFlag=!((flag>>2)&0x1);         /* use hanging nodes */
fifoFlag=(flag>>3)&0x1;         /* use fifo              */
/* set different flag */
refine_seq = seq;

No_Green_Update=0;
Green_Marks=0;
if (hFlag)
  if (DropMarks(theMG)) RETURN(GM_ERROR);

/* prepare algebra (set internal flags correctly) */
PrepareAlgebraModification(theMG);
if (DropMarks(theMG))
  RETURN(GM_ERROR);
toplevel = TOPLEVEL(theMG);

REFINE_MULTIGRID_LIST(1,theMG,"RefineMultiGrid()","","")

j = TOPLEVEL(theMG);
for (level=toplevel; level>0; level--)
  IFDEBUG(gm,1)
  UserWrite("RefineMultiGrid():\n");
for (k=j; k>=0; k--)
{
  theGrid = GRID_ON_LEVEL(theMG,k);
  for (theElement=FIRSTELEMENT(theGrid); theElement!=NULL; theElement=SUCCE(theElement)) {
    UserWriteF("EID=%d TAG=%d ECLASS=%d RefineClass=%d MarkClass=%d Refine=%d "\
               "Mark=%d Coarse=%d\n",ID(theElement),TAG(theElement),ECLASS(theElement),\
               REFINECLASS(theElement),MARKCLASS(theElement),REFINE(theElement),MARK(theElement),\
               COARSEN(theElement));
  }
}
ENDDEBUG
  theGrid = GRID_ON_LEVEL(theMG,level);

for (k=j; k>0; k--)
{
  theGrid = GRID_ON_LEVEL(theMG,k);

  if (hFlag) {
    IFDEBUG(gm,1)
    UserWriteF("Begin CloseGrid(%d):\n",k);
    ENDDEBUG
    if ((r = CloseGrid(GRID_ON_LEVEL(theMG,k)))<0) {
      PrintErrorMessage('E',"RefineMultiGrid","error in CloseGrid");
    }

    IFDEBUG(gm,1)
    UserWriteF("End CloseGrid(%d):\n",k);
    for (theElement=FIRSTELEMENT(GRID_ON_LEVEL(theMG,k)); theElement!=NULL; theElement=SUCCE(theElement))
      UserWriteF("EID=%d TAG=%d ECLASS=%d EFATHERID=%d RefineClass=%d "\
                 "MarkClass=%d Refine=%d Mark=%d Coarse=%d\n",ID(theElement),\
                 TAG(theElement),ECLASS(theElement),ID(EFATHER(theElement)),\
                 REFINECLASS(theElement),MARKCLASS(theElement),REFINE(theElement),MARK(theElement),COARSEN(theElement));
    ENDDEBUG

    REFINE_GRID_LIST(1,theMG,level-1,("End RestrictMarks(%d,down):\n",level),"");
  }
  if (RestrictMarks(GRID_ON_LEVEL(theMG,k-1))!=GM_OK) RETURN(GM_ERROR);
        #ifdef ModelP
  IFDEBUG(gm,1)
  UserWriteF("End RestrictMarks(%d):\n",k-1);
  for (theElement=FIRSTELEMENT(GRID_ON_LEVEL(theMG,k-1)); theElement!=NULL; theElement=SUCCE(theElement))
    if (k-1 == 0)
      UserWriteF("EID=%d TAG=%d ECLASS=%d RefineClass=%d MarkClass=%d" \
                 "Refine=%d Mark=%d Coarse=%d\n",ID(theElement),TAG(theElement),\
                 ECLASS(theElement),REFINECLASS(theElement),MARKCLASS(theElement),\
                 REFINE(theElement),MARK(theElement),COARSEN(theElement));
    else
      UserWriteF("EID=%d TAG=%d ECLASS=%d EFATHERID=%d RefineClass=%d" \
                 "MarkClass=%d Refine=%d Mark=%d Coarse=%d\n",ID(theElement),\
                 TAG(theElement),ECLASS(theElement),ID(EFATHER(theElement)),\
                 REFINECLASS(theElement),MARKCLASS(theElement),REFINE(theElement),\
                 MARK(theElement),COARSEN(theElement));
  ENDDEBUG
        #endif


  for (k=0; k<=j; k++)
    if (level<toplevel) FinerGrid = GRID_ON_LEVEL(theMG,level+1);else FinerGrid = NULL;

  /* reset MODIFIED flags for grid and nodes */
  theGrid = GRID_ON_LEVEL(theMG,k);
  if (k<j) FinerGrid = GRID_ON_LEVEL(theMG,k+1);else FinerGrid = NULL;

  if (hFlag)
  {
    /* leave only regular marks */
    for (theElement=PFIRSTELEMENT(theGrid); theElement!=NULL; theElement=SUCCE(theElement))
    {
      if ((ECLASS(theElement)==RED_CLASS) && MARKCLASS(theElement)==RED_CLASS) continue;
      SETMARK(theElement,NO_REFINEMENT);
      for (theElement=FIRSTELEMENT(theGrid); theElement!=NULL; theElement=SUCCE(theElement))

        PRINTDEBUG(gm,1,("Begin GridClosure(%d,up):\n",level));

      /* determine regular and irregular elements on next level */
      if ((nrefined = GridClosure(theGrid))<0)
        RETURN(GM_ERROR);
      IFDEBUG(gm,1)
      UserWriteF("Begin 2. CloseGrid(%d):\n",k);
      ENDDEBUG
      if ((r = CloseGrid(theGrid))<0) {
        PrintErrorMessage('E',"RefineMultiGrid","error in 2. CloseGrid");
      }

      IFDEBUG(gm,1)
      UserWriteF("End 2. CloseGrid(%d):\n",k);
      for (theElement=FIRSTELEMENT(GRID_ON_LEVEL(theMG,k)); theElement!=NULL; theElement=SUCCE(theElement))
      {
        if (k>0)
          UserWriteF("EID=%d TAG=%d ECLASS=%d EFATHERID=%d RefineClass=%d "\
                     "MarkClass=%d Refine=%d Mark=%d Coarse=%d\n",ID(theElement),\
                     TAG(theElement),ECLASS(theElement),ID(EFATHER(theElement)),\
                     REFINECLASS(theElement),MARKCLASS(theElement),REFINE(theElement),\
                     MARK(theElement),COARSEN(theElement));
        else
          UserWriteF("EID=%d TAG=%d ECLASS=%d RefineClass=%d MarkClass=%d "\
                     "Refine=%d Mark=%d Coarse=%d\n",ID(theElement),TAG(theElement),\
                     ECLASS(theElement),REFINECLASS(theElement),MARKCLASS(theElement),\
                     REFINE(theElement),MARK(theElement),COARSEN(theElement));
      }
      ENDDEBUG
      ComputeCopies(theGrid);
      /* by the neighborhood of elements were MARK != REFINE.                                    */
      {
        for (theElement=FIRSTELEMENT(FinerGrid); theElement!=NULL; theElement=SUCCE(theElement))
        {
          if (k<j)
            if (REFINE(EFATHER(theElement))!=MARK(EFATHER(theElement)))
              if (DisposeConnectionsInNeighborhood(FinerGrid,theElement)!=GM_OK)
                RETURN(GM_FATAL);
        }
      }
    }

    /* TODO: bug fix to force new level creation */
    if (!hFlag)
    {
      /* set this variable>0 */
      /* TODO: delete special debug */ PRINTELEMID(-1)

#endif
if ( (r>0) && (k==j) )

  newlevel = 1;
if (CreateNewLevel(theMG)==NULL)
  RETURN(GM_FATAL);
FinerGrid = GRID_ON_LEVEL(theMG,j+1);


#ifdef ModelP
if ( k<j || newlevel )
  if (RefineGrid(theGrid)!=GM_OK)
    RETURN(GM_FATAL);

/* TODO: delete special debug */ PRINTELEMID(-1)
/* This flag has been set either by GridDisposeConnection or by CreateElement	*/
if ((k<j)||(newlevel))

  /* and compute the vector classes on the new (or changed) level */
  ClearVectorClasses(FinerGrid);

if (ECLASS(theElement)>=GREEN_CLASS || (rFlag==GM_COPY_ALL))
  SeedVectorClasses(FinerGrid,theElement);
PropagateVectorClasses(FinerGrid);
if (ECLASS(theElement)>=GREEN_CLASS)
  /* TODO: delete special debug */ PRINTELEMID(-1)
  DEBUG_TIME(0);

}
        #endif

DisposeTopLevel(theMG);
{
  INT FromLevel = TOPLEVEL(theMG)-1;
  INT ToLevel = TOPLEVEL(theMG);

  if (FromLevel>=0) {

    /* identify multiply created objects */
    IdentifyGridLevels(theMG,FromLevel,ToLevel);

    /* create one-element-overlapping for multigrid */
    CreateGridOverlap(theMG,FromLevel);
    ConnectNewOverlap(theMG,FromLevel);
    dddif_SetBorderPriorities(GRID_ON_LEVEL(theMG,TOPLEVEL(theMG)));
  }
}
        #endif

if (CreateAlgebra(theMG) != GM_OK)
  REP_ERR_RETURN (GM_ERROR);


/* set grid status of grid 0 */
RESETGSTATUS(GRID_ON_LEVEL(theMG,0),GRID_CHANGED);

IFDEBUG(gm,1)
UserWrite("END RefineMultiGrid():\n");
j = TOPLEVEL(theMG);
for (k=j; k>=0; k--)
{
  theGrid = GRID_ON_LEVEL(theMG,k);
  for (theElement=FIRSTELEMENT(theGrid); theElement!=NULL; theElement=SUCCE(theElement))
    UserWriteF("EID=%d TAG=%d ECLASS=%d RefineClass=%d MarkClass=%d "\
               "Refine=%d Mark=%d Coarse=%d\n",ID(theElement),TAG(theElement),\
               ECLASS(theElement),REFINECLASS(theElement),MARKCLASS(theElement),\
               REFINE(theElement),MARK(theElement),COARSEN(theElement));
}
ENDDEBUG
"%d (%d green marks)\n",No_Green_Update,Green_Marks);

UserWriteF(" Number of green refinements not updated: %d (%d green marks)\n",No_Green_Update,Green_Marks);
/*
        /* reset status */
RESETMGSTATUS(theMG);
}
