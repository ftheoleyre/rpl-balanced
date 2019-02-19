/*
 *  neigh.h
 *  
 *
 *  Created by Fabrice Theoleyre on 14/04/11.
 *  Copyright 2011 CNRS. All rights reserved.
 *
 */



#ifndef __NEIGH_H__
#define __NEIGH_H__

#include "802154_slotted.h"







//--------------- NEIGH TABLE ------------------

//----- MEMORY ------

//memory release
void neigh_free(neigh_info  *neigh_elem);

    
    
//----- INFO ------


//Create a neighbor without info
neigh_info *neigh_create_unitialized_neigh(uint16_t addr_short);

//returns the searched^th child
uint16_t neigh_table_get_child(neigh_info *neigh_table, int searched);

//is that a ctree neighbor?
short neigh_is_ctree_neigh(call_t *c, uint16_t addr);

//is this node one child?
short neigh_is_a_child(neigh_info *neigh_table, uint16_t addr_short);

//how many children are there in this neigh trable?
int neigh_table_get_nb_children(neigh_info *neigh_table);

//do I have a child or not?
short neigh_table_I_have_a_child(call_t *c);

//returns the entry associated to this address
neigh_info *neigh_table_get(call_t *c, uint16_t addr_short);

//returns the nb of 1-neighbors
int neigh_table_get_nb_neigh(call_t *c);
	
//the entry for myself in the neightable (0-neighbor)
void neigh_update_myself(call_t *c);

//returns the link metric toward this node
double neigh_get_link_metric_for(call_t *c, uint16_t addr);



//----- BEACON RX ------

//adds or updates a neighbor
void neigh_table_add_after_beacon(call_t *c, packet_t *beacon_pk);





// ---- (DIS)ASSOCIATION ---

//maintains a list of my children
void neigh_table_add_child_after_assoc(call_t *c, packet_t *assoc_req);

//remove a child
void neigh_table_remove_after_disassoc(call_t *c, packet_t *disassoc_notif);




// ---- NEIGH TABLE ---

//we received one packet from this node
void neigh_table_update_last_rx_for_addr(call_t *c, uint16_t addr);







//--------------- SFSLOT MONITORING ------------------


//returns the nest superframe slot we have to listen to
uint64_t neigh_get_next_sfslot_to_listen(call_t *c);
	
//debug
void neigh_sfslot_table_print(call_t *c);






//--------------- HELLO ------------------


//we enqueue a hello
void neigh_hello_enqueue(call_t *);

//neighborhood table update when I received an hello
void neigh_hello_rx(call_t *c, packet_t *pk_helllo);






//--------------- DEBUG ------------------


//print the neigh table
void neigh_table_print(call_t *c);



#endif
