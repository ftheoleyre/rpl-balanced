#!/bin/bash

source algos_default.sh

#where to put the stats
PREFIX="algos_nbnodes"


#what changes in this simulation
NB_NODES_LIST=(10   20  30  40  50  60  70  80  90  100 110 120 130)
SIM_LENGTH_LIST=(80	113 139 161 180 197 213 227 242 255 267 279 290)
#sqrt (n*648)


j=0
while [ $j -lt ${#NB_NODES_LIST[*]} ]
do    
    for SF_ALGO in $SF_ALGO_LIST
    do
		JOB_FILENAME=`mktemp $JOBS_DEST/job.XXXXXX`
		echo "TIME=$TIME" > $JOB_FILENAME

        echo "CMD=$SIM_SCRIPT ${NB_NODES_LIST[j]} ${SIM_LENGTH_LIST[j]} $BOP_ALGO $BOP_SLOTS $SF_ALGO $NB_PARENTS $DEPTH_METRIC $BO $SO $UNICAST_PKTIME $MULTICAST_PKTIME $MULTICAST_DEST $MULTICAST_ALGO $PREFIX" >> $JOB_FILENAME

	done
	j=`expr $j + 1`
done
	
