// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*                                                                          */
/* File:      fieldio.c                                                     */
/*                                                                          */
/* Purpose:   Field I/O commands                                            */
/*                                                                          */
/* Author:    Michael Lampe                                                 */
/*            IWR - Technische Simulation                                   */
/*            Universitaet Heidelberg                                       */
/*            Im Neuenheimer Feld 368                                       */
/*            69120 Heidelberg                                              */
/*            Email: Michael.Lampe@iwr.uni-heidelberg.de                    */
/*                                                                          */
/* History:   20011002 begin, ug3.8                                         */
/*                                                                          */
/* Remarks:                                                                 */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/*																			*/
/* include files															*/
/*			  system include files											*/
/*			  application include files                                                                     */
/*																			*/
/****************************************************************************/

#include <stdio.h>
#include <string.h>
#ifndef __MWCW__
#include <rpc/rpc.h>
#endif

#include "general.h"
#include "gm.h"
#include "evm.h"
#include "shapes.h"
#include "misc.h"
#include "cmdline.h"
#include "commands.h"
#include "ugenv.h"
#include "npscan.h"
#include "np.h"
#include "boxtree.h"
#include "fieldio.h"

/****************************************************************************/
/*																			*/
/* defines in the following order										        */
/*																			*/
/*		compile time constants defining static data size (i.e. arrays)		*/
/*		other constants												                */
/*		macros																*/
/*																			*/
/****************************************************************************/

#ifdef __MWCW__
#define ASCII  /* only ASCII */
#else
#undef ASCII   /* use ACSII or XDR */
#endif

#define MAGIC    "UGFI"
#define MAXVAR   50
#define MAXPROC  512

#define SURFACE_LOOP_BEGIN(mg, e) \
  { \
    int _i; \
    for (_i = 0; _i <= TOPLEVEL(mg); _i++) \
      for (e = FIRSTELEMENT(GRID_ON_LEVEL(mg, _i)); e != NULL; e = SUCCE(e)) { \
        if (!EstimateHere(e)) continue;

#define SURFACE_LOOP_END \
  } \
  }

/****************************************************************************/
/*                                                                          */
/* data structures used in this source file (exported data structures are   */
/*                in the corresponding include file!)                       */
/*                                                                          */
/****************************************************************************/

typedef struct {
  FILE *file;
#ifndef ASCII
  XDR xdr;
#endif
} STREAM;

typedef struct {
  char name[NAMESIZE];
  EVALUES *eval;
} SEVALUATOR;

typedef struct {
  char name[NAMESIZE];
  EVECTOR *eval;
} VEVALUATOR;

typedef struct {
  double x[DIM];
} VT_ARRAY;

typedef struct {
  int nc;
#ifdef __TWODIM__
  int corner[4];
#else
  int corner[8];
#endif
} EL_ARRAY;

typedef struct {
  BT_OBJECT bto;
  ELEMENT *e;
} MY_BT_OBJECT;

typedef struct {
  int no_es;
  int no_ev;
  SHORT *es;
  SHORT *ev;
  int nc;
  DOUBLE_VECTOR p[MAX_CORNERS_OF_ELEM];
  double scalar[MAXVAR];
  DOUBLE_VECTOR vector[MAXVAR];
} IE_DATA;

/****************************************************************************/
/*                                                                          */
/* definition of variables global to this source file only (static!)        */
/*                                                                          */
/****************************************************************************/

/* RCS string */
static char RCS_ID("$Header$",UG_RCS_STRING);

/****************************************************************************/
/*                                                                          */
/* forward declarations of functions used before they are defined           */
/*                                                                          */
/****************************************************************************/

static double AreaOfIntersection(COORD_POINT *a, int na,
                                 COORD_POINT *b, int nb);

/****************************************************************************/
/*                                                                          */
/*      simple I/O layer                                                    */
/*                                                                          */
/****************************************************************************/

static int OpenFile(const char *name, const char *mode, STREAM *stream)
{
  stream->file = fopen(name, mode);
  if (stream->file == NULL) return 1;
#ifndef ASCII
  switch (mode[0]) {
  case 'w' :
    xdrstdio_create(&(stream->xdr), stream->file, XDR_ENCODE);
    break;
  case 'r' :
    xdrstdio_create(&(stream->xdr), stream->file, XDR_DECODE);
    break;
  default :
    return 1;
  }
#endif
  return 0;
}

static int WriteInt(STREAM *stream, int x)
{
#ifdef ASCII
  fprintf(stream->file, "%d\n", x);
  if (ferror(stream->file)) return 1;
#else
  if (!xdr_int(&(stream->xdr), &x)) return 1;
#endif
  return 0;
}

static int ReadInt(STREAM *stream, int *x)
{
#ifdef ASCII
  fscanf(stream->file, "%d\n", x);
  if (ferror(stream->file)) return 1;
#else
  if(!xdr_int(&(stream->xdr), x)) return 1;
#endif
  return 0;
}

static int WriteDouble(STREAM *stream, double x)
{
#ifdef ASCII
  fprintf(stream->file, "%g\n", x);
  if (ferror(stream->file)) return 1;
#else
  if(!xdr_double(&(stream->xdr), &x)) return 1;
#endif
  return 0;
}

static int ReadDouble(STREAM *stream, double *x)
{
#ifdef ASCII
  fscanf(stream->file, "%lg\n", x);
  if (ferror(stream->file)) return 1;
#else
  if (!xdr_double(&(stream->xdr), x)) return 1;
#endif
  return 0;
}

/****************************************************************************/
/*                                                                          */
/*     Write field                                                          */
/*                                                                          */
/****************************************************************************/

static void ClearVertexMarkers(MULTIGRID *mg)
{
  VERTEX *v;
  int i;

  for (i = 0; i <= TOPLEVEL(mg); i++)
    for (v = FIRSTVERTEX(GRID_ON_LEVEL(mg, i)); v != NULL; v = SUCCV(v))
      SETUSED(v,0);
}

static void StatisticsW(MULTIGRID *mg, int *nv, int *ne, double range[DIM][2])
{
  ELEMENT *e;
  VERTEX *v;
  int i, j, n, m;

  n = m = 0;
  for (i = 0; i < DIM; i++) {
    range[i][0] =  MAX_D;
    range[i][1] = -MAX_D;
  }
  ClearVertexMarkers(mg);
  SURFACE_LOOP_BEGIN(mg, e)
  m++;
  for (i = 0; i < CORNERS_OF_ELEM(e); i++) {
    v = MYVERTEX(CORNER(e, i));
    if (USED(v)) continue;
    SETUSED(v , 1);
    for (j = 0; j < DIM; j++) {
      range[j][0] = MIN(range[j][0], v->iv.x[j]);
      range[j][1] = MAX(range[j][1], v->iv.x[j]);
    }
#ifdef ModelP
    ID(v) = n;
#endif
    n++;
  }
  SURFACE_LOOP_END
  *nv = n;
  *ne = m;
}

static int WriteVertices(STREAM *stream, MULTIGRID *mg, int nv, int *id2pos)
{
  ELEMENT *e;
  VERTEX *v;
  int i, j, n;

  n = 0;
  if (WriteInt(stream, nv)) return 1;
  ClearVertexMarkers(mg);
  SURFACE_LOOP_BEGIN(mg, e)
  for (i = 0; i < CORNERS_OF_ELEM(e); i++) {
    v = MYVERTEX(CORNER(e, i));
    if (USED(v)) continue;
    SETUSED(v , 1);
    for (j = 0; j < DIM; j++)
      if (WriteDouble(stream, v->iv.x[j])) return 1;
    id2pos[ID(v)] = n++;
  }
  SURFACE_LOOP_END
  return 0;
}

static int WriteElements(STREAM *stream, MULTIGRID *mg, int ne, int *id2pos)
{
  ELEMENT *e;
  int i, n;

  if (WriteInt(stream ,ne)) return 1;
  SURFACE_LOOP_BEGIN(mg, e)
  n = CORNERS_OF_ELEM(e);
  if (WriteInt(stream, n)) return 1;
  for (i = 0; i < n; i++)
    if (WriteInt(stream, id2pos[ID(MYVERTEX(CORNER(e, i)))])) return 1;
  SURFACE_LOOP_END
  return 0;
}

static int WriteNodeData(STREAM *stream, MULTIGRID *mg,
                         int no_ns, SEVALUATOR *se,
                         int no_nv, VEVALUATOR *ve)
{
  if (WriteInt(stream, 0 /* no_ns */)) return 1;
  if (WriteInt(stream, 0 /* no_nv */)) return 1;

  /* please fill in ... */

  return 0;
}

static int WriteElementData(STREAM *stream, MULTIGRID *mg,
                            int no_es, SEVALUATOR *se,
                            int no_ev, VEVALUATOR *ve)
{
  ELEMENT *e;
  PreprocessingProcPtr pre;
  ElementEvalProcPtr eval_s;
  ElementVectorProcPtr eval_v;
  double *cc[MAX_CORNERS_OF_ELEM], lc[DIM], lo[DIM], s, v[DIM];
  int i, j;

  if (WriteInt(stream, no_es)) return 1;
  if (WriteInt(stream, no_ev)) return 1;
  for (i = 0; i < no_es; i++) {
    pre = se[i].eval->PreprocessProc;
    if (pre != NULL) pre(se[i].name, mg);
  }
  for (i = 0; i < no_ev; i++) {
    pre = ve[i].eval->PreprocessProc;
    if (pre != NULL) pre(ve[i].name, mg);
  }
  SURFACE_LOOP_BEGIN(mg, e)
  for (i = 0; i < CORNERS_OF_ELEM(e); i++)
    cc[i] = CVECT(MYVERTEX(CORNER(e, i)));
  for (i = 0; i < DIM; i++)
    lc[i] = 0;
  for (i = 0; i < CORNERS_OF_ELEM(e); i++) {
    LocalCornerCoordinates(DIM, TAG(e), i, lo);
    for (j = 0; j < DIM; j++)
      lc[j] += lo[j];
  }
  for (i = 0; j < DIM; j++)
    lc[j] /= (double)CORNERS_OF_ELEM(e);
  for (i = 0; i < no_es; i++) {
    eval_s = se[i].eval->EvalProc;
    s = eval_s(e, (const DOUBLE **)cc, lc);
    if (WriteDouble(stream, s)) return 1;
  }
  for (i = 0; i < no_ev; i++) {
    eval_v = ve[i].eval->EvalProc;
    eval_v(e, (const DOUBLE **)cc, lc, v);
    for (j = 0; j < DIM; j++)
      if (WriteDouble(stream, v[j])) return 1;
  }
  SURFACE_LOOP_END

  return 0;
}

static int MagicW(STREAM *stream)
{
  fwrite(MAGIC, 1, sizeof(MAGIC)-1, stream->file);
  if (ferror(stream->file)) return 1;
  return 0;
}

static int BBoxW(STREAM *stream, double bbox[DIM][2])
{
  int i;

  for (i = 0; i < DIM; i++) {
    if (WriteDouble(stream, bbox[i][0])) return 1;
    if (WriteDouble(stream, bbox[i][1])) return 1;
  }
  return 0;
}

static int ParseArgumentsW(INT argc, char **argv, char *fname,
                           int *no_ns, SEVALUATOR *nse,
                           int *no_nv, VEVALUATOR *nve,
                           int *no_es, SEVALUATOR *ese,
                           int *no_ev, VEVALUATOR *eve)
{
  char s[NAMESIZE];
  int i;

  *no_ns = *no_nv = *no_es = *no_ev = 0;
  for(i=1; i<argc; i++) {
    if (strncmp(argv[i],"ns",2) == 0) {
      sscanf(argv[i],"ns %s", s);
      nse[*no_ns].eval = GetElementValueEvalProc(s);
      if (sscanf(argv[i+1],"s %s", s) == 1) {
        strcpy(nse[*no_ns].name, s);
        i++;
      }
      else
        strcpy(nse[*no_ns].name, nse[*no_ns].eval->v.name);
      (*no_ns)++;
    }
    else if (strncmp(argv[i],"nv",2) == 0) {
      sscanf(argv[i],"nv %s", s);
      nve[*no_nv].eval = GetElementVectorEvalProc(s);
      if (sscanf(argv[i+1],"s %s", s) == 1) {
        strcpy(nve[*no_nv].name, s);
        i++;
      }
      else
        strcpy(nve[*no_nv].name, nse[*no_nv].eval->v.name);
      (*no_nv)++;
    }
    else if (strncmp(argv[i],"es",2) == 0) {
      sscanf(argv[i],"es %s", s);
      ese[*no_es].eval = GetElementValueEvalProc(s);
      if (sscanf(argv[i+1],"s %s", s) == 1) {
        strcpy(ese[*no_es].name, s);
        i++;
      }
      else
        strcpy(ese[*no_es].name, ese[*no_es].eval->v.name);
      (*no_es)++;
    }
    else if (strncmp(argv[i],"ev",2) == 0) {
      sscanf(argv[i],"ev %s", s);
      eve[*no_ev].eval = GetElementVectorEvalProc(s);
      if (sscanf(argv[i+1],"s %s", s) == 1) {
        strcpy(eve[*no_ev].name, s);
        i++;
      }
      else
        strcpy(eve[*no_ev].name, eve[*no_ev].eval->v.name);
      (*no_ev)++;
    }
  }
  if (*no_ns == 0 && *no_nv == 0 && *no_es == 0 && *no_ev == 0)
    return 1;
  if (sscanf(argv[0],expandfmt(CONCAT3(" savefield %",NAMELENSTR,"[ -~]")), fname)!=1)
    return 1;
  sprintf(s, ".%04d", me);
  strcat(fname, s);
  return 0;
}

static INT SaveFieldCommand(INT argc, char **argv)
{
  MULTIGRID *mg;
  HEAP *heap;
  INT key;
  SEVALUATOR nse[MAXVAR];
  VEVALUATOR nve[MAXVAR];
  SEVALUATOR ese[MAXVAR];
  VEVALUATOR eve[MAXVAR];
  int no_ns, no_nv, no_es, no_ev;
  char fname[NAMESIZE];
  STREAM stream;
  int no_vertices, no_elements;
  double bbox[DIM][2];
  int *id2pos;

  mg = GetCurrentMultigrid();
  if (mg == NULL) {
    PrintErrorMessage('E', "savefield","no current multigrid\n");
    return CMDERRORCODE;
  }
  if (ParseArgumentsW(argc, argv, fname,
                      &no_ns, nse, &no_nv, nve,
                      &no_es, ese, &no_ev, eve)) {
    PrintErrorMessage('E', "savefield","wrong parameters\n");
    return CMDERRORCODE;
  }
  if (OpenFile(fname, "w", &stream)) {
    PrintErrorMessage('E', "savefield","cannot open output file\n");
    return CMDERRORCODE;
  }
  if (MagicW(&stream)) goto failed;
  StatisticsW(mg, &no_vertices, &no_elements, bbox);
  if (BBoxW(&stream, bbox)) goto failed;
  heap = mg->theHeap;
  MarkTmpMem(heap, &key);
  id2pos = (int *)GetTmpMem(heap, no_vertices*sizeof(int), key);
  if (id2pos == NULL) goto failed;
  if (WriteVertices(&stream, mg, no_vertices, id2pos)) goto failed;
  if (WriteElements(&stream, mg, no_elements, id2pos)) goto failed;
  ReleaseTmpMem(heap, key);
  if (WriteNodeData(&stream, mg, no_ns, nse, no_nv, nve)) goto failed;
  if (WriteElementData(&stream, mg, no_es, ese, no_ev, eve)) goto failed;
  fclose(stream.file);
  return OKCODE;
failed:
  PrintErrorMessage('E',"savefield", "something went wrong\n");
  return CMDERRORCODE;
}

/****************************************************************************/
/*                                                                          */
/*     Read field                                                           */
/*                                                                          */
/****************************************************************************/

static void StatisticsR(MULTIGRID *mg, int *ne, double range[DIM][2])
{
  ELEMENT *e;
  VERTEX *v;
  int i, j, n;

  n = 0;
  for (i = 0; i < DIM; i++) {
    range[i][0] =  MAX_D;
    range[i][1] = -MAX_D;
  }
  SURFACE_LOOP_BEGIN(mg, e)
  n++;
  for (i = 0; i < CORNERS_OF_ELEM(e); i++) {
    v = MYVERTEX(CORNER(e, i));
    for (j = 0; j < DIM; j++) {
      range[j][0] = MIN(range[j][0], v->iv.x[j]);
      range[j][1] = MAX(range[j][1], v->iv.x[j]);
    }
  }
  SURFACE_LOOP_END
  *ne = n;
}

static int ReadVertices(STREAM *stream, int no_vertices, VT_ARRAY *vt_array)
{
  int i, j;

  for (i = 0; i < no_vertices; i++) {
    for (j = 0; j < DIM; j++)
      if (ReadDouble(stream, &(vt_array->x[j]))) return 1;
    vt_array++;
  }
  return 0;
}

static int ReadElements(STREAM *stream, int no_elements, EL_ARRAY *el_array)
{
  int i, j, nc;

  for (i = 0; i < no_elements; i++) {
    if (ReadInt(stream, &nc)) return 1;
    el_array[i].nc = nc;
    for (j = 0; j < nc; j++)
      if (ReadInt(stream, &(el_array[i].corner[j]))) return 1;
  }
  return 0;
}

static int BBoxR(STREAM *stream, double bbox[DIM][2])
{
  int i;

  for (i = 0; i < DIM; i++) {
    if (ReadDouble(stream, &(bbox[i][0]))) return 1;
    if (ReadDouble(stream, &(bbox[i][1]))) return 1;
  }
  return 0;
}

static int MagicR(STREAM *stream)
{
  char buffer[NAMESIZE];

  fread(buffer, 1, sizeof(MAGIC)-1, stream->file);
  if (ferror(stream->file)) return 1;
  if (strncmp(buffer, MAGIC, sizeof(MAGIC)-1)) return 1;
  return 0;
}

static void BBoxOfElement(ELEMENT *e, double range[DIM][2])
{
  VERTEX *v;
  int i, j;

  for (i = 0; i < DIM; i++) {
    range[i][0] =  MAX_D;
    range[i][1] = -MAX_D;
  }
  for (i = 0; i < CORNERS_OF_ELEM(e); i++) {
    v = MYVERTEX(CORNER(e, i));
    for (j = 0; j < DIM; j++) {
      range[j][0] = MIN(range[j][0], v->iv.x[j]);
      range[j][1] = MAX(range[j][1], v->iv.x[j]);
    }
  }
}

static int MakeBoxTree(MULTIGRID *mg, int no_el, INT key, BOXTREE *tree)
{
  HEAP *heap;
  ELEMENT *e;
  MY_BT_OBJECT **array, **array0;

  heap = mg->theHeap;
  array = array0 = (MY_BT_OBJECT **)GetTmpMem(heap, no_el*sizeof(void *), key);
  if (array == NULL)
    return 1;
  SURFACE_LOOP_BEGIN(mg, e)
  *array = (MY_BT_OBJECT *)GetTmpMem(heap, sizeof(MY_BT_OBJECT), key);
  if (*array == NULL)
    return 1;
  (*array)->e = e;
  BBoxOfElement(e, (*array)->bto.range);
  array++;
  SURFACE_LOOP_END
  BT_Init((BT_OBJECT **)array0, no_el, tree);
  return 0;
}

static int ParseArgumentsR(int argc, char **argv, MULTIGRID *mg, char *fname,
                           int *no_ns, SHORT *ns,
                           int *no_nv, SHORT *nv,
                           int *no_es, SHORT *es,
                           int *no_ev, SHORT *ev)
{
  VECDATA_DESC *vd;
  INT dummy;
  int i;

  *no_ns = *no_nv = *no_es = *no_ev = 0;
  for(i=1; i<argc; i++) {
    if (strncmp(argv[i],"ns",2) == 0) {
      vd = ReadArgvVecDescX(mg, "ns", argc, argv, NO);
      if (vd == NULL) return 1;
      dset(mg, 0, TOPLEVEL(mg), ALL_VECTORS, vd, 0.0);
      ns[*no_ns] = VD_ncmp_cmpptr_of_otype(vd, NODEVEC, &dummy)[0];
      (*no_ns)++;
    }
    else if (strncmp(argv[i],"nv",2) == 0) {
      vd = ReadArgvVecDescX(mg, "nv", argc, argv, NO);
      if (vd == NULL) return 1;
      dset(mg, 0, TOPLEVEL(mg), ALL_VECTORS, vd, 0.0);
      nv[*no_nv] = VD_ncmp_cmpptr_of_otype(vd, NODEVEC, &dummy)[0];
      (*no_nv)++;
    }
    else if (strncmp(argv[i],"es",2) == 0) {
      vd = ReadArgvVecDescX(mg, "es", argc, argv, NO);
      if (vd == NULL) return 1;
      dset(mg, 0, TOPLEVEL(mg), ALL_VECTORS, vd, 0.0);
      es[*no_es] = VD_ncmp_cmpptr_of_otype(vd, ELEMVEC, &dummy)[0];
      (*no_es)++;
    }
    else if (strncmp(argv[i],"ev",2) == 0) {
      vd = ReadArgvVecDescX(mg, "ev", argc, argv, NO);
      if (vd == NULL) return 1;
      dset(mg, 0, TOPLEVEL(mg), ALL_VECTORS, vd, 0.0);
      ev[*no_ev] = VD_ncmp_cmpptr_of_otype(vd, ELEMVEC, &dummy)[0];
      (*no_ev)++;
    }
  }
  if (*no_ns == 0 && *no_nv == 0 && *no_es == 0 && *no_ev == 0)
    return 1;
  if (sscanf(argv[0],expandfmt(CONCAT3(" loadfield %",NAMELENSTR,"[ -~]")), fname)!=1)
    return 1;
  return 0;
}

static void IE_Callback(BT_OBJECT *o, void *d)
{
#if DIM==2
  MY_BT_OBJECT *bto;
  IE_DATA *data;
  ELEMENT *e;
  COORD_POINT a[4], b[4];
  double area_inters, area_dest;
  int i, j, na, nb;

  bto  = (MY_BT_OBJECT *)o;
  data = (IE_DATA *)d;
  e    = bto->e;

  na = data->nc;
  nb = CORNERS_OF_ELEM(e);

  for (i = 0; i < na; i++) {
    a[i].x = data->p[i][0];
    a[i].y = data->p[i][1];
  }

  for (i = 0; i < nb; i++) {
    b[i].x = XC(MYVERTEX(CORNER(e, i)));
    b[i].y = YC(MYVERTEX(CORNER(e, i)));
  }

  area_inters = AreaOfIntersection(a, na, b, nb);
  area_dest   = AreaOfIntersection(b, nb, b, nb);

  for (i = 0; i < data->no_es; i++)
    VVALUE(EVECTOR(e), data->es[i]) += data->scalar[i] * area_inters/area_dest;

  for (i = 0; i < data->no_ev; i++) {
    VVALUE(EVECTOR(e), data->ev[i]+0) += data->vector[i][0] * area_inters/area_dest;
    VVALUE(EVECTOR(e), data->ev[i]+1) += data->vector[i][1] * area_inters/area_dest;
  }
#endif
}

static int IntegrateElementData(STREAM *stream, BOXTREE *tree, int no_elements,
                                VT_ARRAY *vertices, EL_ARRAY *elements,
                                int no_es, SHORT *es,
                                int no_ev, SHORT *ev)
{
  IE_DATA data;
  double range[DIM][2];
  int i, j, k, nc;

  for (i = 0; i < no_elements; i++) {
    nc = elements[i].nc;
    for (j = 0; j < nc; j++)
      for (k = 0; k < DIM; k++)
        data.p[j][k] = vertices[elements[i].corner[j]].x[k];
    data.nc = nc;
    for (j = 0; j < DIM; j++) {
      range[j][0] =  MAX_D;
      range[j][1] = -MAX_D;
    }
    for (j = 0; j < nc; j++)
      for (k = 0; k < DIM; k++) {
        range[k][0] = MIN(range[0][0], data.p[j][k]);
        range[k][1] = MAX(range[0][1], data.p[j][k]);
      }
    data.no_es = no_es;
    data.no_ev = no_ev;
    data.es = es;
    data.ev = ev;
    for (j = 0; j < no_es; j++)
      if (ReadDouble(stream, data.scalar+j)) return 1;
    for (j = 0; j < no_ev; j++)
      for (k = 0; k < DIM; k++)
        if (ReadDouble(stream, &(data.vector[j][k]))) return 1;
    BT_Search(tree, range, IE_Callback, (void *)&data);
  }
  return 0;
}

static INT LoadFieldCommand(INT argc, char **argv)
{
  MULTIGRID *mg;
  HEAP *heap;
  INT key, key2;
  SHORT dest_ns[MAXVAR];
  SHORT dest_nv[MAXVAR];
  SHORT dest_es[MAXVAR];
  SHORT dest_ev[MAXVAR];
  int no_dest_ns, no_dest_nv, no_dest_es, no_dest_ev;
  char fbase[NAMESIZE], fname[NAMESIZE], s[NAMESIZE];
  STREAM stream;
  int no_dest_elements;
  int no_src_elements, no_src_vertices;
  double dest_bbox[DIM][2];
  double src_bbox[DIM][2];
  BOXTREE tree;
  VT_ARRAY *src_vertices;
  EL_ARRAY *src_elements;
  int no_src_ns, no_src_nv, no_src_es, no_src_ev;
  int i, j;

  mg = GetCurrentMultigrid();
  if (mg == NULL) {
    PrintErrorMessage('E', "loadfield","no current multigrid\n");
    return CMDERRORCODE;
  }
  if (ParseArgumentsR(argc, argv, mg, fbase,
                      &no_dest_ns, dest_ns, &no_dest_nv, dest_nv,
                      &no_dest_es, dest_es, &no_dest_ev, dest_ev)) {
    PrintErrorMessage('E', "loadfield","wrong parameters\n");
    return CMDERRORCODE;
  }
  StatisticsR(mg, &no_dest_elements, dest_bbox);
  heap = mg->theHeap;
  MarkTmpMem(heap, &key);
  if (MakeBoxTree(mg, no_dest_elements, key, &tree)) {
    PrintErrorMessage('E', "loadfield","OOM\n");
    return CMDERRORCODE;
  }
  for (i = 0; i < MAXPROC; i++) {
    sprintf(s, ".%04d", i); strcpy(fname, fbase); strcat(fname, s);
    if (OpenFile(fname, "r", &stream)) {
      if (i > 0) break;
      PrintErrorMessage('E', "loadfield","cannot open input file\n");
      return CMDERRORCODE;
    }
    if (MagicR(&stream)) {
      PrintErrorMessage('E', "loadfield","no ug field file\n");
      return CMDERRORCODE;
    }
    if (BBoxR(&stream, src_bbox)) goto failed;
    for (j = 0; j < DIM; j++)
      if (src_bbox[j][0] > dest_bbox[j][1] || dest_bbox[j][0] > src_bbox[j][1])
        goto skip;
    if (ReadInt(&stream, &no_src_vertices)) goto failed;
    MarkTmpMem(heap, &key2);
    src_vertices = (VT_ARRAY *)GetTmpMem(heap, no_src_vertices*sizeof(VT_ARRAY), key2);
    if (src_vertices == NULL) {
      PrintErrorMessage('E', "loadfield","OOM\n");
      return CMDERRORCODE;
    }
    if (ReadVertices(&stream, no_src_vertices, src_vertices)) goto failed;
    if (ReadInt(&stream, &no_src_elements)) goto failed;
    src_elements = (EL_ARRAY *)GetTmpMem(heap, no_src_elements*sizeof(EL_ARRAY), key2);
    if (src_elements == NULL) {
      PrintErrorMessage('E', "loadfield","OOM\n");
      return CMDERRORCODE;
    }
    if (ReadElements(&stream, no_src_elements, src_elements)) goto failed;
    if (ReadInt(&stream, &no_src_ns)) goto failed;
    if (ReadInt(&stream, &no_src_nv)) goto failed;
    ASSERT(no_src_ns == 0 && no_src_nv == 0);             /* die or implement */
    if (ReadInt(&stream, &no_src_es)) goto failed;
    if (ReadInt(&stream, &no_src_ev)) goto failed;
    if (IntegrateElementData(&stream, &tree, no_src_elements,
                             src_vertices, src_elements,
                             no_dest_es, dest_es,
                             no_dest_ev, dest_ev)) goto failed;
    ReleaseTmpMem(heap, key2);
skip:
    fclose(stream.file);
  }
  ReleaseTmpMem(heap, key);
  return OKCODE;
failed:
  PrintErrorMessage('E',"loadfield", "something went wrong\n");
  return CMDERRORCODE;
}

/******************************************************************************/

INT InitFieldIO(void)
{
  if (CreateCommand("savefield", SaveFieldCommand) == NULL) return __LINE__;
  if (CreateCommand("loadfield", LoadFieldCommand) == NULL) return __LINE__;
  return 0;
}

/*** EOF ***/

/*******************************************************************************
   This incredible piece of shit is from http://cap-lore.com/MathPhys/IP/aip.c
   It computes the area of the intersection of two polygons...
*******************************************************************************/

static double AreaOfIntersection(COORD_POINT * a, int na, COORD_POINT * b, int nb)
{
#if defined __GNUC__ && DIM==2
  typedef struct {COORD_POINT min; COORD_POINT max;} box;
  typedef long long hp;
  typedef struct {long x; long y;} ipoint;
  typedef struct {long mn; long mx;} rng;
  typedef struct {ipoint ip; rng rx; rng ry; short in;} vertex;
  vertex ipa[na+1], ipb[nb+1];
  box B = {{MAX_D, MAX_D}, {-MAX_D, -MAX_D}};
  double ascale;
  void pp(ipoint p){ /*printf("<%08X, %08X> ", p.x, p.y)*/
    ;
  }
  void range(COORD_POINT * x, int c){
    while(c--) {
      void bd(double * X, double y){
        *X = *X<y ? *X : y;
      }
      void bu(double * X, double y){
        *X = *X>y ? *X : y;
      }
      bd(&B.min.x, x[c].x); bu(&B.max.x, x[c].x);
      bd(&B.min.y, x[c].y); bu(&B.max.y, x[c].y);
    }
  }
  if(na < 3 || nb < 3) return 0;
  range(a, na); range(b, nb);
  {  const double gamut = 500000000., mid = gamut/2.;
     double rngx = B.max.x - B.min.x, sclx = gamut/rngx,
            rngy = B.max.y - B.min.y, scly = gamut/rngy;
     {void fit(COORD_POINT * x, int cx, vertex * ix, int fudge){
        {int c=cx; while(c--) {
           ix[c].ip.x = (long)((x[c].x - B.min.x)*sclx - mid)&~7|fudge|c&1;
           ix[c].ip.y = (long)((x[c].y - B.min.y)*scly - mid)&~7|fudge;
         }}
        ix[0].ip.y += cx&1;
        ix[cx] = ix[0];
        {int c=cx; while(c--) {
           ix[c].rx = ix[c].ip.x < ix[c+1].ip.x ?
                      (rng){ix[c].ip.x,ix[c+1].ip.x}: (rng){ix[c+1].ip.x,ix[c].ip.x};
           ix[c].ry = ix[c].ip.y < ix[c+1].ip.y ?
                      (rng){ix[c].ip.y,ix[c+1].ip.y}: (rng){ix[c+1].ip.y,ix[c].ip.y};
           ix[c].in=0;
         }}
      }
      fit(a, na, ipa, 0); fit(b, nb, ipb, 2);}
     ascale = sclx*scly;}
  { hp area(ipoint a, ipoint p, ipoint q)
    {
      return (hp)p.x*q.y - (hp)p.y*q.x +
             (hp)a.x*(p.y - q.y) + (hp)a.y*(q.x - p.x);
    }
    hp s = 0; int j, k;
    void cntrib(ipoint f, ipoint t, short w, char * S)
    {
      s += (hp)w*(t.x-f.x)*(t.y+f.y)/2;
      if(0) {pp(f); pp(t); printf("%016llX %d, %s\n", s, w, S);}
    }
    int ovl(rng p, rng q){
      return p.mn < q.mx && q.mn < p.mx;
    }
    for(j=0; j<na; ++j) for(k=0; k<nb; ++k)
        if(ovl(ipa[j].rx, ipb[k].rx) && ovl(ipa[j].ry, ipb[k].ry)) {
          hp a1 = -area(ipa[j].ip, ipb[k].ip, ipb[k+1].ip),
             a2 = area(ipa[j+1].ip, ipb[k].ip, ipb[k+1].ip);
          {int o = a1<0; if(o == a2<0)
           {hp a3 = area(ipb[k].ip, ipa[j].ip, ipa[j+1].ip),
               a4 = -area(ipb[k+1].ip, ipa[j].ip, ipa[j+1].ip);
          if(a3<0 == a4<0) {
            void cross(vertex * a, vertex * b, vertex * c, vertex * d,
                       double a1, double a2, double a3, double a4){
              double r1=a1/(a1+a2), r2 = a3/(a3+a4);
              cntrib((ipoint){a->ip.x + r1*(b->ip.x-a->ip.x),
                              a->ip.y + r1*(b->ip.y-a->ip.y)},
                     b->ip, 1, "one");
              cntrib(d->ip, (ipoint){
                       c->ip.x + r2*(d->ip.x - c->ip.x),
                       c->ip.y + r2*(d->ip.y - c->ip.y)
                     }, 1, "two");
              ++a->in; --c->in;
            }
            if(o) cross(&ipa[j], &ipa[j+1], &ipb[k], &ipb[k+1], a1, a2, a3, a4);
            else cross(&ipb[k], &ipb[k+1], &ipa[j], &ipa[j+1], a3, a4, a1, a2);
          }}}
        }
    {void inness(vertex * P, int cP, vertex * Q, int cQ){
       int s=0, c=cQ; ipoint p = P[0].ip;
       while(c--) if(Q[c].rx.mn < p.x && p.x < Q[c].rx.mx)
         {int sgn = 0 < area(p, Q[c].ip, Q[c+1].ip);
         s += sgn != Q[c].ip.x < Q[c+1].ip.x ? 0 : (sgn ? -1 : 1); }
       {int j; for(j=0; j<cP; ++j)
       {if(s) cntrib(P[j].ip, P[j+1].ip, s, "three");
          s += P[j].in;}}
       if(0) {int j; for(j=0; j<=cP; ++j)
                /*printf("ip: %08X %08X, %d\n", P[j].ip.x, P[j].ip.y, P[j].in)*/;
              printf("\n");}
     }
     inness(ipa, na, ipb, nb); inness(ipb, nb, ipa, na);}
    return s/ascale;}
#else
  return 1.0; /* just for fun ... */
#endif
}