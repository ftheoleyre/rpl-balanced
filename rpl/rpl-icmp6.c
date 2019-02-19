#include "rpl.h"
#include "../wsnet_rpl/rplWSNet.h"

/* ************************************************** */
/*						Send DIO					  */ 
/* ************************************************** */

int send_dio(call_t *c, void *args){	
	nodedata_t *nodedata = get_node_private_data(c);
	rpl_dag_t *dag;
	dag = rpl_get_dag(nodedata, RPL_ANY_INSTANCE);
	destination_t destination;
	//printf("DIO Node %d has bottleneck id %d bottlen_const %f bottlen_traffic %d time %llu\n", c->node, dag->bottlen_id, dag->bottlen_const, dag->bottlen_traffic, get_time());

	//If c<k, send DIO
	if(dag->dio_counter < dag->dio_redundancy) {		
		//Broadcast a neighbour packet 
		//printf("[RPL_TX_DIO] Node ID = %d\n size %llu", c->node, );
		
		//verify if I did not become the new bottleneck
		verify_bottleneck(c);
		
		entityid_t *down = get_entity_links_down(c);
		call_t c0 = {down[0], c->node};
		
		packet_t *packet = packet_alloc(c, nodedata->overhead[0] + sizeof(rpl_packet_header_t) + sizeof(rpl_dio_t));
		rpl_packet_header_t *header = (rpl_packet_header_t *) (packet->data + nodedata->overhead[0]);	
		
		rpl_dio_t *dio 			= malloc(sizeof(rpl_dio_t)); 		
		//RPL parameters
		dio->dag_id				= dag->dag_id;
		dio->second_part_dag_id	= dag->second_part_dag_id;
		dio->instance_id		= dag->instance_id;
		dio->version			= dag->version;
		dio->rank				= dag->rank;
		dio->g_mop				= 0;	//not used
		dio->dtsn				= 0;
		dio->flags				= 0;
		dio->reserved			= 0;

		//rpl_dio_config_t *dodag_config  	  = malloc(sizeof(rpl_dio_config_t)); 		

		//add DIO configuration option
		dio->dodag_config.type 			  	  = 4;
		dio->dodag_config.opt_length 		  = 14;
		dio->dodag_config.flags			  	  = 0;	
		dio->dodag_config.dio_intdoubl 	  	  = dag->dio_intdoubl;
		dio->dodag_config.dio_intmin		  = dag->dio_intmin;
		dio->dodag_config.dio_redund		  = dag->dio_redundancy;
		dio->dodag_config.maxRankIncrease     = dag->max_rankinc;
		dio->dodag_config.minHopRankIncrease  = dag->min_hoprankinc;
		dio->dodag_config.ocp				  = dag->of->ocp;
		dio->dodag_config.reserved			  =	0;
		dio->dodag_config.lifetime    	  	  =	0;	//not used;
		dio->dodag_config.lifetime_unit	 	  =	0; 	//not used;
		
		//dio->dodag_config = dodag_config;
	
		//dio->nr_sequence 		= ++dag->dio_sequence;
		
		//info about the bottlenck
		dio->bottlen_const	 	= dag->bottlen_const;
		dio->bottlen_traffic	= dag->bottlen_traffic;
		dio->bottlen_id			= dag->bottlen_id;
		//printf("DIO Node %d has bottleneck id %d bottlen_const %f bottlen_traffic %f\n", c->node, dag->bottlen_id, dag->bottlen_const, dag->bottlen_traffic);

		
		//info for the residual energy metric -> min residual energy of the path
		dio->path_energy_residual = dag->path_energy_residual;
		
		//for the energy metric (etx+residual)
		if(dag->of->ocp == 8){
			//get E_res = residual energy
   	 		double total_consumption = 0;
    		int j;
    		for(j=0; j<RADIO_NB_MODES; j++){   
    			//printf("Mode %d consumption: %f\n", j, nodedata->radio_energy->consumption[j]);
      		  	total_consumption   += nodedata->radio_energy->consumption[j]; 
  			}
  			nodedata->energy_residual = nodedata->start_energy -  total_consumption;
  			dio->residual_energy = nodedata->energy_residual;	
		}
		else
			dio->residual_energy = 0;	

		header->data 	  = dio;		
		header->type_code = RPL_CODE_DIO;
		header->source    = c->node;
		
		if (args != NULL) {
			nodeid_t *unicast;
			unicast = args;
			destination.id = *unicast;
			destination.position = *get_node_position(*unicast);
			header->next      = *unicast;
			//printf("[RPL ICMP6]node %d send dio to %d nr %d time %llu\n", c->node, *unicast, dio->nr_sequence, get_time());
		}
		else {
			destination.id = ADDR_MULTI_CTREE;
			destination.position.x = -1;
			destination.position.y = -1;
			destination.position.z = -1;
			header->next = ADDR_MULTI_CTREE;			
			//printf("[RPL ICMP6]node %d send broadcast DIO nr %d time %llu\n", c->node, dio->nr_sequence, get_time());
		}
		

		
		if (SET_HEADER(&c0, packet, &destination) == -1){
			free(dio);
			packet_dealloc(packet);
			return -1;
		}
		
		//printf("[RPL ICMP6]node %d send dio nr %d time %llu\n", c->node, dio->nr_sequence, get_time());
		
		//stats		
		//rpl_time_parent_t *reset_time = malloc(sizeof(rpl_time_parent_t));
		//reset_time->time = get_time();
		//reset_time->parent_id = c->node;
		//	das_insert(nodedata->time_sent_dio, reset_time);
		
		nodedata->dio_tx++;
		
		//printf("[RPL_TX_DIO] Node ID = %d size %d", c->node, packet->size);

		TX(&c0, packet);
				
		//RESET dio_counter 
		dag->dio_counter = 0;
		return 0;
	}
	else {
		//printf("[RPL_TX_DIO] Node = %d supressing DIO transmission c: (%d) >= (%d) :k\n", c->node, dag->dio_counter, dag->dio_redundancy);
		
		//RESET dio_counter 
		dag->dio_counter = 0;		
		return 1;
	}
}

/* ************************************************** */
/*					Send DIS	   					  */ 
/* ************************************************** */

int send_dis(call_t *c, void *args){
	struct nodedata *nodedata = get_node_private_data(c);
	
	// Broadcast a neighbour packet 
	entityid_t *down = get_entity_links_down(c);
	call_t c0 = {down[0], c->node};
	
	destination_t destination;
	
	packet_t *packet = packet_alloc(c, nodedata->overhead[0] + sizeof(rpl_packet_header_t) + sizeof(rpl_dis_t));
	struct rpl_packet_header *header = (struct rpl_packet_header *) (packet->data + nodedata->overhead[0]);
	
	rpl_dis_t *dis = malloc(sizeof(rpl_dis_t));
	
	dis->flags		= 0;
	dis->reserved	= 0;
	
	header->type_code = RPL_CODE_DIS;
	header->source    = c->node;
	header->data	  = dis;
	
	if (args != NULL) {
		rpl_parent_t *parent = (rpl_parent_t *)args;
		destination.id = parent->id;
		destination.position = *get_node_position(parent->id);
		header->next = parent->id;
		printf("[RPL_SEND_DIS] Node %d send DIS to %d.\n", c->node, parent->id);
	}
	else {
		destination.id = ADDR_MULTI_CTREE;
		destination.position.x = -1;
		destination.position.y = -1;
		destination.position.z = -1;
		header->next = ADDR_MULTI_CTREE;
		printf("[RPL_SEND_DIS] Node %d send DIS in broadcast.\n", c->node);
	}
	
	
	if (SET_HEADER(&c0, packet, &destination) == -1){
		free(dis);
		packet_dealloc(packet);
		return -1;
	}
	
	TX(&c0, packet);
    
	// For stats
	nodedata->dis_tx++;
	
	return 0;
}
