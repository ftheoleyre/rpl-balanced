#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <limits.h>
#include <string.h>
#include <include/modelutils.h>
#include "../radio/radio_generic.h"
#include "../rpl/stats.h"
#include "../rpl/rpl.h"

//node types 
#define SENSOR 1
#define SINK   0


//time multiples
#define uSecond		       1000 //1us
#define mSecond         1000000 //1ms
#define Second       1000000000 //1s


//data packet
#define RPL_DATA_PACKET 25


//from MAC with BEACONs
#define ADDR_MULTI_CTREE                    (0xfffc)    //65532 au lieu de BROADCAST_ADDR


// RPL Packet header
typedef struct rpl_packet_header {
	uint8_t type_code;
	uint32_t pkt_sequence;
	
	nodeid_t source;
	nodeid_t next;
	
	void* data;	//DATA - storing space for DIS or DIO message pointer
}rpl_packet_header_t;



//Function declarations
int	 set_header (call_t *c, packet_t *packet, destination_t *dst);
void tx (call_t *c, packet_t *packet);

