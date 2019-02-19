/*
 *  ctree.c
 *  
 *
 *  Created by Fabrice Theoleyre on 12/04/11.
 *  Copyright 2011 CNRS. All rights reserved.
 *
 */

#include "ctree.h"







//-------------------------------------------
//			INFO
//-------------------------------------------




//is this superframe already used by one of my parents :
// -> the parent is associated (to avoid too strong constraints if we have only a small nb of slots)
// -> the parent has a correct depth (else I will disassociate to in the close future)
short ctree_parent_associated_sf_slot_not_used(call_t *c, uint8_t sf_slot){
	nodedata_info	*nodedata = (nodedata_info*) get_node_private_data(c);
	parent_info		*parent_elem;
	double          my_depth = ctree_compute_my_depth(c);
	
	
	das_init_traverse(nodedata->parents);
	while ((parent_elem = (parent_info*) das_traverse(nodedata->parents)) != NULL) {
		
		if ((parent_elem->sf_slot == sf_slot) && (parent_elem->associated) && (parent_elem->depth < my_depth))
			return(FALSE);
		
	}
	return(TRUE);
}

//is this superframe already used by one of my parents :
// -> the parent has a correct depth (else I will disassociate to in the close future)
short ctree_parent_any_sf_slot_not_used(call_t *c, uint8_t sf_slot){
	nodedata_info	*nodedata = (nodedata_info*) get_node_private_data(c);
	parent_info		*parent_elem;
	double          my_depth = ctree_compute_my_depth(c);
	
	
	das_init_traverse(nodedata->parents);
	while ((parent_elem = (parent_info*) das_traverse(nodedata->parents)) != NULL) {
		
		if ((parent_elem->sf_slot == sf_slot) && (parent_elem->depth < my_depth))
			return(FALSE);
		
	}
	return(TRUE);
}


//how many parents are already associated?
int ctree_parent_get_nb_associated_with_max_depth(parent_info *plist, double depth_max){
	parent_info		*parent_elem;
	int				nb = 0;
	
	das_init_traverse(plist);
	while ((parent_elem = (parent_info*) das_traverse(plist)) != NULL) {
		
		if ((parent_elem->associated) && (parent_elem->depth <= depth_max))
			nb++;
	}	
	return(nb);	
}

//returns the i^th associated parent
uint16_t ctree_parent_get_associated(parent_info *plist, int searched){
	parent_info		*parent_elem;
	int				nb = 0;
	
	das_init_traverse(plist);
	while ((parent_elem = (parent_info*) das_traverse(plist)) != NULL) {
        if (nb == searched)
            return(parent_elem->addr_short);
		
		if ((parent_elem->associated) && (parent_elem->depth <= HOP_INFINITY))
			nb++;
        
	}	
	return(ADDR_INVALID_16);
}

//how many parents are already associated? (whatever their depth is)
int ctree_parent_get_nb_associated(parent_info *plist){
	return (ctree_parent_get_nb_associated_with_max_depth(plist, HOP_INFINITY));
}


//how many parents have we got? (with the smallest depth)
int ctree_parent_get_nb_any_with_max_depth(parent_info *plist, double depth_max){
	parent_info		*parent_elem;
	int				nb = 0;
	
	das_init_traverse(plist);
	while ((parent_elem = (parent_info*) das_traverse(plist)) != NULL) {
	
		if (parent_elem->depth <= depth_max)
			nb++;
	}	
	return(nb);	
}

//Does an already associated parent exist in the list?
short ctree_parent_is_associated(parent_info *plist){
	parent_info		*parent_elem;
	
	das_init_traverse(plist);
	while ((parent_elem = (parent_info*) das_traverse(plist)) != NULL) {
		
		if (parent_elem->associated)
			return(TRUE);
	}	
	return(FALSE);
	
}

//one hello packet was scheduled in the superframe of this parent
void ctree_parent_set_last_hello_tx(parent_info *plist, uint16_t addr){
	parent_info		*parent_elem;
	
	das_init_traverse(plist);
	while ((parent_elem = (parent_info*) das_traverse(plist)) != NULL) {
		
		if (parent_elem->addr_short == addr){
			parent_elem->last_hello_tx = get_time();
			return;
		}
	}	
}

//returns the next scheduled parent to listen to
parent_info *ctree_parent_get_next_parent_to_listen(call_t *c){
	nodedata_info	*nodedata	= (nodedata_info*)get_node_private_data(c);
	
	//times 
	uint64_t	time_sf_closest = 0;
	uint64_t	time_sf_parent_current;
	uint64_t	now = get_time();
	
	//result
	parent_info	*parent_next = NULL;

	
	//walk in the parent list
	parent_info		*parent_elem;	
	das_init_traverse(nodedata->parents);	
	while ((parent_elem = (parent_info*) das_traverse(nodedata->parents)) != NULL) {
		
		//computes the next superframe for this parent
		time_sf_parent_current = parent_elem->sf_slot * tools_get_sd_from_so(nodedata->sframe.SO);
		while(time_sf_parent_current < now)
			time_sf_parent_current += tools_get_bi_from_bo(nodedata->sframe.BO);				
			
		//char msg[150];
		//tools_debug_print(c, "PARENT %d -> %s\n",parent_elem->addr_short,  tools_time_to_str(time_sf_parent_current, msg, 150));
		
		//saves the closest superframe 
		if ((time_sf_closest == 0) || (time_sf_closest >= time_sf_parent_current)){
			time_sf_closest = time_sf_parent_current;		
			parent_next = parent_elem; 
		}		
	}	
	
	return(parent_next);
}



//-------------------------------------------
//			CLUSTERTREE - DEPTH
//-------------------------------------------


//true if this parent is associated 
short ctree_cmp_associated(parent_info* p1){
    if (p1->associated)
        return(TRUE);
    return(FALSE);
}
//always 1 function
short ctree_always_true(parent_info *p){
    return(TRUE);
}

//is it one valid parent (i.e. should we remove this existing one)
//compare it with already associated parents
short ctree_is_parent_curent_valid(call_t *c, uint16_t src, double depth){
 	double     best_depth = ctree_compute_best_depth(c);
    
     //the depth is too large
    if (depth + neigh_get_link_metric_for(c, src) >= best_depth + DEPTH_DELTA)
        return(FALSE);
    
    //it is in the max best parents
    if (ctree_compute_nb_parents_with_depth_less_than(c, depth + neigh_get_link_metric_for(c, src), ctree_cmp_associated) <= param_get_nb_max_parents())
        return(TRUE);
    
    return(FALSE);
}

//is it one new valid parent?
//comaprison with all associated or not parents
//we should insert this new one if it is strictly better than at most max-1 existing parents
short ctree_is_parent_to_add(call_t *c, uint16_t src, double depth){
 	double  best_depth = ctree_compute_best_depth(c);

    //the depth is too large
    if (depth + neigh_get_link_metric_for(c, src) >= best_depth + DEPTH_DELTA)
        return(FALSE);
   
//    tools_debug_print(c, "[CTREE] nb parents with lower depth: %d, max %d, depth limit %f\n", 
//           ctree_compute_nb_parents_with_depth_less_than(c, depth + neigh_get_link_metric_for(c, src), ctree_cmp_associated),
//           param_get_nb_max_parents(),
//           depth + neigh_get_link_metric_for(c, src));
   
    //it is in the max best parents
    if (ctree_compute_nb_parents_with_depth_less_than(c, depth + neigh_get_link_metric_for(c, src), ctree_cmp_associated) < param_get_nb_max_parents())
        return(TRUE);
    
    return(FALSE);
}



//returns the depth of my best parent
double ctree_compute_best_depth(call_t *c){
	double         min_depth = MAX_DEPTH-1;
	nodedata_info	*nodedata	= (nodedata_info*)get_node_private_data(c);
	parent_info		*parent_elem;
    double          depth_tmp;
	
	//Special case for the PAN coordinator
	if (nodedata->node_type == PAN_COORD)
		return(0);	
	
    //searches for the best parent (depth of the parent + link metric)
	das_init_traverse(nodedata->parents);
	while ((parent_elem = (parent_info*) das_traverse(nodedata->parents)) != NULL) {
        tools_debug_print(c, "link weight (%d) = %f\n", parent_elem->addr_short, neigh_get_link_metric_for(c, parent_elem->addr_short));
        
        depth_tmp = parent_elem->depth + neigh_get_link_metric_for(c, parent_elem->addr_short);
        
		if ((parent_elem->associated) && (depth_tmp < min_depth))
			min_depth = depth_tmp;
	}		
	return(min_depth);	
	
}
//returns my depth
double ctree_compute_my_depth(call_t *c){	
    return(ctree_compute_best_depth(c));
}


//returns nb of associated parents with depth <= max_depth
int ctree_compute_nb_parents_with_depth_less_than(call_t *c, double depth_max, short (*func_cmp)(parent_info*)){
	nodedata_info	*nodedata	= (nodedata_info*)get_node_private_data(c);
	parent_info		*parent_elem;
    double          depth_tmp;
   	int             count = 0;
    
    
	das_init_traverse(nodedata->parents);
	while ((parent_elem = (parent_info*) das_traverse(nodedata->parents)) != NULL) {
        depth_tmp = parent_elem->depth + neigh_get_link_metric_for(c, parent_elem->addr_short);
        
         if (func_cmp(parent_elem) && (depth_tmp <= depth_max))
			count++;
	}	
	return(count);
    
}



//update the time our parent notified us we have pending frames
void ctree_parent_change_notif(call_t *c, uint16_t addr_short, short has_packet_for_me){
	nodedata_info	*nodedata = get_node_private_data(c);
	
	//invalid parent
	if (addr_short == ADDR_INVALID_16)
		return;
	
	//the corresponding info for this parent
	parent_info		*pinfo_current = ctree_parent_get_info(nodedata->parents, addr_short);
	if (pinfo_current == NULL){
        ctree_parent_print_debug_info(c);
		tools_exit(3, c, "%d did not find the parent %d to change the pending packets short\n", c->node, addr_short);
    }
	
	
	if (has_packet_for_me)
		pinfo_current->positive_notif = get_time() + tools_get_sd_from_so(nodedata->sframe.SO); //macMaxFrameTotalWaitTime;		
	else
		pinfo_current->positive_notif = get_time();		
}





//-------------------------------------------
//			GET
//-------------------------------------------



//returns the first parent of the list
parent_info *ctree_parent_first_get_info(parent_info *plist){
	das_init_traverse(plist);
	parent_info	*parent_elem = (parent_info*) das_traverse(plist);
	return(parent_elem);	
}



//returns the info of one of my parents
parent_info	*ctree_parent_get_info(parent_info *plist, uint16_t addr_short){
	parent_info		*parent_elem;
	
	das_init_traverse(plist);
	while ((parent_elem = (parent_info*) das_traverse(plist)) != NULL) {
		
		if (parent_elem->addr_short == addr_short)
			return(parent_elem);
	}	
	return(NULL);	
}

//returns the info of one of my associated parent
parent_info	*ctree_parent_associated_get_info(parent_info *plist, uint16_t addr_short){
	parent_info		*parent_elem;
	
	das_init_traverse(plist);
	while ((parent_elem = (parent_info*) das_traverse(plist)) != NULL) {
		
		if ((parent_elem->addr_short == addr_short) && (parent_elem->associated))
			return(parent_elem);
	}	
	return(NULL);	
}





//-------------------------------------------
//			REMOVE
//-------------------------------------------


//does this parent has the corresponding short address?
int ctree_parent_has_addr_short(void *elem, void* arg){
	parent_info	*parent_elem = elem;
	uint16_t	*addr_short = arg;
	
	
	return(parent_elem->addr_short == *addr_short);
}


//removes a particular parent
void ctree_parent_remove_addr_short(call_t *c, uint16_t addr_short){
	nodedata_info	*nodedata	= (nodedata_info*)get_node_private_data(c);
	
	//OANA TODO: IOCTL to RPL and remove it from my  parent list
//	if (ctree_parent_associated_get_info(plist, addr_short) != NULL)
//		printf("[TREE] parent is removed: %d\n", addr_short);
	
    //in the list of parents
	das_selective_delete(nodedata->parents, ctree_parent_has_addr_short, &addr_short);
    
    //remove in the das
    //printf("sup %d %d \n", c->node, addr_short);
    routing_edge_remove_from_mac(c, c->node, addr_short);

    //announce RPL about the deletion of parent
    call_t c_rpl = {get_entity_bindings_up(c)->elts[0], c->node, c->entity};
    IOCTL(&c_rpl, REMOVE_PARENT, NULL, (void*) &addr_short);

    
	//cancel the next beacons if I am not anymore associated to the cluster-tree
	if ((ctree_parent_get_nb_associated(nodedata->parents) == 0) && (nodedata->sframe.event_beacon_callback != NULL)){
		scheduler_delete_callback(c, nodedata->sframe.event_beacon_callback);
		nodedata->sframe.event_beacon_callback = NULL;
        tools_debug_print(c, "[BEACONS] I am not anymore associated -> cancel the next beacons\n");
	}
    else
        tools_debug_print(c, "[BEACONS] parent %d removed, still %d parents (callback %lu)\n", 
                          addr_short, 
                          ctree_parent_get_nb_associated(nodedata->parents), 
                          (long int) nodedata->sframe.event_beacon_callback);
       

}





//-------------------------------------------
//			ADD
//-------------------------------------------

//Creates a new parent in the list
void ctree_parent_add_after_beacon_rx(call_t *c, packet_t *beacon_pk){
	nodedata_info	*nodedata	= (nodedata_info*)get_node_private_data(c);
	_802_15_4_Beacon_header *header_beacon = (_802_15_4_Beacon_header *) (beacon_pk->data + sizeof (_802_15_4_header));
	parent_info	*pinfo_current;		
	
	pinfo_current = malloc(sizeof(parent_info));
	das_insert(nodedata->parents, (void*) pinfo_current);
	
	//and update existing associated info
	pinfo_current->addr_short	= header_beacon->src;
	pinfo_current->sf_slot		= pk_beacon_get_sf_slot(beacon_pk);
	pinfo_current->bop_slot		= pk_beacon_get_bop_slot(beacon_pk);
	pinfo_current->associated	= FALSE;
	pinfo_current->last_hello_tx= 0;
	pinfo_current->positive_notif = 0;
	
	//my depth in the tree
	pinfo_current->depth		= pk_beacon_get_depth(beacon_pk);
}



//-------------------------------------------
//			DEBUG
//-------------------------------------------


//get debug info
void ctree_parent_print_debug_info(call_t *c){
	nodedata_info	*nodedata = get_node_private_data(c);
	parent_info		*parent_elem;
	
	
	tools_debug_print(c, "[TREE] Current parents:\n");
	das_init_traverse(nodedata->parents);
	while ((parent_elem = (parent_info*) das_traverse(nodedata->parents)) != NULL) {
				
		tools_debug_print(c, "[TREE] %d (depth %f, sfslot %d, bopslot %d, assoc %d, posnotif %lu)\n", 
						  parent_elem->addr_short,				 
						  parent_elem->depth,				 
						  parent_elem->sf_slot,				 
						  parent_elem->bop_slot,				 
						  parent_elem->associated,				 
						  (long unsigned int)parent_elem->positive_notif
						  );
	}	
	tools_debug_print(c, "[TREE] My current depth = %f\n" , ctree_compute_my_depth(c));
	
}



