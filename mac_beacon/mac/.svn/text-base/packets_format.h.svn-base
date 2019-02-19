
/**
 *  \file   packets_format.h
 *  \brief  packet's format
 *  \author Fabrice Theoleyre
 *  \date   2011
 **/




#ifndef __PACKETS_FORMAT_H__
#define	__PACKETS_FORMAT_H__





//NB:

// a/ long addresses are only used for association requests. Else, 16 short addresses are always used!


//------------------------------------
//		PACKET TYPE
//------------------------------------



/* Defining packet */
#define MAC_BEACON_PACKET			0x01
#define MAC_ACK_PACKET				0x02
#define MAC_DATA_PACKET				0x03
#define MAC_COMMAND_PACKET			0x04
//not standard compliant
#define	MAC_HELLO_PACKET			0x05


/* Defining MAC command frames */
#define ASSOCIATION_REQUEST			0x01
#define ASSOCIATION_RESPONSE		0x02
#define DISASSOCIATION_NOTIFICATION 0x03
#define DATA_REQUEST				0x04
//not standard compliant
#define MULTICAST_REQUEST           0x05

//NOT YET IMPLEMENTED
#define PANID_CONFLICT_NOTIFICATION	0x05
#define	ORPHAN_NOTIFICATION			0x06
#define	BEACON_REQUEST				0x07
#define COORD_REALIGNMENT			0x08
#define GTS_REQUEST					0x09



//for disassociation command
#define	COORD_LEAVE					0x01
#define	DEVICE_LEAVE				0x02




//------------------------------------
//		FIEDL'S special VALUES
//------------------------------------


//values for the SAM and DAM fields
#define	AM_ADDR_LONG				3
#define	AM_ADDR_SHORT				2
#define	AM_ADDR_NO					0


//for beacons
#define	BEACON_MAX_NB_ADDR			7
#define	BEACON_MAX_NB_NEIGH			15


//------------------------------------
//		COMMON HEADERS
//------------------------------------


// Common Packet header
typedef struct  {
	uint8_t		PHY_length;		//not used
	
	// 1bit: reserved
	// 1bit: C, PAN ID compression = 1 -> only one PAN identifier is specified (we are working in a single PAN)
	// 1bit: A, ack request
	// 1bit: P, frame pending for notification
	// 1bit: S, security enabled = 0
	// 3bit: ftype, frame type
	uint8_t		byte2;	
	
	// 2bit: SAM, source addressing mode (10 for 16 bits, 11 for 64 bits addresses)
	// 2bit: FV, frame version = 01 for 2006 version
	// 2bit: DAM, destination addresssing mode (10 for 16 bits, 11 for 64 bits addresses)
	// 2bit: reserved
	uint8_t		byte3;
	
	//to detect errors
	uint16_t	FCS;	//frame Check Sequence
	
	//sequence nb
	uint8_t		seq_num;
 
	//security headers are not implemented
}_802_15_4_header; 



//------------------------------------
//		BEACON
//------------------------------------

//beacon specific fields
typedef struct {
	//addresses
	uint16_t	src;
	uint16_t	PAN_id;
	   
	//specs
	//4bits: BO
	//4bits: SO
	uint8_t		byte1;
	
	//4bits: Final CAP slot	//unused
	//1bit: battery life	//unused
	//1bit: reserved
	//1bit: PANc			//1 for the PAN coordinator
	//1bit: assoc_permit	//association is authorized, always 1, not used
	uint8_t		byte2;
	
	//GTS fields
	//not yet implemented
	
	
	//pending addresses
	//3bits: nb of short addresses
	//1bit: reserved
	//3bits: nb of long addresses
	//1bit: reserved
	uint8_t		byte3;
	//the actual list is a block of memory, placed just after the struct!!
	
	//additionnal info (not in the standard)
	//4bits: BOP slot
	uint8_t		byte4;
	uint8_t		sf_slot;
 	depth_pk_t  depth;
    
	//byte5
	//1bit:     has children
    //3bits:    nb of multicast pending addresses
    uint8_t		byte5;
} _802_15_4_Beacon_header;
//fist block of memory -> pending destinations
//second block -> slots used in the neighborhood
//third block -> map of pending multicast addresses & seqnums (<seqnm_min, seqnum_max, @multicast compressed>), organized in columns (compressed @ = 4 bits);





//------------------------------------
//		DATA
//------------------------------------

//header for data
typedef struct  {
	//addresses
	uint16_t	src;
	uint16_t	dst;
	uint16_t	PAN_id;
	
	//final addresses (not standard compliant)
	uint16_t	src_end2end;
	uint16_t	dst_end2end;
	
}_802_15_4_DATA_header;




//------------------------------------
//		ACKS
//------------------------------------

//NB: nothing is specific to ACKs


//------------------------------------
//		MANAGEMENT
//------------------------------------

// BE CAREFUL: COMMON FIELDS ARE PLACED AT THE BEGINING
// if variable fields (short/long addresses) are placed before,
// the correct btes will not be read: the same struct is used to 
// extract the first fields (type + dest (always in 16 bits)

//The first field of each command has to be the type command!

//management commands
typedef struct {
	//command
	uint8_t		type_command;

 	//addresses
	uint16_t	src;
	uint16_t	dst;	
} _802_15_4_COMMAND_header;

//management commands (long src, to get my association reply)
typedef struct {
	//command
	uint8_t		type_command;
	
 	//addresses
	uint64_t	src;
	uint16_t	dst;	
	
} _802_15_4_COMMAND_long_src_header;


//------------------------------------
//		MGMT: ASSOCIATION
//------------------------------------

//dst=16bits, src=64bits
typedef struct {
	//command
	uint8_t		type_command;

 	//addresses
	uint64_t	src;
	uint16_t	dst;
} _802_15_4_ASSOCREQ_header;


//dst=64bits, src=16bits
typedef struct {
	//command
	uint8_t		type_command;
	
 	//addresses
	uint16_t	src;
	uint64_t	dst;
	
	//info
	uint16_t	addr_assigned;
	uint8_t		status;
} _802_15_4_ASSOCREP_header;




//------------------------------------
//		MGMT: DISASSOCIATION
//------------------------------------

typedef struct {
	//command
	uint8_t		type_command;
	
 	//addresses
	uint16_t	src;
	uint16_t	dst;
	
	//info
	uint8_t		status;
} _802_15_4_DISASSOC_header;



//------------------------------------
//		MGMT: MULTICAST-REQ
//------------------------------------

typedef struct {
	//command
	uint8_t		type_command;
	
 	//addresses
	uint16_t	src;
	uint16_t	dst;
	
	//info
	uint8_t		byte1;      
    //3bits: nb of multicast addresses
} _802_15_4_MULTIREQ_header;
//first block -> map of pending multicast addresses & seqnums (<seqnm_min, seqnum_max, @multicast compressed>), organized in columns (compressed @ = 4 bits);
//R: list of seqmax, then list of seqmin, and then list of caddr


//------------------------------------
//		HELLOS
//------------------------------------

typedef struct{
	uint16_t	addr;
	uint8_t		sf_slot;	

	//4bits: BOP slot
	//1bit: has children
    //3bits: hops
	uint8_t		byte1;
} hello_info;


//header for hellos packets
typedef struct  {
//	uint16_t	src;
	uint16_t	dst;
	uint8_t		nb_neighs;
}_802_15_4_HELLO_header;
//NB: the neigh table is directly copied just after this header (this is a memory block)





#endif
