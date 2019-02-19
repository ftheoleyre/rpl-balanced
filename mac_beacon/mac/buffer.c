/*
 *  buffer.c
 *  
 *
 *  Created by Fabrice Theoleyre on 06/04/11.
 *  Copyright 2011 CNRS. All rights reserved.
 *
 */

#include "buffer.h"



//--------------------------------------------
//		QUEUE INSERTION / DELETION
//--------------------------------------------


//----- MANAGEMENT -------

//create an empty queue
queue_info *buffer_queue_create(){
	queue_info *queue = malloc(sizeof(queue_info));
	
	queue->size				= 0;
	queue->size_reserved	= PK_INCR;
	queue->elts				= realloc(NULL, sizeof(packet_info*) * queue->size_reserved);
	
	return(queue);
}


//removes all the elements
void buffer_queue_empty(queue_info *queue){
	int		i;
	
	//remove all the elements
	for(i=0; i<queue->size; i++)
		free(queue->elts[i]);
	
	queue->size				= 0;
	queue->size_reserved	= PK_INCR;
	queue->elts				= realloc(NULL, sizeof(packet_info*) * queue->size_reserved);
	
}

//returns the size
int buffer_queue_getsize(queue_info *queue){
	return(queue->size);
}


//destroy a queue
void buffer_queue_destroy(queue_info *queue){
	
	//removes possibly the elements
	buffer_queue_empty(queue);
	
	free(queue->elts);
	free(queue);	
}


//----- GET -------


//returns the position^th element
packet_info	*buffer_queue_get(queue_info *queue, int position){
	assert(position >= 0);
	assert(position < queue->size);
	
	return(queue->elts[position]);
}

//returns the element with seqnum
packet_info	*buffer_queue_get_seqnum(queue_info *queue, uint16_t caddr, uint8_t seqnum){
	int i;
    
 	for(i=0; i<queue->size; i++){
         
        //either we have the correct seqnum or the compressed multicast address doesn't care for seqnums
        if ((pk_get_seqnum(queue->elts[i]->packet) == seqnum) || (tools_is_addr_multicast_seqnum_replacement(caddr)))
            return(queue->elts[i]);
    }
    //not found!
    return(NULL);
}


//extracts the first packet for dest
//Be careful, dest MUST be a parent (since packets for ADDR_ANY_PARENT or BROADCAST are also returned indistinctly)
packet_info *buffer_queue_get_first_for_parent(queue_info *queue, uint16_t dest){
	int		i;
	
	//special case
	if (queue->size == 0)
		return(NULL);

	//searches for the first valid packet (either for dest or for ANY_PARENT or BROADCAST)
	for(i=0; i<queue->size; i++){
		
		if (
			(tools_is_addr_anycast(pk_get_dst_short(queue->elts[i]->packet))) || 
			(tools_is_addr_multicast(pk_get_dst_short(queue->elts[i]->packet))) || 
			(pk_get_dst_short(queue->elts[i]->packet) == dest)			
			){
			return(queue->elts[i]);
		}
	}
			
	//not found!
	return(NULL);	
}


//removes the first packet of the queue and returns it
packet_info *buffer_queue_pop(queue_info *queue){
	packet_info	*pk_res = queue->elts[0];
	int		i;
	
	//special case
	if (queue->size == 0)
		return(NULL);
	
	//shift all the other elements
	for(i=0; i<queue->size-1; i++){
		queue->elts[i] = queue->elts[i+1];
	}
	
	//reduce the size
	(queue->size)--;
	
	
	//memory reduction if required
	if (queue->size_reserved - queue->size > PK_INCR){
		queue->size_reserved -= PK_INCR;
		queue->elts = realloc(queue->elts, sizeof(packet_info*) * queue->size_reserved);
	}
	
	return(pk_res);
}


//----- INSERTION -------



//insert the packet sorted <priority, insertion_time>
void buffer_queue_insert_sorted(queue_info *queue, packet_info *pk_i){
	int		i;
	
	//memory extension if required
	if (queue->size + 1 > queue->size_reserved){
		queue->size_reserved += PK_INCR;
		queue->elts = realloc(queue->elts, sizeof(packet_info*) * queue->size_reserved);
	}
	
	
	//for all the packets, starting from the lowest prioritar packet
	for(i=queue->size-1; i>=0; i--)	{
		
		//this element has to be shifted toward the end of the queue (smallest priority)
		if ((queue->elts[i]->priority < pk_i->priority) 
			||
			((queue->elts[i]->priority == pk_i->priority) && (queue->elts[i]->time_insertion > pk_i->time_insertion))			 
			)
			queue->elts[i+1] = queue->elts[i];

		else{
			queue->elts[i+1] = pk_i;	
			(queue->size)++;
			return;

		}	
	}
	
	//this has to be inserted in the head
	queue->elts[0] = pk_i;
	(queue->size)++;
}



//insert a packet in one buffer
void buffer_insert_pk(call_t *c, queue_info *buf, packet_info *pk_i, uint64_t timeout){
	nodedata_info *nodedata = get_node_private_data(c);
    
 	//verification can only be done at the begining of one sf_slot
	if (timeout == TIMEOUT_DEFAULT)
        pk_i->time_expiration = 0 - 1 + ceil(pk_i->time_expiration / tools_get_sd_from_so(nodedata->sframe.SO)) * tools_get_sd_from_so(nodedata->sframe.SO);
	else if (timeout == TIMEOUT_INFINITY)
        pk_i->time_expiration = TIMEOUT_INFINITY;
    else
        pk_i->time_expiration = timeout;
        
	//insert the packet in the buffer
	buffer_queue_insert_sorted(buf, pk_i);
}


//create a structure to save the packet in the buffer
packet_info * buffer_pk_info_create(call_t *c, packet_t *packet){
	nodedata_info *nodedata = get_node_private_data(c);
	
	//info on the packet to save thereafter in the buffer
	packet_info	*pk_i = malloc(sizeof(packet_info));
	pk_i->buffered			= TRUE;
	pk_i->nb_retry			= 0;
	pk_i->packet			= packet;
	pk_i->time_insertion	= get_time();
	pk_i->priority			= PRIO_DATA;
	pk_i->time_expiration	= get_time() + macTransactionPersistenceTime * tools_get_bi_from_bo(nodedata->sframe.BO);
    pk_i->broadcasted       = FALSE;
	return(pk_i);
}





//----- DELETE -------

//delete the packet info at the position^th element
void buffer_queue_delete(queue_info *queue, int position){
	int		i;
	
	
	//memory
	packet_dealloc(queue->elts[position]->packet);
	free(queue->elts[position]);
	
	//shift all other elements
	for(i=position; i<queue->size-1; i++)
		queue->elts[i] = queue->elts[i+1];
	
	//reduce the size
	(queue->size) --;
	
	//memory reduction if required
	if (queue->size_reserved - queue->size > PK_INCR){
		queue->size_reserved -= PK_INCR;
		queue->elts = realloc(queue->elts, sizeof(packet_info*) * queue->size_reserved);
	}
}


//delete the packet info at the position^th element
void buffer_queue_delete_ptr(queue_info *queue, packet_info *pk_i){
	int		i;
	
	//if this element is not buffered, no interest to remove it from the queue!
	if (!(pk_i->buffered))
		return;
	
	//searches for the element
	i = 0;
	while(queue->elts[i] != pk_i)
		i++;
	
	
	//memory
	packet_dealloc(queue->elts[i]->packet);
	free(queue->elts[i]);
	
	//and starts to shifts following elements
	for(; i<queue->size-1; i++)
		queue->elts[i] = queue->elts[i+1];
	
	//reduce the size
	(queue->size) --;
	
	//memory reduction if required
	if (queue->size_reserved - queue->size > PK_INCR){
		queue->size_reserved -= PK_INCR;
		queue->elts = realloc(queue->elts, sizeof(packet_info*) * queue->size_reserved);
	}
}


//remove the pk_tx_up_pending (memory, etc.)
void buffer_dealloc_pk_tx_up_pending(call_t *c){
  	nodedata_info *nodedata = get_node_private_data(c);
  
    nodedata->mac.NB = 0;
    
    //oana: packets that are deleted from the buffer after being sent?
    if (nodedata->pk_tx_up_pending != NULL){
        if (nodedata->pk_tx_up_pending->buffered)
			buffer_queue_delete_ptr(nodedata->buffer_data_up, nodedata->pk_tx_up_pending);
        else{
            packet_dealloc(nodedata->pk_tx_up_pending->packet);				
            free(nodedata->pk_tx_up_pending);
        }

        buffer_change_pk_tx_up_pending(c, NULL);
    }		

}




//--------------------------------------------
//		INDIRECTIONS (debug purpose)
//--------------------------------------------

//change the current pointer for the next packet to be transmitted toward a child
void buffer_change_pk_tx_down_pending(call_t* c, packet_info *pk_i){
	nodedata_info *nodedata = get_node_private_data(c);
	nodedata->pk_tx_down_pending = pk_i;
}

//change the current pointer for the next packet to be transmitted toward a parent
void buffer_change_pk_tx_up_pending(call_t* c, packet_info *pk_i){
	nodedata_info *nodedata = get_node_private_data(c);
	nodedata->pk_tx_up_pending = pk_i;
}




//--------------------------------------------
//		TIMEOUTS in the BUFFER
//--------------------------------------------

int toolong = 0;

char *buffer_buf_num_to_str(int i){
	switch(i){
		case 0 :
			return("DATA-UP");
		case 1:
			return("DATA-DOWN");
		case 2:
			return("MGMT");
		default:
			return("");
	}
}
	

//remove the outdated packets
//NB: we have a "-1" for the outdated packets verification (we are sure to remove obsolete packets before the superframe begins)
void buffer_remove_outdated_packets(call_t *c){
	nodedata_info *nodedata = get_node_private_data(c);
	    
	//control
	uint64_t	oldest = 0;
    int         i;
	packet_info	*pk_i;
	//char		msg[150];
	int			pk_id, buf_id;
	
	//the buffers
	queue_info	**buf = malloc(sizeof(queue_info*) * (3 + ADDR_MULTI_NB));
	buf[0] = nodedata->buffer_data_up;
	buf[1] = nodedata->buffer_data_down;
	buf[2] = nodedata->buffer_mgmt;
	for(i=3; i<3+ADDR_MULTI_NB; i++)
        buf[i] = nodedata->buffer_multicast[i-3];
    
	//names for debug
	char		**buffer_name;
	buffer_name = malloc(sizeof(char*) * (3 + ADDR_MULTI_NB));
	for(buf_id=0; buf_id<3 + ADDR_MULTI_NB; buf_id++)
		buffer_name[buf_id] = malloc(sizeof(char) * 20);
	strncpy(buffer_name[0] , "DATA-UP",		20);
	strncpy(buffer_name[1] , "DATA-DOWN",	20);
	strncpy(buffer_name[2] , "MGMT",		20);
    for(i=3; i<3+ADDR_MULTI_NB; i++)
        snprintf(buffer_name[i], 20, "MULTI-%d", i-3);

   
	tools_debug_print(c, "[BUFFER] outdated packets verification\n");
	
	//remove outdated entries
	for(buf_id=0; buf_id<3 + ADDR_MULTI_NB ; buf_id++){
		buffer_content_print(c, buf[buf_id], buffer_name[buf_id]);
	
		
		//for each packet (from the end to the start since we may remove some of them)
		for(pk_id=buffer_queue_getsize(buf[buf_id])-1; pk_id>=0 ; pk_id--){
			pk_i = (packet_info*) buffer_queue_get(buf[buf_id], pk_id);
			
			
			//outdated (-1 since we schedule the deletion 1ns before the superframe slot begining)
			if (pk_i->time_expiration <= get_time() + 1){
				
				//it is not currently transmitted (directly the packet in the upload pending, the structure info on the packet for download pending)
				// DOWN & UP directions
				if ((nodedata->pk_tx_up_pending == NULL || nodedata->pk_tx_up_pending->packet != pk_i->packet) && (nodedata->pk_tx_down_pending == NULL || nodedata->pk_tx_down_pending->packet != pk_i->packet)){
					
					//stats
					stats_data_update_drop_reason(c, pk_i->packet, DROP_BUFFER_TIMEOUT);
					
					//debug
					tools_debug_print(c, "[BUFFER] packet is outdated, removed from the buffer (type %s, buffer %s)\n", pk_ftype_to_str(pk_get_ftype(pk_i->packet)), buffer_buf_num_to_str(buf_id));
					pk_print_content(c, pk_i->packet, stdout);
					
					
					//debug for data
					if (pk_get_ftype(pk_i->packet) == MAC_DATA_PACKET){
						cbrv3_packet_header		*header_cbr		= (cbrv3_packet_header *) (pk_i->packet->data + sizeof(_802_15_4_header) + sizeof(_802_15_4_DATA_header));
						_802_15_4_DATA_header	*header_data	= (_802_15_4_DATA_header *) (pk_i->packet->data + sizeof(_802_15_4_header));
						tools_debug_print(c, "[DATA] pk from %d and for %d is present for a too long time in by buffer, I drop it (cbrseq=%d)\n", header_data->src_end2end, header_data->dst_end2end, header_cbr->sequence); 
						if(!tools_is_addr_multicast(header_data->dst) && !tools_is_addr_multicast(header_data->src)){
							printf("[DATA] %d pk from %d and for %d is present for a too long time in by buffer, I drop it total = %d\n\n", c->node, header_data->src_end2end, header_data->dst_end2end, ++toolong);
                            data_drop_packet(c, pk_i->packet);
						}
					}
		
					//actual delete
					buffer_queue_delete(buf[buf_id], pk_id);
				}
				//I let additionnal time to finish the transmission
				else{
					pk_i->time_expiration += macResponseWaitTime;
					tools_debug_print(c, "[TIMEOUT] reincrement the currently transmitted packet timeout by macResponseWaitTime\n");
				}
			}
			//keep the oldest entry
			else if ((oldest ==0) || (pk_i->time_expiration <= oldest))
				oldest = pk_i->time_expiration;		
		}
	}	

	//memory
	for(buf_id=0; buf_id<3 + ADDR_MULTI_NB; buf_id++)
		free(buffer_name[buf_id]);
	free(buffer_name);
	free(buf);
}




//----------------------------------------------
//				PENDING FRAMES
//----------------------------------------------






//is the short address dest present in the mgmt or data buffers? (excepting the packet pk_except)
//NB: (pk_except==NULL) =>no exception)
uint8_t buffer_search_for_addr_short(call_t *c, uint16_t dest, packet_t *pk_except){
	nodedata_info *nodedata = get_node_private_data(c);

	queue_info *buf[2];
	buf[0] = nodedata->buffer_data_down;
	buf[1] = nodedata->buffer_mgmt;

	
	//control
	int		buf_id, pk_id;
	//one packet in the buffer
	packet_info	*pk_i;

	
	//we search for a pending packet for this destination in each buffer
	for(buf_id=0; buf_id<2; buf_id++){
	
		//for each packet of the buffer
		for(pk_id=0; pk_id < buffer_queue_getsize(buf[buf_id]); pk_id++){
			pk_i = buffer_queue_get(buf[buf_id], pk_id);
						
			switch(pk_get_ftype(pk_i->packet)){
					
				//DATA frame
				case MAC_DATA_PACKET:
					;
					_802_15_4_DATA_header	*header_data = (_802_15_4_DATA_header*) (pk_i->packet->data + sizeof(_802_15_4_header));
					if ((header_data->dst == dest) && (pk_i->packet != pk_except))
						return(TRUE);
		
				break;				
					
					
				//COMAND frame (in the mgmt buffer, surely)
				case MAC_COMMAND_PACKET:
					;
					
					_802_15_4_COMMAND_header	*header_mgmt = (_802_15_4_COMMAND_header*) (pk_i->packet->data + sizeof(_802_15_4_header));
					
					switch(header_mgmt->type_command){
						case ASSOCIATION_RESPONSE:
							;
							_802_15_4_ASSOCREP_header	*header_assoc_rep = (_802_15_4_ASSOCREP_header*) (pk_i->packet->data + sizeof(_802_15_4_header));
							if ((param_addr_long_to_short(header_assoc_rep->dst) == dest) && (pk_i->packet != pk_except))
								return(TRUE);

							
						break;
						default:
							tools_exit(2, c, "a bad command frame in the buffer (type %d) - buffer_search_for_addr_short\n", header_mgmt->type_command);
						break;
					}
					break;
					
			}
		}
	}
	return(FALSE);
}




//prints the content
void buffer_content_print(call_t *c, queue_info *buf, char *buffer_name){
	
	//control
	char	msg[500], msg2[500], tmp[150];
	int		pk_id;
	
	//one packet in the buffer
	packet_info	*pk_i;
	
	//debug
	strncpy(msg, "", 500);
	
		
	//for each packet of the buffer
	for(pk_id=0; pk_id < buffer_queue_getsize(buf); pk_id++){
		
		pk_i = buffer_queue_get(buf, pk_id);
		
		switch(pk_get_ftype(pk_i->packet)){
				
				
			//DATA frame
			case MAC_DATA_PACKET:
				;
				_802_15_4_DATA_header	*header_data = (_802_15_4_DATA_header*) (pk_i->packet->data + sizeof(_802_15_4_header));
				cbrv3_packet_header		*header_cbr	= (cbrv3_packet_header *) (pk_i->packet->data + sizeof(_802_15_4_header) + sizeof(_802_15_4_DATA_header));
				
				snprintf(msg2, 500, "%s, %u (data, cbrseq=%ld, to=%s)", msg, (unsigned int)header_data->dst, (long int)header_cbr->sequence, tools_time_to_str(pk_i->time_expiration, tmp, 150));
				snprintf(msg, 500, "%s", msg2);
				
				break;
				
				
				
			//COMAND frame (in the mgmt buffer, surely)
			case MAC_COMMAND_PACKET:
				;
				
				_802_15_4_COMMAND_header	*header_mgmt = (_802_15_4_COMMAND_header*) (pk_i->packet->data + sizeof(_802_15_4_header));
				
				switch(header_mgmt->type_command){
						//extract the destination field in the association reply (always a long address)
					case ASSOCIATION_RESPONSE:
						;
						_802_15_4_ASSOCREP_header	*header_assoc_rep = (_802_15_4_ASSOCREP_header*) (pk_i->packet->data + sizeof(_802_15_4_header));
						
						snprintf(msg2, 500, "%s %llu (mgmt, short addr %d, to %s)", msg, (unsigned long long int)header_assoc_rep->dst, param_addr_long_to_short(header_assoc_rep->dst), tools_time_to_str(pk_i->time_expiration, tmp, 150));
						snprintf(msg, 500, "%s", msg2);
						
						
						break;
					default:
						pk_print_content(c, pk_i->packet, stdout);
						tools_exit(2, c, "a bad command frame in the buffer (type %d) - buffer_content_print\n", header_mgmt->type_command);

						break;
				}
				
				break;
				
			//HELLO PACKET
			case MAC_HELLO_PACKET:
				
				snprintf(msg2, 500, "hello");
				snprintf(msg, 500, "%s", msg2);
				break;
		
			default:
				
				pk_print_content(c, pk_i->packet, stdout);
				tools_exit(2, c, "this frame type has not to be present in the buffer (type %d) - buffer_content_print\n", pk_get_ftype(pk_i->packet));

				
				break;
		}
	}
	if (buffer_queue_getsize(buf) > 0)
        tools_debug_print(c, "[BUFFER] buffer %s contains %d packets: %s\n", buffer_name, buffer_queue_getsize(buf), msg);
}




//does a data frame exist in the buffer for dst?
packet_info	*buffer_get_first_pending_frame_for_addr_short(call_t *c, queue_info *buffer, uint16_t dst, char* buffer_name){
	packet_info	*pk_i;	
	int		pk_id;
	
	//debug
//	tools_debug_print(c, "searching for %d (long addr %llu) in %s\n", dst, (unsigned long int)param_addr_short_to_long(dst), buffer_name);

	//we search for a pending packet for this destination
	for(pk_id=0; pk_id < buffer_queue_getsize(buffer); pk_id++){
		pk_i = buffer_queue_get(buffer, pk_id);
				
		switch(pk_get_ftype(pk_i->packet)){
			
			case MAC_DATA_PACKET:
				;
				_802_15_4_DATA_header	*header_data = (_802_15_4_DATA_header*) (pk_i->packet->data + sizeof(_802_15_4_header));
		
				if (header_data->dst == dst)
					return(pk_i);
			break;
			//COMAND frame (in the mgmt buffer, surely)
	
			case MAC_COMMAND_PACKET:
				;
			
				_802_15_4_COMMAND_header	*header_mgmt = (_802_15_4_COMMAND_header*) (pk_i->packet->data + sizeof(_802_15_4_header));
			
				switch(header_mgmt->type_command){
					
					//extract the destination field in the association reply (always a long address)
					case ASSOCIATION_RESPONSE:
						;
						_802_15_4_ASSOCREP_header	*header_assoc_rep = (_802_15_4_ASSOCREP_header*) (pk_i->packet->data + sizeof(_802_15_4_header));
						
						if (param_addr_long_to_short(header_assoc_rep->dst) == dst)		
							return(pk_i);	
	
						
						break;
					default:
						tools_exit(2, c, "a bad command frame in the buffer (type %d) - buffer_get_first_pending_frame_for_addr_short\n", header_mgmt->type_command);

						break;
				}
		}					
		
	}
	//no packet was found for dst!
	return(NULL);
}



//-------------------------------------------
//					PENDING DESTS
//-------------------------------------------
//NB: down direction for coordinators


//converts a list of pending destinations into a string (for a debug purpose)
char *buffer_dests_short_to_str(short *dests, char *msg, int length){
	int		i;
	char	tmp[50];
	msg[0] = '\0';
	
	//copy each destination
	for(i=0; i<get_node_count(); i++)
		if (dests[i]){
			snprintf(tmp, 50, "%d ", i);
			strncat(msg, tmp, length);
		}
	return(msg);
}
//converts a list of pending destinations into a string (for a debug purpose)
char *buffer_dests_long_to_str(short *dests, char *msg, int length){
	int		i;
	char	tmp[50];
	msg[0] = '\0';
	
	//copy each destination
	for(i=0; i<get_node_count(); i++)
		if (dests[i]){
			snprintf(tmp, 50, "%llu (%d) ", (long long int)param_addr_short_to_long(i), i);
			strncat(msg, tmp, length);
		}
	return(msg);
}

//count the number of destinations in a list
int buffer_count_nb_dests(short *dests){
	int	nb_dests = 0;
	int	i;
	
	for(i=0; i<get_node_count(); i++)
		if (dests[i])
			nb_dests ++;
	return(nb_dests);
}


//constructs the list of pending destinations (included in the beacon) for short and long addresses (arrays of s: present or not)
void buffer_get_list_pending_dests(call_t *c, short *dests_short, short *dests_long){
	nodedata_info *nodedata = (nodedata_info*) get_node_private_data(c);
	int		buf_id, pk_id;
	char	buffer_name[2][20];
	
	
	//initialization
	int ii;
	for(ii=0; ii<get_node_count(); ii++){
		dests_short[ii] = FALSE;	
		dests_long[ii] = FALSE;	
	}
	
	//buffers
	queue_info *buf[2];
	buf[0] = nodedata->buffer_data_down;
	buf[1] = nodedata->buffer_mgmt;
	snprintf(buffer_name[0], 20, "DATA-DOWN");
	snprintf(buffer_name[1], 20, "MGMT");

	//one packet in the buffers
	packet_info	*pk_i;	
	
	//we search for the pending destinations
	for(buf_id=0; buf_id<2; buf_id++){
		
		//debug
		buffer_content_print(c, buf[buf_id], buffer_name[buf_id]);

		//for each packet of the buffer
		for(pk_id=0; pk_id < buffer_queue_getsize(buf[buf_id]); pk_id++){
			pk_i = buffer_queue_get(buf[buf_id], pk_id);
			
			switch(pk_get_ftype(pk_i->packet)){
					
				case MAC_DATA_PACKET:
					;
					_802_15_4_DATA_header	*header_data = (_802_15_4_DATA_header*) (pk_i->packet->data + sizeof(_802_15_4_header));
					
					if (!dests_short[header_data->dst]){
						dests_short[header_data->dst] = TRUE;
						//printf("%d included\n", data_hdr->dst);
					}
					
					break;
					
					
					//COMAND frame (in the mgmt buffer, surely)				
				case MAC_COMMAND_PACKET:
					;
					
					_802_15_4_COMMAND_header	*header_mgmt = (_802_15_4_COMMAND_header*) (pk_i->packet->data + sizeof(_802_15_4_header));
					
					switch(header_mgmt->type_command){
							
							//extract the destination field in the association reply (always a long address)
						case ASSOCIATION_RESPONSE:
							;
							_802_15_4_ASSOCREP_header	*header_assoc_rep = (_802_15_4_ASSOCREP_header*) (pk_i->packet->data + sizeof(_802_15_4_header));
							
							if (!dests_long[param_addr_long_to_short(header_assoc_rep->dst)]){
								dests_long[param_addr_long_to_short(header_assoc_rep->dst)] = TRUE;
								//printf("%d included\n", data_hdr->dst);
							}
							
							break;
							
							
						default:
							tools_exit(2, c, "a bad command frame in the buffer (type %d) - buffer_get_list_pending_dests\n", header_mgmt->type_command);

							break;
					}
			}					
			
		}
	}

}














