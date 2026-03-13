#!/bin/bash

for n in {0..3}; do
    ./maketable.out 14 1 4 "$n" db db db > "14_4_${n}_b.txt" 2>&1 &
done

wait  # すべてのバックグラウンドプロセスが終わるのを待つ
