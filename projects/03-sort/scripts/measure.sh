#!/usr/bin/env bash

MAX_PROC=$(lscpu | awk -F ":" '/Core/ { c=$2; }; /Socket/ { print c*$2 }' )
NUM=8000000
SAMPLES=8
RAND_MIN=-1000000
RAND_MAX=+1000000

echo "# 'n, number processes' 'time (serial), ms' 'time (mpi)' 'ratio'" >> "${OUT_BASENAME}"

for NUM_PROC in $(seq 1 "$MAX_PROC"); do
  mpirun -np "${NUM_PROC}" ./sort --num=${NUM} --min=${RAND_MIN} --max=${RAND_MAX} --samples=${SAMPLES} >> "${OUT_BASENAME}"
done
