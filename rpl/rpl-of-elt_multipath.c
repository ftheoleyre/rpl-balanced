/**
 * \addtogroup uip6
 * @{
 */
/*
 * Copyright (c) 2010, Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 * $Id: rpl-of0.c,v 1.5 2010/06/14 12:44:37 nvt-se Exp $
 */
/**
 * \file
 *         An implementation of RPL's objective function 0.
 *
 * \author Joakim Eriksson <joakime@sics.se>, Nicolas Tsiftes <nvt@sics.se>
 */

#include "rpl.h"

static void reset(void *);
static rpl_parent_t *best_parent(call_t *, rpl_parent_t *, rpl_parent_t *);
static rpl_rank_t calculate_rank(call_t *, rpl_parent_t *, rpl_rank_t);

rpl_of_t rpl_of_elt_multipath = {
  reset,
  best_parent,
  calculate_rank,
  7
};


static void reset(void *dag){
  printf ("[rpl_of_elt_multipath]: Resetting OF\n");
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
		//rank_increase = DEFAULT_STEP_OF_RANK * DEFAULT_MIN_HOPRANKINC;
		rank_increase = LINK_ETX_GUESS * DEFAULT_MIN_HOPRANKINC;
	} 
	
	else {
		dag = (rpl_dag_t *)p->dag;
		//rank_increase = DEFAULT_STEP_OF_RANK * dag->min_hoprankinc;		
		rank_increase = p->etx_4_elt * dag->min_hoprankinc;		

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
		printf("\t[RPL_OF0_CALCULATE_RANK] %d reached the maximum rank, base_rank=%d.\n", c->node, base_rank);
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
	double min_elt_1, min_elt_2, elt_b;
	bottleneck_t *b;
		
	printf("\t[RPL_OF_ELT] - Node %d comparing parent01 ID = %d with parent02 ID = %d\n", c->node, p1->id, p2->id);

	p1->parent_load = 1;
	min_elt_1 = calculate_my_elt_wrt_parent(c, p1);
	
	printf("With parent %d node %d has ELT %f \n", p1->id, c->node, min_elt_1);
	printf("And has bottlenecks: \n");
	das_init_traverse(p1->bottlenecks);
	while((b = (bottleneck_t*) das_traverse(p1->bottlenecks)) != NULL){
			elt_b = b->bottlen_const / (b->ratio_of_traffic * b->bottlen_traffic);
			printf("%d, ELT: %f, const %f, ratio %f, traffic %f\n", b->bottlen_id, elt_b, b->bottlen_const, b->ratio_of_traffic, b->bottlen_traffic );
			if(min_elt_1 > elt_b)
				min_elt_1 = elt_b;
	}//end while
	
	p1->parent_load = 0;
	p2->parent_load = 1;
	min_elt_2 = calculate_my_elt_wrt_parent(c, p2);
		
	printf("With parent %d node %d has ELT %f \n", p2->id, c->node, min_elt_2);
	printf("And has bottlenecks: \n");
	das_init_traverse(p2->bottlenecks);
	while((b = (bottleneck_t*) das_traverse(p2->bottlenecks)) != NULL){
			elt_b = b->bottlen_const / (b->ratio_of_traffic * b->bottlen_traffic);
			printf("%d, ELT: %f, const %f, ratio %f, traffic %f\n", b->bottlen_id, elt_b, b->bottlen_const, b->ratio_of_traffic, b->bottlen_traffic );
			if(min_elt_2 > elt_b)
				min_elt_2 = elt_b;
	}//end while
	p2->parent_load = 0;

	//if(c->node == 2) printf("\t[RPL_OF_ELT] p%d, p%d \n", p1->addr, p2->addr);
	//if(c->node == 2) printf("\t[RPL_OF_ELT] p%d = %f, bottlen = %f, p%d = %f, bottlen = %f\n", p1->addr, p1_elt, p1_bottlen_elt, p2->addr, p2_elt, p2_bottlen_elt);

	if(min_elt_1 < min_elt_2)
		return p2;
	else if (min_elt_1 > min_elt_2)
			return p1;
		else if(p1->etx_4_elt < p2->etx_4_elt)
				return p1;
			else 
				return p2;
	//if(min_p1 > min_p2 + PARENT_SWITCH_ELT_THRESHOLD) 
	//	return p1;
}
