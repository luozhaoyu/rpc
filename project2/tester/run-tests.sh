#!/bin/bash

EXE=./tester
ROOT=../clientbazil/root
#ROOT=tests/data

for i in 2k 4k 16k 256k 4m 16m 64m
do
 $EXE SingleAccess $ROOT/blob-$i 1
 $EXE SingleAccess $ROOT/blob-$i 20
 $EXE SingleReadWrite $ROOT/blob-$i r 5
 $EXE SingleReadWrite $ROOT/blob-$i w 5
done
