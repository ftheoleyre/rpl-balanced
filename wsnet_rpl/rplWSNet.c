/**
 *  \file   rplWSNet.h
 *  \brief  RPL implemented for WSNet
 *  \author Bogdan PAVKOVIC & Oana IOVA
 *  \date   2012
 **/
#include "rplWSNet.h"

model_t model =  {
    "RPL implementation for WSNet",
    "Oana IOVA",
    "0.1",
    MODELTYPE_ROUTING, 
    {NULL, 0}
};

int sink_rx = 0;
int to_sink_tx = 0;





/* ************************************************** */
/*         Init/Destroy Entity                        */
/* ************************************************** */

//Read the entity variables from the xml config file
int init(call_t *c, void *params) {
	printf("[RPL_INIT] Start RPL git multipath for ad hoc journal normalized.\n");	
	
	if (global_cbr_stats == NULL)
		global_cbr_stats = das_create();
				
	return 0;
}

//Destroy entity
int destroy(call_t *c) {		
	struct entitydata *entitydata = get_entity_private_data(c);
	free(entitydata);

    return 0;
}





/* ************************************************** */
/*                 Init/Unset Nodedata                */
/* ************************************************** */

//Read the node variables from the xml config file
int setnode(call_t *c, void *params) {
    //printf("[RPL_SETNODE] Setnode node = %d.\n", c->node);
    
    param_t *param;
    struct nodedata *nodedata = malloc(sizeof(struct nodedata));
    int i = get_entity_links_down_nbr(c);
    
    int test_load_step;
	
    //default values stats
    nodedata->dio_tx   = 0;
    nodedata->dio_rx   = 0;
    nodedata->dis_tx   = 0;
    nodedata->dis_rx   = 0;
	nodedata->data_tx  = 0;
	nodedata->data_rx  = 0;
	nodedata->reset_trickle = 0;
	nodedata->change_parent = 0;
	nodedata->stats_change_parent = 0;
	nodedata->energy_spent = 0;
	nodedata->stats_data_tx = 0;
	
	//for elt 
	nodedata->start_energy		= START_ENERGY; //Joule (= 880 mAh)
	nodedata->energy_residual	= nodedata->start_energy;
	nodedata->my_traffic 		= 1; // equivalent of sending 1 pkt every 1000 seconds  (1 bit/s)
	nodedata->old_tx 	 		= 0; //for the moving average of the traffic
	nodedata->after_bootstrap 	= 0; //for knowing when if i chose my preferred parent or not yet
	nodedata->max_increase		= 0.2; //max increase/decrease from old weight to new weight
		
	nodedata->parents = das_create();
	nodedata->my_bottlenecks = das_create();
	nodedata->time_reset_trickle = das_create();
	nodedata->time_sendto = das_create();
	nodedata->time_sent_dio = das_create();
	nodedata->parent_changing = das_create();
	nodedata->mac_neigh_retransmit = das_create();  
	nodedata->b_old_traffic = das_create();

	  
	if (c->node == 0){
		nodedata->type = SINK;
		sink_x = get_node_position(c->node)->x;
		sink_y = get_node_position(c->node)->y;
	}
	else {
		nodedata->type = SENSOR;
	}
	
    //alloc overhead memory
    if (i) 
        nodedata->overhead = malloc(sizeof(int) * i);
    else 
        nodedata->overhead = NULL;	 
        
    /* get parameters */
    das_init_traverse(params);
    while ((param = (param_t *) das_traverse(params)) != NULL) {
		//ocp is the objective function's code (elt = 5, etx = 1, of0 = 0)
        if (!strcmp(param->key, "ocp")) {
            if (get_param_integer(param->value, &(nodedata->ocp_from_xml))) {
                printf("RPL: OF read does not exist!\n");
                return -1;
            }
        }
        if (!strcmp(param->key, "bottlenecks")) {
            if (get_param_integer(param->value, &(nodedata->max_bottlenecks))) {
                printf("RPL:not a number (needed the maximum number of bottlenecks to advertise)!\n");
                return -1;
            }
        }
    	if (!strcmp(param->key, "load_step")) {
            if (get_param_integer(param->value, &(test_load_step))) {
                printf("RPL:not a number (needed the load step)! Default should be 10 (will be divided by 100)\n");
                return -1;
            }
        }
    }
    
    nodedata->load_step = (float) test_load_step / 100;
    
    set_node_private_data(c, nodedata);
    
    return 0;
}


//Unset node data
int unsetnode(call_t *c) {
    struct nodedata *nodedata = get_node_private_data(c);
    	
	printf("\nunset node %d\n", c->node);
	
	 //energy consummed for each node
    double      global_consumption = 0;
    uint64_t    global_duration = 0;
    int j;

    //global stats
    for(j=0; j<RADIO_NB_MODES; j++){   
        global_consumption   += nodedata->radio_energy->consumption[j];
        global_duration      += nodedata->radio_energy->duration[j];            
    }
    
    nodedata->energy_spent = global_consumption;
    
    printf("%d, consumption %f\n", c->node, global_consumption);	
    
    //rpl_parent_t *q;
    das_init_traverse(nodedata->parents);
	/*while((q = (rpl_parent_t*) das_traverse(nodedata->parents)) != NULL)
		if(q->id == 0)
			calculate_my_elt(c, q);*/
	
		//get E_res = residual energy

    //for(j=0; j<RADIO_NB_MODES; j++){   
    //	printf("Mode %d consumption: %f time: %llu\n", j, nodedata->radio_energy->consumption[j],nodedata->radio_energy->duration[j]/1000000000);
  	//}
	
	if(c->node != 0){
		//das_init_traverse(nodedata->parents);
		//while((q = (rpl_parent_t*) das_traverse(nodedata->parents)) != NULL)
		//	printf("Node %d to parent %d has etx %f\n", c->node, q->id, q->etx);
		rpl_dag_t *dag = rpl_get_dag(nodedata, RPL_ANY_INSTANCE);	
		if(dag){
			if(dag->preferred_parent){
				printf("Node %d preferred parend is: %d.\n", c->node, dag->preferred_parent->id);
				//nodedata->last_known_rank = DAG_RANK(dag->of->calculate_rank(c, dag->preferred_parent, 0), dag);//dag->of->calculate_rank(c, dag->preferred_parent, 0);//DAG_RANK(dag->of->calculate_rank(c, dag->preferred_parent, 0), dag)
				//printf("kiss %d, %d, %d\n", c->node, nodedata->last_known_rank, nodedata->data_tx);
			}
			else
				printf("Node %d has no parent\n", c->node);
		} //endif(dag)
		printf("preferred parent changed %d times\n", nodedata->change_parent);
		
		etx_elt_t *etx_elt;								
		das_init_traverse(nodedata->mac_neigh_retransmit);
		while((etx_elt = (etx_elt_t*) das_traverse(nodedata->mac_neigh_retransmit)) != NULL){
			printf(">>>>>>>>>>>>>>>>>>>>>>>>Node %d sent to node %d, %d packets on the radio, %d from RPL, %d tx_once %d drops, %d acks.\n", c->node, etx_elt->neighbor, etx_elt->packs_tx, etx_elt->packs_net, etx_elt->packs_tx_once, etx_elt->drops, etx_elt->acks);			
			rpl_parent_t *pp = rpl_find_parent(c, etx_elt->neighbor);
			if(pp != NULL){
				printf("etx to parent %d is %f\n", pp->id, pp->etx_4_elt);
				printf("Parent %d has %d bottlenecks\n", pp->id, das_getsize(pp->bottlenecks));
			}
		}
		printf("Node %d has %d bottlenecks: ", c->node, das_getsize(nodedata->my_bottlenecks));
		
		bottleneck_t *b;
		das_init_traverse(nodedata->my_bottlenecks);
		while((b = (bottleneck_t*) das_traverse(nodedata->my_bottlenecks))!= NULL){
			printf("%d with elt %f", b->bottlen_id, b->bottlen_const / b->bottlen_traffic);
		}
		printf("\n");
	}	
		
	if(c->node == 0)
				printf("Total sent to sink %d total receive by the sink %d PDR=%f\n", to_sink_tx, sink_rx, (float)sink_rx/to_sink_tx);

	nodedata->distance = sqrt(((sink_x - get_node_position(c->node)->x)*(sink_x - get_node_position(c->node)->x)) + ((sink_y - get_node_position(c->node)->y)*(sink_y - get_node_position(c->node)->y)));
		
	print_stats(c);

	if(das_getsize(nodedata->parents))
		das_destroy(nodedata->parents);
		
	if(das_getsize(nodedata->my_bottlenecks))
		das_destroy(nodedata->my_bottlenecks);
	
	if(das_getsize(nodedata->time_sent_dio))
		das_destroy(nodedata->time_sent_dio);
		
	if(das_getsize(nodedata->time_reset_trickle))
		das_destroy(nodedata->time_reset_trickle);
		
	if(das_getsize(nodedata->time_sendto))
		das_destroy(nodedata->time_sendto);
		
	if(das_getsize(nodedata->parent_changing))
		das_destroy(nodedata->parent_changing);
		
	if(das_getsize(nodedata->b_old_traffic))
		das_destroy(nodedata->b_old_traffic);		
		
    if (nodedata->overhead) 
        free(nodedata->overhead);
           
    free(nodedata);
        
    return 0;
}





/* ************************************************** */
/*           CREATE Node                              */
/* ************************************************** */

//A node is born
int bootstrap(call_t *c) {
    struct nodedata *nodedata = get_node_private_data(c);
    int i = get_entity_links_down_nbr(c);
    entityid_t *down = get_entity_links_down(c);	
    
    //printf("[bootstrap] rpl_header size %d dio size %d\n", sizeof(rpl_packet_header_t), sizeof(rpl_dio_t));

		
		
	//nr of retransmissions (a pointer toward the stats updated by the MAC module)
    call_t c_mac = {get_entity_bindings_down(c)->elts[0], c->node, c->entity};
    IOCTL(&c_mac, MAC_IOCTL_RETRANSMIT, NULL, (void *)&(nodedata->mac_neigh_retransmit));
	//printf("RPL_IOCTL node %d pointer %d\n\n", c->node, (nodedata->mac_neigh_retransmit));	
	
		
	//energy consumption (a pointer toward the stats updated by the Radio module)
	call_t c_radio = {get_radio_entities(c)->elts[0], c->node, c->entity};
    IOCTL(&c_radio, RADIO_IOCTL_GETENCONSUMMED, NULL, (void*)&(nodedata->radio_energy));


		
    while (i--) {
        call_t c0 = {down[i], c->node};
        
        if ((get_entity_type(&c0) != MODELTYPE_ROUTING) 
            && (get_entity_type(&c0) != MODELTYPE_MAC)) {
            nodedata->overhead[i] = 0;
        } else {
            nodedata->overhead[i] = GET_HEADER_SIZE(&c0);
        }
    }
	
	
	//compute traffic and ETX every RECOMPUTE_TRAFFIC seconds
	scheduler_add_callback(get_time(), c, compute_traffic, NULL);

	//if residual metric is used, recompute residual energy of the node
	//if(nodedata->ocp_from_xml == 6)
	//	scheduler_add_callback(get_time(), c, recompute_energy, NULL);

	if(nodedata->type == SINK){
		nodedata->energy_residual = 1000 * nodedata->start_energy;
		rpl_set_root(c, nodedata, c->node);	
		
	}
	else{
		//compute traffic and ETX every RECOMPUTE_TRAFFIC seconds
		scheduler_add_callback(get_time(), c, compute_traffic, NULL);
	
		//uncomment when enabling DIS messages
		//printf("[RPL_BOOTSTRAP] Node %d I'm in else. I should send DIS now.\n", c->node);
		//scheduler_add_callback(get_time(), c, dis_timer, NULL);
	}
	
    
    return 0;
}





/* ************************************************** */
/*           	IOCTL                                 */
/* ************************************************** */

//ioctl for inter-layer communication
int ioctl(call_t *c, int option, void *in, void **out) {
    struct nodedata *nodedata = get_node_private_data(c);
    rpl_dag_t *dag;
    dag = rpl_get_dag(nodedata, RPL_ANY_INSTANCE);
	rpl_parent_t *p;     
        
    switch(option){
    	
    	case REMOVE_PARENT:		//remove a parent because it was removed by the MAC layer
			;
			if(dag != NULL){
				p = rpl_find_parent(c, (intptr_t)*out);
				if(p != NULL){
					printf("*************************node %d received IOCTL_REMOVE parent %d\n\n", c->node, p->id);
					rpl_remove_parent(c, nodedata, dag, p);
				}
			}
		
		break;
			
		default:
            printf("Unknown IOCTL message\n");
			break;
	}
	
	return 0;
}





/* ************************************************** */
/*         GET HEADER size                            */
/* ************************************************** */

int get_header_size(call_t *c) {
	struct nodedata *nodedata = get_node_private_data(c);
	return nodedata->overhead[0] + sizeof(rpl_packet_header_t);// + sizeof(rpl_data_t));
}
int get_header_real_size(call_t *c) {
	struct nodedata *nodedata = get_node_private_data(c);
	return nodedata->overhead[0] + sizeof(rpl_packet_header_t); //+ sizeof(rpl_data_t));
}





/* **************************************************        */
/*            ROUTING function - Set header                  */
/* Function gets already created packet - just change header */ 
/* **************************************************        */

int set_header (call_t *c, packet_t *packet, destination_t *dst) {
    struct nodedata *nodedata = get_node_private_data(c);
    call_t c0 = {get_entity_bindings_down(c)->elts[0], c->node, c->entity};
	destination_t destination; //	destination (in our case) preferred parent; final destination = SINK
	rpl_packet_header_t *header = (rpl_packet_header_t *) (packet->data + nodedata->overhead[0]);	

	//the SINK does not send any data packets
	if(nodedata->type == SINK){
		return -1;
	}

	//get preferred parent
	int next_hop;
	rpl_dag_t *dag = rpl_get_dag(nodedata, RPL_ANY_INSTANCE);
	if(dag != NULL)
		next_hop = get_next_hop(c,dag);
	else
		return -1;
	
	header->type_code   = RPL_DATA_PACKET;
	header->source      = c->node;
	
	if(next_hop != -1){
		header->next = next_hop;
		destination.id = next_hop;
	}
	else{
		printf("[SET_HEADER] Node %d has no parent to send the packet. \n", c->node);
		return -1;
	}
	
	if (SET_HEADER(&c0, packet, &destination) == -1) {
		packet_dealloc(packet);
		return -1;
	}
	
    return 0;
}





/* **************************************************************************************** */
/*  FORWARD function - Forward a packet. 													*/
/*	Receives packet from a child and sends it to the preferred parent                       */
/* **************************************************************************************** */

void forward(call_t *c, packet_t *packet){
    struct nodedata *nodedata = get_node_private_data(c);
    call_t c0 = {get_entity_bindings_down(c)->elts[0], c->node, c->entity};
   
    destination_t destination; //destination of my parent

	//the SINK does not send any data packets
	if(nodedata->type == SINK){
		packet_dealloc(packet);
		return;
	}

    //find preferred parent
	int next_hop;
	rpl_dag_t *dag = rpl_get_dag(nodedata, RPL_ANY_INSTANCE);
	if(dag != NULL)
		next_hop = get_next_hop(c,dag);
	else
		return;
		
    rpl_packet_header_t *header = (rpl_packet_header_t *) (packet->data + nodedata->overhead[0]);	
	
	header->type_code   = RPL_DATA_PACKET;
	
	//set the destination to be the next hop
	if(next_hop != -1){
		header->next = next_hop;
		destination.id = next_hop;
	}
	else{
		printf("[forward] Node %d has no parent to send the packet. \n", c->node);
		return;
	}

		
	if (SET_HEADER(&c0, packet, &destination) == -1) {
		packet_dealloc(packet);
		return ;
	}
    
    //stats
    if(header->type_code == RPL_DATA_PACKET){
    	nodedata->data_tx++;
    	if(get_time() > START_TIME)
    		nodedata->stats_data_tx++;
	}
		
		
	
	//printf(">>>>[RPL_FWD]Node %d sent packet to %d.\n", c->node, header->next);
	
	TX(&c0, packet);
	
	//To update CBR stats
	stat_cbr_info *stat_elem;
	das_init_traverse(global_cbr_stats);
	while ((stat_elem = (stat_cbr_info*) das_traverse(global_cbr_stats)) != NULL) {
		if ((stat_elem->sequence == header->pkt_sequence) && (stat_elem->src == header->source)){
			nodeid_t *id_ptr = malloc(sizeof(nodeid_t));
			*id_ptr = c->node;
			das_insert(stat_elem->route, (void*)id_ptr); //add node to the route;
		}		
	}		
}





/* **************************************************************************************** */
/*  TX function - Just send the packet. Interaction with APP layer                          */
/* **************************************************************************************** */

void tx (call_t *c, packet_t *packet) {
	struct nodedata *nodedata = get_node_private_data(c);
	call_t c0 = {get_entity_bindings_down(c)->elts[0], c->node, c->entity};
	
	//the SINK does not send any data packets
	if(nodedata->type == SINK){
		packet_dealloc(packet);
		return ;
	}
	
	//For stats
	struct rpl_packet_header *header = (struct rpl_packet_header *) (packet->data + nodedata->overhead[0]);
	
	if(header->type_code == RPL_DATA_PACKET){
		if(get_time() > START_TIME) 
			nodedata->stats_data_tx++;
		nodedata->data_tx++;
		to_sink_tx++;
		header->pkt_sequence = nodedata->data_tx;
			
		//printf(">>>>[RPL] NODE = %d SENT DATA to %d sequence %d\n", c->node, header->next, header->pkt_sequence);
		
		//global_cbr_stats
		stat_cbr_info *stat_elem = (stat_cbr_info*) malloc(sizeof(stat_cbr_info));
		stat_elem->src				= c->node;
		stat_elem->dest				= 0;
		stat_elem->time_generated	= get_time();
		stat_elem->time_received	= 0;
		stat_elem->sequence			= header->pkt_sequence;
		stat_elem->route			= das_create();
		das_insert(global_cbr_stats, stat_elem);

		nodeid_t *id_ptr = malloc(sizeof(nodeid_t));
		*id_ptr = c->node;
		das_insert(stat_elem->route, (void*)id_ptr); //first node on the route is the source node
		//printf("[RPL]Node %d sent packet to %d wsize %d.\n", c->node, header->next, packet->size);

		
		//rpl_dag_t *dag = rpl_get_dag(nodedata, RPL_ANY_INSTANCE);
		//rpl_parent_t *prefer_parent = NULL;     
		//if(dag != NULL)
		//	prefer_parent = dag->preferred_parent;			
	}
	
	//printf("[RPL]Node %d sent packet to %d wsize %d.\n", c->node, header->next, packet->size);
	
	TX(&c0, packet);
}





/* ************************************************** */
/*          RECEIVE function                          */
/* ************************************************** */

void rx(call_t *c, packet_t *packet) {
    struct nodedata *nodedata = get_node_private_data(c);

	rpl_dag_t *dag;
	rpl_parent_t *prefer_parent = NULL;
	
	dag = rpl_get_dag(nodedata, RPL_ANY_INSTANCE);
	if(dag != NULL)
		prefer_parent = dag->preferred_parent;
		
    struct rpl_packet_header *header = (struct rpl_packet_header *) (packet->data + nodedata->overhead[0]);
	
	//printf("T= %lf [RX] Node %d (%lf %lf) received a packet (type=%d)from %d sequence = %d size = %d !\n",((float)get_time())/1e9f, c->node, get_node_position(c->node)->x, get_node_position(c->node)->y,header->type, header->source, header->seq, packet->size);


	switch(header->type_code){
		case RPL_CODE_DIS:
			nodedata->dis_rx++;
			//printf("[RX] Node %d has received DIS.\n", c->node);
			//Respond to DIS only if you belong to DAG
			if (dag != NULL) {
				//TODO - this should be redone to work with TDMA 
				//If the DIS is BROADCAST, reset DIO timer...
				uint64_t delay = 0;
				if (header->next == BROADCAST_ADDR){
					printf("[RX]Node %d reset timer because of BROADCAST DIS.\n", c->node);
					rpl_reset_dio_timer(c, dag, delay, 1); 
				}
				else{         //...otherwise respond to sender
					send_dio(c, &(header->source));
					printf("[RX]Node %d received DIS from %d.\n", c->node, header->source);
				}
			}
			packet_dealloc(packet);
			break;
			
		case RPL_CODE_DIO:
			//keep statistics
			nodedata->dio_rx++;
			
			rpl_dio_t *dio = header->data;			
			//printf("\t[RPL_RX_DIO] Node ID = %d at T = %llu from %d seq = %d size = %d\n", c->node, get_time(), header->source, dio->nr_sequence, packet->size);
			//printf("\t[RPL_RX_DIO] Node ID = %d at T = %llu from %d size = %d\n\n", c->node, get_time(), header->source, packet->size);

			//Suprisingly this function does it ALL - process DIO, creates DAG, add parent
			rpl_process_dio(c, header->source, dio);
			
			packet_dealloc(packet);
			break;	
			
		case RPL_DATA_PACKET:
			//if the packet i received is a copy because the MAC layer send it multiple times
			das_init_traverse(global_cbr_stats);
			stat_cbr_info *stat;  
			while((stat = (stat_cbr_info*) das_traverse(global_cbr_stats)) != NULL) {
				if((stat->sequence == header->pkt_sequence) && (stat->src == header->source)){
				    nodeid_t *id_ptr;
					das_init_traverse(stat->route);
            		while((id_ptr = (nodeid_t*) das_traverse(stat->route)) != NULL){
            			if(c->node == *id_ptr){
            				packet_dealloc(packet);
							return;
            			}
            		}
				}
			}
			
			//keep statistics
			nodedata->data_rx++;
				
			//printf("\t[RPL_RX_DATA] Node = %d received From  = %d sequence %d header->next = %d size %d\n", c->node, header->source, header->pkt_sequence, header->next, packet->size);
			
			//Packet repported to the SINK
			if (nodedata->type == SINK) {
				//printf("\t>>>>[RPL_SUCCESS_DATA] Node ID = %d at T = %llu from %d sequence =%d size = %d\n", c->node, get_time(), header->source, header->pkt_sequence, packet->size);
				
				sink_rx ++;
				
				//To update CBR stats
				stat_cbr_info *stat_elem;
				das_init_traverse(global_cbr_stats);
				while((stat_elem = (stat_cbr_info*) das_traverse(global_cbr_stats)) != NULL) {
				 	if((stat_elem->sequence == header->pkt_sequence) && (stat_elem->src == header->source)){
						if(stat_elem->time_received == 0){
							nodeid_t *id_ptr = malloc(sizeof(nodeid_t));
							*id_ptr = c->node;
							das_insert(stat_elem->route, (void*)id_ptr); //add final node to the route; destination node
							
							stat_elem->time_received = get_time();
							//printf("from %d delay is = %llu: \n", c->node, stat_elem->time_received - stat_elem->time_generated);
						}
						else{
							packet_dealloc(packet);
							return;
						}
					}
				}
				
				//DATA successfully received - push it on APP layer
				array_t *up = get_entity_bindings_up(c);
				
				// Drop packet if no upper layer available 
				if (up == NULL) {
					packet_dealloc(packet);                 
				} 
				else {
					int j = up->size;
					
					while (j--) {
						call_t c_up = {up->elts[j], c->node, c->entity};
						packet_t *packet_up;	     
						if (j > 0) {
							packet_up = packet_clone(packet);         
						} else {
							packet_up = packet;
						}
						RX(&c_up, packet_up);
					}
				}			
			}
			else 
				if(prefer_parent != NULL){
					if(header->source != prefer_parent->id){
						//printf("Node %d OK to forward to %d\n", c->node, prefer_parent->id);
						forward(c, packet);
				 	}
				 	else{ 
						printf("\t Node %d: Not forwarded; bucle. Local repair started.\n", c->node);
						rpl_local_repair(c, nodedata, dag);
					}
				}
				else
					printf("Not forwared; no parent to forward to \n");
			break;	
		default:			
			printf("Error in packet type\n");
			break;
			
	}
}





routing_methods_t methods = {rx,
	tx,
	set_header,
	get_header_size,
	get_header_real_size};
