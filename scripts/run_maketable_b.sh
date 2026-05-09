#!/bin/bash

for n in {0..3}; do
    ./bin/main 15 1 4 "$n" db db db > "data/output/15_4_${n}_b.txt" 2>&1 &
done

wait  # すべてのバックグラウンドプロセスが終わるのを待つ
