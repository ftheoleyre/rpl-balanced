/*
 *  stats.h
 *  
 *
 *  Created by Fabrice Theoleyre on 04/10/11.
 *  Copyright 2011 CNRS. All rights reserved.
 *
 */


#ifndef __STATS_H__
#define __STATS_H__

#include "802154_slotted.h"

#define LIST_NEIGH_RADIO        1
#define LIST_NEIGH_CTREE        2
#define LIST_NEIGH_RCVD         3
#define LIST_ROUTE              4



//--------------- GLOBAL VARIABLES ------------------

//Stats
extern stat_cbr_info	*global_cbr_stats;
extern stat_ctree_info	*global_ctree_stats;

//informations on all nodes (god infos)
extern nodedata_info	**god_knowledge;




//--------------- STATS ------------------

//statistics about the number of packets globally transmitted / received (classified by packet type)
void stats_pk_rx(packet_t *packet);
void stats_pk_tx(packet_t *packet);
int stats_get_pk_rcvd();
int stats_get_pk_tx();

//copy a list of nodeid_t into a string
char *stats_list_to_str(uint16_t *plist, char *msg, int length);



//------------- UNICAST & MULTICAST -----------

//this packet was dropped because of too many CCA -> update the stats
void stats_data_update_drop_reason(call_t *c, packet_t *packet, short reason);

//statistics when I transmit a multicast frame
void stats_multicast_add_pk(call_t *c, packet_t *packet_rcvd);



//--------------- PRINT ------------------

//the node is associated, save the association stat (time)
void stats_register_association(call_t *c);

//adds myself in the list
void stats_list_add(call_t *c, packet_t *packet, short list_type, nodeid_t id, short debug);

//computes and prints CBR stats
void stats_print_all(call_t *c); 





#endif

