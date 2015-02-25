#ifndef CONSTANT_H
#define CONSTANT_H

typedef struct 
{
  volatile int global  __attribute__((aligned(64)));
} counter_t;

static const double ONE_THIRD = 0.3333333333333333;

/* alignment */
#ifdef USE_ALIGNMENT
static const int CL=8;
#else
static const int CL=1;
#endif

/* dimensions */
static const int nThreads = 12;
static const int M_SZ     = 2048;
static const int K_SZ     = 2048;
static const int NITER    = 25;

#endif
