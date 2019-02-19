#!/bin/bash

source algos_default.sh


#where to put the stats
PREFIX="algos_load"

#what changes in this simulation
UNICAST_PKTIME_LIST="40 60 80 100 120 140 160 180 200 220 240"
 

for SF_ALGO in $SF_ALGO_LIST
do
	for UNICAST_PKTIME in $UNICAST_PKTIME_LIST
	do
            JOB_FILENAME=`mktemp $JOBS_DEST/job.XXXXXX`
            echo "TIME=$TIME" > $JOB_FILENAME

            echo "CMD=$SIM_SCRIPT $NB_NODES $SIM_LENGTH $BOP_ALGO $BOP_SLOTS $SF_ALGO $NB_PARENTS $DEPTH_METRIC $BO $SO $UNICAST_PKTIME $MULTICAST_PKTIME $MULTICAST_DEST $MULTICAST_ALGO $PREFIX" >> $JOB_FILENAME
	done
done
	
