// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*																			*/
/* File:      famg_system.h													*/
/*																			*/
/* Purpose:   famg system class												*/
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

#ifndef __FAMG_SYSTEM__
#define __FAMG_SYSTEM__

#include <string.h>
#include "famg_matrix.h"
#include "famg_grid.h"
#include "famg_multigrid.h"

/* RCS_ID
   $Header$
 */

const int FAMGMULTIGRIDS=1;

class FAMGParameter  // struct makes it easier than class
{
public:
  int Getheap();
  int Getnv();
  int Getgamma();
  int Getn1();
  int Getn2();
  double Getcgilut();
  double Getilut();
  int Getcgnodes();
  double Getmincoarse();
  int Getconloops();
  int Gettype();
  int Getstv();
  double Gettol();
  double Getsigma();
  double Getomegar();
  double Getomegal();
  double Geterror1();
  double Geterror2();
  int Getmaxit();
  double Getalimit();
  double Getrlimit();
  double Getdivlimit();
  double Getreduction();
  char* Getsolver();
  char* Getpresmoother();
  char* Getpostsmoother();
  char* Getcgsmoother();
  int Read();
  void Setheap(int i);
  void Setnv(int i);
  void Setgamma(int i);
  void Setn1(int i);
  void Setn2(int i);
  void Setilut(double d);
  void Setcgilut(double d);
  void Setcgnodes(int i);
  void Setmincoarse(double d);
  void Setconloops(int i);
  void Settype(int i);
  void Setstv(int i);
  void Settol(double d);
  void Setsigma(double d);
  void Setomegar(double d);
  void Setomegal(double d);
  void Seterror1(double d);
  void Seterror2(double d);
  void Setmaxit(int i);
  void Setalimit(double d);
  void Setrlimit(double d);
  void Setdivlimit(double d);
  void Setreduction(double d);
  void Setsolver(char *solver);
  void Setpresmoother(char *presmoother);
  void Setpostsmoother(char *postsmoother);
  void Setcgsmoother(char *cgsmoother);
private:
  int heap;
  int nv;
  int gamma;
  int n1;
  int n2;
  double ilut;
  double cgilut;
  int cgnodes;
  double mincoarse;
  int conloops;
  int type;
  int stv;
  double tol;
  double sigma;
  double omegar;
  double omegal;
  double error1;
  double error2;
  int maxit;
  double alimit;
  double rlimit;
  double divlimit;
  double reduction;
  char solver[10];
  char presmoother[10];
  char postsmoother[10];
  char cgsmoother[10];
};

inline int FAMGParameter::Getheap() {
  return heap;
}
inline int FAMGParameter::Getnv() {
  return nv;
}
inline int FAMGParameter::Getgamma() {
  return gamma;
}
inline int FAMGParameter::Getn1() {
  return n1;
}
inline int FAMGParameter::Getn2() {
  return n2;
}
inline double FAMGParameter::Getilut() {
  return ilut;
}
inline double FAMGParameter::Getcgilut() {
  return cgilut;
}
inline int FAMGParameter::Getcgnodes() {
  return cgnodes;
}
inline double FAMGParameter::Getmincoarse() {
  return mincoarse;
}
inline int FAMGParameter::Getconloops() {
  return conloops;
}
inline int FAMGParameter::Gettype() {
  return type;
}
inline int FAMGParameter::Getstv() {
  return stv;
}
inline double FAMGParameter::Gettol() {
  return tol;
}
inline double FAMGParameter::Getsigma() {
  return sigma;
}
inline double FAMGParameter::Getomegar() {
  return omegar;
}
inline double FAMGParameter::Getomegal() {
  return omegal;
}
inline double FAMGParameter::Geterror1() {
  return error1;
}
inline double FAMGParameter::Geterror2() {
  return error2;
}
inline int FAMGParameter::Getmaxit() {
  return maxit;
}
inline double FAMGParameter::Getalimit() {
  return alimit;
}
inline double FAMGParameter::Getrlimit() {
  return rlimit;
}
inline double FAMGParameter::Getdivlimit() {
  return divlimit;
}
inline double FAMGParameter::Getreduction() {
  return reduction;
}
inline char* FAMGParameter::Getsolver() {
  return solver;
}
inline char* FAMGParameter::Getpresmoother() {
  return presmoother;
}
inline char* FAMGParameter::Getpostsmoother() {
  return postsmoother;
}
inline char* FAMGParameter::Getcgsmoother() {
  return cgsmoother;
}

inline void FAMGParameter::Setheap(int i) {
  heap = i;
}
inline void FAMGParameter::Setnv(int i) {
  nv = i;
}
inline void FAMGParameter::Setgamma(int i) {
  gamma = i;
}
inline void FAMGParameter::Setilut(double d) {
  ilut = d;
}
inline void FAMGParameter::Setcgilut(double d) {
  cgilut = d;
}
inline void FAMGParameter::Setn1(int i) {
  n1 = i;
}
inline void FAMGParameter::Setn2(int i) {
  n2 = i;
}
inline void FAMGParameter::Setcgnodes(int i) {
  cgnodes = i;
}
inline void FAMGParameter::Setmincoarse(double d) {
  mincoarse = d;
}
inline void FAMGParameter::Setconloops(int i) {
  conloops = i;
}
inline void FAMGParameter::Settype(int i) {
  type = i;
}
inline void FAMGParameter::Setstv(int i) {
  stv = i;
}
inline void FAMGParameter::Settol(double d) {
  tol = d;
}
inline void FAMGParameter::Setsigma(double d) {
  sigma = d;
}
inline void FAMGParameter::Setomegar(double d) {
  omegar = d;
}
inline void FAMGParameter::Setomegal(double d) {
  omegal = d;
}
inline void FAMGParameter::Seterror1(double d) {
  error1 = d;
}
inline void FAMGParameter::Seterror2(double d) {
  error2 = d;
}
inline void FAMGParameter::Setmaxit(int i) {
  maxit = i;
}
inline void FAMGParameter::Setalimit(double d) {
  alimit = d;
}
inline void FAMGParameter::Setrlimit(double d) {
  rlimit = d;
}
inline void FAMGParameter::Setdivlimit(double d) {
  divlimit = d;
}
inline void FAMGParameter::Setreduction(double d) {
  reduction = d;
}
inline void FAMGParameter::Setsolver(char *ptr) {
  strcpy(solver,ptr);
}
inline void FAMGParameter::Setpresmoother(char *ptr) {
  strcpy(presmoother,ptr);
}
inline void FAMGParameter::Setpostsmoother(char *ptr) {
  strcpy(postsmoother,ptr);
}
inline void FAMGParameter::Setcgsmoother(char *ptr) {
  strcpy(cgsmoother,ptr);
}


class FAMGSystem
{
public:
  FAMGSystem();
  FAMGMatrix * GetMatrix() const;
  int GetN() const;
  double *GetVector(int i) const;
  void **GetExtra() const;
  FAMGMultiGrid *GetMultiGrid(int) const;
  void SetMatrix(FAMGMatrix *);
  void SetExtra(void **);
  void SetVector(int, double *);
  void SetN(int);
  int Init();
  FAMGMultiGrid *CreateMultiGrid();
  int Solve(double *rhs, double *defect, double *unknown);
  int LinIt();
  int AdTVSolve();
  int BiCGStab();
  int BiCG();
  int Arnoldi(FAMGMultiGrid *mg0, double **vec, double *H, double *G, double *Q, double *P, double &q0, int con);
  int ArnoldiTrans(FAMGMultiGrid *mg0, double **vec, double *H, double *G, double *Q, double *P, double &q0, int con);
  int UpdateSolution(FAMGMultiGrid *mg0, double **vec, double *H, double *Q, double &q0, int con);
  int ComputeEigenVector(FAMGMultiGrid *mg0, double **vec, double *G, double *P, int con);
  int ComputeEigenVectorTrans(FAMGMultiGrid *mg0, double **vec, double *G, double *P, int con);
  int GMRES();
  int Construct(double *entr, int *index, int *start, int n, int nl, double *tvA, double *tvB, void **extraptr);
  int ConstructSimple(double *entr, int *index, int *start, int n, int nl, void **extraptr);
  int Deconstruct();
  int DeconstructSimple();
private:
  int nmg;
  int n;             // unknowns
  FAMGMultiGrid *mg[FAMGMULTIGRIDS];
  FAMGMatrix *matrix;
  double *vector[FAMGMAXVECTORS];
  int *colmap;
  void **extra;
  int (FAMGSystem::*SolverPtr)(void);

};

inline FAMGMatrix *FAMGSystem::GetMatrix() const {
  return matrix;
}
inline int FAMGSystem::GetN() const {
  return n;
}
inline double *FAMGSystem::GetVector(int i) const {
  return vector[i];
}
inline void **FAMGSystem::GetExtra() const {
  return extra;
}
inline FAMGMultiGrid *FAMGSystem::GetMultiGrid(int i) const {
  return mg[i];
}
inline void FAMGSystem::SetN(int i) {
  n = i;
}
inline void FAMGSystem::SetMatrix(FAMGMatrix *ptr) {
  matrix = ptr;
}
inline void FAMGSystem::SetExtra(void **ptr) {
  extra = ptr;
}
inline void FAMGSystem::SetVector(int i, double *ptr) {
  vector[i] = ptr;
}

FAMGParameter * FAMGGetParameter();
void FAMGSetParameter(FAMGParameter *ptr);

#endif