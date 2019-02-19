#include <limits.h>
#include <string.h>
#include <stdint.h>

#include "rpl.h"

/************************************************************************/
extern rpl_of_t rpl_of_elt_multipath;
//extern rpl_of_t rpl_of_elt;


static rpl_of_t *objective_functions[] = {&rpl_of_elt_multipath,  NULL};
/************************************************************************/





/************************************************************************/
//                     DAG OPERATION
/************************************************************************/

//Sink nodes should execute this code   OK January 2014
//Sink should init ALL necessary parameters to be able to send DIO!!!
rpl_dag_t *rpl_set_root(call_t *c, nodedata_t *nodedata, int dag_id){
	rpl_dag_t *dag;
	uint8_t version;
	
	version = -1;
	dag = rpl_get_dag(nodedata, RPL_DEFAULT_INSTANCE);
	if(dag != NULL) {
		printf("RPL: Dropping a joined DAG when setting this node as root");
		version = dag->version;
		rpl_free_dag(c, nodedata, dag);
	}
	
	dag = rpl_alloc_dag(nodedata, RPL_DEFAULT_INSTANCE);
	if(dag == NULL) {
		printf("RPL: Failed to allocate a DAG\n");
		return NULL;
	}

		
	//configuration
	dag->version 			= version + 1;
	dag->instance_id		= RPL_DEFAULT_INSTANCE;
	dag->grounded 			= RPL_GROUNDED;
	dag->joined 			= 1;
	dag->used				= 1;
	//dag->dio_sequence 		= 0;
	dag->dag_id 			= dag_id;
	dag->second_part_dag_id = 0;
	dag->of 				= rpl_find_of(nodedata->ocp_from_xml);
	
	
	//rank related
	dag->rank 				= ROOT_RANK;
	dag->min_rank			= INFINITE_RANK;
	dag->max_rankinc		= DEFAULT_MAX_RANKINC;
	dag->min_hoprankinc 	= DEFAULT_MIN_HOPRANKINC;
	dag->preferred_parent 	= NULL;


	//set trickle's default values
	dag->dio_intdoubl	= DEFAULT_DIO_INTERVAL_DOUBLINGS;
	dag->dio_intmin		= pow(2, DEFAULT_DIO_INTERVAL_MIN);
	dag->dio_intcurrent = pow(2, DEFAULT_DIO_INTERVAL_MIN);
	dag->dio_interval   = 0;
	dag->dio_redundancy = DEFAULT_DIO_REDUNDANCY;
	dag->dio_counter	= 0;
	dag->dio_timer = NULL; 	//no DIO events scheduled so far
	
	
	//info about the bottleneck: the root advertises itself as the bottleneck; the root does not transmit packets nor has any parents
	bottleneck_t *bottleneck 	 = (bottleneck_t *) malloc(sizeof(bottleneck_t));
	bottleneck->bottlen_const 	 = (double)(nodedata->energy_residual * DATA_RATE) / (1 * MY_RADIO_TX); //(START_ENERGY * DATA_RATE)	/ (ETX_4_ELT_GUESS * RADIO_TX)	//(E_res * DATA_RATE) / (ETX * 1b/s * Etx)
	bottleneck->bottlen_traffic  = nodedata->my_traffic;
	bottleneck->ratio_of_traffic = 1;
	bottleneck->bottlen_id 		 = c->node;
	das_insert(nodedata->my_bottlenecks, bottleneck);
	
	//infor for residual energy metric
	//dag->path_energy_residual = nodedata->energy_residual;
	//printf("[RPL_SET_ROOT] DAG root set for DAG ID = %d, bottleneck const = %f traffic %f\n", dag->dag_id, dag->bottlen_const, dag->bottlen_traffic);

	//send first dio
	if(nodedata->type == SINK){ 
		scheduler_add_callback(get_time(), c, send_dio, NULL);
		rpl_reset_dio_timer(c, dag, 0, 1);
	}
	
	return dag;
}

/************************************************************************/

//Node receiving first DIO message should execute this OK january 2014
void rpl_join_dag(call_t *c, nodedata_t *nodedata, int from, rpl_dio_t *dio){
	rpl_dag_t *dag;
	rpl_of_t *of;
	rpl_parent_t *p;
	
	dag = rpl_alloc_dag(nodedata, dio->instance_id);
	if(dag == NULL) {
		printf("RPL: Failed to allocate a DAG object!\n");
		return;
	}
	
	p = rpl_add_parent(c, nodedata, dag, dio->rank, from, dio);
	printf("[RPL_JOIN_DAG] Node %d joined the DAG. \n", c->node);	
	p->local_confidence = 0;	// The lowest confidence for new parents.		
			
	//Determine the objective function by using the objective code point of the DIO. 
	of = rpl_find_of(dio->dodag_config.ocp);
	if(of == NULL) {
		printf("RPL: DIO for DAG instance %u does not specify a supported OF\n",dio->instance_id);
		return;
	}
	
	
	//configuration
	dag->version 			= dio->version;
	dag->instance_id		= dio->instance_id;
	dag->grounded 			= RPL_GROUNDED;
	dag->joined 			= 1;
	dag->used				= 1;
	//dag->dio_sequence 	= 0;
	
	
	//rank related
	dag->of 				= rpl_find_of(dio->dodag_config.ocp);
	dag->min_rank			= INFINITE_RANK;
	dag->max_rankinc		= dio->dodag_config.maxRankIncrease;
	dag->min_hoprankinc 	= dio->dodag_config.minHopRankIncrease;
	//dag->preferred_parent 	= p;


	//set trickle's default values
	dag->dio_intdoubl	= dio->dodag_config.dio_intdoubl;
	dag->dio_intmin		= dio->dodag_config.dio_intmin;
	dag->dio_intcurrent = dio->dodag_config.dio_intmin;
	dag->dio_interval   = 0;
	dag->dio_redundancy = dio->dodag_config.dio_redund;
	dag->dio_counter	= 0;
	dag->dio_timer = NULL; 	//no DIO events scheduled so far
	//dag->dio_sequence = 0;

	
	memcpy(&dag->dag_id, &dio->dag_id, sizeof(dio->dag_id));
	dag->second_part_dag_id = 0;

	
	//information for residual energy metric
	/*if(dio->path_energy_residual < nodedata->energy_residual)
		dag->path_energy_residual = dio->path_energy_residual;
	else
		dag->path_energy_residual = nodedata->energy_residual;
	*/

	dag->dio_timer = NULL;
}

/************************************************************************/

//alloc memory for the dag
rpl_dag_t *rpl_alloc_dag(nodedata_t *nodedata, uint8_t instance_id){
	rpl_dag_t *dag;
	rpl_dag_t *end;
	
	for(dag = &nodedata->dag_table[0], end = dag + RPL_MAX_DAG_ENTRIES; dag < end; dag++) {
		if(dag->used == 0) {
			memset(dag, 0, sizeof(*dag));
			//dag->parents		= &dag->parent_list;
			dag->instance_id	= instance_id;
			dag->rank			= INFINITE_RANK;
			dag->min_rank		= INFINITE_RANK;
			dag->joined			= 1;
			dag->min_hoprankinc = DEFAULT_MIN_HOPRANKINC;
			return dag;
		}
	}
	return NULL;
}

/************************************************************************/

//free the dag
void rpl_free_dag(call_t *c, nodedata_t *nodedata, rpl_dag_t *dag){
	printf("RPL: Leaving the DAG ");
	printf("%" PRId64 " ", dag->dag_id);
	printf("\n");
	
	/* TODO - Remove routes installed by DAOs. */
	//rpl_remove_routes(dag);
	
	/* Remove parents and the default route. */
	rpl_remove_parents(c, nodedata, dag);
	//TODO
	//rpl_set_default_route(dag, NULL);
	
	dag->used = 0;
	dag->joined = 0;
}

/************************************************************************/

rpl_dag_t *rpl_get_dag(nodedata_t *nodedata, int instance_id){
	int i;
	for(i = 0; i < RPL_MAX_DAG_ENTRIES; i++) {
		//printf("i = %d\n", i);
		if(nodedata->dag_table[i].joined && (instance_id == RPL_ANY_INSTANCE ||
											 nodedata->dag_table[i].instance_id == instance_id)) {
			return &nodedata->dag_table[i];
		}
	}
	return NULL;
}





/************************************************************************/
//                     PARENT OPERATION
/************************************************************************/

//add parent after receiving DIO from it OK January 2014
rpl_parent_t *rpl_add_parent(call_t *c, nodedata_t *nodedata, rpl_dag_t *dag, uint16_t rank, int addr, rpl_dio_t *dio){
	rpl_parent_t *p;
	
	das_init_traverse(nodedata->parents);
	while((p = (rpl_parent_t*) das_traverse(nodedata->parents)) != NULL){
			if (p->id == addr) {
				return p;
			}
	}

	//If parent is NEW one INIT his structure
	p = (rpl_parent_t *) malloc(sizeof(rpl_parent_t));
	p->id				= addr;
	p->dag				= dag;
	p->rank				= rank;
	p->local_confidence = 0;
	p->instance_id 		= dag->instance_id;
	p->ignore 			= 0;
	//p->last_dio 		= dio->nr_sequence;		

	p->etx_4_elt		= ETX_4_ELT_GUESS; 		//etx for elt
	p->old_etx		 	= (etx_elt_t *) malloc(sizeof(etx_elt_t)); //old etx for elt used for moving average
	
	p->old_etx->neighbor		= addr;
	p->old_etx->packs_net 		= 0;	
	p->old_etx->packs_tx 		= 0;
	p->old_etx->packs_tx_once	= 0;
	p->old_etx->acks			= 0;
	p->old_etx->drops			= 0;
	
	// if ELT multipath: add bottlenecks of the path advertised by the DIOs
	p->bottlenecks 	= das_create();
	p->parent_load	= 0;
	p->parent_old_load	= 0;
	
	
	dio_bottleneck_t	*b;
	bottleneck_t 		*new_bottlenck;
	das_init_traverse(dio->bottlenecks);
	while((b = (dio_bottleneck_t*) das_traverse(dio->bottlenecks)) != NULL){
		new_bottlenck = (bottleneck_t *) malloc(sizeof(bottleneck_t));
		
		//de-normalize
		new_bottlenck->bottlen_id 		 = b->bottlen_id;
		if(b->bottlen_id == 0)
			new_bottlenck->bottlen_const	 = MAX_BOTTLEN_CONST * 1000;  // the sink has "infinite energy"
		else
			new_bottlenck->bottlen_const	 = (double) (b->bottlen_const * MAX_BOTTLEN_CONST) /65535; 
  		new_bottlenck->bottlen_traffic 	 = (float) (b->bottlen_traffic * MAX_TRAFFIC) / 255;
  		new_bottlenck->ratio_of_traffic	 = (float) b->ratio_of_traffic / 255; 
		das_insert(p->bottlenecks, new_bottlenck);
	    //printf("[ADD_PARENT] Node %d, bottleneck %d has ratio of traffic %f and before normalization %d \n", c->node, new_bottlenck->bottlen_id, new_bottlenck->ratio_of_traffic, b->ratio_of_traffic);
	}
	
	printf("[RPL_ADD_PARENT] Node %d adding parent %d\n", c->node, p->id);


	//if residual_energy
	//info about minimum residual energy on the path
	//p->path_energy_residual = dio->path_energy_residual;
	
							
	//printf("[RPL_ADD_PARENT]Node %d added parent %d etx = %f rank =%d. \n", c->node, p->id, p->etx, rank);															
								
	//actually add parent in parents list
 	das_insert(nodedata->parents, p);
 
 	scheduler_add_callback(get_time(), c, compute_etx, NULL);

    return p;
}

/************************************************************************/

//FIND parent; return NULL if no parent with that addr exists
rpl_parent_t *rpl_find_parent(call_t *c, int addr){
	nodedata_t *nodedata = get_node_private_data(c);
	rpl_parent_t *p;	
	das_init_traverse(nodedata->parents);
	while((p = (rpl_parent_t*) das_traverse(nodedata->parents)) != NULL){
		if(p->id == addr) 
			return p;
	}
	return NULL;
}



/*************************************************************************/

//multipath select random preferred parent !!!
rpl_parent_t *get_random_preferred_parent(call_t *c){
	struct nodedata *nodedata = get_node_private_data(c);
    rpl_parent_t *p, *best;
	best = NULL;
	int random_parent;
	
	//best parent = random	
	srand(get_time());
	random_parent = rand() % das_getsize(nodedata->parents);
		
	//get random_parent
	das_init_traverse(nodedata->parents);
	while((p = (rpl_parent_t*) das_traverse(nodedata->parents)) != NULL){		
		if(random_parent == 0){
			//at the beginning, all traffic will be sent to the preferred parent
			//p->parent_load = 1;
			best = p;
		}
		else
			random_parent--;			
	}
		
	return best;
}


//multipath select preferred parent only at bootstrap !!! OK january 2014
			//	choose preferred parent	 OK
			//  remove parents with greater Rank OK
			//  compute traffic load OK
			//  compute new bottlenecks OK
			//  scheduler_add_callback(get_time(), c, send_dio, NULL); OK
int rpl_bootstrap_select_preferred_parent(call_t *c, void *args){	
	struct nodedata *nodedata = get_node_private_data(c);
    rpl_dag_t *dag;
    dag = rpl_get_dag(nodedata, RPL_ANY_INSTANCE);
    rpl_parent_t *p, *best;
	best = NULL;
	
	printf(">>>>>>>>>> Time for node %d to selecte its preferred parent, time = %llu\n", c->node, get_time());
	nodedata->after_bootstrap = 1;

	int nr_parents = das_getsize(nodedata->parents);
	if(nr_parents == 0 || dag == NULL){
		printf("[bootstrap_select_prefered_parent] Node %d no parent or no DAG found. Trigger local repair.\n", c->node);
		rpl_local_repair(c, nodedata, dag);
		return 1;
	}
	else
		//best = get_random_preferred_parent(c);
		best = rpl_select_parent(c, nodedata, dag);
   
   	if(best == NULL) {
		// No suitable parent; trigger a local repair.
		printf("[bootstrap_select_prefered_parent] Node %d no preferred parent found. Trigger local repair.\n", c->node);
		rpl_local_repair(c, nodedata, dag);
		return 1;
	}

	//i have a preferred parent
	dag->preferred_parent = best; //Cache the value
	(dag->preferred_parent) -> local_confidence ++;
	printf("[bootstrap_select_prefered_parent] Node %d has chosen the preferred parent %d.\n", c->node, best->id);
	
	//Update the DAG rank, since link-layer information may have changed the local confidence.
	dag->rank = dag->of->calculate_rank(c, best, 0);
	printf("[bootstrap_select_prefered_parent] Node %d Rank %d\n", c->node, dag->rank);
	dag->min_rank = dag->rank; // So far this is the lowest rank we know 
	
	
	//remove all parents from parent_list that have Rank grater or equal to mine
	das_init_traverse(nodedata->parents);
	while((p = (rpl_parent_t*) das_traverse(nodedata->parents)) != NULL){		
		if(p->rank >= dag->rank){
			das_delete(nodedata->parents, p);	
			printf("[bootstrap_select_prefered_parent] Node %d deleted parent %d, had Rank=%d grater than his %d\n", c->node, p->id, dag->rank, p->rank);
		}
	}
	
	
	nr_parents = das_getsize(nodedata->parents);
	if(nr_parents == 0 || dag == NULL){
		printf("[bootstrap_select_prefered_parent] Node %d no parent or no DAG found. Trigger local repair.\n", c->node);
		rpl_local_repair(c, nodedata, dag);
		return 1;
	}
	else{
		//compute traffic load
		printf("[bootstrap_select_prefered_parent] Node %d has will now compute the traffic loads\n", c->node);
		compute_loads(c, dag);
	
		//update my bottlenecks to the current parents in my list
		update_my_bottlenecks(c,dag);
		
		//printf my bottlenecks
		bottleneck_t* bp;
		printf("[bootstrap_select_prefered_parent] Node %d has the following bottlenecks\n", c->node);
		das_init_traverse(nodedata->my_bottlenecks);
		while((bp = (bottleneck_t*) das_traverse(nodedata->my_bottlenecks)) != NULL){
					printf("[bootstrap_select_prefered_parent] b_id=%d with ratio of traffic=%f\n", bp->bottlen_id, bp->ratio_of_traffic);

		}	
		
		//schedule DIO emission 
		scheduler_add_callback(get_time(), c, send_dio, NULL);
		rpl_reset_dio_timer(c, dag, 0, 1);
	}


	return 0;
}


/*************************************************************************/


//Choose PREFERRED parent among ones that have reported DIO already!!! OK january 2014
rpl_parent_t *rpl_select_parent(call_t *c, nodedata_t *nodedata, rpl_dag_t *dag){
	rpl_parent_t *p, *best, *current;
	best = NULL;
	
	parent_stats *p_stats = (parent_stats *)malloc(sizeof(parent_stats));
		
	if(das_getsize(nodedata->parents) == 0){
		//printf("[RPL_SELECT_PARENT] Node %d has parents' set empty.\n", c->node);
		return best;
	}
	
	current = dag->preferred_parent; //get current PREFERRED parent
	
	void *parents_copy; //make a copy of the parents list, you will need it later
	parents_copy = das_create();
	//printf("Node % d has parents: ", c->node);
	das_init_traverse(nodedata->parents);
	while((p = (rpl_parent_t*) das_traverse(nodedata->parents)) != NULL){		
		das_insert(parents_copy, p);
		//printf("%d ", p->id);
	}
	
	//Find BEST parent
	das_init_traverse(parents_copy);
	while((p = (rpl_parent_t*) das_traverse(parents_copy)) != NULL){		
		if(best == NULL) {
			//Verify that we received DIO from this parent
			if (p->id != -1 && p->ignore == 0){
				best = p;
			}			
		}
		else {
			//Verify that we received DIO from this parent
			if (p->id != -1 && p->ignore == 0){ 
				best = dag->of->best_parent(c, best, p);
				//printf("rpl_selec_parent best is %d\n", best->addr);
			}
		}
	}
	
	//free memory
	if(das_getsize(parents_copy))
		das_destroy(parents_copy);
	//printf("Finally, best parent is %d preferred parent is %d\n\n", best->id, dag->preferred_parent->id);
	
	if(best == NULL){
		printf("%d No preferred parent; local repair >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n", c->node);
		rpl_local_repair(c, nodedata, dag);	
		return NULL;
	}
	//Verify if preferred parent has changed 
	if(dag->preferred_parent){ //verification necessary if we removed the preferred parent before 
		if(dag->preferred_parent->id != best->id){
			dag->preferred_parent = best; //Cache the value
			(dag->preferred_parent) -> local_confidence ++;
			printf("[RPL_SELECT_PARENT] Node %d has changed the preferred parent from %d to %d.\n", c->node, current->id, best->id);
			//printf("[RPL_SELECT_PARENT] Rank of preferred parent changed from %d to %d\n", current->rank, best->rank);
	
			printf("[RPL_SELECT_PARENT] Node %d Rank before re-calculating: %d\n", c->node, dag->rank);
			//Update the DAG rank, since link-layer information may have changed the local confidence.
			dag->rank = dag->of->calculate_rank(c, best, 0);
			printf("[RPL_SELECT_PARENT] Node %d New Rank after re-calculating: %d\n", c->node, dag->rank);
			
			
			//for residual_energy metric
			/*if(dag->of->ocp == 6){
				if(nodedata->energy_residual < best->path_energy_residual)
					dag->path_energy_residual = nodedata->energy_residual;
				else
					dag->path_energy_residual = best->path_energy_residual;
			}*/		
			
			
			//reset dio timer
			//printf("[RPL_SELECT_PARENT] Node %d will reset dio timer.\n", c->node);
			rpl_reset_dio_timer(c, dag, 0, 1);
			
			//stats
			nodedata->change_parent++;
			if(get_time() > START_TIME)
				nodedata->stats_change_parent++;
			p_stats->parent = (dag->preferred_parent)->id;
			p_stats->etx = (dag->preferred_parent)->etx_4_elt;
			p_stats->time = get_time();
			p_stats->my_rank = dag->rank;
			das_insert(nodedata->parent_changing, p_stats);
						
			return best;
		}
	}
	else{
			dag->preferred_parent = best; //Cache the value
			(dag->preferred_parent) -> local_confidence ++;
			printf("[RPL_SELECT_PARENT] Node %d has changed the preferred parent to %d.\n", c->node, best->id);
			//printf("[RPL_SELECT_PARENT] Rank of preferred parent changed to %d\n", best->rank);
	
		
			//Update the DAG rank, since link-layer information may have changed the local confidence.
			dag->rank = dag->of->calculate_rank(c, best, 0);
			printf("[RPL_SELECT_PARENT] Node %d New Rank after re-calculating: %d\n", c->node, dag->rank);

			
			//for residual_energy metric
			/*if(dag->of->ocp == 6){
				if(nodedata->energy_residual < best->path_energy_residual)
					dag->path_energy_residual = nodedata->energy_residual;
				else
					dag->path_energy_residual = best->path_energy_residual;
			}	*/	
			
			
			//reset dio timer
			//printf("[RPL_SELECT_PARENT] Node %d will reset dio timer.\n", c->node);
			rpl_reset_dio_timer(c, dag, 0, 1);
			
			//stats
			nodedata->change_parent++;
			if(get_time() > START_TIME)
				nodedata->stats_change_parent++;
			p_stats->parent = (dag->preferred_parent)->id;
			p_stats->etx = (dag->preferred_parent)->etx_4_elt;
			p_stats->time = get_time();
			p_stats->my_rank = dag->rank;
			das_insert(nodedata->parent_changing, p_stats);
						
			return best;
	}	
	//Update the DAG rank, since link-layer information may have changed the local confidence.
	dag->rank = dag->of->calculate_rank(c, best, 0);	
	
	
	return best;
}




/************************************************************************/

//remove a parent from the parents set
void rpl_remove_parent(call_t *c, nodedata_t *nodedata, rpl_dag_t *dag, rpl_parent_t* p){
	rpl_parent_t *q;
	bottleneck_t *my_b;
	
	if(rpl_find_parent(c, p->id) == NULL)
		return;
	
	//if preferred parent, delete it and rechoose another one;
	if(dag->preferred_parent == p){
		dag->preferred_parent = NULL;
		das_delete(nodedata->parents, p);		
		q = rpl_select_parent(c, nodedata, dag);
		if(q){
			printf("[RPL_REMOVE_PARENT] Node %d chose preferred parent to be %d after removing parent %d.\n", c->node, q->id, p->id);
			
			das_init_traverse(nodedata->my_bottlenecks);
			while((my_b = (bottleneck_t*) das_traverse(nodedata->my_bottlenecks)) != NULL){	
				//my_b->ratio_of_traffic = 0;
				das_delete(nodedata->my_bottlenecks, my_b);
			} 
			
			//printf("[RPL_REMOVE_PARENT] Node %d has will now compute the traffic loads\n", c->node);
			compute_loads(c, dag);
	
			//update my bottlenecks to the current parents in my list
			update_my_bottlenecks(c,dag);
		}
		else{
			printf("[RPL_LOCAL_REPAIR_AFTER_REMOVING_PREF_PARENT]Node %d Starting a local DAG repair\n", c->node);
			rpl_local_repair(c, nodedata,dag);
		}
			
	}
	else{
		das_delete(nodedata->parents, p);	
		printf("[RPL_REMOVE_PARENT] Node %d deleted parent %d (not the preferred parent).\n", c->node, p->id);
		
		das_init_traverse(nodedata->my_bottlenecks);
		while((my_b = (bottleneck_t*) das_traverse(nodedata->my_bottlenecks)) != NULL){	
			das_delete(nodedata->my_bottlenecks, my_b);
		} 
			
		//printf("[RPL_REMOVE_PARENT] Node %d has will now compute the traffic loads\n", c->node);
		compute_loads(c, dag);
	
		//update my bottlenecks to the current parents in my list
		update_my_bottlenecks(c,dag);	
	}
}

/************************************************************************/

/* Remove DAG parents (with a rank that is at least the same as minimum_rank). */
/* Remove ALL if minimum_rank = 0 */
void rpl_remove_parents(call_t *c, nodedata_t *nodedata, rpl_dag_t *dag){
	rpl_parent_t *p;

	//printf("RPL: Removing parents (minimum rank %u)\n", minimum_rank);
	das_init_traverse(nodedata->parents);
	while((p = (rpl_parent_t*) das_traverse(nodedata->parents)) != NULL){
		//nullify the preferred parent
		if(p == dag->preferred_parent)
			dag->preferred_parent = NULL;
		das_delete(nodedata->parents, p);		
	}
	
	printf("[RPL_REMOVE_PARENTS]: The parents of %d have been removed.\n", c->node);
}



/************************************************************************/

//returns the objective function
rpl_of_t *rpl_find_of(rpl_ocp_t ocp){
  rpl_of_t *of;

  for(of = objective_functions[0]; of != NULL; of++) {
  	//printf("for of %d\n", of->ocp);
    if(of->ocp == ocp) {
      return of;
    }
  }

  return NULL;
}

/************************************************************************/





/************************************************************************/
//                     DAG OPERATION
/************************************************************************/

//local repair (poisoning method): set INFINITE RANK and annonce everybody, then rechoose parents 
void rpl_local_repair(call_t *c, nodedata_t *nodedata, rpl_dag_t *dag){
	printf("[RPL_LOCAL_REPAIR] %d Starting a local DAG repair\n", c->node);
	
	dag->rank = INFINITE_RANK;
	rpl_remove_parents(c, nodedata, dag);
	nodedata->after_bootstrap == 0;
	
	//annonce INFINITE RANK
	rpl_reset_dio_timer(c, dag, 0, 1);
	
	//send DIS
	send_dis(c, NULL);

	//schedule select preferred parent in CHOOSE_PREFERRED_PARENT seconds			
	scheduler_add_callback(get_time()+(CHOOSE_PREFERRED_PARENT/2), c, rpl_bootstrap_select_preferred_parent, NULL);			
}

/**************************************************************************************/

//Function that recalculates rank of DAG according to new arrived neighbor
int rpl_process_parent_event(call_t *c, nodedata_t *nodedata, rpl_dag_t *dag, rpl_parent_t *p){
	
	if(p->rank == INFINITE_RANK) {
	 	//This parent is no longer valid */
		printf("[PROCESS_PARENT_EVENT]: Node %d Parent %d is no longer valid. \n", c->node, p->id);
	}
	
	if(rpl_select_parent(c, nodedata, dag) == NULL) {
		// No suitable parent; trigger a local repair.
		printf("[PROCESS_PARENT_EVENT] Node %d no preferred parent found. Trigger local repair.\n", c->node);
		rpl_local_repair(c, nodedata, dag);
		return 1;
	}
	else{
		//printf("\t[RPL] %d DIO received. \n",c->node);
	}
	
	return 0;
}

/************************************************************************/

//Global repair which can be triggered only by the root
void global_repair(call_t *c, nodedata_t *nodedata, rpl_dag_t *dag){	
	dag->version++;
	//dag->dtsn_out = 1;
	dag->of->reset(dag);
	
	uint64_t delay = 0;
	rpl_reset_dio_timer(c, dag, delay, 1);
		
	//TODO - DAO

	printf("[RPL_GLOBAL_REPAIR]: Root triggered global repair (version=%u, rank=%hu)\n",dag->version, dag->rank);
}

/************************************************************************/


/*void verify_reachability(call_t *c, void *arg){
	nodedata_t *nodedata = get_node_private_data(c);
	nud_t *nud = (nud_t *)arg;
	rpl_dag_t *dag = rpl_get_dag(nodedata, (nud->parent)->instance_id);
	//if(nud->parent == dag->preferred_parent && nud->last_dio == (nud->parent)->last_dio){
	//	send_dis(c, nud->parent);
	//}
		

}*/

void check_parent(call_t *c, rpl_dio_t *dio){
	nodedata_t *nodedata = get_node_private_data(c);
	
	bottleneck_t *b; 
	dio_bottleneck_t *bp;
	double max_elt;
	double max_elt_me;
	double b_elt;
	
	max_elt = 0;
	das_init_traverse(dio->bottlenecks);
	while((bp = (dio_bottleneck_t*) das_traverse(dio->bottlenecks)) != NULL){			
		das_init_traverse(nodedata->my_bottlenecks);
		while((b = (bottleneck_t*) das_traverse(nodedata->my_bottlenecks)) != NULL){			
			if(bp->bottlen_id == b->bottlen_id)
				return;
		}
		
		//remember which bottleneck from DIO has the biggest ELT, if i will send my traffic to it
		if(bp->bottlen_id == 0)
			b_elt = MAX_BOTTLEN_CONST;
		else
			b_elt = ((double)(bp->bottlen_const * MAX_BOTTLEN_CONST) /65535) / ((((float)bp->ratio_of_traffic / 255) * nodedata->my_traffic) + ((float)(bp->bottlen_traffic * MAX_TRAFFIC) / 255)); //un-normalized value
			//b_elt = bp->bottlen_const / ((bp->ratio_of_traffic * nodedata->my_traffic) + bp->bottlen_traffic); //old values, before i applied a normalization
		if(b_elt > max_elt)
			max_elt = b_elt;
	}
	
	// if i'm here, it means the sender of the DIO has at least one bottleneck that is not in my list of bottlenecks
	max_elt_me = 0;
	das_init_traverse(nodedata->my_bottlenecks);
	while((b = (bottleneck_t*) das_traverse(nodedata->my_bottlenecks)) != NULL){			
		//compute ELT of my bottlenecks without my traffic, to see if changing the preferred parent is really a benefit
		if(b->bottlen_id != c->node)
			b_elt = b->bottlen_const / (b->bottlen_traffic - (b->ratio_of_traffic * nodedata->my_traffic));
		else	
			//if b = nodedata (c->node) we'll have a division by zero
			b_elt = b->bottlen_const / b->bottlen_traffic;	
		
		if(b_elt > max_elt_me)
			max_elt_me = b_elt;
	}
	
	
	//if the other bottleneck is better => local repair to re-choose my preferred parent
	if(max_elt_me >= max_elt)
		return;
	else{
		//re-choose preferred parent
		printf("\n\n>>>>>>>>>>>>>>>>>>>>>>>[CHECK_PARENT]Node %d re-choosing preferred parent and increasing the Rank\n\n", c->node);
		rpl_dag_t *dag = rpl_get_dag(nodedata, RPL_ANY_INSTANCE);	
		rpl_local_repair(c, nodedata, dag);	
	}
}


/************************************************************************/

//When I receive a dio: OK january 2014
void rpl_process_dio(call_t *c, int from, rpl_dio_t *dio){
	nodedata_t *nodedata = get_node_private_data(c);
	rpl_dag_t *dag;
	rpl_parent_t *p;
		
	//join a DAG
	dag = rpl_get_dag(nodedata, dio->instance_id);
	
	//if(c->node == 4 && from == 5)
	//	return;
	
	//verify if i have already selected my preferred parent
	//if not, i will just join the DAG, add the sender in the parent list and schedule the selecting of the preferred parent
	if(nodedata->after_bootstrap == 0){
		if(dag == NULL) {
			//Join the first possible DAG of this RPL instance.
			if(dio->rank != INFINITE_RANK) {
				printf("[RPL_PROCESS_DIO] Node %d received DIO from = %d -> join first DAG. Wait %llu seconds then join choose preferred parent\n",c->node, from, CHOOSE_PREFERRED_PARENT);
				rpl_join_dag(c, nodedata, from, dio);
			
				//wait x sec then choose preferred parent, x = CHOOSE_PREFERRED_PARENT
				scheduler_add_callback(get_time()+CHOOSE_PREFERRED_PARENT, c, rpl_bootstrap_select_preferred_parent, NULL);			
			}
			return;
		}
		else{
			p = rpl_find_parent(c, from);
			if(p == NULL) {  //if not, add the parent
				// Add the DIO sender as a candidate parent. 
				p = rpl_add_parent(c, nodedata, dag, dio->rank, from, dio);				
			}
			else{	//received DIO from existing parent -> update infos
				p->rank = dio->rank;
				//p->path_energy_residual = dio->path_energy_residual;

				//update list of bottlenecks of corresponding parent
				//printf("[rpl_process_dio_update_parent_still_before_bootstrap] Node %d has will update bottlenecks list of parent %d\n", c->node, p->id);
				update_bottlenck_list_parent(c, dag, p, dio);
			}
			return;
		}
	}
	
	//if I'm here, it means that I have passed the bootstrap period
	
	if(dag == NULL) {
		//Join the first possible DAG of this RPL instance.
		if(dio->rank != INFINITE_RANK) {
			//printf("[RPL_PROCESS_DIO] Node %d from = %d - join first DAG\n",c->node,from);
			rpl_join_dag(c, nodedata, from, dio);
			
			//wait x sec then choose preferred parent, x = CHOOSE_PREFERRED_PARENT
			scheduler_add_callback(get_time(), c, rpl_bootstrap_select_preferred_parent, NULL);			
		} 
		else{ 
			printf("[RPL_PROCESS_DIO]: %d Ignoring DIO from node = %d with infinite rank:\n", c->node, from);
		}
		return;
	}
			
//	printf("[RPL_PROCESS_DIO]: Node %d with rank %d received DIO \n", c->node, dag->rank);

	
	if(memcmp(&dag->dag_id, &dio->dag_id, sizeof(dag->dag_id))) {
		printf("[RPL_PROCESS_DIO]: %d Ignoring DIO for another DAG within our instance\n", c->node);
		return;
	}
	
	if(dio->version > dag->version) {
		if(dag->rank == ROOT_RANK) {
			printf("[RPL_PROCESS_DIO]: Root received inconsistent DIO version number. Global repair started.\n");
			global_repair(c, nodedata, dag); 
			
		} else {
			printf("[RPL_PROCESS_DIO]: %d newer version received => inconsistency detected. Timer will be reset.\n", c->node);
			dag->version = dio->version;
			uint64_t delay = 0;
			rpl_reset_dio_timer(c, dag, delay, 1);		
		}
		return;
	} 
	else if(dio->version < dag->version) {
		// Inconsistency detected - someone is still on old version 
		printf("[RPL_PROCESS_DIO]: %d old version received => inconsistency detected. Timer will be reset.\n", c->node);
		uint64_t delay = 0;
		rpl_reset_dio_timer(c, dag, delay, 1);
		return;
	}
	
	if(dio->rank == INFINITE_RANK) {
		//if DIO was from one of my parents then remove parent 
		//(if preferred parent, rechoose parent and reset trickle) 
		//else ignore 
		p = rpl_find_parent(c, from);
		if(p == NULL){ 
			printf("[RPL_PROCESS_DIO]: %d Ignoring DIO from node = %d with infinite rank:\n", c->node, from);
			return;
		}
		else{
			printf("[RPL_PROCESS_DIO]: %d will delete parent = %d because received DIO with infinite rank:\n", c->node, from);
			rpl_remove_parent(c, nodedata, dag, p);
			return;
		}
	}
	
	if(dag->rank == ROOT_RANK) {
		if(dio->rank != INFINITE_RANK) { //received consistent DIO
			dag->dio_counter++; 
			//printf("[RPL_PROCESS_DIO]: Node %d with rank %d received consistent DIO \n", c->node, dag->rank);
		}
		return;
	}
	
	//RFC: if dio from parent with bigger rank than myself, then ignore DIO
	//modif: not ignore, but check if it doesn't have better bottlenecks
	if(dio->rank >= dag->rank){
		//check if this is not a better parent
		//printf("[RPL_PROCESS_DIO] Node %d has to CHECK parent\n\n\n\n\n", c->node);
		check_parent(c, dio);
		return;
	}
	
	
	
	/*At this point, we know that this DIO belongs to a DAG that we are already part of. 
	We consider the sender of the DIO to be a candidate parent, and let rpl_process_parent_event decide
	whether to keep it in the set.*/
	//printf("\t[RPL_PROCESS_DIO]: Node %d received DIO seq %d from %d\n", c->node, dio->nr_sequence, from);
	//printf("process dio\n");
	
	
	//Search if we already received a DIO from this parent
	p = rpl_find_parent(c, from);
	if(p == NULL) {  //if not, add the parent and compute the traffic load to send to him
		// Add the DIO sender as a candidate parent. 
		p = rpl_add_parent(c, nodedata, dag, dio->rank, from, dio);
		if(p == NULL) {
			//printf("RPL: Failed to add a new parent: %d \n",from);
			return;			
		}
		else{			
			if(dag->preferred_parent){
				//compute traffic load
				printf("[rpl_process_dio_new_parent] Node %d has will now compute the traffic loads\n", c->node);
				compute_loads(c, dag);
	
				//update my bottlenecks to the current parents in my list
				update_my_bottlenecks(c,dag);
		
				//printf my bottlenecks
				/*bottleneck_t* bp;
				printf("[rpl_process_dio_new_parent] Node %d has the following bottlenecks\n", c->node);
				das_init_traverse(nodedata->my_bottlenecks);
				while((bp = (bottleneck_t*) das_traverse(nodedata->my_bottlenecks)) != NULL){	
						printf("[rpl_process_dio_new_parent] b_id=%d with ratio of traffic=%f\n", bp->bottlen_id, bp->ratio_of_traffic);
				}*/
			}
			else
				scheduler_add_callback(get_time(), c, rpl_bootstrap_select_preferred_parent, NULL);			

			return;
		}
	} 
	
	else{	
		p->instance_id = dio->instance_id;
		if(p->rank == dio->rank) {
			//printf("[RPL_PROCESS_DIO]: Node %d received consistent DIO\n", c->node);
			dag->dio_counter++;
		}
		else{
			p->rank = dio->rank;
			//printf("[RPL_PROCESS_DIO]: Node %d's Parent %d changed rank to %d.\n", c->node, p->id, p->rank);
		}
		
		//received DIO from existing parent -> update infos
		//update list of bottlenecks of corresponding parent
		//printf("[rpl_process_dio_update_parent] Node %d has will update bottlenecks list of parent %d\n", c->node, p->id);
		update_bottlenck_list_parent(c, dag, p, dio);
		
		if(dag->preferred_parent){
			//compute traffic load
			//printf("[rpl_process_dio_new_parent] Node %d has will now compute the traffic loads\n", c->node);
			compute_loads(c, dag);
	
			//update my bottlenecks to the current parents in my list
			update_my_bottlenecks(c,dag);
		
			//printf my bottlenecks
			/*bottleneck_t* bp;
			printf("[rpl_process_dio_new_parent] Node %d has the following bottlenecks\n", c->node);
			das_init_traverse(nodedata->my_bottlenecks);
			while((bp = (bottleneck_t*) das_traverse(nodedata->my_bottlenecks)) != NULL){	
					printf("[rpl_process_dio_new_parent] b_id=%d with ratio of traffic=%f\n", bp->bottlen_id, bp->ratio_of_traffic);
			}*/
		}
		else
			scheduler_add_callback(get_time(), c, rpl_bootstrap_select_preferred_parent, NULL);	
		return;
	}	
	
	//here: new parent OR same parent same rank OR same parent new rank	
	//process the DIO further.
	//if(rpl_process_parent_event(c, nodedata, dag, p) == 0) {
		//printf("The candidate parent %d is rejected. rank = %d \n", p->id, p->rank);
		//return;
	//}
}





/************************************************************************/

