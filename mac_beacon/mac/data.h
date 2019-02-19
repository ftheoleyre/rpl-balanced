/*
 *  data.h
 *  
 *
 *  Created by Fabrice Theoleyre on 11/02/11.
 *  Copyright 2011 CNRS. All rights reserved.
 *
 */



#ifndef __DATA__
#define __DATA__


#include "802154_slotted.h"



//------------- ROUTING -----------


//forwards either to the higher layer or put it in the correct queue
void data_route(call_t *c, packet_t *packet_rcvd);



//------------- TX / RX -----------

// when I receive one data packet and I must send an ACK
void data_rx(call_t *c, packet_t *packet);



//--------------- NULL DATA PACKETS ------------------

//create a data packet with a zero payload (reply to a data-req when we have no data to transmit)
packet_info *data_create_pk_with_zero_payload(call_t *c, uint16_t dest);


//oana - nr of packets dropped per parent - for ELT-ETX
void data_drop_packet(call_t *c, packet_t *pkt_drop);


#endif
