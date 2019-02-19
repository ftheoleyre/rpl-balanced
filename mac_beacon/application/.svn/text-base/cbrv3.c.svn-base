/**
 *  \file   cbrv3.c
 *  \brief  CBR application
 *  \author Elyes Ben Hamida & Quentin Lampin & Fabrice Theoleyre
 *  \date   2012
 **/
#include <stdio.h>

#include <include/modelutils.h>


//packet format and other stuff
#include "cbrv3.h"


/* ************************************************** */
/* ************************************************** */
model_t model =  {
    "CBR application v3",
    "Fabrice Theoleyre",
    "0.1",
    MODELTYPE_APPLICATION, 
    {NULL, 0}
};

//---------------------------------------------------------------------
//                  GLOBAL VARIABLES
//---------------------------------------------------------------------

//global variable (unique value for all the CBR processes)
//0 = INVALID_SEQNUM
uint64_t sequence   = 1;





//---------------------------------------------------------------------
//                  PACKET TRANSMISSION 
//---------------------------------------------------------------------

//transmits a CBR packet to the destination dest
void tx(call_t *c, nodeid_t dest) {
    struct _cbr_private *nodedata = get_node_private_data(c);
    array_t *down = get_entity_bindings_down(c);
    packet_t *packet;

    //packet size
    if (dest == nodedata->multicast_dest)
        packet = packet_create(c, nodedata->overhead + sizeof(cbrv3_packet_header), nodedata->multicast_packet_size);
    else
        packet = packet_create(c, nodedata->overhead + sizeof(cbrv3_packet_header), nodedata->unicast_packet_size);
   
    
    //set the destination
    call_t c0 = {down->elts[0], c->node, c->entity};
    destination_t destination = {dest, {0, 0, 0}};
        
    if (SET_HEADER(&c0, packet, &destination) == -1) {
        packet_dealloc(packet);
        return;
    }


    //information in the CBR packet
    cbrv3_packet_header *header = (cbrv3_packet_header*) (packet->data + nodedata->overhead);
    header->source      = c->node;
    header->sequence	= sequence++;
    
	//printf("[CBR_TX] Node %d sends a packet to %d: seq=%lu size=%d bytes real_size=%d bits T = %ld\n", c->node, dest, (long unsigned int)header->sequence, packet->size, packet->real_size, (long int)get_time());
    
    TX(&c0, packet);
}


//---------------------------------------------------------------------
//                  PERIODIC TIMER
//---------------------------------------------------------------------

//trigger to send a unicast packet
int send_message_unicast(call_t *c, void *args) {
    int dest;
    
    //PAN coordinator
    if (c->node == CBR_PAN_COORD_ID)
        dest = get_random_integer_range(1, get_node_count() - 1);
    
    //other nodes:to the PAN coordinator (convergecast)
    else
        dest = CBR_PAN_COORD_ID;
    
    tx(c, dest);    
    return 0;
}

//trigger to send a multicast packet
int send_message_multicast(call_t *c, void *args) {
    struct _cbr_private *nodedata = get_node_private_data(c);
   
     tx(c, nodedata->multicast_dest);
    return 0;
}




//---------------------------------------------------------------------
//                  CBR PACKET RECEPTION
//---------------------------------------------------------------------

//NB: all stats are maintained in the MAC layer because of simplicity (printing)
void rx(call_t *c, packet_t *packet) {  
    packet_dealloc(packet);
}


//---------------------------------------------------------------------
//            CREATION / INITIATION / DESTRUCTION of the node
//---------------------------------------------------------------------
int init(call_t *c, void *params) {
    return 0;
}

int destroy(call_t *c) {
    return 0;
}

int setnode(call_t *c, void *params) {
    struct _cbr_private *nodedata = malloc(sizeof(struct _cbr_private));
    param_t *param;
    
    /* default values */
    nodedata->start                 = 0;
    nodedata->unicast_period		= 1e9;              //every second
    nodedata->unicast_packet_size	= 128 * 8;          //in bits
    nodedata->multicast_period		= PERIOD_INFINITE;  //no multicast
    nodedata->multicast_packet_size	= 128 * 8;          //in bits
    nodedata->multicast_dest        = ADDR_MULTI_DISC;
    
  
    
    /* get parameters */
    das_init_traverse(params);
    while ((param = (param_t *) das_traverse(params)) != NULL) {
		
        //the node is up
        if (!strcmp(param->key, "start")) {
            if (get_param_time(param->value, &(nodedata->start))) {
                goto error;
            }
        }        
        //unicast traffic
         if (!strcmp(param->key, "unicast-packet_size")) {
            if (get_param_integer(param->value, &(nodedata->unicast_packet_size))) {
                goto error;
            }
        }         
        if (!strcmp(param->key, "unicast-period")) {
            if (get_param_time(param->value, &(nodedata->unicast_period))) {
                goto error;
            }
        }        
        //multicast traffic
        if (!strcmp(param->key, "multicast-packet_size")) {
            if (get_param_integer(param->value, &(nodedata->multicast_packet_size))) {
                goto error;
            }
        }         
        if (!strcmp(param->key, "multicast-period")) {
            if (get_param_time(param->value, &(nodedata->multicast_period))) {
                goto error;
            }
         }
        if (!strcmp(param->key, "multicast-dest")) {
            nodedata->multicast_dest = *(uint16_t*) param->value;
            
            //particular case for multicast addresses
            if (strcmp(param->value, "ctree") == 0)
                nodedata->multicast_dest = ADDR_MULTI_CTREE;
            else if (strcmp(param->value, "disc") == 0)
                nodedata->multicast_dest = ADDR_MULTI_DISC;
            else if (strcmp(param->value, "flood-disc") == 0)
                nodedata->multicast_dest = ADDR_MULTI_FLOOD_DISC;
            else if (strcmp(param->value, "flood-ctree") == 0)
                nodedata->multicast_dest = ADDR_MULTI_FLOOD_CTREE;

            //and then an integer value
            else
                nodedata->multicast_dest = strtol(param->value, NULL, 10);
  
            //the destination was perhaps specified in hexa
            if (nodedata->multicast_dest == 0)
                nodedata->multicast_dest = strtol(param->value, NULL, 16);

            //bug
            if (nodedata->multicast_dest == 0){                          
                fprintf(stderr, "Invalid destination parameter, %s\n", param->value);                  
                exit(5);
            }          
        }
    }
 
    
    set_node_private_data(c, nodedata);
    return 0;
    
error:
    free(nodedata);
    return -1;
}

int unsetnode(call_t *c) {
    struct _cbr_private *nodedata = get_node_private_data(c);
    free(nodedata);    
    return 0;
}


//---------------------------------------------------------------------
//                  BOOSTRAPING
//---------------------------------------------------------------------

int bootstrap(call_t *c) {
    struct _cbr_private *nodedata = get_node_private_data(c);
    array_t *down = get_entity_bindings_down(c);
    call_t c0 = {down->elts[0], c->node, c->entity};
    
	//sink case (0) -> to be bidirectionnal, sends more frequently unicast packets
	if (c->node == CBR_PAN_COORD_ID)
		nodedata->unicast_period = nodedata->unicast_period / (get_node_count() - 1);

    //printf("PTR UN %f\n", nodedata->unicast_period / 1e9);
    
    /// overhead for the application layer
    nodedata->overhead = GET_HEADER_SIZE(&c0);
    
    // periodic unicast packets
    if (nodedata->unicast_period != PERIOD_INFINITE){
        nodedata->unicast_timer = create_timer(c, send_message_unicast, never_stop, periodic_trigger, &(nodedata->unicast_period));        
        start_timer(nodedata->unicast_timer, nodedata->start + get_random_double() * nodedata->unicast_period); 
    }
    
    // periodic multicast packets
    if (nodedata->multicast_period != PERIOD_INFINITE){
        nodedata->multicast_timer = create_timer(c, send_message_multicast, never_stop, periodic_trigger, &(nodedata->multicast_period));
        start_timer(nodedata->multicast_timer, nodedata->start + get_random_double() * nodedata->multicast_period);
    }   
    
    return 0;
}

int ioctl(call_t *c, int option, void *in, void **out) {
	struct _cbr_private *nodedata = get_node_private_data(c);
	
	switch (option) {
        case CBR_START:
			nodedata->start = *((uint64_t *) in);
			if (nodedata->unicast_period != PERIOD_INFINITE){
				start_timer(nodedata->unicast_timer, nodedata->start + get_random_double() * nodedata->unicast_period);
			}
			if (nodedata->multicast_period != PERIOD_INFINITE){
				start_timer(nodedata->multicast_timer, nodedata->start + get_random_double() * nodedata->multicast_period);
			}
			printf("[CBR_START] T = %llu\n", nodedata->start); 
			break;
		default:
			break;
	}
	
    return 0;
}

//---------------------------------------------------------------------
//                  INTERFACE
//---------------------------------------------------------------------
application_methods_t methods = {rx};
