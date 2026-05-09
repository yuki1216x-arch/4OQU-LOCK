#!/bin/bash

for n in {12..15}; do
    ./buildReachabilityTable "$n" 0 db db > "data/output/reachability_${n}_w.txt" 2>&1
done

for n in {0..15}; do
    ./buildReachabilityTable "$n" 1 db db > "data/output/reachability_${n}_b.txt" 2>&1
done
