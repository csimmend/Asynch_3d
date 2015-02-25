/*
 * This file is part of a small series of tutorial,
 * which aims to demonstrate key features of the GASPI
 * standard by means of small but expandable examples.
 * Conceptually the tutorial follows a MPI course
 * developed by EPCC and HLRS.
 *
 * Contact point for the MPI tutorial:
 *                 rabenseifner@hlrs.de
 * Contact point for the GASPI tutorial:
 *                 daniel.gruenewald@itwm.fraunhofer.de
 *                 mirko.rahn@itwm.fraunhofer.de
 *                 christian.simmendinger@t-systems.com
 */

#include "assert.h"
#include "constant.h"
#include "data.h"
#include "topology.h"
#include "now.h"
#include "success_or_die.h"

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <omp.h>



/* global stage counters for comp */
static volatile counter_t *compStage = NULL; 

#define MIN(x,y) ((x)<(y)?(x):(y))

int main (int argc, char *argv[])
{
  int i, j;
  int nProc, iProc;
  int provided, required = MPI_THREAD_MULTIPLE;
  MPI_Init_thread(&argc, &argv, required, &provided);
  ASSERT(required == MPI_THREAD_MULTIPLE);

  MPI_Comm_rank (MPI_COMM_WORLD, &iProc);
  MPI_Comm_size (MPI_COMM_WORLD, &nProc);

  // num threads
  omp_set_num_threads(nThreads);

  // global stage counter
  compStage = malloc(nThreads * sizeof(counter_t));

  // left, right neighbour (proc)
  const int left  = LEFT(iProc);
  const int right = RIGHT(iProc);

  // assignment per proc, i-direction 
#ifdef USE_STRONG_SCALING
  int mSize = M_SZ/nProc;
  if (M_SZ % nProc != 0) 
    {
      mSize++;
    }
  const int mStart = iProc*mSize + 1;
  const int mStop  = MIN((iProc+1)*mSize, M_SZ);
  mSize = mStop-mStart+1;
#else
  int mSize = M_SZ;
  const int mStart = iProc*mSize + 1;
  const int mStop  = MIN((iProc+1)*mSize, M_SZ*nProc);
  mSize = mStop-mStart+1;
#endif
 
  // align local array 
  const int CL_SZ = ((mSize+1) % CL) == 0 ? (mSize+1) : CL*(1+(mSize+1)/CL);
  double* array = malloc (CL_SZ * (nThreads+1) * (K_SZ+1) * sizeof (double));
  ASSERT (array != 0);  


#pragma omp parallel default (none) shared(compStage, CL_SZ, \
					   mSize, array, stdout, stderr)
  {
    int const tid = omp_get_thread_num();  
    compStage[tid].global = 0;

    // initialize data
    data_init_tlocal(mSize, tid, array, CL_SZ);
  }


  data_init_global(mStart, mSize, iProc, array, CL_SZ);

  int iter;
  double median[NITER];


  for (iter = 0; iter < NITER; iter++) 
    {
      double time = -now();
      MPI_Barrier(MPI_COMM_WORLD);

#pragma omp parallel default (none) shared(mStart, mSize, \
	    compStage, nThreads, iProc, nProc, stdout, stderr, array, CL_SZ)
      {
	int const tid = omp_get_thread_num();  
	MPI_Status status;
	int k;

	for (k = 1; k <= K_SZ; k++) 
	  {
	    if (left >= 0 )
	      {
		MPI_Recv ( &array_ELEM (0, tid+1, k)
			   , 1
			   , MPI_DOUBLE
			   , left
			   , k*nThreads+tid
			   , MPI_COMM_WORLD
			   , &status
			   );
	      }
	    
	    if(tid > 0)
	      {
		volatile int it;
		while((it = compStage[tid-1].global) <= compStage[tid].global)		  
		  {
		    _mm_pause();
		  }
	      }

	    
	    // compute */ 
	    data_compute (mStart, mSize, tid, k, array, CL_SZ);

	    /* increase stage counter */
	    compStage[tid].global++;

	    // issue send
	    if (right <= nProc - 1)
	      {
		MPI_Send ( &array_ELEM (mSize, tid+1, k)
			   , 1
			   , MPI_DOUBLE
			   , right
			   , k*nThreads+tid
			   , MPI_COMM_WORLD
			   );
	      }

#ifdef USE_OMP_BARRIER
#pragma omp barrier
#endif	    
	  }
      }      

      MPI_Barrier(MPI_COMM_WORLD);
      time += now();

      /* iteration time */
      median[iter] = time;

    }

  MPI_Barrier(MPI_COMM_WORLD);

  // validate */ 
#pragma omp parallel default (none) shared(mStart, array, CL_SZ, mSize)
  {
    int const tid = omp_get_thread_num();  
    data_validate (mStart, mSize, tid, K_SZ, array, CL_SZ);;
  }

  MPI_Barrier(MPI_COMM_WORLD);

  sort_median(&median[0], &median[NITER-1]);
  printf ("# mpi %s nProc: %d nThreads: %d M_SZ: %d K_SZ: %d niter: %d time: %g\n"
	  , argv[0], nProc, nThreads, M_SZ, K_SZ, NITER, median[(NITER-1)/2]
	  );

  if (iProc == nProc-1) 
    {
      double res = 1.0E-06 * 4 * mSize*nThreads*K_SZ*nProc / median[(NITER-1)/2];
      printf("\nRate (MFlops/s): %lf\n",res);
    }

  free(array);

  MPI_Finalize();

  return EXIT_SUCCESS;

}
