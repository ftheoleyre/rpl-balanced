/**
 *  \file   const.h
 *  \brief  constants
 *  \author Fabrice Theoleyre
 *  \date   2011
 **/





#ifndef __CONST_H__
#define __CONST_H__


//------------------------------------
//		IOCTL with RPL
//------------------------------------
#define MAC_IOCTL_RETRANSMIT 		30  //MAC announces RPL when a node dropped a packet because of too many retransmissions (3)
#define ENERGY_CONSUMED 			40  //MAC updates the energy consumed info to the RPL layer
#define REMOVE_PARENT				60  //MAC announces RPL the he deleted the corresponding parent




//------------------------------------
//		COMMON
//------------------------------------

//booleans
#define		TRUE							1
#define		FALSE							0


//special values
#define		HOP_INFINITY					(int)(pow(2,8)-2)
#define		INT_INFINITY					(int)(pow(2,sizeof(int) * 8)-1)




//------------------------------------
//		FIGURES
//------------------------------------

//constants for the figures
#define		GRAPHIC_MAX				10000
#define		GRAPHIC_RADIUS			90
#define		GRAPHIC_POLICE_SIZE		30
#define		GRAPHIC_SHIFT_X			150
#define		GRAPHIC_SHIFT_Y			130

//Colors used for drawing (post processing). Refer to tools.c
#define		BLACK					0
#define		WHITE					7
#define		GRAY					33
#define		RED						4
#define		GREEN					3

//types for lines
#define		SOLID					0
#define		DASHED					1
#define		DOTTED					4








//------------------------------------
//			STATS
//------------------------------------


//we should discard the last seconds
//packets may have not been received just because simulation terminated too early
//NB: times are expressed in ns (here, the last 15 seconds)
#define	STATS_STOP_BEFORE_END				(15e9)


//reasons when a packet is dropped
#define	DROP_NO								0
#define	DROP_CCA							1
#define	DROP_NOT_ASSOCIATED					2
#define	DROP_RETX							3
#define	DROP_BUFFER_TIMEOUT					4


//the number of times we should remove a set of links to test the connectivity)
#define STATS_NB_TESTS_FOR_CONNECTIVITY     50




//------------------------------------
//		PROTOCOL / MAC
//------------------------------------

//the max nb of parents I can save
#define MAX_NB_PARENTS						32


// Defining node type
#define PAN_COORD							1  
#define PAN_NODE							2


//guard time when I have to wake-up (in ns) (here 150us)
#define	GUARD_TIME							150000


//Algo to compute when my own superframe will be scheduled
#define	ALGO_SF_ORG_802154					1
#define	ALGO_SF_ORG_RAND					2
#define	ALGO_SF_ORG_GREEDY					3
#define	ALGO_SF_ORG_GOD						4

//kind of depth
#define DEPTH_HOPS                          1
#define DEPTH_ETX                           2
#define DEPTH_BER                           3

//depth bounds
#define MAX_DEPTH                           16
#define DEPTH_DELTA                         1.0
#define DEPTH_UNIT                          0.25    //the depth is encoded in beacons by multiple of 0.25


//For the Beacon-Only Period
#define ALGO_BOP_ORIG                       1
#define ALGO_BOP_BACKOFF                    2


//For Multicast delivery
#define ALGO_MULTICAST_DUPLICATE            1
#define ALGO_MULTICAST_SEQ                  2
#define ALGO_MULTICAST_DUPLICATE_ACK        3


//interfering range (for stats: does a pair of nodes interfere?)
#define	INTERF_RANGE						120.0
#define	RADIO_RANGE                         50.0


//Hellos
#define HELLO_MAX_HOPS                      1   //we include in the hellos all our HELLO_MAX_HOPS-neighbors



//roles
#define     ROLE_NO                         0
#define     ROLE_COORDINATOR                1
#define     ROLE_CHILD                      2
#define     ROLE_SCAN                       3


//------------------------------------
//		QOS / PK PRIORITY
//------------------------------------

//packets with largest priority will be transmitted first
#define	PRIO_DATA							1
#define	PRIO_HELLO							2
#define	PRIO_ASSOC							3





//------------------------------------
//		ASSIGNMENT
//------------------------------------

//time before convergence (a node never sleeps before to discover several parents) (here 15 seconds)
#define	TIME_BEFORE_CONVERGENCE				(60e9)

//invalid values (the last value of the value's range)
#define	SF_SLOT_INVALID						(int)(pow(2,8) - 1)	//SF slot: 8 bits
#define	BOP_SLOT_INVALID					(int)(pow(2,4) - 1)	//BOP slot: 4 bits

#define INVALID_DISTANCE                    50     //in hops, a realistic max that works with 6 bits fields
#define INVALID_BOOLEAN                     2
#define INVALID_UINT8_T                     253

//Seq nums
#define    SEQNUM_INVALID                   0    //this sequence number is reserved for invalid values


//------------------------------------
//		NEIGHBORS
//------------------------------------


//keep only the beacons for the X last superframes
#define	BEACONS_FOR_STATS					3


#define	INVALID_BOPERIOD					0
#define	INVALID_PERIOD						0.0



//------------------------------------
//		ADDRS
//------------------------------------


//special adddresses
#define	ADDR_INVALID_16						(0xdfff)
#define	ADDR_INVALID_64						(0xfffffffffffffffe)

//Anycast Addresses
#define ADDR_ANY_MIN                        (0xeff0)
#define ADDR_ANY_MAX                        (0xefff)
#define	ADDR_ANY_PARENT_16					(0xefff)

//Multicast addresses - limits
#define ADDR_MULTI_NB                       16
#define ADDR_MULTI_MIN                      (0xfff0)
#define ADDR_MULTI_MAX                      (0xffff)

//specific multicast addresses
//NB: destination for CBR boradcasts: ADDR_MULTI_CTREE
#define ADDR_MULTI_HELLO                    (0xffff)    //65535  
#define ADDR_MULTI_BEACONS                  (0xfffe)    //65534
#define ADDR_MULTI_DISC                     (0xfffd)    //65533 //tous les noeds dans la portee radio
#define ADDR_MULTI_CTREE                    (0xfffc)    //65532
#define ADDR_MULTI_FLOOD_DISC               (0xfffb)    //65531
#define ADDR_MULTI_FLOOD_CTREE              (0xfffa)    //65530

//PAN ID
#define	ID_PAN_COORD						(0x0000)





//------------------------------------
//		STATE MACHINE
//------------------------------------

//This is not a 'real' state (it just means we don't change it)
#define	STATE_KEPT							-1

// STATES for the State machine of the IEEE 802.15.4 protocol
#define	STATE_SLEEP							1

//for the transmissions from/to my coordinator (i.e. parent)
#define	STATE_CHILD_WAIT_BEACON				10
#define	STATE_CHILD_STOP_WAIT_BEACON		11
#define	STATE_CHILD_IDLE					12
#define	STATE_CHILD_BACKOFF					13
#define	STATE_CHILD_CHECK_CHANNEL			14
#define	STATE_CHILD_CHECK_CHANNEL_CCA_END	15
#define	STATE_CHILD_TX						16
#define	STATE_CHILD_WAIT_ACK				17			
#define	STATE_CHILD_WAIT_REP				18

//for my own superframe management
#define	STATE_COORD_IDLE					20
#define	STATE_COORD_WAIT_ACK				21
#define	STATE_COORD_DATA_TX					22

//the end is not yet connected to the PAN
#define	STATE_UNASSOC_WAIT_BEACON			30
#define	STATE_UNASSOC_WAIT_ACK_ASSOC_REQ	31
#define	STATE_UNASSOC_WAIT_ASSOC_REP		32

//for the BOP backoff
#define STATE_BOP_BACKOFF                   40
#define STATE_BOP_WAIT_FREE                 41






//------------------------------------
//		PARAMETERS
//------------------------------------



//parameters
#define	BO_DEFAULT						5
#define	SO_DEFAULT						3




//------------------------------------
//		802.15.4 PARAM'S VALUES
//------------------------------------
//NB: times are defined in ns
// 1 symbol = 16,000 = 16us

//PHY symbols
#define	phySymbolsPerOctet		2			// p46, cf. IEEE154_SYMBOLS_PER_OCTET of TinyOS 2
#define aMaxPHYPacketSize       127         //nb of bytes per frame p45
#define	phySHRDuration			10			// synchronization header in the PHY (p46)
//#define	phyMaxFrameDuration     ( phySHRDuration + ceiling([aMaxPHYPacketSize + 1] x phySymbolsPerOctet)
#define	phyMaxFrameDuration		4256000		//266 symbols -> 

//durations
#define aNumSuperFrameSlot		16			/* cf p159 ref 802.15.4-2006 */
#define aBaseSLotDuration		960000		/* 60 symbols cf p159 ref 802.15.4-2006 */
#define symbolTime				16000		// 16us 
#define	aTurnaroundTime			192000		//12 symbols, turnarround time rx/tx
#define CCAduration				128000		//8 symbols

//IFS
#define aMaxSIFSFrameSize		18		/* frame size in bytes under which an SIFS is authorized, bytes, 802.15.4-2006, p159 */
#define macMinLIFSPeriod		640000  /* 40 symbols cf p30 ref 802.15.4-2006 */
#define macMinSIFSPeriod		192000  /* 12 symbols cf p30 ref 802.15.4-2006 */

//backoffs
#define aUnitBackoffPeriod		320000.0  /* 20 symbols cf p159 ref 802.15.4-2006 */
#define EDThresholdMin			-105.0    
#define macMinBE				3       /* cf p164 ref 802.15.4-2006 */
#define macMinBEMin				0       /* cf p164 ref 802.15.4-2006 */
#define macMaxBE				5       /* cf p163 ref 802.15.4-2006 */
#define macMaxBEMin				3       /* cf p163 ref 802.15.4-2006 */
#define macMaxBEMax				8       /* cf p163 ref 802.15.4-2006 */
#define CW						2		/* cf p30 ref 802.15.4-2006 */

//acks
#define	mactACK					(aTurnaroundTime)	
/* time before ack transmission (p170 802.15.4-2006) */
/* aTurnaroundTime <= value <= aTurnaroundTime + aUnitBackoffPeriod */

//retransmissions
#define macMaxCSMABackoffs		4       /* nb of unsuccessful CCA before dropping the packet cf p163 ref 802.15.4-2006 */
#define macMaxCSMABackoffsMin	0       /* cf p163 ref 802.15.4-2006 */
#define macMaxCSMABackoffsMax	4       /* cf p163 ref 802.15.4-2006 */
#define macMaxFrameRetries		3		/* nb of retx if no ack rcvd cf p163 ref 802.15.4-2006 */

//timeouts
#define	macTransactionPersistenceTime (3)	/* no default value in the standard -> defined in number of BIs */
#define	NEIGH_TIMEOUT			8			/* in number of BI (I MUST miss e.g. 8 beacons or have no news from this node for 8BI before removing this node) */
#define TIMEOUT_SEQNUM_VERIF    macTransactionPersistenceTime   //in number of BIs

//Timeouts in the data buffer
#define TIMEOUT_INFINITY        (0-1)
#define TIMEOUT_DEFAULT         0


//waiting time
#define macResponseWaitTime		491520000  /*  = aBaseSuperframeDuration * 32 cf p165 ref 802.15.4-2006 */
#define macAckWaitDuration		864000		/* 54 symbols for channels 11->26  (120 symbols for channels 0->10)*/

//aUnitBackoffPeriod + aTurnaroundTime + phySHRDuration + 6 * phySymbolsPerOctet)	/* cf. eq. p160 */
extern int	macMaxFrameTotalWaitTime;			//time we must wait a data after having tx a data-req (p160)

#endif

