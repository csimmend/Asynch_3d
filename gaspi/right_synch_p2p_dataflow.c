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
#include "mm_pause.h"
#include "success_or_die.h"
#include "queue.h"

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

  gaspi_rank_t iProcG, nProcG;
  SUCCESS_OR_DIE (gaspi_proc_init (GASPI_BLOCK));
  SUCCESS_OR_DIE (gaspi_proc_rank (&iProcG));
  SUCCESS_OR_DIE (gaspi_proc_num (&nProcG));
  ASSERT(iProc == iProcG);
  ASSERT(nProc == nProcG);

  gaspi_number_t notification_num;
  SUCCESS_OR_DIE (gaspi_notification_num (&notification_num));
  ASSERT(K_SZ*nThreads <= notification_num);

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

  // allocate segment for array
  gaspi_segment_id_t const segment_id = 0;
  SUCCESS_OR_DIE ( gaspi_segment_create
                   ( segment_id
                     , CL_SZ * (nThreads+1) * (K_SZ+1) * sizeof (double)
                     , GASPI_GROUP_ALL
                     , GASPI_BLOCK
                     , GASPI_MEM_UNINITIALIZED
                     ));
  gaspi_pointer_t array;
  SUCCESS_OR_DIE ( gaspi_segment_ptr ( segment_id, &array) );
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
	gaspi_queue_id_t queue_id = 0;
	int k;

	for (k = 1; k <= K_SZ; k++) 
	  {
	    if (left >= 0 )
	      {
		gaspi_notification_id_t id, data_available = (k-1)*nThreads+tid;
		SUCCESS_OR_DIE(gaspi_notify_waitsome (segment_id
						      , data_available
						      , 1
						      , &id
						      , GASPI_BLOCK
						      ));
		ASSERT (id == data_available);          
		gaspi_notification_t value;
		SUCCESS_OR_DIE (gaspi_notify_reset (segment_id
						    , id
						    , &value
						    ));
		ASSERT (value == 1);
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
	    if (right < nProc)
	      {
		gaspi_notification_id_t data_available = (k-1)*nThreads+tid;
		wait_for_queue_entries_for_write_notify(&queue_id);
		SUCCESS_OR_DIE ( gaspi_write_notify
				 ( segment_id
				   , array_OFFSET (mSize, tid+1, k)
				   , right
				   , segment_id
				   , array_OFFSET (0, tid+1, k)
				   , sizeof (double)
				   , data_available
				   , 1
				   , queue_id
				   , GASPI_BLOCK
				   ));
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

  printf ("# gaspi %s nProc: %d nThreads: %d M_SZ: %d K_SZ: %d niter: %d time: %g\n"
	  , argv[0], nProc, nThreads, M_SZ, K_SZ, NITER, median[NITER/2]
         );

  if (iProc == nProc-1) 
    {
      double res = 1.0E-06 * 4 * mSize*nThreads*K_SZ*nProc / median[NITER/2];
      printf("\nRate (MFlops/s): %lf\n",res);
    }
  
  MPI_Barrier(MPI_COMM_WORLD);
 
  MPI_Finalize();


  return EXIT_SUCCESS;

}
