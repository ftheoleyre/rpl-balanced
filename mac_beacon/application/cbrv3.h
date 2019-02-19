
/**
 *  \file   cbrv3.h
 *  \brief  packet's format for CBR packets and other material
 *  \author Fabrice Theoleyre
 *  \date   2011
 **/




#ifndef __CBRV3_H__
#define	__CBRV3_H__

#include "../mac/const.h"



//this means we don't generate this type of packets
#define PERIOD_INFINITE         0

//any neighbor
//nb: assumes we have a 16 bits address
//#define CBR_MULTICAST           0xffff


//The PAN coordinator
#define CBR_PAN_COORD_ID        0


#define CBR_START				0


//------------------------------------
//		CBR INTERFACE
//------------------------------------

uint64_t cbr_get_inter_pk_time();

//CBR packet
typedef struct{
    int			source;
    uint64_t	sequence;
} cbrv3_packet_header;


//private info
struct _cbr_private {
    uint64_t    start;
    position_t  position;
    
    //unicast
    uint64_t    unicast_period;
	int         unicast_packet_size;		//in bits
    void*       unicast_timer;
    
    //multicast
    uint64_t    multicast_period;
	int         multicast_packet_size;		//in bits
    void*       multicast_timer;
    uint16_t    multicast_dest;
    
    //common
    int         overhead;
};




#endif

