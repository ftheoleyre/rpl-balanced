/**
 *  \file   half1d.c
 *  \brief  Halfduplex 1-d radio model
 *  \author Guillaume Chelius
 *  \date   2007
 **/
#include <include/modelutils.h>


#include "radio_generic.h"


/* ************************************************** */
/* ************************************************** */
model_t model =  {
    "IEEE 802.15.4 radio with sleeping mode",
    "Fabrice Theoleyre",
    "0.1",
    MODELTYPE_RADIO, 
    {NULL, 0}
};


//parameters
short RADIO_IOCTL_NOTIFFREE_ = 0;    //does the radio layer must notify the MAC layer when the medium becomes idle?

/* ************************************************** */
//                  ENERGY
/* ************************************************** */


//converts a uint64_t time into a human readeable string
char *time_to_str(uint64_t time_u, char *msg, int length){
    float   time = time_u *1.0;
    
    double  minutes_f;
    double  seconds = modf(time / 60e9, &minutes_f);    //separate minutes and seconds
    seconds *= 60;
    int     minutes = (int)minutes_f % 60;              //discard hours in printing
        
	snprintf(msg, length , "%2dmn, %2ds, %3dms, %3dus", 
			 (int) minutes, 
			 (int) floor(seconds), 
			 (int) (floor(seconds * 1E3) - 1E3 * floor(seconds)), 
			 (int) (floor(seconds * 1E6) - 1E3 * floor(seconds * 1E3)));	
	
	return(msg);
    
}
//the energy consumption for the different modes (here only 4 modes)
//Watts / ns
double  RADIO_ENERGY_CONSUMPTION[RADIO_NB_MODES];


//consumes the energy according to the current state since the last state change 
//NB: we could introduce here energy for changing the state of the radio
void radio_energy_update(call_t *c){
    radio_t *nodedata = get_node_private_data(c);
    uint64_t duration;
    
    //updates the energy consumption according to the model
    switch (nodedata->state) {
        case RADIO_OFF:
        case RADIO_RX:
        case RADIO_TX:
             
            //updates the energy consumption before the state changes!
            duration = get_time() - nodedata->state_time;
            nodedata->energy.duration[nodedata->state]       += duration; 
            nodedata->energy.consumption[nodedata->state]    += duration * RADIO_ENERGY_CONSUMPTION[nodedata->state];
            
         //   char msg[50];
           // printf("%d - RADIO - %s (%s) - cumulative time\n", c->node, radio_mode_to_str_(nodedata->state), time_to_str(nodedata->energy.duration[nodedata->state], msg, 50));
            break;
            
        default:
            fprintf(stderr, "[RADIO] %d -> unknown state %d\n", c->node, nodedata->state);
            exit(5);
            
            break;
    }
    battery_consume(c, duration * RADIO_ENERGY_CONSUMPTION[nodedata->state]);    
}

/* ************************************************** */
//                  STATES
/* ************************************************** */

//mode into string
char *radio_mode_to_str(int mode){
    
    switch(mode){
        case RADIO_RX:
            return("RX ");
        case RADIO_TX:
            return("TX ");
        case RADIO_OFF:
            return("OFF");
    }
    fprintf(stderr, "[RADIO] Unknown radio mode %d\n", mode);
    return("");
}

void sleep(call_t *c) {
    radio_t *nodedata = get_node_private_data(c);
    cs_init(c);
    nodedata->sleep = 1;
    radio_switch_state(c, RADIO_OFF);

}

void wakeup(call_t *c) {
    radio_t *nodedata = get_node_private_data(c);
    cs_init(c);
    nodedata->sleep = 0;
    radio_switch_state(c, RADIO_RX);
}


//switch the current mode
void radio_switch_state(call_t *c, int state){
    radio_t *nodedata = get_node_private_data(c);
    
    if (nodedata->state == state) {
        return;
    }
    //if a transmision is on-going and we change our radio state, the reception will fail
    if (nodedata->rx_busy != -1)
        nodedata->rx_invalid = 1;
    
    //update energy consumption
    radio_energy_update(c);
    
    //new state
    nodedata->state        = state;   
    nodedata->state_time   = get_time();
    
    
    
    //sleeping mode update
    if (nodedata->state == RADIO_OFF)
        nodedata->sleep = 1;
    else
        nodedata->sleep = 0;

}



/* ************************************************** */
//                    SIMULATION BEGINING
/* ************************************************** */
int init(call_t *c, void *params) {
    return 0;
}

int destroy(call_t *c) {
    return 0;
}


int setnode(call_t *c, void *params) {
    radio_t *nodedata = malloc(sizeof(radio_t));
    param_t *param;
    
//    printf("init radio\n");
    
	//set_node_private_data(c, nodedata);
	
    //default values
    nodedata->Ts            = 91;
    nodedata->channel       = 0;
    nodedata->power         = 0;
    nodedata->modulation    = -1;
    nodedata->mindBm        = -92;
 
    //state
    nodedata->tx_busy       = -1;
    nodedata->rx_busy       = -1;
    nodedata->rxdBm         = MIN_DBM;
    nodedata->sleep         = 0;
 
    //printf("TX-%d: setnode\n", c->node);
    
    //initial mode
    nodedata->state         = RADIO_OFF;
    nodedata->state_time    = get_time();
	
	
    //a reception becomes invalid if the node changes its state *during* the reception
    nodedata->rx_invalid    = 0;
    
    //energy for all the modes (BE CAREFUL: RADIO_TX is the largest mode value)
    int i;
    for(i=0; i<RADIO_NB_MODES; i++){
        nodedata->energy.duration[i]    = 0; 
        nodedata->energy.consumption[i] = 0;
    }
    
    //default values for the energy consumption (Joule / ns)
    RADIO_ENERGY_CONSUMPTION[RADIO_OFF]   = RADIO_CC1100_OFF;
    RADIO_ENERGY_CONSUMPTION[RADIO_RX]    = RADIO_CC1100_RX;
    RADIO_ENERGY_CONSUMPTION[RADIO_TX]    = RADIO_CC1100_TX;
     
    /* get parameters */
    das_init_traverse(params);
    while ((param = (param_t *) das_traverse(params)) != NULL) {
        if (!strcmp(param->key, "sensibility")) {
            if (get_param_double(param->value, &(nodedata->mindBm))) {
                goto error;
            }
        }
        if (!strcmp(param->key, "T_s")) {
            if (get_param_time(param->value, &(nodedata->Ts))) {
                goto error;
            }
        }
        if (!strcmp(param->key, "channel")) {
            if (get_param_integer(param->value, &(nodedata->channel))) {
                goto error;
            }
        }
        if (!strcmp(param->key, "dBm")) {
            if (get_param_double(param->value, &(nodedata->power))) {
                goto error;
            }
        }
        if (!strcmp(param->key, "modulation")) {
            if (get_param_entity(param->value, &(nodedata->modulation))) {
                goto error;
            }
        }
        if (!strcmp(param->key, "en-off")) {
            if (get_param_double(param->value, &(RADIO_ENERGY_CONSUMPTION[RADIO_OFF]))) {
                goto error;
            }
        }
        if (!strcmp(param->key, "en-rx")) {
            if (get_param_double(param->value, &(RADIO_ENERGY_CONSUMPTION[RADIO_RX]))) {
                goto error;
            }
        }
        if (!strcmp(param->key, "en-tx")) {
            if (get_param_double(param->value, &(RADIO_ENERGY_CONSUMPTION[RADIO_TX]))) {
                goto error;
            }
        }
  }

    set_node_private_data(c, nodedata);
    return 0;

 error:
    free(nodedata);
    return -1;
}

int unsetnode(call_t *c) {
//    free(get_node_private_data(c));       //SHOULD NOT since this info may be used by other layers which have not yet been stopped
    return 0;
}

//initializes the parameters
int bootstrap(call_t *c) {
//    radio_t *nodedata = get_node_private_data(c);
     return 0;
}


//ioctl for inter-layer communication
int	ioctl(call_t *c, int option, void *in, void **out) {
    radio_t *nodedata = get_node_private_data(c);
        
//    printf("option %d\n", option);
    
    switch(option){
           
        //change the radio mode for this module
        case RADIO_IOCTL_MODE_SET:
            radio_switch_state(c, *(int*)in);
            
            break;
            
        //return the radio mode for this module
        case RADIO_IOCTL_MODE_GET:
            *(int*)out =  nodedata->state;
            
            break;
            
        //to retrieve the current rx energy level (enegry threshold detection)
        case RADIO_IOCTL_GETRXPOWER:   
            ;
            double signal = get_rx_power(c);            
             *(double*)out = signal;
            
          //  printf("signal %e\n", signal);            
        break;
            
        //another 802.15.4 signal is currently received
        case RADIO_IOCTL_GETCS:
            ;
            short carrier_sense = (nodedata->rx_busy != -1);
            *(short*)out = carrier_sense;
            
        break;            
            
            
        //GET RADIO CONSUMPTION
        case RADIO_IOCTL_GETENCONSUMMED:
            ;
           // printf("ptr set: %d\n", &(nodedata->energy));
            //energy_t **out_en;
            *out = &(nodedata->energy);            
            
           // printf("PTR set : %ld\n", (long int)(nodedata->energy.duration));
            
            break;
        
        //Notifies the MAC layer (or not) when the medium becomes idle
        case RADIO_IOCTL_NOTIFFREE_SET:
            
            RADIO_IOCTL_NOTIFFREE_ = *(short*)in;
            
            break;
            
        default:
            fprintf(stderr, "[BUG] unknown ioctl option %d\n", option);
            exit(6);
            break;            
    
    }
    
    return 0;
}


/* ************************************************** */
//                     TRANSMISSION
/* ************************************************** */
void radio_init(call_t *c) {
    radio_t *nodedata = get_node_private_data(c);
    /* log */
    if (nodedata->tx_busy != -1) {
        PRINT_REPLAY("radio-tx1 %"PRId64" %d\n", get_time(), c->node);
    }
    if (nodedata->rx_busy != -1) {
        PRINT_REPLAY("radio-rx1 %"PRId64" %d\n", get_time(), c->node);
    }
    // reinit the radio variables
    nodedata->tx_busy = -1;
    nodedata->rx_busy = -1;
    nodedata->rxdBm = MIN_DBM;
}

//Be careful: the transmission is done even if the medium is already busy!
//We have *no* "idle medium" verification
void tx(call_t *c, packet_t *packet) {
    radio_t *nodedata = get_node_private_data(c);
    array_t *down = get_entity_bindings_down(c);
    int i = down->size;
  
    
    //sleeping mode
    if (nodedata->state == RADIO_OFF) {
        printf("[RADIO] node %d is sleeping: it dropped the packet to transmit\n", c->node);
        packet_dealloc(packet);
        exit(7);
        return;
    }
     
    //the node is currently receiving something else
    //NB: this can happen when two nodes starts exactly at the same time to send one packet (the rx state is updated before the second transmitter "sends" its frame)
 
    //ERROR: double transmission (I try to send two packets at the same time)
    if (nodedata->tx_busy != -1) {
        printf("[BUG TX] ID = %d should not transmit now it already started another transmission\nA node CANNOT send two packets at the same time!\nTX busy flag: %d\n", c->node, nodedata->tx_busy);
        printf("time %f\n", (double)get_time());
        exit(5);
        return;
    }
    
    //new transmitting mode
    cs_init(c);
    nodedata->tx_busy = packet->id;
    radio_switch_state(c, RADIO_TX);
    //printf("TX-%d: started\n", c->node);
    
    // log tx
    PRINT_REPLAY("radio-tx0 %"PRId64" %d 50\n", get_time(), c->node);
    
    // transmit to antenna
    while (i--) {
        packet_t *packet_down;
        
        if (i > 0) {
            packet_down = packet_clone(packet);         
        } else {
            packet_down = packet;
        }
        c->from = down->elts[i];
        MEDIA_TX(c, packet_down);
    }
    
    return;
}

void tx_end(call_t *c, packet_t *packet) {
    radio_t *nodedata = get_node_private_data(c);
    
    
     // check wether the reception has killed us
    if (!is_node_alive(c->node)) {
        return;
    }

    // the pointer for the currently transmitted packet on the medium
    if (nodedata->tx_busy == packet->id) {
        PRINT_REPLAY("radio-tx1 %"PRId64" %d\n", get_time(), c->node);
        nodedata->tx_busy = -1;
        
   //     printf("TX-%d finished\n", c->node);
 //       printf("[RADIO] %d is not anymore busy, it is now in receiving mode\n\n", packet->id);
    }
    else{
        printf("problem, a transmission is ending although we are currently transmitting another packet (2 simultaneous transmissions)\n");
        exit(3);
    }

    //the new state is receiving by default
    radio_switch_state(c, RADIO_RX);
    
    return;
}


/* ************************************************** */
//                      RECEPTION
/* ************************************************** */
void rx(call_t *c, packet_t *packet) {
    radio_t *nodedata = get_node_private_data(c);
    array_t *up = get_entity_bindings_up(c);
    int i = up->size;
   
    // handle carrier sense
    if (nodedata->rx_busy == packet->id) {
        nodedata->rx_busy = -1;
        nodedata->rxdBm   = MIN_DBM;            //flush the current signal level (= noise since the signal has finished)
        
       // printf("%d -> end of tx\n", c->node);
        
        //notifies the MAC layer the medium is now free
        if (RADIO_IOCTL_NOTIFFREE_){
            call_t c_mac = {get_entity_bindings_up(c)->elts[0], c->node, c->entity};
            IOCTL(&c_mac, MAC_IOCTL_NOTIFFREE, NULL, NULL);
            
         }
        
        /* log rx */
        PRINT_REPLAY("radio-rx1 %"PRId64" %d\n", get_time(), c->node);
    } 
    //this signal was discarded
    else { 
        //this was the max signal -> reset the current noise level
        if (nodedata->rxdBm == packet->rxdBm){
            nodedata->rxdBm   = MIN_DBM;
        }
        packet_dealloc(packet);
        return;
    }

    // check wether the reception has killed us
    if (!is_node_alive(c->node)) {
        packet_dealloc(packet);
        return;
    }

    // drop packet depending on the Packet Error Rate
    if (get_random_double() < packet->PER) {
        packet_dealloc(packet);
        return;
    }    

    // forward to upper layers
    else if ((!nodedata->rx_invalid) && (nodedata->state == RADIO_RX)) 
        while (i--) {
            call_t c_up = {up->elts[i], c->node, c->entity};
            packet_t *packet_up;

            if (i > 0) {
                packet_up = packet_clone(packet);         
            } else {
                packet_up = packet;
            }
            RX(&c_up, packet_up);
        }
    //the reception has failed (either currently not in RX mode or the radio state has been changed during the reception)
    else{
        nodedata->rx_invalid = 0;       //reset of the invalid flag
        packet_dealloc(packet);
        return;
    }
}



/* ************************************************** */
//                      CARRIER SENSE
/* ************************************************** */
void cs_init(call_t *c) {
    radio_t *nodedata = get_node_private_data(c);
    /* log */
    if (nodedata->tx_busy != -1) {
        PRINT_REPLAY("radio-tx1 %"PRId64" %d\n", get_time(), c->node);
    }
    if (nodedata->rx_busy != -1) {
        PRINT_REPLAY("radio-rx1 %"PRId64" %d\n", get_time(), c->node);
    }
    // reinit the radio variables
    nodedata->tx_busy = -1;
    nodedata->rx_busy = -1;
    nodedata->rxdBm = MIN_DBM;
    
  //  printf("TX-%d: init\n", c->node);
    
//    printf("%d -> radio reset\n", c->node);
}

void cs(call_t *c, packet_t *packet) {
    radio_t *nodedata = get_node_private_data(c);
    short   pk_is_correct = 1;
    
    //when the node is asleep or already transmitting something, the reception will anyway be considered invalid after the signal has finished to be transmitted
    //however, we can with this method make a CCA even when the node has just terminated its transmission / finished to sleep
        
    /* check channel */
    if (nodedata->channel != packet->channel) {
        return;
    }
    
    /* check sensibility */
    if (packet->rxdBm < nodedata->mindBm) {
        pk_is_correct = 0;
    }

    /* check Ts */
    if (nodedata->Ts != (packet->Tb*radio_get_modulation_bit_per_symbol(c))) {
        pk_is_correct = 0;
    }

    /* check modulation */
    if (packet->modulation != nodedata->modulation) {
        pk_is_correct = 0;
    }

    /* the receiver captures the best signal */
    if (packet->rxdBm > nodedata->rxdBm) {
        nodedata->rxdBm = packet->rxdBm;
       
        //this is considered a packet and not noise
       if (pk_is_correct)
            nodedata->rx_busy = packet->id;
         
        /* log cs */
        PRINT_REPLAY("radio-rx0 %"PRId64" %d\n", get_time(), c->node);
        return;
    }
        
        
    return;
}


double get_noise(call_t *c) {
    radio_t *nodedata = get_node_private_data(c);
    entityid_t *down = get_entity_links_down(c);
    
    c->from = down[0];
    return MEDIA_GET_NOISE(c, nodedata->channel);
}


double get_rx_power(call_t *c) {
    radio_t *nodedata = get_node_private_data(c);
    return nodedata->rxdBm;
}

double get_cs(call_t *c){
    return(get_rx_power(c));
}





/* ************************************************** */
//         INTERFACE FOR SOME INTERN VALUES
/* ************************************************** */


//-------- POWER ---------

double get_power(call_t *c) {
    radio_t *nodedata = get_node_private_data(c);
    return nodedata->power;
}

void set_power(call_t *c, double power) {
    radio_t *nodedata = get_node_private_data(c);
    nodedata->power = power;
}


//-------- CHANNEL ---------


int get_channel(call_t *c) {
    radio_t *nodedata = get_node_private_data(c);
    return nodedata->channel;
}

void set_channel(call_t *c, int channel) {
    radio_t *nodedata = get_node_private_data(c);
    cs_init(c);
    nodedata->channel = channel;
}

//-------- MODULATION ---------

entityid_t get_modulation(call_t *c) {
    radio_t *nodedata = get_node_private_data(c);
    return nodedata->modulation;
}

void set_modulation(call_t *c, entityid_t modulation) {
    radio_t *nodedata = get_node_private_data(c);
    cs_init(c);
    nodedata->modulation = modulation;
}

int get_modulation_bit_per_symbol(call_t *c){
    radio_t *nodedata = get_node_private_data(c);
    return modulation_bit_per_symbol(nodedata->modulation);
}

uint64_t get_Tb(call_t *c) {
    radio_t *nodedata = get_node_private_data(c);
    return (nodedata->Ts/modulation_bit_per_symbol(nodedata->modulation));
}

void set_Ts(call_t *c, uint64_t Ts) {
    radio_t *nodedata = get_node_private_data(c);
    cs_init(c);
    nodedata->Ts = Ts;
}

uint64_t get_Ts(call_t *c) {
    radio_t *nodedata = get_node_private_data(c);
    return nodedata->Ts;
}



//-------- SENSITIVITY ---------


double get_sensibility(call_t *c) {
    radio_t *nodedata = get_node_private_data(c);
    return nodedata->mindBm;
}

void set_sensibility(call_t *c, double sensibility) {
    radio_t *nodedata = get_node_private_data(c);
    cs_init(c);
    nodedata->mindBm = sensibility;
}



/* ************************************************** */
//              HEADERS
/* ************************************************** */
int get_header_size(call_t *c) {
    return 0;
}

int get_header_real_size(call_t *c) {
    return 0;
}

int set_header(call_t *c, packet_t *packet, destination_t *dst) {
    return 0;
}




/* ************************************************** */
/* ************************************************** */
radio_methods_t methods = {
    rx, 
    tx, 
    set_header,
    get_header_size,
    get_header_real_size,
    tx_end, 
    cs,
    get_noise,
    get_cs,
    get_power,
    set_power,
    get_channel,
    set_channel,
    get_modulation, 
    set_modulation, 
    get_Tb, 
    get_Ts,
    set_Ts,
    get_sensibility,
    set_sensibility, 
    sleep, 
    wakeup,
    get_modulation_bit_per_symbol
};
