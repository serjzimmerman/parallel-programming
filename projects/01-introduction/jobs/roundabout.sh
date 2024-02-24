#!/bin/bash
#PBS -l walltime=00:00:10,nodes=4:ppn=2
#PBS -N intro_roundabout_job
#PBS -q batch
cd "$PBS_O_WORKDIR" || exit
mpirun --hostfile "$PBS_NODEFILE" -np 8 ../install/bin/roundabout
