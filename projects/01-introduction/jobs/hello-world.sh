#!/bin/bash
#PBS -l walltime=00:00:15,nodes=2:ppn=2
#PBS -N intro_hello_job
#PBS -q batch
cd "$PBS_O_WORKDIR" || exit
mpirun --hostfile "$PBS_NODEFILE" -np 4 ../install/bin/hello
