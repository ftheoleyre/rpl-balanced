#include <include/modelutils.h>




#ifndef _RADIO_GENERIC_H_
#define _RADIO_GENERIC_H_


//The state of the radio
#define RADIO_OFF           0x00
#define RADIO_RX            0x01
#define RADIO_TX            0x02
#define RADIO_NB_MODES      3


//ENERGY CC110 (J / ns)
#define RADIO_CC1100_OFF   (400*1e-9/1e9)     //400 * 1e-9 (nA)  * 4V  * 1e-9 (during 1ns)    
#define RADIO_CC1100_TX    (31.0*1e-3*4/1e9)  //31 mA, 10dBm
#define RADIO_CC1100_RX    (15.1*1e-3*4/1e9)  //15.1 mA, 250 Kbps, above the sensitivity limit
#define RADIO_CC1100_CS    RADIO_CC1100_RX


//Is this radio busy or idle? (medium activity)
#define RADIO_IDLE          0x00
#define RADIO_BUSY          0x01


//ioctl commands
//IN direction
#define RADIO_IOCTL_MODE_SET        1
#define RADIO_IOCTL_MODE_GET        2
#define RADIO_IOCTL_GETRXPOWER      3       //instantaneous received power
#define RADIO_IOCTL_GETCS           4       //another 802.15.4 signal is currently reveived
#define RADIO_IOCTL_GETENCONSUMMED  5       //energy consummed from the begining of the simulation
#define RADIO_IOCTL_NOTIFFREE_SET   6       //must an interrupt be triggered toward the MAC when the medium becomes free?

//OUT direction (notification)
#define MAC_IOCTL_NOTIFFREE         1       //the medium became idle



//--------------- STRUCTURES ------------------

//Energy
typedef struct{
    uint64_t    duration[RADIO_NB_MODES];
    double      consumption[RADIO_NB_MODES];
}energy_t;


//my private properties
typedef struct{    
    uint64_t    state_time; 
	short       state; 
    energy_t    energy;
    //old variables
    uint64_t    Ts;
    double      power;
    int         channel;
    entityid_t  modulation;
    double      mindBm;
    int         sleep;  
    int         tx_busy;
    int         rx_busy;
    double      rxdBm;
    int         rx_invalid;     //the reception is invalid if the node has changed its radio state *during* the reception (the packet will not be forwarded to the upper layers)
}radio_t;


//--------------- TRANSMISSION - RECEPTION ------------------
 
void        cs_init(call_t *c);
void        tx(call_t *c, packet_t *packet);
void        tx_end(call_t *c, packet_t *packet);
void        rx(call_t *c, packet_t *packet);

//--------------- CARRIER SENSE ------------------

void        cs(call_t *c, packet_t *packet);
double      get_rx_power(call_t *c);


//--------------- INTERNAL VALUES ------------------

//power
double      get_power(call_t *c);
void        set_power(call_t *c, double tx_power_dBm);

//channel (frequency)
int         get_channel(call_t *c);
void        set_channel(call_t *c, int channel);

//modulation
entityid_t  get_modulation(call_t *c);
void        set_modulation(call_t *c, entityid_t modulation);
int         get_modulation_bit_per_symbol(call_t *c);
uint64_t    get_Tb(call_t *c);
void        set_Ts(call_t *c, uint64_t Ts);
uint64_t    get_Ts(call_t *c);

//sensitivity
double      get_sensibility(call_t *c);
void        set_sensibility(call_t *c, double sensitivity);

//headers
int         get_header_size(call_t *c);
int         get_header_real_size(call_t *c);
int         set_header(call_t *c, packet_t *packet, destination_t *dst);


//--------------- ENERGY ------------------

//consumes the energy according to the current state since the last state change 
void radio_energy_update(call_t *c);

//mode into string
char *radio_mode_to_str(int mode);

//switch the radio mode (TX / RX / SLEEP)
void radio_switch_state(call_t *c, int state);






#endif


