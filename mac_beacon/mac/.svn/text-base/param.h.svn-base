/*
 *  params.h
 *  
 *
 *  Created by Fabrice Theoleyre on 21/04/11.
 *  Copyright 2011 CNRS. All rights reserved.
 *
 */



#ifndef __PARAMS_H__
#define __PARAMS_H__


#include "802154_slotted.h"


//------------------------------------
//		GLOBAL VARIABLES
//------------------------------------

//for debug
short param_debug();
void param_debug_set(short value);

//return the inter packet time
uint64_t param_get_inter_pk_time_unicast(call_t *c);
uint64_t param_get_inter_pk_time_multicast(call_t *c);
uint16_t param_get_dest_multicast(call_t *c);


//--------------- GRAPH ------------------

void params_set_graph(igraph_t *G);
igraph_t* params_get_graph();



//--------------- PARAMS ------------------

//BOP SLOTS
int		param_get_bop_nb_slots();
void	param_set_bop_nb_slots(int value);

//BOP algo
int     param_get_bop_algo();
void    param_set_bop_algo(int value);

//Multicast algo
int     param_get_multicast_algo();
void    param_set_multicast_algo(int value);

//Hellos (0 = no hello)
uint64_t param_get_hello_period();
void    param_set_hello_boperiod(int value);

//monitoring of neighbors (listen to their beacons)
short param_get_neigh_monitor_all();
void    param_set_neigh_monitor_all(short value);

//Nodes have to listen perdiodically each superframe slot
uint64_t param_get_sfslot_scan_period();
void param_set_sfslot_scan_period(int value);


//BO & SO
void	param_set_global_bo(int value);
int		param_get_global_bo();
void	param_set_global_so(int value);
int		param_get_global_so();

//BOP/SF slots
int		param_get_algo_sf();
void	param_set_algo_sf(int value);
int		param_get_nb_sfslot();

//how we should compute the depth
int param_get_depth_metric();
void param_set_depth_metric(int value);

//Cluster-tree
void param_set_nb_max_parents(int value);
int param_get_nb_max_parents();
	

//--------------- ADDRs ------------------

//returns the identifier associated to a long address
uint16_t param_addr_long_to_short(uint64_t addr_long);

//returns the long address associated to a short address
uint64_t param_addr_short_to_long(uint16_t addr_short);


//--------------- INITIALIZATION ------------------

//assign the default values
void param_assign_default_value(call_t *c);


#endif

