#include "rpl.h"
#include "../wsnet_rpl/rplWSNet.h"


/* ************************************************** */
//						DIS Timer				 
/* ************************************************** */

int dis_timer(call_t *c, void *args){
	nodedata_t *nodedata = get_node_private_data(c);
	
	//We haven't still joined any DAG
	if(rpl_get_dag(nodedata, RPL_ANY_INSTANCE) == NULL){
		send_dis(c, NULL);
		scheduler_add_callback(get_time() + RPL_DIS_INTERVAL * (uint64_t) Second, c, dis_timer, NULL);
	}
	
	return 0;
}

/* ************************************************** */
//						DIO Timer				
/* ************************************************** */

//only reset if not just reset or started 
void rpl_reset_dio_timer(call_t *c, rpl_dag_t *dag, uint64_t delay, uint8_t force){
	nodedata_t *nodedata = get_node_private_data(c);
	
	if(force || dag->dio_intcurrent > dag->dio_intmin) {
		dag->dio_counter	= 0;
		dag->dio_intcurrent = dag->dio_intmin;
		
		//if we have already scheduled a DIO period => delete it and make more recent
		if (dag->dio_timer != NULL){
			scheduler_delete_callback(c, dag->dio_timer);
		}
				
		//set random I in the interval [I/2, I) 
		srand(get_time());
		dag->dio_interval = dag->dio_intcurrent - rand() % (dag->dio_intcurrent / 2); 
		dag->dio_timer = scheduler_add_callback(get_time()+ (dag->dio_interval*1000000), c, dio_timer, NULL);
		
		//printf("!!!!!!!!!!!!Node %d si-a resetat timerul!!!!!!!!!!!!!!!!!!!!!!!!!!!\n", c->node);
		
		//statistics
		//nodedata->reset_trickle++;
		//rpl_time_parent_t *reset_time = malloc(sizeof(rpl_time_parent_t));
		//reset_time->time = get_time();
		//reset_time->parent_id = c->node;
		//das_insert(nodedata->time_reset_trickle, reset_time);
	}
}

//double the interval I for choosing new transmission time
void new_dio_interval(call_t *c, rpl_dag_t *dag){
	//double the interval if we haven't depased the allowed limit	
	if((dag->dio_intcurrent * 2) / dag->dio_intmin < pow(2, dag->dio_intdoubl)){
		dag->dio_intcurrent = dag->dio_intcurrent * 2;	
	}
	else{
	 	dag->dio_intcurrent = pow(2, dag->dio_intdoubl) * dag->dio_intmin;
	}
		
	//set random I in the interval [I/2, I) 
	srand(get_time());
	dag->dio_interval = dag->dio_intcurrent - (rand() % (dag->dio_intcurrent / 2));
}	


//schedule new DIO transmission
int dio_timer(call_t *c, void *args){
	nodedata_t *nodedata = get_node_private_data(c);
	rpl_dag_t *dag = rpl_get_dag(nodedata, RPL_ANY_INSTANCE);
	
	//Execute DIO tentative right now!
	scheduler_add_callback(get_time(), c, send_dio, NULL);
	
	//Get new DIO interval
	new_dio_interval(c, dag);
	
	//Schedule next self-call	
	dag->dio_timer = scheduler_add_callback(get_time()+ (dag->dio_interval*1000000), c, dio_timer, NULL);
	
	return 0;
}

/* ************************************************** */
/*						DAO Timer				*/ 
/* ************************************************** */

//TODO - To be done