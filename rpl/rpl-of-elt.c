#include "rpl.h"

static void reset(void *);
static rpl_parent_t *best_parent(call_t *, rpl_parent_t *, rpl_parent_t *);
static rpl_rank_t calculate_rank(call_t *, rpl_parent_t *, rpl_rank_t);

rpl_of_t rpl_of_elt = {
  reset,
  best_parent,
  calculate_rank,
  5
};


static void reset(void *dag){
  printf ("[RPL_OF_RESET]: Resetting ELT\n");
}

static rpl_rank_t calculate_rank(call_t *c, rpl_parent_t *p, rpl_rank_t base_rank){
	rpl_dag_t *dag;
	rpl_rank_t new_rank;
	rpl_rank_t rank_increase;
	
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
		rank_increase = dag->min_hoprankinc;		
		
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
	
	//printf("\t[RPL_OF_ELT] - comparing parent01 ID = %d with parent02 ID = %d\n", p1->addr, p2->addr);
	
	double p1_elt, p2_elt, p1_bottlen_elt, p2_bottlen_elt, min_p1, min_p2;
	p1_elt 			= calculate_my_elt(c, p1);
	p1_bottlen_elt	= calculate_bottlen_elt(c, p1);
	//if(c->node == 2)printf("call from rpl_of_elt_best_parent\n");
	p2_elt 			= calculate_my_elt(c, p2);
	//if(c->node == 2)printf("call from rpl_of_elt_best_parent\n");
	p2_bottlen_elt	= calculate_bottlen_elt(c, p2);
	
	//if(c->node == 2) printf("\t[RPL_OF_ELT] p%d, p%d \n", p1->addr, p2->addr);
	//if(c->node == 2) printf("\t[RPL_OF_ELT] p%d = %f, bottlen = %f, p%d = %f, bottlen = %f\n", p1->addr, p1_elt, p1_bottlen_elt, p2->addr, p2_elt, p2_bottlen_elt);
	
	if(p1_elt < p1_bottlen_elt)
		min_p1 = p1_elt;
	else
		min_p1 = p1_bottlen_elt;
		
		
	if(p2_elt < p2_bottlen_elt)
		min_p2 = p2_elt;
	else
		min_p2 = p2_bottlen_elt;
	
	
	
	//printf("[RPL_PAR_BEST] - exit\n");
	/* Maintain stability of the preferred parent in case of similar ranks. */
	if(min_p1 == min_p2) {
		if(p1 == dag->preferred_parent) {
			return p1;
		} 
		else if(p2 == dag->preferred_parent) {
			return p2;
		}
	}
	
	if(min_p1 > min_p2 + PARENT_SWITCH_ELT_THRESHOLD) {
		return p1;
	}
	
	return p2;	
}
