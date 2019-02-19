/*
 *  radio.h
 *  
 *
 *  Created by Fabrice Theoleyre on 11/02/11.
 *  Copyright 2011 CNRS. All rights reserved.
 *
 */



#ifndef __RADIO_H__
#define __RADIO_H__


#include "802154_slotted.h"



//--------------- SLEEPING ------------------

//mode into string
char *radio_mode_to_str(int mode);

//returns the current radio mode
int radio_get_current_mode(call_t *c);

//to swithc the radio on/off
void radio_switch(call_t *c, int mode);



//--------------- CCA ------------------

//for carrier sensing
int radio_check_channel_busy(call_t *c);



//--------------- TRANSMISSION ------------------


//fill some generic fields before the transmission 
//NB: this is a 'stub' function that MUST be called instead of TX for EACH transmission
int radio_pk_tx(call_t *t, void *pk);


#endif

