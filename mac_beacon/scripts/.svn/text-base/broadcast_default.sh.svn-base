#!/bin/bash

#where to put the jobs
JOBS_DEST="jobs"
if [ ! -d $JOBS_DEST ]
then
mkdir -p $JOBS_DEST
fi


#default parameters
#802.15.4
BO=8					#Beacon Order
SO=1					#Superframe Order

#environment
NB_NODES=50
SIM_LENGTH=270

#DAG & scheduling
NB_PARENTS=4			#max nb parents in the DAG (0 means no limit)
SF_ALGO=3
BOP_SLOTS=4
BOP_ALGO=2

#traffic
MULTICAST_PKTIME=20000           #in ms
UNICAST_PKTIME=0
MULTICAST_DEST=ctree            #ctree / disc / flood-disc / flood-ctree

#for a list of inter packet times
MULTICAST_PKTIME_LIST="500 1000 2000 3000 5000 10000 14000 18000 22000 26000 30000 35000 40000 45000 50000"		#in ms
FLOOD_PKTIME_LIST="20000 30000 40000 50000 60000 70000 80000 90000 100000 110000 150000 200000 250000 300000 350000" #in ms

#The algos we use here
MULTICAST_ALGO_LIST="1 2 3"	

#Names for files and directories
SIM_SCRIPT="802154_start.sh"

#JOB TIME ESTIMATION (in seconds)
TIME=2000
