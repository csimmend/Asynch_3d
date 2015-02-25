#!/bin/bash

#export OMP_PROC_BIND=true
#numactl --preferred=0 --cpunodebind=0 $@
#unset OMP_PROC_BIND
#RANK=$OMPI_COMM_WORLD_RANK
RANK=$PMI_RANK
SOCKET=$(( $RANK % 2 ))
export OMP_NUM_THREADS=12

if [ "x"$SOCKET == "x0" ]; then
#    echo "$RANK - socket 0"
#    export KMP_AFFINITY=verbose,granularity=thread,proclist=[0,1,2,3,4,5,6,7,8,9,10,11,24,25,26,27,28,29,30,31,32,33,34,35],explicit
#    export KMP_AFFINITY=verbose,granularity=thread,proclist=[0,1,2,3,4,5,6,7,8,9,10,11],explicit
    export KMP_AFFINITY=granularity=thread,proclist=[0,1,2,3,4,5,6,7,8,9,10,11],explicit
#    export GOMP_CPU_AFFINITY="0-23"
#    export GOMP_CPU_AFFINITY="0-11 24-35"
#    export GOMP_CPU_AFFINITY="0 1 2 3 4 5 6 7 8 9 10 11 24 25 26 27 28 29 30 31 32 33 34 35"
#    numactl --physcpubind="0,1,2,3,4,5,6,7,8,9,10,11,24,25,26,27,28,29,30,31,32,33,34,35"  $@
#    numactl --cpunodebind=0 $@
#    taskset -c 0,1,2,3,4,5,6,7,8,9,10,11,24,25,26,27,28,29,30,31,32,33,34,35 $@
    exec $@
else
#    echo "$RANK - socket 1"
#    export KMP_AFFINITY=verbose,granularity=thread,proclist=[12,13,14,15,16,17,18,19,20,21,22,23,36,37,38,39,40,41,42,43,44,45,46,47],explicit
#    export KMP_AFFINITY=verbose,granularity=thread,proclist=[12,13,14,15,16,17,18,19,20,21,22,23],explicit
    export KMP_AFFINITY=granularity=thread,proclist=[12,13,14,15,16,17,18,19,20,21,22,23],explicit
#    export GOMP_CPU_AFFINITY="0-23"
#    export GOMP_CPU_AFFINITY="12-23 36-47"
#    export GOMP_CPU_AFFINITY="12 13 14 15 16 17 18 19 20 21 22 23 36 37 38 39 40 41 42 43 44 45 46 47"
#    numactl --physcpubind="12,13,14,15,16,17,18,19,20,21,22,23,36,37,38,39,40,41,42,43,44,45,46,47"  $@
#    numactl --cpunodebind=1 $@ 
#    taskset -c 12,13,14,15,16,17,18,19,20,21,22,23,36,37,38,39,40,41,42,43,44,45,46,47 $@
    exec $@
fi




