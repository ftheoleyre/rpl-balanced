#!/bin/bash

for script in algos_*
do
  printf "\n $script: "
  for i in $(seq 1 10);
  do
    printf "%d " $i
    `echo "./$script"`  
  done
done

echo ""
echo "finished!"
