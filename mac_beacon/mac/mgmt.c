 /*
 *  mgmt.c
 *  
 *
 *  Created by Fabrice Theoleyre on 11/02/11.
 *  Copyright 2011 CNRS. All rights reserved.
 *  Obj: association / data-requests / acks
 *
 */

#include "mgmt.h"





//-------------------------------------------
//			ASSOCIATION REQ
//-------------------------------------------



//we received one ASSOCIATION_REQUEST -> ack it immediately (I will send an association reply later)
void mgmt_assoc_req_rx(call_t *c, packet_t *cmd_pk){
	nodedata_info	*nodedata	= get_node_private_data(c);
	_802_15_4_ASSOCREQ_header	*header_assocreq= (_802_15_4_ASSOCREQ_header *) (cmd_pk->data + sizeof(_802_15_4_header));
	
	//unicast only with 16 bits addr
	if (header_assocreq->dst != nodedata->addr_short)
		return;
	
	tools_debug_print(c, "[ASSOC] association req received from %llu (short id %hu) -> I send an ack\n", header_assocreq->src, param_addr_long_to_short(header_assocreq->src)); 
	
	//we ack the data request 
	//NB: no pending packet: the node is not yet associated!
	mgmt_ack_tx(c, pk_get_seqnum(cmd_pk), FALSE);
	
	//enqueue the association reply in the download buffer
	mgmt_assoc_rep_create_and_insert_in_down_buffer(c, header_assocreq->src);	
}


//creates an association request
packet_info* mgmt_assoc_req_create(call_t *c, uint16_t dest){
	nodedata_info	*nodedata	= (nodedata_info*)get_node_private_data(c);
	
	packet_t					*assoc_req_pk	= packet_create(c, sizeof(_802_15_4_header) + sizeof(_802_15_4_ASSOCREQ_header), -1);
	_802_15_4_ASSOCREQ_header	*header_assoc	= (_802_15_4_ASSOCREQ_header*) (assoc_req_pk->data + sizeof(_802_15_4_header));
	
 	//common headers
	pk_set_A(assoc_req_pk, TRUE);		
	pk_set_SAM(assoc_req_pk, AM_ADDR_LONG);
	pk_set_DAM(assoc_req_pk, AM_ADDR_SHORT);
	pk_set_ftype(assoc_req_pk, MAC_COMMAND_PACKET);
	
	//association req headers	
	header_assoc->type_command	= ASSOCIATION_REQUEST;
	header_assoc->src			= nodedata->addr_long;
	header_assoc->dst			= dest;
	
    //role
    tools_debug_print(c, "[CTREE] I am now child for parent %d (associating step)\n", nodedata->parent_addr_current);
    nodedata->sframe.my_current_role = ROLE_CHILD;
	
	//debug
	tools_debug_print(c, "[ASSOC] I (long address %llu) transmit an association-request to %d (subtype %d), pk id %d\n", header_assoc->src, dest, header_assoc->type_command, assoc_req_pk->id);
	
	packet_info		*pk_buffer_elem = (packet_info*) malloc(sizeof(packet_info));
	pk_buffer_elem->packet			= assoc_req_pk;
	pk_buffer_elem->nb_retry		= 0;				//not used for such packet
	pk_buffer_elem->buffered		= FALSE;
	pk_buffer_elem->time_insertion	= get_time();		
	pk_buffer_elem->priority		= PRIO_ASSOC;		//not used
	pk_buffer_elem->time_expiration = get_time();		//not used
    pk_buffer_elem->broadcasted     = FALSE;
	return(pk_buffer_elem);	
}




//-------------------------------------------
//			ASSOCIATION REP
//-------------------------------------------




//we received one ASSOCIATION_REPLY -> I am now connected to the PAN
void mgmt_assoc_rep_rx(call_t *c, packet_t *cmd_pk){
	nodedata_info	*nodedata	= get_node_private_data(c);
	_802_15_4_ASSOCREP_header	*header_assocrep	= (_802_15_4_ASSOCREP_header *) (cmd_pk->data + sizeof(_802_15_4_header));
	
	//radio entity
	call_t c_radio = {get_entity_bindings_down(c)->elts[0], c->node, c->entity};

	//control
	uint64_t ack_size;
	char	msg[50];
	
	//my current parent (if I am receiveing a response, I MUST be in its superframe!)
	parent_info	*pinfo_current = ctree_parent_get_info(nodedata->parents, nodedata->parent_addr_current);

	//unicast only with 16 bits addr
	if (header_assocrep->dst != nodedata->addr_long)
		return;
	
	//neighborhood table
	neigh_table_update_last_rx_for_addr(c, header_assocrep->src);
	
	
	//our data-req is implicitly acked...
	nodedata->mac.NB		= 0;			
	nodedata->mac.nb_retry	= 0;
	
	//desallocate the current packet (no further retransmission is required)
	buffer_dealloc_pk_tx_up_pending(c);	
	
	//ack the reply
	ack_size = mgmt_ack_tx(c, pk_get_seqnum(cmd_pk), FALSE);
	
	//If I am already associated, this means that my previous ack for the association reply failed... -> nothing to do
	if (pinfo_current->associated){
		fsm_change(c, STATE_CHILD_IDLE, get_time() + macMinSIFSPeriod);
		tools_debug_print(c, "[ASSOC-REP] I already received an association-reply. Nothing to do except acknowledging it\n");
		return;			
	}
	
	//association is now effective
	mgmt_assoc_validated(c, cmd_pk);
	tools_debug_print(c, "[ASSOC-REP] I received an association-reply. I am now associated to %d\n", pinfo_current->addr_short);
	
	//I can sleep when the ack has finished (I will maintain my own superframe and behave like an associated node)
	fsm_change(c, STATE_SLEEP, get_time() + ack_size * radio_get_Tb(&c_radio) + aTurnaroundTime);
	tools_debug_print(c, "[SLEEP] come back to the Sleeping state at %s\n", tools_time_to_str(nodedata->fsm.clock, msg, 50));
	
}



//creates an association reply (packaged to be placed in the management buffer)
void mgmt_assoc_rep_create_and_insert_in_down_buffer(call_t *c, uint64_t dest){
	nodedata_info	*nodedata	= (nodedata_info*)get_node_private_data(c);
	
	packet_t					*assoc_rep_pk	= (packet_t*) packet_create(c, sizeof(_802_15_4_header) + sizeof(_802_15_4_ASSOCREP_header), -1);
	_802_15_4_ASSOCREP_header	*header_assoc	= (_802_15_4_ASSOCREP_header*) (assoc_rep_pk->data + sizeof(_802_15_4_header));
	
	//common headers
	pk_set_A(assoc_rep_pk, TRUE);		
	pk_set_SAM(assoc_rep_pk, AM_ADDR_SHORT);
	pk_set_DAM(assoc_rep_pk, AM_ADDR_LONG);
	pk_set_ftype(assoc_rep_pk, MAC_COMMAND_PACKET);
	
	//association rep headers	
	header_assoc->type_command	= ASSOCIATION_RESPONSE;
	header_assoc->src			= nodedata->addr_short;
	header_assoc->dst			= dest;
	header_assoc->addr_assigned	= param_addr_long_to_short(dest);
	header_assoc->status		= 0x00;	//association is succesful (the only one implemented)
	
	//debug
	tools_debug_print(c, "[ASSOC] I insert in my mgmt buffer the association-reply to %llu (short %d) (subtype %d), pk id %d\n", header_assoc->dst, param_addr_long_to_short(header_assoc->dst), header_assoc->type_command, assoc_rep_pk->id);
	
	//the packaged result
	packet_info		*pk_buffer_elem = (packet_info*) malloc(sizeof(packet_info));
	pk_buffer_elem->packet			= assoc_rep_pk;
	pk_buffer_elem->nb_retry		= 0;				//not used for such packet
	pk_buffer_elem->buffered		= TRUE;
	pk_buffer_elem->time_insertion	= get_time();
	pk_buffer_elem->priority		= PRIO_ASSOC;
	pk_buffer_elem->time_expiration = get_time() + macResponseWaitTime + macTransactionPersistenceTime * tools_get_bi_from_bo(nodedata->sframe.BO);
    pk_buffer_elem->broadcasted     = FALSE;
	buffer_insert_pk(c, nodedata->buffer_mgmt, pk_buffer_elem, TIMEOUT_DEFAULT);	
	
}

//-------------------------------------------
//			ASSOCIATION PROCESS
//-------------------------------------------






//this node is now associated
void mgmt_assoc_validated(call_t *c, packet_t *cmd_pk){
	nodedata_info	*nodedata = (nodedata_info*)get_node_private_data(c);
	_802_15_4_ASSOCREP_header	*header_assocrep	= (_802_15_4_ASSOCREP_header *) (cmd_pk->data + sizeof(_802_15_4_header));
	
	
	//my current parent (if I am receiveing a response, I MUST be in its superframe!)
	parent_info	*pinfo_current = ctree_parent_get_info(nodedata->parents, nodedata->parent_addr_current);
	
	// I am now associated
	stats_register_association(c);			
	
	//adds this link as active for routing ------ we use c->node since we use here the ids (we don't care about the short address)
	routing_edge_add_from_mac(c, c->node, pinfo_current->addr_short);
	
    //no current short address -> I was not yet associated
    if (nodedata->addr_short == ADDR_INVALID_16){
        
        tools_debug_print(c, "[NEIGH] I was not yet associated -> new hello packet after this association to notify my parent\n");
        neigh_hello_enqueue(c);
    }
	
	//I have to use the short address assigned by this coordinator
	nodedata->addr_short = header_assocrep->addr_assigned;
	tools_debug_print(c, "[ASSOC] my short address is now %d\n", nodedata->addr_short);
	if (nodedata->addr_short == ADDR_INVALID_16)
		tools_exit(7, c, "ERROR: this association response (no short addr) is not yet implemented\n");
	
	if (header_assocrep->status != 0)
		tools_exit(7, c, "ERROR: this association response (status %d) is not yet implemented\n", header_assocrep->status);
		
	
	//I am now associated to this parent
	pinfo_current->associated = TRUE;
	
	//superframe + short addr -> only if it is the first parent I am associated to
	if (nodedata->node_type != PAN_COORD)
		beacon_slots_change(c);
}




//-------------------------------------------
//			DISASSOCIATION 
//-------------------------------------------

//this parent is not anymore associated 
void mgmt_parent_disassociated(call_t *c, uint16_t addr_short){
	nodedata_info	*nodedata = (nodedata_info*)get_node_private_data(c);
	parent_info		*pinfo_current = ctree_parent_get_info(nodedata->parents, addr_short);
    
	//debug
	tools_debug_print(c, "[TREE] the parent %d is not anymore associated (removed from the parent's list)\n", pinfo_current->addr_short);
	
	//remove the parent
	ctree_parent_remove_addr_short(c, addr_short);
	tools_debug_print(c, "[TREE] %d remaining associated parent(s)\n", ctree_parent_get_nb_associated(nodedata->parents));
	
  	
    //I am now coordinator -> does not change my state
    if (nodedata->sframe.my_current_role == ROLE_COORDINATOR){//sf_slot_current == nodedata->sframe.my_sf_slot){        
        tools_debug_print(c, "[ASSOC] I stay awake since this is my own superframe\n");
        return;
    } 
    //No parent remains -> We now have to associate to another parent
    else if (das_getsize(nodedata->parents) == 0){
        tools_debug_print(c, "[ASSOC] I must discover a new parent\n");
        fsm_change(c, STATE_UNASSOC_WAIT_BEACON, get_time());				
    }
    //this is not our current parent
    else if (nodedata->parent_addr_current != addr_short){
        tools_debug_print(c, "[ASSOC] this is not our current parent, let remove this parent silently\n");
        return;
    }    
    //go sleeping to not participate to its superframe
    else{  
        tools_debug_print(c, "[ASSOC] I can sleep: this parent is not anymore valid\n");
        fsm_change(c, STATE_SLEEP, get_time());        
    }
    
    //I do not follow anymore this parent
	if (nodedata->parent_addr_current == addr_short)
		nodedata->parent_addr_current = ADDR_INVALID_16;
    
	return;	
}


//creates a disassociation notification
packet_info *mgmt_disassoc_notif_create(call_t *c, uint16_t dest){
	nodedata_info	*nodedata	= (nodedata_info*)get_node_private_data(c);
	
	packet_t					*disassoc_pk		= (packet_t*) packet_create(c, sizeof(_802_15_4_header) + sizeof(_802_15_4_DISASSOC_header), -1);
	_802_15_4_DISASSOC_header	*header_disassoc	= (_802_15_4_DISASSOC_header*) (disassoc_pk->data + sizeof(_802_15_4_header));

	
	//BUG: disassociation only initiated by a child (currently)
	if (ctree_parent_get_info(nodedata->parents, dest) == NULL)
		tools_exit(3, c, "%d should be a parent for %d. Disassocaition failed\n", dest, c->node);

	
	//common headers
	pk_set_A(disassoc_pk, TRUE);		
	pk_set_SAM(disassoc_pk, AM_ADDR_SHORT);
	pk_set_DAM(disassoc_pk, AM_ADDR_SHORT);
	pk_set_ftype(disassoc_pk, MAC_COMMAND_PACKET);

	//association rep headers	
	header_disassoc->type_command	= DISASSOCIATION_NOTIFICATION;
	header_disassoc->src			= nodedata->addr_short;
	header_disassoc->dst			= dest;
	header_disassoc->status			= DEVICE_LEAVE;	//asked by the child, the inverse is not yet implemented
	
	//debug
	tools_debug_print(c, "[DISASSOC] I insert in my mgmt buffer the disassociation-notification to %d (subtype %d), pk id %d\n", header_disassoc->dst, header_disassoc->type_command, disassoc_pk->id);
	
	//the packaged result
	packet_info		*pk_buffer_elem = (packet_info*) malloc(sizeof(packet_info));
	pk_buffer_elem->packet			= disassoc_pk;
	pk_buffer_elem->nb_retry		= 0;				//not used for such packet
	pk_buffer_elem->buffered		= FALSE;
	pk_buffer_elem->time_insertion	= get_time();
	pk_buffer_elem->priority		= PRIO_ASSOC;		//not used
	pk_buffer_elem->time_expiration = get_time();		//not used
    pk_buffer_elem->broadcasted     = FALSE;	
    return(pk_buffer_elem);
}



//we received one DISASSOCIATION_NOTIFICATION -> I drop the corresponding source (from my children)
//NB: disassocitation notif is currently driven only by a child (a parent NEVER disassociates with one child, TO BE IMPLEMENTED)
void mgmt_disassoc_notif_rx(call_t *c, packet_t *cmd_pk){
	nodedata_info	*nodedata	= get_node_private_data(c);
	_802_15_4_DISASSOC_header	*header_disassoc	= (_802_15_4_DISASSOC_header *) (cmd_pk->data + sizeof(_802_15_4_header));
	
	//unicast only with 16 bits addr
	if (header_disassoc->dst != nodedata->addr_short)
		return;
	
	//ack the disassociation
	mgmt_ack_tx(c, pk_get_seqnum(cmd_pk), FALSE);
	
	
	//disassociation is now effective
	neigh_table_remove_after_disassoc(c, cmd_pk); 
	tools_debug_print(c, "[DISASSOC-NOTIF] I received an disassociation-notification. I am deleting child %d\n", header_disassoc->src);
	

	//the FSM stays unchanged
}





//------------------------------------
//		DATA - REQUESTS
//------------------------------------



//we received one DATA_REQUEST -> searches a data packet to send
void mgmt_data_req_rx(call_t *c, packet_t *cmd_pk){
	nodedata_info	*nodedata		= get_node_private_data(c);

	_802_15_4_COMMAND_header			*header_cmd			= (_802_15_4_COMMAND_header *)  (cmd_pk->data + sizeof(_802_15_4_header));
	_802_15_4_COMMAND_long_src_header	*header_cmd_long_src= (_802_15_4_COMMAND_long_src_header *)  (cmd_pk->data + sizeof(_802_15_4_header));
		
	//radio entity
	call_t c_radio = {get_entity_bindings_down(c)->elts[0], c->node, c->entity};
	
	//control
	uint64_t ack_size;

	//an on-going transmission exists -> drop the new request
	if (nodedata->fsm.state != STATE_COORD_IDLE)
		return;			
	
	
	//A packet to ack exists -> the transmission failed!
	if (nodedata->pk_tx_down_pending != NULL){
		
		//debug
		tools_debug_print(c, "[RETX] we receive a frame although we have a pending packet. The previous transmission failed\n"); 
		
		//flush the current pointer (but the corresponding packet remains present in the buffer!)
		buffer_change_pk_tx_down_pending(c, NULL);
	}
	
	
	//am I the destination? (the destination is always a short address)
	if (pk_get_SAM(cmd_pk) == AM_ADDR_SHORT){
		
		//neighborhood table only maintained for short addresses
		neigh_table_update_last_rx_for_addr(c, header_cmd->src);
		
		if (header_cmd->dst != nodedata->addr_short){
			tools_debug_print(c, "[MGMT] packet is not for me, short addr (%d != %d)\n", header_cmd->dst, nodedata->addr_short);
			return;				
		}
	}	
	else{
		//pk_print_content(c, cmd_pk, stdout);
		
		if (header_cmd_long_src->dst != nodedata->addr_short){
			tools_debug_print(c, "[MGMT] packet is not for me, long addr (%d != %d)\n", header_cmd_long_src->dst, nodedata->addr_short);
			return;				
		}					
	}	
	
	//what is the short address of the source?
	uint16_t	addr_short;
	if (pk_get_SAM(cmd_pk) == AM_ADDR_SHORT)
		addr_short = header_cmd->src;
	else
		addr_short = param_addr_long_to_short(header_cmd_long_src->src);
	
	
	//error when the source is a long address and we don't have any management frame to send!
	//NB: NOT STANDARD COMPLIANT (we should send a null frame, but this has no real interest!)
	if ((buffer_get_first_pending_frame_for_addr_short(c, nodedata->buffer_mgmt, addr_short, "MGMT") == NULL)  && (pk_get_SAM(cmd_pk) == AM_ADDR_LONG)){
		
		tools_debug_print(c, "[ASSOC] the association-response was removed from the buffer probably. We generate a new one for %d\n", addr_short);
		mgmt_assoc_rep_create_and_insert_in_down_buffer(c, header_cmd_long_src->src);		
		

	}	
	//pop the first packet from the management buffer
	buffer_change_pk_tx_down_pending(c, buffer_get_first_pending_frame_for_addr_short(c, nodedata->buffer_mgmt, addr_short, "MGMT"));
	
	
	
	//If MGMT buffer is empty, let pop a DATA packet!
	if (nodedata->pk_tx_down_pending == NULL){
		tools_debug_print(c, "[DATA] data packet popped from the buffer\n");
		buffer_change_pk_tx_down_pending(c, buffer_get_first_pending_frame_for_addr_short(c, nodedata->buffer_data_down, addr_short, "DATA-DOWN"));				
	}
	else
		tools_debug_print(c, "[MGMT] management frame popped from the buffer\n");

	
	
	//we ack the data request (the data frame is pending after the ack transmission if the buffer is not empty!)
	ack_size = mgmt_ack_tx(c, pk_get_seqnum(cmd_pk), nodedata->pk_tx_down_pending != NULL);
	
	//no packet to send -> generate a 'false' data frame (zero length payload)
	if (nodedata->pk_tx_down_pending == NULL){
		tools_debug_print(c, "[DATA-BUFFER] NO PACKET FOR %d found\n", addr_short);		
		buffer_change_pk_tx_down_pending(c, data_create_pk_with_zero_payload(c, addr_short));
	}
	else
		tools_debug_print(c, "[DATA_REQ] data req received -> I send an ack\n"); 
	
	
	//When the ack has been finished to be tx, we will send the data/mgmt frame
	fsm_change(c, STATE_COORD_DATA_TX, get_time() +	ack_size * radio_get_Tb(&c_radio) + aTurnaroundTime);					
	
}



//create a pull management frame to retrieve our packet
packet_info		*mgmt_data_request_create(call_t *c, uint16_t dest){
	nodedata_info	*nodedata = (nodedata_info*)get_node_private_data(c);

	packet_t					*pull_pk = packet_create(c, sizeof(_802_15_4_header) + sizeof(_802_15_4_COMMAND_header), -1);
	_802_15_4_COMMAND_header	*header_cmd		= (_802_15_4_COMMAND_header*) (pull_pk->data + sizeof(_802_15_4_header));
		
	//common headers
	pk_set_ftype(pull_pk, MAC_COMMAND_PACKET);
	pk_set_A(pull_pk, TRUE);
	pk_set_SAM(pull_pk, AM_ADDR_SHORT);
	pk_set_DAM(pull_pk, AM_ADDR_SHORT);
	
	//pull specific
	header_cmd->type_command	= DATA_REQUEST;
	header_cmd->src				= nodedata->addr_short;
	header_cmd->dst				= dest;
		
	tools_debug_print(c, "[DATA_REQ] I transmit a data-request with a short addr to %d (subtype %d), pk id %d\n", header_cmd->dst, header_cmd->type_command, pull_pk->id); 

	//creation of metadata
	packet_info		*pk_buffer_elem = (packet_info*) malloc(sizeof(packet_info));
	pk_buffer_elem->packet			= pull_pk;
	pk_buffer_elem->nb_retry		= 0;				//not used for such packet
	pk_buffer_elem->buffered		= FALSE;
	pk_buffer_elem->time_insertion	= get_time();
	pk_buffer_elem->priority		= PRIO_ASSOC;		//not used
	pk_buffer_elem->time_expiration = get_time();		//not used
	pk_buffer_elem->broadcasted     = FALSE;
    return(pk_buffer_elem);
}


//create a pull management frame to retrieve our packet (assoc-rep)
packet_info *mgmt_data_request_long_src_create(call_t *c, uint16_t dest){
	nodedata_info	*nodedata = (nodedata_info*)get_node_private_data(c);
	
	packet_t							*pull_pk = packet_create(c, sizeof(_802_15_4_header) + sizeof(_802_15_4_COMMAND_long_src_header), -1);
	_802_15_4_COMMAND_long_src_header	*header_cmd		= (_802_15_4_COMMAND_long_src_header*) (pull_pk->data + sizeof(_802_15_4_header));
	
	//common headers
	pk_set_ftype(pull_pk, MAC_COMMAND_PACKET);
	pk_set_A(pull_pk, TRUE);
	pk_set_SAM(pull_pk, AM_ADDR_LONG);
	pk_set_DAM(pull_pk, AM_ADDR_SHORT);
	
	//pull specific
	header_cmd->type_command	= DATA_REQUEST;
	header_cmd->src				= nodedata->addr_long;
	header_cmd->dst				= dest;
	
	tools_debug_print(c, "[DATA_REQ] I transmit a data-request with a long src to %d (subtype %d), pk id %d\n", header_cmd->dst, header_cmd->type_command, pull_pk->id); 
	
	//creation of metadata
	packet_info		*pk_buffer_elem = (packet_info*) malloc(sizeof(packet_info));
	pk_buffer_elem->packet			= pull_pk;
	pk_buffer_elem->nb_retry		= 0;				//not used for such packet
	pk_buffer_elem->buffered		= FALSE;
	pk_buffer_elem->time_insertion	= get_time();
	pk_buffer_elem->priority		= PRIO_ASSOC;		//not used
	pk_buffer_elem->time_expiration = get_time();		//not used
    pk_buffer_elem->broadcasted     = FALSE;
	return(pk_buffer_elem);
}




//----------------------------------------------
//				ACKS
//----------------------------------------------

//returns the size of an ack
int mgmt_ack_get_size(){
	return(sizeof(_802_15_4_header));
}


//I have to transmit one ack  (returns the ack packet size)
int mgmt_ack_tx(call_t *c, uint8_t seq_num, short pending_frame) {
	nodedata_info	*nodedata		= get_node_private_data(c);
	packet_t		*packet_new		= packet_create(c, sizeof(_802_15_4_header), -1);
	call_t          c_radio = {get_entity_bindings_down(c)->elts[0], c->node, c->entity};
    
	//packet size
	int size = packet_new->real_size;
	
	//some special fields
	pk_set_ftype(packet_new, MAC_ACK_PACKET);
	pk_set_A(packet_new, FALSE);
	pk_set_P(packet_new, pending_frame);				 
	pk_set_seqnum(packet_new, seq_num);
	
	//transmission time
    uint64_t tx_time = packet_new->real_size * radio_get_Tb(&c_radio);

    //transmits the ack (we should have enough time to do it: the receiver has verified it)
    tools_debug_print(c, "[ACK] ack transmitted for seq num %d (PF %d), pk id %d\n", seq_num, pending_frame, packet_new->id);	
    scheduler_add_callback(get_time() + aTurnaroundTime, c, radio_pk_tx, (void*)packet_new);

    
	//Sleeping state -> idle (I was probably trying to transmit a data-req. Since I received the data packet, it is useless to sleep here) 
	if (nodedata->fsm.state == STATE_SLEEP){
		tools_debug_print(c, "[ACK] I come back to the idle state!\n");
		fsm_change(c, STATE_CHILD_IDLE, get_time());
	}

	//packet size returned
	return(size);
}



//I received one ack
void mgmt_ack_rx(call_t *c, packet_t *packet) {
	nodedata_info				*nodedata		= (nodedata_info*)get_node_private_data(c);

	
	cbrv3_packet_header	*header_cbr;
	char	msg[150];
	
	
	//I have no such sequence number to be acknowledged
	if (nodedata->last_seq_num_to_ack != pk_get_seqnum(packet)){
		tools_debug_print(c, "[MAC_RX_ACK] we are not the destination (last tx %d, rcvd %d)\n", nodedata->last_seq_num_to_ack, pk_get_seqnum(packet));
		return;	
	}
		
	
	//pending frames (I am not the coordinator for this node!)
	if (nodedata->sframe.my_sf_slot != tools_compute_sfslot(get_time()))
        ctree_parent_change_notif(c, nodedata->parent_addr_current, pk_get_P(packet));

	
	//I come back to the idle state (it depends if I am in MY or MY_PARENT superframe)
	switch (nodedata->fsm.state){
		
		//I sent one packet -> I just have to remove it from the pending buffer (the child will solicit me if required)
		case STATE_COORD_WAIT_ACK :
			
			//I will wait for other frames
			fsm_change(c, STATE_COORD_IDLE, get_time());

			//debug
			tools_debug_print(c, "[MAC_RX_ACK] packet removed from the buffer\n");
			
			
			//This is a neighbor
			neigh_table_add_child_after_assoc(c, nodedata->pk_tx_down_pending->packet);		
			
			//packet header
  			_802_15_4_DATA_header	*header_data	= (_802_15_4_DATA_header *) (nodedata->pk_tx_down_pending->packet->data + sizeof(_802_15_4_header));
			//printf("\tNode %d received ack from, dest %d\n", c->node, header_data->dst);
					
			//for ETX ELT - count the no of ACKs received
			etx_elt_t *etx_elt; 								
			das_init_traverse(nodedata->neigh_retransmit);
			while((etx_elt = (etx_elt_t*) das_traverse(nodedata->neigh_retransmit)) != NULL){
				if (etx_elt->neighbor == header_data->dst) {
					etx_elt->acks++;
					nodedata->last_ack_time = get_time();
				}			
			}	
			
			//remove the packet from the correct buffer
			dequeue_desalloc_tx_down_pending_packet(c);
			
			//the retransmission has to be reset for the next frame!
			//NB: direction may change the next time (I will follow my parent)
			nodedata->mac.NB		= 0;
			nodedata->mac.nb_retry	= 0;
			
			break;
		
			
			
		case STATE_CHILD_WAIT_ACK :
			
			//BUG
			if (nodedata->pk_tx_up_pending == NULL){
				tools_debug_print(c, "[ERROR] We MUST have a pending packet in the upload direction\n");
				exit(3);
			}
			
			//the retransmission has to be reset for the next frame!
			nodedata->mac.NB		= 0;
			nodedata->mac.nb_retry	= 0;
			
			//time between two frames (IFS)
			uint64_t	ifs_time;
			if (nodedata->pk_tx_up_pending->packet->real_size > aMaxSIFSFrameSize * 8)
				ifs_time = macMinLIFSPeriod;
			else
				ifs_time = macMinSIFSPeriod;
			
			//I will be authorized to transmit after this IFS (either long or short, it depends on the frame size I sent)
			fsm_change(c, STATE_CHILD_IDLE, get_time() + ifs_time + mactACK);			
	
				
			//particular actions
			switch (pk_get_ftype(nodedata->pk_tx_up_pending->packet)){
				
				//the txed packet was a disassociation-notification -> I can now remove the corresponding parent
				case MAC_COMMAND_PACKET:
					;
					_802_15_4_DISASSOC_header	*header_disassoc	= (_802_15_4_DISASSOC_header*) (nodedata->pk_tx_up_pending->packet->data + sizeof(_802_15_4_header));
					_802_15_4_COMMAND_header	*header_cmd			= (_802_15_4_COMMAND_header*) (nodedata->pk_tx_up_pending->packet->data + sizeof(_802_15_4_header));
					
					if (header_disassoc->type_command == DISASSOCIATION_NOTIFICATION){
						ctree_parent_remove_addr_short(c, header_disassoc->dst);
						tools_debug_print(c, "[TREE] the parent %d is not anymore associated (removed from the parent's list)\n", header_disassoc->dst);
					
						//go sleeping (this is the ongoing frame of our parent => we MUST NOT participate!)
						//overwrite the previous default FSM transition
						nodedata->parent_addr_current = ADDR_INVALID_16;
						fsm_change(c, STATE_SLEEP, get_time());
					}
					
					//data request was acknowleged -> we MUST wait for the following data frame
					else if (header_cmd->type_command == DATA_REQUEST){
						
						uint64_t timeout_data = get_time() + macMaxFrameTotalWaitTime;
						tools_debug_print(c, "[DATA] the command frame was acknowledged, we have now to wait for the data until %s\n", tools_time_to_str(timeout_data, msg, 150));
						
						//I must wait for the response
						fsm_change(c, STATE_CHILD_WAIT_REP, timeout_data);
					}

				break;
					
				//debug
				case MAC_DATA_PACKET :				
					;
					//packet header
  					_802_15_4_DATA_header	*header_data	= (_802_15_4_DATA_header *) (nodedata->pk_tx_up_pending->packet->data + sizeof(_802_15_4_header));
					//printf("\tNode %d received ack from, dest %d\n", c->node, header_data->dst);
					
					//for ETX ELT - count the no of ACKs received
					etx_elt_t *etx_elt; 								
					das_init_traverse(nodedata->neigh_retransmit);
					while((etx_elt = (etx_elt_t*) das_traverse(nodedata->neigh_retransmit)) != NULL){
						if (etx_elt->neighbor == header_data->dst) {
							etx_elt->acks++;
							nodedata->last_ack_time = get_time();
						}			
					}

					header_cbr = (cbrv3_packet_header*) (nodedata->pk_tx_up_pending->packet->data + get_header_size(c));
					tools_debug_print(c, "[MAC_RX_ACK] packet removed from the buffer (seq data %d)\n", header_cbr->sequence);
					
					break;
				
				//info for the general case
				default: 
					tools_debug_print(c, "[MAC_RX_ACK] command packet acknowledged\n");
					
				break;
			}
			
	
			//desallocate the current packet (no further retransmission is required)
			buffer_dealloc_pk_tx_up_pending(c);
			
			
			break;
			
		//Our association req was acked -> we have now to send a data-req to get the association reply
		case STATE_UNASSOC_WAIT_ACK_ASSOC_REQ:			
            
			//time before going back to the idle state to transmit our data-req
			if (ctree_parent_is_associated(nodedata->parents))
                fsm_change(c, STATE_SLEEP, get_time() + macResponseWaitTime);
			else 
                fsm_change(c, STATE_SLEEP, get_time());

			//debug
			tools_debug_print(c, "[MAC_RX_ACK] Our association request was acknowledged, will enter in the idle state at %s\n", tools_time_to_str(nodedata->fsm.clock, msg, 50));			
			ctree_parent_print_debug_info(c);
			
			//desallocate the current packet (no further retransmission is required)
			buffer_dealloc_pk_tx_up_pending(c);
			
			//the retransmission has to be reset for the next frame!
			nodedata->mac.NB		= 0;
			nodedata->mac.nb_retry	= 0;		
			
			break;
		
		//our data-req was acked -> nothing to do (the association reply follows)
		case STATE_UNASSOC_WAIT_ASSOC_REP:
			
			//desallocate the current packet (no further retransmission is required)
			buffer_dealloc_pk_tx_up_pending(c);
			
			//the retransmission has to be reset for the next frame!
			nodedata->mac.NB		= 0;
			nodedata->mac.nb_retry	= 0;		
			
			//time before going back to the idle state to retransmit our data-req
			fsm_change(c, STATE_SLEEP, get_time() + macResponseWaitTime);
				
			tools_debug_print(c, "[MAC_RX_ACK] Our data-request was acknowledged, the association reply will surely follow\n");			
			//NB, else it behaves like an ack loss (that's the same)
		break;
			
			
			
		default: 			
			tools_exit(4, c, "INCORRECT STATE FOR RECEIVING AN ACK\n");

	}



	return;
}


//----------------------------------------------
//			MULTICAST-REQUEST
//----------------------------------------------


//create a request packet to receive a multicast
packet_info *mgmt_multicast_req_pk_create(call_t *c, uint16_t dest){
    //control
    int             i;
	nodedata_info   *nodedata = (nodedata_info*) get_node_private_data(c);
	seqnum_beacon_info  *seqnum_elem;
    
    //seqnums for each multicast address
    seqnum_beacon_info *seqnums_for_multicastreq = das_create();
    mgmt_seqnum_compute_list_for_multicastreq(c, dest, &seqnums_for_multicastreq);
    int nb_multicast = das_getsize(seqnums_for_multicastreq);
    
    //packet creation
    packet_t *multireq_pk	= packet_create(c, 
                                            sizeof(_802_15_4_header) +
                                            sizeof(_802_15_4_MULTIREQ_header) + 
                                            (int)ceil(nb_multicast * (sizeof(uint8_t) * 2 + 0.5))
                                                      , -1);
    _802_15_4_MULTIREQ_header    *hdr_multireq    = (_802_15_4_MULTIREQ_header*) (multireq_pk->data + sizeof(_802_15_4_header));
    
    
	//common headers
	pk_set_A(multireq_pk, FALSE);
	pk_set_SAM(multireq_pk, AM_ADDR_SHORT);
	pk_set_DAM(multireq_pk, AM_ADDR_SHORT);
	pk_set_ftype(multireq_pk, MAC_COMMAND_PACKET);
	
    //association req headers	
	hdr_multireq->type_command	= MULTICAST_REQUEST;
	hdr_multireq->dst			= dest;
    hdr_multireq->src           = nodedata->addr_short;

    //just temporary implemented as an hack (an unassociated node should use its long address SAM=LONG with a specific packet header)
  	if (nodedata->addr_short == ADDR_INVALID_16)
        hdr_multireq->src		= param_addr_long_to_short(nodedata->addr_long);
    
    //nb pf multicast addresses
    pk_multireq_set_nb_pending_multicast(multireq_pk, nb_multicast);
    
    //info for each multicast address
    das_init_traverse(seqnums_for_multicastreq);
	i = 0;
    tools_debug_print(c, "Compressed ADDR: MIN < < MAX (%d multicast addr)\n", nb_multicast);
    while ((seqnum_elem = (seqnum_beacon_info*) das_traverse(seqnums_for_multicastreq)) != NULL) {
        pk_multireq_set_seqnum_max(multireq_pk, i, seqnum_elem->seqnum_max);
        pk_multireq_set_seqnum_min(multireq_pk, i, seqnum_elem->seqnum_min);
        pk_multireq_set_seqnum_caddr(multireq_pk, i, seqnum_elem->caddr);        
        
        tools_debug_print(c, "%d addr: caddr=%d, seq %d< <%d\n", 
                          i,
                          seqnum_elem->caddr, 
                          seqnum_elem->seqnum_min, 
                          seqnum_elem->seqnum_max);
        
         i++;        
    }    
    
    //creation of metadata
	packet_info		*pk_buffer_elem = (packet_info*) malloc(sizeof(packet_info));
	pk_buffer_elem->packet			= multireq_pk;
	pk_buffer_elem->nb_retry		= 0;				//not used for such packet
	pk_buffer_elem->buffered		= FALSE;            //this packet is not placed in the unicast/multicast buffers
	pk_buffer_elem->time_insertion	= get_time();
	pk_buffer_elem->priority		= PRIO_ASSOC;		//not used
	pk_buffer_elem->time_expiration = get_time();		//not used
    pk_buffer_elem->broadcasted     = FALSE;	
    
    //destroy now the unused memory
    das_destroy(seqnums_for_multicastreq);
  
    
    //pk_print_content(c, multireq_pk, stdout);
    //result
    return(pk_buffer_elem);
}


//I received a multicast request
//returns the time during which the node will send the replies
uint64_t mgmt_multicast_req_rx(call_t *c, packet_t *multireq_pk){
    nodedata_info     *nodedata	= (nodedata_info*)get_node_private_data(c);
    call_t c_radio = {get_entity_bindings_down(c)->elts[0], c->node, c->entity};
 
    //control
    int     i, j;
    int     nb_multicast = pk_multireq_get_nb_pending_multicast(multireq_pk);
    uint8_t caddr;
    uint8_t min, max;
    char    msg[100];
    
    //packet transmission
    packet_info     *pk_elem;
    packet_t        *packet_new;
    
    //the time required to transmit everything
    uint64_t        tx_time = get_time() + mactACK;
    uint64_t        tx_time_next = 0;
        
    //pk_print_content(c, multireq_pk, stdout);
   
     //sends the required multicast packets
    for(i=0; i<nb_multicast; i++){        
        max = pk_multireq_get_seqnum_max(multireq_pk, i);
        min = pk_multireq_get_seqnum_min(multireq_pk, i);
        caddr = pk_multireq_get_seqnum_caddr(multireq_pk, i);        
        
        tools_debug_print(c, "[MULTICAST] ask for addr %d, seqnum between %d & %d\n", caddr, min, max);
        for(j=min; (j<=max) && (tx_time + tx_time_next < nodedata->sframe.end_my_sframe); j++){
             
            //searches for the packet
            pk_elem = buffer_queue_get_seqnum(nodedata->buffer_multicast[caddr], caddr, j);
            if (pk_elem == NULL)
                tools_exit(3, c, "[MULTICAST] we do not find the seqnum %d in the multicast buffer for address %d\n", j, caddr);
            
            //transmission time for this packet
            tx_time_next = pk_elem->packet->real_size * radio_get_Tb(&c_radio);
            if (pk_elem->packet->real_size > aMaxSIFSFrameSize * 8)
                tx_time_next += macMinLIFSPeriod;
            else
                tx_time_next += macMinSIFSPeriod;
   
            
            //transmission of one copy if it fits in the active part of the current superframe
            if (tx_time + tx_time_next < nodedata->sframe.end_my_sframe){ 
                //duplication to maintain one copy in the buffer
                packet_new = packet_clone(pk_elem->packet);
                pk_set_A(packet_new, FALSE);            
                pk_elem->broadcasted = TRUE;
                
                //transmission
                tools_debug_print(c, "[MULTICAST] seqnum %d scheduled at %s (pksize %d)\n", 
                                  j, 
                                  tools_time_to_str(tx_time, msg, 100), 
                                  pk_elem->packet->real_size);
                scheduler_add_callback(tx_time, c, radio_pk_tx, (void*)packet_new); 
                tx_time += tx_time_next;
                
                //I am busy until I finished to transmit these frames
                nodedata->mac.tx_end    = tx_time;
            }

        }  
        if (j != max)
            tools_debug_print(c, "[MULTICAST] seqnum from %d do not fit in the active part of the current superframe. They will be transmitted later\n", j);
    }     
    
    return(tx_time);
}



//----------------------------------------------
//			CBRSEQ MGMT
//----------------------------------------------

//to avoid duplicated packets in flooded packets


//is this packet already received?
short mgmt_cbrseq_rx(call_t *c, uint16_t src, uint64_t cbrseq){
    nodedata_info   *nodedata	= (nodedata_info*)get_node_private_data(c);
    cbrseq_info     *cbrseq_elem;
    
        
    das_init_traverse(nodedata->cbrseq_rx);
	while ((cbrseq_elem = (cbrseq_info*) das_traverse(nodedata->cbrseq_rx)) != NULL) {
        if ((cbrseq_elem->src == src) && (cbrseq_elem->cbrseq == cbrseq))
            return(TRUE);
    }    
    return(FALSE);
}

//this packet is now received
void mgmt_cbrseq_add(call_t *c,  uint16_t src, uint64_t cbrseq){
    nodedata_info       *nodedata	= (nodedata_info*)get_node_private_data(c);
    
    cbrseq_info *cbrseq_elem = malloc(sizeof(cbrseq_info));
    cbrseq_elem->src        = src;
    cbrseq_elem->cbrseq     = cbrseq;        
    das_insert(nodedata->cbrseq_rx, cbrseq_elem);
}




//----------------------------------------------
//			SEQNUM MGMT
//----------------------------------------------

//compute the list of multicast addresses and seqnums which have to be piggybacked in beacons
void mgmt_seqnum_compute_list_for_beacons(call_t *c, seqnum_beacon_info **list){    
    seqnum_beacon_info  *seqnums_for_beacons = *list;
    nodedata_info       *nodedata	= (nodedata_info*)get_node_private_data(c);
    seqnum_beacon_info  *seqnum_elem;
    
    //control
    int             addr;
    packet_info     *pk_elem;
    
    //for each multicast address 
    for(addr=0; addr<ADDR_MULTI_NB; addr++){
        
        //creates the list of multicast seqnums (if the associated buffer is not empty, else we do not have to include this address in our beacons)
        if (buffer_queue_getsize(nodedata->buffer_multicast[addr]) > 0){
            
            
            //info about the multicast address
            seqnum_elem = (seqnum_beacon_info*) malloc(sizeof(seqnum_beacon_info));
            seqnum_elem->caddr = addr;
 
            //min = first packet in the queue
            pk_elem = buffer_queue_get(nodedata->buffer_multicast[addr], 0);
            seqnum_elem->seqnum_min = pk_get_seqnum(pk_elem->packet);            
            
            //max = last packet
            pk_elem = buffer_queue_get(nodedata->buffer_multicast[addr], buffer_queue_getsize(nodedata->buffer_multicast[addr])-1);
            seqnum_elem->seqnum_max = pk_get_seqnum(pk_elem->packet);
            
            //push this seqnum_elem in the list
            das_insert(seqnums_for_beacons, seqnum_elem);
        }        
    }
}




//assign a new sequence number for this destination (each unicast/multicast destination implies a different sequence number list)
uint8_t mgmt_assign_seqnum(call_t *c, uint16_t dest){
    nodedata_info	*nodedata	= (nodedata_info*)get_node_private_data(c);
   
    
    //UNICAST
    if (dest < ADDR_MULTI_MIN){
        nodedata->seqnum_tx_unicast++;
    
        if (nodedata->seqnum_tx_unicast != SEQNUM_INVALID)
            return(nodedata->seqnum_tx_unicast);
        else
            return(nodedata->seqnum_tx_unicast++);
    }
    //MULTICAST
    else{
        nodedata->seqnum_tx_multicast[dest-ADDR_MULTI_MIN]++;    
        tools_debug_print(c, "SEQNUM-ASSIGNMENT for addr %d, seq=%d\n", dest-ADDR_MULTI_MIN, nodedata->seqnum_tx_multicast[dest-ADDR_MULTI_MIN]);
        
        if (nodedata->seqnum_tx_multicast[dest-ADDR_MULTI_MIN] != SEQNUM_INVALID)
            return(nodedata->seqnum_tx_multicast[dest-ADDR_MULTI_MIN]);
        else
            return(nodedata->seqnum_tx_multicast[dest-ADDR_MULTI_MIN]++);

    }
}

//print the content of a list
void mgmt_seqnum_rx_print(call_t *c, seqnum_rx_info* list){
    seqnum_rx_info  *seqnum_rx_elem = NULL; 
    char    msg[150];
    
    tools_debug_print(c, "[SEQNUM] List of seqnums already received\n");
    das_init_traverse(list);	
	while ((seqnum_rx_elem = (seqnum_rx_info*) das_traverse(list)) != NULL) {
        tools_debug_print(c, "%d    %s\n", seqnum_rx_elem->seqnum, tools_time_to_str(seqnum_rx_elem->time, msg, 150));
    }    
}

//was this seqnum already received?
short mgmt_seqnum_rx(seqnum_rx_info *list, uint8_t seqnum){
    seqnum_rx_info  *seqnum_rx_elem;    
    
    das_init_traverse(list);	
	while ((seqnum_rx_elem = (seqnum_rx_info*) das_traverse(list)) != NULL) {
        if (seqnum_rx_elem->seqnum == seqnum)
            return(TRUE);
    }
    return(FALSE);        
}


//is this entry obsolete?
int mgmt_seqnum_compare(void* elem, void* arg){
    if (((seqnum_rx_info*)elem)->time < get_time())
        return(1);
    return(0);
}


//remove the timeouted multicast
//executed periodically to limit the number of executions
int mgmt_seqnum_remove_timeout(call_t *c, void *arg){
 	nodedata_info	*nodedata	= (nodedata_info*)get_node_private_data(c);
  
    //control
    uint8_t         addr;
    neigh_info      *neigh_elem;
    
    //for each neighbor
    das_init_traverse(nodedata->neigh_table);	
	while ((neigh_elem = (neigh_info*) das_traverse(nodedata->neigh_table)) != NULL) {

        //for each multicast address, remove obsolete entries
        for(addr=0; addr < ADDR_MULTI_NB; addr++)            
            das_selective_delete(neigh_elem->seqnum_rx[addr], mgmt_seqnum_compare, NULL);
     }    
    
    //schedule the next verification
    scheduler_add_callback(get_time() + TIMEOUT_SEQNUM_VERIF * tools_get_bi_from_bo(nodedata->sframe.BO), c, mgmt_seqnum_remove_timeout, NULL);
    return(0);
}

//this seqnum multicast packet is just received
void mgmt_seqnum_rcvd(call_t *c, packet_t *packet){
    nodedata_info	*nodedata	= (nodedata_info*)get_node_private_data(c);
    
    //control
    seqnum_rx_info  *seqnum_rx_elem = NULL; 
    uint8_t         caddr = pk_get_dst_short(packet) - ADDR_MULTI_MIN;
    neigh_info      *neigh_elem;

    //No neighbor may exist in the neigh-table (e.g. the creation for the hello reception is handled after mgmt_seqnum_rcvd())
    if ((neigh_elem = neigh_table_get(c, pk_get_src_short(packet))) == NULL){
        neigh_elem = neigh_create_unitialized_neigh(pk_get_src_short(packet));
        das_insert(nodedata->neigh_table, neigh_elem);       
        
        //pk_print_content(c, packet, stdout);
        //tools_exit(10,c, "The neighbor %d MUST already exist in the neighborhood table when I update a seqnum for it\n", pk_get_src_short(packet));
        
    }
    
    //timeouts
    uint64_t    timeout;
    if (pk_get_ftype(packet) == MAC_HELLO_PACKET)
        timeout = TIMEOUT_INFINITY;
    else
        timeout = get_time() + macTransactionPersistenceTime * tools_get_bi_from_bo(nodedata->sframe.BO);

 
    
    //insert a new seqnum in the list
    if ((pk_get_ftype(packet) != MAC_HELLO_PACKET) || (das_getsize(neigh_elem->seqnum_rx[pk_get_dst_short(packet)-ADDR_MULTI_MIN])==0)){        
        seqnum_rx_elem = malloc(sizeof(seqnum_rx_info));
        das_insert(neigh_elem->seqnum_rx[caddr], seqnum_rx_elem);
        tools_debug_print(c, "seqnum creation\n");
    }
    //hellos -> particular case, we maintain one single seqnum if at least one exists (and replace the seqnum advertised in beacons)
    else{        
        das_init_traverse(neigh_elem->seqnum_rx[caddr]);
        seqnum_rx_elem = (seqnum_rx_info*) das_traverse(neigh_elem->seqnum_rx[caddr]);
        
        neigh_elem->seqnum_beacons[caddr].seqnum_min = pk_get_seqnum(packet);
        neigh_elem->seqnum_beacons[caddr].seqnum_max = pk_get_seqnum(packet);
    }
    
    //BUG
    if (seqnum_rx_elem == NULL)
        tools_exit(99, c, "seqnum_rx_elem cannot be NULL\n");
    
    //info
    seqnum_rx_elem->seqnum  = pk_get_seqnum(packet);
    seqnum_rx_elem->time    = timeout;
    
    //other pending multicast packets?
    tools_debug_print(c, "[SSEQNUM] seqnum %d, neighbor %d is now received, caddr %d\n", seqnum_rx_elem->seqnum, pk_get_src_short(packet), caddr);   
    
    //update the pending multicast flag
    neigh_elem->multicast_to_rx = mgmt_seqnum_is_multicast_pending_for(c, pk_get_src_short(packet));
   
    //I am a coordinator in this superframe -> nothing to do       
    if (nodedata->sframe.my_current_role == ROLE_COORDINATOR){
    }
     
    //I have not anymore packets to reveive and I am not a child in this superframe -> go sleeping!
    else if ((!neigh_elem->multicast_to_rx) && 
             ((ctree_parent_get_info(nodedata->parents, pk_get_src_short(packet)) == NULL)
             ||
             (nodedata->parent_addr_current != pk_get_src_short(packet))
             )){
        tools_debug_print(c, "[MULTICAST] we do not have anymore packets to retrieve in this superframe and we are not a child -> go sleeping\n");
       
        buffer_dealloc_pk_tx_up_pending(c);        
        fsm_change(c, STATE_SLEEP, get_time());
    }
             
    //go to the idle state
    else if (!neigh_elem->multicast_to_rx){
        tools_debug_print(c, "[MULTICAST] we do not have anymore multicast packets to retrieve from our parent %d -> go to the IDLE state\n", pk_get_src_short(packet));
        
        buffer_dealloc_pk_tx_up_pending(c);        
        fsm_change(c, STATE_CHILD_IDLE, get_time());
        
        //cancel the event ending the beacon listening
        if (nodedata->sframe.event_beacon_stop != NULL)
            scheduler_delete_callback(c, nodedata->sframe.event_beacon_stop);
        nodedata->sframe.event_beacon_stop = NULL;      
    }
}



//is min < value < max
//NB: this may be cyclic (min may be superior to max)
short mgmt_seqnum_is_between(uint8_t value, uint8_t min, uint8_t max){
    
    if (min <= max)
        return(value >= min && value <= max);
    
    else
        return(value <= max || value >= min);
}


//does a multicast packet be retrieved from addr?
short mgmt_seqnum_is_multicast_pending_for(call_t *c, uint16_t addr_short){
    //control
    uint8_t         j;      //j++ with j=255 -> j=0
    neigh_info      *neigh_elem;
    uint8_t         caddr, min, max;
    short         pending = FALSE;
	nodedata_info	*nodedata	= (nodedata_info*)get_node_private_data(c);
    
    //corresponding node
    neigh_elem = neigh_table_get(c, addr_short);
    
    if (neigh_elem == NULL){
        neigh_table_print(c);
        tools_exit(5, c, "The Neighbor %d MUST exist in my neighborhood table before I can verify it has pending packets for me\n", addr_short);
    }
    
    //I am a coordinator -> I CANNOT send a multicast-request to the source
    if (nodedata->sframe.my_current_role == ROLE_COORDINATOR)
        return(FALSE);
   
    //for each possible caddr
    for(caddr=0; caddr<ADDR_MULTI_NB; caddr++){         
        min = neigh_elem->seqnum_beacons[caddr].seqnum_min;
        max = neigh_elem->seqnum_beacons[caddr].seqnum_max;
        
        //are each of these sequence numbers already received?
        if (min != SEQNUM_INVALID)
            for(j=min; !pending && mgmt_seqnum_is_between(j, min, max); j++)           
                if ((j != SEQNUM_INVALID) && (!mgmt_seqnum_rx(neigh_elem->seqnum_rx[caddr], j))){
                        
                    pending = TRUE;                
                    tools_debug_print(c, "[SEQNUM] we still must ask for compressed multicast addr %d, seqnum %d from %d\n", caddr, j, addr_short);
                    //mgmt_seqnum_rx_print(c, neigh_elem->seqnum_rx[caddr]);
                }
        } 
    return(pending);
}


//Must this seqnum for caddr be received from addr_from?
short mgmt_seqnum_to_get_from(call_t *c, uint16_t addr_from, uint16_t caddr){
   
    if (
        (caddr == ADDR_MULTI_DISC - ADDR_MULTI_MIN) ||
        (caddr == ADDR_MULTI_DISC - ADDR_MULTI_MIN) || 
        (caddr == ADDR_MULTI_FLOOD_DISC - ADDR_MULTI_MIN) ||
        (caddr == ADDR_MULTI_HELLO - ADDR_MULTI_MIN)
        )
        return(TRUE);

    tools_debug_print(c, "[NEIGH] node %d ctree neigh %d\n", addr_from, neigh_is_ctree_neigh(c, addr_from));    
    return(neigh_is_ctree_neigh(c, addr_from));
}
    
//updates the list of pending seqnums from this source 
void mgmt_seqnum_update_from_beacon(call_t *c, packet_t *beacon_pk){
    //control
    int         i;
    uint8_t     caddr;
    neigh_info  *neigh_elem;
           
    //packet headers
    _802_15_4_Beacon_header *header_beacon = (_802_15_4_Beacon_header *) (beacon_pk->data + sizeof (_802_15_4_header));
    
    //search for the corresponding neighbor
    neigh_elem = neigh_table_get(c, header_beacon->src);
    
    //BUG
    if (neigh_elem == NULL)
        tools_exit(10, c, "BUG: a neighbor must exist in the neigh table before updating the seqnums\n");
    
    
    //re-intiialization of previous info
    for(caddr=0; caddr<ADDR_MULTI_NB; caddr++){
        neigh_elem->seqnum_beacons[caddr].seqnum_min    = SEQNUM_INVALID;
        neigh_elem->seqnum_beacons[caddr].seqnum_max    = SEQNUM_INVALID;
        neigh_elem->seqnum_beacons[caddr].caddr         = caddr;
    }
    neigh_elem->multicast_to_rx = FALSE;
   
     //update the information using the beacon
    for(i=0; i<pk_beacon_get_nb_pending_multicast(beacon_pk); i++){        
        caddr = pk_beacon_get_seqnum_caddr(beacon_pk, i);
       
        //either a ctree-neighbor or a discovery packet
        if (mgmt_seqnum_to_get_from(c, header_beacon->src, caddr)){
            
            neigh_elem->seqnum_beacons[caddr].seqnum_min     = pk_beacon_get_seqnum_min(beacon_pk, i);
            neigh_elem->seqnum_beacons[caddr].seqnum_max     = pk_beacon_get_seqnum_max(beacon_pk, i);
            neigh_elem->seqnum_beacons[caddr].caddr          = caddr;
        
            tools_debug_print(c, "[BEACONS] %d: %d < < %d\n",
                              caddr,
                              neigh_elem->seqnum_beacons[caddr].seqnum_min,
                              neigh_elem->seqnum_beacons[caddr].seqnum_max);      
        }
        else
            tools_debug_print(c, "[NOT FROM NEIGH] %d \n", caddr);
     } 
    neigh_elem->multicast_to_rx = mgmt_seqnum_is_multicast_pending_for(c, header_beacon->src);
}


//compute the list of multicast addresses and seqnums which have to be piggybacked in beacons
void mgmt_seqnum_compute_list_for_multicastreq(call_t *c, uint16_t dest, seqnum_beacon_info **list){    
    //control
    int             addr;
    uint8_t         j;      //j++ with j=255 -> j=0
    uint8_t         min, max;
    
    //the result
    seqnum_beacon_info  *seqnums_for_multireq = *list;
    
     //search for the corresponding neighbor
    neigh_info              *neigh_elem = neigh_table_get(c, dest);
    seqnum_beacon_info      *seqnum_elem;
    
    //BUG
    if (neigh_elem == NULL)
        tools_exit(10, c, "BUG: a neighbor must exist in the neigh table to ask the coresspondig multicast seqnums\n");

    //for each seqnums advertised by this neighbor
    for(addr=0; addr<ADDR_MULTI_NB; addr++)
                
        //stop when the first non received seqnum is found for this address
        if (neigh_elem->seqnum_beacons[addr].seqnum_min != SEQNUM_INVALID){
             
            min = neigh_elem->seqnum_beacons[addr].seqnum_min;
            max = neigh_elem->seqnum_beacons[addr].seqnum_min;
            
            for(j=min; mgmt_seqnum_is_between(j, min, max); j++){
 
                //this seqnum was not found -> this is the min seqnum I will ask for this multicast address
                if (!mgmt_seqnum_rx(neigh_elem->seqnum_rx[addr], j)){
                    seqnum_elem = (seqnum_beacon_info*) malloc(sizeof(seqnum_beacon_info));
                    seqnum_elem->caddr          = addr;
                    seqnum_elem->seqnum_min     = j;
                    seqnum_elem->seqnum_max     = neigh_elem->seqnum_beacons[addr].seqnum_max;                
                    das_insert(seqnums_for_multireq, seqnum_elem);                
                 }
            }
        }
}




//----------------------------------------------
//				COMMANDS RX
//----------------------------------------------


//One command frame was received
void mgmt_cmd_rx(call_t *c, packet_t *cmd_pk) {		
	nodedata_info	*nodedata	= (nodedata_info*)get_node_private_data(c);
	_802_15_4_COMMAND_header	*header_cmd	= (_802_15_4_COMMAND_header *)  (cmd_pk->data + sizeof(_802_15_4_header));
	
    
	switch(header_cmd->type_command){
			
		case DATA_REQUEST:				
            mgmt_data_req_rx(c, cmd_pk);
			return;		
			break;
			
		case ASSOCIATION_REQUEST:
            mgmt_assoc_req_rx(c, cmd_pk);			
			return;
			break;
			
		case ASSOCIATION_RESPONSE:			
			mgmt_assoc_rep_rx(c, cmd_pk);			
			return;
			break;
			
		case DISASSOCIATION_NOTIFICATION:
			mgmt_disassoc_notif_rx(c, cmd_pk);
			return;
			break;
			
        case MULTICAST_REQUEST:
            if (pk_get_dst_short(cmd_pk) == nodedata->addr_short)
                mgmt_multicast_req_rx(c, cmd_pk);
            return;
            break;
        
        default:
			tools_exit(9, c, "This command subtype (%d) was not yet implemented\n", header_cmd->type_command);

	}
	
	tools_exit(4, c, "THIS COMMAND FRAME IS NOT YET IMPLEMENTED\n");
}


