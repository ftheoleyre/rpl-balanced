/*
 *  beacons.h
 *  
 *
 *  Created by Fabrice Theoleyre on 11/02/11.
 *  Copyright 2011 CNRS. All rights reserved.
 *
 */




#ifndef __BEACONS_H__
#define __BEACONS_H__

#include "802154_slotted.h"



//--------------- BEACON scheduling ------------------

//returns the time of the beginning of the active part of the current superframe
uint64_t beacon_get_time_start_sframe(call_t *c);

//the time for our next superframe
uint64_t beacon_compute_next_sf(uint8_t sf_slot);
	
//reschedule my beacon 
void beacon_reschedule_my_beacon_after_rx(call_t *c);

//change the slots and schedule the beacons
void beacon_slots_change(call_t *c);


//--------------- RX/TX ------------------

//stops listening to beacons
int beacon_stop_waiting_schedule(call_t *c, uint64_t time);

//I received one beacon
void beacon_rx(call_t *c, packet_t *packet);

//create one beacon and fill its different fields
packet_t *beacon_pk_create(call_t *c);

//Creates the beacons and reschedule the next ones
int beacon_callback(call_t *c, void *args);




//--------------- SLOTS assignement  ------------------


//returns a superframe slot (depends on the assignment algorithm
int beacon_choose_sf_slot(call_t *c);

//returns an usued BOP slot
int beacon_choose_bop_slot(call_t *c);



#endif

