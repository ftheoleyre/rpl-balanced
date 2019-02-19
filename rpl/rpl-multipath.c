#include <limits.h>
#include <string.h>
#include <stdint.h>

#include "rpl.h"

//ELT: ETX and traffic computation

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
		printf("\t\t -------------------1 node %d nodata_tx is %d new_traffic is %f my traffic is %f  at time %llu \n", c->node, nodedata->data_tx, new_traffic, nodedata->my_traffic, get_time());
		nodedata->old_tx = nodedata->data_tx;
	}
	
	
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
	
	
	scheduler_add_callback(get_time() + RECOMPUTE_ETX, c, compute_etx, NULL);
	return 0;
}




/************************************************************************/




//MULTIPATH
//calculate my ELT OK january 2014
double calculate_my_elt(call_t *c){
	struct nodedata *nodedata = get_node_private_data(c);
	//printf("\t\t ELT node %d traffic %f parents %d\n", c->node, nodedata->my_traffic, das_getsize(nodedata->parents));
	
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
  	//printf("\t\t total energy consumed %f, remaining %f (Joule)\n", total_consumption, nodedata->energy_residual);
	

	//get my_ELT in nanoseconds OK
	double my_ELT = 0;
	float traffic_to_parents = 0;
	rpl_parent_t *p;
	
	das_init_traverse(nodedata->parents);
	while((p = (rpl_parent_t*) das_traverse(nodedata->parents)) != NULL){
		//printf("[compute_my_elt] node %d to parent %d traffic_load %f, traffic total %f, etx %f \n", c->node, p->id, p->parent_load, nodedata->my_traffic, p->etx_4_elt);
		traffic_to_parents += p->parent_load * p->etx_4_elt;
	}
		 
	my_ELT = (nodedata->energy_residual * DATA_RATE) / (traffic_to_parents * nodedata->my_traffic * MY_RADIO_TX);
	
	//printf("[compute_my_elt]Node %d ELT is %f\n", c->node, my_ELT);
	return my_ELT;
}



//MULTIPATH
//calculate my ELT when choosing the preferred parent: all of the traffic goes to parent p  OK january 2014 NOT returns INF???
//used when choosing the preferred parent 
double calculate_my_elt_wrt_parent(call_t *c, rpl_parent_t *p){
	struct nodedata *nodedata = get_node_private_data(c);
	//printf("\t\t ELT node %d traffic %f parents %d\n", c->node, nodedata->my_traffic, das_getsize(nodedata->parents));
	
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
  	//printf("\t\t total energy consumed %f, remaining %f (Joule)\n", total_consumption, nodedata->energy_residual);
			
	//get my_ELT in nanoseconds OK
	double my_ELT = 0;	
	my_ELT = (nodedata->energy_residual * DATA_RATE) / (p->etx_4_elt * nodedata->my_traffic * MY_RADIO_TX);
	
	//printf("[compute_my_elt_wrt_parent]Node %d ELT is %f p->etx %f my_traffic %f residual_energy %f\n", c->node, my_ELT, p->etx_4_elt, nodedata->my_traffic, nodedata->energy_residual);
	return my_ELT;
}



//MULTIPATH
//calculate ELT of a bottleneck
double calculate_bottlen_elt(call_t *c, bottlen_parent_t *bparent){
	struct nodedata *nodedata = get_node_private_data(c);
	
	double bottlen_ELT = 0;
	float traffic_to_bottleneck = 0;
	rpl_parent_t *p;
	bottleneck_t *bp;
	
	//get info about the old quantity of traffic that node c was sending to this bottleneck
	float b_old_traffic = 0;
	bottlen_old_t *old;
	das_init_traverse(nodedata->b_old_traffic);
	while((old = (bottlen_old_t*) das_traverse(nodedata->b_old_traffic)) != NULL){
		if(old->bottlen_id == bparent->bottlen_id)
			b_old_traffic = old->old_traffic;
	}
	
	
	//find out the ratio of traffic that node c sends to the bottleneck
	das_init_traverse(nodedata->parents);
	while((p = (rpl_parent_t*) das_traverse(nodedata->parents)) != NULL){
		if(p->parent_load != 0){
			das_init_traverse(p->bottlenecks);
			while((bp = (bottleneck_t*) das_traverse(p->bottlenecks)) != NULL){
				if(bp->bottlen_id == bparent->bottlen_id){
					traffic_to_bottleneck += p->parent_load * bp->ratio_of_traffic;
				}//end if
			}//end while
		}//end if
		//printf("\t[calculate_bottlen_elt]Node %d has parent %d parent load %f\n", c->node, p->id, p->parent_load);
	}//end while
	
	float new_traffic;
	new_traffic = (traffic_to_bottleneck * nodedata->my_traffic) + bparent->bottlen_traffic - b_old_traffic;
	if(new_traffic > 0)
		bottlen_ELT = bparent->bottlen_const / ((traffic_to_bottleneck * nodedata->my_traffic) + bparent->bottlen_traffic - b_old_traffic);
	else
		bottlen_ELT = bparent->bottlen_const / ((traffic_to_bottleneck * nodedata->my_traffic) + bparent->bottlen_traffic);
	//printf("\t[calculate_bottlen_elt]node %d: bottleneck %d through parent %d, ELT of bottleneck is: %f, const = %f, traffic=%f, traffic_to_bottlenck=%f, time %llu\n",c->node, bparent->bottlen_id, bparent->parent_id , bottlen_ELT, bparent->bottlen_const, bparent->bottlen_traffic, traffic_to_bottleneck, get_time());
	
	return bottlen_ELT;
}



//MULTIPATH
//from all parents, create the list of my_bottlenecks to advertise in the DIOs OK january 2014 ; did not check if they have a common bottleneck what happens 
void update_my_bottlenecks(call_t *c, rpl_dag_t *dag){
	nodedata_t *nodedata = get_node_private_data(c);
	
	bottleneck_t *b, *my_b, *new_bottlenck;
	rpl_parent_t *p;
	bottlen_parent_t *bp;
	int found;
	double my_elt, b_elt;
	my_elt = 0;
	b_elt = 0;


	if(dag == NULL)
		return ;
		
	void *parents_copy;
	parents_copy = das_create();
	das_init_traverse(nodedata->parents);
	while((p = (rpl_parent_t*) das_traverse(nodedata->parents)) != NULL){	
		//printf("[update_bottlenecks] Node %d has parent %d \n", c->node, p->id);
		das_insert(parents_copy, p);
	}
	
	
	void *bparents;
	bparents = das_create();
	das_init_traverse(nodedata->parents);
	while((p = (rpl_parent_t*) das_traverse(nodedata->parents)) != NULL){	
			das_init_traverse(p->bottlenecks);
			//printf("[update_bottlenecks] Node %d has parent %d \n", c->node, p->id);

			while((b = (bottleneck_t*) das_traverse(p->bottlenecks)) != NULL){  //get the minimum ELT of all the bottlenecks
				bp = (bottlen_parent_t *) malloc(sizeof(bottlen_parent_t));
				bp->parent_id  		 = p->id;
				bp->bottlen_id 		 = b->bottlen_id;
				bp->bottlen_const	 = b->bottlen_const; 
  				bp->bottlen_traffic  = b->bottlen_traffic;
  				bp->ratio_of_traffic = b->ratio_of_traffic;  
				das_insert(bparents, bp);
				//printf("[update_bottlenecks] Node %d has parent %d who has bottleneck %d\n", c->node, p->id, b->bottlen_id);
			}
	}
	
	my_elt = calculate_my_elt(c);
	//printf("[update_bottlenecks]calculate_my_elt(%d) %f\n", c->node, my_elt);

	
	das_init_traverse(parents_copy);
	while((p = (rpl_parent_t*) das_traverse(parents_copy)) != NULL){
		if(p->parent_load != 0){
			das_init_traverse(bparents);
			while((bp = (bottlen_parent_t*) das_traverse(bparents)) != NULL){
				if(bp->parent_id == p->id){
					b_elt = calculate_bottlen_elt(c, bp);
					//printf("[update_bottlenecks] elt of bottleneck = %d, is %f\n", bp->bottlen_id, b_elt);

					if(my_elt < b_elt){
						//i become the new bottleneck
						//printf("[update_bottlenecks] i become the new bottleneck %d\n", c->node);
						found = 0;
						das_init_traverse(nodedata->my_bottlenecks);
						while((my_b = (bottleneck_t*) das_traverse(nodedata->my_bottlenecks)) != NULL){
							if(c->node == my_b->bottlen_id){
								found = 1;			
							}
						}
						
						if(!found){
							new_bottlenck = (bottleneck_t *) malloc(sizeof(bottleneck_t));
							new_bottlenck->bottlen_const 	= (double)(nodedata->energy_residual * DATA_RATE) / (p->etx_4_elt * MY_RADIO_TX); //(START_ENERGY * DATA_RATE)	/ (ETX_4_ELT_GUESS * RADIO_TX)	//(E_res * DATA_RATE) / (ETX * 1b/s * Etx)
							new_bottlenck->bottlen_traffic  = nodedata->my_traffic;
							new_bottlenck->ratio_of_traffic = 1;
							new_bottlenck->bottlen_id 		= c->node;	
							das_insert(nodedata->my_bottlenecks, new_bottlenck);
							//printf("[update_my_bottlenecks]Node %d added himself as bottleneck with ID %d \n", c->node, new_bottlenck->bottlen_id);	
						}
					}
					else{ //if c->node does not become the new bottleneck
						//printf("[update_my_bottlenecks] i(%d) did not become the new bottleneck, the bottleneck is %d through parent %d\n", c->node, bp->bottlen_id, bp->parent_id);
						found = 0;
						das_init_traverse(nodedata->my_bottlenecks);
						while((my_b = (bottleneck_t*) das_traverse(nodedata->my_bottlenecks)) != NULL){
							//printf("in while\n");
							if(bp->bottlen_id == my_b->bottlen_id){
								found = 1;		
								//printf("[update_my_bottlenecks] Node %d before updating bottleneck %d with ratio of traffic %f %f \n\n\n", c->node, my_b->bottlen_id, my_b->ratio_of_traffic, bp->ratio_of_traffic);	
								if(p->id == bp->parent_id)
									my_b->ratio_of_traffic = bp->ratio_of_traffic * p->parent_load;
								else
									my_b->ratio_of_traffic += bp->ratio_of_traffic * p->parent_load; //update the traffic that will be sent to this bottleneck
								//printf("[update_my_bottlenecks] Node %d updated bottleneck %d with ratio of traffic %f\n\n\n", c->node, my_b->bottlen_id, my_b->ratio_of_traffic);	
							}
						}
						//printf("outside while\n");
						if(!found){
							//printf("not found\n");
							new_bottlenck = (bottleneck_t *) malloc(sizeof(bottleneck_t));
							new_bottlenck->bottlen_id 		 = bp->bottlen_id;
							new_bottlenck->bottlen_const	 = bp->bottlen_const; 
  							new_bottlenck->bottlen_traffic 	 = bp->bottlen_traffic;
  							new_bottlenck->ratio_of_traffic	 = bp->ratio_of_traffic * p->parent_load; 
  							das_insert(nodedata->my_bottlenecks, new_bottlenck);
  							//printf("[update_my_bottlenecks] Node %d added bottleneck node with ID %d and ratio of traffic is %f\n\n\n", c->node, new_bottlenck->bottlen_id, new_bottlenck->ratio_of_traffic);	
						}
					}//if - else(my_elt < b_elt)	
				}//if bp->p_id == p->id
			}//while bp
		}//if parent_load != 0
	}//while p
				
	
	/*das_init_traverse(nodedata->my_bottlenecks);
	while((my_b = (bottleneck_t*) das_traverse(nodedata->my_bottlenecks)) != NULL){	
		//das_delete(nodedata->my_bottlenecks, my_b);
		printf("[update_my_bottlencks] After while Node %d has bottlenecks %d\n", c->node, my_b->bottlen_id);
	}*/
	
	//free memory
	if(das_getsize(bparents))
		das_destroy(bparents);
		
	if(das_getsize(parents_copy))
		das_destroy(parents_copy);
}//void function



//MULTIPATH
//compute the traffic_loads to each of the parents OK january 2014 ; did not check if they have a common bottleneck what happens 
void compute_loads(call_t *c, rpl_dag_t *dag){
	struct nodedata *nodedata = get_node_private_data(c);
	rpl_parent_t *p;
	double max_elt;
	double min_elt;
	bottleneck_t *b;
	int parent_max;
	int i;
	double elt_b;
	bottlen_parent_t *bp;

	void *bparents;
	bparents = das_create();
	
	
	if(dag == NULL){
		rpl_local_repair(c, nodedata,dag);
		return ;
	}
	
	if(dag->preferred_parent == NULL){
		p = rpl_select_parent(c, nodedata, dag);
		
		if(p == NULL){
			rpl_local_repair(c, nodedata,dag);
			return;
		}
	}
		
	//UNCOMMENT THIS :-)
	if(das_getsize(nodedata->parents) == 1){
		(dag->preferred_parent)->parent_load = 1;
		(dag->preferred_parent)->parent_old_load = 1;
		printf("[compute_load]Node % d has 1 parent: %d \n", c->node, dag->preferred_parent->id);
		return ;
	}
	
	float old_loads = 0;
	void *parents_copy;
	parents_copy = das_create();
	das_init_traverse(nodedata->parents);
	while((p = (rpl_parent_t*) das_traverse(nodedata->parents)) != NULL){	
		old_loads += p->parent_load;	//compute the sum of all the loads to verify that none of the parents has been removed from the parent set
		
		p->parent_old_load = p->parent_load;
		p->parent_load = 0;
		das_insert(parents_copy, p);
	}

	
	das_init_traverse(nodedata->parents);
	while((p = (rpl_parent_t*) das_traverse(nodedata->parents)) != NULL){	
			das_init_traverse(p->bottlenecks);
			while((b = (bottleneck_t*) das_traverse(p->bottlenecks)) != NULL){  //get the minimum ELT of all the bottlenecks
				bp = (bottlen_parent_t *) malloc(sizeof(bottlen_parent_t));
				bp->parent_id  		 = p->id;
				bp->bottlen_id 		 = b->bottlen_id;
				bp->bottlen_const	 = b->bottlen_const; 
  				bp->bottlen_traffic  = b->bottlen_traffic;
  				bp->ratio_of_traffic = b->ratio_of_traffic;  
				das_insert(bparents, bp);
				//printf("[compute_load] Node %d has parent %d who has bottleneck %d\n", c->node, p->id, b->bottlen_id);
			}
	}


	//create info about old traffic sent to this bottleneck;
	//will be used in the computation of the ELT for a bottleneck
	bottlen_old_t *old;	
	bottleneck_t *my_b;
	
	if(das_getsize(nodedata->b_old_traffic))
		das_destroy(nodedata->b_old_traffic);	
	
	nodedata->b_old_traffic = das_create();
	das_init_traverse(nodedata->my_bottlenecks);
	while((my_b = (bottleneck_t*) das_traverse(nodedata->my_bottlenecks)) != NULL){
		old = (bottlen_old_t *) malloc(sizeof(bottlen_old_t));
		old->bottlen_id = my_b->bottlen_id;
		old->old_traffic = my_b->ratio_of_traffic;
		das_insert(nodedata->b_old_traffic, old);
	}
	
						
	//start algorithm of load balancing
	int for_n = 1/nodedata->load_step;
	for(i=0; i<for_n; i++){
		max_elt = 0;
		parent_max = (dag->preferred_parent)->id;
		das_init_traverse(parents_copy);
		while((p = (rpl_parent_t*) das_traverse(parents_copy)) != NULL){		
			min_elt = MAX_ELT;
			p->parent_load += nodedata->load_step; //"virtual" give 0.1 of the traffic to one parent and see how it influences all the ELTs	
			//printf("[compute_load]Parent %d added 0.1 to its traffic and has now %f \n", p->id, p->parent_load);
				
			das_init_traverse(bparents);
			while((bp = (bottlen_parent_t*) das_traverse(bparents)) != NULL){  //get the minimum ELT of all the bottlenecks
				elt_b = calculate_bottlen_elt(c, bp);
				//printf("[compute_load] Bottleneck %d has ELT %f\n", bp->bottlen_id, elt_b);
				if(elt_b <= min_elt){
					min_elt = elt_b;
					//printf("[compute_load]Minimum changed to Bottleneck %d has ELT %f\n", bp->bottlen_id, elt_b);
				}
			}//end while bp

			double my_elt = calculate_my_elt(c);	//get the minimum ELT between the node and the ELT of the bottlenecks 
			if(my_elt <= min_elt){
				min_elt = my_elt;
				//printf("[compute_load]Minimum changed to be the node %d has ELT %f\n", c->node, my_elt);
			}
			
			p->parent_load -= nodedata->load_step; //take back the "virtual" 0.1 of the traffic
			//printf("[compute_load]Parent %d subtracted 0.1 to its traffic and has now %f\n", p->id, p->parent_load);
					
			if(min_elt >= max_elt){		//get the maximum ELT between all bottlenecks and the respective parent 
				max_elt = min_elt;
				parent_max = p->id;
				//printf("[compute_load]Parent max is %d\n", parent_max);
			}	
		}//end while p
		
		//give 0.1 of traffic to the parent that maximizes the ELT; this ALSO modifies parents_copy !!!
		das_init_traverse(nodedata->parents);
		while((p = (rpl_parent_t*) das_traverse(nodedata->parents)) != NULL){	
			if(p->id == parent_max){
				p->parent_load += nodedata->load_step;
				//printf("[compute_load]Parent %d has load %f \n", p->id, p->parent_load);
			}
		}
		
		//now retry 0.1 of the traffic to all of the parents again
	}//end for i=0,10
	
	if(old_loads > 0.95){ //verify that none of the parents has been removed from the parent set
		//normalize in function of all the differences (so that the sum of all loads = 100%)
		float max_diff_loads = 0;
		float diff;
		das_init_traverse(nodedata->parents);   //first, get the max of all the diffs, so that we can normalize afterwards
		while((p = (rpl_parent_t*) das_traverse(nodedata->parents)) != NULL){	
			diff = fabsf(p->parent_load - p->parent_old_load);
			if(max_diff_loads < diff)
				max_diff_loads = diff;
		}
		
		//normalize so that no load increases/decreases more than 10%
		if(max_diff_loads != 0){
			das_init_traverse(nodedata->parents);
			while((p = (rpl_parent_t*) das_traverse(nodedata->parents)) != NULL){	
				diff = p->parent_load - p->parent_old_load;
				p->parent_load = p->parent_old_load + (diff * nodedata->max_increase) / max_diff_loads;
			}
		}
	}
	
	das_init_traverse(nodedata->parents);
	while((p = (rpl_parent_t*) das_traverse(nodedata->parents)) != NULL){	
		//printf("[compute_load] Node %d to parent %d new load %f\n", c->node, p->id, p->parent_load); 
		
		//delete parent if it's not useful anymore
		if(p->parent_load < 0.01){
			printf("[compute_load] Node %d deleting parent %d because load < 0.1\n", c->node, p->id);
			rpl_remove_parent(c, nodedata, dag, p);
		}
	}
	
	/* //old algorithm using moving window
	float new_load;
	das_init_traverse(nodedata->parents);
	while((p = (rpl_parent_t*) das_traverse(nodedata->parents)) != NULL){	
		new_load = LAMDA_LOAD*p->parent_old_load + (1-LAMDA_LOAD)*p->parent_load;
		p->parent_load = new_load;
		p->parent_old_load = p->parent_load;
		
		if(p->parent_load < 0.01){
			printf("[compute_load] Node %d deleting parent %d because load =0\n", c->node, p->id);
			rpl_remove_parent(c, nodedata, dag, p);
		}
	}*/
	
	//free memory
	if(das_getsize(bparents))
		das_destroy(bparents);
	if(das_getsize(parents_copy))
		das_destroy(parents_copy);
}



void update_bottlenck_list_parent(call_t *c, rpl_dag_t *dag, rpl_parent_t *p, rpl_dio_t *dio){
	//printf("[update_bottlenck_list_parent]Node %d updating bottlenecks list of parent %d\n", c->node, p->id);

	dio_bottleneck_t *b;
	bottleneck_t *new_bottlenck;	
	
	bottleneck_t *bb;
	das_init_traverse(p->bottlenecks);
	while((bb = (bottleneck_t*) das_traverse(p->bottlenecks)) != NULL){
		das_delete(p->bottlenecks, bb);
	}
		
	
	das_init_traverse(dio->bottlenecks);
	while((b = (dio_bottleneck_t*) das_traverse(dio->bottlenecks)) != NULL){
		//printf("[update_bottlenck_list_parent]Parent %d adding b_id %d, b_const %f, b_traffic %f, b_ratio %f\n", p->id, b->bottlen_id, b->bottlen_const, b->bottlen_traffic,b->ratio_of_traffic );
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
		//printf("[update_bottlenck_list_parent]Node %d through parent %d added a new bottleneck with ID %d \n", c->node, p->id, new_bottlenck->bottlen_id);	
	}//while
	
	
}



int get_next_hop(call_t *c, rpl_dag_t *dag){
	nodedata_t *nodedata = get_node_private_data(c);
	
	rpl_parent_t *p;
	int random_nr;
	float random_float;
	float old_load;


	if(dag == NULL)
		return -1;
		
	
	//get a random value between 0 and 10
	srand(get_time());
	random_nr = rand() % 10;
	random_float = (float)random_nr / 10;
	//printf("[get_next_hop]My(%d) random number is %f\n", c->node, random_float);
		
		
	old_load = 0;
	das_init_traverse(nodedata->parents);
	while((p = (rpl_parent_t*) das_traverse(nodedata->parents)) != NULL){	
		if(random_float < (p->parent_load + old_load)){
			//printf("[get_next_hop]My(%d) next hop is %d because its load is %f and old_load is %f\n\n\n", c->node, p->id, p->parent_load, old_load);
			return p->id;
		}
		else
			old_load = p->parent_load;
	}
	return -1;
}
