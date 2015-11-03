#!/bin/bash

for (( i=0; i<8; ++i ))
do
  dd if=/dev/urandom of=blob-2k-t$i bs=2018 count=1
done

for (( i=0; i<8; ++i ))
do
  dd if=/dev/urandom of=blob-4k-t$i bs=4096 count=1
done

for (( i=0; i<8; ++i ))
do
  dd if=/dev/urandom of=blob-16k-t$i bs=4096 count=4
done

for (( i=0; i<8; ++i ))
do
  dd if=/dev/urandom of=blob-256k-t$i bs=4096 count=64
done

for (( i=0; i<8; ++i ))
do
  dd if=/dev/urandom of=blob-4m-t$i bs=4096 count=1024
done

for (( i=0; i<8; ++i ))
do
  dd if=/dev/urandom of=blob-16m-t$i bs=4096 count=4096
done

for (( i=0; i<8; ++i ))
do
  dd if=/dev/urandom of=blob-64m-t$i bs=2018 count=16384
done
