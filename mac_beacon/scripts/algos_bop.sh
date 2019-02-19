#!/bin/bash

source algos_default.sh

#where to put the stats
PREFIX="algos_bop"


#what changes in this simulation
BOP_SLOTS_LIST="1 2 3 4 5 6 7"


for SF_ALGO in $SF_ALGO_LIST
do
	for BOP_SLOTS in $BOP_SLOTS_LIST
	do
            JOB_FILENAME=`mktemp $JOBS_DEST/job.XXXXXX`
            echo "TIME=$TIME" > $JOB_FILENAME

            echo "CMD=$SIM_SCRIPT $NB_NODES $SIM_LENGTH $BOP_ALGO $BOP_SLOTS $SF_ALGO $NB_PARENTS $DEPTH_METRIC $BO $SO $UNICAST_PKTIME $MULTICAST_PKTIME $MULTICAST_DEST $MULTICAST_ALGO $PREFIX" >> $JOB_FILENAME
	done
done
