// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/** \defgroup lgm The LGM Domain
 * \ingroup dom
 */
/*! \file lgm_domain.h
 * \ingroup lgm
 */

/****************************************************************************/
/*                                                                                                                                                      */
/* File:          lgm_domain.h                                                                                                  */
/*                                                                                                                                                      */
/* Purpose:   header file for lgm_domain                                                                        */
/*                                                                                                                                                      */
/* Author:        Klaus Johannsen                                                                                               */
/*                        Institut fuer Computeranwendungen III                                                 */
/*                        Universitaet Stuttgart                                                                                */
/*                        Pfaffenwaldring 27                                                                                    */
/*                        70550 Stuttgart                                                                                               */
/*                        email: klaus@ica3.uni-stuttgart.de                                                    */
/*                                                                                                                                                      */
/* History:   07.09.96 begin                                                                                            */
/*                                                                                                                                                      */
/* Remarks:                                                                                                                             */
/*                                                                                                                                                      */
/****************************************************************************/



/****************************************************************************/
/*                                                                                                                                                      */
/* auto include mechanism and other include files                                                       */
/*                                                                                                                                                      */
/****************************************************************************/

#ifndef __LGM_DOM__
#define __LGM_DOM__

#ifdef Grape

#include "grape.h"

#define SHORT  short
#define INT    int
#define FLOAT  float
#define DOUBLE double
#define COORD  float
#define SCREEN_COORD  float

/*dummy types */
#define ENVDIR char
#define ENVVAR char
#define ConfigProcPtr char

#warning LGM_DIM 3 for Grape is overwritten later anyway ?!
#define LGM_DIM 3

#else

#include "ugenv.h"
#include "heaps.h"
#include "domain.h"

#endif

/* need dimension defines even */
#include "dimension.h"

/* OS_CHANGED */
/* Set LGM_VERBOSE 1 to get a more verbose LGM module */
#define LGM_VERBOSE 1

/* Undefine the following line to get the old lgm geometry routines (3D) */
#define NEW_LGM
/* Undefine the following line to switch off accelerated lgm geometry access (3D) */
#define LGM_ACCELERATE

/* new lgm only for 3D, acceleration only for new lgm */
#ifdef UG_DIM_2
#undef NEW_LGM
#endif
#ifndef NEW_LGM
#undef LGM_ACCELERATE
#endif
#ifdef LGM_ACCELERATE
#include "bbtree.h"
#endif

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

/****************************************************************************/
/*                                                                                                                                                      */
/*      defines for basic configuration                                                                                 */
/*                                                                                                                                                      */
/****************************************************************************/

#ifdef UG_DIM_2
#define LGM_DIM                                                         2
#endif

#ifdef UG_DIM_3
#define LGM_DIM                                                         3
#endif

/****************************************************************************/
/*																			*/
/*	other																	*/
/*																			*/
/****************************************************************************/

#define LGM_PROBLEM_NAME(p)                                     ENVITEM_NAME(p)
#define LGM_PROBLEM_INIT(p)                                     ((p)->InitProblem)
#define LGM_PROBLEM_CONFIG(p)                           ((p)->ConfigProblem)
#define LGM_PROBLEM_DOMCONFIG(p)                        ((p)->ConfigDomainSize)
#define LGM_PROBLEM_NCOEFF(p)                           ((p)->numOfCoeffFct)
#define LGM_PROBLEM_NUSERF(p)                           ((p)->numOfUserFct)
#define LGM_PROBLEM_COEFF(p,i)                          (CoeffProcPtr)(p)->CU_ProcPtr[(i)]
#define LGM_PROBLEM_SETCOEFF(p,i,q)                     (p)->CU_ProcPtr[(i)]=(void*)(q)
#define LGM_PROBLEM_USERF(p,i)                          (UserProcPtr)(p)->CU_ProcPtr[(i)+LGM_PROBLEM_NCOEFF(p)]
#define LGM_PROBLEM_SETUSERF(p,i,q)                     (p)->CU_ProcPtr[(i)+LGM_PROBLEM_NCOEFF(p)]=(void*)(q)
#define LGM_PROBLEM_BNDCOND(p)                          ((p)->BndCond)
#define LGM_PROBLEM_INNERBNDCOND(p)                     ((p)->InnerBndCond)
#define LGM_DOMAIN_S2P_PTR(p)                           ((p)->s2p)
#define LGM_DOMAIN_S2P(p,s)                                     ((p)->s2p[s])

#if (LGM_DIM==2)

/* macros for LGM_POINT */
#define LGM_POINT_POS(p)                                        ((p)->position)
#define LGM_POINT_DIST(p,q)                                     (sqrt((LGM_POINT_POS(p)[0]-LGM_POINT_POS(q)[0])*(LGM_POINT_POS(p)[0]-LGM_POINT_POS(q)[0])+(LGM_POINT_POS(p)[1]-LGM_POINT_POS(q)[1])*(LGM_POINT_POS(p)[1]-LGM_POINT_POS(q)[1])))

/* macros for LGM_LINE */
#define LGM_LINE_ID(p)                                          ((p)->id)
#define LGM_LINE_FLAG(p)                                        ((p)->flag)
#define LGM_LINE_NPOINT(p)                                      ((p)->nPoint)
#define LGM_LINE_LEFT(p)                                        ((p)->left)
#define LGM_LINE_RIGHT(p)                                       ((p)->right)
#define LGM_LINE_BEGIN(p)                                       ((p)->begin)
#define LGM_LINE_END(p)                                         ((p)->end)
#define LGM_LINE_BNDCOND(p)                                     ((p)->Bndcond)
#define LGM_LINE_POINT(p,i)                                     ((p)->point+(i))
#define LGM_LINE_POINTDIST(p,i,j)                       LGM_POINT_DIST(LGM_LINE_POINT(p,i),LGM_LINE_POINT(p,j))
#define LGM_LINE_ID_2_LINE(i)                           (LinePtrArray[i])

/* macros for LGM_SUBDOMAIN */
#define LGM_SUBDOMAIN_UNIT(p)                           ((p)->Unit)
#define LGM_SUBDOMAIN_ID(p)                                     ((p)->id)
#define LGM_SUBDOMAIN_SDDATA(p)                         ((p)->SubDomData)
#define LGM_SUBDOMAIN_NLINE(p)                          ((p)->nLines)
#define LGM_SUBDOMAIN_LINE(p,i)                         ((p)->line[(i)])

/* macros for LGM_DOMAIN */
#define LGM_DOMAIN_NAME(p)                                      ENVITEM_NAME(p)
#define LGM_DOMAIN_PROBLEMNAME(p)                       ((p)->ProblemName)
#define LGM_DOMAIN_PROBLEM(p)                           ((p)->theProblem)
#define LGM_DOMAIN_CONVEX(p)                            ((p)->convex)
#define LGM_DOMAIN_RADIUS(p)                            ((p)->radius)
#define LGM_DOMAIN_MIDPOINT(p)                          ((p)->midpoint)
#define LGM_DOMAIN_DOMDATA(p)                           ((p)->DomainData)
#define LGM_DOMAIN_NSUBDOM(p)                           ((p)->nSubDomain)
#define LGM_DOMAIN_NPART(p)                                     ((p)->nDomParts)
#define LGM_DOMAIN_NPOINT(p)                            ((p)->nPoint)
#define LGM_DOMAIN_SUBDOM(p,i)                          ((p)->theSubdom[(i)])

/* macros for LGM_BNDP */
#define LGM_BNDP_N(p)                                           ((p)->n)
#define LGM_BNDP_LINES(p,i)                                     ((p)->Line[(i)])
#define LGM_BNDP_LINE(p,i)                                      ((p)->Line[(i)].l.theLine)
#define LGM_BNDP_ID(p,i)                                        ((p)->Line[(i)].l.theLineID)
#define LGM_BNDP_LOCAL(p,i)                                     ((p)->Line[(i)].local)
#define LGM_BNDP_LINE_GLINE(p)                          ((p).l.theLine)
#define LGM_BNDP_LINE_LOCAL(p)                          ((p).local)

/* macros for LGM_BNDS */
#define LGM_BNDS_LINE(p)                                        ((p)->theLine)
#define LGM_BNDS_LOCAL(p,i)                                     ((p)->local[(i)])
#ifdef ModelP
#define LGM_BNDS_SIZE(p)                        (sizeof(LGM_BNDS))
#define LGM_BNDP_SIZE(p)                        (sizeof(LGM_BNDP))
#define LINE_ID_2_LINE(i)                                       (LinePtrArray[i])
#endif


#endif

#if (LGM_DIM==3)

#define NO_PROJECT
#undef NO_PROJECT // OS_CHANGED

#define MAXTRIANGLES 30

/* macros for LGM_POINT */
#define LGM_POINT_POS(p)                                        ((p)->position)
#define LGM_POINT_DIST(p,q)                                     (sqrt( (LGM_POINT_POS(p)[0]-LGM_POINT_POS(q)[0])*(LGM_POINT_POS(p)[0]-LGM_POINT_POS(q)[0])\
                                                                       + (LGM_POINT_POS(p)[1]-LGM_POINT_POS(q)[1])*(LGM_POINT_POS(p)[1]-LGM_POINT_POS(q)[1])\
                                                                       + (LGM_POINT_POS(p)[2]-LGM_POINT_POS(q)[2])*(LGM_POINT_POS(p)[2]-LGM_POINT_POS(q)[2])))

/* macros for LGM_LINEDISC */
#define LGM_LINEDISC_NPOINT(p)                          ((p)->npoint)
#define LGM_LINEDISC_LOCAL(p,i)                         ((p)->local[(i)])

/* macros for LGM_LINEDISCNEW */
#define LGM_LINEDISCNEW_NPOINT(p)                       ((p)->npoints)
#define LGM_LINEDISCNEW_START(p)                        ((p)->start)
#define LGM_LINEDISCNEW_POINT(p)                        ((p)->point)

/* macros for LGM_SURFDISC */
#define LGM_SURFDISC_NPOINT(p)                          ((p)->npoint)
#define LGM_SURFDISC_NTRIANGLE(p)                       ((p)->ntriangle)
#define LGM_SURFDISC_LOCAL(p,i,j)                       ((p)->local[(i)][(j)])
#define LGM_SURFDISC_TRIANGLE(p,i,j)            ((p)->triangle[(i)][(j)])
#define LGM_SURFDISC_TRIANGLE_NEIGHBOUR(p,i,j)          ((p)->neighbour[(i)][(j)])
#define LGM_SURFDISC_MESH_ID(p,i)                       ((p)->mesh_id[i])
#define LGM_SURFDISC_FMESH_ID(p)                        ((p)->mesh_id)
#define LGM_SURFDISC_XY_ID(p,i)                         ((p)->xy_id[i])

/* macros for LGM_LINE */
#define LGM_LINE_ID(p)                                          ((p)->id)
#define LGM_LINE_FLAG(p)                                        ((p)->flag)
#define LGM_LINE_NPOINT(p)                                      ((p)->nPoint)
#define LGM_LINE_POINT(p,i)                                     ((p)->point+(i))
#define LGM_LINE_LINEDISC(p)                            ((p)->ldisc)
#define LGM_LINE_LINEDISCNEW(p)                         ((p)->ldiscnew)
#define LGM_LINE_BEGIN(p)                                       ((p)->begin)
#define LGM_LINE_END(p)                                         ((p)->end)
#define LGM_LINE_USED(p)                                        ((p)->used)
#define LGM_LINE_ID_2_LINE(i)                           (LinePtrArray[i])

/* macros for LGM_TRIANGLE */
#define LGM_TRIANGLE_CORNER(p,i)                        ((p)->corner[(i)])
#define LGM_TRIANGLE_CORNERID(p,i)                      ((p)->cornerid[(i)])
#define LGM_TRIANGLE_NEIGHBOR(p,i)                      ((p)->neighbor[(i)])

/* macros for LGM_SURFACE */
#define LGM_SURFACE_ID(p)                                       ((p)->id)
#define LGM_SURFACE_FLAG(p)                                     ((p)->flag)
#define LGM_SURFACE_NPOINT(p)                           ((p)->nPoint)
#define LGM_SURFACE_NTRIANGLE(p)                        ((p)->nTriangle)
#define LGM_SURFACE_NLINE(p)                            ((p)->nLine)
#define LGM_SURFACE_LEFT(p)                                     ((p)->left)
#define LGM_SURFACE_RIGHT(p)                            ((p)->right)
#define LGM_SURFACE_BNDCOND(p)                          ((p)->Bndcond)
#define LGM_SURFACE_DISC(p)                                     ((p)->sdisc)
#define LGM_SURFACE_POINT(p,i)                          ((p)->point+(i))
#define LGM_SURFACE_TRIANGLE(p,i)                       ((p)->triangle+(i))
#define LGM_SURFACE_FPOINT(p)                           ((p)->point)
#define LGM_SURFACE_FTRIANGLE(p)                        ((p)->triangle)
#define LGM_SURFACE_LINE(p,i)                           ((p)->line[(i)])
#define LGM_SURFACE_ID_2_SURFACE(i)                     (SurfacePtrArray[i])

/* macros for LGM_SUBDOMAIN */
#define LGM_SUBDOMAIN_UNIT(p)                           ((p)->Unit)
#define LGM_SUBDOMAIN_ID(p)                                     ((p)->id)
#define LGM_SUBDOMAIN_SDDATA(p)                         ((p)->SubDomData)
#define LGM_SUBDOMAIN_NSURFACE(p)                       ((p)->nSurface)
#define LGM_SUBDOMAIN_SURFACE(p,i)                      ((p)->surface[(i)])

/* macros for LGM_DOMAIN */
#define LGM_DOMAIN_NAME(p)                                      ENVITEM_NAME(p)
#define LGM_DOMAIN_PROBLEMNAME(p)                       ((p)->ProblemName)
#define LGM_DOMAIN_PROBLEM(p)                           ((p)->theProblem)
#define LGM_DOMAIN_CONVEX(p)                            ((p)->convex)
#define LGM_DOMAIN_RADIUS(p)                            ((p)->radius)
#define LGM_DOMAIN_MIDPOINT(p)                          ((p)->midpoint)
#define LGM_DOMAIN_DOMDATA(p)                           ((p)->DomainData)
#define LGM_DOMAIN_NPOINT(p)                            ((p)->nPoint)
#define LGM_DOMAIN_NSUBDOM(p)                           ((p)->nSubDomain)
#define LGM_DOMAIN_SUBDOM(p,i)                          ((p)->theSubdom[(i)])
#define LGM_DOMAIN_NPART(p)                                     ((p)->nDomParts)
#define LGM_DOMAIN_S2P_PTR(p)                           ((p)->s2p)
#define LGM_DOMAIN_S2P(p,s)                                     ((p)->s2p[s])

/* macros for LGM_BNDP */
#define LGM_BNDP_NLINE(p)                                       ((p)->nlines)
#define LGM_BNDP_LINES(p,i)                                     ((p)->Line[(i)])
#define LGM_BNDP_LINE(p,i)                                      ((p)->Line[(i)].l.theLine)
#define LGM_BNDP_ID(p,i)                                        ((p)->Line[(i)].l.theLineID)
#ifndef NO_PROJECT
#define LGM_BNDP_LINE_LEFT(p,i)                         ((p)->Line[(i)].local_left)
#define LGM_BNDP_LINE_RIGHT(p,i)                        ((p)->Line[(i)].local_right)
#else
#define LGM_BNDP_LINE_GLOBALLEFT(p,i)           ((p)->Line[(i)].global_left)
#define LGM_BNDP_LINE_GLOBALRIGHT(p,i)          ((p)->Line[(i)].global_right)
#endif
#define LGM_BNDP_LINE_GLINE(p)                          ((p).l.theLine)
#ifndef NO_PROJECT
#define LGM_BNDP_LOCAL_LEFT(p)                          ((p).local_left)
#define LGM_BNDP_LOCAL_RIGHT(p)                         ((p).local_right)
#else
#define LGM_BNDP_GLOBAL_LEFT(p)                         ((p).global_left)
#define LGM_BNDP_GLOBAL_RIGHT(p)                        ((p).global_right)
#endif
#define LGM_BNDP_N(p)                                           ((p)->nsurf)
#define LGM_BNDP_SURFACES(p,i)                          ((p)->Surf[(i)])
#define LGM_BNDP_SURFACE(p,i)                           ((p)->Surf[(i)].s.theSurf)
#define LGM_BNDP_SURFACEID(p,i)                           ((p)->Surf[(i)].s.theSurfID)
#define LGM_BNDP_SURFACEPTR(p)                          ((p)->Surf)
#define LGM_BNDP_LINEPTR(p)                                     ((p)->Line)

#ifndef NO_PROJECT
#define LGM_BNDP_LOCAL(p,i)                                     ((p)->Surf[(i)].local)
#else
#define LGM_BNDP_GLOBAL(p,i)                            ((p)->Surf[(i)].global)
#endif
#define LGM_BNDP_SURFACE_GSURFACE(p)            ((p).s.theSurf)
#ifndef NO_PROJECT
#define LGM_BNDP_SURFACE_LOCAL(p)                       ((p).local)
#else
#define LGM_BNDP_SURFACE_GLOBAL(p)                      ((p).global)
#endif

/* macros for LGM_BNDS */
#define LGM_BNDS_N(p)                                           ((p)->nn)
#define LGM_BNDS_SURFACE(p)                                     ((p)->theSurf)
#ifndef NO_PROJECT
#define LGM_BNDS_LOCAL(p,i,j)                           ((p)->local[(i)][(j)])
#else
#define LGM_BNDS_GLOBAL(p,i,j)                          ((p)->global[(i)][(j)])
#endif
#define LGM_BNDS_TRIANGLE(p,i)                          ((p)->triangle[(i)])


#endif

typedef INT (*InitProcPtr)(INT, char **, INT, char *, HEAP *);
typedef INT (*BndCondProcPtr)(DOUBLE *, DOUBLE *, INT *);
typedef INT (*DomainSizeConfig)(DOUBLE *min, DOUBLE *max);


/****************************************************************************/
/*                                                                                                                                                      */
/* data structures exported by the corresponding source file                            */
/*                                                                                                                                                      */
/****************************************************************************/

struct lgm_problem {
  /* fields for environment directory */
  ENVDIR v;

  /* fields for problem */
  InitProcPtr InitProblem;          /* procedure to initialize problem          */
  ConfigProcPtr ConfigProblem;      /* procedure to reinitialize problem		*/
  DomainSizeConfig ConfigDomainSize;      /* procedure to reinitialize size of d*/
  BndCondProcPtr BndCond;               /* global boundary condition				*/
  BndCondProcPtr InnerBndCond;      /* global inner boundary condition                  */
  INT numOfCoeffFct;                            /* number of coefficient functions			*/
  INT numOfUserFct;                             /* number of User functions					*/
  void *CU_ProcPtr[1];                  /* coefficient functions					*/
};


#if (LGM_DIM==2)
/****************************************************************************/
/*                                                                          */
/*      2D structures                                                       */
/*                                                                          */
/****************************************************************************/

struct lgm_point {

  DOUBLE position[LGM_DIM];                              /* position of corner */
};

struct lgm_line {

  INT id;                               /* id of the line                               */
  INT flag;                             /* general purpose flag                 */
  INT nPoint;                           /* nb. of points on the line            */
  INT left, right;                      /* subdomain on left and right side     */
  INT begin, end;                       /* global id's starting from 0          */
  BndCondProcPtr Bndcond;               /* boundary condition                   */
  struct lgm_point point[1];                    /* points of line stored here           */
};

struct lgm_subdom_data {

  INT dummy;
};

struct lgm_subdom {

  /* parameters */
  char Unit[128];                                               /* unit-identification                                                  */
  INT id;                                                               /* unique id, beginning with 1                                  */
  INT nLines;                                                           /* nb. of lines                                                                 */
  struct lgm_subdom_data *SubDomData;           /* data for subdomain                                                   */

  /* references */
  struct lgm_line *line[1];                             /* begin of line reference field                                */
};

struct lgm_dom_data {

  INT dummy;
};

struct lgm_domain {

  /* fields for environment directory */
  ENVVAR v;

  /* heap */
  HEAP *theHeap;

  /* parameters */
  INT convex;                                                           /* 0 (no) or 1 (yes)                                                    */
  float radius, midpoint[LGM_DIM];              /* sphere of which domain is a subset                   */
  INT nSubDomain;                                               /* nb. of subdomains                                                    */
  INT nDomParts;                                                /* nb. of domain parts                                                  */
  INT *s2p;                                                             /* pointer to table subbdom --> part                    */
  INT nPoint;                                                           /* nb. of points                                                                */
  struct lgm_dom_data *DomainData;              /* data for domain                                                              */

  /* problem */
  char ProblemName[128];                                /* name of problem                                                              */
  struct lgm_problem *theProblem;               /* ptr to problem                                                               */

  /* reference to domain data */
  struct lgm_subdom *theSubdom[1];              /* begin of subdom reference field                              */
};

struct lgm_bndp_line {
  union {
    struct lgm_line *theLine;   /* real line data                */
    INT theLineID;              /* used by Load_Ext and Save_Ext */
  } l;
  DOUBLE local;                                                 /* local coordinate                                                             */
};

struct lgm_bndp {

  INT n;                                /* number of lines                                      */
  struct lgm_bndp_line Line[1];         /* line(s)                                                          */
};

struct lgm_bnds {

  struct lgm_line *theLine;                             /* line                                                                                 */
  DOUBLE local[2];                                              /* local coordinates                                                    */
};

typedef struct lgm_point LGM_POINT;
typedef struct linedisc LGM_LINEDISC;
typedef struct surfdisc LGM_SURFDISC;
typedef struct lgm_line LGM_LINE;
typedef struct lgm_subdom_data LGM_SUBDOMAIN_DATA;
typedef struct lgm_subdom LGM_SUBDOMAIN;
typedef struct lgm_domain LGM_DOMAIN;
typedef struct lgm_problem LGM_PROBLEM;
typedef struct lgm_bndp_line LGM_BNDP_PLINE;
typedef struct lgm_bndp LGM_BNDP;
typedef struct lgm_bnds LGM_BNDS;

#endif

#if (LGM_DIM==3)
/****************************************************************************/
/*                                                                          */
/*      3D structures                                                                                                                   */
/*                                                                          */
/****************************************************************************/

struct lgm_point {

  DOUBLE position[LGM_DIM];                     /* position of corner                                           */
};

struct linepoint
{
  DOUBLE local;
  struct linepoint *next;
};

typedef struct linepoint LINEPOINT;

struct linediscnew
{
  INT npoints;
  LINEPOINT *start;
  LINEPOINT *point;
};

typedef struct linediscnew LINEDISCNEW;

struct linedisc {

  INT npoint;                                                   /* nb. of discretization points on the line     */
  DOUBLE *local;                                        /* local coorddinates of the points                     */
};

struct lgm_line {
  INT id;                                                       /* id of the line */
  INT flag;                                                     /* general purpose flag         */
  INT nPoint;                                                   /* nb. of points on the line    */
  INT begin, end;                                       /* global id's starting from 0  */
  struct linedisc *ldisc;                       /* discretization of the line   */
  LINEDISCNEW *ldiscnew;                        /* discretization of the line   */

  /* specials for grape */
        #ifdef Grape
  INT active;
        #endif
  INT used;
  struct lgm_point point[1];                    /* points of line stored here   */
};

struct surfdisc {

  INT npoint;                                                   /* nb. of discretization points on the surface*/
  INT ntriangle;                                        /* nb. of ggtriangles on the surface            */
  DOUBLE **local;                                       /* local coorddinates of the points                     */
  INT *mesh_id;                                         /* id of point in mesh-structure                        */
  INT **triangle;                                       /* triangle-list                                                        */
  INT *xy_id;                                                   /* triangle-list                                                        */
  INT **neighbour;                                      /* triangle-neighbour-list                                                      */
  INT dummy;                                                    /* to fill according to the needs                       */
};

struct lgm_triangle {

  struct lgm_point *corner[3];          /* corners of the triangle                                      */
  int cornerid[3];
  int neighbor[3];                                      /* neighbors of the triangle                            */
};

struct lgm_surface {

  INT id;
  INT flag;                                                             /* general purpose flag		                        */
  INT nPoint;                                                           /* nb. of points on the surface			*/
  INT nTriangle;                                                /* nb. of triangles on the surface		*/
  INT nLine;                                                            /* nb. of lines on the surface			*/
  INT left, right;                                              /* subdomain on left and right side		*/
  BndCondProcPtr Bndcond;                               /* boundary condition					*/
  struct surfdisc *sdisc;                               /* discretization of the surface		*/
  struct lgm_point *point;                              /* ptr to first point					*/
  struct lgm_triangle *triangle;                /* ptr to first triangle				*/

  /* specials for grape */
        #ifdef Grape
  INT active;
        #endif

  /* for fast triangle search OS_CHANGED */
        #ifdef LGM_ACCELERATE
  BBT_TREE *bbtree;
        #endif

  /* references */
  struct lgm_line *line[1];                             /* ptr to lines							*/
};

struct lgm_subdom_data {

  INT dummy;
};

struct lgm_subdom {

  /* parameters */
  char Unit[128];                                               /* unit-identification                                  */
  INT id;                                                               /* unique id, beginning with 1                  */
  INT nSurface;                                                 /* nb. of surfaces                                              */
  INT nPoint;                                                           /* nb. of points                                                */
  struct lgm_subdom_data *SubDomData;           /* data for subdomain                                   */

  /* problem */
  char ProblemName[128];                                /* name of problem                                              */
  struct lgm_problem *theProblem;               /* ptr to problem                                               */

  /* specials for grape */
        #ifdef Grape
  INT active;
  SUPROP_DEV *suprop;
        #endif

  /* references */
  struct lgm_surface *surface[1];               /* begin of surface reference field             */
};

struct lgm_dom_data {

  INT dummy;
};

struct lgm_domain {

  /* fields for environment directory */
  ENVVAR v;

  /* heap */
  HEAP *theHeap;

  /* parameters */
  INT convex;                                                           /* 0 (no) or 1 (yes)                                                    */
  float radius, midpoint[LGM_DIM];              /* sphere of which domain is a subset                   */
  INT nSubDomain;                                               /* nb. of subdomains                                                    */
  INT nDomParts;                                                /* nb. of domain parts                                                  */
  INT *s2p;                                                             /* pointer to table subbdom --> part                    */
  INT nPoint;                                                           /* nb. of points                                                                */
  struct lgm_dom_data *DomainData;              /* data for domain                                                              */

  /* problem */
  char ProblemName[128];                                /* name of problem                                                              */
  struct lgm_problem *theProblem;               /* ptr to problem                                                               */

  /* reference to domain data */
  struct lgm_subdom *theSubdom[1];              /* begin of subdom reference field                              */
};

struct lgm_bndp_line {

  union
  {
    struct lgm_line *theLine;  /* either pointer to line data ...  */
    INT theLineID;             /* ... or just an ID (for Save_Ext/Load_Ext) */
  } l;

        #ifdef NO_PROJECT
  DOUBLE global_left[3];                                /* global coordinate of the left neighbor       */
  DOUBLE global_right[3];                               /* global coordinate of the right neighbor      */
        #else
  DOUBLE local_left;                                            /* local coordinate of the left neighbor        */
  DOUBLE local_right;                                           /* local coordinate of the right neighbor       */
        #endif
};

struct lgm_bndp_surf {

  union
  {
    struct lgm_surface *theSurf;  /* surface... */
    INT theSurfID;                /* ... or just ID */
  } s;

    #ifdef NO_PROJECT
  DOUBLE global[3];                                                     /* global coordinate                                                            */
        #else
  DOUBLE local[2];                                                      /* local coordinate                                                             */
    #endif
};

struct lgm_bndp {

  INT nlines;                                                           /* number of lines                                                              */
  struct lgm_bndp_line *Line;                           /* line(s)                                                                              */
  INT nsurf;                                /* number of surfaces                                       */
  struct lgm_bndp_surf *Surf;           /* surface(s)                                                   */
};

struct lgm_bnds_triangle {

  struct lgm_triangle *triangle;                /* ptr to triangle                                                              */
  DOUBLE local[2];                                              /* local coordinate                                                             */
};

struct lgm_bnds {

  INT nn;
  struct lgm_surface *theSurf;                  /* surface										*/
        #ifdef NO_PROJECT
  DOUBLE global[4][3];                                  /* global coordinates							*/
        #else
  DOUBLE local[4][2];                                           /* local coordinates							*/
        #endif
};


typedef struct lgm_point LGM_POINT;
typedef struct linedisc LGM_LINEDISC;
typedef struct linediscnew LGM_LINEDISCNEW;
typedef struct lgm_line LGM_LINE;
typedef struct surfdisc LGM_SURFDISC;
typedef struct lgm_triangle LGM_TRIANGLE;
typedef struct lgm_surface LGM_SURFACE;
typedef struct lgm_subdom_data LGM_SUBDOMAIN_DATA;
typedef struct lgm_subdom LGM_SUBDOMAIN;
typedef struct lgm_domain LGM_DOMAIN;
typedef struct lgm_problem LGM_PROBLEM;
typedef struct lgm_bndp LGM_BNDP;
typedef struct lgm_bnds LGM_BNDS;
typedef struct lgm_bndp_line LGM_BNDP_PLINE;
typedef struct lgm_bndp_surf LGM_BNDP_PSURFACE;

#endif

#ifdef Grape
typedef struct Domain3d
{
  INSTANCE_STRUCT;\
  LGM_DOMAIN *domain;
} DOMAIN3D;
#endif

/****************************************************************************/
/*                                                                                                                                                      */
/* definition of exported global variables                                                                      */
/*                                                                                                                                                      */
/****************************************************************************/


/****************************************************************************/
/*                                                                                                                                                      */
/* function declarations                                                                                                        */
/*                                                                                                                                                      */
/****************************************************************************/
#if (LGM_DIM==2)
LGM_LINE                                *FirstLine                      (LGM_DOMAIN *theDomain);
LGM_LINE                                *NextLine                       (LGM_DOMAIN *theDomain);
#endif
#if (LGM_DIM==3)
LGM_SURFACE                     *FirstSurface           (LGM_DOMAIN *theDomain);
LGM_SURFACE                     *NextSurface            (LGM_DOMAIN *theDomain);
LGM_LINE                                *FirstLine                      (LGM_DOMAIN *theDomain);
LGM_LINE                                *NextLine                       (LGM_DOMAIN *theDomain);
INT                                     LGM_BNDS_SurfId         (BNDS *aBndS);

/*for evaluation of global coordinates*/
INT Line_Local2GlobalNew (LGM_LINE *theLine, DOUBLE *global, DOUBLE local);
INT Surface_Local2Global (LGM_SURFACE *theSurface, DOUBLE *global, DOUBLE *local);

#endif

#ifdef __THREEDIM__
BNDP *BNDP_InsertBndP (HEAP *Heap, BVP *aBVP, double *global);
#endif

#ifndef Grape
LGM_PROBLEM *CreateProblem (const char *name, InitProcPtr config, DomainSizeConfig domconfig, BndCondProcPtr BndCond, int numOfCoefficients, CoeffProcPtr coeffs[], int numOfUserFct, UserProcPtr userfct[]);
LGM_PROBLEM *CreateProblemWithInnerBCs (const char *name, InitProcPtr config, DomainSizeConfig domconfig, BndCondProcPtr BndCond, BndCondProcPtr InnerBndCond, int numOfCoefficients, CoeffProcPtr coeffs[], int numOfUserFct, UserProcPtr userfct[]);
#endif
INT SetBoundaryCondition (LGM_DOMAIN *theDomain, BndCondProcPtr BndCond, BndCondProcPtr InnerBndCond);
INT SetDomainSize (LGM_DOMAIN *theDomain);

INT GetMaximumSurfaceID (LGM_DOMAIN *theDomain);
INT OuterBndSurfaceIDs (LGM_DOMAIN *theDomain, INT *sf); /* OS_CHANGED */
INT SurfaceIDsOfSubdomain (LGM_DOMAIN *theDomain, INT *sf, INT i); /* OS_CHANGED */


END_UGDIM_NAMESPACE

#endif
