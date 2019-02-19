/**
 *  \file   ieee802154_slotted.c
 *  \brief  IEEE 802.15.4 beacon eanabled mode
 *  \author Mohammed Nazim Abdeddaim, Fabrice Theoleyre
 *  \date   2011
 **/


//classic libs
#include <stdio.h>
#include <time.h>

//wsnet
#include <include/modelutils.h>

//our .h
#include "802154_slotted.h"



// *********************************************************** 
//					MODEL
// *********************************************************** 

model_t model =  {
    "IEEE 802.15.4 slotted CSMA/CA protocol (beacon-enabled mode)",
    "Mohamed-Nazim Abdeddaim & Fabrice Theoleyre",
    "1.0",
    MODELTYPE_MAC, 
    {NULL, 0}
};




//----------------------------------------------
//				CREATION
//----------------------------------------------




//called only once for all the nodes
int init(call_t *c, void *params) {
	int		i;
  	
	//memory allocation
	if (god_knowledge == NULL){
		god_knowledge = malloc(sizeof(nodedata_info) * (get_node_count()+1));
		for (i=0; i<get_node_count() ; i++)
			god_knowledge[i] = NULL;
	}	
	
	return(0);
}


//----------------------------------------------
//				DESTROY
//----------------------------------------------
int destroy(call_t *c) {
    printf("\n\nEND\n\n");    
	free(god_knowledge);
	return(0);
}



//----------------------------------------------
//				CREATION
//----------------------------------------------

//when the node is created
int setnode(call_t *c, param_t *params) {
	int		i;
	int		nb_nodes = get_node_count();
    
	//global stats/vars creation
	if (global_cbr_stats == NULL)
		global_cbr_stats = das_create();
	if (global_ctree_stats == NULL){
		global_ctree_stats = malloc(nb_nodes * sizeof(stat_ctree_info));
		
		for(i=0; i<nb_nodes; i++){
			global_ctree_stats[i].association_time = 0;
		}
	}
   
	//default values
	param_set_global_bo(BO_DEFAULT);
	param_set_global_so(SO_DEFAULT);	

    //memory allocation and registation
	nodedata_info *nodedata = (nodedata_info*) malloc(sizeof(nodedata_info));
	set_node_private_data(c, nodedata);
	
	nodedata->neigh_retransmit = das_create();
	nodedata->last_ack_time = 0;

	//so that each node can have an access to every one
	god_knowledge[c->node]		= nodedata;
 
    
	//the parameters	
    param_t		*param;    	
	int			value_int;
	
	//my node's parameters
    das_init_traverse(params);
    while ((param = (param_t *) das_traverse(params)) != NULL) {
        // printf("%s\n", param->key);
        
        //My BO value
        if (strcmp(param->key, "BO") == 0) {
            if (get_param_integer(param->value, &value_int))
                tools_exit(8, c, "BO is invalid\n");
            param_set_global_bo(value_int);
          //  printf("BO value %d\n", value_int);
        }		
        //My SO value
        if (strcmp(param->key, "SO") == 0) {
            if (get_param_integer(param->value, &value_int))
                tools_exit(8, c, "SO is invalid\n");
            param_set_global_so(value_int);
           // printf("SO value %d\n", value_int);
        }
        if (!strcmp(param->key, "start")) {
            if (get_param_time(param->value, &(nodedata->start))) 
                tools_exit(8, c, "start is invalid\n");
        }
        //DEBUG
        if (!strcmp(param->key, "debug")) {
            if (get_param_integer(param->value, &value_int) || (value_int !=1 && value_int != 0))
                tools_exit(8, c, "debug option is incorrectly specified\n");
            param_debug_set(value_int);
        }
        
        //CLUSTER-TREE ORGANIZATION
        if (strcmp(param->key, "bop-slots") == 0) {
            if (get_param_integer(param->value, &value_int))
                tools_exit(8, c, "BOP_NB_SLOTS_ is invalid\n");
            param_set_bop_nb_slots(value_int);
        }
        if (strcmp(param->key, "bop-algo") == 0) {
            if (get_param_integer(param->value, &value_int))
                tools_exit(8, c, "BOP_ALGO_ is invalid\n");
            param_set_bop_algo(value_int);
        }       
        if (strcmp(param->key, "sf-algo") == 0) {
            if (get_param_integer(param->value, &value_int))
                tools_exit(8, c, "ALGO_SF_ORG_ is invalid\n");
            param_set_algo_sf(value_int);			
        }
        if (strcmp(param->key, "nbmax-parents") == 0) {
            if (get_param_integer(param->value, &value_int))
                tools_exit(8, c, "nbmax-parents is invalid\n");
            param_set_nb_max_parents(value_int);
        }
        if (strcmp(param->key, "depth-metric") == 0) {
            if (get_param_integer(param->value, &value_int))
                tools_exit(8, c, "depth-metric is invalid\n");
            param_set_depth_metric(value_int);
        }

        
        //MULTICAST
        if (strcmp(param->key, "multicast-algo") == 0) {
            if (get_param_integer(param->value, &value_int))
                tools_exit(8, c, "MUTICAST_ALGO_ is invalid\n");
            param_set_multicast_algo(value_int);
        }
        
        
        //HELLOS
        if (strcmp(param->key, "hello-boperiod") == 0) {
            if (get_param_integer(param->value, &value_int))
                tools_exit(8, c, "hello period is invalid. It must be written in  nb of BI\n");
            param_set_hello_boperiod(value_int);
        }
        if (strcmp(param->key, "monitorbeacons") == 0) {
            if ((get_param_integer(param->value, &value_int)) || (value_int < 0) || (value_int > 1))
                tools_exit(8, c, "monitorbeacons is invalid. It must be a boolean\n");
            param_set_neigh_monitor_all(value_int);
        }
        if (strcmp(param->key, "scan-boperiod") == 0) {
            if (get_param_integer(param->value, &value_int))
                tools_exit(8, c, "scan-boperiod is invalid. It must be written in  nb of BI\n");
            param_set_sfslot_scan_period(value_int);
        }
     }  
 	
//	printf("%d %d\n", BO_, SO_);
		
	//assign the default values
	param_assign_default_value(c);
	
	
	//particular case: the 802.15.4 original algo has default values
	if (param_get_algo_sf() == ALGO_SF_ORG_802154){
		param_set_nb_max_parents(1);
	}
 	
	//OK
    return(0);
}

//----------------------------------------------
//				UNSET
//----------------------------------------------

short global_fig_created = FALSE;

int unsetnode(call_t *c) {
    nodedata_info *nodedata = get_node_private_data(c);    
	int i;
    
	//figure for debug (done once)
	if (!global_fig_created){
		tools_generate_figures(c);
		global_fig_created = TRUE;
	}
	
	//computes and prints the CBR stats
	if (global_cbr_stats != NULL)
		stats_print_all(c);
	
	
	//elements of the different lists
    neigh_info	*neigh = NULL;
		
	//neighborhood table (the elements in the das contain also das) 
	while ((neigh = (neigh_info*) das_pop(nodedata->neigh_table)) != NULL){		
        neigh_free(neigh);
        free(neigh);
    }
	das_destroy(nodedata->neigh_table);
	free(nodedata->sfslot_table);	
	
	//release memory for all the buffers
	buffer_queue_destroy(nodedata->buffer_data_up);
	buffer_queue_destroy(nodedata->buffer_data_down);
	buffer_queue_destroy(nodedata->buffer_mgmt);
	for(i=0; i<ADDR_MULTI_NB; i++)
        buffer_queue_destroy(nodedata->buffer_multicast[i]);

	
	//private properties
    free(nodedata);
    return(0);
}

//----------------------------------------------
//				START NODES
//----------------------------------------------



//send energy consumption info to the RPL layer
/*void send_energy_consumption_info(call_t *c){
	call_t c_rpl = {get_entity_bindings_up(c)->elts[0], c->node, c->entity};
	
	int j;
	my_energy_t *total_consumed =  malloc(sizeof(my_energy_t));
	total_consumed->consumption = 0;
	total_consumed->duration = 0;
	for(j=0; j<RADIO_NB_MODES; j++){
		total_consumed->consumption += god_knowledge[c->node]->radio_energy->consumption[j];
		total_consumed->duration += god_knowledge[c->node]->radio_energy->duration[j];		
	}
	//printf("%d, consumption %f\n", c->node, god_knowledge[c->node]->radio_energy->consumption[j]);
		
	IOCTL(&c_rpl, ENERGY_CONSUMED, &total_consumed, NULL);
	scheduler_add_callback(get_time()+10000000000, c, send_energy_consumption_info, NULL);
}*/

int	macMaxFrameTotalWaitTime = phyMaxFrameDuration + aUnitBackoffPeriod;
int bootstrap(call_t *c) {
    nodedata_info *nodedata = get_node_private_data(c);
  
	//ernergy consumption (a pointer toward the stats updated by the radio module)
    call_t c_radio = {get_entity_bindings_down(c)->elts[0], c->node, c->entity};
    IOCTL(&c_radio, RADIO_IOCTL_GETENCONSUMMED, NULL, (void*)&(nodedata->radio_energy));
    //printf("MAC Node %d energy pointer %d\n ", c->node, &(nodedata->radio_energy));          

    //send_energy_consumption_info(c);
    
    //BUG
    if (ADDR_MULTI_MAX - ADDR_MULTI_MIN + 1 != ADDR_MULTI_NB)
        tools_exit(3,c, "[BUG] the mtulicast addresses are not well configured. ADDR_MULTI_MAX (%d) - ADDR_MULTI_MIN (%d) != ADDR_MULTI_NB (%d)\n", 
                   ADDR_MULTI_MAX, ADDR_MULTI_MIN, ADDR_MULTI_NB);

	//PAN COORDINATOR -> go to STATE_SLEEP (it will then go to STATE_COORD_IDLE after having transmitted one beacon)	
	if (nodedata->node_type == PAN_COORD){
		fsm_change(c, STATE_SLEEP, get_time());
		
		//beacon schedulings
		radio_switch(c, RADIO_RX);
		scheduler_add_callback (get_time(), c, beacon_callback, NULL);
        
        //enqueue a new hello
        //neigh_hello_enqueue(c);
        
        //update my neightable (myself -> 0-neighbor)
        neigh_update_myself(c);
	}
	//NORMAL NODES -> we having nothing to do except waiting for beacons to be associated
	else{
		fsm_change(c, STATE_UNASSOC_WAIT_BEACON, get_time());		
		radio_switch(c, RADIO_RX);
	}
    
    //debug
    if (CBR_PAN_COORD_ID != ID_PAN_COORD)
        tools_exit(2, c, "[BUG] the PAN_COORD id MUST be the same in the application and MAC layers (%d != %d)\n", CBR_PAN_COORD_ID, ID_PAN_COORD);
    
	//printf("dur : %f\n", (double)pk_get_bop_slot_duration(c) /1000000.0);

	return 0;	
}


//----------------------------------------------
//				IOCTL
//----------------------------------------------

//ioctl for inter-layer communication
int ioctl(call_t *c, int option, void *in, void **out) {
 	nodedata_info	*nodedata		= get_node_private_data(c);

    switch(option){
 
        case MAC_IOCTL_NOTIFFREE:
            
            //forces a state transition for the backoff: medium is busy during the backoff (BOP period)
            if (nodedata->fsm.state == STATE_BOP_WAIT_FREE){
                state_machine(c, NULL);
                tools_debug_print(c, "[RADIO] the medium is now free\n");
            }
                        
        break;
        
        
        
        case MAC_IOCTL_RETRANSMIT:
        	//printf("MAC_IOCTL node %d pointer %d\n\n", c->node, &(nodedata->neigh_retransmit));
        	 *out = nodedata->neigh_retransmit;            

        break;
     }
    return(0);
}


//----------------------------------------------
//				RETRANSMISSION MANAGEMENT
//----------------------------------------------

//the packet was corectly txed, acked, etc. I must now dequeue it from the correct buffer
void dequeue_desalloc_tx_down_pending_packet(call_t *c){
	nodedata_info	*nodedata		= get_node_private_data(c);
	
	//EROOR
	if (nodedata->pk_tx_down_pending == NULL)
		tools_exit(9, c, "ERROR: %d cannot dealloc and dequeue a non existing packet\n", c->node);

	
	tools_debug_print(c, "[BUFFER] desalloc packet with type %s\n", pk_ftype_to_str(pk_get_ftype(nodedata->pk_tx_down_pending->packet)));
	
    
  	//de-buffering -> it depends on the packet type
	switch(pk_get_ftype(nodedata->pk_tx_down_pending->packet)){
			
		//data packet from the coordinator to the children are buffered automatically (indirect mode)
		case MAC_DATA_PACKET:
			;
			
			//remove the element from buffer (element automatically freed)
			if (nodedata->pk_tx_down_pending->buffered)
				buffer_queue_delete_ptr(nodedata->buffer_data_down, nodedata->pk_tx_down_pending); 
			
			//or have to release manually the memory
			else{
				//oana: packet size = 18; must be a beacon?
				packet_dealloc(nodedata->pk_tx_down_pending->packet);
				free(nodedata->pk_tx_down_pending);
			}
			buffer_change_pk_tx_down_pending(c , NULL);			
			
			break;
			
			
		//Only association responses are buffered
		case MAC_COMMAND_PACKET:
			;			
			_802_15_4_COMMAND_header	*new_hdr_cmd	= (_802_15_4_COMMAND_header*) (nodedata->pk_tx_down_pending->packet->data + sizeof(_802_15_4_header));				
		
			if ((new_hdr_cmd->type_command == ASSOCIATION_RESPONSE) && (nodedata->pk_tx_down_pending->buffered))
				buffer_queue_delete_ptr(nodedata->buffer_mgmt, nodedata->pk_tx_down_pending); 
		
			else{
			    //oana: must be a beacon?
				packet_dealloc(nodedata->pk_tx_down_pending->packet);
				free(nodedata->pk_tx_down_pending);
			}
			buffer_change_pk_tx_down_pending(c , NULL);			

            
            break;
			
		default:
			fprintf(stderr, "ERROR: this frame type (%d) is not handled in COORD_DATA_TX state\n", pk_get_ftype(nodedata->pk_tx_down_pending->packet));
			break;					
	}	

}

//we are a child and the transmission is canceled
void dequeue_desalloc_tx_up_pending_packet(call_t *c){
	nodedata_info	*nodedata		= get_node_private_data(c);

    tools_debug_print(c, "[UP]remove the pending up packet\n");
    
    //empty the current pointer for the next transmitted packet
    if (nodedata->pk_tx_up_pending != NULL){
        if (nodedata->pk_tx_up_pending->buffered)
            buffer_queue_delete_ptr(nodedata->buffer_data_up, nodedata->pk_tx_up_pending);
		else{
			//oana: packet size ~ 16	
			packet_dealloc(nodedata->pk_tx_up_pending->packet);		
			free(nodedata->pk_tx_up_pending);
		}
        buffer_change_pk_tx_up_pending(c, NULL);
    }
}

int toomany = 0;

//the transmission failed (e.g. the medium is busy), schedule a retransmission (if we have not too many retx or CCA)
void schedule_retx_if_possible(call_t *c){
	nodedata_info	*nodedata		= get_node_private_data(c);
	uint64_t		now = get_time();
	
	//the tx has failed -> update the MAC parameters
	nodedata->mac.cw = 2;		//Nb of CCA
	nodedata->mac.NB ++;		//Nb of CCA
	nodedata->mac.BE = tools_min(++(nodedata->mac.BE), macMaxBE);		//the BE value to choose the backoff
	
	
	//we MUST be in a superframe, else we don't transmit/backoff, we just respond to our children)
	parent_info	*pinfo_current = ctree_parent_get_info(nodedata->parents, nodedata->parent_addr_current);
    if ((pinfo_current == NULL) && (pk_get_subtype(nodedata->pk_tx_up_pending->packet) != MULTICAST_REQUEST))
        tools_exit(12, c, "[ERROR] %d MUST be in an active superframe\n", c->node); 
    neigh_info  *neigh_elem = neigh_table_get(c, nodedata->parent_addr_current);
 	
	//verify that we are in the active part of our superframe, else, go sleeping and consider the transmission failed
	uint64_t	parent_beacon_time = neigh_elem->sf_slot * tools_get_sd_from_so(nodedata->sframe.SO);
	while (parent_beacon_time + tools_get_sd_from_so(nodedata->sframe.SO) < now)
		parent_beacon_time += tools_get_bi_from_bo(nodedata->sframe.BO);
	
	
	// too many CCA 
	// OR out of the boundaries of our superframe	
	//=> drop the packet and go to the idle state
	//NB: we just have to verify we are inside a superframe. BACKOFF will verify later we have enough time to transmit + receive the ack in the sf
	if (
		(nodedata->mac.NB > macMaxCSMABackoffs) 
			||
		(now - parent_beacon_time > tools_get_sd_from_so(nodedata->sframe.SO))
		){
		
		
		//My state depends on if I am associated or not
        if ((pinfo_current == NULL) && (pk_get_subtype(nodedata->pk_tx_up_pending->packet) != MULTICAST_REQUEST))
            tools_exit(2, c, "%d should have a valid current parent (currently %d) to retransmit one packet (transmissions are triggered by a child)\n", nodedata->addr_short, nodedata->parent_addr_current);

		//this parent is now invalid: the association failed
		else if ((pinfo_current != NULL) && (!pinfo_current->associated)){
			ctree_parent_print_debug_info(c);
			mgmt_parent_disassociated(c, nodedata->parent_addr_current);
			tools_debug_print(c, "[TX] too many CCA (%d), I delete the packet and the corresponding parent\n", nodedata->mac.NB);
			//if(nodedata->pk_tx_up_pending->packet->size == 127)
			//	printf("\t\t[MAC_TX] node %d too many CCA, I delete the packet after %d retransmissions and come back to my IDLE state size %d total %d\n\n",c->node, nodedata->mac.NB, nodedata->pk_tx_up_pending->packet->size, ++toomany);
		}

		//the frame is dropped
        else{
			nodedata->fsm.state = STATE_CHILD_IDLE;			
			tools_debug_print(c, "[TX] too many CCA, I delete the packet after %d retransmissions and come back to my IDLE state\n", nodedata->mac.NB);
			//if(nodedata->pk_tx_up_pending->packet->size == 127)
            if(pk_get_ftype(nodedata->pk_tx_up_pending->packet) == MAC_DATA_PACKET){
				printf("\t\t[MAC_TX] node %d too many CCA, I delete the packet after %d retransmissions and come back to my IDLE state size %d total %d\n\n",c->node, nodedata->mac.NB, nodedata->pk_tx_up_pending->packet->size, ++toomany);
                data_drop_packet(c,nodedata->pk_tx_up_pending->packet);
            }
			stats_data_update_drop_reason(c, nodedata->pk_tx_up_pending->packet, DROP_CCA);
		}
		fsm_change(c, STATE_KEPT, get_time());
		
		
        //cancel pending transmission (memory desalloc)
        dequeue_desalloc_tx_up_pending_packet(c);
			
		//for the next time
		nodedata->mac.NB = 0;
	
		return;
	}
	// Another backoff / retransmission
	else {
		tools_debug_print(c, "[TX] we schedule a retransmission\n");
		//if(nodedata->pk_tx_up_pending->packet->size > 1000)
        //	printf("[TX] %d we schedule a retransmission channel was busy\n", c->node);
         
		fsm_change(c, STATE_CHILD_BACKOFF, get_time());
		return;
	}
	

}




//----------------------------------------------
//				STATE MACHINE
//----------------------------------------------




//changes the next transition
void fsm_change(call_t *c, int state, uint64_t time){
	nodedata_info	*nodedata		= get_node_private_data(c);
	char			msg[150];
	
	//cancel the previous event and schedule a new one
	if (nodedata->fsm.event != NULL){
		tools_debug_print(c, "[SCHEDULER] CANCEL event scheduled at %s (old state %s)\n", tools_time_to_str(nodedata->fsm.event->clock, msg, 150), tools_fsm_state_to_str(nodedata->fsm.state));
		scheduler_delete_callback(c, nodedata->fsm.event);
	}
	
	//change the state or maintain it as is
	if (state != STATE_KEPT)
		nodedata->fsm.state = state;
	
	//the time for the transition
	nodedata->fsm.clock = time;
	
	nodedata->fsm.event = scheduler_add_callback(nodedata->fsm.clock, c, state_machine, NULL);
	//tools_debug_print(c, "[SCHEDULER] STATE %s at %s\n", tools_fsm_state_to_str(nodedata->fsm.state), tools_time_to_str(nodedata->fsm.clock, msg, 150));
}

						   
//walk in the state machine
int state_machine(call_t *c, void *arg) { 

	nodedata_info	*nodedata		= get_node_private_data(c);
	call_t c_radio = {get_entity_bindings_down(c)->elts[0], c->node, c->entity};
	
	
	//Time parameters
	uint64_t	now = get_time();//Curent time
	uint64_t	tx_time;
	
	//control
	char	msg[150], msg2[50];
	
	//------ FSM -----

	//the event of the FSM is now considered treated
	
    //tools_debug_print(c, "[FSM] state machine\n");
    nodedata->fsm.event = NULL;

	//this interruption is obsolete (it was schedule but is not anymore accurate)
	//E.g. an ack verification although the ack was already received
	if (nodedata->fsm.clock != get_time()){
		tools_debug_print(c, "[SCHEDULER] obsolete fsm change (another one replaces this event)\n");
		return(0);
	}
	
	
	//the current active parent (I am currently following its superframe)
	parent_info		*pinfo_current  = ctree_parent_get_info(nodedata->parents, nodedata->parent_addr_current);
 
	//debug	
	tools_debug_print(c, "[FSM] current state %s, radio mode %s\n", tools_fsm_state_to_str(nodedata->fsm.state), radio_mode_to_str(radio_get_current_mode(c)));

	
	
	switch (nodedata->fsm.state) {
		
			
			
//-------------------------------------------------------------------------------
//       UNASSOCIATED MODE
//-------------------------------------------------------------------------------
		
		//we wait for a possible parent
		case STATE_UNASSOC_WAIT_BEACON:	
			radio_switch(c, RADIO_RX);
			
				
			return(0);
			break;
			

		//The COMMAND was transmitted, we wait for the ack
		//case 1: association req transmitted -> wait for an ack
		//case 2: data-req transmitted -> wait for an ack + association reply
		case STATE_UNASSOC_WAIT_ACK_ASSOC_REQ:					
		case STATE_UNASSOC_WAIT_ASSOC_REP:	
			tools_debug_print(c, "[ASSOC] My association-req failed\n");			
			schedule_retx_if_possible(c);
				
			return(0);
			break;

		
			
//-------------------------------------------------------------------------------
//           SLEEPING MODE
//-------------------------------------------------------------------------------

			
		//Sleeping mode!
		case STATE_SLEEP:
			
			//A node never sleeps during the X first seconds (to find possibly several parents to follow)
       		if (get_time() > TIME_BEFORE_CONVERGENCE)
				radio_switch(c, RADIO_OFF);			
            else
                tools_debug_print(c, "[ASSOC] a node does not sleep during the first seconds to sniff other parents\n");			
             
            //bug
            if (nodedata->pk_tx_down_pending != NULL)
               tools_exit(15, c, "non null pending frames although we are going sleeping\n");
			
			//MAC parameters are re-initialized for the next superframe
			nodedata->mac.NB = 0;
			
			//no active parent
			nodedata->parent_addr_current       = ADDR_INVALID_16;				
            nodedata->sframe.my_current_role    = ROLE_NO;
 			
			
			//-------- Next Parent -----
			
			//get the next scheduled parent to listen to
			parent_info *parent_next = ctree_parent_get_next_parent_to_listen(c);
			//ctree_parent_print_debug_info(c);
			
			// no more parent -> go to STATE_UNASSOC_WAIT_BEACON without sleeping!
			if ((parent_next == NULL) && (nodedata->node_type != PAN_COORD)){
				
				//unschedule our beacon
				if (nodedata->sframe.event_beacon_callback != NULL)
					scheduler_delete_callback(c, nodedata->sframe.event_beacon_callback);
				
				//change to the correct unassociated STATE with a radio ON to receive beacons
				fsm_change(c, STATE_UNASSOC_WAIT_BEACON, now);			
				radio_switch(c, RADIO_RX);
				
				tools_debug_print(c, "[UNASSOC] the node is not anymore associated to the PAN, wait for a beacon\n");				
				
				return(0);
			}

			
			//their superframe
			uint64_t	time_sf_next_sf_mine			= beacon_compute_next_sf(nodedata->sframe.my_sf_slot);
			uint64_t	time_sf_next_sf_closest_parent;
			
			if (nodedata->node_type != PAN_COORD)
				time_sf_next_sf_closest_parent	= beacon_compute_next_sf(parent_next->sf_slot);
			else
				time_sf_next_sf_closest_parent = time_sf_next_sf_mine + 1;

				
			//not yet associated -> I have no superframe
			if ((!ctree_parent_is_associated(nodedata->parents)) && (nodedata->node_type != PAN_COORD))
				time_sf_next_sf_mine = time_sf_next_sf_closest_parent + 1;
				
			
			//-------- Next Neighbor to monitor -----
	
			uint64_t time_sf_next_sfslot = 0;
			if (param_get_neigh_monitor_all())
				time_sf_next_sfslot = neigh_get_next_sfslot_to_listen(c);
			
			// no neigbor to monitor
			if (time_sf_next_sfslot == 0)
				time_sf_next_sfslot = time_sf_next_sf_mine + 1;			

           
			//-------- Wake-up -----			
			//NB: we have a priority: in a particular superframe, I follow my superframe / thus of one parent / thus of one neighbor
			
			
			//tools_debug_print(c, "[DEBUG] my sf %llu, next parent %llu, next sfslot %llu\n", time_sf_next_sf_mine - GUARD_TIME, time_sf_next_sf_closest_parent - GUARD_TIME, time_sf_next_sfslot - GUARD_TIME);
			
			//our own frame is the closest -> the beacon generation will wake-up us!
			if ((time_sf_next_sf_mine <= time_sf_next_sf_closest_parent) && (time_sf_next_sf_mine <= time_sf_next_sfslot)){
				tools_debug_print(c, "[BEACON] I will wake up automatically at %llu for my own beacon\n", time_sf_next_sf_mine);
					
				return(0);				
			}
			//we have to wake-up to receive the beacon of one parent or of one neighbor
			else{
				uint64_t	time_wakeup;				

				//PARENT case
				if (time_sf_next_sf_closest_parent <= time_sf_next_sfslot){
					tools_debug_print(c, "[WAIT] I will wait for the beacon from my parent %d at %llu (sf slot %d, bop slot %d, startframe %llu, NB %d)\n", parent_next->addr_short, time_sf_next_sf_closest_parent - GUARD_TIME, parent_next->sf_slot, parent_next->bop_slot, time_sf_next_sf_closest_parent, nodedata->mac.NB);
					time_wakeup = time_sf_next_sf_closest_parent - GUARD_TIME;
				}				
				//BEACON CASE
				else{
					tools_debug_print(c, "[SLEEP] I will wait for the superframe slot %d (current sfslot %d), at %s\n",
                                      tools_compute_sfslot(time_sf_next_sfslot),
                                      tools_compute_sfslot(get_time()),
                                      tools_time_to_str(time_sf_next_sfslot, msg, 50)
                                      );
					time_wakeup = time_sf_next_sfslot - GUARD_TIME;	
				}							
				
				//if time is negative, this means we must stay awake in this sf slot, whatever the GUARD_TIME is!
				if (time_wakeup > get_time())
					fsm_change(c, STATE_CHILD_WAIT_BEACON, time_wakeup);			
				else
					fsm_change(c, STATE_CHILD_WAIT_BEACON, get_time());			
				
				return(0);				
			}	
			
			
			tools_exit(3, c, "we cannot enter here if everything is ok\n");	

			break;	
            
            
//-------------------------------------------------------------------------------
//          COORDINATOR MODE
//-------------------------------------------------------------------------------
		
		//when I am coordinating my own superframe				
		case STATE_COORD_IDLE :
			
			//that's the end of my superframe -> switch to sleeping state (nothing occurs currently)
			if (nodedata->sframe.end_my_sframe <= now){
				//tools_debug_print(c, "[COORD] my frame is finished\n");
				
				//flush possible pending packets
				if (nodedata->pk_tx_down_pending != NULL)
					dequeue_desalloc_tx_down_pending_packet(c);
						
				
				//go to the sleeping state
				nodedata->fsm.clock = get_time();
				nodedata->fsm.state = STATE_SLEEP;
				state_machine(c, NULL);
				return(0);
			}			
			
		break;
			
			
		//I received a data-request and I transmitted the corresponding ack, I have now to send the data frame 				
		case STATE_COORD_DATA_TX:			
			;
			//clone the packet before transmission
			packet_t	*packet_new = packet_clone(nodedata->pk_tx_down_pending->packet);			
			
			//does it exist pending packets for this destination? 
			uint16_t	dest_short = pk_get_dst_short(packet_new);
			short		PF = buffer_search_for_addr_short(c, dest_short, nodedata->pk_tx_down_pending->packet);
			pk_set_P(packet_new , PF);
						
			//tranmission time
			tx_time = packet_new->real_size * radio_get_Tb(&c_radio);
			
			//debug
			tools_debug_print(c, "[DATA] will tx data seq num %d at %s (pending frames %d)\n", pk_get_seqnum(packet_new), tools_time_to_str(get_time() + mactACK, msg, 50), PF);
			
			//ack is required?
			if (pk_get_A(packet_new)){
				//to be acked
				nodedata->last_seq_num_to_ack = pk_get_seqnum(packet_new);
				
				//FSM
                if (get_time() + mactACK + tx_time + macAckWaitDuration >= nodedata->sframe.end_my_sframe)
                    fsm_change(c, STATE_COORD_WAIT_ACK, nodedata->sframe.end_my_sframe - 1);
                else
                    fsm_change(c, STATE_COORD_WAIT_ACK, get_time() + mactACK + tx_time + macAckWaitDuration);
				tools_debug_print(c, "[TX] will wait for an ack (until %s)\n", tools_time_to_str(nodedata->fsm.clock, msg, 50));
			}
			//come back directly to the idle state after having removed the corresponding packet and finished to tx the packet
			else{
				tools_debug_print(c, "[TX] no ack required\n");

				fsm_change(c, STATE_COORD_IDLE, get_time() + mactACK + tx_time);              
                
				//qeueue the packet from the corresponding queue
				dequeue_desalloc_tx_down_pending_packet(c);						
			}
			
			//transmission of the data frame
            if (get_time() + mactACK + tx_time <= nodedata->sframe.end_my_sframe)
                scheduler_add_callback(get_time() + mactACK, c, radio_pk_tx, (void*)packet_new);
            else{
                tools_debug_print(c, "[TX] cannot transmit the packet, it will be finished after the end of my superframe\n");
                //printf("[TX] %d cannot transmit the packet, it will be finished after the end of my superframe\n", c->node);
			}
			break;
			
		//ack was not received coreclty, enqueue the transmitted packet and maintain a retx counter
		case STATE_COORD_WAIT_ACK:
			
			//too many retransmissions
			if ((++(nodedata->pk_tx_down_pending->nb_retry)) > macMaxFrameRetries){
				tools_debug_print(c, "\n\n[TX] the packet was transmitted too many times, we drop it\n");
				if(nodedata->pk_tx_down_pending->packet->size == 127){ //data pkt
					printf("\t\t[MAC_TX] Node %d the packet was transmitted too many times, we drop it size %d\n\n", c->node, nodedata->pk_tx_down_pending->packet->size);
				}

				//qeueue the packet from the corresponding queue
				dequeue_desalloc_tx_down_pending_packet(c);				
			}
			//we have to try again -> just reinit the pointer (without releasing the memory, be careful)
			else{
				tools_debug_print(c, "[RETX] %d erases pending packet\n", c->node);
				buffer_change_pk_tx_down_pending(c, NULL);
			}
			
			//IDLE STATE immediately
			fsm_change(c, STATE_COORD_IDLE, get_time());
			return(0);
			
		break;
			
			

			
			
//-------------------------------------------------------------------------------
//              CHILD MODE
//-------------------------------------------------------------------------------
            
		//We do nothing: we are waiting the beacon of our parent
		case STATE_CHILD_WAIT_BEACON:
			
			//turns our radio on, we have just woke up
			radio_switch(c, RADIO_RX);
			
            //I scanned this superframe slot (load reinitialized)
			nodedata->sfslot_table[tools_compute_sfslot(get_time())].last_scan	= get_time();
			nodedata->sfslot_table[tools_compute_sfslot(get_time())].nb_pk_rx	= 0;
            nodedata->sframe.my_current_role = ROLE_SCAN;
            
			//while we have one parent that is expected to wake up around the current time, stay awake
			//else, sleep and schedule a wake-up for the next parent that will send a beacon			
			if (pinfo_current == NULL){
				//tools_debug_print(c, "[BEACON] we are waiting for a beacon - scanning phase (sfslot %d)\n", tools_compute_sfslot(get_time()));
            }
			else
				tools_debug_print(c, "[BEACON] we are waiting for a beacon from %d (sfslot %d)\n", pinfo_current->addr_short, tools_compute_sfslot(get_time()));
			
            //BOP backoff -> will stop the waiting time when the period has finished
            if (param_get_bop_algo() == ALGO_BOP_BACKOFF){
                beacon_stop_waiting_schedule(c, get_time() + tools_get_sd_from_so(nodedata->sframe.SO));
                fsm_change(c, STATE_BOP_WAIT_FREE, get_time());
            }  
            
            //schedule the end of the beacon scanning
            else{
                uint64_t time_verif = get_time() + pk_get_bop_slot_duration(c) * param_get_bop_nb_slots() + 2 * GUARD_TIME;
           
                //schedule sleeping verification if no beacon is received during the BOP period
                fsm_change(c, STATE_CHILD_STOP_WAIT_BEACON, time_verif);
               // tools_debug_print(c, "[BEACON] end of waiting scheduled at %s, bop algo %d\n", 
               //                   tools_time_to_str(time_verif, msg, 150),
               //                   param_get_bop_algo());
			}
			break;

			
			
		//We are waiting a beacon for a too long time -> go immediately sleeping, we missed it
		case STATE_CHILD_STOP_WAIT_BEACON:
			
			tools_debug_print(c, "[BEACON] I stop waiting for beacons (the BOP period has finished)\n");
			
			nodedata->fsm.state = STATE_SLEEP;
			nodedata->fsm.clock = get_time();
			state_machine(c, NULL);
			
		break;
			
			
			
		//We can transmit: we are in the superframe of one of our parents
		case STATE_CHILD_IDLE :
			;
			packet_info *pk_i;

			//I MUST either be associated to a parent or I have a multicast-req to send (I am a simple "follower")
            neigh_info *neigh_elem  = neigh_table_get(c, nodedata->parent_addr_current);   
            if (neigh_elem == NULL)
                tools_exit(4, c, "[ASSOC] %d is a parent. It MUST be present in the neigh table\n", nodedata->parent_addr_current);
            
            if ((pinfo_current == NULL) && (!neigh_elem->multicast_to_rx) && (nodedata->pk_tx_up_pending == NULL)){
				ctree_parent_print_debug_info(c);
				tools_exit(3, c, "%d is a child but the current parent %d cannot be found (invalid parent = %d, nbparents %d)\n", c->node, nodedata->parent_addr_current, nodedata->parent_addr_current == ADDR_INVALID_16, das_getsize(nodedata->parents));
			}            
   		
			//verify that we are in the active part of our superframe, else, go sleeping!
			if (get_time() - beacon_get_time_start_sframe(c) > tools_get_sd_from_so(nodedata->sframe.SO)){		
				tools_debug_print(c, "[SF] the superframe of my parent is finished, I have now to sleep\n");
				fsm_change(c, STATE_SLEEP, get_time());
				return(0);
			}			
			
			//printf("pk_tx_up_pending: %d, parent_notification: %f\n", (int) nodedata->pk_tx_up_pending->packet, pinfo_current->positive_notif);
			if (nodedata->mac.NB != 0)
				tools_exit(4, c, "ERROR, %d should not have a non-null NB here (=%d). This means that we have a current retransmission although we are in the idle state!\n", c->node, nodedata->mac.NB);			
			
			//re-initization for transmissions
			nodedata->mac.NB = 0;			
            
            //tools_debug_print(c, "[TEST] pending %d, null %d, assoc %d, node %d\n", nodedata->pk_tx_up_pending,
              //                pinfo_current,
                  //            pinfo_current != NULL && pinfo_current->associated,
                //              nodedata->parent_addr_current);
            //buffer_content_print(c, nodedata->buffer_data_up, "DATA-UP");            
            
			//We cannot retransmit packets for nodes that are not our parent in this superframe
			if (
				(nodedata->pk_tx_up_pending != NULL)
				&& (pk_get_dst_short(nodedata->pk_tx_up_pending->packet) != pinfo_current->addr_short)
				&& (!tools_is_addr_multicast(pk_get_dst_short(nodedata->pk_tx_up_pending->packet)))
				&& (!tools_is_addr_anycast(pk_get_dst_short(nodedata->pk_tx_up_pending->packet)))
				){
				pk_print_content(c, nodedata->pk_tx_up_pending->packet, stdout);
				tools_exit(5, c, "[ERROR] We cannot have a pending packet to %d while being in the superframe of %d\n", pk_get_dst_short(nodedata->pk_tx_up_pending->packet), pinfo_current!= NULL && pinfo_current->addr_short);
			}
			//if we are not associated, we have to send a data-req to get our association reply
			else if ((nodedata->pk_tx_up_pending == NULL) && (pinfo_current != NULL) && (!pinfo_current->associated)){
				
				nodedata->pk_tx_up_pending = mgmt_data_request_long_src_create(c, pinfo_current->addr_short);
				tools_debug_print(c, "[DATA-REQ] I send a command frame to my parent %d to get my assoc-rep\n", pinfo_current->addr_short);
				
			}
            //we must ask for a multicast packet
            else if ((nodedata->pk_tx_up_pending == NULL) && (neigh_elem->multicast_to_rx)){
                nodedata->pk_tx_up_pending = mgmt_multicast_req_pk_create(c, nodedata->parent_addr_current);
    			tools_debug_print(c, "[MULTICAST-REQ] I send a command frame to %d\n", nodedata->parent_addr_current);
            }
            else if (pinfo_current == NULL)
                tools_exit(12, c, "is a child but the current parent %d cannot be found and this is not for an multicast-req (invalid parent = %d, nbparents %d)\n", c->node, nodedata->parent_addr_current, nodedata->parent_addr_current == ADDR_INVALID_16, das_getsize(nodedata->parents));        
            
			//if we have pending packet in the current superframe, let ask them first
			else if ((nodedata->pk_tx_up_pending == NULL) && (pinfo_current != NULL) && (pinfo_current->positive_notif > get_time())){
				
				nodedata->pk_tx_up_pending = mgmt_data_request_create(c, pinfo_current->addr_short);
				tools_debug_print(c, "[DATA-REQ] I send a command frame to my parent (short src %d)\n", nodedata->addr_short);
				
			}		
			//if we don't have any packet to transmit, dequeue the first one (in the upload direction)
			else if ((nodedata->pk_tx_up_pending == NULL) && (pinfo_current != NULL) && 
					 ((pk_i = buffer_queue_get_first_for_parent(nodedata->buffer_data_up, pinfo_current->addr_short)) != NULL)){
							
				//replace the pointer for the pending transmitted packet
				nodedata->pk_tx_up_pending	= pk_i;
				nodedata->mac.nb_retry		= nodedata->pk_tx_up_pending->nb_retry;

				//particular stats for DATA
				if (pk_get_ftype(nodedata->pk_tx_up_pending->packet) == MAC_DATA_PACKET){
					_802_15_4_DATA_header	*header_data	= (_802_15_4_DATA_header *) (nodedata->pk_tx_up_pending->packet->data + sizeof(_802_15_4_header));
					
					header_data->src		= nodedata->addr_short; 
					header_data->dst		= pinfo_current->addr_short ;
					
					//printf("TEST %d = %d | %d = %d\n", header_data->src, nodedata->addr_short, header_data->dst, pinfo_current->addr_short);
					
					tools_debug_print(c, "[DATA] I dequeue one packet to send to my parent (last positive notif %llu, seqnum=%d, cbrseq=%d)\n", pinfo_current->positive_notif, pk_get_seqnum(nodedata->pk_tx_up_pending->packet), pk_get_cbrseq(nodedata->pk_tx_up_pending->packet));
					//pk_print_content(c, nodedata->pk_tx_up_pending->packet, stdout);
					
				}
				else{
					tools_debug_print(c, "[MGMT] I dequeue one packet from the UP queue\n");					
				}

			}
			
			//we have a pending transmission
			if (nodedata->pk_tx_up_pending != NULL){
				//tools_debug_print(c, "[RETX] We have now a pending packet for %d (nbretx %d)\n", pk_get_dst_short(nodedata->pk_tx_up_pending->packet), nodedata->mac.NB);
				
				//FSM
				fsm_change(c, STATE_CHILD_BACKOFF, now);
				return(0);
			}
				
				
			//If we don't have any packet to transmit, let sleep
			else{
				fsm_change(c, STATE_SLEEP, get_time());
				return(0);
			}	
			
			
		break;
		
			
			
		//backoff before transmission
            
		case STATE_CHILD_BACKOFF: 			
            
            //I MUST be associated to a parent if this is not an MULTICAST-REQ
            if ((pinfo_current == NULL) && (pk_get_subtype(nodedata->pk_tx_up_pending->packet) != MULTICAST_REQUEST))
 				tools_exit(3, c, "%d is a child but the current parent %d cannot be found (invalid parent = %d)\n", c->node, nodedata->parent_addr_current, nodedata->parent_addr_current == ADDR_INVALID_16);

			
			//The UP buffer MUST be non null
			if (nodedata->pk_tx_up_pending == NULL)
				tools_exit(5, c, "[ERROR]: the PK UP is empty\n");			
			
			//reinitializes the backoff exponent and other parameters when this is the first transmission
			if (nodedata->mac.NB == 0){
				nodedata->mac.BE		= 3;
				nodedata->mac.nb_retry	= 0;
			}
			
			//choose a random backoff
			int	backoff_int = get_random_integer_range(0, (pow(2, nodedata->mac.BE) - 1));
			uint64_t backoff = aUnitBackoffPeriod * backoff_int;
			
			//we are out of the superframe -> BUG!
			if (beacon_get_time_start_sframe(c) > now){				
				tools_debug_print(c, "now %s\n", tools_time_to_str(get_time(), msg, 100));
				tools_debug_print(c, "time beacon %s\n", tools_time_to_str(beacon_get_time_start_sframe(c), msg, 100));
				tools_debug_print(c, "sd %f\n", tools_time_to_str(tools_get_sd_from_so(nodedata->sframe.SO), msg, 100));				
				tools_exit(2, c, "[ERROR] %d is out of the active part of the superframe\n", c->node);
			}
			
            //to find the backoff boundaries: extract the nb of slots and find if we are closed to one slot
			uint64_t	backoff_boundary = beacon_get_time_start_sframe(c);
			while (backoff_boundary < now)
				backoff_boundary += aUnitBackoffPeriod;
			
			
			//------- TIME REQUIRED FOR THE WHOLE TRANSMISSION -----
			
			//timeout befor we consider the transmission failed
			uint64_t	time_to_wait_response = 0;
			switch (pk_get_ftype(nodedata->pk_tx_up_pending->packet)){
				case MAC_DATA_PACKET :
				case MAC_HELLO_PACKET :
					time_to_wait_response = macAckWaitDuration;	
					break;
				
				case MAC_COMMAND_PACKET:
					;
					_802_15_4_COMMAND_header	*header_cmd = (_802_15_4_COMMAND_header*) (nodedata->pk_tx_up_pending->packet->data + sizeof(_802_15_4_header));
					
					if (header_cmd->type_command == DATA_REQUEST)
                             time_to_wait_response = macMaxFrameTotalWaitTime + macAckWaitDuration + aTurnaroundTime;
					//p160 of the standard (ack , then turnarroundtime, and then the data packet)
					else
						time_to_wait_response = macAckWaitDuration;

					break;
					
				default:
					tools_exit(2, c, "[ERROR] this packet type is not treated here\n");
					break;			
			}
            
			
			//time for the whole transmission
			uint64_t	time_for_exchange		= backoff_boundary + backoff + 2 * aUnitBackoffPeriod +	nodedata->pk_tx_up_pending->packet->real_size * radio_get_Tb(&c_radio) + time_to_wait_response;
		
            
           
         /*   tools_debug_print(c, "backoff_boundary %s\n", tools_time_to_str(backoff_boundary, msg, 150));
            tools_debug_print(c, "backoff %s\n", tools_time_to_str(backoff, msg, 150));
            tools_debug_print(c, "2 * aUnitBackoffPeriod %s\n", tools_time_to_str(2 * aUnitBackoffPeriod, msg, 150));
            tools_debug_print(c, "nodedata->pk_tx_up_pending->packet->real_size * radio_get_Tb(&c_radio) %s\n", tools_time_to_str(nodedata->pk_tx_up_pending->packet->real_size * radio_get_Tb(&c_radio), msg, 150));
            tools_debug_print(c, "macAckWaitDuration %s\n", tools_time_to_str(macAckWaitDuration, msg, 150));
       */
            
			//printf("time to wait response : %s\n", tools_time_to_str(time_to_wait_response, msg2, 50) );
			
			//------ CANCEL TX ------
			
			//Our transmision is scheduled after the end of active period -> too late
			if (time_for_exchange >= beacon_get_time_start_sframe(c) + tools_get_sd_from_so(nodedata->sframe.SO)){
				tools_debug_print(c, "backoff canceled, it requires too much time (%s > %s)\n", \
								  tools_time_to_str(time_for_exchange, msg2, 50) , \
								  tools_time_to_str(beacon_get_time_start_sframe(c) + tools_get_sd_from_so(nodedata->sframe.SO), msg, 50));
				
				//this was an association-request -> consider the parent as not associated since the assoc-req failed during the superframe
				if (pk_get_ftype(nodedata->pk_tx_up_pending->packet) == MAC_COMMAND_PACKET){
					_802_15_4_COMMAND_header	*header_cmd	= (_802_15_4_COMMAND_header*) (nodedata->pk_tx_up_pending->packet->data + sizeof(_802_15_4_header));

					if (header_cmd->type_command == ASSOCIATION_REQUEST){						
						ctree_parent_print_debug_info(c);
						mgmt_parent_disassociated(c, nodedata->parent_addr_current);
						tools_debug_print(c, "[TX] The association-request failed (%d retx). I delete the packet and go to the sleeping state\n", nodedata->mac.NB);
					}	
				}
					
				
				//remove the current retransmission (a copy exists elsewhere in the buffer if its is required)
				if (nodedata->pk_tx_up_pending != NULL){
					tools_debug_print(c, "[TX] I remove the packet with cbrseq=%d because the backoff is too long to fit in the current active period (nb retransmissions %d)\n", pk_get_cbrseq(nodedata->pk_tx_up_pending->packet), nodedata->mac.nb_retry);
					
					//save the nb of retransmissions for this packet
					nodedata->pk_tx_up_pending->nb_retry = nodedata->mac.nb_retry + 1;
						
					pk_print_content(c, nodedata->pk_tx_up_pending->packet, stdout);
					
					//NB: we DON'T DEALLOCATE the memory associated to this packet!!! The memory is directly thus present in the buffer
					//a copy is only created when the packet has to be transmitted, here, we just manipulate a pointer to the corresponding packet
					buffer_change_pk_tx_up_pending(c, NULL);		
					
				}				
				nodedata->mac.NB = 0;				
					
				fsm_change(c, STATE_SLEEP, get_time());
				return(0);				
			}
			else{
				tools_debug_print(c, "[BACKOFF] end of tx expected at %s, end of the superframe at %s\n",\
								  tools_time_to_str(time_for_exchange, msg2, 50) ,\
								  tools_time_to_str(beacon_get_time_start_sframe(c) + tools_get_sd_from_so(nodedata->sframe.SO), msg, 50));
			}
			
											
			
			//------ TX of PACKET ------
			
			//the node has not to listen before checking the channel (backoff = sleeping time)
            //multicast-req to be transmitted -> the node CANNOT sleep
			if (pk_get_subtype(nodedata->pk_tx_up_pending->packet) != MULTICAST_REQUEST)
                radio_switch(c, RADIO_OFF);

  			//we have the time to possibly transmit the frame -> schedule the channel check after the corresponding backoff
			fsm_change(c, STATE_CHILD_CHECK_CHANNEL, backoff_boundary + backoff);			
			nodedata->mac.cw	= CW;

			tools_debug_print(c, "[BACKOFF] CCA scheduled at %s (backoff value %d, subtype %d)\n", tools_time_to_str(nodedata->fsm.clock, msg, 50), backoff_int, pk_get_subtype(nodedata->pk_tx_up_pending->packet));
			return(0);

		break;
			
		//Backoff is finished, we have to do several CCA before tx (until cw = 0)
		case  STATE_CHILD_CHECK_CHANNEL:  
			
			//switch the radio on before cheking the channel
			radio_switch(c, RADIO_RX);
            //tools_debug_print(c, "[CCA] medium busy: %d\n", radio_check_channel_busy(c));
			
			//the channel is free -> wait for the end of the CCA
			if(!radio_check_channel_busy(c)){

				fsm_change(c, STATE_CHILD_CHECK_CHANNEL_CCA_END, get_time() + CCAduration);
				return(0);
				
			//Chanel was busy => double the BE to choose a larger backoff
			}else 
				schedule_retx_if_possible(c);
			return(0);	
		break;
			
			
		//End of the CCA
		case  STATE_CHILD_CHECK_CHANNEL_CCA_END:  
			
            //tools_debug_print(c, "[CCA] medium busy: %d\n", radio_check_channel_busy(c));
            
			//the channel is free
			if(!radio_check_channel_busy(c)){
				(nodedata->mac.cw)--; 
				
				//we can now transmit our frame
				if(nodedata->mac.cw == 0){ 
					fsm_change(c, STATE_CHILD_TX, get_time() + aUnitBackoffPeriod - CCAduration);
					return(0);
				}
				//we have still to do (at least) another CCA after having slept for a time
				else{
					fsm_change(c, STATE_CHILD_CHECK_CHANNEL, get_time() + aUnitBackoffPeriod - CCAduration);
                    if (pk_get_subtype(nodedata->pk_tx_up_pending->packet) != MULTICAST_REQUEST)
                        radio_switch(c, RADIO_OFF);
					return(0);
				}
				
			//Channel was busy => double the BE to choose a larger backoff
			}else 
				schedule_retx_if_possible(c);
			return(0);	
			
			break;
			
			
			
		//We have now to transmit the packet
		case STATE_CHILD_TX:
			;
                  
			//The UP buffer MUST be non null
				if (nodedata->pk_tx_up_pending == NULL){
				tools_debug_print(c, "[ERROR]: the PK UP is empty\n");
				exit(5);
			}            
            
            //I MUST be associated to a parent if this is not an MULTICAST_REQUEST
            if ((pinfo_current == NULL) && (pk_get_subtype(nodedata->pk_tx_up_pending->packet) != MULTICAST_REQUEST))
                tools_exit(3, c, "%d is a child but the current parent %d cannot be found (invalid parent = %d)\n", c->node, nodedata->parent_addr_current, nodedata->parent_addr_current == ADDR_INVALID_16);
  
	
			//we copy the packet (for possible retransmissions)
			packet_t *packet_to_send = packet_clone(nodedata->pk_tx_up_pending->packet);
			tx_time = packet_to_send->real_size * radio_get_Tb(&c_radio);
			
			//time between two frames
			uint64_t	ifs_time;
			if (packet_to_send->real_size > aMaxSIFSFrameSize * 8)
				ifs_time = macMinLIFSPeriod;
			else
				ifs_time = macMinSIFSPeriod;			

			//There is never pending frames for our parent!
			pk_set_P(packet_to_send, FALSE);			
		
			//pk_print_content(c, packet_to_send, stdout);
			
			switch(pk_get_ftype(packet_to_send)){
				//DATA FRAME -> we have to wait for the ack
				case MAC_DATA_PACKET :
				case MAC_HELLO_PACKET :
					;
					//_802_15_4_DATA_header	*header_data = (_802_15_4_DATA_header*) (packet_to_send->data + sizeof(_802_15_4_header));
				
					//an ack is required
					if (pk_get_A(packet_to_send)){
						nodedata->last_seq_num_to_ack = pk_get_seqnum(packet_to_send);
						
						fsm_change(c, STATE_CHILD_WAIT_ACK, get_time() + macAckWaitDuration + tx_time);
						tools_debug_print(c, "[DATA] ack is required before %s\n", tools_time_to_str(get_time() + macAckWaitDuration + tx_time, msg, 50));
					}
					//or not -> go to the idle state as soon as the transmission is finished
					else{
						fsm_change(c, STATE_CHILD_IDLE, get_time() + tx_time + ifs_time);
                        tools_debug_print(c, "[DATA] no ack is required, I come back to the idle state at %s\n", tools_time_to_str(get_time() + tx_time + ifs_time, msg, 50));
						
						//MAC reinitizalition for next packets
						nodedata->mac.NB = 0;
						
						
						//either the packet is removed from the buffer (and freed)
						dequeue_desalloc_tx_up_pending_packet(c);

					}
					break;
					
			
				//COMMAND FRAME
				case MAC_COMMAND_PACKET:
					;
					_802_15_4_COMMAND_header	*header_cmd = (_802_15_4_COMMAND_header*) (packet_to_send->data + sizeof(_802_15_4_header));
					
					//data sequence number (all command packets are acknowledged)
					nodedata->last_seq_num_to_ack = pk_get_seqnum(packet_to_send);

					//we transmitted a data_request -> wait for the ack (and then the data)
					if (header_cmd->type_command == DATA_REQUEST){
                        
                        //waiting time = data-req transmission + mactACK + DATA transmission
                        uint64_t   time_state_change;
                        if (get_time() + macMaxFrameTotalWaitTime  < beacon_get_time_start_sframe(c) + tools_get_sd_from_so(nodedata->sframe.SO)) 
                            time_state_change = get_time() + macMaxFrameTotalWaitTime;
                        else
                            time_state_change = beacon_get_time_start_sframe(c) + tools_get_sd_from_so(nodedata->sframe.SO);
                        
   						//I am waiting either for a data packet						
						if (pinfo_current->associated){                           
                            fsm_change(c, STATE_CHILD_WAIT_ACK, time_state_change);
							tools_debug_print(c, "[DATA-REQ] release (seqnum %d), wait for a data frame before %s\n", pk_get_seqnum(packet_to_send), tools_time_to_str(nodedata->fsm.clock, msg, 150));	                            
 
						}
						//or for an association reply
						else{
                            //waiting time = data-req transmission + mactACK + DATA transmission
							fsm_change(c, STATE_UNASSOC_WAIT_ASSOC_REP, time_state_change);
                            tools_debug_print(c, "[DATA-REQ] release (seqnum %d), wait for an association reply before %s (pk pending %d)\n", pk_get_seqnum(packet_to_send), tools_time_to_str(nodedata->fsm.clock, msg, 150), nodedata->pk_tx_up_pending);
                        }						
						
					}
					//We transmitted an association request -> wait for the ack
					else if (header_cmd->type_command == ASSOCIATION_REQUEST){
						
						//I must now wait the ack and then the association reply
						fsm_change(c, STATE_UNASSOC_WAIT_ACK_ASSOC_REQ, get_time() + macAckWaitDuration + tx_time);
						
					}
                    
					//I must now wait the ack for this DISASSOCIATION
					else if (header_cmd->type_command == DISASSOCIATION_NOTIFICATION){
						
						fsm_change(c, STATE_CHILD_WAIT_ACK, get_time() + macAckWaitDuration + tx_time);
					}
                    
                    //MULTICAST_REQUEST transmission -> wait for the response (come back in the IDLE state as soon as the frame is transmitted)
                    else if (header_cmd->type_command == MULTICAST_REQUEST){
                        
                        tools_debug_print(c, "[MULTICAST-REQ] no ack is required, I come back to the idle state at %s\n", tools_time_to_str(get_time() + tx_time + ifs_time, msg, 50));
                            
                        //MAC reinitizalition for next packets
                        nodedata->mac.NB = 0;
                            
                        // memory release 
                        dequeue_desalloc_tx_up_pending_packet(c);
                        
                        //come back to the idle state
                        fsm_change(c, STATE_CHILD_IDLE, get_time() + tx_time);
         
                    }
					else
						tools_exit(5, c, "this subtype command (%d) is not yet implemented\n", header_cmd->type_command);
					

					break;			
			
				default:
					tools_exit(4, c, "Unknown packet type for transmission (%d)\n", pk_get_ftype(packet_to_send));
					break;
			}
			
			//transmission			
			radio_pk_tx(c, packet_to_send); 
			
			
			return 0;
		break;
		
			
		//the ack did not come! We have to either retransmit the packet or drop it
		case STATE_CHILD_WAIT_ACK:
			
			//The UP buffer MUST be non null
			if (nodedata->pk_tx_up_pending == NULL){
				tools_debug_print(c, "[ERROR]: the PK UP is empty\n");
				exit(5);
			}
			
			//a command frame -> the packet will be regenerated from the IDLE state
			if (pk_get_ftype(nodedata->pk_tx_up_pending->packet) == MAC_COMMAND_PACKET){
				
				tools_debug_print(c, "[ACK-MISS] I did not receive an ack for a command frame\n");
				_802_15_4_COMMAND_header	*header_cmd_buf = (_802_15_4_COMMAND_header*) (nodedata->pk_tx_up_pending->packet->data + sizeof(_802_15_4_header));
				int	new_state;
				
				
				//this was a disassociation -> I drop the corresponding parent and go sleeping
				if (header_cmd_buf->type_command == DISASSOCIATION_NOTIFICATION){		
					tools_debug_print(c, "[ASSOC] my disassociation was not acked. We remove anyway this parent\n");
					mgmt_parent_disassociated(c, header_cmd_buf->dst);
					new_state = STATE_SLEEP;
				}	
				//ELse, I am going back to the idle state since I keep ion being associated to this superframe
				else
					new_state = STATE_CHILD_IDLE;
				
				
				//packet dropped
				dequeue_desalloc_tx_up_pending_packet(c);						
		
				
				//MAC reinitialization
				nodedata->mac.nb_retry	= 0;
				nodedata->mac.NB		= 0;
				
				//triggers the state change after everything has been reinitialiazed
				fsm_change(c, new_state, get_time());
			
			}
			//too many retransmissions (DATA pkt)
			else if ((++nodedata->mac.nb_retry) > macMaxFrameRetries) {
				tools_debug_print(c, "\n\n[ACK-MISS] too many retransmissions, NB=%d, BE=%d (cbrseq=%d)\n\n\n", nodedata->mac.NB, nodedata->mac.BE, pk_get_cbrseq(nodedata->pk_tx_up_pending->packet));				
				printf("\t\t[ACK-MISS] Node %d too many retransmissions, NB=%d, BE=%d \n\n\n", c->node, nodedata->mac.NB, nodedata->mac.BE);

				//oana for elt_etx stats
				data_drop_packet(c, nodedata->pk_tx_up_pending->packet);
				
				//update the stats for this dropped packet 
				stats_data_update_drop_reason(c, nodedata->pk_tx_up_pending->packet, DROP_RETX);
				
				//packet dropped
				dequeue_desalloc_tx_up_pending_packet(c);					
				
				
				//MAC reinitialization
				nodedata->mac.nb_retry	= 0;
				nodedata->mac.NB		= 0;
				
				//FSM
				fsm_change(c, STATE_CHILD_IDLE, get_time());
				return(0);			
			}
			
			//retransmission
			else {						
				//reinitialization for the next transmission
				nodedata->mac.cw = 2;
				nodedata->mac.NB ++;
				nodedata->mac.BE = tools_min((++nodedata->mac.BE), macMaxBE);
				
				//FSM
				fsm_change(c, STATE_CHILD_BACKOFF, get_time());  
				
				
				tools_debug_print(c, "[ACK-MISS] the ack was not received, NB=%d, BE=%d\n", nodedata->mac.NB, nodedata->mac.BE);				
				return(0);
			}
		break;
			
		//the node has received an ack for its data-req and must now wait for the data frame which will follow
		case STATE_CHILD_WAIT_REP:
			
			tools_debug_print(c, "[DATA-MISS] the data frame was not received after our command frame was acknowledged\n");				
			fsm_change(c, STATE_CHILD_IDLE, get_time());

			break;
			
//-------------------------------------------------------------------------------
//      BEACON-ONLY PERIOD MODE
//-------------------------------------------------------------------------------

            
        //I must wait the medium becomes free and then trigger a backoff
        case STATE_BOP_WAIT_FREE:
            ;
            short notif;
            
            //I am now active during the BOP
            radio_switch(c, RADIO_RX);
         
            //free medium -> start the backoff 
            if (!radio_check_channel_busy(c)){
                
                //desactivate the notification from the radio module
                notif = FALSE;
                ioctl(&c_radio, RADIO_IOCTL_NOTIFFREE_SET, (void*)&notif, NULL);

                //coordinators 
                if (nodedata->sframe.my_current_role == ROLE_COORDINATOR){                
                    
                    fsm_change(c, STATE_BOP_BACKOFF, get_time() + nodedata->sframe.my_bop_slot * aUnitBackoffPeriod);
                    tools_debug_print(c, "[BOP] I am coordinator and must wait for %d aUnitBackoffPeriod before transmitting my beacon\n", nodedata->sframe.my_bop_slot);
                }
                
                //CHILD mode
                else{
                    fsm_change(c, STATE_BOP_BACKOFF, get_time() + param_get_bop_nb_slots() * aUnitBackoffPeriod);
                    tools_debug_print(c, "[BOP] I am a child and I must wait for %d aUnitBackoffPeriod (the max) before entering the CAP\n", param_get_bop_nb_slots());
                }                
 
            } 
            //else -> wait the radio becomes idle again
            else{
                notif = TRUE;
                ioctl(&c_radio, RADIO_IOCTL_NOTIFFREE_SET, (void*)&notif, NULL);
                fsm_change(c, STATE_BOP_BACKOFF, get_time() + param_get_bop_nb_slots() * aUnitBackoffPeriod);
                
                tools_debug_print(c, "busy\n");
            }           
           
            
            break;

        
        //that's the end and the medium is free?  
        //let's start either the CAP (child) or transmit the beacon (coordinator)
        case STATE_BOP_BACKOFF:
            
                 
            //busy -> come back to the waiting state
            if (radio_check_channel_busy(c)){
            /*    if (nodedata->sframe.my_current_role == ROLE_COORDINATOR)
                    tools_debug_print(c, "[BOP] the medium is busy during the BOP. We have to wait for %d aUnitBackoffPeriod (the max) before entering the CAP\n", nodedata->sframe.my_bop_slot);
                else
                    tools_debug_print(c, "[BOP] the medium is busy during the BOP. We have to wait for %d aUnitBackoffPeriod (the max) before entering the CAP\n", param_get_bop_nb_slots());             
             */
                fsm_change(c, STATE_BOP_WAIT_FREE, get_time());
            }
    
            //coordinator -> send the beacon
            else if (nodedata->sframe.my_current_role == ROLE_COORDINATOR){
                tools_debug_print(c, "[BOP] I am a coordinator and should now send my beacon\n");             

                //cancel the previous event
                if (nodedata->sframe.event_beacon_stop != NULL)
                    scheduler_delete_callback(c, nodedata->sframe.event_beacon_stop);
                nodedata->sframe.event_beacon_stop = NULL;
                
                //control
                call_t          c_radio = {get_entity_bindings_down(c)->elts[0], c->node, c->entity};
                uint8_t         caddr, i;
                packet_info     *pk_elem;
                char            msg[100];
          
                
                //transmit the beacon
                packet_t    *beacon_pk = beacon_pk_create(c);                
                uint64_t    tx_time_next = get_time() + beacon_pk->real_size * radio_get_Tb(&c_radio) + macMinSIFSPeriod; 
                uint64_t    tx_time_pk = 0;
                radio_pk_tx(c, beacon_pk);            
                
                //transmit also the multicast packets buffered since the last beacon (BI)
                //for each multicast address 
                for(caddr=0; caddr<ADDR_MULTI_NB; caddr++)
                    
                    //creates the list of multicast seqnums (if the associated buffer is not empty, else we do not have to include this address in our beacons)
                    for(i=0; i<nodedata->buffer_multicast[caddr]->size; i++){
                         
                        //next packet in the multicast buffer
                        pk_elem = buffer_queue_get(nodedata->buffer_multicast[caddr], i);
                        
                        //this packet was probably generated after our last beacon
                        if (!pk_elem->broadcasted){
                            
                            tools_debug_print(c, "[MULTICAST] transmission of seqnum %d, cbrseq=%d, scheduled at %s\n", 
                                              pk_get_seqnum(pk_elem->packet), 
                                              pk_get_cbrseq(pk_elem->packet), 
                                              tools_time_to_str(tx_time_next, msg, 100));
                            
                            //expected transmission time
                            tx_time_pk = pk_elem->packet->real_size * radio_get_Tb(&c_radio) + macMinSIFSPeriod;
                            
                            // the whole transmission time does not exceed our fair part of the SD
                            if (tx_time_pk + tx_time_next - get_time() < tools_get_sd_from_so(nodedata->sframe.SO) / param_get_bop_nb_slots()){
                               
                                // we clone the packet to send an exemplary *now*, just after the beacon
                                packet_t    *pk_dup = packet_clone(pk_elem->packet);                                   
                                scheduler_add_callback(tx_time_next, c, radio_pk_tx, pk_dup);
                                pk_elem->broadcasted = TRUE;
                                
                                //next packet
                                tx_time_next += tx_time_pk;
                            }
                       }
                        
                }                   
                
                
                //idle state after having transmitted the beacon (that's safe: all my children MUST wait the CAP before transmitting packets to me)
                fsm_change(c, STATE_COORD_IDLE, tx_time_next);
            }
                
            //"non coordinator mode" 
            else if (nodedata->sframe.my_current_role == ROLE_SCAN){
                
                //cancel the verification of beacon ending
                if (nodedata->sframe.event_beacon_stop != NULL)
                    scheduler_delete_callback(c, nodedata->sframe.event_beacon_stop);
                nodedata->sframe.event_beacon_stop = NULL;
                
                //flush pending packets (association-req, diassociation-notif, etc.)
                if (nodedata->pk_tx_up_pending != NULL)
                    dequeue_desalloc_tx_up_pending_packet(c);

                
                tools_debug_print(c, "[BOP] the BOP is finished, we are a neither child nor coordinator and we go sleeping (busy=%d)\n", radio_check_channel_busy(c));                
                fsm_change(c, STATE_SLEEP, get_time());
            }
            
            //child -> enter in CAP
            else if (nodedata->sframe.my_current_role == ROLE_CHILD){
                //cancel the previous event
                if (nodedata->sframe.event_beacon_stop != NULL)
                    scheduler_delete_callback(c, nodedata->sframe.event_beacon_stop);
                nodedata->sframe.event_beacon_stop = NULL;

                
                tools_debug_print(c, "[BOP] the BOP is finished, we are a child with parent %d and we go to the CAP\n", nodedata->parent_addr_current);                
                fsm_change(c, STATE_CHILD_IDLE, get_time());
            }
            
            
            break;
            
            
             
//-------------------------------------------------------------------------------
//        BUG
//-------------------------------------------------------------------------------
			
		default:
			tools_exit(3, c, "ERROR, this state (%d) does not exist in the state machine\n", (int)nodedata->fsm.state); 
		break;
	}
	return(0);
} 







//----------------------------------------------
//		PK RECEPTION -> DISPATCHER
//----------------------------------------------



//I am receiving a packet from the PHY layer
void rx(call_t *c, packet_t *packet) {
	nodedata_info	*nodedata = get_node_private_data(c);
	call_t c_radio = {get_entity_bindings_down(c)->elts[0], c->node, c->entity};
	char       msg[100];
    
	//debug
	//pk_print_content(c, packet, stdout);
	
	//non-implemented features
	if (pk_get_S(packet))
		tools_exit(5, c, "ERROR: I don't implement the security headers of IEEE 802.15.4. Thus, the S bit has to be set to 0\n");

	if (!pk_get_C(packet))
		tools_exit(6, c, "ERROR: I don't implement the PAN identifiers in the headers of IEEE 802.15.4. Thus, the C bit has to be set to 1\n");


	
	//statistics (verifies bounds)
	if (nodedata->sfslot_table[tools_compute_sfslot(get_time())].nb_pk_rx < pow(2, sizeof(int) * 8))
		nodedata->sfslot_table[tools_compute_sfslot(get_time())].nb_pk_rx ++;
	
	//stats update
	stats_pk_rx(packet);
	
	
	//I was waiting for this reply -> unschedule the corresponding timeout
	if (nodedata->fsm.state == STATE_CHILD_WAIT_REP){
		
		//if a ack is required -> we come back to the idle state after the ack transmission
		uint64_t time_idle;
		if (pk_get_A(packet))
			 time_idle = get_time() + aTurnaroundTime + mgmt_ack_get_size() * 8 * radio_get_Tb(&c_radio) + mactACK;
		
		//else, immediately
		else
			time_idle = get_time();
		
		tools_debug_print(c, "[DATA] the frame we were waiting was finally received -> we unschedule the corresponding timeout\n");				
		fsm_change(c, STATE_CHILD_IDLE, time_idle);
	}	
    
    //packet info
    uint16_t    dst_short, src_short   = pk_get_src_short(packet);
    uint8_t     ftype       = pk_get_ftype(packet);
    if (ftype == MAC_HELLO_PACKET || ftype == MAC_DATA_PACKET || ftype == MAC_COMMAND_PACKET)
        dst_short = pk_get_dst_short(packet);
    else
        dst_short = ADDR_INVALID_16; 
    
    
    //maintains the list of already received seqnums for multicast packets
    if (dst_short >= ADDR_MULTI_MIN)
        mgmt_seqnum_rcvd(c, packet);
    
    //and then, it depends on the packet type
	switch (ftype){
		
		//BEACON RECEPTION
		case MAC_BEACON_PACKET:
			;
			//get the beacon header
			_802_15_4_Beacon_header *header_beacon = (_802_15_4_Beacon_header*) (packet->data + sizeof(_802_15_4_header));
			tools_debug_print(c, "[MAC_RX_RAW] type = %s. Src = %d. Node state = %s, P = %d\n", pk_ftype_to_str(pk_get_ftype(packet)), header_beacon->src, tools_fsm_state_to_str(nodedata->fsm.state), pk_get_P(packet));

			//I am not the transmitter of this beacon -> treat it
			if(header_beacon->src != nodedata->addr_short)
				beacon_rx(c, packet);
			
			//memory
			//oana: must be a beacon
			packet_dealloc(packet);

		break;
			
			
		//ACK PROCESSS
		case MAC_ACK_PACKET:
			tools_debug_print(c, "[MAC_RX_RAW] type = %s. Node state = %s, PF = %d\n", pk_ftype_to_str(pk_get_ftype(packet)), tools_fsm_state_to_str(nodedata->fsm.state), pk_get_P(packet));

			//We wait for an ACK, packet is for us and from the node that we expect
			if (nodedata->fsm.state == STATE_CHILD_WAIT_ACK || nodedata->fsm.state == STATE_COORD_WAIT_ACK || nodedata->fsm.state == STATE_UNASSOC_WAIT_ACK_ASSOC_REQ || nodedata->fsm.state == STATE_UNASSOC_WAIT_ASSOC_REP)
				mgmt_ack_rx(c, packet);

			//memory
			//oana: must be an ack 
			packet_dealloc(packet);
		break;
		
		
		//THIS IS A DATA PACKET
		case MAC_DATA_PACKET: 
			;
			//get the data header
			_802_15_4_DATA_header *header_data = (_802_15_4_DATA_header*) (packet->data + sizeof(_802_15_4_header));
			
			//neighborhood table
			tools_debug_print(c, "[MAC_RX_RAW] type = %s. Src = %d. Dst = %d. Node state = %s, PF = %d, Ack = %d\n", 
                              pk_ftype_to_str(pk_get_ftype(packet)), 
                              pk_get_src_short(packet), 
                              header_data->dst, 
                              tools_fsm_state_to_str(nodedata->fsm.state), 
                              pk_get_P(packet),
                              pk_get_A(packet));
		
			//Unicast Data + Multicast (from my parent)
			if ((header_data->dst == nodedata->addr_short && nodedata->mac.tx_end <= get_time()) || (tools_is_addr_multicast(header_data->dst))){
                
                //the source is my parent: has it pending packets for me? 
				if (ctree_parent_get_info(nodedata->parents, pk_get_src_short(packet)) != NULL)
                    //nodedata->sframe.my_sf_slot != tools_compute_sfslot(get_time()))
                    ctree_parent_change_notif(c, pk_get_src_short(packet), pk_get_P(packet));			

                //route the data packet
                data_rx(c, packet);
			}
  			//memory
            else{  
				//oana: this is a data packet, most surely not for me		
                packet_dealloc(packet);            
            }
			break;
			
			
		//MANAGEMENT FRAME
		case MAC_COMMAND_PACKET:
			tools_debug_print(c, "[MAC_RX_RAW] type = %s. Node state = %s. PF = %d\n", pk_ftype_to_str(pk_get_ftype(packet)), tools_fsm_state_to_str(nodedata->fsm.state), pk_get_P(packet));
			
			//commands are in unicast -> I will filter directly in the function (depends on the long/short address)            
            if (nodedata->mac.tx_end <= get_time())
                 mgmt_cmd_rx(c, packet);
            else
                tools_debug_print(c, "[RX] I am busy until %s. I cannot handle this command frame\n", 
                                  tools_time_to_str(nodedata->mac.tx_end, msg, 100));
			
			//memory
			//oana: not data packet
			packet_dealloc(packet);
		break;
	
		//HELLO
		case MAC_HELLO_PACKET:
			tools_debug_print(c, "[MAC_RX_RAW] type = %s. Node state = %s. PF = %d\n", pk_ftype_to_str(pk_get_ftype(packet)), tools_fsm_state_to_str(nodedata->fsm.state), pk_get_P(packet));

			//hellos are in multicast
			neigh_hello_rx(c, packet);
			
			//memory
			//oana: not data packet
			packet_dealloc(packet);
		break;
			
		//ERROR
		default:
			tools_exit(7, c, "Unkwnown packet type: %d\n", pk_get_ftype(packet));
		break;		
	}
    
    //last reception from this neighbor (except for ack packets -> we do not have any idea which is the transmitter)
    if ((ftype != MAC_ACK_PACKET) && (src_short != ADDR_INVALID_16))
        neigh_table_update_last_rx_for_addr(c, src_short);

    
    //I am during the BOP but I probably missed the beacon if I receive such packets
    if ((ftype == MAC_COMMAND_PACKET) || (ftype == MAC_ACK_PACKET) || (dst_short < ADDR_MULTI_MIN && dst_short != ADDR_INVALID_16)){
        
        switch (nodedata->fsm.state){
                
            //the BOP period is finished -> children will enter in the CAP, coordinator will send their beacon
            //best-effort since we had a big problem
            case STATE_BOP_WAIT_FREE :                     
                tools_debug_print(c, "[BOP] the BOP period has probably finished: such packet CANNOT be received during the BOP\n");
 
                if (nodedata->sframe.my_current_role == ROLE_COORDINATOR)
                    state_machine(c, NULL);
               
                else if (nodedata->sframe.my_current_role == ROLE_CHILD)
                    fsm_change(c, STATE_CHILD_IDLE, get_time());
                
                else                     
                    //fsm_change(c, STATE_CHILD_IDLE, get_time());
                    fsm_change(c, STATE_SLEEP, get_time());
               
                break; 
                
                
            //stop scanning this slot
            case STATE_CHILD_STOP_WAIT_BEACON:
                tools_debug_print(c, "[BOP] the BOP period has probably finished: such packet CANNOT be received during the BOP\n");
                
                state_machine(c, NULL);
                break;
                
                     
        }
    }
    
  
	return;     
}





//----------------------------------------------
//			RX FROM UPPER LAYER
//----------------------------------------------


void tx(call_t *c, packet_t *packet) {
    nodedata_info *nodedata = get_node_private_data(c);
  
	
	//fill some special fields in the header (NB: facultative)
	_802_15_4_DATA_header	*header_data	= (_802_15_4_DATA_header *) (packet->data + sizeof(_802_15_4_header));
	cbrv3_packet_header		*header_cbr		= (cbrv3_packet_header *) (packet->data + sizeof(_802_15_4_header) + sizeof(_802_15_4_DATA_header));
	
    
    //debug
    tools_debug_print(c, "[MAC_DATA] generates a new packet toward %d (seqnum=%lu)\n", header_data->dst_end2end, header_cbr->sequence);	
    
	//frame type
	pk_set_ftype(packet, MAC_DATA_PACKET);	
	
	
	//PACKET ERROR: we are not yet associated
    if (nodedata->addr_short == ADDR_INVALID_16){
		tools_debug_print(c, "[MAC_DATA] I am not yet associated, I MUST drop the data packet (cbrseq=%d)\n", header_cbr->sequence);	
	}
	
    //multicast / broadcast -> no ack
    if (header_data->dst_end2end >= ADDR_MULTI_MIN)
        pk_set_A(packet, FALSE);
    
    //unicast data packets are always acked
    else
        pk_set_A(packet, TRUE);
    
	//stats
	stat_cbr_info *stat_elem = (stat_cbr_info*) malloc(sizeof(stat_cbr_info));
	stat_elem->src				= c->node;
	stat_elem->dest				= header_data->dst_end2end;
	stat_elem->time_generated	= get_time();
	stat_elem->time_received	= 0;
	stat_elem->sequence			= header_cbr->sequence;
	stat_elem->route			= das_create();
	stat_elem->neigh_radio		= das_create();
	stat_elem->neigh_ctree		= das_create();
	stat_elem->neigh_rcvd		= das_create();
	stat_elem->drop_reason		= DROP_NO;
	das_insert(global_cbr_stats, (void*)stat_elem);
	
    // tools_debug_print(c, "[STATS] ptr creation : %ld, das %ld\n", (long int)stat_elem, (long int)stat_elem->neigh_ctree);
	
	//emulate the reception of this packet to re-use the same function
	//(i.e. find the correct queue and next hop)
	data_route(c, packet);
}






//----------------------------------------------
//				SET HEADER
//----------------------------------------------


//When I receive one packet from the uppper layer
int set_header(call_t *c, packet_t *packet, destination_t *dst) {
	nodedata_info *nodedata = get_node_private_data(c);
 	
	_802_15_4_DATA_header *header_data = (_802_15_4_DATA_header *) (packet->data + sizeof(_802_15_4_header));
	
	header_data->src_end2end	= nodedata->addr_short;
    header_data->src            = nodedata->addr_short;
	header_data->dst_end2end	= dst->id;
	header_data->dst			= dst->id;
		
	return 0;
}






//----------------------------------------------
//				GET HEADER
//----------------------------------------------



// the header size (returned for higher layers)
// Thus, I only return the size for the data frames 
// (other frames are not forwaded to the upper layers)


//in bytes
int get_header_size(call_t *c) {
	return (sizeof(_802_15_4_header) + sizeof(_802_15_4_DATA_header));
}

//in bits
int get_header_real_size(call_t *c) {
	return (8 * (sizeof(_802_15_4_header) + sizeof(_802_15_4_DATA_header)));
}






//----------------------------------------------
//				PUBLIC METHODS
//----------------------------------------------


mac_methods_t methods = {rx, 
	tx,
	set_header, 
	get_header_size,
	get_header_real_size
};






