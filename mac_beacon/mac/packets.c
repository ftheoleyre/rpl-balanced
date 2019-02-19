/**
 *  \file   packets.c
 *  \brief  packet management and subfields get/set
 *  \author Fabrice Theoleyre
 *  \date   2011
 **/


#include "packets.h"



//----------------------------------------------
//				TOOLS
//----------------------------------------------

//returns 1 if this packet has a zero payload length (only the common + data heders and nothing more)
short pk_data_has_zero_payload(packet_t *pk){
	
	return(pk->real_size == 8 * (sizeof(_802_15_4_header) + sizeof(_802_15_4_DATA_header)));
}



//returns the duration of a BOP slot
uint64_t pk_get_bop_slot_duration(call_t *c){	
	call_t		c_radio = {get_entity_bindings_down(c)->elts[0], c->node, c->entity};
	int			pk_length;
	uint64_t	duration;
	
	//Worst case: all the pending addresses are long addresses
	pk_length = sizeof(_802_15_4_header) + sizeof(_802_15_4_Beacon_header) + BEACON_MAX_NB_ADDR * sizeof(uint64_t) + \
		BEACON_MAX_NB_NEIGH * (sizeof(uint16_t) + sizeof(uint8_t)) + (int)floor(BEACON_MAX_NB_NEIGH*0.5) * sizeof(uint8_t);	
	duration = pk_length * 8 * radio_get_Tb(&c_radio);	
	//duration += mactACK;
     
	return(duration);
}





//----------------------------------------------
//				BINARY
//----------------------------------------------


//bits manipulation
uint8_t pk_uint8_t_get_bit(uint8_t byte, int pos){
	
	return((byte & (1<<(pos)))>>(pos)); 
}
void pk_uint8_t_set_bit(uint8_t *byte, uint8_t value, int pos){
	
	if (value)
		*byte = *byte | (1<<(pos)) ;
	else
		*byte = *byte & (~(1<<(pos))) ;
}


//returns the binary form of the byte
char *pk_byte_to_str(uint8_t byte, char *buf, int length){
	int	i;
	buf[0] = '\0';
	
	for(i=0; i<8; i++)
		snprintf(buf, length, "%s %d", buf, pk_uint8_t_get_bit(byte, i));
	
	return(buf);	
}


//bits manipulation
uint8_t pk_uint8_t_get_bits(uint8_t byte, int start, int end){
	int			i;
	uint8_t		value = 0;

	//constructs the byte value (rightmost bit is the least significant bit)
	for(i=start; i<=end; i++)
		pk_uint8_t_set_bit(&value, pk_uint8_t_get_bit(byte, i), end-i);

	return(value);	
}
void pk_uint8_t_set_bits(uint8_t *byte, uint8_t value, int start, int end){
	int		i;

	for(i=start; i<=end; i++)
		pk_uint8_t_set_bit(byte, pk_uint8_t_get_bit(value, end-i), i);
}


//----------------------------------------------
//		PACKET'S FIELDS: COMMON HEADERS
//----------------------------------------------

//frame type
uint8_t pk_get_ftype(packet_t *pk){
	_802_15_4_header *hdr = (_802_15_4_header *) pk->data;
	
	return(pk_uint8_t_get_bits(hdr->byte2, 5, 7));
}
//we compute our own values to not care about little/big endians
void pk_set_ftype(packet_t *pk, uint8_t value){
	_802_15_4_header *hdr = (_802_15_4_header *) pk->data;
	
	pk_uint8_t_set_bits(&(hdr->byte2), value, 5, 7);
}

//return the subtype for command frame
uint8_t pk_get_subtype(packet_t *pk){
    if (pk_get_ftype(pk) !=  MAC_COMMAND_PACKET)
        return(INVALID_UINT8_T);
    
	_802_15_4_COMMAND_header	*header_cmd	= (_802_15_4_COMMAND_header *)  (pk->data + sizeof(_802_15_4_header));
    return(header_cmd->type_command);
}


//PAN ID compression
uint8_t pk_get_C(packet_t *pk){
	_802_15_4_header *hdr = (_802_15_4_header *) pk->data;
	
	return(pk_uint8_t_get_bit(hdr->byte2, 1));
}
void pk_set_C(packet_t *pk, uint8_t value){
	_802_15_4_header *hdr = (_802_15_4_header *) pk->data;
	
	pk_uint8_t_set_bit(&(hdr->byte2), value, 1);
}

//ack request
uint8_t pk_get_A(packet_t *pk){
	_802_15_4_header *hdr = (_802_15_4_header *) pk->data;
	
	return(pk_uint8_t_get_bit(hdr->byte2, 2));
}
void pk_set_A(packet_t *pk, uint8_t value){
	_802_15_4_header *hdr = (_802_15_4_header *) pk->data;
	
	pk_uint8_t_set_bit(&(hdr->byte2), value, 2);
}

//frame pending
uint8_t pk_get_P(packet_t *pk){
	_802_15_4_header *hdr = (_802_15_4_header *) pk->data;
	
	return(pk_uint8_t_get_bit(hdr->byte2, 3));
}
void pk_set_P(packet_t *pk, uint8_t value){
	_802_15_4_header *hdr = (_802_15_4_header *) pk->data;
	
	pk_uint8_t_set_bit(&(hdr->byte2), value, 3);
}

//security
uint8_t pk_get_S(packet_t *pk){
	_802_15_4_header *hdr = (_802_15_4_header *) pk->data;
	
	return(pk_uint8_t_get_bit(hdr->byte2, 4));
}
void pk_set_S(packet_t *pk, uint8_t value){
	_802_15_4_header *hdr = (_802_15_4_header *) pk->data;
	
	pk_uint8_t_set_bit(&(hdr->byte2), value, 4);
}

//SAM
uint8_t pk_get_SAM(packet_t *pk){
	_802_15_4_header *hdr = (_802_15_4_header *) pk->data;
	
	return(pk_uint8_t_get_bits(hdr->byte3, 0, 1));
}
void pk_set_SAM(packet_t *pk, uint8_t value){
	_802_15_4_header *hdr = (_802_15_4_header *) pk->data;
	
	pk_uint8_t_set_bits(&(hdr->byte3), value, 0, 1);
}

//FV
uint8_t pk_get_FV(packet_t *pk){
	_802_15_4_header *hdr = (_802_15_4_header *) pk->data;
	
	return(pk_uint8_t_get_bits(hdr->byte3, 2, 3));
}
void pk_set_FV(packet_t *pk, uint8_t value){
	_802_15_4_header *hdr = (_802_15_4_header *) pk->data;
	
	pk_uint8_t_set_bits(&(hdr->byte3), value, 2, 3);
}

//DAM
uint8_t pk_get_DAM(packet_t *pk){
	_802_15_4_header *hdr = (_802_15_4_header *) pk->data;
	
	return(pk_uint8_t_get_bits(hdr->byte3, 4, 5));
}
void pk_set_DAM(packet_t *pk, uint8_t value){
	_802_15_4_header *hdr = (_802_15_4_header *) pk->data;
	
	pk_uint8_t_set_bits(&(hdr->byte3), value, 4, 5);
}

//SEQ_NUM
uint8_t pk_get_seqnum(packet_t *pk){
	_802_15_4_header *hdr = (_802_15_4_header *) pk->data;
	
	return(hdr->seq_num);
}
void pk_set_seqnum(packet_t *pk, uint8_t value){
	_802_15_4_header *hdr = (_802_15_4_header *) pk->data;
	
	hdr->seq_num = value;
}



//----------------------------------------------
//		PACKET'S FIELDS: BEACON HEADERS
//----------------------------------------------



//BO
uint8_t pk_beacon_get_BO(packet_t *pk){
	_802_15_4_Beacon_header *hdr = (_802_15_4_Beacon_header *) (pk->data + sizeof(_802_15_4_header));
	
	return(pk_uint8_t_get_bits(hdr->byte1, 0, 3));
	
}
void pk_beacon_set_BO(packet_t *pk, uint8_t value){
	_802_15_4_Beacon_header *hdr = (_802_15_4_Beacon_header *) (pk->data + sizeof(_802_15_4_header));
	
	pk_uint8_t_set_bits(&(hdr->byte1), value, 0, 3);
}
//SO
uint8_t pk_beacon_get_SO(packet_t *pk){
	_802_15_4_Beacon_header *hdr = (_802_15_4_Beacon_header *) (pk->data + sizeof(_802_15_4_header));
	
	return(pk_uint8_t_get_bits(hdr->byte1, 4, 7));
}
void pk_beacon_set_SO(packet_t *pk, uint8_t value){
	_802_15_4_Beacon_header *hdr = (_802_15_4_Beacon_header *) (pk->data + sizeof(_802_15_4_header));
	
	pk_uint8_t_set_bits(&(hdr->byte1), value, 4, 7);
}
//PAN coordinator
uint8_t pk_beacon_get_PANc(packet_t *pk){
	_802_15_4_Beacon_header *hdr = (_802_15_4_Beacon_header *) (pk->data + sizeof(_802_15_4_header));
	
	return(pk_uint8_t_get_bit(hdr->byte2, 6));
}
void pk_beacon_set_PANc(packet_t *pk, uint8_t value){
	_802_15_4_Beacon_header *hdr = (_802_15_4_Beacon_header *) (pk->data + sizeof(_802_15_4_header));
	
	pk_uint8_t_set_bit(&(hdr->byte2), value, 6);
}
//association permit
uint8_t pk_beacon_get_assoc_permit(packet_t *pk){
	_802_15_4_Beacon_header *hdr = (_802_15_4_Beacon_header *) (pk->data + sizeof(_802_15_4_header));
	
	return(pk_uint8_t_get_bit(hdr->byte2, 7));
}
void pk_beacon_set_assoc_permit(packet_t *pk, uint8_t value){
	_802_15_4_Beacon_header *hdr = (_802_15_4_Beacon_header *) (pk->data + sizeof(_802_15_4_header));
	
	pk_uint8_t_set_bit(&(hdr->byte2), value, 7);
}
//nb of short addresses
uint8_t pk_beacon_get_nb_pending_short_addr(packet_t *pk){
	_802_15_4_Beacon_header *hdr = (_802_15_4_Beacon_header *) (pk->data + sizeof(_802_15_4_header));
	
	return(pk_uint8_t_get_bits(hdr->byte3, 0, 2));
}
void pk_beacon_set_nb_pending_short_addr(packet_t *pk, uint8_t value){
	_802_15_4_Beacon_header *hdr = (_802_15_4_Beacon_header *) (pk->data + sizeof(_802_15_4_header));
	
	pk_uint8_t_set_bits(&(hdr->byte3), value, 0, 2);
	
	//overflow verification
	if (pk_beacon_get_nb_pending_long_addr(pk) + pk_beacon_get_nb_pending_short_addr(pk) > BEACON_MAX_NB_ADDR)
		tools_exit_short(4, "you have too many long addresses for this field\n");

}
//nb of long addresses
uint8_t pk_beacon_get_nb_pending_long_addr(packet_t *pk){
	_802_15_4_Beacon_header *hdr = (_802_15_4_Beacon_header *) (pk->data + sizeof(_802_15_4_header));
	
	return(pk_uint8_t_get_bits(hdr->byte3, 4, 6));
}
void pk_beacon_set_nb_pending_long_addr(packet_t *pk, uint8_t value){
	_802_15_4_Beacon_header *hdr = (_802_15_4_Beacon_header *) (pk->data + sizeof(_802_15_4_header));
	
	pk_uint8_t_set_bits(&(hdr->byte3), value, 4, 6);
	
	//overflow verification
	if (pk_beacon_get_nb_pending_long_addr(pk) + pk_beacon_get_nb_pending_short_addr(pk) > BEACON_MAX_NB_ADDR)
		tools_exit_short(4, "you have too many long addresses for this field\n");

}



//bop slot
uint8_t pk_beacon_get_bop_slot(packet_t *pk){
	_802_15_4_Beacon_header *hdr = (_802_15_4_Beacon_header *) (pk->data + sizeof(_802_15_4_header));
	
	return(pk_uint8_t_get_bits(hdr->byte4, 0, 3));
}
void pk_beacon_set_bop_slot(packet_t *pk, uint8_t value){
	_802_15_4_Beacon_header *hdr = (_802_15_4_Beacon_header *) (pk->data + sizeof(_802_15_4_header));
	
	if (value > 16)
		tools_exit_short(4, "the bop slot is too large, %d is badly defined (or you random generator..)\n", value);

	pk_uint8_t_set_bits(&(hdr->byte4), value, 0, 3);
}

//superframe slot
uint8_t pk_beacon_get_sf_slot(packet_t *pk){
	_802_15_4_Beacon_header *hdr = (_802_15_4_Beacon_header *) (pk->data + sizeof(_802_15_4_header));
	
	return(hdr->sf_slot);
}
void pk_beacon_set_sf_slot(packet_t *pk, uint8_t value){
	_802_15_4_Beacon_header *hdr = (_802_15_4_Beacon_header *) (pk->data + sizeof(_802_15_4_header));
	
	hdr->sf_slot = value;
}

//depth
double pk_beacon_get_depth(packet_t *pk){
	_802_15_4_Beacon_header *hdr = (_802_15_4_Beacon_header *) (pk->data + sizeof(_802_15_4_header));
	
	return((double)hdr->depth * DEPTH_UNIT);
}
void pk_beacon_set_depth(packet_t *pk, double value){
	_802_15_4_Beacon_header *hdr = (_802_15_4_Beacon_header *) (pk->data + sizeof(_802_15_4_header));
	
    int value_int = (int)(value / DEPTH_UNIT);
    
	if (value_int > pow(2, sizeof(depth_pk_t) * 8 - 1))
		tools_exit_short(4, "the depth is too large\n");
    
	hdr->depth = value_int;
}



//has children
short pk_beacon_get_has_a_child(packet_t *pk){
	_802_15_4_Beacon_header *hdr = (_802_15_4_Beacon_header *) (pk->data + sizeof(_802_15_4_header));
	
	return(pk_uint8_t_get_bits(hdr->byte5, 0, 0));
}
void pk_beacon_set_has_a_child(packet_t *pk, short value){
	_802_15_4_Beacon_header *hdr = (_802_15_4_Beacon_header *) (pk->data + sizeof(_802_15_4_header));
	
	pk_uint8_t_set_bits(&(hdr->byte5), value, 0, 0);
}






//------------------------------------------------------------------------------------------------
//SEQNUM: 
//list of SEQMAX (8bits . X)
//list of SEQMIN (8bits . X)
//list of caddr  (4bits . X)
//------------------------------------------------------------------------------------------------

//max seqnum for the i^th muticast addr
uint8_t pk_common_get_seqnum_max(packet_t *pk, int offset, int i){
    uint8_t	*hdr_seqnum = (uint8_t*) (pk->data
                                      + offset
                                      + i * sizeof(uint8_t));
    
    return(*hdr_seqnum);
}
void pk_common_set_seqnum_max(packet_t *pk, int offset, int i, uint8_t value){
    uint8_t	*hdr_seqnum = (uint8_t*) (pk->data 
                                      + offset
                                      + i * sizeof(uint8_t));
    *hdr_seqnum = value;
}

//min seqnum for the i^th muticast addr
uint8_t pk_common_get_seqnum_min(packet_t *pk, int offset, int nb_addrs, int i){
    uint8_t	*hdr_seqnum = (uint8_t*) (pk->data 
                                      + offset
                                      + nb_addrs * sizeof(uint8_t)
                                      + i * sizeof(uint8_t));
//   printf("get min %d, mem offset %d, val %d\n", i, (int) ((void*)hdr_seqnum - (void*)pk->data), *hdr_seqnum);
   return(*hdr_seqnum);
}
void pk_common_set_seqnum_min(packet_t *pk, int offset, int nb_addrs, int i, uint8_t value){
    uint8_t	*hdr_seqnum = (uint8_t*) (pk->data  
                                      + offset
                                      + nb_addrs * sizeof(uint8_t)
                                      + i * sizeof(uint8_t));
//   printf("set min %d, mem offset %d, val %d\n", i, (int) ((void*)hdr_seqnum - (void*)pk->data), value);
   *hdr_seqnum = value;
}

//multicast address for the i^th address
uint16_t pk_common_get_seqnum_caddr(packet_t *pk, int offset, int nb_addrs, int i){
    uint8_t *byte_ptr = (uint8_t*) (pk->data 
                                    + offset
                                    + nb_addrs * (sizeof(uint8_t) + sizeof(uint8_t))
                                    + (int)floor(i / 2));
    
 //   printf("get caddr %d, mem offset %d\n", i, (int) ((void*)byte_ptr - (void*)pk->data));
    
    //half of the byte for each multicast address
    if (i % 2 == 0)
        return(pk_uint8_t_get_bits(*byte_ptr, 0, 3));
    else
        return(pk_uint8_t_get_bits(*byte_ptr, 4, 7));
}
void pk_common_set_seqnum_caddr(packet_t *pk, int offset, int nb_addrs, int i, uint8_t caddr){    
    uint8_t *byte_ptr = (uint8_t*) (pk->data
                                    + offset
                                    + nb_addrs * (sizeof(uint8_t) + sizeof(uint8_t))
                                    + (int)floor(i / 2));
   
    if (caddr > ADDR_MULTI_NB){
        printf("[SEQNUM] the address must be specified in the compressed notation\n");
        exit(8);
    }
    
//    printf("set caddr %d, mem offset %d\n", i, ((void*)byte_ptr - (void*)pk->data));

 
    if (i % 2 == 0)
        pk_uint8_t_set_bits(byte_ptr, caddr, 0, 3);
     else
        pk_uint8_t_set_bits(byte_ptr, caddr, 4, 7);
}




//----------------------------------------------
//		SEQNUMs - BEACONS
//----------------------------------------------


//the number of pending multicast addresses
uint8_t pk_beacon_get_nb_pending_multicast(packet_t *pk){
	_802_15_4_Beacon_header *hdr = (_802_15_4_Beacon_header *) (pk->data + sizeof(_802_15_4_header));
	
	return(pk_uint8_t_get_bits(hdr->byte5, 4, 6));
}
void pk_beacon_set_nb_pending_multicast(packet_t *pk, uint8_t value){
    _802_15_4_Beacon_header *hdr = (_802_15_4_Beacon_header *) (pk->data + sizeof(_802_15_4_header));
    
    pk_uint8_t_set_bits(&(hdr->byte5), value, 4, 6);    
}


//return the offset for the multicast fields 
int pk_beacon_get_seqnum_offset(packet_t *pk){
    return(sizeof(_802_15_4_header)                                                
           + sizeof(_802_15_4_Beacon_header)
           + sizeof(uint16_t) * pk_beacon_get_nb_pending_short_addr(pk) 
           + sizeof(uint64_t) * pk_beacon_get_nb_pending_long_addr(pk)
           );
}

//max seqnum for the i^th muticast addr
uint8_t pk_beacon_get_seqnum_max(packet_t *pk, int i){
    return(pk_common_get_seqnum_max(pk, pk_beacon_get_seqnum_offset(pk), i));
}
void pk_beacon_set_seqnum_max(packet_t *pk, int i, uint8_t value){
    pk_common_set_seqnum_max(pk, pk_beacon_get_seqnum_offset(pk), i, value);
 }

//min seqnum for the i^th muticast addr
uint8_t pk_beacon_get_seqnum_min(packet_t *pk, int i){
    return(pk_common_get_seqnum_min(pk, pk_beacon_get_seqnum_offset(pk), pk_beacon_get_nb_pending_multicast(pk), i));
 }
void pk_beacon_set_seqnum_min(packet_t *pk, int i, uint8_t value){
    pk_common_set_seqnum_min(pk, pk_beacon_get_seqnum_offset(pk), pk_beacon_get_nb_pending_multicast(pk), i, value);
}

//multicast address for the i^th address
uint8_t pk_beacon_get_seqnum_caddr(packet_t *pk, int i){
    return(pk_common_get_seqnum_caddr(pk, pk_beacon_get_seqnum_offset(pk), pk_beacon_get_nb_pending_multicast(pk), i));
}
void pk_beacon_set_seqnum_caddr(packet_t *pk, int i, uint8_t caddr){    
    if (caddr > ADDR_MULTI_NB){
        printf("[SEQNUM] the address must be specified in the compressed notation\n");
        exit(9);
    }
    pk_common_set_seqnum_caddr(pk, pk_beacon_get_seqnum_offset(pk), pk_beacon_get_nb_pending_multicast(pk), i, caddr);
}



//----------------------------------------------
//		SEQNUMs - MULTICAST-REQUEST
//----------------------------------------------

//the number of pending multicast addresses
uint8_t pk_multireq_get_nb_pending_multicast(packet_t *pk){
	_802_15_4_MULTIREQ_header *hdr = (_802_15_4_MULTIREQ_header *) (pk->data + sizeof(_802_15_4_header));
	
	return(pk_uint8_t_get_bits(hdr->byte1, 0, 2));
}
void pk_multireq_set_nb_pending_multicast(packet_t *pk, uint8_t value){
    _802_15_4_MULTIREQ_header *hdr = (_802_15_4_MULTIREQ_header *) (pk->data + sizeof(_802_15_4_header));
    
    pk_uint8_t_set_bits(&(hdr->byte1), value, 0, 2);    
}

//return the offset for the multicast fields 
int pk_multireq_get_seqnum_offset(packet_t *pk){
    return(sizeof(_802_15_4_header) + sizeof(_802_15_4_MULTIREQ_header));
}


//max seqnum for the i^th muticast addr
uint8_t pk_multireq_get_seqnum_max(packet_t *pk, int i){
    return(pk_common_get_seqnum_max(pk, pk_multireq_get_seqnum_offset(pk), i));
}
void pk_multireq_set_seqnum_max(packet_t *pk, int i, uint8_t value){
    pk_common_set_seqnum_max(pk, pk_multireq_get_seqnum_offset(pk), i, value);
}

//min seqnum for the i^th muticast addr
uint8_t pk_multireq_get_seqnum_min(packet_t *pk, int i){
    return(pk_common_get_seqnum_min(pk, pk_multireq_get_seqnum_offset(pk), pk_multireq_get_nb_pending_multicast(pk), i));
}
void pk_multireq_set_seqnum_min(packet_t *pk, int i, uint8_t value){
    pk_common_set_seqnum_min(pk, pk_multireq_get_seqnum_offset(pk), pk_multireq_get_nb_pending_multicast(pk), i, value);
}

//multicast address for the i^th address
uint8_t pk_multireq_get_seqnum_caddr(packet_t *pk, int i){
    return(pk_common_get_seqnum_caddr(pk, pk_multireq_get_seqnum_offset(pk), pk_multireq_get_nb_pending_multicast(pk), i));
}
void pk_multireq_set_seqnum_caddr(packet_t *pk, int i, uint8_t caddr){
    if (caddr > ADDR_MULTI_NB){
        printf("[SEQNUM] the address must be specified in the compressed notation\n");
        exit(9);
    }    
    pk_common_set_seqnum_caddr(pk, pk_multireq_get_seqnum_offset(pk), pk_multireq_get_nb_pending_multicast(pk), i, caddr);
}




//----------------------------------------------
//		PACKET'S FIELDS: HELLOS HEADERS
//----------------------------------------------


//nb neighs
uint8_t pk_hello_get_nb_neighs(packet_t *pk){
	_802_15_4_HELLO_header *hdr = (_802_15_4_HELLO_header *) (pk->data + sizeof(_802_15_4_header));
	
	return(hdr->nb_neighs);
}
void pk_hello_set_nb_neighs(packet_t *pk, uint8_t value){
	_802_15_4_HELLO_header *hdr = (_802_15_4_HELLO_header *) (pk->data + sizeof(_802_15_4_header));
	
	hdr->nb_neighs = value;
}

//addr
uint16_t pk_hello_get_addr(packet_t *pk, int neigh){
	hello_info *neigh_table = (hello_info*) (pk->data + sizeof(_802_15_4_header) + sizeof(_802_15_4_HELLO_header));
	
	return(neigh_table[neigh].addr);
}
void pk_hello_set_addr(packet_t *pk, int neigh, uint16_t value){
	hello_info *neigh_table = (hello_info*) (pk->data + sizeof(_802_15_4_header) + sizeof(_802_15_4_HELLO_header));
	
	neigh_table[neigh].addr = value;
}

//superframe slot
uint8_t pk_hello_get_sf_slot(packet_t *pk, int neigh){
	hello_info *neigh_table = (hello_info*) (pk->data + sizeof(_802_15_4_header) + sizeof(_802_15_4_HELLO_header));
	
	return(neigh_table[neigh].sf_slot);
}
void pk_hello_set_sf_slot(packet_t *pk, int neigh, uint8_t value){
	hello_info *neigh_table = (hello_info*) (pk->data + sizeof(_802_15_4_header) + sizeof(_802_15_4_HELLO_header));
	
	neigh_table[neigh].sf_slot = value;
}

//BOP slot
uint8_t pk_hello_get_bop_slot(packet_t *pk, int neigh){
	hello_info *neigh_table = (hello_info*) (pk->data + sizeof(_802_15_4_header) + sizeof(_802_15_4_HELLO_header));
	
	return(pk_uint8_t_get_bits(neigh_table[neigh].byte1, 0, 3));
}
void pk_hello_set_bop_slot(packet_t *pk, int neigh, uint8_t value){
	hello_info *neigh_table = (hello_info*) (pk->data + sizeof(_802_15_4_header) + sizeof(_802_15_4_HELLO_header));
	
	pk_uint8_t_set_bits(&(neigh_table[neigh].byte1), value, 0, 3);
}

//Has a child
uint8_t pk_hello_get_has_a_child(packet_t *pk, int neigh){
	hello_info *neigh_table = (hello_info*) (pk->data + sizeof(_802_15_4_header) + sizeof(_802_15_4_HELLO_header));
	
	return(pk_uint8_t_get_bits(neigh_table[neigh].byte1, 4, 4));
}
void pk_hello_set_has_a_child(packet_t *pk, int neigh, uint8_t value){
	hello_info *neigh_table = (hello_info*) (pk->data + sizeof(_802_15_4_header) + sizeof(_802_15_4_HELLO_header));
	
	pk_uint8_t_set_bits(&(neigh_table[neigh].byte1   ), value, 4, 4);
}

//Hops
uint8_t pk_hello_get_hops(packet_t *pk, int neigh){
	hello_info *neigh_table = (hello_info*) (pk->data + sizeof(_802_15_4_header) + sizeof(_802_15_4_HELLO_header));
	
	return(pk_uint8_t_get_bits(neigh_table[neigh].byte1, 5, 8));
}
void pk_hello_set_hops(packet_t *pk, int neigh, uint8_t value){
	hello_info *neigh_table = (hello_info*) (pk->data + sizeof(_802_15_4_header) + sizeof(_802_15_4_HELLO_header));
	
	pk_uint8_t_set_bits(&(neigh_table[neigh].byte1   ), value, 5, 8);
}




//----------------------------------------------
//				RETRIEVE THE ADDRs
//----------------------------------------------


//extracts the short address of one packet (converting a long into a short address if required))
uint16_t pk_get_src_short(packet_t *pk){
    
    switch(pk_get_ftype(pk)){

		case MAC_BEACON_PACKET:
            ;
			_802_15_4_Beacon_header *header_beacon = (_802_15_4_Beacon_header*) (pk->data + sizeof(_802_15_4_header));

            return(header_beacon->src);
            break;
        
        case MAC_DATA_PACKET:
			;
			_802_15_4_DATA_header	*header_data = (_802_15_4_DATA_header*) (pk->data + sizeof(_802_15_4_header));
		
			return(header_data->src);
			
		break;				

        case MAC_ACK_PACKET:
            return(ADDR_INVALID_16);
            break;
			
		case MAC_COMMAND_PACKET:			
			;			
			_802_15_4_COMMAND_header	*header_mgmt = (_802_15_4_COMMAND_header*) (pk->data + sizeof(_802_15_4_header));
			
			switch(header_mgmt->type_command){
					
					
				case ASSOCIATION_REQUEST:
					;
					_802_15_4_ASSOCREQ_header	*header_assocreq = (_802_15_4_ASSOCREQ_header*) (pk->data + sizeof(_802_15_4_header));
	
					return(param_addr_long_to_short(header_assocreq->src));
					break;
				case ASSOCIATION_RESPONSE:
					;
					_802_15_4_ASSOCREP_header	*header_assocrep= (_802_15_4_ASSOCREP_header*) (pk->data + sizeof(_802_15_4_header));
					
					return(header_assocrep->src);

					break;
				case DATA_REQUEST:
					
					if (pk_get_SAM(pk) == AM_ADDR_SHORT){
						return(header_mgmt->src);
					}
					else{
						_802_15_4_COMMAND_long_src_header	*header_datareq_long = (_802_15_4_COMMAND_long_src_header*) (pk->data + sizeof(_802_15_4_header));
						
						return(param_addr_long_to_short(header_datareq_long->src));						
					}
					
					
					break;
				case DISASSOCIATION_NOTIFICATION:
					
					return(header_mgmt->src);
					
					break;
				case MULTICAST_REQUEST:
					
					return(header_mgmt->src);
					
					break;
				default:
					tools_exit_short(2, "ERROR: unknown command frame (subtype %d), pk_get_src_short()\n", header_mgmt->type_command);

					break;
			}
            
        //the source is the 0^th address in the hello packet
        case MAC_HELLO_PACKET:	
            return(pk_hello_get_addr(pk, 0));
            
            break;	

	}
	tools_exit_short(3, "pk_get_src_short(). Packet type %d is unkwown for this function\n", pk_get_ftype(pk));
	exit(3);
}

void hook(){

}
//extracts the short address of one packet (converting a long into a short address if required))
uint16_t pk_get_dst_short(packet_t *pk){
	switch(pk_get_ftype(pk)){
			
		case MAC_BEACON_PACKET:			
			return(ADDR_MULTI_BEACONS);
			
            break;				

		case MAC_DATA_PACKET:
			;
			_802_15_4_DATA_header	*header_data = (_802_15_4_DATA_header*) (pk->data + sizeof(_802_15_4_header));			
			return(header_data->dst);
			
		break;				
				
		case MAC_COMMAND_PACKET:			
			;
			
			_802_15_4_COMMAND_header	*header_mgmt = (_802_15_4_COMMAND_header*) (pk->data + sizeof(_802_15_4_header));
			
			switch(header_mgmt->type_command){
					
					
				case ASSOCIATION_REQUEST:
					;
					_802_15_4_ASSOCREQ_header	*header_assocreq = (_802_15_4_ASSOCREQ_header*) (pk->data + sizeof(_802_15_4_header));
					
					return(header_assocreq->dst);
					break;
				case ASSOCIATION_RESPONSE:
					;
					_802_15_4_ASSOCREP_header	*header_assocrep= (_802_15_4_ASSOCREP_header*) (pk->data + sizeof(_802_15_4_header));
					
					return(param_addr_long_to_short(header_assocrep->dst));
					
					break;
				case DATA_REQUEST:
					
					if (pk_get_SAM(pk) == AM_ADDR_SHORT){
						return(header_mgmt->dst);
					}
					else{
						_802_15_4_COMMAND_long_src_header	*header_datareq_long = (_802_15_4_COMMAND_long_src_header*) (pk->data + sizeof(_802_15_4_header));
						
						return(header_datareq_long->dst);						
					}
					
					
					break;
				case DISASSOCIATION_NOTIFICATION:
					
					return(header_mgmt->dst);
					
					break;
				case MULTICAST_REQUEST:
					
					return(header_mgmt->dst);
					
					break;
				default:
					tools_exit_short(2, "ERROR: unknown command frame (type %d), pk_get_dst_short()\n", header_mgmt->type_command);

					break;
			}
			
		case MAC_HELLO_PACKET:
			return(ADDR_MULTI_HELLO);
			break;
	}
    
    hook();    
	tools_exit_short(2, "ERROR, we should not enter here, pk type %d, pk_get_dst_short()\n", pk_get_ftype(pk));
	exit(2);
}








//----------------------------------------------
//					DEBUG-PACKET
//----------------------------------------------


uint64_t pk_get_cbrseq(packet_t *pk){
	switch(pk_get_ftype(pk)){
			
		case MAC_DATA_PACKET:
			;
			cbrv3_packet_header		*header_cbr		= (cbrv3_packet_header *) (pk->data + sizeof(_802_15_4_header) + sizeof(_802_15_4_DATA_header));
			return(header_cbr->sequence);
			
		default:
			return(SEQNUM_INVALID);
	}
}

//returns the humand readable packet type
char *pk_ftype_to_str(int ftype){
	switch(ftype){
			
		case MAC_BEACON_PACKET:
			return("BEACON");
			
		case MAC_ACK_PACKET:
			return("ACK");
			
		case MAC_DATA_PACKET:
			return("DATA");
			
		case MAC_COMMAND_PACKET:
			return("CMD");
			
		case MAC_HELLO_PACKET:
			return("HELLO");
			
		default:
			tools_exit_short(5, "pk_ftype_to_str() - Unknown packet type (%d)\n", ftype);

			break;			
	}	
	exit(5);
}

//comand type into str
char* pk_command_to_str(int command){
	switch(command){
		case ASSOCIATION_REQUEST:
			return("ASSOCIATION_REQUEST");
		
		case ASSOCIATION_RESPONSE:
			return("ASSOCIATION_RESPONSE");
		
		case DISASSOCIATION_NOTIFICATION:
			return("DISASSOCIATION_NOTIFICATION");
	
		case DATA_REQUEST:
			return("DATA_REQUEST");
            
		case MULTICAST_REQUEST:
			return("MULTICAST_REQUEST");
            
		default:	
			tools_exit_short(3, "Unkwnon command type: %d\n", command);

	}	
	exit(3);
}

//this is a multicast request
void pk_print_multireq(call_t *c, packet_t *packet, FILE *pfile){
    int     nb_multicast = pk_multireq_get_nb_pending_multicast(packet);
    int     i;
    
    
	fprintf(pfile, "---- MULTIREQ -----\n");
	fprintf(pfile, "Nb addrs: %d\n",nb_multicast);
	
    for(i=0; i<nb_multicast; i++){
        fprintf(pfile, "caddr: %d\n",	pk_multireq_get_seqnum_caddr(packet, i));
        fprintf(pfile, "min: %d\n",		pk_multireq_get_seqnum_min(packet, i));
        fprintf(pfile, "max: %d\n",		pk_multireq_get_seqnum_max(packet, i));
    
    }
}


//prints the headers
void pk_print_content(call_t *c, packet_t *packet, FILE *pfile){
	
	if (!param_debug())
		return;
	
	uint8_t	pk_type = pk_get_ftype(packet);
	_802_15_4_header *hdr_common = (_802_15_4_header*) packet->data;
	
	fprintf(pfile, "\n------ NODE %d, PK ID %d\n", c->node, packet->id);
	
	//Common headers
	fprintf(pfile, "---- COMMON -----\n");
	fprintf(pfile, "A: %d\n",		pk_get_A(packet));
	fprintf(pfile, "P: %d\n",		pk_get_P(packet));
	fprintf(pfile, "SAM: %d\n",		pk_get_SAM(packet));
	fprintf(pfile, "DAM: %d\n",		pk_get_DAM(packet));
	fprintf(pfile, "ftype: %s\n",	pk_ftype_to_str(pk_type));
	fprintf(pfile, "seqnum: %d\n",	hdr_common->seq_num);
	
	// --- not used ----
	//fprintf(pfile, "S: %d\n",		pk_get_S(hdr_common));
	//fprintf(pfile, "C: %d\n",		pk_get_C(hdr_common));
	//fprintf(pfile, "FCS: %d\n",		hdr_common->FCS);
	
	//specific fields
	switch(pk_type){
		case MAC_DATA_PACKET:
			
			fprintf(pfile, "---- DATA -----\n");
			
			_802_15_4_DATA_header		*header_data	= (_802_15_4_DATA_header*) (packet->data + sizeof(_802_15_4_header));
			fprintf(pfile, "src: %d\n",		header_data->src);
			fprintf(pfile, "dst: %d\n",		header_data->dst);
			fprintf(pfile, "src_e2e: %d\n",	header_data->src_end2end);
			fprintf(pfile, "dst_e2e: %d\n",	header_data->dst_end2end);
			
			break;
		case MAC_COMMAND_PACKET:
			
			fprintf(pfile, "---- CMD -----\n");
			
			_802_15_4_COMMAND_header		*header_cmd	= (_802_15_4_COMMAND_header*) (packet->data + sizeof(_802_15_4_header));
			fprintf(pfile, "type: %s\n",	pk_command_to_str(header_cmd->type_command));
			fprintf(pfile, "src: %d\n",		pk_get_src_short(packet));
			fprintf(pfile, "dst: %d\n",		pk_get_dst_short(packet));		
			
            //remaining parts depend on the packet subtype
            switch (pk_get_subtype(packet)) {
                case MULTICAST_REQUEST:
                    pk_print_multireq(c, packet, pfile);
                    break;
                    
                default:
                    break;
            }
            
			break;
			
			
		case MAC_ACK_PACKET:
			
			fprintf(pfile, "---- ACK -----\n");
			
			//_802_15_4_ACK_header			*header_ack	= (_802_15_4_ACK_header*) (packet->data + sizeof(_802_15_4_header));
			break;
			
			
			
		case MAC_BEACON_PACKET:
			fprintf(pfile, "---- BEACON -----\n");
			
			_802_15_4_Beacon_header		*header_beacon	= (_802_15_4_Beacon_header*) (packet->data + sizeof(_802_15_4_header));
			fprintf(pfile, "src: %d\n",				header_beacon->src);
			fprintf(pfile, "PAN_id: %d\n",			header_beacon->src);
			fprintf(pfile, "BO: %d\n",				pk_beacon_get_BO(packet));
			fprintf(pfile, "SO: %d\n",				pk_beacon_get_SO(packet));
			fprintf(pfile, "PANc: %d\n",			pk_beacon_get_PANc(packet));
			fprintf(pfile, "Assoc permit: %d\n",	pk_beacon_get_assoc_permit(packet));
			fprintf(pfile, "Nb short addr: %d\n",	pk_beacon_get_nb_pending_short_addr(packet));
			fprintf(pfile, "Nb long addr: %d\n",	pk_beacon_get_nb_pending_long_addr(packet));
			fprintf(pfile, "Depth: %f\n",			pk_beacon_get_depth(packet));
			fprintf(pfile, "BOP slot: %d\n",		pk_beacon_get_bop_slot(packet));

			break;
			
			
		case MAC_HELLO_PACKET:
			fprintf(pfile, "---- HELLO -----\n");            
			
            _802_15_4_HELLO_header *header_hello	= (_802_15_4_HELLO_header*) (packet->data + sizeof(_802_15_4_header));
            
			fprintf(pfile, "src: %d\n",             pk_get_src_short(packet));
			fprintf(pfile, "dst: %d\n",             header_hello->dst);
			fprintf(pfile, "nb neighbors: %d\n",	header_hello->nb_neighs);
			break;
			
			
			//_802_15_4_ACK_header			*header_ack	= (_802_15_4_ACK_header*) (packet->data + sizeof(_802_15_4_header));
	}
	
	
}



