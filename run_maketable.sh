#!/bin/bash

for n in {0..3}; do
    ./maketable.out 0 0 4 "$n" db db db > "0_4_${n}_w.txt" 2>&1 &
done

wait  # すべてのバックグラウンドプロセスが終わるのを待つ