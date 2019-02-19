#include "rpl.h"

static void reset(void *);
static rpl_parent_t *best_parent(call_t *, rpl_parent_t *, rpl_parent_t *);
static rpl_rank_t calculate_rank(call_t *, rpl_parent_t *, rpl_rank_t);

rpl_of_t rpl_of_residual = {
  reset,
  best_parent,
  calculate_rank,
  6
};


static void reset(void *dag){
  printf ("[RPL_OF_RESET]: Resetting ELT\n");
}

static rpl_rank_t calculate_rank(call_t *c, rpl_parent_t *p, rpl_rank_t base_rank){
	rpl_dag_t *dag;
	rpl_rank_t new_rank;
	rpl_rank_t rank_increase;
	int node_energy;
	int step;
	
	struct nodedata *nodedata = get_node_private_data(c);

	//Parent was not passed as parameter i.e. first DIO reception
	if(p == NULL) {
		//Error prevention - if function was called without parameters
		if(base_rank == 0) {
			return INFINITE_RANK;
		}
		rank_increase = DEFAULT_MIN_HOPRANKINC;
	} 
	
	else {
		dag = (rpl_dag_t *)p->dag;
		
		node_energy = (nodedata->energy_residual * 255) / START_ENERGY;
		step = 255 - node_energy;
		rank_increase = step + dag->min_hoprankinc;		
		
		//We do this when we have new prefered parent and we want to calc our rank according to his
		if(base_rank == 0) {
			base_rank = p->rank;
			//printf("base rank == 0, p= %d rank_p = %d\n", p->id, p->rank);
		}
		//TODO: local confidence
	}
	
	if(INFINITE_RANK - base_rank < rank_increase) {
		/* Reached the maximum rank. */
		new_rank = INFINITE_RANK;
		printf("\t[RPL_ELT_CALCULATE_RANK] %d reached the maximum rank, base_rank=%d.\n", c->node, base_rank);
		printf("/**********************************************************************************************************************************************\n");

	} 
	else {
		 /* Calculate the rank based on the new rank information from DIO or
		 stored otherwise. */
		 

		 new_rank = base_rank + rank_increase;
	}

	return new_rank;
}



static rpl_parent_t *best_parent(call_t *c, rpl_parent_t *p1, rpl_parent_t *p2){
	rpl_dag_t *dag;
	//rpl_rank_t p1_rank;
	//rpl_rank_t p2_rank;
	
	dag = (rpl_dag_t *)p1->dag; /* Both parents must be in the same DAG. */
	
	//printf("\t[RPL_OF_residual] - Node %d comparing parent01 ID = %d having path energy %f with parent02 ID = %d having path energy %f \n", c->node, p1->id, p1->path_energy_residual, p2->id, p2->path_energy_residual);
	
	//recompute residual energy of a node
	
	double p1_residual, p2_residual;
	p1_residual = p1->path_energy_residual;
	p2_residual = p2->path_energy_residual;
	
	
	//printf("[RPL_PAR_BEST] - exit\n");
	/* Maintain stability of the preferred parent in case of similar ranks. */
	if(p1_residual == p2_residual) {
		if(p1 == dag->preferred_parent) {
			return p1;
		} 
		else if(p2 == dag->preferred_parent) {
			return p2;
		}
	}
	
	if(p1_residual > p2_residual) {
		return p1;
	}
	
	return p2;	
}
