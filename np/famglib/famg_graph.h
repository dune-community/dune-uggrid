// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*																			*/
/* File:      famg_graph.h													*/
/*																			*/
/* Purpose:   famg graph classes											*/
/*																			*/
/* Author:    Christian Wagner												*/
/*			  Institut fuer Computeranwendungen  III						*/
/*			  Universitaet Stuttgart										*/
/*			  Pfaffenwaldring 27											*/
/*			  70569 Stuttgart												*/
/*			  internet: chris@ica3.uni-stuttgart.de							*/
/*																			*/
/*																			*/
/* History:   November 97 begin, Stuttgart									*/
/*			  August 98 integration into ug (Christian Wrobel)				*/
/*																			*/
/* Remarks:																	*/
/*																			*/
/****************************************************************************/
#ifndef __FAMG_GRAPH__
#define __FAMG_GRAPH__

#include <assert.h>

#ifdef USE_UG_DS
extern "C"
{
        #include "parallel.h"
        #include "pargm.h"
}
#endif

#include "famg_algebra.h"

const int FAMGMAXPARENTS=2;


class FAMGList
{
public:
  int GetData() const;
  void Insert(class FAMGNode *);
  void Init(FAMGList *,FAMGList *,int );
  FAMGList* GetPred() const;
  FAMGList* GetSucc() const;
  class FAMGNode* GetFirst() const;
  class FAMGNode* GetLast() const;
  void SetData(int);
  void SetPred(FAMGList *);
  void SetSucc(FAMGList *);
  void SetFirst(class FAMGNode *);
  void SetLast(class FAMGNode *);
private:
  int data;                                             // rating
  FAMGList *succ, *pred;                        // linked list
  class FAMGNode *first, *last;
};

inline int FAMGList::GetData() const {
  return data;
}
inline FAMGList* FAMGList::GetPred() const {
  return pred;
}
inline FAMGList* FAMGList::GetSucc() const {
  return succ;
}
inline class FAMGNode* FAMGList::GetFirst() const {
  return first;
}
inline class FAMGNode* FAMGList::GetLast() const {
  return last;
}
inline void FAMGList::SetData(int val) {
  data = val;
}
inline void FAMGList::SetPred(FAMGList *p) {
  pred = p;
}
inline void FAMGList::SetSucc(FAMGList *s) {
  succ = s;
}
inline void FAMGList::SetFirst(class FAMGNode *f) {
  first = f;
}
inline void FAMGList::SetLast(class FAMGNode *l) {
  last = l;
}


#ifdef FAMG_SPARSE_BLOCK

class FAMGPaList
{
public:
  int GetNp() const;
  int GetPa(int index) const;
  const int *GetPaPtr() const;
  int GetNewLinks() const;
  double GetNewCG() const;
  double GetApprox() const;
  double *GetCoeff(int) const;
  double *GetCoefft(int) const;
  double **GetCoeff() const;
  double **GetCoefft() const;
  FAMGPaList* GetNext() const;
  void SetNp(int);
  void SetPa(int index, int p);
  void SetNext(FAMGPaList* );
  void SetNewLinks(int);
  void SetNewCG(double);
  void SetApprox(double);
  void SetCoeff(int, double*);
  void SetCoefft(int, double*);
  void SetCoeff(int, short, double);
  void SetCoefft(int, short, double);
  void Init(FAMGPaList *next, int np, const int p[], FAMGSparseVector *sp, FAMGSparseVector *sr, double **c, double **ct, double error);
  void MarkParents(class FAMGGrid *grid);
  double TotalWeight();
private:
  int np;                                                               // 1..FAMGMAXPARENTS
  int pa[FAMGMAXPARENTS];                               // the parent NODES (not vectors!)
  double *coeff[FAMGMAXPARENTS];
  double *coefft[FAMGMAXPARENTS];
  double approx;
  int newlinks;
  double newcg;                                                 // in special situations this may be a rational
  class FAMGPaList *next;
};

inline int FAMGPaList::GetNp() const {
  return np;
}
inline int FAMGPaList::GetPa(int index) const {
  return pa[index];
}
inline const int *FAMGPaList::GetPaPtr() const {
  return pa;
}
inline int FAMGPaList::GetNewLinks() const {
  return newlinks;
}
inline double FAMGPaList::GetApprox() const {
  return approx;
}
inline double FAMGPaList::GetNewCG() const {
  return newcg;
}
inline double *FAMGPaList::GetCoeff(int i) const {
  return coeff[i];
}
inline double *FAMGPaList::GetCoefft(int i) const {
  return coefft[i];
}
inline double **FAMGPaList::GetCoeff() const {
  return (double **)coeff;
}
inline double **FAMGPaList::GetCoefft() const {
  return (double **)coefft;
}
inline FAMGPaList* FAMGPaList::GetNext() const {
  return next;
}
inline void FAMGPaList::SetNp(int v) {
  np = v;
}
inline void FAMGPaList::SetPa(int index, int p) {
  pa[index] = p;
}
inline void FAMGPaList::SetNewLinks(int v) {
  newlinks = v;
}
inline void FAMGPaList::SetNewCG(double v) {
  newcg = v;
}
inline void FAMGPaList::SetApprox(double v) {
  approx = v;
}
inline void FAMGPaList::SetCoeff(int i, double *c) {
  coeff[i] = c;
}
inline void FAMGPaList::SetCoefft(int i, double *c) {
  coefft[i] = c;
}
inline void FAMGPaList::SetCoeff(int i, short j, double c) {
  (coeff[i])[j] = c;
}
inline void FAMGPaList::SetCoefft(int i, short j, double c) {
  (coefft[i])[j] = c;
}
inline void FAMGPaList::SetNext(FAMGPaList* ptr) {
  next = ptr;
}

#else

class FAMGPaList
{
public:
  int GetNp() const;
  int GetPa(int index) const;
  const int *GetPaPtr() const;
  int GetNewLinks() const;
  double GetNewCG() const;
  double GetApprox() const;
  double GetCoeff(int) const;
  double GetCoefft(int) const;
  double *GetCoeff() const;
  double *GetCoefft() const;
  FAMGPaList* GetNext() const;
  void SetNp(int);
  void SetPa(int index, int p);
  void SetNext(FAMGPaList* );
  void SetNewLinks(int);
  void SetNewCG(double);
  void SetApprox(double);
  void SetCoeff(int, double);
  void SetCoefft(int, double);
  void Init(FAMGPaList *next, int np, const int p[], double c[], double ct[], double error);
  void MarkParents(class FAMGGrid *grid);
  double TotalWeight();
private:
  int np;                                                               // 1..FAMGMAXPARENTS
  int pa[FAMGMAXPARENTS];                               // the parent NODES (not vectors!)
  double coeff[FAMGMAXPARENTS];
  double coefft[FAMGMAXPARENTS];
  double approx;
  int newlinks;
  double newcg;                                                 // in special situations this may be a rational
  class FAMGPaList *next;
};

inline int FAMGPaList::GetNp() const {
  return np;
}
inline int FAMGPaList::GetPa(int index) const {
  return pa[index];
}
inline const int *FAMGPaList::GetPaPtr() const {
  return pa;
}
inline int FAMGPaList::GetNewLinks() const {
  return newlinks;
}
inline double FAMGPaList::GetApprox() const {
  return approx;
}
inline double FAMGPaList::GetNewCG() const {
  return newcg;
}
inline double FAMGPaList::GetCoeff(int i) const {
  return coeff[i];
}
inline double FAMGPaList::GetCoefft(int i) const {
  return coefft[i];
}
inline double *FAMGPaList::GetCoeff() const {
  return (double *) coeff;
}
inline double *FAMGPaList::GetCoefft() const {
  return (double *)coefft;
}
inline FAMGPaList* FAMGPaList::GetNext() const {
  return next;
}
inline void FAMGPaList::SetNp(int v) {
  np = v;
}
inline void FAMGPaList::SetPa(int index, int p) {
  pa[index] = p;
}
inline void FAMGPaList::SetNewLinks(int v) {
  newlinks = v;
}
inline void FAMGPaList::SetNewCG(double v) {
  newcg = v;
}
inline void FAMGPaList::SetApprox(double v) {
  approx = v;
}
inline void FAMGPaList::SetCoeff(int i, double c) {
  coeff[i] = c;
}
inline void FAMGPaList::SetCoefft(int i, double c) {
  coefft[i] = c;
}
inline void FAMGPaList::SetNext(FAMGPaList* ptr) {
  next = ptr;
}

#endif


struct FAMGNodeBitField
{
  unsigned f0 : 1;
  unsigned f1 : 1;
  unsigned f2 : 1;
  unsigned nt : 2;
  unsigned ns : 10;
#ifdef ModelP
  unsigned newmarked : 1;
#endif
};

class FAMGNode
{
  friend class FAMGGraph;       // to access NodeMarkCG and NodeMarkFG
public:
  int GetData() const;
  const FAMGVectorEntry &GetVec() const;
  int GetNSons() const;
  int GetLocalId() const;
  int GetId() const;
  double GetLocalNormA() const;
  double GetLocalNormB() const;
  FAMGNode* GetPred() const;
  FAMGNode* GetSucc() const;
  FAMGList* GetList() const;
  FAMGPaList* GetPaList() const;
  void SetPred(FAMGNode *);
  void SetSucc(FAMGNode *);
  void SetList(FAMGList *);
  void SetData(int val);
  void SetVec(const FAMGVectorEntry &);
  void SetNSons(int);
  void SetId(int);
  void SetLocalId(int);
  void SetLocalNormA(double);
  void SetLocalNormB(double);
  void SetPaList(FAMGPaList*);
  int GetFlag() const;
  int GetFlag1() const;
  int GetFlag2() const;
  int GetFlagNewMarked() const;
  int IsCGNode() const;
  int IsFGNode() const;
  int IsUndecidedNode() const;
  void SetUndecidedNode();
  void SetFlag(int);
  void SetFlag1(int);
  void SetFlag2(int);
  void SetFlagNewMarked(int);
  void Init(int index, const FAMGVectorEntry &id);
  int UpdateNeighborsFG(class FAMGGrid *grid);
  int Eliminate(FAMGGrid *grid);
  void MarkBestNeighbor(FAMGGrid *grid);
  int CountNewLinks(FAMGGrid *grid, class FAMGGraph *graph);
  void CountNewCG(class FAMGGraph *graph);
  int CountNewCG(FAMGGraph *graph, int j);
  void ComputeTotalWeight();
  int CheckPaList(FAMGGraph *graph);
protected:
  void NodeMarkCG();
  void NodeMarkFG();
private:
  FAMGNodeBitField control;
  int data;                                                     // rating
  int local_id;                                         // index into local array
  int myid;                                                     // my index in the node table
  FAMGVectorEntry myvec;                        // corresponding vector
  class FAMGNode *pred, *succ;          // linked list
  class FAMGList *list;
  class FAMGPaList *palist;
};

inline int FAMGNode::GetData() const {
  return data;
}
inline const FAMGVectorEntry &FAMGNode::GetVec() const {
  return myvec;
}
inline int FAMGNode::GetNSons() const {
  return control.ns;
}
inline int FAMGNode::GetId() const {
  return myid;
}
inline int FAMGNode::GetLocalId() const {
  return local_id;
}
inline FAMGNode* FAMGNode::GetPred() const {
  return pred;
}
inline FAMGNode* FAMGNode::GetSucc() const {
  return succ;
}
inline FAMGList* FAMGNode::GetList() const {
  return list;
}
inline FAMGPaList* FAMGNode::GetPaList() const {
  return palist;
}
inline void FAMGNode::SetData(int val) {
  data = val;
}
inline void FAMGNode::SetVec(const FAMGVectorEntry &i) {
  myvec = i;
}
inline void FAMGNode::SetNSons(int i) {
  control.ns = i;
}
inline void FAMGNode::SetId(int i) {
  myid = i;
}
inline void FAMGNode::SetLocalId(int i) {
  local_id = i;
}
inline void FAMGNode::SetList(FAMGList *l) {
  list = l;
}
inline void FAMGNode::SetPred(FAMGNode *p) {
  pred = p;
}
inline void FAMGNode::SetSucc(FAMGNode *s) {
  succ = s;
}
inline void FAMGNode::SetPaList(FAMGPaList *ptr) {
  palist = ptr;
}
inline int FAMGNode::IsCGNode() const {
  return (control.nt == 2);
}
inline int FAMGNode::IsFGNode() const {
  return (control.nt == 1);
}
inline int FAMGNode::IsUndecidedNode() const {
  return (control.nt == 0);
}
inline void FAMGNode::SetUndecidedNode() {
  control.nt = 0;
}
inline int FAMGNode::GetFlag() const {
  return control.f0;
}
inline int FAMGNode::GetFlag1() const {
  return control.f1;
}
inline int FAMGNode::GetFlag2() const {
  return control.f2;
}
#ifdef ModelP
inline int FAMGNode::GetFlagNewMarked() const {
  return control.newmarked;
}
inline void FAMGNode::SetFlagNewMarked(int f) {
  control.newmarked = f;
}
inline void FAMGNode::NodeMarkCG() { /*printf("%d: node %d marked Coarse "VINDEX_FMTX"\n",me,GetId(),VINDEX_PRTX(myvec.myvector()));*/
  control.nt = 2;SetFlagNewMarked(1);
}                                                                                                                                                                               // only in the node; ensure that the CG is also set in the gridvector
inline void FAMGNode::NodeMarkFG() { /*printf("%d: node %d marked Fine "VINDEX_FMTX"\n",me,GetId(),VINDEX_PRTX(myvec.myvector()));*/
  control.nt = 1;SetFlagNewMarked(1);
}                                                                                                                                                                       // only in the node; ensure that the FG is also set in the gridvector
#else
inline void FAMGNode::NodeMarkCG() { /*printf("node %d marked Coarse\n",GetId());*/
  control.nt = 2;
}                                                                                                       // only in the node; ensure that the CG is also set in the gridvector
inline void FAMGNode::NodeMarkFG() { /*printf("node %d marked Fine\n",GetId());*/
  control.nt = 1;
}                                                                                                       // only in the node; ensure that the FG is also set in the gridvector
#endif
inline void FAMGNode::SetFlag(int f) {
  control.f0 = f;
}
inline void FAMGNode::SetFlag1(int f) {
  control.f1 = f;
}
inline void FAMGNode::SetFlag2(int f) {
  control.f2 = f;
}


class FAMGGraph
{
public:
  ~FAMGGraph() {
    delete[] node;
  }
  int* GetMap() const;
  class FAMGDecompRow* GetRow() const;
  int GetN() const;
  int GetNF() const;
  FAMGList* GetList() const;
  void SetList(FAMGList*);
  FAMGList* GetFreeList() const;
  void SetFreeList(FAMGList*);
  FAMGNode* GetHelpList() const;
  void SetHelpList(FAMGNode*);
  int Insert(FAMGNode *);
  void Remove(FAMGNode *);
  void Store(FAMGNode *);
  int InsertHelplist();
  FAMGNode *GetNode(const FAMGVectorEntry &ve) const;
  FAMGNode *GetNode(int index) const;
  FAMGNode *GetNodePtr() const;
  FAMGNode *GetFirstNode();
  FAMGNode *GetLastNode();
  int Init(class FAMGGrid*);
  int Construct(class FAMGGrid *);
  int Construct2(class FAMGGrid *);
  int InitList();
  FAMGPaList *GetFreePaList() const;
  void SetFreePaList(FAMGPaList * ptr);
  void MarkFGNode(FAMGNode *);
  void MarkCGNode(FAMGNode *);
  void ClearPaList(FAMGPaList *);
  void ClearPaListRev(FAMGPaList *&);
  void CorrectPaList(FAMGPaList *&palist, double threshold);
#ifdef FAMG_SPARSE_BLOCK
  int SavePaList(FAMGPaList *&list, int np, const int pa[], double **c, double** ct, double error);
  void ConstructSparseBlocks(class FAMGGrid *);
#else
  int SavePaList(FAMGPaList *&list, int np, const int pa[], double c[], double ct[], double error);
#endif
  int EliminateNodes(FAMGGrid *gridptr);
  int EliminateDirichletNodes(FAMGGrid *gridptr);
  int RemainingNodes( FAMGGrid *grid, int doInconsistent = 0 );
  void UpdateNSons(FAMGPaList *newlist, FAMGPaList *oldlist, FAMGGrid *grid);
  void InitNSons();
  FAMGGridVector &GetGridVector() const {
    return *gridvec;
  }
  void SetGridVector(FAMGGridVector *gv) {
    gridvec = gv;
  }
  int InsertNode(FAMGGrid *gridptr, FAMGNode *nodei);
#ifdef FAMG_ILU
  int OrderILUT(FAMGMatrix *matrix);
#endif
#ifdef FAMG_SPARSE_BLOCK
  const FAMGSparseBlock *Get_sb1Ptr() const;
  const FAMGSparseBlock *Get_sb1TPtr() const;
  const FAMGSparseBlock *Get_sb2Ptr() const;
  const FAMGSparseBlock *Get_sb2TPtr() const;
  const FAMGSparseBlock *Get_sb3Ptr() const;
  const FAMGSparseBlock *Get_sb3TPtr() const;
  const FAMGSparseBlock *Get_sb0aPtr() const;
  const FAMGSparseBlock *Get_sb0aTPtr() const;
  const FAMGSparseBlock *Get_sbiaPtr() const;
  const FAMGSparseBlock *Get_sbiaTPtr() const;

  const FAMGSparseVector *Get_stvPtr() const;
  const FAMGSparseVector *Get_stvTPtr() const;
  const FAMGSparseVector *Get_spiPtr() const;
  const FAMGSparseVector *Get_spiTPtr() const;
  const FAMGSparseVector *Get_sp0Ptr() const;
  const FAMGSparseVector *Get_sp0TPtr() const;
  const FAMGSparseVector *Get_sp0iPtr() const;
  const FAMGSparseVector *Get_sp0iTPtr() const;
  const FAMGSparseVector *Get_spPtr() const;
  const FAMGSparseVector *Get_srPtr() const;
#endif
private:
  int n;                        // number nodes
  int nf;                               // number fine grid nodes
  FAMGNode *node;       // array of nodes
  FAMGList *list;
  FAMGNode *helplist;
  FAMGPaList *freepalist;
  FAMGList *freelist;
  FAMGGridVector *gridvec;
#ifdef FAMG_ILU
  int *map;
#endif
#ifdef FAMG_SPARSE_BLOCK
  FAMGSparseBlock sb1;
  FAMGSparseBlock sb1T;
  FAMGSparseBlock sb2;
  FAMGSparseBlock sb2T;
  FAMGSparseBlock sb3;
  FAMGSparseBlock sb3T;
  FAMGSparseBlock sb0a;
  FAMGSparseBlock sb0aT;
  FAMGSparseBlock sbia;
  FAMGSparseBlock sbiaT;

  FAMGSparseVector stv;
  FAMGSparseVector stvT;
  FAMGSparseVector spi;
  FAMGSparseVector spiT;
  FAMGSparseVector sp0;
  FAMGSparseVector sp0T;
  FAMGSparseVector sp0i;
  FAMGSparseVector sp0iT;
  FAMGSparseVector sp;
  FAMGSparseVector sr;
#endif
};

#ifdef FAMG_ILU
inline int* FAMGGraph::GetMap() const {
  return map;
}
#endif
inline int FAMGGraph::GetNF() const {
  return nf;
}
inline int FAMGGraph::GetN() const {
  return n;
}
inline FAMGNode *FAMGGraph::GetNode(int index) const {
  assert(index<GetN());return node+index;
}
inline FAMGNode *FAMGGraph::GetNode(const FAMGVectorEntry &ve) const {
  return GetNode(ve.GetIndex());
}
inline FAMGNode *FAMGGraph::GetNodePtr() const {
  return node;
}
inline FAMGList *FAMGGraph::GetList() const {
  return list;
}
inline void FAMGGraph::SetList(FAMGList *pl) {
  list = pl;
}
inline FAMGList *FAMGGraph::GetFreeList() const {
  return freelist;
}
inline void FAMGGraph::SetFreeList(FAMGList *pl) {
  freelist = pl;
}
inline FAMGNode *FAMGGraph::GetHelpList() const {
  return helplist;
}
inline void FAMGGraph::SetHelpList(FAMGNode *nodel) {
  helplist = nodel;
}
inline FAMGPaList *FAMGGraph::GetFreePaList() const {
  return freepalist;
}
inline void FAMGGraph::SetFreePaList(FAMGPaList * ptr) {
  freepalist = ptr;
}

#ifdef FAMG_SPARSE_BLOCK
inline const FAMGSparseBlock *FAMGGraph::Get_sb1Ptr() const {
  return &sb1;
}
inline const FAMGSparseBlock *FAMGGraph::Get_sb1TPtr() const {
  return &sb1T;
}
inline const FAMGSparseBlock *FAMGGraph::Get_sb2Ptr() const {
  return &sb2;
}
inline const FAMGSparseBlock *FAMGGraph::Get_sb2TPtr() const {
  return &sb2T;
}
inline const FAMGSparseBlock *FAMGGraph::Get_sb3Ptr() const {
  return &sb3;
}
inline const FAMGSparseBlock *FAMGGraph::Get_sb3TPtr() const {
  return &sb3T;
}
inline const FAMGSparseBlock *FAMGGraph::Get_sb0aPtr() const {
  return &sb0a;
}
inline const FAMGSparseBlock *FAMGGraph::Get_sb0aTPtr() const {
  return &sb0aT;
}
inline const FAMGSparseBlock *FAMGGraph::Get_sbiaPtr() const {
  return &sbia;
}
inline const FAMGSparseBlock *FAMGGraph::Get_sbiaTPtr() const {
  return &sbiaT;
}

inline const FAMGSparseVector *FAMGGraph::Get_stvPtr() const {
  return &stv;
}
inline const FAMGSparseVector *FAMGGraph::Get_stvTPtr() const {
  return &stvT;
}
inline const FAMGSparseVector *FAMGGraph::Get_spiPtr() const {
  return &spi;
}
inline const FAMGSparseVector *FAMGGraph::Get_spiTPtr() const {
  return &spiT;
}
inline const FAMGSparseVector *FAMGGraph::Get_sp0Ptr() const {
  return &sp0;
}
inline const FAMGSparseVector *FAMGGraph::Get_sp0TPtr() const {
  return &sp0T;
}
inline const FAMGSparseVector *FAMGGraph::Get_sp0iPtr() const {
  return &sp0i;
}
inline const FAMGSparseVector *FAMGGraph::Get_sp0iTPtr() const {
  return &sp0iT;
}
inline const FAMGSparseVector *FAMGGraph::Get_spPtr() const {
  return &sp;
}
inline const FAMGSparseVector *FAMGGraph::Get_srPtr() const {
  return &sr;
}
#endif

#endif
