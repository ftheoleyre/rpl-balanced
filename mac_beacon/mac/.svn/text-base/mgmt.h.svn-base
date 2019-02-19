/*
 *  mgmt.h
 *  
 *
 *  Created by Fabrice Theoleyre on 11/02/11.
 *  Copyright 2011 CNRS. All rights reserved.
 *
 */



#ifndef __MGMT_H__
#define __MGMT_H__


#include "802154_slotted.h"





//--------------- COMMANDS RX ------------------

//One command frame was received
void mgmt_cmd_rx(call_t *c, packet_t *packet);




//--------------- ASSOCIATION ------------------

//received an association request/response
void mgmt_assoc_req_rx(call_t *c, packet_t *cmd_pk);
void mgmt_assoc_rep_rx(call_t *c, packet_t *cmd_pk);

//this node is now associated
void mgmt_assoc_validated(call_t *c, packet_t *cmd_pk);

//creates an association request
packet_info* mgmt_assoc_req_create(call_t *c, uint16_t dest);

//creates an association reply
void mgmt_assoc_rep_create_and_insert_in_down_buffer(call_t *c, uint64_t dest);



//--------------- DISASSOCIATION ------------------

//this parent is not anymore associated (I am in its superframe)
void mgmt_parent_disassociated(call_t *c, uint16_t addr_short);

//creates a disassociation notification
packet_info *mgmt_disassoc_notif_create(call_t *c, uint16_t dest);




//------------- DATA REQUEST -----------

//we received one DATA_REQUEST -> searches a data packet to send
void mgmt_data_req_rx(call_t *c, packet_t *cmd_pk);

//create a data request management frame to retrieve our packet
packet_info *mgmt_data_request_create(call_t *c, uint16_t dest);

//create a pull management frame to retrieve our packet (assoc-rep)
packet_info *mgmt_data_request_long_src_create(call_t *c, uint16_t dest);




//--------------- ACKS ------------------

//returns the size of an ack
int mgmt_ack_get_size();
//I have to transmit one ack  (return the packet size)
int mgmt_ack_tx(call_t *c, uint8_t seq_num, short pending_frame);
//I received one ack
void mgmt_ack_rx(call_t *c, packet_t *packet);

    



//--------------- MULTICAST_REQUEST ------------------

//create a request packet to receive a multicast packet
packet_info *mgmt_multicast_req_pk_create(call_t *c, uint16_t dest);

//I received a multicast request
uint64_t mgmt_multicast_req_rx(call_t *c, packet_t *pk);




//--------------- CBRSEQ MGMT ------------------

//is this packet already received?
short mgmt_cbrseq_rx(call_t *c, uint16_t src, uint64_t cbrseq);

//this packet is now received
void mgmt_cbrseq_add(call_t *c,  uint16_t src, uint64_t cbrseq);

    
    


//--------------- SEQNUM MGMT ------------------

//compute the list of multicast addresses and seqnums which have to be piggybacked in beacons
void mgmt_seqnum_compute_list_for_beacons(call_t *c, seqnum_beacon_info **list);


//assign a new sequence number for this destination (each unicast/multicast destination implies a different sequence number list)
uint8_t mgmt_assign_seqnum(call_t *c, uint16_t dest);

//remove the timeouted multicast seqnums in my tx and rx buffers
int mgmt_seqnum_remove_timeout(call_t *c, void *arg);

//this seqnum multicast packet was received
void mgmt_seqnum_rcvd(call_t *c, packet_t *packet);

//does a multicast packet be retrieved from addr?
short mgmt_seqnum_is_multicast_pending_for(call_t *c, uint16_t addr_short);

//updates the list of enqueued seqnums for this node 
void mgmt_seqnum_update_from_beacon(call_t *c, packet_t *beacon_pk);


//compute the list of multicast addresses and seqnums which have to be piggybacked in beacons
void mgmt_seqnum_compute_list_for_multicastreq(call_t *c, uint16_t dest, seqnum_beacon_info **list);


#endif

