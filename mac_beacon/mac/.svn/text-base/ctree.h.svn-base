/*
 *  ctree.h
 *  
 *
 *  Created by Fabrice Theoleyre on 14/04/11.
 *  Copyright 2011 CNRS. All rights reserved.
 *
 */



#ifndef __CTREE_H__
#define __CTREE_H__

#include "802154_slotted.h"





//--------------- INFO ------------------

//is this superframe already used by one of my parents?
short ctree_parent_associated_sf_slot_not_used(call_t *c, uint8_t sf_slot);
short ctree_parent_any_sf_slot_not_used(call_t *c, uint8_t sf_slot);

//how many parents have I? (for a max depth or whatever the depth is)
int ctree_parent_get_nb_associated_with_max_depth(parent_info *plist, double depth_max);
int ctree_parent_get_nb_any_with_max_depth(parent_info *plist, double depth_max);
int ctree_parent_get_nb_associated(parent_info *plist);

//returns the i^th associated parent
uint16_t ctree_parent_get_associated(parent_info *plist, int searched);

	
//Does an already associated parent exist in the list?
short ctree_parent_is_associated(parent_info *plist);

//one hello packet was scheduled in the superframe of this parent
void ctree_parent_set_last_hello_tx(parent_info *plist, uint16_t addr);

//returns the next scheduled parent to listen to
parent_info *ctree_parent_get_next_parent_to_listen(call_t *c);




//--------------- CLUSTERTREE / DEPTH ------------------

//true if this parent is associated 
short ctree_cmp_associated(parent_info* p1);
//always 1 function
short ctree_always_true(parent_info *p);

//is it one valid parent? (new or existing)
//among the k parents with the smallest depth (k = nb max parents)
short ctree_is_parent_curent_valid(call_t *c, uint16_t src, double depth);
short ctree_is_parent_to_add(call_t *c, uint16_t src, double depth);

//returns the depth of my best parent
double ctree_compute_best_depth(call_t *c);
int ctree_compute_nb_parents_with_depth_less_than(call_t *c, double depth_max, short (*func_cmp)(parent_info*));

//returns my depth (i.e. the min of my parents + 1)
double ctree_compute_my_depth(call_t *c);
double ctree_compute_my_depth_any_parent(call_t *c);

//update the time our parent notified us we have pending frames
void ctree_parent_change_notif(call_t *c, uint16_t addr_short, short has_packet_for_me);





//--------------- GET ------------------


//returns the first parent of the list
parent_info *ctree_parent_first_get_info(parent_info *plist);

//returns the info of one of my parents
parent_info	*ctree_parent_get_info(parent_info *plist, uint16_t addr_short);

//returns the info of one of my associated parent
parent_info	*ctree_parent_associated_get_info(parent_info *plist, uint16_t addr_short);





//--------------- REMOVE ------------------


//does this parent has the corresponding short address?
int ctree_parent_has_addr_short(void *elem, void* arg);

//removes a particular parent
void ctree_parent_remove_addr_short(call_t *c, uint16_t addr_short);





//--------------- ADD ------------------


//Creates a new parent in the list after having received a beacon
void ctree_parent_add_after_beacon_rx(call_t *c, packet_t *beacon_pk);








//--------------- DEBUG ------------------

//get debug info
void ctree_parent_print_debug_info(call_t *c);




#endif
