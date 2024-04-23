#!/usr/bin/env bash

gnuplot -p -e "set terminal pdf; plot '${OUT_BASENAME}' using 1:4 with lp title 'speedup', '${OUT_BASENAME}' using 1:5 with lp title 'eff'" > \
  "${OUT_BASENAME}.pdf"

