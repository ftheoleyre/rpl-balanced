/*
 *  tools.h
 *  
 *
 *  Created by Fabrice Theoleyre on 11/02/11.
 *  Copyright 2011 CNRS. All rights reserved.
 *
 */


#ifndef __TOOLS_H__
#define __TOOLS_H__

#include "802154_slotted.h"



//--------------- GRAPH ------------------

//euclidean distance between both nodes
double  tools_distance(nodeid_t a, nodeid_t b);

//do these nodes interfere with each other?
short tools_graph_nodes_interf(nodeid_t a, nodeid_t b);

//do these nodes interfere with each other?
short tools_graph_links_interf(nodeid_t s1, nodeid_t d1, nodeid_t s2, nodeid_t d2);


//--------------- ADDRESSES ------------------

//for this compressed multicast address, are seqnums replaced or inserted in the buffer? 
short tools_is_addr_multicast_seqnum_replacement(uint16_t caddr);

//is this a multicast address?
short tools_is_addr_multicast(uint16_t addr);

//is this an anycast address?
short tools_is_addr_anycast(uint16_t addr);

//converts this MAM value in a number of bits
int tools_MAM_to_bits(uint8_t MAM);

//converts a number of seqnum into a MAM
uint8_t tools_nb_to_MAM(int nb);

//multicast address type into an human readable string
char *tools_multicast_addr_to_str(uint16_t addr);


//--------------- MATHS ------------------

//returns the minimum
double tools_min_dbl(double a, double b);

//returns the minimum
int tools_min(int a, int b);



//--------------- FIGURE ------------------

//generates the .fig file
void tools_generate_figures(call_t *c);

	
	
//--------------- DEBUG ------------------

//fatal error
void tools_exit(int codeerr, call_t *c, const char* fmt, ...);
void tools_exit_short(int code,  const char* fmt, ...);

//print a debug message on stdout
void tools_debug_print(call_t *c, const char* fmt, ...);

//converts a uint64_t into a human readeable string
char *tools_time_to_str(uint64_t time, char *msg, int length);

//conversion of the superframe scheduling algo into a string
char *tools_algo_sf_to_str(int algo);

//conversion of the BOP algo into a string
char *tools_bop_algo_to_str(int algo);

//conversion of the mullticast algo into a string
char *tools_multicast_algo_to_str(int algo);

//int alias into a string
char* tools_depth_metric_to_str(int metric);

//state machine into string
char *tools_fsm_state_to_str(int state);

//the reason fordropping the packet
char *tools_drop_reason_to_str(int reason);



//--------------- CONVERSION ------------------

//converts the BO SO into BI SD (i.e. constants into durations)
uint64_t tools_get_bi_from_bo(int BO);
uint64_t tools_get_sd_from_so(int SO);

//returns the corresponding sf slot
int tools_compute_sfslot(uint64_t time);

#endif

