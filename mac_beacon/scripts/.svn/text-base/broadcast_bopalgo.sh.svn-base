#!/bin/bash

source broadcast_default.sh


#where to put the stats
PREFIX="broad_bopalgo"


#what changes in this simulation
BOP_ALGO_LIST="1 2"


for MULTICAST_PKTIME in $MULTICAST_PKTIME_LIST
do
    
    for MULTICAST_ALGO in $MULTICAST_ALGO_LIST;
    do
        for BOP_ALGO in $BOP_ALGO_LIST
        do
            JOB_FILENAME=`mktemp $JOBS_DEST/job.XXXXXX`            
            echo "TIME=$TIME" > $JOB_FILENAME

    
            echo "CMD=$SIM_SCRIPT $NB_NODES $SIM_LENGTH $BOP_ALGO $BOP_SLOTS $SF_ALGO $NB_PARENTS $BO $SO $UNICAST_PKTIME $MULTICAST_PKTIME $MULTICAST_DEST $MULTICAST_ALGO $PREFIX" >> $JOB_FILENAME
            done
	done
done
	
