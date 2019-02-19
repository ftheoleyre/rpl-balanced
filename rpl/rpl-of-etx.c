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
 * $Id: rpl-of-etx.c,v 1.8 2010/11/03 15:41:23 adamdunkels Exp $
 */
/**
 * \file
 *         An implementation of RPL's objective function 1 (ETX).
 *
 * \author Joakim Eriksson <joakime@sics.se>, Nicolas Tsiftes <nvt@sics.se>
 */

#include "rpl.h"

static void reset(void *);
static rpl_parent_t *best_parent(call_t *, rpl_parent_t *, rpl_parent_t *);
static rpl_rank_t calculate_rank(call_t *, rpl_parent_t *, rpl_rank_t);


rpl_of_t rpl_of_etx = {
  reset,
  best_parent,
  calculate_rank,
  1
};


static void reset(void *dag){
  printf ("[RPL_OF_ETX_RESET]: Resetting OF_ETX.\n");
}


static rpl_rank_t calculate_rank(call_t *c, rpl_parent_t *p, rpl_rank_t base_rank){
	rpl_dag_t *dag;
	rpl_rank_t new_rank;
	rpl_rank_t rank_increase;
	
	int rank_factor = 1;
	int stretch = 0;
	
	//Parent was not passed as parameter i.e. first DIO reception
	if(p == NULL) {
		//Error prevention - if function was called without parameters
		if(base_rank == 0) {
			return INFINITE_RANK;
		}
		rank_increase = (rank_factor * LINK_ETX_GUESS  + stretch) * DEFAULT_MIN_HOPRANKINC;
	} 
	
	else {
		dag = (rpl_dag_t *)p->dag;
		rank_increase = (rank_factor * p->etx_4_elt + stretch) * dag->min_hoprankinc;		
		
		
		//We do this when we have new prefered parent and we want to calc our rank according to his
		if(base_rank == 0) {
			base_rank = p->rank;
			//printf("base rank == 0, p= %d rank_p = %d\n", p->id, p->rank);
		}
	}
	
	if(INFINITE_RANK - base_rank < rank_increase) {
		/* Reached the maximum rank. */
		new_rank = INFINITE_RANK;
		dag = (rpl_dag_t *)p->dag;
		printf("\t[RPL_OF_ETX_CALCULATE_RANK] %d reached the maximum rank, base_rank=%d. from %d, pref parent %d \n", c->node, base_rank, p->id, (dag->preferred_parent)->id);
	} 
	else {
		/* Calculate the rank based on the new rank information from DIO or
		 stored otherwise. */
		new_rank = base_rank + rank_increase;
	}

	//printf("\t[RPL_OF_ETX_CALCULATE_RANK] %d parent %d, base_rank %d, rank_increase %d \n", c->node, p->id, base_rank, rank_increase);

	return new_rank;
}

//choose the preferred parent
static rpl_parent_t *best_parent(call_t *c, rpl_parent_t *p1, rpl_parent_t *p2){
	rpl_dag_t *dag;
	rpl_rank_t p1_rank;
	rpl_rank_t p2_rank;
	
	dag = (rpl_dag_t *)p1->dag; //Both parents must be in the same DAG.
	
	//printf("[OF_BEST_PAR] - parent01 ID = %d parent02 ID = %d\n", p1->addr, p2->addr);
	
	p1_rank = DAG_RANK(calculate_rank(c, p1, 0), dag);
	p2_rank = DAG_RANK(calculate_rank(c, p2, 0), dag);
	
	//if(c->node == 2)
	//printf("[OF_BEST_PAR] Node %d - parent1 %d rank1=%u etx1=%f parent2 %d rank2=%u etx2=%f\n", c->node, p1->id, p1_rank, p1->etx_4_elt, p2->id, p2_rank, p2->etx_4_elt);
	
	//Maintain stability of the preferred parent in case of similar ranks.
	if(p1_rank == p2_rank){
		if(p1 == dag->preferred_parent){
			return p1;
		}
		else if(p2 == dag->preferred_parent){
			return p2;
		}
	}	
	//hysteresis 
	if(p1 == dag->preferred_parent || p2 == dag->preferred_parent)
		if(p1_rank < p2_rank + PARENT_SWITCH_ETX_THRESHOLD && p1_rank > p2_rank - PARENT_SWITCH_ETX_THRESHOLD)
			return dag->preferred_parent;
	
	//return p1_rank < p2_rank ? p1 : p2;

	if(p1_rank < p2_rank) {
		return p1;
	}
	
	return p2;
}
