// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*                                                                          */
/* File:      ident.c                                                       */
/*                                                                          */
/* Purpose:   object identification for ddd module                          */
/*                                                                          */
/* Author:    Klaus Birken                                                  */
/*            Rechenzentrum Uni Stuttgart                                   */
/*            Universitaet Stuttgart                                        */
/*            Allmandring 30                                                */
/*            70550 Stuttgart                                               */
/*            internet: birken@rus.uni-stuttgart.de                         */
/*                                                                          */
/* History:   93/11/30 kb  begin                                            */
/*            94/04/25 kb  major revision, all Identfy-functions impl.      */
/*            96/02/08 kb  fixed bug in vchannel handling (due to T3D)      */
/*            96/11/25 kb  added indirect identification                    */
/*                                                                          */
/* Remarks:                                                                 */
/*                                                                          */
/****************************************************************************/

#define DebugIdent 10  /* 10 is off */



/*
        in debuglevel DebugIdentCons, addititional data is sent with
        the identify-messages, in order to check the consistency of
        the identification tupels.
 */
#define DebugIdentCons  8



/****************************************************************************/
/*                                                                          */
/* include files                                                            */
/*            system include files                                          */
/*            application include files                                     */
/*                                                                          */
/****************************************************************************/

/* standard C library */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "dddi.h"

#include "basic/notify.h"



/****************************************************************************/
/*                                                                          */
/* defines in the following order                                           */
/*                                                                          */
/*        compile time constants defining static data size (i.e. arrays)    */
/*        other constants                                                   */
/*        macros                                                            */
/*                                                                          */
/****************************************************************************/


/* types of IDENTINFO items, ID_OBJECT must be the smallest value!! */
#define ID_OBJECT   1
#define ID_NUMBER   2
#define ID_STRING   3


#define TUPEL_LEN(t)    ((t)&0x3f)


/* overall mode of identification */
enum IdentMode {
  IMODE_IDLE = 0,          /* waiting for next DDD_IdentifyBegin() */
  IMODE_CMDS,              /* after DDD_IdentifyBegin(), before DDD_IdentifyEnd() */
  IMODE_BUSY               /* during DDD_IdentifyEnd() */
};



/****************************************************************************/
/*                                                                          */
/* data structures                                                          */
/*                                                                          */
/****************************************************************************/


/****************************************************************************/
/* IDENTIFIER:                                                              */
/****************************************************************************/

typedef union {
  int number;
  char       *string;
  DDD_GID object;
} IDENTIFIER;



/****************************************************************************/
/* MSGITEM:                                                                 */
/****************************************************************************/

typedef struct {
  DDD_GID gid;
  DDD_PRIO prio;

#       if DebugIdent<=DebugIdentCons
  unsigned long tupel;         /* send tupel ID for checking consistency */
#       endif

} MSGITEM;



/****************************************************************************/
/* IDENTINFO:                                                               */
/****************************************************************************/

typedef struct _ID_REFDBY
{
  struct _IDENTINFO  *by;
  struct _ID_REFDBY  *next;
} ID_REFDBY;


typedef struct _IDENTINFO {
  int typeId;
  unsigned long tupel;
  int entry;
  IDENTIFIER id;


  MSGITEM msg;                     /* this item is sent to other procs */

  DDD_HDR hdr;

  struct _IDENTINFO  *tupel_head;

  /* information only for tupel_head */
  /* NOTE: other tupel-items MUST contain same information,
     at least when they are switched to tupel_head during
     SortTupelObjectItems() !
   */
  int nObjIds;                    /* number of entries with typeID==ID_OBJECT */
  int loi;                        /* level of indirection */
  ID_REFDBY     *refd;            /* list of referencing ID_ENTRIES */

} IDENTINFO;



/****************************************************************************/
/* ID_ENTRY:                                                                */
/****************************************************************************/


typedef struct _ID_ENTRY {
  IDENTINFO msg;
  struct _ID_ENTRY *next;

} ID_ENTRY;



/****************************************************************************/
/* ID_PLIST:                                                                */
/****************************************************************************/

typedef struct _ID_PLIST {
  DDD_PROC proc;
  int entries;
  int nIdentObjs;

  struct _ID_PLIST *next;
  ID_ENTRY    *first;

  IDENTINFO   *local_ids;
  IDENTINFO   **indexmap;               /* index-mapping of local_ids array */

  MSGITEM     *msgin, *msgout;
  msgid idin, idout;
} ID_PLIST;


/****************************************************************************/
/*                                                                          */
/* definition of exported global variables                                  */
/*                                                                          */
/****************************************************************************/



/****************************************************************************/
/*                                                                          */
/* definition of variables global to this source file only (static!)        */
/*                                                                          */
/****************************************************************************/

/* Revision Control System string */
RCSID("$Header$",DDD_RCS_STRING)


static ID_PLIST   *thePLists;
static int cntIdents, nPLists;

static int identMode;



/****************************************************************************/
/*                                                                          */
/* routines                                                                 */
/*                                                                          */
/****************************************************************************/
/*
        management functions for IdentMode.

        these functions control the mode the ident-module is
        currently in. this is used for error detection, but
        also for correct detection of coupling inconsistencies
        and recovery.
 */

static char *IdentModeName (int mode)
{
  switch(mode)
  {
  case IMODE_IDLE :
    return "idle-mode";
    break;
  case IMODE_CMDS :
    return "commands-mode";
    break;
  case IMODE_BUSY :
    return "busy-mode";
    break;
  }
  return "unknown-mode";
}


static void IdentSetMode (int mode)
{
  identMode = mode;

#       if DebugIdent<=8
  sprintf(cBuffer, "%4d: IdentMode=%s.\n",
          me, IdentModeName(identMode));
  DDD_PrintDebug(cBuffer);
#       endif
}


static int IdentSuccMode (int mode)
{
  switch(mode)
  {
  case IMODE_IDLE : return IMODE_CMDS; break;
  case IMODE_CMDS : return IMODE_BUSY; break;
  case IMODE_BUSY : return IMODE_IDLE; break;
  }
  return IMODE_IDLE;
}



static int IdentMode (void)
{
  return identMode;
}


static int IdentActive (void)
{
  return identMode!=IMODE_IDLE;
}


static int IdentStepMode (int old)
{
  if (identMode!=old)
  {
    sprintf(cBuffer, "wrong ident-mode (currently in %s, expected %s)",
            IdentModeName(identMode), IdentModeName(old));
    DDD_PrintError('E', 3070, cBuffer);
    return FALSE;
  }

  IdentSetMode(IdentSuccMode(identMode));
  return TRUE;
}


/****************************************************************************/


static void PrintPList (ID_PLIST *plist)
{
  sprintf(cBuffer, "%d: PList proc=%04d entries=%05d\n",
          me, plist->proc, plist->entries);
  DDD_PrintDebug(cBuffer);
}


/****************************************************************************/


static int compareId (IDENTINFO *el1, IDENTINFO *el2)
{
  int cmp;

  /* first compare id type (NUMBER, STRING or OBJECT) */
  if (el1->typeId < el2->typeId) return(-1);
  if (el1->typeId > el2->typeId) return(1);


  /* same typeIds, compare identification then */
  switch (el1->typeId) {
  case ID_NUMBER :
    if (el1->id.number < el2->id.number) return(-1);
    if (el1->id.number > el2->id.number) return(1);
    break;

  case ID_STRING :
    cmp = strcmp(el1->id.string, el2->id.string);
    if (cmp!=0) return(cmp);
    break;

  case ID_OBJECT :
    if (el1->id.object < el2->id.object) return(-1);
    if (el1->id.object > el2->id.object) return(1);
    break;
  }

  return(0);
}



/****************************************************************************/

/*
        two functions for sorting IdentifyXXX-requests into tupels:

        sort_intoTupelsLists keeps order of IdentifyXXX-issueing by
           application program, i.e., the ordering is relevant

        sort_intoTupelsSets reorders the IdentifyXXX-items inside each
           tupel itself;
           at this level the ordering is done only by typeId, where
           ID_OBJECT comes first. lateron the IdentifyObject-items will
           be sorted according to their gid (for objects with loi==0)
           or the index of the loi-1 object (for objects with loi>0)
 */


static int sort_intoTupelsLists (const void *e1, const void *e2)
{
  IDENTINFO       *el1, *el2;

  el1 = (IDENTINFO *) e1;
  el2 = (IDENTINFO *) e2;


  /* sort according to (old) global ids */
  if (el1->msg.gid < el2->msg.gid) return(-1);
  if (el1->msg.gid > el2->msg.gid) return(1);

  /* if equal, keep ordering of input  */
  if (el1->entry < el2->entry) return(-1);
  if (el1->entry > el2->entry) return(1);

  return(0);
}


static int sort_intoTupelsSets (const void *e1, const void *e2)
{
  IDENTINFO       *el1, *el2;

  el1 = (IDENTINFO *) e1;
  el2 = (IDENTINFO *) e2;


  /* sort according to (old) global ids */
  if (el1->msg.gid < el2->msg.gid) return(-1);
  if (el1->msg.gid > el2->msg.gid) return(1);

  /* if equal, sort according to identificator itself */
  return (compareId(el1,el2));
}




/****************************************************************************/


static int sort_loi (const void *e1, const void *e2)
{
  IDENTINFO       *el1, *el2;
  int cmp, i, nIds;

  el1 = *((IDENTINFO **) e1);
  el2 = *((IDENTINFO **) e2);

  /* sort according to level-of-indirection */
  if (el1->loi < el2->loi) return(-1);
  if (el1->loi > el2->loi) return(1);

  return(0);
}

static int sort_tupelOrder (const void *e1, const void *e2)
{
  IDENTINFO       *el1, *el2;
  int cmp, i, nIds;

  el1 = *((IDENTINFO **) e1);
  el2 = *((IDENTINFO **) e2);


  /* sort according to tupel id */
  if (el1->tupel < el2->tupel) return(-1);
  if (el1->tupel > el2->tupel) return(1);

  /* ids are equal, sort according tupel value */

  /* recode tupel length from lowest 6 bits */
  nIds = TUPEL_LEN(el1->tupel);


  /* compare until one tupel entry differs */
  for(i=0; i<nIds; i++) {
    if ((cmp=compareId(&(el1[i]), &(el2[i]))) != 0)
      return(cmp);
  }


  /* if tupels are equal by all means up to now, we
     sort according to DDD_TYPE of local object.
     hence, we can identify two pairs of local objects with
     the same tupel.
     this has to be ommitted if objects with different
     types should be identifiable. KB 960814
   */
  if (OBJ_TYPE(el1->hdr) < OBJ_TYPE(el2->hdr)) return(-1);
  if (OBJ_TYPE(el1->hdr) > OBJ_TYPE(el2->hdr)) return(1);


  if (el1->hdr!=el2->hdr)
  {
    sprintf(cBuffer, "same identification tupel for objects %08x and %08x",
            OBJ_GID(el1->hdr), OBJ_GID(el2->hdr));
    DDD_PrintError('E', 3030, cBuffer);
    exit(1);
  }

  return(0);
}


/****************************************************************************/

static void SetLOI (IDENTINFO *ii, int loi)
{
  ID_REFDBY *rby;
  IDENTINFO *head = ii->tupel_head;

  /*
     printf("%4d: %08x SetLOI(%d, %d)\n", me, ii->msg.gid, loi, ii->loi);
   */

  /* set loi to maximum of current and new value */
  head->loi = MAX(loi, head->loi);

  /* primitive cycle detection */
  if (head->loi > 64)
  {
    sprintf(cBuffer, "IdentifyObject-cycle, objects %08x and %08x",
            ii->msg.gid, ii->id.object);
    DDD_PrintError('E', 3310, cBuffer);
    exit(1);
  }


  for(rby=head->refd; rby!=NULL; rby=rby->next)
  {
    SetLOI(rby->by, loi+1);

    /* TODO detection of cycles */
  }
}



static int sort_refd_gid (const void *e1, const void *e2)
{
  DDD_GID g1, g2;

  g1 = (*(IDENTINFO **) e1)->id.object;
  g2 = (*(IDENTINFO **) e2)->id.object;

  /* sort according to global id */
  if (g1 < g2) return(-1);
  if (g1 > g2) return(1);

  return(0);
}



static void ResolveDependencies (
  IDENTINFO **tupels, int nTupels,
  IDENTINFO *id, int nIds, int nIdentObjs)
{
  IDENTINFO **refd;
  int i, j;

  refd = (IDENTINFO **) AllocTmp(sizeof(IDENTINFO *)*nIdentObjs);
  if (refd==NULL) {
    DDD_PrintError('E', 3300, "not enough memory in ResolveDependencies");
    return;
  }

  /* build array of pointers to objects being used for identification */
  for(i=0, j=0; i<nIds; i++)
  {
    if (id[i].typeId==ID_OBJECT)
    {
      refd[j] = &(id[i]);
      j++;
    }
  }

  /* sort it according to GID of referenced objects */
  qsort(refd, nIdentObjs, sizeof(IDENTINFO *), sort_refd_gid);


  /*
     for(j=0; j<nIdentObjs; j++)
          printf("%4d: DepObj %08x  (from %08x)\n",
                  me, refd[j]->id.object, OBJ_GID(refd[j]->hdr));
   */


  for(i=0, j=0; i<nTupels; i++)
  {
    while (j<nIdentObjs && refd[j]->id.object < tupels[i]->msg.gid)
      j++;

    while (j<nIdentObjs && refd[j]->id.object == tupels[i]->msg.gid)
    {
      ID_REFDBY *rby = (ID_REFDBY *)AllocTmp(sizeof(ID_REFDBY));
      /* TODO memory management, error checking */

      /* remember that idp[i] is referenced by refd[j] */
      rby->by      = refd[j];
      rby->next    = tupels[i]->refd;
      tupels[i]->refd = rby;

      j++;
    }
  }

  FreeTmp(refd);


  for(i=0; i<nTupels; i++)
  {
    ID_REFDBY *rby;

    for(rby=tupels[i]->refd; rby!=NULL; rby=rby->next)
    {
      /* if loi>0, this subtree has been loi-ed before */
      if (tupels[i]->loi==0)
        SetLOI(rby->by, tupels[i]->loi+1);
    }
  }


#       if DebugIdent<=2
  /* display */
  for(i=0; i<nTupels; i++)
  {
    ID_REFDBY *rby;

    printf("%4d: %08x has loi %d\n",
           me, tupels[i]->msg.gid, tupels[i]->loi);

    for(rby=tupels[i]->refd; rby!=NULL; rby=rby->next)
    {
      printf("%4d: %08x referenced by %08x\n",
             me, tupels[i]->msg.gid, rby->by->msg.gid);
    }
  }
#       endif
}


static void CleanupLOI (IDENTINFO **tupels, int nTupels)
{
  int i;

  for(i=0; i<nTupels; i++)
  {
    ID_REFDBY *rby, *next;

    for(rby=tupels[i]->refd; rby!=NULL; rby=next)
    {
      next = rby->next;

      /* TODO use freelists */
      FreeTmp(rby);
    }
  }
}


/****************************************************************************/


/*
        tupel-id doesn't contain information about the data in
        the tupel, it does only contain information about the
        structure of a tupel!
 */
static void TupelId (IDENTINFO *id, int nIds)
{
  int i, nObjIds;
  unsigned long tId;

  /* compute tupel id */
  tId = 0;
  nObjIds = 0;
  for(i=0; i<nIds; i++)
  {
    tId = (tId<<2) | id[i].typeId;

    /* count entries with ID_OBJECT */
    if (id[i].typeId==ID_OBJECT)
      nObjIds++;
  }

  /* set number of entries with ID_OBJECT */
  id->nObjIds = nObjIds;


  /* code length of tupel into lowest 6 bits */
  tId = (tId<<6) | nIds;

  /* printf("%4d: compute tupel id = %08x\n", me, tId); */


  /* mark items with tupel id and set first in tupel */
  for(i=0; i<nIds; i++)
  {
    id[i].tupel      = tId;
    id[i].tupel_head = id;
  }
}



/*
        reorder the section of a tupel's IdentifyObject entries.
        assumption: the IdentifyObject-entries are already sorted according
        to their id.object entry (ascending order).
        the entry 'id' has been changed, therefore its position in the
        IdentifyObject-section of the tupel could have changed.
        find new position by linear search. avoid using qsort because the number
        of entries will be small.
 */
static void SortTupelObjectItems (IDENTINFO *id)
{
  IDENTINFO *head = id->tupel_head;
  IDENTINFO bubble = *id;
  int i, pos = id-head;

  /* move down if necessary */
  if (pos > 0)
  {
    /* backwards from id's position until tupel_head is reached */
    for(i=pos; i>0; i--)
    {
      if (bubble.id.object>=head[i-1].id.object)
      {
        /* reached correct position, we are ready */
        break;
      }
      else
      {
        /* otherwise bubble item down, bubble head[i-1] up */
        head[i] = head[i-1];
      }
    }
    if (i<pos)
    {
      /* insert bubble at gap */
      head[i] = bubble;
      pos = i;
    }
    if (i==0)
    {
      /* tupel_head has been moved up, copy tupel_head-data
         to new tupel_head! */
      head[0].nObjIds = head[1].nObjIds;
      head[0].loi     = head[1].loi;
      head[0].refd    = head[1].refd;
      head[1].refd = NULL;                    /* refd-list anchor is moved to head! */
    }
  }
  /* else:  id IS already tupel_head, we dont have to move down */

  /* move up if necessary */
  if (pos < head->nObjIds-1)
  {
    /* forward from id's position until last ID_OBJECT is reached */
    for(i=pos; i< head->nObjIds-1; i++)
    {
      if (bubble.id.object<=head[i+1].id.object)
      {
        /* reached correct position, we are ready */
        break;
      }
      else
      {
        /* otherwise bubble item up, bubble head[i+1] down */
        head[i] = head[i+1];
      }
    }
    if (i>pos)
    {
      /* insert bubble at gap */
      head[i] = bubble;
      pos = i;
    }
  }
  /* else:  id IS already last item, we dont have to move up */
}



static int IdentifySort (IDENTINFO *id, int nIds,
                         int nIdentObjs, MSGITEM *items_out, IDENTINFO ***indexmap_out)
{
  IDENTINFO **idp;
  int i, j, last, nTupels;
  int keep_order_inside_tupel;

  /* sort to recognize identification tupels */
  /* in case of IDMODE_LISTS, the original ordering
     inside each tupel is kept. for IDMODE_SETS, each tupel
     is sorted according to the identificators themselves. */
  switch (DDD_GetOption(OPT_IDENTIFY_MODE))
  {
  case IDMODE_LISTS :
    qsort(id, nIds, sizeof(IDENTINFO), sort_intoTupelsLists);
    keep_order_inside_tupel = TRUE;
    break;

  case IDMODE_SETS :
    qsort(id, nIds, sizeof(IDENTINFO), sort_intoTupelsSets);
    keep_order_inside_tupel = FALSE;
    break;

  default :
    DDD_PrintError('E', 3330, "unknown OPT_IDENTIFY_MODE");
    exit(1);
  }


  /* compute tupel id for all items and mark items */
  for(i=0, last=0, nTupels=0; i<nIds; i++)
  {
    if (id[i].msg.gid > id[last].msg.gid)
    {
      TupelId(&(id[last]), i-last);
      nTupels++;
      last=i;
    }
  }
  TupelId(&(id[last]), nIds-last);
  nTupels++;


  /* construct array with one pointer onto each tupel */
  idp = (IDENTINFO **) AllocTmp(sizeof(IDENTINFO *)*nTupels);
  if (idp==NULL) {
    DDD_PrintError('E', 3000, "not enough memory in IdentifySort");
    return(0);
  }

  for(i=0, j=0; j<nTupels; j++) {
    idp[j] = &(id[i]);
    do {
      i++;
    } while (i<nIds && id[i].msg.gid==id[i-1].msg.gid);
  }

  /*
          now, idp is an array with a pointer onto each tupel,
          sorted according the gid of the object the tupel has
          been specified for.

          i.e., idp is a list of object gids which will be identified.
   */

  /* resolve dependencies caused by IdentifyObject,
     and set level-of-indirection accordingly */
  ResolveDependencies(idp, nTupels, id, nIds, nIdentObjs);


  /*
          the setting for loi is used for the next sorting procedure,
          first level of indirection comes first.
   */

  /* sort array for loi */
  qsort(idp, nTupels, sizeof(IDENTINFO *), sort_loi);


  i=0; j=0;
  do {
    while (j<nTupels && idp[i]->loi==idp[j]->loi)
      j++;

    /* sort sub-array for tupelId, tupelValue */
    if (j-i > 1)
      qsort(idp+i, j-i, sizeof(IDENTINFO *), sort_tupelOrder);

    /* inherit index to tupels referencing this one */
    while (i<j)
    {
      ID_REFDBY *rby;

      for(rby=idp[i]->refd; rby!=NULL; rby=rby->next)
      {
        /* dont use gid of referenced object (because it
           will be known only after identification), but its
           position in the identification table instead! */
        rby->by->id.object = i;

        /* now the tupel value has been changed! */
        if (! keep_order_inside_tupel)
        {
          /* if the ordering is not significant, we must reorder
             the tupel here (i.e., for IDMODE_SETS).
           */
          /* NOTE: each tupel will be reordered for each reference,
             although it would be enough to order it after the
             last change from the above program line. we ignore
             this here, because performance will not suffer
             except for VERY complicated id tupels. */
          SortTupelObjectItems(rby->by);
        }
      }

      i++;
    }
    /* now i==j */

  } while (i<nTupels);


  /* construct array which will be sent actually */
  for(j=0; j<nTupels; j++)
  {
#               if DebugIdent<=1
    printf("%4d: msg_idx[%08x] = %5d, loi=%d\n",
           me, idp[j]->msg.gid, j, idp[j]->loi);
#               endif

    items_out[j] = idp[j]->msg;

#               if DebugIdent<=DebugIdentCons
    /* send additional data for cons-checking */
    items_out[j].tupel = idp[j]->tupel;
#               endif
  }


  CleanupLOI(idp, nTupels);

  /* return indexmap table, in order to keep ordering of tupels */
  /* note: this array has to be freed in the calling function! */
  *indexmap_out = idp;


  return(nTupels);
}



static int InitComm (int nPartners)
{
  ID_PLIST  *plist;
  int i, err;
  DDD_PROC  *partners = DDD_ProcArray();

  /* fill partner processor numbers into array */
  for(plist=thePLists, i=0; i<nPartners; i++, plist=plist->next)
    partners[i] = plist->proc;

  DDD_GetChannels(nPartners);


  /* initiate asynchronous receives and sends */
  for(plist=thePLists; plist!=NULL; plist=plist->next)
  {
    plist->idin = RecvASync(VCHAN_TO(plist->proc),
                            plist->msgin, sizeof(MSGITEM)*plist->entries, &err);

    plist->idout = SendASync(VCHAN_TO(plist->proc),
                             plist->msgout, sizeof(MSGITEM)*plist->entries, &err);
  }

  return TRUE;
}


/****************************************************************************/

/*
        this routine checks whether number of idents per proc is indeed
        pairwise consistent.
 */

static idcons_CheckPairs (void)
{
  NOTIFY_DESC *msgs = DDD_NotifyBegin(nPLists);
  ID_PLIST        *plist;
  int i, j, nRecvs, err=FALSE;

  for(i=0, plist=thePLists; plist!=NULL; plist=plist->next, i++)
  {
    msgs[i].proc = plist->proc;
    msgs[i].size = plist->entries;
  }

  /* communicate */
  nRecvs = DDD_Notify();

  /* perform checking */
  for(plist=thePLists; plist!=NULL; plist=plist->next)
  {
    for(j=0; j<nRecvs; j++)
    {
      if (msgs[j].proc==plist->proc)
        break;
    }

    if (j==nRecvs)
    {
      sprintf(cBuffer,
              "Identify: no Ident-calls from proc %d, expected %d\n",
              plist->proc, plist->entries);
      DDD_PrintError('E', 3900, cBuffer);
      err=TRUE;
    }
    else
    {
      if (msgs[j].size!=plist->entries)
      {
        sprintf(cBuffer,
                "Identify: %d Ident-calls from proc %d, expected %d\n",
                msgs[j].size, plist->proc, plist->entries);
        DDD_PrintError('E', 3901, cBuffer);
        err=TRUE;
      }
    }
  }

  DDD_NotifyEnd();

  if (err)
  {
    DDD_PrintError('E', 3908, "found errors in IdentifyEnd()");
    exit(1);
  }
  else
  {
    DDD_PrintError('W', 3909, "Ident-ConsCheck level 0: ok.");
  }
}



/****************************************************************************/


void DDD_IdentifyEnd (void)
{
  ID_PLIST        *plist;
  ID_ENTRY        *id;
  int i, cnt, j;

#       if DebugIdent<=9
  printf("%4d: DDD_IdentifyEnd.\n", me);
  fflush(stdout);
#       endif

  /* step mode and check whether call to IdentifyEnd is valid */
  if (!IdentStepMode(IMODE_CMDS))
  {
    DDD_PrintError('E', 3071, "DDD_IdentifyEnd() aborted.");
    exit(1);
  };


#   if DebugIdent<=9
  idcons_CheckPairs();
#       endif


  /* for each id_plist entry */
  for(plist=thePLists, cnt=0; plist!=NULL; plist=plist->next, cnt++)
  {
    /* allocate message buffers */
    /* use one alloc for three buffers */
    plist->local_ids = (IDENTINFO *) AllocTmp(
      sizeof(IDENTINFO)*plist->entries +                    /* for local id-infos */
      sizeof(MSGITEM)  *plist->entries +                    /* for incoming msg   */
      sizeof(MSGITEM)  *plist->entries                      /* for outgoing msg   */
      );

    if (plist->local_ids==NULL)
    {
      DDD_PrintError('F',3100,"not enough memory in DDD_IdentifyEnd");
      exit(1);
    }
    plist->msgin  = (MSGITEM *) &plist->local_ids[plist->entries];
    plist->msgout =             &plist->msgin[plist->entries];


    /* construct contiguous buffer */
    /* TODO this may be a time-consuming step */
    for(id=plist->first, i=0; id!=NULL; id=id->next, i++)
      plist->local_ids[i] = id->msg;


    /* sort outgoing items */
    plist->entries = IdentifySort(plist->local_ids, plist->entries,
                                  plist->nIdentObjs,
                                  plist->msgout,   /* output: msgbuffer outgoing */
                                  &plist->indexmap); /* output: mapping of indices to local_ids array */


#               if DebugIdent<=5
    PrintPList(plist);
#               endif
  }

  /* initiate comm-channels and send/receive calls */
  if (!InitComm(cnt))
    exit(1);


  /*
          each pair of processors now has a plist with one copy on
          each side. the actual OBJ_GID is computed as the minimum
          of the two local object ids on each processor.
   */

#       if DebugIdent<=4
  printf("%4d: DDD_IdentifyEnd. PLists ready.\n", me); fflush(stdout);
#       endif


  /* poll receives */
  for(plist=thePLists, j=0; j<cnt; )
  {
    if (plist->msgin!=NULL)
    {
      if (InfoARecv(VCHAN_TO(plist->proc), plist->idin)==1)
      {
        /* process single plist */
        MSGITEM   *msgin  = plist->msgin;
        IDENTINFO **msgout = plist->indexmap;

        for(i=0; i<plist->entries; i++, msgin++, msgout++)
        {
#                                       if DebugIdent<=1
          printf("%4d: identifying %08x with %08x to %08x\n", me,
                 OBJ_GID((*msgout)->hdr), msgin->gid,
                 MIN(OBJ_GID((*msgout)->hdr), msgin->gid));
#                                       endif

#                                       if DebugIdent<=DebugIdentCons
          if ((*msgout)->tupel != msgin->tupel)
          {
            sprintf(cBuffer, "inconsistent tupels, "
                    "gid %08x on %d, gid %08x on %d,"
                    " in DDD_IdentifyEnd()",
                    OBJ_GID((*msgout)->hdr), me,
                    msgin->gid, plist->proc);
            DDD_PrintError('E', 3920, cBuffer);
            exit(1);
          }
#                                       endif

          /* compute new GID from minimum of both current GIDs */
          OBJ_GID((*msgout)->hdr) =
            MIN(OBJ_GID((*msgout)->hdr), msgin->gid);

          /* add a coupling for new object copy */
          AddCoupling((*msgout)->hdr, plist->proc, msgin->prio);
        }

        /* free indexmap array */
        FreeTmp(plist->indexmap);

        /* mark plist as finished */
        plist->msgin=NULL;
        j++;
      }
    }

    /* next plist, perhaps restart */
    plist=plist->next; if (plist==NULL) plist=thePLists;
  };


  /* poll sends */
  for(plist=thePLists; plist!=0; plist=plist->next)
  {
    /* wait for correct send and free buffer */
    while(InfoASend(VCHAN_TO(plist->proc), plist->idout)!=1)
      ;
    FreeTmp(plist->local_ids);
  };


#       if DebugIdent<=8
  printf("%4d: DDD_IdentifyEnd. Rebuilding interfaces.\n", me);
  fflush(stdout);
#       endif


  /* rebuild interfaces after topological change */
  IFAllFromScratch();


#       if DebugIdent<=9
  printf("%4d: DDD_IdentifyEnd. Ready.\n", me); fflush(stdout);
#       endif


  IdentStepMode(IMODE_BUSY);
}



/****************************************************************************/


static void IdentifyIdEntry (DDD_HDR hdr, ID_ENTRY *id, DDD_PROC proc)
{
  ID_PLIST        *plist, *pnew;

  /* step mode and check whether Identify-call is valid */
  if (!IdentActive())
  {
    DDD_PrintError('E', 3072, "Missing DDD_IdentifyBegin(), aborted");
    exit(1);
  }

  if (proc==me)
  {
    sprintf(cBuffer, "cannot identify %08x with myself", OBJ_GID(hdr));
    DDD_PrintError('E', 3060, cBuffer);
    exit(1);
  }

  if (proc>=procs)
  {
    sprintf(cBuffer, "cannot identify %08x with processor %d",
            OBJ_GID(hdr), proc);
    DDD_PrintError('E', 3061, cBuffer);
    exit(1);
  }


#       if DebugIdent<=2
  switch (id->msg.typeId)
  {
  case ID_NUMBER :
    printf("%4d: IdentifyIdEntry %08x %4d %d\n", me, OBJ_GID(hdr), proc, id->msg.id.number);
    break;

  case ID_STRING :
    printf("%4d: IdentifyIdEntry %08x %4d %s\n", me, OBJ_GID(hdr), proc, id->msg.id.string);
    break;

  case ID_OBJECT :
    printf("%4d: IdentifyIdEntry %08x %4d %d\n", me, OBJ_GID(hdr), proc, id->msg.id.object);
    break;
  }
#       endif

  id->msg.hdr      = hdr;
  id->msg.msg.gid  = OBJ_GID(hdr);
  id->msg.msg.prio = OBJ_PRIO(hdr);

  id->next     = NULL;
  id->msg.entry= cntIdents++;

  id->msg.loi  = 0;
  id->msg.refd = NULL;

  /* search current plist entries */
  for(plist=thePLists; plist!=NULL; plist=plist->next) {
    if (plist->proc==proc)
      break;
  }

  if (plist==NULL)
  {
    /* get new id_plist record */
    pnew = (ID_PLIST *) AllocTmp(sizeof(ID_PLIST));
    if (pnew==NULL) {
      DDD_PrintError('F', 3210,
                     "not enough memory in IdentifyIdEntry");
      return;
    }

    pnew->proc = proc;
    pnew->entries = 1;
    pnew->first = id;
    pnew->nIdentObjs = 0;
    pnew->next = thePLists;
    thePLists = pnew;
    nPLists++;

    if (id->msg.typeId==ID_OBJECT)
    {
      pnew->nIdentObjs++;
    }
  }
  else
  {
    /* insert at current plist */
    id->next = plist->first;
    plist->first = id;
    plist->entries++;

    if (id->msg.typeId==ID_OBJECT)
    {
      plist->nIdentObjs++;
    }
  }
}


#ifdef C_FRONTEND
void DDD_IdentifyNumber (DDD_HDR hdr, DDD_PROC proc, int ident)
#else
void orgDDD_IdentifyNumber (DDD_HDR hdr, DDD_PROC proc, int ident);

void DDD_IdentifyNumber (DDD_TYPE *type, DDD_OBJ *obj, DDD_PROC *proc, int *ident)
{
  DDD_HDR hdr = OBJ2HDR(*obj,&(theTypeDefs[*type]));

  orgDDD_IdentifyNumber (hdr, *proc, *ident);
}

void orgDDD_IdentifyNumber (DDD_HDR hdr, DDD_PROC proc, int ident)
#endif
{
  ID_ENTRY        *id;

  id = (ID_ENTRY *) AllocTmp(sizeof(ID_ENTRY));
  if (id==NULL) {
    DDD_PrintError('F', 3200,
                   "not enough memory in DDD_IdentifyNumber");
    return;
  }


  id->msg.typeId = ID_NUMBER;
  id->msg.id.number = ident;

  IdentifyIdEntry(hdr, id, proc);
}



#ifdef C_FRONTEND
void DDD_IdentifyString (DDD_HDR hdr, DDD_PROC proc, char *ident)
#else
void orgDDD_IdentifyString (DDD_HDR hdr, DDD_PROC proc, char *ident);

void DDD_IdentifyString (DDD_TYPE *type, DDD_OBJ *obj, DDD_PROC *proc, char *ident)
{
  DDD_HDR hdr = OBJ2HDR(*obj,&(theTypeDefs[*type]));

  orgDDD_IdentifyString (hdr, *proc, ident);
}

void orgDDD_IdentifyString (DDD_HDR hdr, DDD_PROC proc, char *ident)
#endif
{
  ID_ENTRY        *id;

  id = (ID_ENTRY *) AllocTmp(sizeof(ID_ENTRY));
  if (id==NULL) {
    DDD_PrintError('F', 3201,
                   "not enough memory in DDD_IdentifyString");
    return;
  }

  id->msg.typeId = ID_STRING;
  id->msg.id.string = ident;

  IdentifyIdEntry(hdr, id, proc);
}


#ifdef C_FRONTEND
void DDD_IdentifyObject (DDD_HDR hdr, DDD_PROC proc, DDD_HDR ident)
#else
void orgDDD_IdentifyObject (DDD_HDR hdr, DDD_PROC proc, DDD_HDR ident);

void DDD_IdentifyObject (DDD_TYPE *type, DDD_OBJ *obj, DDD_PROC *proc, DDD_TYPE *type2, DDD_OBJ *obj2)
{
  DDD_HDR hdr = OBJ2HDR(*obj,&(theTypeDefs[*type]));
  DDD_HDR ident = OBJ2HDR(*obj2,&(theTypeDefs[*type2]));

  orgDDD_IdentifyObject (hdr, *proc, ident);
}

void orgDDD_IdentifyObject (DDD_HDR hdr, DDD_PROC proc, DDD_HDR ident)
#endif
{
  ID_ENTRY        *id;

  id = (ID_ENTRY *) AllocTmp(sizeof(ID_ENTRY));
  if (id==NULL) {
    DDD_PrintError('F', 3202,
                   "not enough memory in DDD_IdentifyObject");
    return;
  }

  id->msg.typeId = ID_OBJECT;

  /* use OBJ_GID as estimate for identification value, this estimate
     might be replaced when the corresponding object is identified
     itself. then its index in the identify-message will be used.
     remember identification value in order to replace above estimate,
     if necessary (i.e., remember ptr to ddd-hdr) */
  id->msg.id.object = OBJ_GID(ident);

  IdentifyIdEntry(hdr, id, proc);
}



void DDD_IdentifyBegin (void)
{
  /* step mode and check whether call to IdentifyBegin is valid */
  if (!IdentStepMode(IMODE_IDLE))
  {
    DDD_PrintError('E', 3073, "DDD_IdentifyBegin() aborted.");
    exit(1);
  }

  thePLists    = NULL;
  nPLists      = 0;
  cntIdents    = 0;
}


/****************************************************************************/


void ddd_IdentInit (void)
{
  IdentSetMode(IMODE_IDLE);
}


void ddd_IdentExit (void)
{}



/****************************************************************************/
