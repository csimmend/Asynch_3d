#ifndef DATA_H
#define DATA_H

#include "constant.h"

#define POSITION(i,j,k) ((i) + CL_SZ * (j) + CL_SZ * (nThreads+1) * (k))
#define array_ELEM(i,j,k) ((double *)array)[POSITION (i,j,k)]
#define array_OFFSET(i,j,k) (POSITION (i,j,k) * sizeof(double))

void data_init_tlocal(int mStart, int tid, double* array, int CL_SZ);
void data_init_global(int mStart, int mSize, int iProc, double* array, int CL_SZ);

void data_compute (int mStart, int mSize, int tid, int k, double* array, int CL_SZ);
void data_validate (int mStart, int mSize, int tid, int k, double* array, int CL_SZ);
void sort_median(double *begin, double *end);

#endif
