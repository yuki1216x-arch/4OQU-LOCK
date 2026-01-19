#!/bin/bash

for n in {0..3}; do
    ./maketable.out 0 1 4 "$n" db db db > "0_4_${n}_b.txt" 2>&1 &
done

wait  # すべてのバックグラウンドプロセスが終わるのを待つ