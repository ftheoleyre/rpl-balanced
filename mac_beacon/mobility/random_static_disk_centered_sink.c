/**
 *  \file   random_static_disk_centered_sink.c
 *  \brief  no mobility
 *  \author Fabrice Theoleyre
 *  \date   2011
 **/
#include <include/modelutils.h>


/* ************************************************** */
/* ************************************************** */
model_t model =  {
    "No mobility, random position on a disk (centered sink=node 0)",
    "Fabrice Theoleyre",
    "0.1",
    MODELTYPE_MOBILITY, 
    {NULL, 0}
};


/* ************************************************** */
/* ************************************************** */
int init(call_t *c, void *params) {
    return 0;
}

int destroy(call_t *c) {
    return 0;
}


/* ************************************************** */
/* ************************************************** */
int setnode(call_t *c, void *params) {
  //  param_t *param;
	double	distance;
	
	//center of the simulation area
	position_t	center;	
	center.x = get_topology_area()->x / 2;
	center.y = get_topology_area()->y / 2;
	center.z = get_topology_area()->z / 2;
		
	
	//diameter
	double	diameter = 0;
	if (center.x > diameter)
		diameter = center.x;
	if (center.y > diameter)
		diameter = center.y;
	if (center.z > diameter)
		diameter = center.z;
	
	//printf("center %f %f %f\n", center.x, center.y, center.z);
	
	//special case for the sink
	if (c->node == 0){
		get_node_position(c->node)->x = center.x;
		get_node_position(c->node)->y = center.y;
		get_node_position(c->node)->z = center.z;
	}
	else
	//choose another random position while the node is not in the disk
	do{
		
		get_node_position(c->node)->x = get_random_x_position();
		get_node_position(c->node)->y = get_random_y_position();
		get_node_position(c->node)->z = get_random_z_position();
		
		distance = sqrt( pow(get_node_position(c->node)->x  - center.x, 2) + 
						pow(get_node_position(c->node)->y  - center.y, 2) + 
						pow(get_node_position(c->node)->z  - center.z, 2));
		
		//printf("node %d: distance = %f < %f\n", c->node, distance, diameter);
	}while(distance > diameter);
	
	
	

    return 0;
    
// error:
  //  return -1;
}

int unsetnode(call_t *c) {
    return 0;
}


/* ************************************************** */
/* ************************************************** */
int bootstrap(call_t *c) {
    PRINT_REPLAY("mobility %"PRId64" %d %lf %lf %lf\n", get_time(), c->node, 
                 get_node_position(c->node)->x, get_node_position(c->node)->y, 
                 get_node_position(c->node)->z);
    return 0;
}

int ioctl(call_t *c, int option, void *in, void **out) {
    return 0;
}


/* ************************************************** */
/* ************************************************** */
void update_position(call_t *c) {
    return;
}


/* ************************************************** */
/* ************************************************** */
mobility_methods_t methods = {update_position};
