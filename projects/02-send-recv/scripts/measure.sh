#!/usr/bin/env bash

BYTES=1
BASE=2
MAX_POW=16

for t in send ssend bsend rsend; do
  echo "# 'n, bytes' 'time, microseconds'" >> "${OUT_BASENAME}".${t}
done

for _ in $(seq 1 $MAX_POW); do
  BYTES=$((BYTES * BASE))

  for t in send ssend bsend rsend; do
    mpirun -np 2 bench -t ${t} --bytes=${BYTES} >> "${OUT_BASENAME}".${t}
  done
done
