#!/bin/bash


for item in "$@"
do
  grep " $item]" _log.txt > _$item.txt
done






