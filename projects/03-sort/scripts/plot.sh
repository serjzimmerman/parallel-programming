#!/usr/bin/env bash

gnuplot -p -e "set terminal pdf; set logscale x 2; plot '${OUT_BASENAME}' using 1:4 with lp" > "${OUT_BASENAME}.pdf"

