#ifndef __RPL_TYPES_H__
#define	__RPL_TYPES_H__

#include "rpl-const.h"




//------------------------------------------------------------------------
//			 same STRUCT as in MAC to be transmitted through IOCTL
//------------------------------------------------------------------------

//Energy from MAC
typedef struct{
    uint64_t    duration;
    double      consumption;
}my_energy_t;


//Retransmissions form MAC for ETX
typedef struct{
	int neighbor;
	int packs_net; 		//nr of packets received by MAC from network layer (RPL)
	int packs_tx;		//nr of packets transmitted (retransmissions included)
	int packs_tx_once; 	//nr packs transmitted on the radio having nr_retries = 0
	int acks;			//nr of acknowledgments 
	int drops;			//nr of packet drops
}etx_elt_t;




//-----------------------------------------
//			FOR RPL
//-----------------------------------------
typedef uint16_t rpl_rank_t;
typedef uint16_t rpl_ocp_t;


//info about the bottleneck 
typedef struct{
	double bottlen_const; // Eres/(Etx * ETX)
  	float bottlen_traffic;
  	float ratio_of_traffic;  //the ratio of my traffic that will actually get to the bottleneck
  							 //has to be modified at each node!!!	
  	int bottlen_id;
}bottleneck_t;


//info about the bottleneck to put in the DIO
typedef struct{
	uint16_t bottlen_const; // Eres/(Etx * ETX) *DATA_RATE
  	uint8_t bottlen_traffic;
  	uint8_t ratio_of_traffic;  //the ratio of my traffic that will actually get to the bottleneck
  							 //has to be modified at each node!!!	
  	int bottlen_id;
}dio_bottleneck_t;


//pair of bottleneck_id and parent_id to avoid the infinite loops
typedef struct bottlen_parent{
	nodeid_t 	parent_id;	
	int 		bottlen_id;
	double bottlen_const; // Eres/(Etx * ETX)
  	float bottlen_traffic;
  	float ratio_of_traffic;  //the ratio of my traffic that will actually get to the bottleneck
  							 //has to be modified at each node!!!	
}bottlen_parent_t;


//pair of bottleneck id and old traffic
typedef struct bottlen_old{
	int bottlen_id;
	float old_traffic;
}bottlen_old_t;

//info about the parent
typedef struct rpl_parent {
	struct rpl_parent *next;
	void *dag;

	nodeid_t 	id;		//ID of the parent

	rpl_rank_t	rank;	//Rank of the parent
	uint8_t 	instance_id;
	uint8_t 	local_confidence;
	uint8_t 	ignore;
	//int 		last_dio; //sequence number of the last DIO received used to calculate ETX (not anymore)
	
	
	//for ELT
	double etx_4_elt; // ETX param from the ELT formula; initialized with 1;
	etx_elt_t *old_etx; //needed for moving average of the ETX for elt
	
	
	//info about the bottlenck
	void *bottlenecks;  // a list with all bottlenecks 
	float parent_load;	// the proportion of traffic to send to this parent
	float parent_old_load; // the old proportion of traffic to send to this parent 
	
	//for residual energy metric;
	//double path_energy_residual; //of the path!!!! going through this parent (=min energy_residual on the path)
}rpl_parent_t;




//-----------------------------------------
//		API for the Objective Function
//-----------------------------------------
typedef struct rpl_of {
  void (*reset)(void *);
  rpl_parent_t *(*best_parent)(call_t *, rpl_parent_t *, rpl_parent_t *);
  rpl_rank_t (*calculate_rank)(call_t *, rpl_parent_t *, rpl_rank_t);
  rpl_ocp_t ocp;
}rpl_of_t;




//--------------------------------------------------------------------
//		Control Messages
//---------------------------------------------------------------------

typedef struct rpl_dio_config {
	uint8_t 	type;
	uint8_t		opt_length;
	uint8_t		flags;
	uint8_t		dio_intdoubl;
	uint8_t		dio_intmin;
	uint8_t		dio_redund;
	uint16_t	maxRankIncrease;
	uint16_t	minHopRankIncrease;
	uint16_t	ocp;
	uint8_t		reserved;
	uint8_t		lifetime;
	uint16_t	lifetime_unit;	
}rpl_dio_config_t;


//DAG Information Object (DIO) 
typedef struct rpl_dio {
  	uint64_t 	dag_id;  				//DODAG ID should be on 128bits, bur for me is only an ID, so I add another uint64_t that I will initialize with 0 and I will never use
  	uint64_t 	second_part_dag_id;
	uint8_t 	instance_id;
	uint8_t 	version;
  	rpl_rank_t 	rank;
	uint8_t 	g_mop;					//parameters: G, O, MOP and Prf from the standard
  	uint8_t 	dtsn;   				//not used => 0
  	uint8_t		flags;					// = 0
	uint8_t		reserved;				// = 0  	
	
	//configuration Option
	rpl_dio_config_t dodag_config;
	
	//used to compute ETX, not used anymore
  	//uint8_t nr_sequence;
  
	//info about the bottlenck
	void *bottlenecks;  // a list with all bottlenecks 
						//space wise, is 8 bytes which is (almost) equivalent to the space needed to store values from the DAG Metric Container

  
	//minimum residual energy on the path
	//double path_energy_residual;
}rpl_dio_t;


//DIO Information Solicitation (DIS)
typedef struct rpl_dis {
	uint8_t	flags;
	uint8_t reserved;
}rpl_dis_t;




//--------------------------------------------------------------------
//		DODAG representation 
//---------------------------------------------------------------------

typedef struct rpl_dag {
	//configuration
	uint64_t 	dag_id; 	//DODAG ID -should be on 128bits, bur for me is only an ID, so I add another uint64_t that I will initialize with 0 and I will never use
	uint64_t 	second_part_dag_id;
	uint8_t 	instance_id;
	uint8_t 	version;
	rpl_of_t 	*of;

	//rank related
	rpl_rank_t 	rank;
	rpl_rank_t 	min_rank; //lowest rank that we know; should be reset per DODAG iteration!
	rpl_rank_t 	max_rankinc;
	rpl_rank_t 	min_hoprankinc;
	
	//preferred parent
	rpl_parent_t *preferred_parent;
	//list of parents is found in nodedata

	//Trickle timer parameters
	uint8_t 	dio_intdoubl;       // Imax
	uint8_t 	dio_intmin;         //Imin
	uint32_t 	dio_intcurrent;     //Lcurrent = Parameter I of trickle timer
	uint64_t 	dio_interval;		//Duration of the trickle timer interval; + get_time() = parametrul t (din trickle timer)
	uint8_t 	dio_redundancy;		//Parameter k of trickle timer
	uint8_t 	dio_counter;		//Parameter c of trickle timer
	
	//others
	//uint8_t 	preference;
	uint8_t 	grounded;
	uint8_t 	used;
	uint8_t 	joined;
	//uint8_t 	dtsn; 			//used for DAOs
	//uint8_t 	dio_sequence; 	//dio sequence number, used for computing ETX
	event_t 	*dio_timer;		//variable containing scheduler events for DIO message 	
	
	//for min path residual energy
	//double path_energy_residual;
}rpl_dag_t;


	
	
//-----------------------------------------
//			 Node private data
//-----------------------------------------

typedef struct nodedata {
	int *overhead;
	int type; //0 - Sink; 1 - Node
	
	
	//All the parameters concerning network are stored in this table
	//oana: Each entry represents different instances
	rpl_dag_t dag_table[RPL_MAX_DAG_ENTRIES];
	
	//Reserve dinamicaly memory for neighbors and feasible parents
	void *parents;
	
	
	//for ELT multipath
	void *my_bottlenecks; // a list with all bottlenecks 
	int max_bottlenecks;  //maximum number of bottlenecks allowed to advertise (read from the xml file) 
	int after_bootstrap;  //boolean value: 1 if i selected the preferred parent at bootstrap after CHOOSE_PREFERRED_PARENT seconds, 0 otherwise
	void *b_old_traffic;  //for computing the ELT of a bottleneck after attachment to the DODAG

	//for etx
	//void *link_etx;
	
	//for Expected Lifetime
    energy_t    *radio_energy;   		//for energy - pointing to the energy info updated by the radio module
	void 		*mac_neigh_retransmit;  //retransmissions from MAC - pointing to the retransmission info updated by the MAC module
	double 		start_energy;			// total energy of the battery at the starting of the simulation; should be different for each node
	double 		energy_residual;		// initialized to start_energy
	float 		my_traffic;				//nb_pkts per second initialized with the value from the app layer
	int			old_tx;					//to compute moving average of the traffic
	int 		ocp_from_xml;			//code of the OF to use: elt=5, etx=1, of0=0
	float		load_step;				//step of the load balancing; default 0.1
	float		max_increase;			//max increase/decrease from the old weight to the new one; default is 0.2
	
	/// for stats 
	int dio_tx;
	int dio_rx;
    int dis_tx;
    int dis_rx;
    int data_tx;
    int data_rx;	
    int stats_data_tx;
	int reset_trickle;
	int change_parent;
	int stats_change_parent;
	int last_known_rank;
	double distance;
	double energy_spent;
	
	//oana: stats for results
	void *time_reset_trickle;
	void *time_sendto;
	void *time_sent_dio;
	void *parent_changing; //how often I change my parent; parent_stats type;
		
}nodedata_t;




//-----------------------------------------
//			 STATS
//-----------------------------------------

//statistics about the CBR
typedef struct{
	uint64_t	time_generated;
	uint64_t	time_received;
	int			src;
	int			dest;	
	int			sequence;
	uint16_t	*route;
}stat_cbr_info;


//global stats -> list of stat_cbr_info
void *global_cbr_stats;


//compute node distance from the sink
double sink_x;
double sink_y;


//stats when did my parent changed
typedef struct{
	int parent;
	float etx;
	uint64_t time;
	int my_rank;
}parent_stats;


#endif