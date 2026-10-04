#!/usr/bin/env bash

set -e

OUTPUT_DIR="data/output/locked_piece_stats"

mkdir -p "$OUTPUT_DIR"

for turn in {0..15}
do
    # White vision
    ./bin/locked_piece_stats "$turn" 0 > "$OUTPUT_DIR/locked_piece_stats_white_${turn}.txt" 2>&1

    # Black vision
    ./bin/locked_piece_stats "$turn" 1 > "$OUTPUT_DIR/locked_piece_stats_black_${turn}.txt" 2>&1
done
