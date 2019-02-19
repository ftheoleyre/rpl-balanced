#ifndef __STATS_H__
#define __STATS_H__

#include "rpl.h"
#include <include/igraph/igraph.h>

void stats_init();

void elt_status(call_t *c);

void print_stats(call_t *);

//general stats about the graph / network

//general stats about the DAG at the end of the simulation

//graph stuff
void write_graph(call_t *);



#endif

