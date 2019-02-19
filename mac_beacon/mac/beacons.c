/*
 *  beacons.c
 *  
 *
 *  Created by Fabrice Theoleyre on 11/02/11.
 *  Copyright 2011 CNRS. All rights reserved.
 *
 */

#include "beacons.h"




//-------------------------------------------
//			BEACON SCHEDULING 
//-------------------------------------------

//returns the time of the beginning of the active part of the current superframe
uint64_t beacon_get_time_start_sframe(call_t *c){
	nodedata_info	*nodedata	= (nodedata_info*)get_node_private_data(c);

	//info on the current active parent
    neigh_info      *neigh_elem = neigh_table_get(c, nodedata->parent_addr_current);
    
    //error
    if ((neigh_elem == NULL) || (neigh_elem->sf_slot == SF_SLOT_INVALID))
        tools_exit(13, c, "The parent %d does not exist: we cannot delimit the superframe\n", nodedata->parent_addr_current);
    
    //the SD and BI values (in uint64_t)
    uint64_t	SD = tools_get_sd_from_so(nodedata->sframe.SO);
    uint64_t	BI = tools_get_bi_from_bo(nodedata->sframe.BO);
    uint64_t    now = get_time();
    
    uint64_t	time_beacon = neigh_elem->sf_slot * tools_get_sd_from_so(nodedata->sframe.SO);			
    while (((uint64_t)(now - time_beacon) >= SD) && (time_beacon < now)){
        //	printf("now %llu time beacon %llu, sd %llu\n", (long unsigned int)now , (long unsigned int)time_beacon, (long unsigned int)SD);
        time_beacon += BI;
    }
    return(time_beacon);
}



//the time for the corresponding next superframe slot
uint64_t beacon_compute_next_sf(uint8_t sf_slot){
	uint64_t	time_sf;
	uint64_t	now = get_time();
	
	time_sf = sf_slot * tools_get_sd_from_so(param_get_global_so());
	while(time_sf < now)
		time_sf += tools_get_bi_from_bo(param_get_global_bo());				

	return(time_sf);
}



//reschedule my beacon -- ONLY FOR ALREADY ASSSOCIATED NODES!
void beacon_reschedule_my_beacon_after_rx(call_t *c){
	nodedata_info	*nodedata	= (nodedata_info*)get_node_private_data(c);
	char	msg[50];
				
	//my superframe (normalized value between [0..BI]
	uint64_t	my_sframe_time	= nodedata->sframe.my_sf_slot * tools_get_sd_from_so(nodedata->sframe.SO);
	while(my_sframe_time < get_time())
		my_sframe_time += tools_get_bi_from_bo(nodedata->sframe.BO);			
	
	//cancel the previous interrupt if one exists
	if (nodedata->sframe.event_beacon_callback != NULL)
		scheduler_delete_callback(c, nodedata->sframe.event_beacon_callback);
	
	nodedata->sframe.event_beacon_callback = scheduler_add_callback(my_sframe_time, c, beacon_callback, NULL);
	tools_debug_print(c, "[COORD] next beacon rescheduled at %s (bop slot %d)\n", tools_time_to_str(my_sframe_time, msg, 50), nodedata->sframe.my_bop_slot);
	
	
/*	printf("beacon parent %f\n", (double)(get_time() - tx_time - pk_beacon_get_bop_slot(packet) * pk_get_bop_slot_duration(c)) / 1e9);
	printf("my beacon time %f\n", (double)(my_sframe_time) / 1e9);
	printf("bop slot %d sf slot %d\n", nodedata->sframe.my_bop_slot, nodedata->sframe.my_sf_slot);	
*/		
}


//-------------------------------------------
//		PENDING PACKETS in BEACONS
//-------------------------------------------


//does this parent has packet for me?
void beacon_update_pk_for_me(call_t *c, packet_t *packet){
	nodedata_info			*nodedata	= (nodedata_info*)get_node_private_data(c);
	_802_15_4_Beacon_header *header_beacon = (_802_15_4_Beacon_header *) (packet->data + sizeof (_802_15_4_header));
	
	//current parent
	parent_info *pinfo_current = ctree_parent_get_info(nodedata->parents, header_beacon->src);
	
	//control
	int			i;
	
	//initialization
	pinfo_current->positive_notif = get_time();

	//searches for my short address in pending destinations
	uint16_t	*hdr_dests_short = (uint16_t*) (packet->data + sizeof(_802_15_4_header) + sizeof(_802_15_4_Beacon_header));
	for(i=0; i<pk_beacon_get_nb_pending_short_addr(packet); i++){
		if (hdr_dests_short[i] == nodedata->addr_short)
			pinfo_current->positive_notif = get_time() + tools_get_sd_from_so(nodedata->sframe.SO);
            //pinfo_current->positive_notif = get_time() + macMaxFrameTotalWaitTime;
	}
	//long addresses are put just after the short addresses
	uint64_t	*hdr_dests_long = (uint64_t*) (packet->data +\
											   sizeof(_802_15_4_header) +\
											   sizeof(_802_15_4_Beacon_header) +\
											   pk_beacon_get_nb_pending_short_addr(packet) * sizeof(uint16_t));
	for(i=0; i<pk_beacon_get_nb_pending_long_addr(packet); i++){
		if (hdr_dests_long[i] == nodedata->addr_long)
			pinfo_current->positive_notif = get_time() + macMaxFrameTotalWaitTime;			
	}
	
	//debug
	tools_debug_print(c, "[BEACON] %d has unicast packets for me? %d (positiv notif until %lu)\n", header_beacon->src, pinfo_current->positive_notif > get_time(), pinfo_current->positive_notif);
}




//-------------------------------------------
//		BEACON RECEPTION
//-------------------------------------------




//I received one beacon
void beacon_rx(call_t *c, packet_t *packet) {
	nodedata_info	*nodedata	= (nodedata_info*)get_node_private_data(c);
	call_t c_radio = {get_entity_bindings_down(c)->elts[0], c->node, c->entity};
	uint64_t	tx_time = packet->real_size * radio_get_Tb(&c_radio);
	char		msg[100];
	
	//the header of the packet
	_802_15_4_Beacon_header *header_beacon = (_802_15_4_Beacon_header *) (packet->data + sizeof (_802_15_4_header));
	
	//802.15.4 parameters are the same for all the nodes
	nodedata->sframe.BO	 = pk_beacon_get_BO(packet);
	nodedata->sframe.SO	 = pk_beacon_get_SO(packet);	
	
	//update the neighborhood table
	neigh_table_add_after_beacon(c, packet);
  	
	//Is this already a parent? 
	parent_info     *pinfo_current  = ctree_parent_get_info(nodedata->parents, header_beacon->src);
    neigh_info      *neigh_elem     = neigh_table_get(c, header_beacon->src);

	//debug
	ctree_parent_print_debug_info(c);

	//the currrent sf slot
	int		sf_slot_current = (get_time() % tools_get_bi_from_bo(nodedata->sframe.BO)) / tools_get_sd_from_so(nodedata->sframe.SO);
	

	
	
	//---------------- NEW PARENT SELECTION / NEIGH UPDATE ---------
	
	//data part of the superframe
    uint64_t time_data_part_sf = 0;
    switch (param_get_bop_algo()){
            
        //the data part begins after the BOP slots have finished
        case ALGO_BOP_ORIG :
            time_data_part_sf = get_time() - tx_time + pk_get_bop_slot_duration(c) * (param_get_bop_nb_slots() - pk_beacon_get_bop_slot(packet));
            
            //the beacon was too large to fit in the BOP slot -> automatic correction
            if (time_data_part_sf < get_time())
                time_data_part_sf = get_time();

            break;
            
        //the data part begins when the last beacon was received (idle time during at least MAX_BOP_SLOT * timeslot)
        case ALGO_BOP_BACKOFF:
                        
            time_data_part_sf = get_time();
        break;
            
        default:
            tools_exit(19, c, "This BOP algo is unkwnown (%d)\n", param_get_bop_algo());
            break;
    }
	
       
    
	//condition to choose this new parent
	short		not_parent              = (pinfo_current == NULL);
    short       one_of_the_best_depths  = (ctree_is_parent_to_add(c, header_beacon->src, pk_beacon_get_depth(packet))); // consider all parents (unassoc & associated)
    short		one_parent_per_sf       = (ctree_parent_any_sf_slot_not_used(c, pk_beacon_get_sf_slot(packet)));
	short		parent_change_sf        = (sf_slot_current != pk_beacon_get_sf_slot(packet));
	short       not_coordinator         = (sf_slot_current != nodedata->sframe.my_sf_slot);
	short       sufficient_quality      = 1; //(tools_distance(c->node, header_beacon->src) <= RADIO_RANGE);
	
	short		parent_to_choose    = not_parent && one_of_the_best_depths && one_parent_per_sf && !parent_change_sf && not_coordinator && sufficient_quality;
	
	//if this parent announces it changed its superframe slot, we cannot choose it as parent!!!
	if (pk_beacon_get_sf_slot(packet) != tools_compute_sfslot(get_time())){
		parent_to_choose = FALSE;
		tools_debug_print(c, "[CTREE] this coordinator has just changed its superframe slot and does not maintain this superframe (current sflot %d, announced %d). We cannot choose it as parent\n", tools_compute_sfslot(get_time()), pk_beacon_get_sf_slot(packet));
	}
    
   /* tools_debug_print(c, "[CTREE] %d -> new parent %d = not parent %d && one of the best depths %d && one_parent_per_sf %d && !parent_change_sf %d && not_coordinator %d && quality %d\n",
                      header_beacon->src,
                      parent_to_choose,
                      not_parent,
                      one_of_the_best_depths,
                      one_parent_per_sf,
                      !parent_change_sf,
                      not_coordinator,
                      sufficient_quality);
     ctree_parent_print_debug_info(c);  
    */
    
	//we add this new parent
	if (parent_to_choose){ 
		
		tools_debug_print(c, "[TREE] New parent %d (its depth %f, my depth %f, sfslot %d)\n", 
						  header_beacon->src, 
						  pk_beacon_get_depth(packet), 
						  ctree_compute_my_depth(c),
						  pk_beacon_get_sf_slot(packet));
			
		//current parent 
		nodedata->parent_addr_current = header_beacon->src;
        nodedata->sframe.my_current_role = ROLE_CHILD;
        
		//add this new (not yet associated) parent
		ctree_parent_add_after_beacon_rx(c, packet);				
		
		//sends an assoctiation request and waits for an ack
		nodedata->pk_tx_up_pending = mgmt_assoc_req_create(c, header_beacon->src);
        
        //My next state depends on the BOP algo
        switch (param_get_bop_algo()){
            case ALGO_BOP_ORIG :
                fsm_change(c, STATE_CHILD_BACKOFF, time_data_part_sf);
                break;
            case ALGO_BOP_BACKOFF:
                fsm_change(c, STATE_BOP_WAIT_FREE, get_time());                
                beacon_stop_waiting_schedule(c , beacon_get_time_start_sframe(c) + tools_get_sd_from_so(nodedata->sframe.SO));

                break;
            default:
                tools_exit(18, c, "bug in BOP algo\n");
        }            
		return;
	}
    
    //update the list of pending seqnum multicast packets for this node
    mgmt_seqnum_update_from_beacon(c, packet);

	
	//---------------- CURRENT PARENT ---------
	
	//this is one of our current parents
	if (pinfo_current != NULL){		

		//update the info on the tree
		pinfo_current->sf_slot		= pk_beacon_get_sf_slot(packet);
		pinfo_current->bop_slot		= pk_beacon_get_bop_slot(packet);
		pinfo_current->depth		= pk_beacon_get_depth(packet);
		
        //tools_debug_print(c, "[TREE] depth of %d is %f (my depth %f)\n", header_beacon->src, pinfo_current->depth, ctree_compute_my_depth(c));
        if (pinfo_current->sf_slot > pow(2, nodedata->sframe.BO - nodedata->sframe.SO))
            printf("ERROR, bad sfslot: %d (>%d)\n", pinfo_current->sf_slot, (int)pow(2, nodedata->sframe.BO - nodedata->sframe.SO));
        
        
		//pending packets for me?
		beacon_update_pk_for_me(c, packet);
 		
		//the beacon is just to announce it changed its superframe!
		if (parent_change_sf)
			return;
		
		//we do not participate to another superframe
		if (nodedata->parent_addr_current == ADDR_INVALID_16){

			//change our active parent 
			nodedata->parent_addr_current = header_beacon->src;			
			
			//too many parents or sub-optimal depth or too many parents if we associate to this new one
            //considers only associated parent before removing this one (the on-going association request may fail) 
			short parent_to_remove;
            parent_to_remove = !ctree_is_parent_curent_valid(c, header_beacon->src, pk_beacon_get_depth(packet));
            parent_to_remove = parent_to_remove || (nodedata->sframe.my_current_role == ROLE_COORDINATOR);
            
              
			//disassosiation
			if (parent_to_remove){
				
				tools_debug_print(c, "[TREE] parent suppression (one of the best depths %d, already coordinator %d, state transition at %s)\n", 
                                  !ctree_is_parent_curent_valid(c, header_beacon->src, pk_beacon_get_depth(packet)),
                                  nodedata->sframe.my_current_role == ROLE_COORDINATOR,
								  tools_time_to_str(time_data_part_sf, msg, 100));
               
                ctree_parent_print_debug_info(c);  
                
                
                //I am already a coordinator in this superframe -> I just remove this parent
                if (nodedata->sframe.my_current_role == ROLE_COORDINATOR){//sf_slot_current == nodedata->sframe.my_sf_slot){
                    tools_debug_print(c, "[TREE] without notification: we use the same superframe slot\n");
					ctree_parent_remove_addr_short(c, pinfo_current->addr_short);				
					nodedata->parent_addr_current = ADDR_INVALID_16;    
                    
                    return;
                }                
				
				//Not yet associated -> just drop the parent from the list
				else if (!pinfo_current->associated){
					tools_debug_print(c, "[TREE] without notification (not associated)\n");
					ctree_parent_remove_addr_short(c, pinfo_current->addr_short);				
					nodedata->parent_addr_current = ADDR_INVALID_16;                    
                    
                    dequeue_desalloc_tx_up_pending_packet(c);
                    fsm_change(c, STATE_SLEEP, get_time());
				}
 
				//sends an assoc-notification and waits for an ack if we are not in another superframe
				else{
					nodedata->pk_tx_up_pending = mgmt_disassoc_notif_create(c, header_beacon->src);
                    nodedata->sframe.my_current_role = ROLE_CHILD;
                  
                    //My next state depends on the BOP algo
                    switch (param_get_bop_algo()){
                        case ALGO_BOP_ORIG :
                            fsm_change(c, STATE_CHILD_BACKOFF, time_data_part_sf);
                            break;
                        case ALGO_BOP_BACKOFF:
                            fsm_change(c, STATE_BOP_WAIT_FREE, get_time());
                            beacon_stop_waiting_schedule(c, beacon_get_time_start_sframe(c) + tools_get_sd_from_so(nodedata->sframe.SO));

                            break;
                        default:
                            tools_exit(18, c, "bug in BOP algo\n");
                    }            
				}
				
				return;
			}

			
			//Already associated nodes
			if (pinfo_current->associated){
			
				//re-schedule my beacons (to deal with clock drifts)
				beacon_reschedule_my_beacon_after_rx(c);
			}	
				
		
			//Special case -> association in progress but no pending packet -> timeout occured in this coordinator
			if ((pinfo_current->positive_notif <= get_time()) && (!pinfo_current->associated)){

				//this is not anymore a valid parent -> we have to remove it and move to the correct state
				mgmt_parent_disassociated(c, pinfo_current->addr_short);
				tools_debug_print(c, "[TREE] parent %d removed -> although my assoc-req was acked, I did not receive my assoc-rep & no pending packet for me!\n", header_beacon->src);
  			
 				return;
			}
		
	
			//state transition -> I can transmit the buffered packets only when the data part of the superframe begins (BOP)
            //My next state depends on the BOP algo
            switch (param_get_bop_algo()){
                case ALGO_BOP_ORIG :
                    fsm_change(c, STATE_CHILD_IDLE, time_data_part_sf);
                    break;
                    
                case ALGO_BOP_BACKOFF:
                    fsm_change(c, STATE_BOP_WAIT_FREE, get_time());
                    beacon_stop_waiting_schedule(c, beacon_get_time_start_sframe(c) + tools_get_sd_from_so(nodedata->sframe.SO));
                   break;
                    
                default:
                    tools_exit(18, c, "bug in BOP algo\n");
            }         

            //I am now a CHILD
            tools_debug_print(c, "[TREE] now child for parent %d\n", nodedata->parent_addr_current);
            nodedata->sframe.my_current_role = ROLE_CHILD;
			return;
		}
		else
			tools_debug_print(c, "[TREE] I am already following another parent (%d)\n", nodedata->parent_addr_current);
	}
    
    //we have to send a multicast-req
    else if ((pinfo_current == NULL) && (nodedata->pk_tx_up_pending == NULL) && (neigh_elem->multicast_to_rx)){
        if (nodedata->fsm.state != STATE_CHILD_WAIT_BEACON){
            nodedata->parent_addr_current = header_beacon->src;            
            
            //My next state depends on the BOP algo
            switch (param_get_bop_algo()){
                case ALGO_BOP_ORIG :
                    fsm_change(c, STATE_CHILD_IDLE, time_data_part_sf);
                    break;
                    
                case ALGO_BOP_BACKOFF:
                    fsm_change(c, STATE_BOP_WAIT_FREE, get_time());
                    beacon_stop_waiting_schedule(c, beacon_get_time_start_sframe(c) + tools_get_sd_from_so(nodedata->sframe.SO));

                    break;
                    
                default:
                    tools_exit(18, c, "bug in BOP algo\n");
            }         
	
        }
  	}
        
    //    
	else if (nodedata->parent_addr_current != ADDR_INVALID_16)
		tools_debug_print(c, "[TREE] I am already following another parent (%d)\n", nodedata->parent_addr_current);
}


//-------------------------------------------
//			BOP/SF slots ASSIGNMENT 
//-------------------------------------------


//the slots used by my parents
void beacon_compute_slots_parent(call_t *c, short *slots_parents){
	nodedata_info *nodedata = (nodedata_info*) get_node_private_data(c);
	
	//tmp vars
	parent_info		*parent_elem;
	int				nb_slots_parents = 0;
	
    
	//params
	int		nb_sf_slots = pow(2, nodedata->sframe.BO - nodedata->sframe.SO);
	
	das_init_traverse(nodedata->parents);
	while ((parent_elem = (parent_info*) das_traverse(nodedata->parents)) != NULL) {
		
		if ((parent_elem!= NULL) && (parent_elem->associated)){           
            slots_parents[parent_elem->sf_slot] = TRUE;
			nb_slots_parents ++;
		}		
	}	
	
	//If all the slots are fobidden, this means we have a problem!!!
	if (nb_slots_parents >= nb_sf_slots)
		tools_exit(3, c, "%d has more parents than SF slots (%d >= %d). We clearly have a problem to maintain our own superframe\n", c->node, nb_slots_parents, nb_sf_slots);

}




//---- Test some conditions for this neighbor elem --------
//true function
int beacon_neigh_return_always_true(neigh_info *n1, call_t *c){
	return(TRUE);
}
//does this node has a child and a lower id?
int beacon_neigh_has_prio(neigh_info *n1, call_t *c){
	nodedata_info *nodedata = (nodedata_info*) get_node_private_data(c);	
	
	return ((n1->has_a_child) && (n1->depth <= ctree_compute_my_depth(c)) && (n1->addr_short < nodedata->addr_short));
}

//does this node has a child?
int beacon_neigh_has_child(neigh_info *n1, call_t *c){
	//nodedata_info *nodedata = (nodedata_info*) get_node_private_data(c);	
	
	return(n1->has_a_child);
}



//---- Test some conditions for this nodeid_t --------
//the RETURN_TRUE function 
int beacon_id_return_always_true(nodeid_t id){
	return(TRUE);
}
//this node has at least one child
int beacon_id_has_a_child(nodeid_t id){
	return(neigh_table_get_nb_children(god_knowledge[id]->neigh_table) > 0);
}




//the nb of nodes I know that use each slot
int beacon_compute_slots_usage_from_neigh_table(call_t *c, short *slots_usage, short *slots_forbidden, int (*func_consider)(neigh_info*, call_t*)){
	nodedata_info *nodedata = (nodedata_info*) get_node_private_data(c);
	neigh_info	*neigh_elem;
	
	//min usage among all slots
	int			nb_sf_slots = pow(2, nodedata->sframe.BO - nodedata->sframe.SO);
	int			min_usage = -1;
	

	das_init_traverse(nodedata->neigh_table);	
	while ((neigh_elem = (neigh_info*) das_traverse(nodedata->neigh_table)) != NULL) {	
			
		if ((func_consider(neigh_elem, c)) && (neigh_elem->sf_slot != SF_SLOT_INVALID)){
            // tools_debug_print(c, "[NEIGH] sfslot %d used by %d\n", neigh_elem->sf_slot, neigh_elem->addr_short);
		
            if (neigh_elem->sf_slot > nb_sf_slots -1)
                printf("ERROR, invalid sf_slot: %d (inv %d)\n", neigh_elem->sf_slot, SF_SLOT_INVALID);            
            slots_usage[neigh_elem->sf_slot]++;
            
        }
	}	
	
	//track the min value
	int	i;
	for(i=0; i<nb_sf_slots; i++)
		if (!slots_forbidden[i])
			if ((min_usage == -1) || (slots_usage[i] < min_usage))
				min_usage = slots_usage[i];
			
	return(min_usage);
}


//references all the nodes that may interfere with my transmissions (god mode, I know a priori the interfering nodes!)
int beacon_compute_slots_usage_god(call_t *c, short *slots_usage, short *slots_forbidden, int (*func_consider)(nodeid_t id)){
	nodedata_info *nodedata = (nodedata_info*) get_node_private_data(c);
	int		i;
	int		nb_nodes = get_node_count();
	char	msg[400], msg2 [20];
	msg[0] = '\0';
	
	//list of interfering coordinators
	short	*coord_interf = calloc(nb_nodes, sizeof(short));
	
	//min usage among all slots
	int		nb_sf_slots = pow(2, nodedata->sframe.BO - nodedata->sframe.SO);
	int		min_usage = -1;
	bzero(slots_usage, sizeof(short) * nb_sf_slots);

	
	//constructs the list of interfering nodes
	int	my_sf, coord, coord_child;
	for(my_sf=0; my_sf<nb_nodes; my_sf++)
		if (my_sf == c->node || ctree_parent_associated_get_info(god_knowledge[my_sf]->parents, c->node))
			
			for (coord=0; coord<nb_nodes; coord++)
				if ((coord != c->node) && (func_consider(coord))){
					//tools_debug_print(c, "%d -> consider %d\n", coord, func_consider(coord));
				
					//this coordinator directly interferes
					if (tools_graph_nodes_interf(my_sf, coord)){
						//tools_debug_print(c, "coord %d interferes with %d\n", coord, my_sf);
						coord_interf[coord] = TRUE;
					}				
				
					//or one of its children
					else
						for (coord_child=0; !coord_interf[coord] && coord_child<nb_nodes; coord_child++)
							if (ctree_parent_associated_get_info(god_knowledge[coord_child]->parents, coord))
								if (tools_graph_nodes_interf(my_sf, coord_child)){
									coord_interf[coord] = TRUE;
									//tools_debug_print(c, "child %d (parent %d) interferes with %d\n", coord_child, coord, my_sf);
								}
				}

					
	
	//computes the nb of interfering coordinators for each sfslot
	for(coord=0; coord<nb_nodes; coord++)
		if (coord_interf[coord])
			if (god_knowledge[coord]->sframe.my_sf_slot != SF_SLOT_INVALID){
				(slots_usage[god_knowledge[coord]->sframe.my_sf_slot])++;
				
				//debug
				snprintf(msg2, 20, "%d(%d) ", coord, god_knowledge[coord]->sframe.my_sf_slot);
				if (strlen(msg) + strlen (msg2) < 290)
					strcat(msg, msg2);
				else if (strlen(msg) + strlen (msg2) < 295)
					strcat(msg, "[...]");
			}
	//tools_debug_print(c, "[ASSIGNMENT] interfering nodes [addr (sfslot)]: %s\n", msg);
					
			
	//tools_debug_print(c, "slot usage:\n");
	//track the min value
	for(i=0; i<nb_sf_slots; i++){
		//tools_debug_print(c, "%d : %d\n", i, slots_usage[i]);
		
		
		if (!slots_forbidden[i])
			if ((min_usage == -1) || (slots_usage[i] < min_usage))
				min_usage = slots_usage[i];
	}
	
	//memory + return
	free(coord_interf);	
	return(min_usage);	
}	



//Choose a slot not used by any interfering node
int beacon_choose_sf_slot_god(call_t *c){
	nodedata_info *nodedata = (nodedata_info*) get_node_private_data(c);
	
	//parameters 
	uint8_t nb_sf_slots = pow(2, nodedata->sframe.BO - nodedata->sframe.SO);
	
	//info about each slot
	short	*slots_forbidden		= calloc(nb_sf_slots, sizeof(short));
	short	*slots_usage			= calloc(nb_sf_slots, sizeof(short));
	//result
	int		new_sf_slot;

	
	//extracts some info about the slots
	beacon_compute_slots_parent(c, slots_forbidden);
	int	min_usage = beacon_compute_slots_usage_god(c, slots_usage, slots_forbidden, &beacon_id_return_always_true);
	
	
	//if min usage is not null, consider only the nodes with a smaller id and with children
	if (min_usage != 0)
		min_usage = beacon_compute_slots_usage_god(c, slots_usage, slots_forbidden, &beacon_id_has_a_child);
	
	//If my current sf slot has the min usage -> keep it (CONSERVATIVE RULE)
	//NB: the sf slot MUST be obviously correct
	if (nodedata->sframe.my_sf_slot < nb_sf_slots)
		if (!slots_forbidden[nodedata->sframe.my_sf_slot])
			if (nodedata->sframe.my_sf_slot < nb_sf_slots)
				if (min_usage == slots_usage[nodedata->sframe.my_sf_slot])
					return(nodedata->sframe.my_sf_slot);
	

	//else choose randomly one of the min slots
	do{
		new_sf_slot = 	get_random_integer_range(0, nb_sf_slots-1);	
	}while(min_usage != slots_usage[new_sf_slot] || slots_forbidden[new_sf_slot]);	
	
	//memory
	free(slots_forbidden);
	free(slots_usage);
		
	//resulting bop slot
	return(new_sf_slot);
	
}




//Choose a random slot for my superframe (it just has to be different from my parent)
int beacon_choose_sf_slot_greedy(call_t *c){
	nodedata_info *nodedata = (nodedata_info*) get_node_private_data(c);
	
	
	//parameters 
	int	nb_sf_slots = pow(2, nodedata->sframe.BO - nodedata->sframe.SO);
    
	//info about each slot
	short	*slots_forbidden		= calloc(nb_sf_slots, sizeof(short));
	short	*slots_usage			= calloc(nb_sf_slots, sizeof(short));
	//result
	int		new_sf_slot;
	int     min_usage;
    
	//extracts some info about the slots
	beacon_compute_slots_parent(c, slots_forbidden);
	min_usage = beacon_compute_slots_usage_from_neigh_table(c, slots_usage, slots_forbidden, &beacon_neigh_has_child);
    
    //slot 0 => for the PAN coordinator. Should not be used
    slots_forbidden[0] = TRUE;
	
	//if min usage is not null, consider only the nodes with a smaller id and with children
	if (min_usage != 0)
		min_usage = beacon_compute_slots_usage_from_neigh_table(c, slots_usage, slots_forbidden, &beacon_neigh_has_prio);
	
	//If my current sf slot has the min usage -> keep it (CONSERVATIVE RULE)
	//NB: the sf slot MUST be obviously correct
	if (nodedata->sframe.my_sf_slot < nb_sf_slots){

        if (!slots_forbidden[nodedata->sframe.my_sf_slot] && (nodedata->sframe.my_sf_slot < nb_sf_slots) && (min_usage == slots_usage[nodedata->sframe.my_sf_slot])){
            free(slots_forbidden);
            free(slots_usage);
            return(nodedata->sframe.my_sf_slot);
        }
        else{
            tools_debug_print(c, "[CTREE] usage of our current slot (%d) is %d >> min usage %d\n", 
                              nodedata->sframe.my_sf_slot, 
                              slots_usage[nodedata->sframe.my_sf_slot], 
                              min_usage);
        }
    }

	
	//else choose randomly one of the min slots
	do{
		new_sf_slot = 	get_random_integer_range(0, nb_sf_slots-1);	
	}while(min_usage != slots_usage[new_sf_slot] || slots_forbidden[new_sf_slot]);	
		
	//memory
	free(slots_forbidden);
	free(slots_usage);
	
	//resulting bop slot
	return(new_sf_slot);
}


//Choose a random slot for my superframe (it just has to be different from my parent)
int beacon_choose_sf_slot_rand(call_t *c){
	nodedata_info *nodedata = (nodedata_info*) get_node_private_data(c);
	int		new_sf_slot;
	int		nb_sf_slots = pow(2, nodedata->sframe.BO - nodedata->sframe.SO);
	

	
	//find a free slot
	//NB: we MUST have a free slot since we have at most nb_sf_slots-1 parents 
	do{
		new_sf_slot = 	get_random_integer_range(0, nb_sf_slots-1);
		
	}while (!ctree_parent_associated_sf_slot_not_used(c, new_sf_slot));	//This slot is NOT used by one of my parents
		
	
	//resulting bop slot
	return(new_sf_slot);

}


//returns a superframe slot (depends on the assignment algorithm
int beacon_choose_sf_slot(call_t *c){
	nodedata_info *nodedata = (nodedata_info*) get_node_private_data(c);
	int		sf_slot = nodedata->sframe.my_sf_slot;
	int		nb_sf_slots = pow(2, nodedata->sframe.BO - nodedata->sframe.SO);

	
	//NB: If I have already children, this means that I should not choose a new sf_slot for the greedy/random methods
	switch(param_get_algo_sf()){
			
			
		//IN the 802.15.4 standard, my superframe must follow directly the superframe of my parent (my depth may have changed -> update the sf_slot)
		case ALGO_SF_ORG_802154:			
						
			sf_slot = ((int)ctree_compute_my_depth(c)) % nb_sf_slots;
			
		break;
			
		//random slot (not used by any parent). If this is a new parent, we will re-schedule the beacon if we have collisions after the next beacon generation
		case ALGO_SF_ORG_RAND:			
			if (nodedata->sframe.event_beacon_callback == NULL)
				sf_slot = beacon_choose_sf_slot_rand(c);
			
		break;
			
		//random unused slot in the neighborhood. If this is a new parent, we will re-schedule the beacon if we have collisions after the next beacon generation
		case ALGO_SF_ORG_GREEDY:			

			sf_slot = beacon_choose_sf_slot_greedy(c);
			
			break;
				
		//random unused slot (god mode)
		case ALGO_SF_ORG_GOD:			
			
			sf_slot = beacon_choose_sf_slot_god(c);
			
			break;

		default:
			tools_exit(2, c, "This algo (%d) is not yet implemented for sf scheduling\n", param_get_algo_sf());

	}
    
    neigh_table_print(c);
	ctree_parent_print_debug_info(c);
	tools_debug_print(c, "[BEACON] I selected the sf slot %d (diff sf slot %d, previously %d)\n", sf_slot, sf_slot!=nodedata->sframe.my_sf_slot, nodedata->sframe.my_sf_slot);
	return(sf_slot);	
}



//returns an usued BOP slot
int beacon_choose_bop_slot(call_t *c){
	nodedata_info *nodedata = (nodedata_info*) get_node_private_data(c);
	
	//control
	int		i;
	char	tmp[100], msg[100];
	
	//tmp vars
	neigh_info	*neigh_elem;
	
	//bop slots
    int     bop_slot = param_get_bop_nb_slots();
	short	*used_bop_slots = calloc(bop_slot, sizeof(short));
	int		nb_used_bop_slots = 0;
	
	
	//init
	strcpy(msg, "");
	
	//constructs the list of BOP slots used by neighbors with children
	das_init_traverse(nodedata->neigh_table);	
	while ((neigh_elem = (neigh_info*) das_traverse(nodedata->neigh_table)) != NULL) {
		
		if ((neigh_elem->sf_slot == nodedata->sframe.my_sf_slot)){
			used_bop_slots[neigh_elem->bop_slot] = TRUE;
			tools_debug_print(c, "[NEIGH] %d uses bop slot %d\n", neigh_elem->addr_short, neigh_elem->bop_slot);
		}
	}	
	
	//nb of used bop slots (if all are used, I have to create collisions ...
	for(i=0; i<param_get_bop_nb_slots(); i++)
		if (used_bop_slots[i]){
			snprintf(tmp, 100, "%d ", i);
			strncat(msg, tmp, 100);					 
			nb_used_bop_slots ++;
		}
	tools_debug_print(c, "[NEIGH] already used bop slots: %s\n", msg);
	
	
	
	//find a free slot
	do{
		bop_slot = 	get_random_integer_range(0, param_get_bop_nb_slots()-1);
		
	}while ((nb_used_bop_slots != param_get_bop_nb_slots()) && (used_bop_slots[bop_slot]));
	
	tools_debug_print(c, "[BEACON] I chose the bop slot %d\n", bop_slot);
	
	//memory
	free(used_bop_slots);
	
	//resulting bop slot
	return(bop_slot);
}


//ASSOCATION FINALIZATION
// -> compute my superframe (BOP + sf slot)
// -> schedule my first beacon
// -> assign my new short address
void beacon_slots_change(call_t *c){
	nodedata_info	*nodedata = (nodedata_info*)get_node_private_data(c);
	
	//my current parent (if I am receiveing a response, I MUST be in its superframe!)
	parent_info	*pinfo_current = ctree_parent_get_info(nodedata->parents, nodedata->parent_addr_current);
	
	//reschedule a new beacon only in one of these conditions:
	// -> no beacon scheduled
	// -> collision with my current parent
	if (
		(pinfo_current->sf_slot == nodedata->sframe.my_sf_slot)
		||
		(nodedata->sframe.event_beacon_callback == NULL)
		){
	
		//BOP and Superframe slots to start my superframe
		nodedata->sframe.my_sf_slot		= beacon_choose_sf_slot(c);
		nodedata->sframe.my_bop_slot	= beacon_choose_bop_slot(c);
        tools_debug_print(c, "[BEACON] sf slot %d, bop slot %d (parent %d uses sfslot %d / bopslot %d)\n", nodedata->sframe.my_sf_slot, nodedata->sframe.my_bop_slot, pinfo_current->addr_short, pinfo_current->sf_slot, pinfo_current->bop_slot);
        
        //update my neightable (myself -> 0-neighbor)
        neigh_update_myself(c);

		//schedule my next beacon
		beacon_reschedule_my_beacon_after_rx(c);	
	}
}


//-------------------------------------------
//					TX 
//-------------------------------------------


//stops the superframe for one coordinator (go sleeping)
int beacon_end_sframe(call_t *c, void *arg){
	
	//will trigger a state change (will go sleeping)
	tools_debug_print(c, "[COORD] we finished our superframe, let now go sleeping\n");
	fsm_change(c, STATE_SLEEP, get_time());
	
	return(0);
}

//stops listening to beacons
int beacon_stop_waiting_event(call_t *c, void *arg){
	nodedata_info *nodedata = (nodedata_info*) get_node_private_data(c);
    
	tools_debug_print(c, "[LISTENER] we finished our superframe without entering in the CAP\n");
	fsm_change(c, STATE_SLEEP, get_time());
	
    //remove pending transmissions
    if (nodedata->pk_tx_down_pending != NULL)
        dequeue_desalloc_tx_down_pending_packet(c);
    
    nodedata->sframe.event_beacon_stop = NULL;
    
	return(0);
}
int beacon_stop_waiting_schedule(call_t *c, uint64_t time){
	nodedata_info *nodedata = (nodedata_info*) get_node_private_data(c);
     
    char    msg[100];
    tools_debug_print(c, "[LISTENER] we will finish waiting a beacon at %s\n", tools_time_to_str(time, msg, 100));
     
    //cancel the previous event
    if (nodedata->sframe.event_beacon_stop != NULL)
        scheduler_delete_callback(c, nodedata->sframe.event_beacon_stop);
    
    //schedules a new one
    nodedata->sframe.event_beacon_stop = scheduler_add_callback(time, c, beacon_stop_waiting_event, NULL);
    
 	
	return(0);
}



//change the sf/bop slots if they are not accurate
void beacon_reschedule_slots_and_beacon(call_t *c){
	nodedata_info *nodedata = (nodedata_info*) get_node_private_data(c);
	char	msg[200];
	
	
	//empty mgmt buffer and no child -> we should chose another BOP slot (a collision maybe occurs)
	if ((!neigh_table_I_have_a_child(c)) && (nodedata->node_type != PAN_COORD))
		nodedata->sframe.my_bop_slot = beacon_choose_bop_slot(c);
	
	//SF slot -> verify we have no collision
	if (nodedata->node_type != PAN_COORD)
		nodedata->sframe.my_sf_slot = beacon_choose_sf_slot(c);
		
	//Next beacon scheduling
	uint64_t	my_next_beacon = beacon_compute_next_sf(nodedata->sframe.my_sf_slot);
	if (my_next_beacon == get_time())									//same function used to compute the next slot to listen (this could be NOW in this case)
		my_next_beacon += tools_get_bi_from_bo(nodedata->sframe.BO);	
    
    tools_debug_print(c, "BI %s\n", tools_time_to_str(tools_get_bi_from_bo(nodedata->sframe.BO), msg, 50));
	
        
    //cancel the previous interrupt if one exists
	if (nodedata->sframe.event_beacon_callback != NULL)
		scheduler_delete_callback(c, nodedata->sframe.event_beacon_callback);
    
	tools_debug_print(c, "[COORD] next beacon scheduled at %s\n", tools_time_to_str(my_next_beacon, msg, 50)), 
	nodedata->sframe.event_beacon_callback = scheduler_add_callback(my_next_beacon, c, beacon_callback, NULL);
}



//create one beacon and fill its different fields
packet_t *beacon_pk_create(call_t *c){
	nodedata_info *nodedata = (nodedata_info*) get_node_private_data(c);
	int		i;
	
	
	//-------PENDING PACKETS -----
	
	
	//pending list of destinations
	short		*dests_short	= calloc(get_node_count(), sizeof(short));
	short		*dests_long		= calloc(get_node_count(), sizeof(short));
	buffer_get_list_pending_dests(c, dests_short, dests_long);
	
	//uper bound the cumulative number of destinations (privileges short addresses)
	short nb_dests_short;
	if (buffer_count_nb_dests(dests_short) > BEACON_MAX_NB_ADDR)
		nb_dests_short = BEACON_MAX_NB_ADDR;
	else
		nb_dests_short = buffer_count_nb_dests(dests_short);
	
	//the nb of long addresses (upper bounded by MAX - nb_short)
	short nb_dests_long;
    if (buffer_count_nb_dests(dests_long) > BEACON_MAX_NB_ADDR - nb_dests_short)
		nb_dests_long = BEACON_MAX_NB_ADDR - nb_dests_short;
	else
		nb_dests_long = buffer_count_nb_dests(dests_long);
	
    
    
    
	//-------PENDING SEQNUMs -----
	
    seqnum_beacon_info *seqnums_for_beacons = das_create();
    mgmt_seqnum_compute_list_for_beacons(c, &seqnums_for_beacons);
    int nb_multicast = das_getsize(seqnums_for_beacons);
    
	//-------PK CREATION -----
	
	//new packet with corresponding headers to fill it (common + beacon headers + list of pending destinations)
	// common + beacon 
    // + pending short dests + pending long dests
    // + map of seqnums: X * (multicast addr (4bits) * seqnum min/max (2 bytes) 
	
	int pk_size = sizeof(_802_15_4_header) + sizeof(_802_15_4_Beacon_header) +	\
	(nb_dests_short * sizeof(uint16_t)) + (nb_dests_long * sizeof(uint64_t)) +\
    (int)ceil(nb_multicast * (sizeof(uint8_t) * 2 + 0.5));
//	printf("pk size %d (%d short dest, %d long dest, %d multicast) mem %d\n", pk_size, nb_dests_short, nb_dests_long, nb_multicast, (int)ceil(nb_multicast * (sizeof(uint8_t) * 2 + 0.5)));
	
	packet_t				*beacon_pk	= packet_create(c, pk_size, -1);
	
   
    tools_debug_print(c, "******************************************************************************************************\n");

	//-------FIELDS -----

	_802_15_4_Beacon_header *hdr_beacon = (_802_15_4_Beacon_header *) (beacon_pk->data + sizeof(_802_15_4_header));
	
	//common header
	pk_set_ftype(beacon_pk, MAC_BEACON_PACKET);
	pk_set_A(beacon_pk, FALSE);
	pk_set_seqnum(beacon_pk, mgmt_assign_seqnum(c, ADDR_MULTI_BEACONS));
	
	//beacon header
	hdr_beacon->src				= nodedata->addr_short;
	
	//parameters
	pk_beacon_set_BO(beacon_pk, nodedata->sframe.BO);
	pk_beacon_set_SO(beacon_pk, nodedata->sframe.SO);
	
	//pending destinations
	pk_beacon_set_nb_pending_short_addr(beacon_pk, nb_dests_short);
	pk_beacon_set_nb_pending_long_addr(beacon_pk, nb_dests_long);
	
	//BOP and SF slot
	pk_beacon_set_bop_slot(beacon_pk, nodedata->sframe.my_bop_slot);	
	pk_beacon_set_sf_slot(beacon_pk, nodedata->sframe.my_sf_slot);	
	
    
	//additionnal info to optimize the cluster-tree
	pk_beacon_set_depth(beacon_pk, ctree_compute_my_depth(c));	
	pk_beacon_set_has_a_child(beacon_pk, neigh_table_I_have_a_child(c));	
	
	
	//------- PENDING DESTINATIONS -------
	
    char    tmp[50];
    char    pending_dest_str[200];
    strcpy(pending_dest_str, "");
    
	//copy the list of pending destinations
	//I am directly hacking the corresponding memory in the packet, considering it is a block of bytes (i.e. array) 
	uint16_t	*hdr_dests_short = (uint16_t*) (beacon_pk->data + \
												sizeof(_802_15_4_header) + \
												sizeof(_802_15_4_Beacon_header));
	int			dest_current = 0;
	for(i=0; (i<get_node_count()) && (dest_current < nb_dests_short); i++)
		if (dests_short[i]){
            snprintf(tmp, 50, "%d ", (int)i);
            strncat(pending_dest_str, tmp, 200);
			hdr_dests_short[dest_current++] = i;	
        }
	
	uint64_t	*hdr_dests_long = (uint64_t*) (beacon_pk->data + \
											   sizeof(_802_15_4_header) + \
											   sizeof(_802_15_4_Beacon_header) + \
											   sizeof(uint16_t) * nb_dests_short);
	dest_current = 0;
	for(i=0; (i<get_node_count()) && (dest_current < nb_dests_long); i++)
		if (dests_long[i]){
            snprintf(tmp, 50, "%lu(%d) ", (long int)param_addr_short_to_long(i), i);
            strncat(pending_dest_str, tmp, 200);
 			hdr_dests_long[dest_current++] = param_addr_short_to_long(i);
       }
	
    
 	//------- PENDING SEQNUM ranges for each multicast address -------
    
    //nb pf multicast addresses
    pk_beacon_set_nb_pending_multicast(beacon_pk, nb_multicast);
    
    //info for each multicast address
    das_init_traverse(seqnums_for_beacons);
	seqnum_beacon_info  *seqnum_elem;
	i = 0;
    tools_debug_print(c, "Compressed ADDR: MIN < < MAX\n");
    while ((seqnum_elem = (seqnum_beacon_info*) das_traverse(seqnums_for_beacons)) != NULL) {
        pk_beacon_set_seqnum_min(beacon_pk, i, seqnum_elem->seqnum_min);
        pk_beacon_set_seqnum_max(beacon_pk, i, seqnum_elem->seqnum_max);
        pk_beacon_set_seqnum_caddr(beacon_pk, i, seqnum_elem->caddr);        
 	
        tools_debug_print(c, "%d: %d< <%d\n", seqnum_elem->caddr, seqnum_elem->seqnum_min, seqnum_elem->seqnum_max);
        i++;        
    }
	
	tools_debug_print(c, "[MAC_TX_BCN] SeqNum %d, depth %f, BOP slot %d, SF slot %d, pending dests: %s (%d short, %d long), pk id %d\n", 
                      pk_get_seqnum(beacon_pk), 
                      ctree_compute_my_depth(c), 
                      nodedata->sframe.my_bop_slot, 
                      nodedata->sframe.my_sf_slot, 
                      pending_dest_str, 
                      nb_dests_short, 
                      nb_dests_long, 
                      beacon_pk->id);
	
	
 
 /*   int		sf_slot_current = (get_time() % tools_get_bi_from_bo(nodedata->sframe.BO)) / tools_get_sd_from_so(nodedata->sframe.SO);
    uint64_t start = 0;
    
    while(start < get_time()){
        start += tools_get_bi_from_bo(nodedata->sframe.BO);
    }                       
                                      
    
	printf("current sfslot : %d\n", sf_slot_current);
    printf("start of current sfslot : %s\n", tools_time_to_str(msg, 100, ));
    printf("my sfslot : %d\n", nodedata->sframe.my_sf_slot);
    printf("start of my sfslot : %s\n", tools_time_to_str(msg, 100, nodedata->sframe.my_sf_slot));
    */
    
    
	//memory
	free(dests_short);
	free(dests_long);
	das_destroy(seqnums_for_beacons);
    
	return(beacon_pk);
}


//Creates the beacons and reschedule the next ones
int beacon_callback(call_t *c, void *args) {
	nodedata_info *nodedata = (nodedata_info*) get_node_private_data(c);
	char	msg[200];
	
	//aux vars
	uint64_t now = get_time();	
	int		sf_slot_current = tools_compute_sfslot(get_time());
		
    
	//is the callback correct? (if the superframe timing changed, we have to already have re-scheduled our beacon!)
	if (sf_slot_current != nodedata->sframe.my_sf_slot){
		tools_debug_print(c,"my sf slot %d, current sf slot %d\n", nodedata->sframe.my_sf_slot, sf_slot_current);
		tools_debug_print(c,"now offset %s\n", tools_time_to_str(now % tools_get_bi_from_bo(nodedata->sframe.BO), msg, 50));
		tools_debug_print(c,"sframe %s\n", tools_time_to_str(nodedata->sframe.my_sf_slot * tools_get_sd_from_so(nodedata->sframe.SO), msg, 50));
		tools_exit(3, c, "ERROR the beacon was rescheduled for %d, impossible usually\n", c->node);
	}	
		
    
	//we have no parent -> we should not have schedule a beacon!
	if ((das_getsize(nodedata->parents) == 0) && (nodedata->node_type != PAN_COORD))
		tools_exit(4, c, "ERROR: %d schedules a beacon although it is not attached to the PAN\n", c->node);
	
    
    //I am now coordinator
    nodedata->sframe.my_current_role = ROLE_COORDINATOR;
    
	//change the slots if we have a conflict
	beacon_reschedule_slots_and_beacon(c);	
    
    //remove obsolete packets in the buffers
    buffer_remove_outdated_packets(c);
	
    //my behavior depends on the implemented BOP algo
	switch (param_get_bop_algo()){            
            
        case ALGO_BOP_ORIG:
            ;
            
            //create the beacon
            packet_t *beacon_pk = beacon_pk_create(c);

            //schedule the transmission of the beacon in the correct BOP slot
            //radio_turnon(c);
            radio_switch(c, RADIO_RX);
            tools_debug_print(c, "[BEACON] tx scheduled at %s\n", tools_time_to_str(now + pk_get_bop_slot_duration(c) * nodedata->sframe.my_bop_slot, msg, 100));
            scheduler_add_callback(now + pk_get_bop_slot_duration(c) * nodedata->sframe.my_bop_slot, c, radio_pk_tx, beacon_pk);            
                        
            // go sleeping if I changed my sf slot (the current sfslot is not anymore active!)
            if (sf_slot_current != nodedata->sframe.my_sf_slot){
                tools_debug_print(c, "[BEACON] I changed my sf slot (%d) which is not the current sfslot (%d)\n", nodedata->sframe.my_sf_slot, sf_slot_current);
                fsm_change(c, STATE_SLEEP, now + pk_get_bop_slot_duration(c) * param_get_bop_nb_slots());
                return 0;
            }            
            
            //will go to the idle state when the superframe begins 
            tools_debug_print(c, "[BEACON] I will go the state COORD_IDLE at %s\n", tools_time_to_str(now + pk_get_bop_slot_duration(c) * param_get_bop_nb_slots(), msg, 150));
            fsm_change(c, STATE_COORD_IDLE, now + pk_get_bop_slot_duration(c) * param_get_bop_nb_slots());
      
            break;
            
        case ALGO_BOP_BACKOFF:
            
            //starts to monitor idle time before transmitting the beacon
            fsm_change(c, STATE_BOP_WAIT_FREE, now);
 
            break;
            
            
        default:            
            tools_exit(16, c, "This BOP algo (%d = %s) is not yet imlemented\n", param_get_bop_algo(), tools_bop_algo_to_str(param_get_bop_algo()));
            break;
    }
  
		
	//I will come back to sleeping state when the superframe has terminated
	nodedata->sframe.end_my_sframe = get_time() + tools_get_sd_from_so(nodedata->sframe.SO);
	scheduler_add_callback(nodedata->sframe.end_my_sframe, c, beacon_end_sframe, NULL);
	tools_debug_print(c, "[COORD] end of the superframe scheduled at %s (SO %d, BO %d)\n", 
					  tools_time_to_str(nodedata->sframe.end_my_sframe, msg, 50), 
					  nodedata->sframe.SO, 
					  nodedata->sframe.BO);		
		
	return 0;
}				 



