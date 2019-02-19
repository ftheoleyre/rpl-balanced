#!/bin/bash

#where to put the jobs
JOBS_DEST="jobs"
if [ ! -d $JOBS_DEST ]
then
    mkdir -p $JOBS_DEST
fi


#default parameters
#802.15.4
BO=7					#Beacon Order
SO=2					#Superframe Order

#environment
NB_NODES=50
SIM_LENGTH=180

#DAG & scheduling
NB_PARENTS=4			#max nb parents in the DAG (0 means no limit)
SF_ALGO=3				#1=802154, rand=2, greedy=3, god=4
SF_ALGO_LIST="1 2 3"
DEPTH_METRIC="3"
BOP_SLOTS=4
BOP_ALGO=1				#1=classical TDMA scheme

#traffic
UNICAST_PKTIME=120000   #in ms

#not used here
MULTICAST_PKTIME=0
MULTICAST_DEST="ctree"
MULTICAST_ALGO=1

#Names for files and directories
SIM_SCRIPT="802154_start.sh"

#JOB TIME ESTIMATION (in seconds)
TIME=500
