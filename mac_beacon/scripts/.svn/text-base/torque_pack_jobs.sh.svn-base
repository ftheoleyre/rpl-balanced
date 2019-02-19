#!/bin/bash

#constants
# for i in `seq 0 1`; do qdel $i; done

#where I will put the jobs
DEST="jobs_torque"

#how much time I am allowed to reserve at most (in seconds), here 8 hour
MAX_TIME_RESERVED=36000
MAX_NB_HOURS_TORQUE=10
#memory in MB
MEMORY=1000

#Parameters
JOB_NAME="theo802154wsnet"


#the parameters to execute the jobs
#cplex cannot handle efficiently more than 2 cores with this problem
NB_CORES=1
NB_PARALLEL_JOBS=1
QUEUE="public"


mkdir $DEST

# create a torque script that will call iteratively for all these jobs
function flush_jobs
{
	# cumulative time converted in h:min:s
	CUMUL_TIME=`echo $CUMUL_TIME / 1 | bc`
    h=`expr $CUMUL_TIME / 3600` 
    min=`expr \( $CUMUL_TIME - $h \* 3600 \) / 60`

    if [ $h -ge $MAX_NB_HOURS_TORQUE ]
    then
      echo "$h h -> upper bounded to $MAX_NB_HOURS_TORQUE h"
      h=`expr $MAX_NB_HOURS_TORQUE - 1`
    fi

    echo "--- $jobid JOBs $CUMUL_TIME s ---"

    echo "#!/bin/bash" >> `echo $DEST/$SCRIPT_DIR.pbs`
    echo "#PBS -M theoleyre@unistra.fr" >> `echo $DEST/$SCRIPT_DIR.pbs`
    echo "#PBS -m bae" >> `echo $DEST/$SCRIPT_DIR.pbs`
    echo "#PBS -l walltime=$h:$min:00,pvmem=$MEMORY`echo mb`,mem=$MEMORY`echo mb`,nodes=1:ppn=$NB_CORES" >> `echo $DEST/$SCRIPT_DIR.pbs`
    echo "#PBS -q $QUEUE"  >> `echo $DEST/$SCRIPT_DIR.pbs`
    echo "#PBS -N $JOB_NAME"  >> `echo $DEST/$SCRIPT_DIR.pbs`
    
    #for the nb of cores: executed when the job starts on the grid node (i.e. the nb of cores on this particular node)
    #CPLEX should use as many threads as the number of cores
    #echo "NB_CORES=\`egrep -c '^processor' /proc/cpuinfo\`" >> `echo $DEST/$SCRIPT_DIR.pbs`
    echo "NB_PARALLEL_JOBS=$NB_PARALLEL_JOBS" >> `echo $DEST/$SCRIPT_DIR.pbs`
   

    #replace the template with the correct call (use absolute path to avoid problems)
	  echo "LOCATION_JOBS=`pwd`/$DEST/$SCRIPT_DIR" >> `echo "$DEST/$SCRIPT_DIR.pbs"`
	  cat torque_skeleton.pbs >> `echo "$DEST/$SCRIPT_DIR.pbs"`

    echo "-----------"

}

#to initializes all the variables, etc.
function init_torque_script
{
	jobid=0
	CUMUL_TIME=0

	SCRIPT_DIR=`mktemp -d $DEST/XXXXXX |cut -d '/' -f 2`
	echo "creates $DEST/$SCRIPT_DIR"
	mkdir -p `echo "$DEST/$SCRIPT_DIR"`
}




# initialization
init_torque_script

for file in jobs/*
do
	#add this job
	CMD=`cat $file | grep CMD | cut -d '=' -f 2` 
	
	# extracts the time for this job, keep only the int part, and increment the cumulative time
	TIME_PLUS=`cat $file | grep TIME | cut -d '=' -f 2`
	TIME_PLUS=`echo "tmp=$TIME_PLUS; tmp /= 1; tmp" | bc` 
	CUMUL_TIME=`echo "$CUMUL_TIME + $TIME_PLUS" | bc`

	#gives in argument the number of cores for the execution (uses all the cores of one node)
	#echo "$CMD \`egrep -c '^processor' /proc/cpuinfo\`" >> `echo "$DEST/$SCRIPT_DIR/$jobid.sh"`

	#uses 2 threads/cores for cplex (will be put in the auto torque file)
	echo "$CMD" >> `echo "$DEST/$SCRIPT_DIR/$jobid.sh"`
	chmod u+x `echo $DEST/$SCRIPT_DIR/$jobid.sh`
	jobid=`expr $jobid + 1`
  
	#create a new big torque job (of 2 hours each)
	#echo "$CUMUL_TIME >= $MAX_TIME_RESERVED"
	
	if [ $CUMUL_TIME -ge $MAX_TIME_RESERVED ]
	then
		flush_jobs

		# re-initialization
		init_torque_script
	fi
done

#we have at least one job to flush
if [ $jobid -ne 0 ]
then
  flush_jobs
else
  rm -rf "$DEST/$SCRIPT_DIR"
  echo "removes the empty job $DEST/$SCRIPT_DIR"
  echo "-----------"
fi
