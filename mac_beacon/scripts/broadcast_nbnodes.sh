#!/bin/bash

source broadcast_default.sh

#where to put the stats
PREFIX="broad_nbnodes"


#what changes in this simulation
NB_NODES_LIST=(10    20  30  40  50  60  70  80  90  100 110 120 130 140 150)
SIM_LENGTH_LIST=(120 170 209 241 270 295 319 341 362 381 400 418 435 451 467)
#for d in 10 20 30 40 50 60 70 80 90 100 110 120 130 140 150 ; do echo $d: ; echo "sqrt($d * 1458)" | bc ; done


j=0
while [ $j -lt ${#NB_NODES_LIST[*]} ]
do
    
    for MULTICAST_ALGO in $MULTICAST_ALGO_LIST;
    do
		JOB_FILENAME=`mktemp $JOBS_DEST/job.XXXXXX`
		echo "TIME=$TIME" > $JOB_FILENAME

        echo "CMD=$SIM_SCRIPT ${NB_NODES_LIST[j]} ${SIM_LENGTH_LIST[j]} $BOP_ALGO $BOP_SLOTS $SF_ALGO $NB_PARENTS $BO $SO $UNICAST_PKTIME $MULTICAST_PKTIME $MULTICAST_DEST $MULTICAST_ALGO $PREFIX" >> $JOB_FILENAME

	done
	j=`expr $j + 1`
done
	
