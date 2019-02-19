/**
 *  \file   param.c
 *  \brief  methods to get parameters values, etc 
 *  \author Fabrice Theoleyre
 *  \date   2011
 **/


#include "param.h"






//----------------------------------------------
//				APPLI 
//----------------------------------------------


//for debug
short DEBUG_= FALSE;
short param_debug(){
	return(DEBUG_);
}
void param_debug_set(short value){
	DEBUG_ = value;
}

//return the inter packet time for unicast packets
uint64_t param_get_inter_pk_time_unicast(call_t *c){
	call_t cbr_proc = {get_entity_bindings_up(c)->elts[0], c->node, c->entity};
	
	struct _cbr_private	*nodedata = get_node_private_data(&cbr_proc);
	return(nodedata->unicast_period);
}

//return the inter packet time for multicast packets
uint64_t param_get_inter_pk_time_multicast(call_t *c){
	call_t cbr_proc = {get_entity_bindings_up(c)->elts[0], c->node, c->entity};
	
	struct _cbr_private	*nodedata = get_node_private_data(&cbr_proc);
 	return(nodedata->multicast_period);
}

//return the destination for mutlicast packets (CBR entity)
uint16_t param_get_dest_multicast(call_t *c){
	call_t cbr_proc = {get_entity_bindings_up(c)->elts[0], c->node, c->entity};
 	struct _cbr_private	*nodedata = get_node_private_data(&cbr_proc);
    return(nodedata->multicast_dest);
}

//----------------------------------------------
//				GRAPH
//----------------------------------------------

/*

//the graph (UDG based)
igraph_t *G_ = NULL;

//private access
void params_set_graph(igraph_t *G){
	G_ = G;
}
igraph_t* params_get_graph(){
	return(G_);
}
*/



//----------------------------------------------
//				PARAMS - HELLOS
//----------------------------------------------

//0 means that nodes do not send hellos
//period is saved as a number of BI
int HELLO_BOPERIOD_ = INVALID_BOPERIOD;

//Nodes have to send perdiodically hellos (returned in time uint64_t)
uint64_t param_get_hello_period(){
	return(HELLO_BOPERIOD_ * tools_get_bi_from_bo(param_get_global_bo()));
}
void param_set_hello_boperiod(int value){
	HELLO_BOPERIOD_ = value;
}


//MUST I follow all my neighbors (i.e. their beacon), even if I don't participate to their superframe? (child, sibling, etc.)
short NEIGH_MONITOR_ALL_ = FALSE;

short param_get_neigh_monitor_all(){
	return(NEIGH_MONITOR_ALL_);
}
void param_set_neigh_monitor_all(short value){
	NEIGH_MONITOR_ALL_ = value;
}

//SCAN period
int SFSLOT_SCAN_BOPERIOD_ = INVALID_BOPERIOD;

//Nodes have to listen perdiodically each superframe slot
uint64_t param_get_sfslot_scan_period(){
    return(SFSLOT_SCAN_BOPERIOD_ * tools_get_bi_from_bo(param_get_global_bo()));
}
void param_set_sfslot_scan_period(int value){
	SFSLOT_SCAN_BOPERIOD_ = value;
}





//----------------------------------------------
//				PARAMS - BO / SO
//----------------------------------------------

//global variables (they have the same value, changed only once during the initalization)
int GLOBAL_BO_, GLOBAL_SO_;


//BO & SO
void param_set_global_bo(int value){
	GLOBAL_BO_ = value;
}
int param_get_global_bo(){
	return(GLOBAL_BO_);
}
void param_set_global_so(int value){
	GLOBAL_SO_ = value;
}
int param_get_global_so(){
	return(GLOBAL_SO_);
}







//----------------------------------------------
//				TOPOLOGY
//----------------------------------------------


//how superframes and beacons are organized
int	BOP_NB_SLOTS_       = 1;
int	ALGO_SF_ORG_        = ALGO_SF_ORG_802154;
int ALGO_BOP_           = ALGO_BOP_ORIG;
int ALGO_MULTICAST_     = ALGO_MULTICAST_DUPLICATE_ACK;


//BOP SLOTS
int param_get_bop_nb_slots(){
	return(BOP_NB_SLOTS_);
}
void param_set_bop_nb_slots(int value){
	BOP_NB_SLOTS_ = value;
}

//BOP algo
int param_get_bop_algo(){
    return(ALGO_BOP_);
}
void param_set_bop_algo(int value){
    ALGO_BOP_ = value;
}

//multicast algo
int param_get_multicast_algo(){
    return(ALGO_MULTICAST_);
}
void param_set_multicast_algo(int value){
    ALGO_MULTICAST_ = value;

}



//ALGO
int param_get_algo_sf(){
	return(ALGO_SF_ORG_);
}
void param_set_algo_sf(int value){
	ALGO_SF_ORG_ = value;
	
	if ((value != ALGO_SF_ORG_802154) && (value != ALGO_SF_ORG_RAND) && (value != ALGO_SF_ORG_GREEDY) && (value != ALGO_SF_ORG_GOD))
		tools_exit_short(5, "Unknown algorithm value %d\n", value);
	
}

//how many sf_slot may we use?
int param_get_nb_sfslot(){
	return((int) (pow(2, GLOBAL_BO_ - GLOBAL_SO_)));
}


//how we should compute the depth
int DEPTH_METRIC_ = DEPTH_HOPS;
int param_get_depth_metric(){
    return(DEPTH_METRIC_); 
}
void param_set_depth_metric(int value){
    DEPTH_METRIC_ = value; 
    
    if (value != DEPTH_HOPS && value != DEPTH_ETX && value != DEPTH_BER)
 		tools_exit_short(5, "Unknown depth metric value %d\n", value);
}


//cluster-tree
int NB_MAX_PARENTS_ = 256;
void param_set_nb_max_parents(int value){
	
	if (value != 0)
		NB_MAX_PARENTS_ = value;
	else
		NB_MAX_PARENTS_ = 256;
	
}
int param_get_nb_max_parents(){
	//upper bound
	int nb_sf_slots = pow(2, GLOBAL_BO_ - GLOBAL_SO_);
	if (NB_MAX_PARENTS_ > nb_sf_slots - 1)
		NB_MAX_PARENTS_ = nb_sf_slots - 1;
	
	if (NB_MAX_PARENTS_ == 0){
		fprintf(stderr, "The BO/SO values were inconsistently chosen. BO > SO so that superframe scheduling works!\n");
		tools_exit_short(2, "Currently BO=%d, SO=%d\n", GLOBAL_BO_, GLOBAL_SO_);
	}
	
	
	return(NB_MAX_PARENTS_);
}






//----------------------------------------------
//				ADDR MANAGEMENT
//----------------------------------------------


//for conversions and unicity
uint64_t	*id_to_long_addr;
uint16_t	*id_to_short_addr;


//memory allocation
void param_init_addr_vars(){
	int		i;
	
	//for address assignment
	if (id_to_long_addr == NULL){
		id_to_long_addr = (uint64_t*) malloc(get_node_count() * sizeof(uint64_t));
		for(i=0; i<get_node_count(); i++)
			id_to_long_addr[i] = ADDR_INVALID_64;
	}
	
	//for address assignment
	if (id_to_short_addr == NULL){
		id_to_short_addr = (uint16_t*) malloc(get_node_count() * sizeof(uint16_t));
		for(i=0; i<get_node_count(); i++)
			id_to_short_addr[i] = ADDR_INVALID_16;
	}
	
}



//returns a unique long address
void param_assign_unique_long_addr(call_t *c){
	nodedata_info *nodedata = get_node_private_data(c);    
	int		nb_nodes = get_node_count();
	
	//control
	short		unique;
	uint8_t		i;
	
	param_init_addr_vars();
	
	//finds a random address that was not already assigned
	do{
		//random long addressed (chosen byte per byte)
		nodedata->addr_long = 0;
		uint8_t *ptr = (uint8_t*) &(nodedata->addr_long);
		for (i=0; i<sizeof(uint64_t); i++)
			ptr[i] = (uint8_t)get_random_integer();
		
		
		//initialization
		unique = TRUE;
		
		//is this long address already assigned?
		for(i=0; (unique) && (i<nb_nodes); i++)
			if (id_to_long_addr[i] == nodedata->addr_long)
				unique = FALSE;
		
	}while(!unique);	
	
	
	//save this address
	id_to_long_addr[c->node]	= nodedata->addr_long;		
	
	//wsnet nodeid = short address, but can be modified later -> they are maintained independently!
	//NB: nodeid != addr_short has not been yet tested. It may not work and create segmentation faults
	id_to_short_addr[c->node]	= (uint16_t)c->node;
	
	tools_debug_print(c, "[ADDR] I choose the long address %llu (short %d)\n", (long long int)id_to_long_addr[c->node], id_to_short_addr[c->node]);

}


//returns the identifier associated to a long address
uint16_t param_addr_long_to_short(uint64_t addr_long){
	int		i;
	int		nb_nodes = get_node_count();
	
	param_init_addr_vars();

	for(i=0; (i<nb_nodes); i++)		
		if (id_to_long_addr[i] == addr_long)
			return(id_to_short_addr[i]);
	
	return(ADDR_INVALID_16);
}

//returns the long address associated to a short address
uint64_t param_addr_short_to_long(uint16_t addr_short){
	int		nb_nodes = get_node_count();
	
	param_init_addr_vars();

	if (addr_short >= nb_nodes)
		return(ADDR_INVALID_64);
	
	//if wsnet nodeid != short addr, this has to be modified!
	return(id_to_long_addr[addr_short]);
	
}


//----------------------------------------------
//		PRIVATE DATA INITIALIZATION
//----------------------------------------------


void param_verif_bounds(call_t *c){
	nodedata_info *nodedata = get_node_private_data(c);   
	
	//Superframe scheduling (the last value is dedicated to represent an "invalid slot"
	if (param_get_nb_sfslot()-1 >= SF_SLOT_INVALID){
		fprintf(stderr, "ERROR: BO=%d, SO=%d\n", nodedata->sframe.BO, nodedata->sframe.SO);
		tools_exit(3, c, "We don't have enough place to save the max nb of sf slots in the beacons (%d > %d)\n", param_get_nb_sfslot(), SF_SLOT_INVALID);
	}
	
	//BOP slots bound
	if (param_get_bop_nb_slots() >= pow(2, 8) - 1){
		tools_debug_print(c, "[ERROR] BOPslots=%d\n", param_get_bop_nb_slots());
		tools_exit(4, c, "We cannot place this value in 4 bits since the last value is dedicated to represent invalid value\n");
	}
	
}	


//assign the default values
void param_assign_default_value(call_t *c){
	nodedata_info *nodedata = get_node_private_data(c);    
	int		i;
	
	//save the wsnet id	
	nodedata->my_id = c->node;
	if (nodedata->my_id == 0)
		nodedata->node_type				= PAN_COORD;
	else
		nodedata->node_type				= PAN_NODE;
	
	
	//FSM
	nodedata->fsm.event						= NULL;
	nodedata->sframe.event_beacon_callback	= NULL;
	nodedata->sframe.event_beacon_stop      = NULL;
    
	//addresses
	param_assign_unique_long_addr(c);
	nodedata->addr_short				= ADDR_INVALID_16;
	    
	//cluster-tree
	nodedata->parents					= das_create();
	nodedata->parent_addr_current		= ADDR_INVALID_16;
	nodedata->sframe.my_current_role    = ROLE_NO;
    
	//mac
	//nodedata->mac.spf					= 0;
	nodedata->mac.cw					= CW;
	nodedata->mac.NB					= 0;
	nodedata->mac.BE					= 0;
	nodedata->mac.nb_retry				= 0;
    nodedata->mac.tx_end                = 0;
	
	//queues
	nodedata->buffer_data_up			= buffer_queue_create();
	nodedata->buffer_data_down			= buffer_queue_create();
	nodedata->buffer_mgmt				= buffer_queue_create();
    for(i=0; i<ADDR_MULTI_NB; i++)
                nodedata->buffer_multicast[i] = buffer_queue_create();
	nodedata->last_tx_seq_num			= get_random_integer_range(0, pow(2, sizeof(uint8_t) * 8));
	nodedata->last_seq_num_to_ack		= 0;
	nodedata->pk_tx_up_pending			= NULL; 
	buffer_change_pk_tx_down_pending(c, NULL); 
	
    //seqnums assigned randomly intially
    for(i=0; i<ADDR_MULTI_NB; i++)
        nodedata->seqnum_tx_multicast[i]    = get_random_integer_range(0, pow(2, sizeof(uint8_t) * 8) - 1);
    nodedata->seqnum_tx_unicast             = get_random_integer_range(0, pow(2, sizeof(uint8_t) * 8) - 1);
    nodedata->cbrseq_rx                     = das_create();
                         
	//radio info
	nodedata->channel					= -1;
    radio_switch(c, RADIO_OFF);
	
	//Only the PAN coordinator knows a priori the parameters (other ones will use beacons to know it)
	nodedata->sframe.BO					= -1;
	nodedata->sframe.SO					= -1;
	nodedata->sframe.end_my_sframe		= get_time();
	nodedata->sframe.my_bop_slot		= -1;
	nodedata->sframe.my_sf_slot			= -1;
	
	//neighborhood table
	nodedata->neigh_table				= das_create();		

	//Sf slot monitoring
	nodedata->sfslot_table				= malloc(sizeof(sfslot_info) * param_get_nb_sfslot());
	for(i=0; i<param_get_nb_sfslot(); i++){
		nodedata->sfslot_table[i].last_scan = 0;
		nodedata->sfslot_table[i].nb_pk_rx	= 0;
	}
    

	//------ SPECIAL CASE for the PAN COORD
	if (nodedata->node_type == PAN_COORD){
		nodedata->sframe.BO			= GLOBAL_BO_;
		nodedata->sframe.SO			= GLOBAL_SO_;
		nodedata->channel			= 0;		//use the common standard channel
		
		//short address
		nodedata->addr_short				= 0;
		
		//superframe
		nodedata->sframe.end_my_sframe		= -1;
		nodedata->sframe.my_bop_slot		= 0;		
		nodedata->sframe.my_sf_slot			= 0;		
	}
	
	
	//verification (parameters have to be included in the bounds)
	param_verif_bounds(c);
}



	
	
	
	
