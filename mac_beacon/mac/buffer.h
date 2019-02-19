/*
 *  buffer.h
 *  
 *
 *  Created by Fabrice Theoleyre on 06/04/11.
 *  Copyright 2011 CNRS. All rights reserved.
 *
 */



#ifndef __BUFFER__
#define __BUFFER__


#include "802154_slotted.h"



//------------- QUEUES -----------

//----- MGMT

//create an empty queue
queue_info *buffer_queue_create();

//removes all the elements
void buffer_queue_empty(queue_info *queue);

//returns the size
int buffer_queue_getsize(queue_info *queue);

//destroy a queue
void buffer_queue_destroy(queue_info *queue);


//----- GET

//returns the position^th element
packet_info	*buffer_queue_get(queue_info *queue, int position);

//returns the element with seqnum
packet_info	*buffer_queue_get_seqnum(queue_info *queue, uint16_t caddr, uint8_t seqnum);


//returns a pointer to the first element
//Be careful, dest MUST be a parent (else it will get also all the packets for ADDR_ANY_PARENT!)
packet_info *buffer_queue_get_first_for_parent(queue_info *queue, uint16_t dest);
	

//----- INSERT

//insert the packet sorted <priority, insertion_time>
void buffer_queue_insert_sorted(queue_info *queue, packet_info *pk_i);

//insert a packet in one buffer
void buffer_insert_pk(call_t *c, queue_info *buf, packet_info *pk_i, uint64_t timeout);

//create a structure to save the packet in the buffer
packet_info * buffer_pk_info_create(call_t *c, packet_t *packet);




//----- REMOVE

//delete the packet info at the position^th element
void buffer_queue_delete(queue_info *queue, int position);

//delete the packet info at the position^th element
void buffer_queue_delete_ptr(queue_info *queue, packet_info *pk_i);

//remove the pk_tx_up_pending (memory, etc.)
void buffer_dealloc_pk_tx_up_pending(call_t *c);




//------------- INDIRECTIONS -----------

//change the current pointer for the next packet to be transmitted toward a child
void buffer_change_pk_tx_down_pending(call_t* c, packet_info *pk_i);
void buffer_change_pk_tx_up_pending(call_t* c, packet_info *pk_i);	





//------------- OUTDATED packets -----------

//remove the outdated packets
void buffer_remove_outdated_packets(call_t *c);






//------------- PENDING FRAMES -----------

//is the short address dest present in the mgmt or data buffers? (excepting the packet pk_except)
uint8_t buffer_search_for_addr_short(call_t *c, uint16_t dest, packet_t *pk_except);

//prints the content
void buffer_content_print(call_t *c, queue_info *buf, char *buffer_name);

//does a  frame exist in the buffer for dst?
packet_info	*buffer_get_first_pending_frame_for_addr_short(call_t *c, queue_info *buffer, uint16_t dst, char* buffer_name);




//------------- PENDING DESTS for COORD (down direction) -----------

//converts a list of pending destinations into a string (for a debug purpose)
char *buffer_dests_short_to_str(short *dests, char *msg, int length);
char *buffer_dests_long_to_str(short *dests, char *msg, int length);

//count the number of destinations in a list
int buffer_count_nb_dests(short *dests);

//constructs the list of pending destinations (included in the beacon) for short and long addresses (arrays of s: present or not)
void buffer_get_list_pending_dests(call_t *c,  short *dests_short, short *dest_long);





#endif

