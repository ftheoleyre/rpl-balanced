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

rpl_of_t rpl_of0 = {
  reset,
  best_parent,
  calculate_rank,
  0
};


static void reset(void *dag){
  printf ("[RPL_OF0_RESET]: Resetting OF0\n");
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
		rank_increase = DEFAULT_STEP_OF_RANK * DEFAULT_MIN_HOPRANKINC;
	} 
	
	else {
		dag = (rpl_dag_t *)p->dag;
		rank_increase = DEFAULT_STEP_OF_RANK * dag->min_hoprankinc;		
		
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
	rpl_dag_t *dag;
	rpl_rank_t p1_rank;
	rpl_rank_t p2_rank;
	
	dag = (rpl_dag_t *)p1->dag; /* Both parents must be in the same DAG. */
	
	//printf("[RPL_BEST_PAR] - comparing parent01 ID = %d parent02 ID = %d\n", p1->addr, p2->addr);
	
	p1_rank = DAG_RANK(calculate_rank(c, p1, 0), dag);
	p2_rank = DAG_RANK(calculate_rank(c, p2, 0), dag);
	
	//printf("[RPL_PAR_BEST] - exit\n");
	/* Maintain stability of the preferred parent in case of similar ranks. */
	if(p1_rank == p2_rank) {
		if(p1 == dag->preferred_parent) {
			return p1;
		} 
		else if(p2 == dag->preferred_parent) {
			return p2;
		}
	}
	
	if(p1_rank < p2_rank) {
		return p1;
	}
	
	return p2;	
}
