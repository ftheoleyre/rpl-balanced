#!/bin/bash

#the parameters to execute the jobs directly without PBS

for file in jobs/*
do
	#cmd of this job
	CMD=`cat $file | grep CMD | cut -d '=' -f 2` 

	echo "executing $CMD"
	$CMD 
	
done