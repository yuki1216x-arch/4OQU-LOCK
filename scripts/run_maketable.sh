#!/usr/bin/env bash

function build_database()
{
    ./bin/main_heiretsu2 "$1" 0 > "data/output/${1}_w_new2.txt" 2>&1 &
    ./bin/main_heiretsu2 "$1" 1 > "data/output/${1}_b_new2.txt" 2>&1 &
    wait  # すべてのバックグラウンドプロセスが終わるのを待つ
}

# build_database_white 15
# build_database_black 15
# ./bin/main_heiretsu2 14 0 > data/output/14_w_new2.txt 2>&1 &
# wait

for ((turn=12; turn>=0; turn--)); do
    build_database "$turn"
done
