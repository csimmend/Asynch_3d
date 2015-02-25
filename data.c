#include "data.h"
#include "assert.h"
#include "math.h"

void data_init_tlocal(int mSize, int tid, double* array, int CL_SZ)
{  

  int i, k;

  /* clear the array, first touch strategy */  
  if (tid == 0)
    {
      for (k = 0; k <= K_SZ; k++)
	{
	  for (i = 0; i <= mSize; i++) 	
	    {
	      array_ELEM(i,0,k) = 0.0;
	    }
	}
    }

  int j = tid+1;
  for (k = 0; k <= K_SZ; k++)
    {
      for (i = 0; i <= mSize; i++) 	
	{
	  array_ELEM(i,j,k) = 0.0;
	}
    }

}

void data_init_global(int mStart, int mSize, int iProc, double* array, int CL_SZ)
{

  int i, j, k;

  /* set boundary values */
  if (iProc == 0) 
    {
      for (k = 0; k <= K_SZ; k++) 
	{
	  for (j = 0; j <= nThreads; j++) 
	    {
	      array_ELEM(0,j,k) = (double) j + k;
	    }
	}
    }

  for (k = 0; k <= K_SZ; k++) 
    {
      for (i = 0; i <= mSize; i++) 	
	{
	  array_ELEM(i,0,k) = (double) mStart - 1 + i + k;
	}
    }

  for (j = 0; j <= nThreads; j++) 	
    {
      for (i = 0; i <= mSize; i++) 	
	{
	  array_ELEM(i,j,0) = (double) mStart - 1 + i + j;
	}
    }
}



void data_compute (int mStart, int mSize, int tid, int k, double* array, int CL_SZ)
{
  const int j = tid + 1;
  int i;
  for (i = 1; i <= mSize; i++) 
    {
      array_ELEM(i,j,k) = (array_ELEM(i-1,j,k)
			   + array_ELEM(i,j-1,k) 
			   + array_ELEM(i,j,k-1)) * ONE_THIRD + 1;
    }


}

void data_validate (int mStart, int mSize, int tid, int k, double* array, int CL_SZ)
{
  int i;
  int j = tid + 1;
  for (i = 1; i <= mSize; i++) 
    {
      ASSERT(array_ELEM(i,j,k)  == (double) i + mStart - 1 + j + k); 
    }

}

static void swap(double *a, double *b)
{
  double tmp = *a;
  *a = *b;
  *b = tmp;
}

void sort_median(double *begin, double *end)
{
  double *ptr;
  double *split;
  if (end - begin <= 1)
    return;
  ptr = begin;
  split = begin + 1;
  while (++ptr != end) {
    if (*ptr < *begin) {
      swap(ptr, split);
      ++split;
    }
  }
  swap(begin, split - 1);
  sort_median(begin, split - 1);
  sort_median(split, end);
}

