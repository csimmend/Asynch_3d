#ifndef SUCCESS_OR_DIE_H
#define SUCCESS_OR_DIE_H

#include <stdlib.h>

static __inline void _mm_pause (void)
{
  __asm__ __volatile__ ("rep; nop" : : );
}


#endif
