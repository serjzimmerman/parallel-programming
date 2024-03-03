#!/usr/bin/env bash

for t in send ssend bsend rsend; do
  gnuplot -p -e "set terminal pdf; set logscale x 2; plot '${OUT_BASENAME}.${t}' with lines" > "${OUT_BASENAME}".${t}.pdf
done

