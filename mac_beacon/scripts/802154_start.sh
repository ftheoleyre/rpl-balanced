#!/bin/bash

#directories
if [ -d "/scratch" ]
then
  TMPDIR="/scratch/job.$PBS_JOBID/"
else
  TMPDIR="/tmp/theoleyre"
fi
mkdir -p $TMPDIR

#default values
SIM_LENGTH=0
DEST_MULTI=0

FILE_PREFIX="802154_$RANDOM$RANDOM$RANDOM$RANDOM$RANDOM"
echo "prefix="$FILE_PREFIX

#PARTICULAR CASE: only one argument -> consider it is a filename and extracts the stats from it!
if [ $# -ne 2 ]
then 
	#Temporary file
	FILE_OUT="$TMPDIR/`echo $FILE_PREFIX`_out.tmp"
	TIME_OUT="$TMPDIR/`echo $FILE_PREFIX`_time.tmp"
	CONFIG_WSNET="$TMPDIR/`echo $FILE_PREFIX`.xml"
		
	#verify we have the correct nb of arguments
	if [ $# -ne 14 ]
	then
		echo ""
		echo "$# arguments, required 13"
		echo "usage $0 nb_nodes simulation_length bop_algo bop_slots sf_algo nb_parents depth-metric bo so inter_pk_time_unicast(ms) inter_pk_time_multicast(ms) multicast_dest multicast_algo directory_stats "
		exit
	fi
	
	#PARAMETERS RENAMING
	NB_NODES=$1
	SIM_LENGTH=$2
    BOP_ALGO=$3
    BOP_SLOTS=$4
    SF_ALGO=$5
    NB_PARENTS=$6
    DEPTH_METRIC=$7
	BO=$8
	SO=$9
	UNICAST_PKTIME=${10}
    MULTICAST_PKTIME=${11}
    MULTICAST_DEST=${12}
    MULTICAST_ALGO=${13}
    DIR_STATS=${14}

 
	#Creation of the XML configuration file for wsnet
	echo "wsnet_generate_xml.sh $NB_NODES $SIM_LENGTH $BOP_ALGO $BOP_SLOTS $SF_ALGO $NB_PARENTS $DEPTH_METRIC $BO $SO $UNICAST_PKTIME $MULTICAST_PKTIME $MULTICAST_DEST $MULTICAST_ALGO > $CONFIG_WSNET"
	wsnet_generate_xml.sh $NB_NODES $SIM_LENGTH $BOP_ALGO $BOP_SLOTS $SF_ALGO $NB_PARENTS $DEPTH_METRIC $BO $SO $UNICAST_PKTIME $MULTICAST_PKTIME $MULTICAST_DEST $MULTICAST_ALGO > $CONFIG_WSNET
		
		
		
	#the simulation command
	CMD="wsnet -c $CONFIG_WSNET"
	echo $CMD >> $FILE_OUT
	echo "jobid $PBS_JOBID" >> $FILE_OUT
		
	#redirect the 4th flow into: a/stdout b/the file of results
    exec 4> >(while read a; do echo $a; echo $a >>$FILE_OUT; done)

	#executes the simulation and extracts the execution time in seconds
	rm -rf $FILE_OUT
	echo "$CMD > $FILE_OUT"
	(time -p $CMD > $FILE_OUT) 2> $TIME_OUT
	TIME=`grep real $TIME_OUT | cut -d " " -f 2`
	echo "$TIME seconds"
else
	FILE_OUT=$1
	DIR_STATS=$2
fi

#WTF - here we export to particular folder named according to varied param

#DIR for results
HOME_RESULTS="$HOME/results/$DIR_STATS"
mkdir -p $HOME_RESULTS

#WTF - add parsing of your own stats - energy before, energy after...

#parameters
SEED=`cat $FILE_OUT |  grep "SEED"  |cut -d ":" -f 2`
NB_NODES=`cat $FILE_OUT |  grep "Number of nodes"  |cut -d ":" -f 2`
DEGREE=`cat $FILE_OUT |  grep "Degree" | cut -d ":" -f 2` 
SO=`cat $FILE_OUT |  grep "SO value" | cut -d ":" -f 2` 
BO=`cat $FILE_OUT |  grep "BO value" | cut -d ":" -f 2` 
CW=`cat $FILE_OUT |  grep "CW value" | cut -d ":" -f 2` 
BOPSLOTS=`cat $FILE_OUT |  grep "BOP number of slots" | cut -d ":" -f 2` 
NB_PARENTS=`cat $FILE_OUT |  grep "Max number of parents" | cut -d ":" -f 2`
DURATION=`cat $FILE_OUT |  grep "Duration (s)" | cut -d ":" -f 2`
SF_ALGO=`cat $FILE_OUT | grep "Superframe scheduling algo (int)" | cut -d ":" -f 2` 
DEPTH_METRIC=`cat $FILE_OUT | grep "Depth metric (int)" | cut -d ":" -f 2` 
UNICAST_PKTIME=`cat $FILE_OUT |  grep "Unicast - Inter Packet Time (s)" | cut -d ":" -f 2`
MULTICAST_PKTIME=`cat $FILE_OUT |  grep "Multicast - Inter Packet Time (s)" | cut -d ":" -f 2`

#MAC
MAC_PDR=`cat $FILE_OUT | grep "BIDIR_UN - PDR" | cut -d ":" -f 2` 
MAC_DELAY=`cat $FILE_OUT | grep "BIDIR_UN - Avg end-to-end delay (s)" | cut -d ":" -f 2` 
MAC_JAIN=`cat $FILE_OUT | grep "BIDIR_UN - Jain Index" | cut -d ":" -f 2` 
MAC_ROUTE_LENGTH=`cat $FILE_OUT | grep "BIDIR_UN - Avg route length" | cut -d ":" -f 2` 
MAC_PDR_DOWN=`cat $FILE_OUT | grep "DOWN_UN - PDR" | cut -d ":" -f 2` 
MAC_DELAY_DOWN=`cat $FILE_OUT | grep "DOWN_UN - Avg end-to-end delay (s)" | cut -d ":" -f 2` 
MAC_PDR_UP=`cat $FILE_OUT | grep "UP_UN - PDR" | cut -d ":" -f 2` 
MAC_DELAY_UP=`cat $FILE_OUT | grep "UP_UN - Avg end-to-end delay (s)" | cut -d ":" -f 2` 


#DROPS
DROP_CCA=`cat $FILE_OUT | grep "UN - Too many CCA (%)" | cut -d ":" -f 2` 
DROP_RETX=`cat $FILE_OUT | grep "UN - Too many retx (%)" | cut -d ":" -f 2` 
DROP_ASSOC=`cat $FILE_OUT | grep "UN - Not yet associated (%)" | cut -d ":" -f 2` 
DROP_BUFFER=`cat $FILE_OUT | grep "UN - Too long time in the buffer (%)" | cut -d ":" -f 2` 
DROP_OTHER=`cat $FILE_OUT | grep "UN - other (%)" | cut -d ":" -f 2` 

#TOPOLOGY
ASSOC_TIME=`cat $FILE_OUT | grep "Association time (s)" | cut -d ":" -f 2` 
ASSOC_RATIO=`cat $FILE_OUT | grep "Associated Ratio" | cut -d ":" -f 2` 
ASSOC_AVG_COORD=`cat $FILE_OUT | grep "Ratio of Coord with children " | cut -d ":" -f 2` 
ASSOC_AVG_CHILDREN=`cat $FILE_OUT | grep "Avg nb children per coord" | cut -d ":" -f 2` 
ASSOC_NB_PARENTS=`cat $FILE_OUT | grep "Avg nb of parents" | cut -d ":" -f 2` 
ASSOC_ROUTE_LENGTH=`cat $FILE_OUT | grep "Average Route length (hops)" | cut -d ":" -f 2` 
ASSOC_DEPTH=`cat $FILE_OUT | grep "Tree / DAG Depth" | cut -d ":" -f 2` 


#Collisions
COLL_BOP=`cat $FILE_OUT | grep "Beacon Collision Ratio (wihout children)" | cut -d ":" -f 2` 
COLL_BOP_CHILDREN=`cat $FILE_OUT | grep "Beacon Collision Ratio (with children)" | cut -d ":" -f 2` 
COLL_SF=`cat $FILE_OUT | grep "Superframe Collision Ratio (wihout children)" | cut -d ":" -f 2` 
COLL_SF_CHILDREN=`cat $FILE_OUT | grep "Superframe Collision Ratio (with children)" | cut -d ":" -f 2` 

#interferences
RATIO_HIDDEN_TERMINALS=`cat $FILE_OUT | grep "Ratio with at least one hidden terminal" | cut -d ":" -f 2` 

#Overhead
ACK_TX=`cat $FILE_OUT | grep "ACK - Nb transmitted packets" | cut -d ":" -f 2` 
ACK_RX=`cat $FILE_OUT | grep "ACK - Nb received packets" | cut -d ":" -f 2` 
DATA_TX=`cat $FILE_OUT | grep "DATA - Nb transmitted packets" | cut -d ":" -f 2` 
DATA_RX=`cat $FILE_OUT | grep "DATA - Nb received packets" | cut -d ":" -f 2` 
CMD_TX=`cat $FILE_OUT | grep "CMD - Nb transmitted packets" | cut -d ":" -f 2` 
CMD_RX=`cat $FILE_OUT | grep "CMD - Nb received packets" | cut -d ":" -f 2` 
HELLO_TX=`cat $FILE_OUT | grep "HELLO - Nb transmitted packets" | cut -d ":" -f 2` 
HELLO_RX=`cat $FILE_OUT | grep "HELLO - Nb received packets" | cut -d ":" -f 2` 
ALL_TX=`cat $FILE_OUT | grep "SUM - Nb transmitted packets" | cut -d ":" -f 2` 
ALL_RX=`cat $FILE_OUT | grep "SUM - Nb received packets" | cut -d ":" -f 2` 

#Energy
EN_OFF_J=`cat $FILE_OUT | grep "Mode OFF (Joule)" | cut -d ":" -f 2`
EN_RX_J=`cat $FILE_OUT | grep "Mode  RX (Joule)" | cut -d ":" -f 2`
EN_TX_J=`cat $FILE_OUT | grep "Mode  TX (Joule)" | cut -d ":" -f 2`
EN_OFF_RATIO=`cat $FILE_OUT | grep "Mode OFF (ratio of time)" | cut -d ":" -f 2`
EN_RX_RATIO=`cat $FILE_OUT | grep "Mode  RX (ratio of time)" | cut -d ":" -f 2`
EN_TX_RATIO=`cat $FILE_OUT | grep "Mode  TX (ratio of time)" | cut -d ":" -f 2`

#Multicast
EN_OFF_J=`cat $FILE_OUT | grep "Mode OFF (Joule)" | cut -d ":" -f 2`
EN_RX_J=`cat $FILE_OUT | grep "Mode  RX (Joule)" | cut -d ":" -f 2`
EN_TX_J=`cat $FILE_OUT | grep "Mode  TX (Joule)" | cut -d ":" -f 2`
EN_OFF_RATIO=`cat $FILE_OUT | grep "Mode OFF (ratio of time)" | cut -d ":" -f 2`
EN_RX_RATIO=`cat $FILE_OUT | grep "Mode  RX (ratio of time)" | cut -d ":" -f 2`
EN_TX_RATIO=`cat $FILE_OUT | grep "Mode  TX (ratio of time)" | cut -d ":" -f 2`

#Multicast
MULTI_NBTX=`cat $FILE_OUT | grep "MULTI - Nb transmitted packets" | cut -d ":" -f 2`
MULTI_PDR_RADIO=`cat $FILE_OUT | grep "MULTI - Packet Delivery Ratio (radio neigh)" | cut -d ":" -f 2`
MULTI_PDR_CTREE=`cat $FILE_OUT | grep "MULTI - Packet Delivery Ratio (ctree neigh)" | cut -d ":" -f 2`
MULTI_PDR_FLOODING=`cat $FILE_OUT | grep "MULTI - Packet Delivery Ratio (flooding)" | cut -d ":" -f 2`
MULTI_DELAY=`cat $FILE_OUT | grep "MULTI - Average delay" | cut -d ":" -f 2`

#Connectivity in the graph
CONNEC_LINK=`cat $FILE_OUT | grep "Nb edge removal before disconnection" | cut -d ":" -f 2`
CONNEC_NODE=`cat $FILE_OUT | grep "Nb node removal before disconnection" | cut -d ":" -f 2`



# stat file (discard bad networks with a null reliability)
if [ $SEED ] 
then
	echo "$NB_NODES	$SIM_LENGTH	$DEGREE	$SF_ALGO	$MULTICAST_ALGO $CW	$SO	$BO	$BOPSLOTS	$UNICAST_PKTIME $MULTICAST_PKTIME $NB_PARENTS $ACK_RX	$ACK_TX $DATA_RX	$DATA_TX        $CMD_RX	$CMD_TX    $HELLO_RX	$HELLO_TX    	$ALL_RX	$ALL_TX	$EN_OFF_J   $EN_RX_J $EN_TX_J $EN_OFF_RATIO    $EN_RX_RATIO $EN_TX_RATIO    $MULTI_PDR_RADIO    $MULTI_PDR_CTREE    $MULTI_PDR_FLOODING   $MULTI_DELAY  $COLL_BOP	$COLL_BOP_CHILDREN $COLL_SF	$COLL_SF_CHILDREN	$RATIO_HIDDEN_TERMINALS	$MAC_PDR	$MAC_DELAY	$MAC_JAIN	$MAC_ROUTE_LENGTH	$MAC_PDR_DOWN	$MAC_DELAY_DOWN	$MAC_PDR_UP	$MAC_DELAY_UP	$ASSOC_TIME	$ASSOC_RATIO	$ASSOC_AVG_COORD	$ASSOC_AVG_CHILDREN	$ASSOC_NB_PARENTS	$ASSOC_ROUTE_LENGTH	$ASSOC_DEPTH	$DEPTH_METRIC	$CONNEC_NODE	$CONNEC_LINK	$DURATION   $SEED" >> $HOME_RESULTS/sfalgo=`echo $SF_ALGO`-multialgo=`echo $MULTICAST_ALGO`-bopalgo=`echo $BOP_ALGO`-depth-metric=`echo $DEPTH_METRIC`.txt
	
	echo "$NB_NODES	$SIM_LENGTH	$DEGREE	$SF_ALGO	$MULTICAST_ALGO $CW	$SO	$BO	$BOPSLOTS	$UNICAST_PKTIME $MULTICAST_PKTIME $NB_PARENTS $ACK_RX	$ACK_TX $DATA_RX	$DATA_TX        $CMD_RX	$CMD_TX    $HELLO_RX	$HELLO_TX    	$ALL_RX	$ALL_TX	$EN_OFF_J   $EN_RX_J $EN_TX_J $EN_OFF_RATIO    $EN_RX_RATIO $EN_TX_RATIO    $MULTI_PDR_RADIO    $MULTI_PDR_CTREE    $MULTI_PDR_FLOODING   $MULTI_DELAY  $COLL_BOP	$COLL_BOP_CHILDREN $COLL_SF	$COLL_SF_CHILDREN	$RATIO_HIDDEN_TERMINALS	$MAC_PDR	$MAC_DELAY	$MAC_JAIN	$MAC_ROUTE_LENGTH	$MAC_PDR_DOWN	$MAC_DELAY_DOWN	$MAC_PDR_UP	$MAC_DELAY_UP	$ASSOC_TIME	$ASSOC_RATIO	$ASSOC_AVG_COORD	$ASSOC_AVG_CHILDREN	$ASSOC_NB_PARENTS	$ASSOC_ROUTE_LENGTH	$ASSOC_DEPTH	$DEPTH_METRIC	$CONNEC_NODE	$CONNEC_LINK	$DURATION   $SEED"

	echo "saved in file $HOME_RESULTS/sfalgo=`echo $SF_ALGO`-multialgo=`echo $MULTICAST_ALGO`-bopalgo=`echo $BOP_ALGO`-depth-metric=`echo $DEPTH_METRIC`.txt"
fi


#common case: we have more than 1 argument (i.e. this is a full simulation + stats extraction)
if [ $# -ne 2 ]
then 

	#save temporary files
	if [ $SEED ]
	then
		echo "seed:|"$SEED"|"
		REP_RES="$HOME_RESULTS/`echo $SEED | sed 's/ //g'`"
		echo $REP_RES
		mkdir -p $REP_RES

		#the results
		echo "mv $FILE_OUT $REP_RES"
		mv $FILE_OUT $REP_RES
	
		#time required 
		echo "mv $TIME_OUT $REP_RES"
        mv $TIME_OUT $REP_RES
	
		#xml configuration file
		echo "mv $CONFIG_WSNET $REP_RES"
		mv $CONFIG_WSNET $REP_RES
	
	#we just delete temporary files
	else
		echo "seed:|"$SEED"|"
		echo "simulation failed: let move temporary files in the trash directory"
		cat $FILE_OUT
		
        REP_RES="$HOME_RESULTS/trash/`echo $SEED | sed 's/ //g'`"
        echo $REP_RES
        mkdir -p $REP_RES

		#the results
		echo "mv $FILE_OUT $REP_RES"
		mv $FILE_OUT $REP_RES
	
		#time required 
		echo "mv $TIME_OUT $REP_RES"
		mv $TIME_OUT $REP_RES
	
		#xml configuration file
		echo "mv $CONFIG_WSNET $REP_RES"
		mv $CONFIG_WSNET $REP_RES
	
	fi
fi


#Parameters list: 
#NB_NODES
#SIM_LENGTH
#DEGREE
#SF_ALGO
#MULTICAST_ALGO
#CW
#SO	
#BO
#BOPSLOTS
#10#PKTIME_UNI
#PKTIME_MULTI
#NB_PARENTS
#ACK_RX
#ACK_TX
#15#DATA_RX
#DATA_TX
#CMD_RX
#CMD_TX
#HELLO_RX
#20#HELLO_TX
#TOTAL_RX
#TOTAL_TX
#EN_OFF_J
#EN_RX_J
#25#EN_TX_J
#EN_OFF_RATIO
#EN_RX_RATIO
#EN_TX_RATIO
#MULTI_PDR_RADIO
#30#MULTI_PDR_CTREE
#MULTI_PDR_FLOODING
#MULTI_DELAY
#COLL_BOP
#COLL_BOP_CHILDREN
#35#COLL_SF
#COLL_SF_CHILDREN
#RATIO_HIDDEN_TERMINALS
#MAC_PDR
#MAC_DELAY 
#40#MAC_JAIN
#MAC_ROUTE_LENGTH
#MAC_PDR_DOWN
#MAC_DELAY_DOWN
#MAC_PDR_UP
#45#MAC_DELAY_UP
#ASSOC_TIME
#ASSOC_RATIO
#ASSOC_AVG_COORD
#ASSOC_AVG_CHILDREN
#50#ASSOC_NB_PARENTS
#ASSOC_ROUTE_LENGTH
#ASSOC_DEPTH
#DEPTH_METRIC
#CONNEC_NODE
#55#CONNEC_LINK
#DURATION
#SEED

	

