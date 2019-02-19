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

	//If c<k, send DIO
	if(dag->dio_counter < dag->dio_redundancy) {		
		//Broadcast a neighbour packet 
		//printf("[RPL_TX_DIO] Node ID = %d\n size %llu", c->node, );
		
		//update my bottlenecks to the current parents in my list (equivalent of recomputing my ELT and checking if i'm not the new bottlenck
		update_my_bottlenecks(c,dag);
		
		entityid_t *down = get_entity_links_down(c);
		call_t c0 = {down[0], c->node};
		
		int my_size = das_getsize(nodedata->my_bottlenecks);
		int actual_size = my_size;
		
		if(my_size > nodedata->max_bottlenecks)
			actual_size = nodedata->max_bottlenecks;
		
	
		packet_t *packet = packet_alloc(c, nodedata->overhead[0] + sizeof(rpl_packet_header_t) + sizeof(rpl_dio_t) + actual_size*sizeof(dio_bottleneck_t));
		rpl_packet_header_t *header = (rpl_packet_header_t *) (packet->data + nodedata->overhead[0]);	
		
		//printf("[SEND_DIO] node %d, packet size %d, rpl_header size %d dio size %d sizeof bottlenecks %d\n", c->node, packet->size, sizeof(rpl_packet_header_t), sizeof(rpl_dio_t), actual_size*sizeof(dio_bottleneck_t));
		
		
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
		//dio->nr_sequence 	= ++dag->dio_sequence;
		
		//info about the bottlenck
		dio->bottlenecks 		= das_create();
		
		bottleneck_t *b;
		dio_bottleneck_t *new_bottlenck;	
		
		das_init_traverse(nodedata->my_bottlenecks);
		while((b = (bottleneck_t*) das_traverse(nodedata->my_bottlenecks)) != NULL){
			new_bottlenck = (dio_bottleneck_t *) malloc(sizeof(dio_bottleneck_t));
			//normalize the values
			new_bottlenck->bottlen_id 		 = b->bottlen_id;   	//4 bytes, stays like this (int)
			if(b->bottlen_id == 0)
				new_bottlenck->bottlen_const	 = 65535; 	// 2 bytes (sink has max value for bottlen_const)
			else
				new_bottlenck->bottlen_const	 = (b->bottlen_const * 65535) / MAX_BOTTLEN_CONST; 	// 2 bytes
  			
  			if(((b->bottlen_traffic * 255) / MAX_TRAFFIC) < 1)
  				new_bottlenck->bottlen_traffic 	 = 1;
  			else 
  				new_bottlenck->bottlen_traffic = (b->bottlen_traffic * 255) / MAX_TRAFFIC;   // 1 byte
  				
  			//printf("[SEND_DIO] Node %d, bottleneck %d has traffic %f and after normalization %d \n", c->node, b->bottlen_id, b->bottlen_traffic, new_bottlenck->bottlen_traffic);
  				
  			new_bottlenck->ratio_of_traffic	 = b->ratio_of_traffic * 255;  //1 byte
			das_insert(dio->bottlenecks, new_bottlenck);
		}
		
		
		
			
		dio_bottleneck_t *b_to_delete, *bb;
		double max_elt, b_elt;	
		int max_bottlencks;
	
		max_bottlencks = das_getsize(dio->bottlenecks);
	
		while(max_bottlencks > nodedata->max_bottlenecks){
			max_elt = 0;
			b_to_delete = NULL;
			das_init_traverse(dio->bottlenecks);
			while((bb = (dio_bottleneck_t*) das_traverse(dio->bottlenecks)) != NULL){	
				if(bb->bottlen_id == 0)
					b_elt = MAX_BOTTLEN_CONST;
				else{	
					if(bb->bottlen_id == c->node)
						b_elt = ((double)(bb->bottlen_const * MAX_BOTTLEN_CONST) /65535)  / ((float)(bb->bottlen_traffic * MAX_TRAFFIC) / 255);
					else
						b_elt = ((double)(bb->bottlen_const * MAX_BOTTLEN_CONST) /65535) / ((((float) bb->ratio_of_traffic / 255) * nodedata->my_traffic) + ((float)(bb->bottlen_traffic * MAX_TRAFFIC) / 255));
				}
				
				if(max_elt < b_elt){
					max_elt = b_elt;
					b_to_delete = bb;
				}
			}
			if(b_to_delete != NULL){
				printf("[send_dio] Node %d will delete bottleneck %d", c->node, b_to_delete->bottlen_id );
				das_delete(dio->bottlenecks, b_to_delete);
				printf("[send_dio] Now %d has %d bottlenecks advertising\n", c->node, das_getsize(dio->bottlenecks));
				max_bottlencks--;
			}
		}	
			
		//printf("DIO Node %d has bottleneck id %d bottlen_const %f bottlen_traffic %f\n", c->node, dag->bottlen_id, dag->bottlen_const, dag->bottlen_traffic);

		
		//info for the residual energy metric -> min residual energy of the path
		//dio->path_energy_residual = dag->path_energy_residual;
			
		/*if(get_time() > 500000000000 && c->node == 4){
			das_init_traverse(dio->bottlenecks);
			while((b = (bottleneck_t*) das_traverse(dio->bottlenecks)) != NULL)
				das_delete(dio->bottlenecks, b);
			new_bottlenck = (bottleneck_t *) malloc(sizeof(bottleneck_t));
			new_bottlenck->bottlen_id 		 = 3;
			new_bottlenck->bottlen_const	 = MAX_ELT; 
  			new_bottlenck->bottlen_traffic 	 = 1;
  			new_bottlenck->ratio_of_traffic	 = 1; 
			das_insert(dio->bottlenecks, new_bottlenck);	
			
			
			new_bottlenck = (bottleneck_t *) malloc(sizeof(bottleneck_t));
			new_bottlenck->bottlen_id 		 = 1;
			new_bottlenck->bottlen_const	 = MAX_ELT; 
  			new_bottlenck->bottlen_traffic 	 = 1;
  			new_bottlenck->ratio_of_traffic	 = 1; 
			das_insert(dio->bottlenecks, new_bottlenck);	
			printf("DIO node %d has MAX energy \n\n", c->node);
		}*/

		
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
		
		//printf("[RPL ICMP6]node %d send dio time %llu\n", c->node, get_time());
		
		//stats		
		//rpl_time_parent_t *reset_time = malloc(sizeof(rpl_time_parent_t));
		//reset_time->time = get_time();
		//reset_time->parent_id = c->node;
		//	das_insert(nodedata->time_sent_dio, reset_time);
		
		nodedata->dio_tx++;
		
		//printf("[RPL_TX_DIO] Node ID = %d sends DIO size %d", c->node, packet->size);

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
