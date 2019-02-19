#include <limits.h>
#include <string.h>
#include <stdint.h>

#include "rpl.h"

/************************************************************************/
extern rpl_of_t rpl_of_energy;
extern rpl_of_t rpl_of_residual;
extern rpl_of_t rpl_of_elt;
extern rpl_of_t rpl_of_etx;
extern rpl_of_t rpl_of0;

static rpl_of_t *objective_functions[] = {&rpl_of_etx, &rpl_of_elt, &rpl_of0, &rpl_of_residual, &rpl_of_energy, NULL};
/************************************************************************/





/************************************************************************/
//                     DAG OPERATION
/************************************************************************/

//Sink nodes should execute this code
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
	
	
	//info about the bottleneck; the root does not transmit packets nor has any parents
	dag->bottlen_const 		= (nodedata->energy_residual * DATA_RATE) / (1 * MY_RADIO_TX); //(START_ENERGY * DATA_RATE)	/ (ETX_4_ELT_GUESS * MY_RADIO_TX)	//(E_res * DATA_RATE) / (ETX * 1b/s * Etx)
	dag->bottlen_traffic	= 1;	 // 1b/s
	dag->bottlen_id			= c->node;
	
	//infor for residual energy metric
	dag->path_energy_residual = nodedata->energy_residual;
	//printf("[RPL_SET_ROOT] DAG root set for node = %d, bottleneck const = %lf traffic %f\n", c->node, dag->bottlen_const, dag->bottlen_traffic);

	//send first dio
	if(nodedata->type == SINK){ 
		scheduler_add_callback(get_time(), c, send_dio, NULL);
		rpl_reset_dio_timer(c, dag, 0, 1);
	}
	
	return dag;
}

/************************************************************************/

//Node receiving first DIO message should execute this
void rpl_join_dag(call_t *c, nodedata_t *nodedata, int from, rpl_dio_t *dio){
	rpl_dag_t *dag;
	rpl_parent_t *p;
	rpl_of_t *of;
	
	dag = rpl_alloc_dag(nodedata, dio->instance_id);
	if(dag == NULL) {
		printf("RPL: Failed to allocate a DAG object!\n");
		return;
	}
	
	p = rpl_add_parent(c, nodedata, dag, dio->rank, from, dio);
	printf("[RPL_JOIN_DAG] Node %d adding %d as a parent. \n", c->node,from);	
		
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
	dag->preferred_parent 	= p;


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

	
	//printf("before calculate rank p=%d dio_rank=%d ocp %d\n", p->addr, dio->rank, of->ocp);
	dag->rank = dag->of->calculate_rank(c, p, dio->rank);
	//printf("Printf new rank \n");
	
	
	//information for residual energy metric
	if(dio->path_energy_residual < nodedata->energy_residual)
		dag->path_energy_residual = dio->path_energy_residual;
	else
		dag->path_energy_residual = nodedata->energy_residual;
		
		
	//if when i join the dodag i become the bottleneck 
	//if(c->node == 2)printf("call from rpl_join_dag\n");
	if(calculate_bottlen_elt(c, p) > calculate_my_elt(c,p)){  
		dag->bottlen_const	 	= (nodedata->energy_residual * DATA_RATE) / ((dag->preferred_parent)->etx_4_elt * MY_RADIO_TX);
		dag->bottlen_traffic	= 1;
		dag->bottlen_id			= c->node;
	}
	else{
		dag->bottlen_const	 	= dio->bottlen_const;
		dag->bottlen_traffic	= dio->bottlen_traffic;
		dag->bottlen_id			= dio->bottlen_id;
	}
	
	
	printf("New rank: %d\n", dag->rank);
	dag->min_rank = dag->rank; // So far this is the lowest rank we know 
	
	dag->dio_timer = NULL;
	rpl_reset_dio_timer(c, dag, 0, 1);
	
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
	//printf("RPL: Leaving the DAG ");
	//printf("%d", dag->dag_id);
	//printf("\n");
	
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

//add parent after receiving DIO from it
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
	
	//if ELT
	p->bottlen_const	= dio->bottlen_const;
	p->bottlen_traffic 	= dio->bottlen_traffic;
	p->bottlen_id 		= dio->bottlen_id;
	
	//if residual_energy
	//info about minimum residual energy on the path
	p->path_energy_residual = dio->path_energy_residual;

	//for the energy metric (etx+residual)
  	p->residual_energy = dio->residual_energy;	
	
							
	//printf("[RPL_ADD_PARENT]Node %d added parent %d etx = %f rank =%d. \n", c->node, p->id, p->etx, rank);															
								
	//actually add parent in parents list
 	das_insert(nodedata->parents, p);
 		 	
 	/*rpl_parent_t *q;	 	
 	if(c->node == 2 && das_getsize(nodedata->parents) > 0){
		printf("[RPL_ADD_PARENT] Node %d has parents:\n", c->node);
		das_init_traverse(nodedata->parents);
		while((q = (rpl_parent_t*) das_traverse(nodedata->parents)) != NULL)
			printf("\t %d etx= %f; \n", q->id, q->etx);
			
		if(dag->preferred_parent != NULL)
			printf("Preferred parend is: %d.", dag->preferred_parent->id);
		
		printf("\n");
	}
 	*/
 
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

//Chose PREFERRED parent among ones that have reported DIO already!!!
rpl_parent_t *rpl_select_parent(call_t *c, nodedata_t *nodedata, rpl_dag_t *dag){
	rpl_parent_t *p, *best, *current;
	best = NULL;
	
	parent_stats *p_stats = (parent_stats *)malloc(sizeof(parent_stats));
		
	if(das_getsize(nodedata->parents) == 0){
		//printf("[RPL_SELECT_PARENT] Node %d has parents' set empty.\n", c->node);
		return best;
	}
	
	current = dag->preferred_parent; //get current PREFERRED parent
	
	//Find BEST parent
	das_init_traverse(nodedata->parents);
	while((p = (rpl_parent_t*) das_traverse(nodedata->parents)) != NULL){		
		if(best == NULL) {
			//Verify that we received DIO from this parent
			if (p->id != -1 && p->ignore == 0){
				if(dag->of->ocp == 1){
					if(p->etx_4_elt < 10)
						best = p;
				}
				else
					best = p;
			}			
		}
		else {
			//Verify that we received DIO from this parent
			if (p->id != -1 && p->ignore == 0){ 
				if(dag->of->ocp == 1){
					if(p->etx_4_elt < 10)
						best = dag->of->best_parent(c, best, p);
				}
				else
					best = dag->of->best_parent(c, best, p);
				//printf("rpl_selec_parent best is %d\n", best->addr);
			}
		}
	}
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
			
			//for ELT: update bottleneck 
			if(calculate_bottlen_elt(c, best) > calculate_my_elt(c,best)){  //if i'm the new bottlenck
				//if(c->node == 2)printf("call from rpl_select_best_parent\n");
				dag->bottlen_const	 	= (nodedata->energy_residual * DATA_RATE) / ((dag->preferred_parent)->etx_4_elt * MY_RADIO_TX);
				dag->bottlen_traffic	= nodedata->my_traffic;
				dag->bottlen_id			= c->node;
				printf("Node %d has bottleneck id %d bottlen_const %f\n", c->node, dag->bottlen_id, dag->bottlen_const);
				
			}
			else{
				//if(c->node == 2)printf("call from rpl_select_best_parent\n");
				dag->bottlen_const	 	= best->bottlen_const;
				dag->bottlen_traffic	= best->bottlen_traffic;
				dag->bottlen_id			= best->bottlen_id;
				printf("Node %d has bottleneck id %d bottlen_const %f \n", c->node, dag->bottlen_id, dag->bottlen_const);

			}
			
			
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
		} //end if "i changed my preferred parent"
		else{
			//for ELT: i did not change my preferred parent, still, update bottleneck info
			if(calculate_bottlen_elt(c, best) > calculate_my_elt(c,best)){  //if i'm the new bottlenck
				//if(c->node == 2)printf("call from rpl_select_best_parent\n");
				dag->bottlen_const	 	= (nodedata->energy_residual * DATA_RATE) / ((dag->preferred_parent)->etx_4_elt * MY_RADIO_TX);
				dag->bottlen_traffic	= nodedata->my_traffic;
				dag->bottlen_id			= c->node;
				printf("Node %d has bottleneck id %d bottlen_const %f\n", c->node, dag->bottlen_id, dag->bottlen_const);
			}
		}
		
		//if i changed or didn't changed the preferred parent, update information
		//for residual_energy metric
		if(dag->of->ocp == 6){
			if(nodedata->energy_residual < best->path_energy_residual)
				dag->path_energy_residual = nodedata->energy_residual;
			else
				dag->path_energy_residual = best->path_energy_residual;
			//printf("\t[RPL_SELECT_PARENT]Node %d has residual energy %f and dag has path energy %f\n", c->node, nodedata->energy_residual,  dag->path_energy_residual);
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
			
			//for ELT: update bottleneck 
			if(calculate_bottlen_elt(c, best) > calculate_my_elt(c,best)){  //if i'm the new bottlenck
				//if(c->node == 2)printf("call from rpl_select_best_parent\n");
				dag->bottlen_const	 	= (double)(nodedata->energy_residual * DATA_RATE) / ((dag->preferred_parent)->etx_4_elt * RADIO_TX);
				dag->bottlen_traffic	= nodedata->my_traffic;
				dag->bottlen_id			= c->node;
				printf("Node %d has bottleneck id %d bottlen_const %f\n", c->node, dag->bottlen_id, dag->bottlen_const);
			}
			else{
				//if(c->node == 2)printf("call from rpl_select_best_parent\n");
				dag->bottlen_const	 	= best->bottlen_const;
				dag->bottlen_traffic	= best->bottlen_traffic;
				dag->bottlen_id			= best->bottlen_id;
				printf("Node %d has bottleneck id %d bottlen_const %f\n", c->node, dag->bottlen_id, dag->bottlen_const);

			}
			
			//for residual_energy metric
			if(dag->of->ocp == 6){
				if(nodedata->energy_residual < best->path_energy_residual)
					dag->path_energy_residual = nodedata->energy_residual;
				else
					dag->path_energy_residual = best->path_energy_residual;
				//printf("\t[RPL_SELECT_PARENT]Node %d has residual energy %f and best has path energy %f dag energy is %f\n", c->node, nodedata->energy_residual,  best->path_energy_residual, dag->path_energy_residual);
			}		
			
			
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
	
	//TODO - DAO
			
	//if(c->node ==2 && best != NULL)
	//	printf("\t[RPL_SELECT_PARENT] Best parent is: %d rank %d\n", best->id, best->rank);
	
	return best;
}

/************************************************************************/

//remove a parent from the parents set
void rpl_remove_parent(call_t *c, nodedata_t *nodedata, rpl_dag_t *dag, rpl_parent_t* p){
	rpl_parent_t *q;
	
	if(rpl_find_parent(c, p->id) == NULL)
		return;
		
	//if preferred parent, delete it and rechoose another one;
	if(dag->preferred_parent == p){
		dag->preferred_parent = NULL;
		das_delete(nodedata->parents, p);		
		if(das_getsize(nodedata->parents) == 0){
			rpl_local_repair(c, nodedata, dag);
			return;
		}
		q = rpl_select_parent(c, nodedata, dag);
		if(q){
			printf("[RPL_REMOVE_PARENT] Node %d chose preferred parent to be %d. Resetting DIO timer\n", c->node, q->id);
			rpl_reset_dio_timer(c, dag, 0, 1);
		}
		else{
			rpl_local_repair(c, nodedata, dag);
		}

	}
	else{
		das_delete(nodedata->parents, p);	
		printf("[RPL_REMOVE_PARENT] Node %d deleted parent %d (not the preferred parent).\n", c->node, p->id);
		if(das_getsize(nodedata->parents) == 0)
			rpl_local_repair(c, nodedata, dag);
	}
}

/************************************************************************/

/* Remove DAG parents (with a rank that is at least the same as minimum_rank). */
/* Remove ALL if minimum_rank = 0 */
void rpl_remove_parents(call_t *c, nodedata_t *nodedata, rpl_dag_t *dag){
	rpl_parent_t *p;// = (rpl_parent_t *) malloc(sizeof(rpl_parent_t));

	if(das_getsize(nodedata->parents) == 0){	//no parents to remove
		return ;
	}


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
	
	//annonce INFINITE RANK
	rpl_reset_dio_timer(c, dag, 0, 1);
	
	//send DIS
	send_dis(c, NULL);
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
		//printf("[PROCESS_PARENT_EVENT]Node %d has parent %d\n", c->node, dag->preferred_parent->id);
		//printf("\t\t\t>>>>>>>>>>>>>>>>>>>>>>>>>Verifying my parent selection!!!!!!!!!!! Re-selecting preferred parent, re-computing ELT\n\n");
	}
	
	return 1;
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

/************************************************************************/

//When I receive a dio:
void rpl_process_dio(call_t *c, int from, rpl_dio_t *dio){
	nodedata_t *nodedata = get_node_private_data(c);
	rpl_dag_t *dag;
	rpl_parent_t *p;
		
	//join a DAG
	dag = rpl_get_dag(nodedata, dio->instance_id);
	if(dag == NULL) {
		//Join the first possible DAG of this RPL instance.
		if(dio->rank != INFINITE_RANK) {
			//printf("[RPL_PROCESS_DIO] Node %d from = %d - join first DAG\n",c->node,from);
			rpl_join_dag(c, nodedata, from, dio);
		} 
		else 
			printf("[RPL_PROCESS_DIO]: %d Ignoring DIO from node = %d with infinite rank:\n", c->node, from);
		
		return;
	}
		
	//if(c->node == 4 && from ==7)	
	//	printf("[RPL_PROCESS_DIO]: Node %d with rank %d received DIO from %d with bottleneck_id %d my_bottlenck %d\n", c->node, dag->rank, from, dio->bottlen_id, dag->bottlen_id);

	
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
			//ISPRAVI - delay ???
			uint64_t delay = 0;
			rpl_reset_dio_timer(c, dag, delay, 1);		
		}
		return;
	} 
	else if(dio->version < dag->version) {
		// Inconsistency detected - someone is still on old version 
		printf("[RPL_PROCESS_DIO]: %d old version received => inconsistency detected. Timer will be reset.\n", c->node);
		//ISPRAVI - delay = ???
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
	
	//ELT - if dio from parent with bigger rank than myself, then ignore DIO
	if(dag->of->ocp == 5){
		if(dio->rank >= dag->rank)
			return;
	}
	
	
	/*At this point, we know that this DIO belongs to a DAG that we are already part of. 
	We consider the sender of the DIO to be a candidate parent, and let rpl_process_parent_event decide
	whether to keep it in the set.*/
	//printf("\t[RPL_PROCESS_DIO]: Node %d received DIO from %d\n", c->node, from);
	//printf("process dio\n");
	//Search if we already received a DIO from this parent
	p = rpl_find_parent(c, from);
	if(p == NULL) {
		// Add the DIO sender as a candidate parent. 
		p = rpl_add_parent(c, nodedata, dag, dio->rank, from, dio);
		if(p == NULL) {
			//printf("RPL: Failed to add a new parent: %d \n",from);
			return;			
		}
		else{
			//p->lqi = (1 / (dio->rxdBm + EDThresholdMin)) * (RcvMaxSignal + EDThresholdMin);
			//if(p->lqi < 0)
			//	printf("[RPL_PROCESS DIO] Node %d parent %d added rxdBm = %f, lqi calculated = %f \n", c->node, p->id, dio->rxdBm, p->lqi);	
		}
	} 
	else{
		//re-calculating etx of the link to the parent p
		//if((p->last_dio + 1) != dio->nr_sequence){ 
			//printf("[RPL_RECEIVED_DIO]Node %d to parent %d last_dio sequence %d new_dio sequence %d \n",c->node, p->id, p->last_dio, dio->nr_sequence);
		//	float new_etx = (float) dio->nr_sequence / (p->last_dio + 1); 
		//	p->etx = (LAMDA*p->etx) + ((1-LAMDA)*new_etx);
		//	p->last_dio = dio->nr_sequence;
			//printf("[RPL_RECEIVED_DIO]Node %d to parent %d new etx = %f rank = %d\n", c->node, p->id, p->etx, p->rank);
		//}
		//end recalculate ETX

		p->instance_id 		= dio->instance_id;
		p->bottlen_const	= dio->bottlen_const;
		p->bottlen_traffic 	= dio->bottlen_traffic;
		p->bottlen_id 		= dio->bottlen_id;
		if(p->rank == dio->rank) {
			//printf("[RPL_PROCESS_DIO]: Node %d received consistent DIO\n", c->node);
			dag->dio_counter++;
		}
		else{
			p->rank = dio->rank;
			//printf("[RPL_PROCESS_DIO]: Node %d's Parent %d changed rank to %d.\n", c->node, p->id, p->rank);
		}
	}	
	
	
	//here: new parent OR same parent same rank OR same parent new rank	
	//process the DIO further.
	if(rpl_process_parent_event(c, nodedata, dag, p) == 0) {
		//printf("The candidate parent %d is rejected. rank = %d \n", p->id, p->rank);
		return;
	}
	
	//if(c->node == 4 && from == 7)	
	//	printf("[RPL_PROCESS_DIO_after_processing]: Node %d with rank %d received DIO from %d with bottleneck_id %d my bottlenck %d\n", c->node, dag->rank, from, dio->bottlen_id, dag->bottlen_id);

}

/************************************************************************/
//calculate my ELT to a certain parent
double calculate_my_elt(call_t *c, rpl_parent_t *p){
	struct nodedata *nodedata = get_node_private_data(c);
	//if(c->node==2) printf("\t\t node %d, parent %d\n", c->node, p->addr);
	
	//get E_res = residual energy
    double total_consumption = 0;
    int j;
    for(j=0; j<RADIO_NB_MODES; j++){   
    	//printf("Mode %d consumption: %f\n", j, nodedata->radio_energy->consumption[j]);
        total_consumption   += nodedata->radio_energy->consumption[j]; 
  	}
  	//nodedata->energy_residual = nodedata->start_energy - (total_consumption / 3600 * 1000 / 4); //impartit la 3600 ca sa obtin din Joule -> Wh si apoi inmultesc cu 1000 ca sa am mWh si impart la 4V ca sa obtin mAh
  	if(c->node == 0) //SINK has infinite energy
  		nodedata->energy_residual = 1000 * nodedata->start_energy;
  	else
  		nodedata->energy_residual = nodedata->start_energy -  total_consumption;
  	//if(c->node==2) printf("\t\t total energy consumed %f, remaining %f (Joule)\n", total_consumption, nodedata->energy_residual);
	
	
	//get my_ELT in nanoseconds OK
	double my_ELT = (nodedata->energy_residual * DATA_RATE)/ (p->etx_4_elt * nodedata->my_traffic * MY_RADIO_TX);
	//if(c->node==2) printf("\t\t ELT is %f\n", my_ELT);
	
	//printf("\t\t\t>>>>>>>>>>>>>>>>>>>>>>>>> Node %d recomputes ELT!!!!!!!!!!! T\n\n", c->node);

	
	return my_ELT;
}



//calculate ELT to a certain parent and to the bottleneck on that path and return the minimum between the two
double calculate_bottlen_elt(call_t *c, rpl_parent_t *p){
	struct nodedata *nodedata = get_node_private_data(c);
	//printf("node %d, parent %d\n", c->node, p->addr);
	
	
	//get traffic = nr sent of pakets * 1024 / sec (b/s)
	//nodedata->old_traffic = nodedata->my_traffic;
 	//printf("nodata_tx is %d and my traffic is %f /sec\n", nodedata->data_tx, nodedata->my_traffic);
	
	double bottlen_ELT = p->bottlen_const * (1 / (p->bottlen_traffic + nodedata->my_traffic));
	
	//if(c->node == 2) printf("\tnode %d to parent %d my bottleneck is %d ELT is: %f time %llu\n",c->node, p->addr, p->bottlen_id, bottlen_ELT, get_time() );

	return bottlen_ELT;
}



//compute ELT every VERIFY_BOTTLENECK seconds to see if the node did not become the new bottleneck
int verify_bottleneck(call_t *c){
	struct nodedata *nodedata = get_node_private_data(c);
	double elt_me, elt_bottlenck;
	rpl_dag_t *dag;
    dag = rpl_get_dag(nodedata, RPL_ANY_INSTANCE);
	
	
	if(dag != NULL){
		if(dag->preferred_parent != NULL){
			elt_me = calculate_my_elt(c, dag->preferred_parent);
			elt_bottlenck = calculate_bottlen_elt(c, dag->preferred_parent);
		
			if(elt_me < elt_bottlenck && dag->bottlen_id != c->node){ //i become the new bottleneck
				dag->bottlen_const	 	= (nodedata->energy_residual * DATA_RATE) / ((dag->preferred_parent)->etx_4_elt * MY_RADIO_TX);
				dag->bottlen_traffic	= nodedata->my_traffic;
				dag->bottlen_id			= c->node;
				//printf("^^^^^^^^^^^^^^^Node %d has become itself the bottleneck id %d bottlen_const %f time %llu\n\n", c->node, dag->bottlen_id, dag->bottlen_const, get_time());
			}
		}
	}
	
	//compute ELT every VERIFY_BOTTLENECK seconds
	//scheduler_add_callback(get_time() + VERIFY_BOTTLENECK, c, verify_bottleneck, NULL);

	return 0;
}



//for ELT: compute traffic every RECOMPUTE_TRAFFIC seconds OK
int compute_traffic(call_t *c, void *args){
	struct nodedata *nodedata = get_node_private_data(c);
	
	
	//compute traffic = nr of sent pakets * PACKET_SIZE / sec (b/s) 
	if(nodedata->data_tx == 0)
		nodedata->my_traffic = 1;
	else{
		//if(c->node==2)	printf("old traffic: %f", nodedata->my_traffic);
		float new_traffic = (float)(nodedata->data_tx - nodedata->old_tx) * PACKET_SIZE / (RECOMPUTE_TRAFFIC/1000000000);
		nodedata->my_traffic = (LAMDA_TRAFFIC * nodedata->my_traffic) + ((1 - LAMDA_TRAFFIC) * new_traffic);
		//printf("\t\t -------------------1 node %d nodata_tx is %d new_traffic is %f my traffic is %f  at time %llu \n", c->node, nodedata->data_tx, new_traffic, nodedata->my_traffic, get_time());
		nodedata->old_tx = nodedata->data_tx;
	}
	
	
	rpl_dag_t *dag;
	dag = rpl_get_dag(nodedata, RPL_ANY_INSTANCE);
	
	if(dag!=NULL)
		if(dag->of->ocp == 5)
			rpl_select_parent(c, nodedata, dag);
  	/*	
			//update info about the bottleneck if it's me
			if(dag->bottlen_id == c->node){
			
				//get E_res = residual energy
				double total_consumption = 0;
				int j;
				for(j=0; j<RADIO_NB_MODES; j++){   
					//printf("Mode %d consumption: %f\n", j, nodedata->radio_energy->consumption[j]);
				   total_consumption   += nodedata->radio_energy->consumption[j]; 
				}
				//nodedata->energy_residual = nodedata->start_energy - (total_consumption / 3600 * 1000 / 4); //impartit la 3600 ca sa obtin din Joule -> Wh si apoi inmultesc cu 1000 ca sa am mWh si impart la 4V ca sa obtin mAh
				if(c->node == 0) //SINK has infinite energy
					nodedata->energy_residual = 1000 * nodedata->start_energy;
				else
					nodedata->energy_residual = nodedata->start_energy -  total_consumption;
  				
				dag->bottlen_const	 	= (nodedata->energy_residual * DATA_RATE) / ((dag->preferred_parent)->etx_4_elt * MY_RADIO_TX);
				dag->bottlen_traffic	= nodedata->my_traffic;
				dag->bottlen_id			= c->node;
			}
		}
	}*/
	
	//compute traffic and ETX every RECOMPUTE_TRAFFIC seconds
	scheduler_add_callback(get_time() + RECOMPUTE_TRAFFIC, c, compute_traffic, NULL);

	return 0;
}



//for ELT and ETX: compute ETX every RECOMPUTE_TRAFFIC seconds for all parents in the parent set
int compute_etx(call_t *c, void *args){
	struct nodedata *nodedata = get_node_private_data(c);

	//get ETX = nr of retransmissions OK
	double new_etx_4_elt;
	etx_elt_t *mnr;								
	das_init_traverse(nodedata->mac_neigh_retransmit);
	while((mnr = (etx_elt_t*) das_traverse(nodedata->mac_neigh_retransmit)) != NULL){
	  rpl_parent_t *p   = rpl_find_parent(c, mnr->neighbor);
	  if(p != NULL){	
		int packs_net 	  = mnr->packs_net;// - p->old_etx->packs_net; 
		int packs_tx 	  = mnr->packs_tx;// - p->old_etx->packs_tx;
		int acks		  = mnr->acks;// - p->old_etx->acks;
		int drops		  = mnr->drops;// - p->old_etx->drops;
		int packs_tx_once =	mnr->packs_tx_once;// - p->old_etx->packs_tx_once;

		//printf("\n\n");
		
		if(packs_net == 0){
			new_etx_4_elt = p->etx_4_elt;
			//if(c->node == 2) 
			//printf("\t\t node %d data_tx from RPL is 0 in the time interval my new ETX is %f\n", c->node, p->etx_4_elt);
		}
		else
			{
			if(packs_tx == 0){
				if(drops <= 1)
					new_etx_4_elt = 1.5;
				else
					new_etx_4_elt = (double) drops;
				//if(c->node == 2) 
				//printf("1>>>>>>>>>>>>>>>>>>>>>>>>Node %d sent to node %d, %d packets on the radio, %d from RPL,  %d drops, %d acks ETX = %f.\n", c->node, mnr->neighbor, packs_tx, packs_net, drops, acks, new_etx_4_elt);			
			}
				
			else{
				if(acks == 0){ 
					new_etx_4_elt = (double) packs_tx;
					//if(c->node == 2) 
					//printf("2>>>>>>>>>>>>>>>>>>>>>>>>Node %d sent to node %d, %d packets on the radio, %d from RPL,  %d drops, %d acks ETX = %f.\n", c->node, mnr->neighbor, packs_tx, packs_net, drops, acks, new_etx_4_elt);			
				}
				else{ 
					if (packs_net == drops){
						new_etx_4_elt = ((double) packs_tx / acks) * (packs_net+1) / (packs_tx_once + 1);
						//if(c->node == 2) 
						//printf("3>>>>>>>>>>>>>>>>>>>>>>>>Node %d sent to node %d, %d packets on the radio, %d from RPL, %d packs_tx_once,  %d drops, %d acks ETX = %f.\n", c->node, mnr->neighbor, packs_tx, packs_net, packs_tx_once, drops, acks, new_etx_4_elt);			
					}
					else{
						new_etx_4_elt = ((double) packs_tx / acks) * (packs_net+1) / (packs_tx_once + 1);
						//if(c->node == 2) 
						//printf("4>>>>>>>>>>>>>>>>>>>>>>>>Node %d sent to node %d, %d packets on the radio, %d from RPL, %d packs_tx_once, %d drops, %d acks ETX = %f.\n", c->node, mnr->neighbor, packs_tx, packs_net, packs_tx_once, drops, acks, new_etx_4_elt);			
					}
				}
			}
			p->etx_4_elt = LAMDA_ETX * p->etx_4_elt + (1-LAMDA_ETX) * new_etx_4_elt;
			p->old_etx->packs_net = mnr->packs_net;
			p->old_etx->packs_tx = mnr->packs_tx;
			p->old_etx->acks = mnr->acks;
			p->old_etx->drops = mnr->drops;
			p->old_etx->packs_tx_once = mnr->packs_tx_once;
			//if(c->node == 2) 
			//printf("\t\t node %d my new ETX is %f\n", c->node, p->etx_4_elt);
		} //end else (paks_net == 0)
	  }//end if (p!= null)
	} //end while
	
	
	/*rpl_dag_t *dag;
	dag = rpl_get_dag(nodedata, RPL_ANY_INSTANCE);
	
	if(dag!=NULL)
		if(dag->of->ocp == 1)
			rpl_select_parent(c, nodedata, dag);
	*/
	
	scheduler_add_callback(get_time() + RECOMPUTE_ETX, c, compute_etx, NULL);
	return 0;
}



//for residual energy metric
int recompute_energy(call_t *c, void *args){
	struct nodedata *nodedata = get_node_private_data(c);
	
	//get E_res = residual energy
    double total_consumption = 0;
    int j;
    for(j=0; j<RADIO_NB_MODES; j++){   
    	//printf("Mode %d consumption: %f\n", j, nodedata->radio_energy->consumption[j]);
        total_consumption   += nodedata->radio_energy->consumption[j]; 
  	}
  	//nodedata->energy_residual = nodedata->start_energy - (total_consumption / 3600 * 1000 / 4); //impartit la 3600 ca sa obtin din Joule -> Wh si apoi inmultesc cu 1000 ca sa am mWh si impart la 4V ca sa obtin mAh
  	nodedata->energy_residual = nodedata->start_energy -  total_consumption;


	rpl_dag_t *dag;
	dag = rpl_get_dag(nodedata, RPL_ANY_INSTANCE);
	
	if(dag!=NULL)
		if(dag->of->ocp == 6)
			rpl_select_parent(c, nodedata, dag);
			
	scheduler_add_callback(get_time() + RECOMPUTE_RESIDUAL, c, recompute_energy, NULL);
	return 0;
}
