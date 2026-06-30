#!/usr/bin/env bash

for n in {0..1}; do
    ./bin/main 14 0 2 "$n" db db db > "data/output/14_4_${n}_w.txt" 2>&1 &
done

wait  # すべてのバックグラウンドプロセスが終わるのを待つ
