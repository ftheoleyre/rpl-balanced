/**
 *  \file   802154_slotted.h
 *  \brief  IEEE 802.15.4 beacon eanabled mode
 *  \author Mohammed Nazim Abdeddaim, Bogdan Pavkovic, Fabrice Theoleyre
 *  \date   2011
 **/





#ifndef __802154_SLOTTED_H__
#define __802154_SLOTTED_H__

//------- LIBRARIES

//maths
#include <math.h>

//standard libraries
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

//errors
#include <errno.h>
#include <assert.h>

#include <sys/types.h>
#include <sys/stat.h>

//graph manipulation  & algos
#include <include/igraph/igraph.h>

//Wsnet functions
#include <include/modelutils.h>



//------- INTERN FUNCTIONS

//types and constants
#include "types.h"
#include "const.h"

//data (reception, buffer, etc)
#include "data.h"

//management (acks, commands)
#include "mgmt.h"

//radio, etc.
#include "radio.h"
#include "packets.h"
#include "../application/cbrv3.h"
#include "../radio/radio_generic.h"

//cluster-tree
#include "beacons.h"
#include "ctree.h"
#include "neigh.h"

//tools
#include "tools.h"
#include "buffer.h"
#include "param.h"
#include "stats.h"

//routing in an ideal manner (no overhead, omniscient)
#include "routing_ideal.h"

//CBR
#include "../application/cbrv3.h"










//--------------- NODE INIT ------------------

//classical process functions in wsnet
int bootstrap(call_t *c);



//--------------- RETX ------------------

//the packet was corectly txed, acked, etc. I must now dequeue it from the correct buffer
void dequeue_desalloc_tx_down_pending_packet(call_t *c);

//in the upload direction (I am a child)
void dequeue_desalloc_tx_up_pending_packet(call_t *c);


//--------------- STATE MACHINE ------------------

//changes the next transition
void fsm_change(call_t *c, int state, uint64_t time);

//prints a string for the state constant
char *fsm_state_to_str(int state);

//walk in the state machine
int state_machine(call_t *c, void *args);


//--------------- TRANSMISSION / RECEPTION ------------------

//update the time our parent notified us we have pending frames
void change_parent_notif(call_t *c, short has_packet_for_me);

//Public methods for transmission/reception (resp. from the upper layer and from the lower layer)
void tx(call_t *c, packet_t *packet);
void rx(call_t *c, packet_t *packet);



//--------------- HEADERS ------------------

//headers size
int get_header_size(call_t *c);
int get_header_real_size(call_t *c);
//When I receive one packet from the uppper layer
int set_header(call_t *c, packet_t *packet, destination_t *dst);



#endif
