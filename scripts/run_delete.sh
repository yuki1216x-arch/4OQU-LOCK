#!/usr/bin/env bash

# build_database_white 15
# build_database_black 15

for n in {0..13}; do
    rm "data/output/${n}_w_new2.txt" "data/output/${n}_b_new2.txt"
    for i in {0..3}; do
	rm "data/output/${n}_4_${i}_w.txt" "data/output/${n}_4_${i}_b.txt"
    done
done
