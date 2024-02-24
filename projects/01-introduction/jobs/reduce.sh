#!/bin/bash
#PBS -l walltime=00:01:00,nodes=2:ppn=2
#PBS -N intro_reduce_job
#PBS -q batch
cd "$PBS_O_WORKDIR" || exit
NUM_ELEMENTS=1000000
mpirun --hostfile "$PBS_NODEFILE" -np 4 ../install/bin/reduce -n$NUM_ELEMENTS
