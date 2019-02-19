#ifndef __RPL_CONST_H__
#define __RPL_CONST_H__


//------------------------------------
// RPL PARAMETERS defined by RFC
//------------------------------------

//RPL Codes for the message types
#define RPL_CODE_DIS                     0   /* DIS message */
#define RPL_CODE_DIO                     1   /* DIO message */
#define RPL_CODE_DAO                     2   /* DAO message */
#define RPL_CODE_DAO_ACK                 3   /* DAO ACK message */
#define RPL_CODE_DATA					 5   /* DATA message */


// Default values for RPL constants
#define DEFAULT_MIN_HOPRANKINC          128 
#define BASE_RANK                       0 //Rank of a node outside the LLN.
#define ROOT_RANK                       DEFAULT_MIN_HOPRANKINC //Rank of a root node. 
#define INFINITE_RANK                   0xffff
#define RPL_DEFAULT_INSTANCE            0
#define DEFAULT_PATH_CONTROL_SIZE		0


//Default values for Trickle Algorithm
#define DEFAULT_DIO_INTERVAL_MIN        7   //12    //7   //Imin from Trickle = Represents 2^n ms. (DEFAULT in RFC is 3, but in our case is too small)
#define DEFAULT_DIO_INTERVAL_DOUBLINGS  16  //11    //16  //Imax from Trickle = Maximum amount of timer doublings (DEFAULT in RFC is 20, but we computed it in function of DEFAULT_DIO_INTERVAL_MIN).
#define DEFAULT_DIO_REDUNDANCY          10  //k from Trickle = DIO redundancy. (DEFAULT in RFC is 10, draft-gnawali-roll-rpl says to use a constant between 3 and 5)


/* DIO codes */
 #define RPL_DIO_GROUNDED            0x80
 #define RPL_DIO_DEST_ADV_SUPPORTED  0x40
 #define RPL_DIO_DEST_ADV_TRIGGER    0x20
 #define RPL_DIO_MOP_MASK            0x18
 #define RPL_DIO_MOP_NON_STORING     0x00
 #define RPL_DIO_MOP_STORING         0x10
 #define RPL_DIO_DAG_PREFERENCE_MASK 0x07




//------------------------------------
// RPL PARAMETERS suggested values 
//------------------------------------
// DIS related 
#define RPL_DIS_INTERVAL                60


//DAG related
#define DAG_RANK(fixpt_rank, dag)	    ((fixpt_rank) / dag->min_hoprankinc)
#define RPL_MAX_DAG_ENTRIES             2
#define DEFAULT_MAX_RANKINC             3*DEFAULT_MIN_HOPRANKINC
#define RPL_ANY_INSTANCE               -1
#define RPL_DEFAULT_OCP                 0 //elt=5, etx=1, of0=0, residual_eng=6 
#define RPL_GROUNDED                    0




//------------------------------------
// constants for OF0
//------------------------------------

//step_of_rank represents the link properties (1 for excellent... 9 for worst acceptable)
#define DEFAULT_STEP_OF_RANK 1  // DEFAULT_STEP_OF_RANK = 3 in OF0 but I will use 1 in MULTIPATH
#define MINIMUM_STEP_OF_RANK 1
#define MAXIMUM_STEP_OF_RANK 9


//stretch_of_rank is the maximum augumentation to the step_of_rank of a preferred parent to allow the selection of an additional feasible successor
#define DEFAULT_RANK_STRETCH 0
#define MAXIMUM_RANK_STRETCH 5


//rank_factor is used to multiply the effect of the link properties
#define DEFAULT_RANK_FACTOR  1   
#define MINIMUM_RANK_FACTOR  1
#define MAXIMUM_RANK_FACTOR  4




//------------------------------------
// constants for ROUTING METRICS
//------------------------------------

//constants for ETX 
#define LINK_ETX_MIN				1  	//equivalent of MINIMUM_STEP_OF_RANK
#define LINK_ETX_MAX				9  	//for PDR = 0.1 //equivalent of MAXIMUM_STEP_OF_RANK
#define LINK_ETX_GUESS				1.5  	//equivalent of MINIMUM_STEP_OF_RANK
#define PARENT_SWITCH_ETX_THRESHOLD	1.5
#define LAMDA_ETX					0.1	//for computing moving average of the ETX = LAMDA*old_value + (1-LAMDA)*new_value
#define RECOMPUTE_ETX				120000000000		//for ELT and ETX: compute ETX every RECOMPUTE_ETX seconds


//constats for ELT
#define DATA_RATE					250000				//250 kb/s (250000 b/s)
#define START_ENERGY				12672.0				//Joule (= 880 mAh)
#define ETX_4_ELT_GUESS				1					//when i don't know the value of the ETX on the link
#define ETX_4_ELT_MAX				10					//maximum value of the ETX on the link
#define MAX_ELT						FLT_MAX //3168000000.0	// = 12672 * 250000 		//max lifetime if I have battery full (880 mAh in Joule) and I transmit at a rate of 250 kb/s
#define PARENT_SWITCH_ELT_THRESHOLD	600				//threshold for changing the preferred parent: 10 minutes
#define MY_RADIO_TX					(31.0*1e-3*4)  		//31 mA, 10dBm

#define LAMDA_LOAD					0.5  		//how much the new loads influences the old ones (to not pass from 50% - 50% directly at 100% - 0%)
												//new_load = LAMDA_LOAD*p->parent_old_load + (1-LAMDA_LOAD)*p->parent_load;
#define LAMDA_TRAFFIC				0.1					//for computing moving average of the traffic = LAMDA*old_value + (1-LAMDA)*new_value
#define RECOMPUTE_TRAFFIC			300000000000		//for ELT: compute traffic every RECOMPUTE_TRAFFIC seconds
#define PACKET_SIZE					1016				//127*8 bits

//constants for normalizing ELT components of the DIO
#define MAX_TRAFFIC					1020			//apx 1 kb/s (255*4)
#define MAX_BOTTLEN_CONST			792000000000 	// 12672 * 250000 / (10*0.004) we consider MAX_ETX = 10


//for MULTI elt
#define CHOOSE_PREFERRED_PARENT		30000000000		//time at which a node should choose its preferred parent, after it received the first DIO


//constants for Residual Energy
#define PARENT_SWITCH_RESIDUAL_THRESHOLD	0
#define RECOMPUTE_RESIDUAL					10000000000	//for residual energy metric: recompute residual energy every RECOMPUTE_RESIDUAL seconds



//------------------------------------
// constants for IOCTL
//------------------------------------

#define MAC_IOCTL_RETRANSMIT 		30  	//RPL gets a pointer to the MAC statistics about packet drops & transmitted (used to compute ETX) 
#define ENERGY_CONSUMED 			40  	//RPL gets a pointer to the MAC updates of the energy consumed
#define REMOVE_PARENT				60  	//MAC announces RPL the parent is not reachable anymore




//------------------------------------
// constants for SIMULATIONS
//------------------------------------

#define START_TIME	1800000000000  //stats should be computed only START_TIME seconds after the beginning of the simulation
#define SINK  		0			   //node 0 is the SINK in all simulations

#endif /* CONST_H */
