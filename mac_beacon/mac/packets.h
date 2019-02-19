
/**
 *  \file   packets.h
 *  \brief  packet's management
 *  \author Fabrice Theoleyre
 *  \date   2011
 **/




#ifndef __PACKETS_H__
#define	__PACKETS_H__


#include "802154_slotted.h"
#include "packets_format.h"




//---------------  TOOLS ------------------

short pk_data_has_zero_payload(packet_t *pk);

//returns the duration of a BOP slot
uint64_t pk_get_bop_slot_duration(call_t *c);


//--------------- BINARY ------------------

//bits manipulation
uint8_t pk_uint8_t_get_bit(uint8_t byte, int pos);
void pk_uint8_t_set_bit(uint8_t *byte, uint8_t value, int pos);

//string conversion for binary values
char *pk_byte_to_str(uint8_t byte, char *buf, int length);

//bits manipulation
uint8_t pk_uint8_t_get_bits(uint8_t byte, int start, int end);
void pk_uint8_t_set_bits(uint8_t *byte, uint8_t value, int start, int end);
	



//--------------- FIELDS / COMMON ------------------

//PAN ID compression
uint8_t pk_get_C(packet_t *pk);
void pk_set_C(packet_t *pk, uint8_t value);

//ack request
uint8_t pk_get_A(packet_t *pk);
void pk_set_A(packet_t *pk, uint8_t value);

//frame pending
uint8_t pk_get_P(packet_t *pk);
void pk_set_P(packet_t *pk, uint8_t value);

//security
uint8_t pk_get_S(packet_t *pk);
void pk_set_S(packet_t *pk, uint8_t value);

//frame type
uint8_t pk_get_ftype(packet_t *pk);
void pk_set_ftype(packet_t *pk, uint8_t value);

//subtype (commmand frames)
uint8_t pk_get_subtype(packet_t *pk);

//SAM
uint8_t pk_get_SAM(packet_t *pk);
void pk_set_SAM(packet_t *pk, uint8_t value);
	

//FV
uint8_t pk_get_FV(packet_t *pk);
void pk_set_FV(packet_t *pk, uint8_t value);
		
//DAM
uint8_t pk_get_DAM(packet_t *pk);
void pk_set_DAM(packet_t *pk, uint8_t value);
	
//SEQNUM
uint8_t pk_get_seqnum(packet_t *pk);
void pk_set_seqnum(packet_t *pk, uint8_t value);





//--------------- FIELDS / BEACON ------------------


//BO
uint8_t pk_beacon_get_BO(packet_t *pk);
void pk_beacon_set_BO(packet_t *pk, uint8_t value);

//SO
uint8_t pk_beacon_get_SO(packet_t *pk);
void pk_beacon_set_SO(packet_t *pk, uint8_t value);

//PAN coordinator
uint8_t pk_beacon_get_PANc(packet_t *pk);
void pk_beacon_set_PANc(packet_t *pk, uint8_t value);

//association permit
uint8_t pk_beacon_get_assoc_permit(packet_t *pk);
void pk_beacon_set_assoc_permit(packet_t *pk, uint8_t value);

//nb of short addresses
uint8_t pk_beacon_get_nb_pending_short_addr(packet_t *pk);
void pk_beacon_set_nb_pending_short_addr(packet_t *pk, uint8_t value);

//nb of long addresses
uint8_t pk_beacon_get_nb_pending_long_addr(packet_t *pk);
void pk_beacon_set_nb_pending_long_addr(packet_t *pk, uint8_t value);

// depth (converts the double into the depth field)
double pk_beacon_get_depth(packet_t *pk);
void pk_beacon_set_depth(packet_t *pk, double value);

//sf slot
uint8_t pk_beacon_get_sf_slot(packet_t *pk);
void pk_beacon_set_sf_slot(packet_t *pk, uint8_t value);

//bop slot
uint8_t pk_beacon_get_bop_slot(packet_t *pk);
void pk_beacon_set_bop_slot(packet_t *pk, uint8_t value);

//has at least one child
short pk_beacon_get_has_a_child(packet_t *pk);
void pk_beacon_set_has_a_child(packet_t *pk, short value);


//--------------- SEQNUM MULTICAST in BEACONS ------------------

//the number of pending multicast addresses
uint8_t pk_beacon_get_nb_pending_multicast(packet_t *pk);
void pk_beacon_set_nb_pending_multicast(packet_t *pk, uint8_t value);

//get the min seqnum for the line^th muticast addr
uint8_t pk_beacon_get_seqnum_min(packet_t *pk, int i);
void pk_beacon_set_seqnum_min(packet_t *pk, int i, uint8_t value);

//get the max seqnum for the line^th muticast addr
uint8_t pk_beacon_get_seqnum_max(packet_t *pk, int i);
void pk_beacon_set_seqnum_max(packet_t *pk, int i, uint8_t value);

//multicast address for the line^th address
uint8_t pk_beacon_get_seqnum_caddr(packet_t *pk, int i);
void pk_beacon_set_seqnum_caddr(packet_t *pk, int i, uint8_t addr);   


//--------------- SEQNUM MULTICAST in MULTIREQ ------------------

//the number of pending multicast addresses
uint8_t pk_multireq_get_nb_pending_multicast(packet_t *pk);
void pk_multireq_set_nb_pending_multicast(packet_t *pk, uint8_t value);

//get the min seqnum for the line^th muticast addr
uint8_t pk_multireq_get_seqnum_min(packet_t *pk, int i);
void pk_multireq_set_seqnum_min(packet_t *pk, int i, uint8_t value);

//get the max seqnum for the line^th muticast addr
uint8_t pk_multireq_get_seqnum_max(packet_t *pk, int i);
void pk_multireq_set_seqnum_max(packet_t *pk, int i, uint8_t value);

//multicast address for the line^th address
uint8_t pk_multireq_get_seqnum_caddr(packet_t *pk, int i);
void pk_multireq_set_seqnum_caddr(packet_t *pk, int i, uint8_t addr);   



//--------------- FIELDS / HELLOS------------------

//nb neighs
uint8_t pk_hello_get_nb_neighs(packet_t *pk);
void pk_hello_set_nb_neighs(packet_t *pk, uint8_t value);

//addr
uint16_t pk_hello_get_addr(packet_t *pk, int neigh);
void pk_hello_set_addr(packet_t *pk, int neigh, uint16_t value);
	
//superframe slot
uint8_t pk_hello_get_sf_slot(packet_t *pk, int neigh);
void pk_hello_set_sf_slot(packet_t *pk, int neigh, uint8_t value);
	
//BOP slot
uint8_t pk_hello_get_bop_slot(packet_t *pk, int neigh);
void pk_hello_set_bop_slot(packet_t *pk, int neigh, uint8_t value);

//has child
uint8_t pk_hello_get_has_a_child(packet_t *pk, int neigh);
void pk_hello_set_has_a_child(packet_t *pk, int neigh, uint8_t value);

//hops
uint8_t pk_hello_get_hops(packet_t *pk, int neigh);
void pk_hello_set_hops(packet_t *pk, int neigh, uint8_t value);




//--------------- ADDRs ------------------

//extracts the short address of one packet (converting a long into a short address if required))
uint16_t pk_get_src_short(packet_t *pk);
uint16_t pk_get_dst_short(packet_t *pk);





	
//--------------- DEBUG-PACKET ------------------

//return the CBR sequence number if it is a data packet (else, returns -1)
uint64_t pk_get_cbrseq(packet_t *pk);

//prints the headers
void pk_print_content(call_t *c, packet_t *packet, FILE *pfile);

//returns the humand readable packet type
char *pk_ftype_to_str(int ftype);






#endif

