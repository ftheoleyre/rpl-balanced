#include "stats.h"


//function used for route calculation
int same_route(void *prev_route, void *current_route){
	if(das_getsize(prev_route) != das_getsize(current_route))
		return 0;
	else{
		nodeid_t* node, *id_ptr;
		int found;
        das_init_traverse(prev_route);
        while ((node = (nodeid_t*) das_traverse(prev_route)) != NULL){
        	found = 0;
        	das_init_traverse(current_route);
  			while((id_ptr = (nodeid_t*) das_traverse(current_route)) != NULL){
            	if(*node == *id_ptr){
					found = 1;
            	}
            }
            if(!found)
            	return 0;
		}
	}	
	return 1;
}


//initialization
void stats_init(){
	
}

//writes the link (node, parent) as edge of a graph in a file
void write_graph(call_t *c){
  	FILE *output, *outall;
  
  	//write dag
  	output=fopen("dag.txt", "a");
  	if (!output) {
  		printf("Error opening file.\n");
    	return ;
  	}
  	  	
  	struct nodedata *nodedata = get_node_private_data(c);
  	//get preferred parent
	rpl_dag_t *dag;
	rpl_parent_t *prefer_parent = NULL;
	dag = rpl_get_dag(nodedata, RPL_ANY_INSTANCE);
	if(dag != NULL)
		prefer_parent = dag->preferred_parent;
	
	if(prefer_parent != NULL){
		fprintf(output, "%d %d \n", c->node, prefer_parent->id);
		//printf("dag %d %d \n", c->node, prefer_parent->id);
  	}
  	
  	//write graph
 	outall=fopen("graph.txt", "a");
  	if (!outall) {
  		printf("Error opening file.\n");
    	return ;
  	}
  	
  	//get neighbours
  	rpl_parent_t *p;
  	das_init_traverse(nodedata->parents);
	while((p = (rpl_parent_t*) das_traverse(nodedata->parents)) != NULL){
		fprintf(outall, "%d %d \n", c->node, p->id);

	}
	
 	fclose(outall);
 	fclose(output);
}

/******** General stats *********/
//calculates the average delay for the network OK
double calculate_delay_all(){
	uint64_t delay = 0;
	int nr_packets = 0;
	stat_cbr_info	*stat_elem;
	
	das_init_traverse(global_cbr_stats);
	while ((stat_elem = (stat_cbr_info*) das_traverse(global_cbr_stats)) != NULL) {
		if (stat_elem->time_received != 0){ // && stat_elem->time_generated > START_TIME){
			delay += (stat_elem->time_received - stat_elem->time_generated);	
			nr_packets++;
		}
	}
	
	if(nr_packets)
		return (double) delay / (1e9 * nr_packets);
	else 
		return 0;
}

//calculates jain index for the network OK
double calculate_jain_all(){	
	int	nr_nodes = get_node_count();

	int	*flow_nr_pk_sent = (int*)calloc(nr_nodes, sizeof(int));
	int	*flow_nr_pk_rcvd = (int*)calloc(nr_nodes, sizeof(int));

	stat_cbr_info *stat_elem;
	
	das_init_traverse(global_cbr_stats);
	while ((stat_elem = (stat_cbr_info*) das_traverse(global_cbr_stats)) != NULL) {
			flow_nr_pk_sent[stat_elem->src] ++;
			//the packet was received
			if (stat_elem->time_received != 0){		
				flow_nr_pk_rcvd[stat_elem->src] ++;
			}
	}
	
	//Jain index
	double jain_index;
	double cumul_numerator = 0;
	double cumul_denominator = 0;
	int	count =0;
	int i;
	for(i=0; i<nr_nodes; i++)
		if (flow_nr_pk_sent[i] > 0){
			count++;
			//printf("i: %d pdr: %f \n", i, (double) flow_nr_pk_rcvd[i] / flow_nr_pk_sent[i]);
			cumul_denominator	+= pow((double) flow_nr_pk_rcvd[i] / flow_nr_pk_sent[i] , 2); 
			cumul_numerator		+= (double) flow_nr_pk_rcvd[i] / flow_nr_pk_sent[i];
		}
	jain_index = pow(cumul_numerator, 2) / (count * cumul_denominator); 
	
	return jain_index;
}

//prints the PDR for the network OK
double calculate_pdr_all(call_t* c){
	stat_cbr_info *stat_elem;
	int packs_sent = 0;
	int packs_rcv = 0;
	
	das_init_traverse(global_cbr_stats);
	while ((stat_elem = (stat_cbr_info*) das_traverse(global_cbr_stats)) != NULL) {
			if(stat_elem->time_generated){
				packs_sent++;
				//the packet was received
				if (stat_elem->time_received != 0){		
					packs_rcv++;
				}
			}
	}
	
	return (double) packs_rcv/packs_sent;
}


/******** stats for the nodes *********/

//calculates PDR for all the nodes (from source to destination, not the next hop) OK
void print_pdr_all(call_t* c){
	struct nodedata *nodedata = get_node_private_data(c);
	stat_cbr_info *stat_elem;
	int packs_sent = 0;
	int packs_rcv = 0;
	
	int all_packs_sent = 0;
	int all_packs_rcv = 0;
	
	FILE *output;
  
  	//open file
  	output=fopen("stats/pdr_flow_network_level.txt", "a");
  	if (!output) {
  		printf("Error opening file.\n");
    	return ;
  	}
	
	das_init_traverse(global_cbr_stats);
	while ((stat_elem = (stat_cbr_info*) das_traverse(global_cbr_stats)) != NULL) {
			if(stat_elem->src == c->node && stat_elem->time_generated > START_TIME){
				packs_sent++;
				//the packet was received
				if (stat_elem->time_received != 0){		
					packs_rcv++;
				}
			}
			
			if(stat_elem->src == c->node){
				all_packs_sent++;
				if (stat_elem->time_received != 0){		
					all_packs_rcv++;
				}
			}
	}
	
	etx_elt_t *etx_elt;	
  	int total_tx = 0;
	das_init_traverse(nodedata->mac_neigh_retransmit);
	while((etx_elt = (etx_elt_t*) das_traverse(nodedata->mac_neigh_retransmit)) != NULL){
			total_tx += etx_elt->packs_tx;
	} 
	
	fprintf(output, "%d %f %f %lf %d %d\n", c->node, (double) packs_rcv/packs_sent, (double) all_packs_rcv/all_packs_sent, nodedata->distance, total_tx, nodedata->data_tx);

	fclose(output);
}


//calculates average end-to-end delay for all the nodes (from source to destination, not the next hop) OK
void print_delay_all(call_t* c){
	struct nodedata *nodedata = get_node_private_data(c);
	stat_cbr_info *stat_elem;
	uint64_t delay = 0;
	int pack_sent = 0;
	int all_pack_sent = 0;
	uint64_t all_delay = 0;

	
	FILE *output;
  
  	//open file
  	output=fopen("stats/delay.txt", "a");
  	if (!output) {
  		printf("Error opening file.\n");
    	return ;
  	}
	
	das_init_traverse(global_cbr_stats);
	while ((stat_elem = (stat_cbr_info*) das_traverse(global_cbr_stats)) != NULL) {
			if(stat_elem->src == c->node && stat_elem->time_generated > START_TIME){
				//the packet was received
				if (stat_elem->time_received != 0){		
					delay += stat_elem->time_received - stat_elem->time_generated;
					pack_sent++;
				}
			}
			
			if(stat_elem->src == c->node){
				//the packet was received
				if (stat_elem->time_received != 0){		
					all_delay += stat_elem->time_received - stat_elem->time_generated;
					all_pack_sent++;
				}
			}
	}

	if(pack_sent)
		fprintf(output, "%d %lf %lf %lf\n", c->node, (double) delay/(1e9 *pack_sent), (double) all_delay/(1e9 *all_pack_sent), nodedata->distance);
	//else
	//	fprintf(output, "%d 0 %lf\n", c->node, nodedata->distance);


	fclose(output);
}



//calculate & print data_tx OK
void print_data_tx(call_t *c){
	FILE *output;
  
  	//open file
  	output=fopen("stats/data_tx.txt", "a");
  	if (!output) {
  		printf("Error opening file.\n");
    	return ;
  	}
  	  	
  	struct nodedata *nodedata = get_node_private_data(c);
  	fprintf(output, "%d %lf %d %d\n", c->node, nodedata->distance, nodedata->stats_data_tx, nodedata->data_tx);
	  	
 	fclose(output);
}



//calculate & print dio_tx OK
void print_dio_tx(call_t *c){
	FILE *output;
  
  	//open file
  	output=fopen("stats/dio_tx.txt", "a");
  	if (!output) {
  		printf("Error opening file.\n");
    	return ;
  	}
  	  	
  	struct nodedata *nodedata = get_node_private_data(c);
  	fprintf(output, "%d %lf %d\n", c->node, nodedata->distance, nodedata->dio_tx);
	  	
 	fclose(output);
}

//calculate & print parent_changed OK
void print_parent_changed2(call_t *c){
	FILE *output;
  
  	//open file
  	output=fopen("stats/parent_changed.txt", "a");
  	if (!output) {
  		printf("Error opening file.\n");
    	return ;
  	}
  	  	
  	struct nodedata *nodedata = get_node_private_data(c);
  	fprintf(output, "%d %lf %d %d\n", c->node, nodedata->distance, nodedata->stats_change_parent, nodedata->change_parent);
	  	
 	fclose(output);
}

//print energy consumed by a node OK
void print_energy(call_t *c){
	FILE *output;
	
	//open file
  	output=fopen("stats/energy_consumed.txt", "a");
  	if (!output) {
  		printf("Error opening file stats/energy_consumed.txt.\n");
    	return ;
  	}
  	  	
  	struct nodedata *nodedata = get_node_private_data(c);
    fprintf(output, "%d, %lf, %lf, %lf, %lf %f\n", c->node, get_node_position(c->node)->x, get_node_position(c->node)->y, get_node_position(c->node)->z, nodedata->distance, nodedata->energy_spent);
	 
	fclose(output);
}

void elt_status(call_t *c){
	FILE *output;
	double elt;
	
	
	output=fopen("stats/elt_status.txt", "a");
	if (!output) {
  	  	 printf("Error opening file stats/elt_status.txt.\n");
	  	 return ;
	}
	  	  	  		    	  	  	  	
	struct nodedata *nodedata = get_node_private_data(c);
	rpl_dag_t *dag;
	dag = rpl_get_dag(nodedata, RPL_ANY_INSTANCE);
	  	  	  		    	  	  	  	  	  	    		
	if(dag != NULL){
	  	   if(dag->preferred_parent == NULL){
 	  	  	if(rpl_select_parent(c, nodedata, dag) == NULL)
	  	  	       	printf("Node %d has no preferred parent\n", c->node);	  
	  	    }
	  	   	if(dag->preferred_parent != NULL){
 		    	  	elt = calculate_my_elt(c);						
 		    	  	fprintf(output, "%d %lf %lf %lf %lf %f %f %f\n", c->node, get_node_position(c->node)->x, get_node_position(c->node)->y, get_node_position(c->node)->z, nodedata->distance, elt, elt/3600, (elt/3600)/24);
	  	  	}
  	  	    else{
	  	  	  		fprintf(output, "%d %lf %lf %lf %lf %d %d %d\n", c->node, get_node_position(c->node)->x, get_node_position(c->node)->y, get_node_position(c->node)->z, nodedata->distance, 0, 0, 0);
			}
	}  		    	  	  	  	  	  	    														
	
	fclose(output);
}

//print number of parents OK
void number_of_parents(call_t *c){
	FILE *output;
	
	//open file
  	output=fopen("stats/nr_parents.txt", "a");
  	if (!output) {
  		printf("Error opening file stats/nr_parents.txt.\n");
    	return ;
  	}
  	  	
	nodedata_t *nodedata = get_node_private_data(c);
	rpl_parent_t *p;	
	das_init_traverse(nodedata->parents);
	int nr = 0;
	while((p = (rpl_parent_t*) das_traverse(nodedata->parents)) != NULL)
		nr++;
	fprintf(output, "%d, %lf, %lf, %lf, %lf %d\n", c->node, get_node_position(c->node)->x, get_node_position(c->node)->y, get_node_position(c->node)->z, nodedata->distance, nr);

	fclose(output);
}


//print ETX info
void etx_info(call_t *c){
	FILE *output;
	
	//open file
  	output=fopen("stats/etx_info.txt", "a");
  	if (!output) {
  		printf("Error opening file stats/etx_info.txt.\n");
    	return ;
  	}
  	  	
	nodedata_t *nodedata = get_node_private_data(c);
	
	int packs_net 		= 0; //nr of packets received from network layer (RPL)
	int packs_tx		= 0;
	int packs_tx_once	= 0; //nr packs transmitted on the radio having nr_retries = 0
	int acks			= 0;
	int drops			= 0;
		
	etx_elt_t *etx_elt;	
	das_init_traverse(nodedata->mac_neigh_retransmit);
	while((etx_elt = (etx_elt_t*) das_traverse(nodedata->mac_neigh_retransmit)) != NULL){
			packs_net 		+= etx_elt->packs_net;
			packs_tx 		+= etx_elt->packs_tx;
			packs_tx_once 	+= etx_elt->packs_tx_once;
			acks			+= etx_elt->acks;
			drops			+= etx_elt->drops;
	} 
	
	fprintf(output, "%d, %lf, %lf, %lf, %lf %d %d %d %d %d\n", c->node, get_node_position(c->node)->x, get_node_position(c->node)->y, get_node_position(c->node)->z, nodedata->distance, packs_net, packs_tx, packs_tx_once, acks, drops);

	fclose(output);

}


//time at which a node has reseted its dio timer
/*void print_reset_timer(call_t *c){
	FILE *output;
  
  	//open file
  	output=fopen("reset_timer.txt", "a");
  	if (!output) {
  		printf("Error opening file.\n");
    	return ;
  	}
  	  	
  	struct nodedata *nodedata = get_node_private_data(c);
  	rpl_time_parent_t *reset_time;
  	das_init_traverse(nodedata->time_reset_trickle);
	while((reset_time = (rpl_time_parent_t*) das_traverse(nodedata->time_reset_trickle)) != NULL)
  		fprintf(output, "%d %llu \n", reset_time->parent_id, reset_time->time);
	  	
 	fclose(output);
}*/

//when did my parent change 
void print_parent_changed(call_t *c){
	FILE *output;
  
  	//open file
  	output=fopen("parent_changed.txt", "a");
  	if (!output) {
  		printf("Error opening file.\n");
    	return ;
  	}

	struct nodedata *nodedata = get_node_private_data(c);
  	parent_stats *p_stats;
  	das_init_traverse(nodedata->parent_changing);
	while((p_stats = (parent_stats*) das_traverse(nodedata->parent_changing)) != NULL)
  		fprintf(output, "%d %d %f %llu %d\n", c->node, p_stats->parent, p_stats->etx, p_stats->time, p_stats->my_rank / 256);
	  	
 	fclose(output);
}


/******** Route stats *********/

//for route persistence OK
void print_routes_time(call_t *c){
	stat_cbr_info *stat_elem;
	stat_elem = NULL;
	int total_time = 0;
	void *first_route = das_create();
	uint64_t last_time = get_time();
	nodeid_t *id_ptr;
	int	first = 1;

	FILE *output;
  
  	char file_name[100];
  	sprintf(file_name, "stats/all_routes_time/routes_%d.txt", c->node);
 	 
  	//open file
  	output=fopen(file_name, "w");
  	if (!output) {
  		printf("Error opening file stats/all_routes_time/routes_%d.txt.\n", c->node);
    	return ;
  	} 	
		
	//pick the stats for current node	
	das_init_traverse(global_cbr_stats);
	while ((stat_elem = (stat_cbr_info*) das_traverse(global_cbr_stats)) != NULL){
		if(stat_elem->time_received != 0 && stat_elem->src == c->node && stat_elem->time_generated > START_TIME){
			if(first == 1){ //buffer first route
				first_route = stat_elem->route;
				total_time += (last_time - stat_elem->time_received) / 1000000000;
				first++;
			}
			else{
				if(same_route(first_route, stat_elem->route)){
					fprintf(output, "0 ");
					das_init_traverse(stat_elem->route);
            		while ((id_ptr = (nodeid_t*) das_traverse(stat_elem->route)) != NULL){
  						fprintf(output, "%d ", *id_ptr);
            		}
            		fprintf(output, "\n");
            		total_time += (last_time - stat_elem->time_received) / 1000000000;
				}
				else{
					fprintf(output, "%d ", total_time);
					das_init_traverse(first_route);
					while ((id_ptr = (nodeid_t*) das_traverse(first_route)) != NULL){
  						fprintf(output, "%d ", *id_ptr);
            		}
            		fprintf(output, "\n");
            		first_route = stat_elem->route;
            		total_time = (last_time - stat_elem->time_received) / 1000000000;
				}
			}
			last_time = stat_elem->time_received;
		}
	}
	fprintf(output, "%d ", total_time);
	das_init_traverse(first_route);
	while ((id_ptr = (nodeid_t*) das_traverse(first_route)) != NULL){
  		fprintf(output, "%d ", *id_ptr);
    }
	fprintf(output, "\n");

	fclose(output);
}

//for prevalence OK
void print_routes(call_t *c){
	stat_cbr_info *stat_elem;
	stat_elem = NULL;
	int total_time = 0;
	void *first_route = das_create();
	uint64_t last_time = get_time();
	nodeid_t *id_ptr;
	int	first = 1;

	FILE *output;
  
  	char file_name[100];
  	sprintf(file_name, "stats/all_routes/routes_%d.txt", c->node);
 	 
  	//open file
  	output=fopen(file_name, "w");
  	if (!output) {
  		printf("Error opening file stats/all_routes/routes_%d.txt.\n", c->node);
    	return ;
  	} 	
		
	//pick the stats for current node	
	das_init_traverse(global_cbr_stats);
	while ((stat_elem = (stat_cbr_info*) das_traverse(global_cbr_stats)) != NULL){
		if(stat_elem->time_received != 0 && stat_elem->src == c->node && stat_elem->time_generated > START_TIME){
			if(first == 1){ //buffer first route
				first_route = stat_elem->route;
				total_time += (last_time - stat_elem->time_received) / 1000000000;
				first++;
			}
			else{
				if(same_route(first_route, stat_elem->route)){
					das_init_traverse(stat_elem->route);
            		while ((id_ptr = (nodeid_t*) das_traverse(stat_elem->route)) != NULL){
  						fprintf(output, "%d ", *id_ptr);
            		}
            		fprintf(output, "\n");
            		total_time += (last_time - stat_elem->time_received) / 1000000000;
				}
				else{
					das_init_traverse(first_route);
					while ((id_ptr = (nodeid_t*) das_traverse(first_route)) != NULL){
  						fprintf(output, "%d ", *id_ptr);
            		}
            		fprintf(output, "\n");
            		first_route = stat_elem->route;
            		total_time = (last_time - stat_elem->time_received) / 1000000000;
				}
			}
			last_time = stat_elem->time_received;
		}
	}
	das_init_traverse(first_route);
	while ((id_ptr = (nodeid_t*) das_traverse(first_route)) != NULL){
  		fprintf(output, "%d ", *id_ptr);
    }
	fprintf(output, "\n");

	fclose(output);
}

//general node statistics
void node_stats(call_t *c){
	if(c->node != 0){
		//print_routes(c);          //for route prevalence  
		//print_routes_time(c);	  //for route persistence 
		print_pdr_all(c);		  //real PDR (successfully arrived at the destination not at the next hop) - at the network level (RPL)
		print_delay_all(c); 	  //average end-to-end delay for a node to the sink
		print_data_tx(c);		  //data_tx for each node
		print_dio_tx(c);		  //dio_tx for each node
		print_parent_changed2(c); //how many times the preferred parent has changed
		print_energy(c);
		number_of_parents(c);
		etx_info(c);
		elt_status(c);

	}

	if(c->node == 0){
		printf("Node %d:", c->node);
		printf("\t\tPDR = %f\n", calculate_pdr_all(c)); //calcuated for the flows
		printf("\t\tAverage end-to-end delay %fs.\n", calculate_delay_all());
		printf("\t\tJain index is %f.\n", calculate_jain_all()); //calculated with the flows for the sources
	}
	
	/*if(c->node != 0){
		print_pdr_all(c);
		print_routes(c);
	}
	
	if(c->node == 0){
		printf("\t\tPDR = %f\n", calculate_pdr_all(c)); //calcuated for the flows
		printf("\t\tAverage end-to-end delay %fs.\n", calculate_delay_all());
		printf("\t\tJain index is %f.\n", calculate_jain_all()); //calculated with the flows for the sources
	}
	
	print_parent_changed(c);
	
	//printf(" number of trickle timer resets %llu.\n", nodedata->reset_trickle);
	
	print_reset_timer(c);	
	*/
}




void print_stats(call_t *c){
	mkdir("stats", 0777);
	mkdir("stats/all_routes", 0777);
	mkdir("stats/all_routes_time", 0777);

	node_stats(c);
	write_graph(c);
}
