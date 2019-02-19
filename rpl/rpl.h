#ifndef RPL_H
#define RPL_H

#include <stdio.h>
#include <stdint.h>
#include <include/scheduler.h>
#include <include/modelutils.h>
#include "../radio/radio_generic.h"
#include "rpl-const.h"
#include "rpl-types.h"



//						rpl-dag.c

/************************************************************************/
//                     DIO RECEPTION PARSING
/************************************************************************/

void rpl_process_dio(call_t *c, int from, rpl_dio_t *dio);

/************************************************************************/
//                     DAG OPERATION
/************************************************************************/

rpl_dag_t *rpl_set_root(call_t *c, nodedata_t *nodedata, int dag_id);
void rpl_join_dag(call_t *c, nodedata_t *nodedata, int from, rpl_dio_t *dio);
rpl_dag_t *rpl_alloc_dag(nodedata_t *nodedata, uint8_t instance_id);
void rpl_free_dag(call_t *c, nodedata_t *nodedata, rpl_dag_t *dag);
rpl_dag_t *rpl_get_dag(nodedata_t *nodedata, int instance_id);

/************************************************************************/
//                     PARENT OPERATION
/************************************************************************/

rpl_parent_t *rpl_add_parent(call_t *c, nodedata_t *nodedata, rpl_dag_t *dag, rpl_rank_t rank, int addr, rpl_dio_t *dio);
rpl_parent_t *rpl_find_parent(call_t *c, int addr);
rpl_parent_t *rpl_select_parent(call_t *c, nodedata_t *nodedata, rpl_dag_t *dag);
void rpl_remove_parent(call_t *c, nodedata_t *nodedata, rpl_dag_t *dag, rpl_parent_t* p);
void rpl_remove_parents(call_t *c, nodedata_t *nodedata, rpl_dag_t *dag);
int rpl_process_parent_event(call_t *c, nodedata_t *nodedata, rpl_dag_t *dag, rpl_parent_t *p);


//VERIFICATION
rpl_of_t *rpl_find_of(rpl_ocp_t ocp);
void rpl_process_dio(call_t *c, int from, rpl_dio_t *dio);


//REPAIR
void rpl_local_repair(call_t *c, nodedata_t *nodedata, rpl_dag_t *dag);
void global_repair(call_t *c, nodedata_t *nodedata, rpl_dag_t *dag);



//						rpl-timers.c
int dis_timer(call_t *c, void *args);
void rpl_reset_dio_timer(call_t *c, rpl_dag_t *dag, uint64_t delay, uint8_t force);
void new_dio_interval(call_t *c, rpl_dag_t *dag);
int dio_timer(call_t *c, void *args);

//						rpl-icmp6.c
int send_dio(call_t *c, void *args);
int send_dis(call_t *c, void *args);


//metric
double calculate_my_elt(call_t *c, rpl_parent_t* p);
double calculate_bottlen_elt(call_t *c, rpl_parent_t *p);
int compute_traffic(call_t *c, void *args);
int compute_etx(call_t *c, void *args);
int verify_bottleneck(call_t *c);

//residual energy metric
int recompute_energy(call_t *c, void *args);

#endif /* RPL_H */
