#!/usr/bin/env bash

MAX_PROC=$(lscpu | awk -F ":" '/Core/ { c=$2; }; /Socket/ { print c*$2 }' )
RAND_MIN=-1000000
RAND_MAX=+1000000

echo "# 'n, number processes' 'time (serial), ms' 'time (mpi)' 'ratio'" > "${OUT_BASENAME}"

serial_time=$(./sort --num="$NUM" --min="$RAND_MIN" --max="$RAND_MAX" --samples="$SAMPLES")
for NUM_PROC in $(seq 1 "$MAX_PROC"); do
  parallel_time=$(mpirun -np "$NUM_PROC" ./sort --num="$NUM" --min="$RAND_MIN" --max="$RAND_MAX" --samples="$SAMPLES" --parallel)
  ratio=$(echo "${serial_time} / ${parallel_time}" | bc -l)
  echo "${NUM_PROC} ${serial_time} ${parallel_time} ${ratio}" >> "${OUT_BASENAME}"
done
