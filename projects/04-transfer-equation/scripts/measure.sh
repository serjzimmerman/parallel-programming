#!/usr/bin/env bash

MAX_PROC=$(lscpu | awk -F ":" '/Core/ { c=$2; }; /Socket/ { print c*$2 }' )
SAMPLES=512
PARAMS="--h 0.001 --tau 0.01 --t 50.0 --b 1.0"

echo "# 'n, number processes' 'time (serial), ms' 'time (mpi)' 'ratio' 'eff'" > "${OUT_BASENAME}"

serial_time=$(./transfer-solver --samples="$SAMPLES" --measure $PARAMS)
for NUM_PROC in $(seq 1 "$MAX_PROC"); do
  parallel_time=$(mpirun -np "$NUM_PROC" ./transfer-solver --samples="$SAMPLES" --measure $PARAMS)
  ratio=$(echo "${serial_time} / ${parallel_time}" | bc -l)
  eff=$(echo "${ratio} / ${NUM_PROC}" | bc -l)
  echo "${NUM_PROC} ${serial_time} ${parallel_time} ${ratio} ${eff}" >> "${OUT_BASENAME}"
done
