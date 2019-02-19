/**
 *  \file   routing_ideal.c
 *  \brief  routing in "god mode" without a separated protocol
 *  \author Fabrice Theoleyre
 *  \date   2011
 **/



#include "routing_ideal.h"



//the directed graph (should be a DAG)
igraph_t	*G_ = NULL;


//routes
typedef struct{
	uint16_t	nexthop;
	short		dist;
} route_info;
route_info	*routes_to_sink = NULL;




//----------------------------------------------
//				CONNECTIVITY MEASURES
//----------------------------------------------

//returns the average number of nodes to remove before having a disconnected networks
double routing_get_connectivity_metric_node_removal(call_t *c){
    int         nb_nodes = get_node_count();
    
    //result (nb of removed links)
    int         count = 0;
    
    //edge removal and connection verification
    igraph_integer_t    a;
    short               disconnected = FALSE;
    
    //loops
    int         node;
    int         i;
    
    //graph
    igraph_vs_t     from, to;
 	igraph_matrix_t shortest_routes;
    igraph_t        *G_copy = (igraph_t*)malloc(sizeof(igraph_t));
    
    //routes from the sink to all the nodes (input links, directed graph)
    igraph_vs_1(&from, 0);
    igraph_vs_all(&to);    
   
    
    for(i=0; i<STATS_NB_TESTS_FOR_CONNECTIVITY; i++){
        
        //copy the original graph
        igraph_copy(G_copy, G_);
        disconnected = FALSE;     
        
        //iterative node removal + connectivity test 
        do{ 
            //remove randomly one existing edge
            a = get_random_integer_range(1, igraph_vcount(G_copy)-1);
            routing_node_remove_in_graph(&G_copy, a);
            
            
            //verify the route is not infitnite for at least one node to the sink
            igraph_matrix_init(&shortest_routes, 1, igraph_vcount(G_copy));
            igraph_shortest_paths(G_copy, &shortest_routes, from, to, IGRAPH_IN);
            for(node=1; node<nb_nodes && !disconnected; node++)
                if (igraph_matrix_e(&shortest_routes, 0, node) == IGRAPH_INFINITY){
                    disconnected = TRUE;
                }
                
            
            
            //this link removal is safe (it preserves the connectivity)
            if (!disconnected)
                count++;
            
            igraph_matrix_destroy(&shortest_routes);           
        }while(!disconnected && igraph_vcount(G_copy) > 1);        
        igraph_destroy(G_copy);     
    }
    
    //end
    free(G_copy); 
    
    
    
    //the modified graph  
    return((double) count / STATS_NB_TESTS_FOR_CONNECTIVITY);
}


//returns the average number of links to remove before having a disconnected networks
double routing_get_connectivity_metric_link_removal(call_t *c){
    int         nb_nodes = get_node_count();
    
    //result (nb of removed links)
    int         count = 0;
    
    //edge removal and connection verification
    short       disconnected = FALSE;

    //loops
    int         node;
    int         i;
     
    //graph
    igraph_vs_t     from, to;
	igraph_matrix_t shortest_routes;
    igraph_t        *G_copy = (igraph_t*)malloc(sizeof(igraph_t));

    //routes from the sink to all the nodes (input links, directed graph)
    igraph_vs_1(&from, 0);
    igraph_vs_all(&to);
   
    for(i=0; i<STATS_NB_TESTS_FOR_CONNECTIVITY; i++){
      
        //copy the original graph
        igraph_copy(G_copy, G_);
        disconnected = FALSE;
  
       
         //iterative edge removal + connectivity test
        do{ 
            
            //remove randomly one existing edge
            routing_edge_remove_random_in_graph(&G_copy);
 
            //verify the route is not infitnite for at least one node to the sink
            igraph_matrix_init(&shortest_routes, 1, nb_nodes);
            igraph_shortest_paths(G_copy, &shortest_routes, from, to, IGRAPH_IN);             
            for(node=0; node<nb_nodes; node++)                
                if (igraph_matrix_e(&shortest_routes, 0, node) == IGRAPH_INFINITY)
                    disconnected = TRUE;

            
            //this link removal is safe (it preserves the connectivity)
            if (!disconnected)
                count++;
            
            igraph_matrix_destroy(&shortest_routes);
            
        }while(!disconnected);        
        igraph_destroy(G_copy);
    }
    //end
    free(G_copy); 
    
    //the modified graph
    return((double) count / STATS_NB_TESTS_FOR_CONNECTIVITY);
}




//----------------------------------------------
//				SHORTEST HOP ROUTES
//----------------------------------------------

//Computes the distance to the sink
void routing_construct_shortest_routes_to_sink(call_t *c){
	int		i , j;
	short	at_least_one_route_changed;
	int		nb_nodes = get_node_count();
	
	//memory creation (this first time)
	if (routes_to_sink == NULL)
		routes_to_sink = (route_info*) malloc(sizeof(route_info) * nb_nodes);
	
	
	//initialization
	for(i=0; i<nb_nodes; i++){
		routes_to_sink[i].nexthop	= ADDR_INVALID_16;
		routes_to_sink[i].dist		= HOP_INFINITY;
	}
	routes_to_sink[ID_PAN_COORD].nexthop	= ADDR_INVALID_16;
	routes_to_sink[ID_PAN_COORD].dist		= 0;
	
	
	//updates the distance
	at_least_one_route_changed = TRUE;
	while(at_least_one_route_changed){
		at_least_one_route_changed = FALSE;
		
		//one optimization is found: a node can use a better route than the route it currently has
		for(i=0; i<nb_nodes; i++)
			for(j=0; j<nb_nodes; j++)
			
				//I can use this link ... and the distance is shorter
				if ((routing_edge_exist_in_graph(&G_, i, j)) || (routing_edge_exist_in_graph(&G_, j, i))){
					if ((routes_to_sink[j].dist > routes_to_sink[i].dist + 1) && (routes_to_sink[i].dist < HOP_INFINITY)){
						
						routes_to_sink[j].dist		= routes_to_sink[i].dist + 1;
						routes_to_sink[j].nexthop	= i;
						at_least_one_route_changed	= TRUE;
                    }
				}
	}
	/*printf("routes computed\n");
	for(i=0; i<nb_nodes; i++)
		printf("%d -> %d [...] -> %d (%d hops)\n", i, routes_to_sink[i].nexthop, ID_PAN_COORD, routes_to_sink[i].dist);
	*/
}



//----------------------------------------------
//				GRAPH maintenance
//----------------------------------------------
//does this link exist?
short routing_edge_exist_in_graph(igraph_t **G_ptr, igraph_integer_t node_a, igraph_integer_t node_b){
	igraph_integer_t eid;
	int             code;
 	
	//error
	if ((node_a >= get_node_count()) || (node_b >= get_node_count()))
		tools_exit_short(2, "ERROR: bad link, routing_edge_exist()\n");
   
		
	//error desactivated for igraph
	igraph_set_error_handler(igraph_error_handler_ignore);
	
	//get the eid (directed links, no error if not found)
	code = igraph_get_eid(*G_ptr, &eid, node_a, node_b, FALSE, TRUE);
	//printf("code %d (%d %d %d), eid %d\n", code, IGRAPH_FAILURE, IGRAPH_SUCCESS, IGRAPH_EINVAL, (int) eid);
	
	//error
	if (code != IGRAPH_EINVAL && code != IGRAPH_SUCCESS)
		tools_exit_short(4, "Error in graph_is_link(), unknown error code (%d)\n", code);

	
	//re-activate the erros
	igraph_set_error_handler(igraph_error_handler_abort);
	
	return(code != IGRAPH_EINVAL);	
}


//add an edge for routing (i.e. link can be used to route packets)
//return 1 if the link was really added (none existed)
short routing_edge_add_in_graph(igraph_t **G_ptr, igraph_integer_t node_a, igraph_integer_t node_b){
	int         nb_nodes = get_node_count();
	
	//error
	if ((node_a >= get_node_count()) || (node_b >= get_node_count()))
		tools_exit_short(2, "ERROR: bad link, routing_edge_add(), one of these nodes does not exist\n");

		
	//does the graph already exist?
	if (*G_ptr == NULL){
		*G_ptr = (igraph_t*)malloc(sizeof(igraph_t));
		
		if (igraph_empty(*G_ptr, nb_nodes, TRUE) != 0)              //directed graph
			tools_exit_short(4, "Error in the graph creation\n");
	
	}
 	
	//the link already exists!
	if (routing_edge_exist_in_graph(G_ptr, node_a, node_b))
		return(FALSE);
	
	//insert the edge
	igraph_add_edge(*G_ptr, node_a, node_b);
    return(TRUE);
}

//remove an edge randomly
short routing_edge_remove_random_in_graph(igraph_t **G_ptr){
    igraph_integer_t    eid = get_random_integer_range(0, (int)igraph_ecount(*G_ptr)-1);
    int                 code_ret;
    
    igraph_es_t edges = igraph_ess_1(eid);
    
    igraph_integer_t to, from;
    igraph_edge(*G_ptr,eid, &to, &from);
    //printf("--random suppression: %d %d\n", (int)to, (int)from);

    //remove it
	if ((code_ret = igraph_delete_edges(*G_ptr, edges)) != 0)
		tools_exit_short(2, "ERROR: routing_edge_remove(), failed, code %d\n", code_ret);
    
    return(TRUE);

}

//remove an edge for routing (it is not anymore present)
//return 1 if the link was really removed (it existed)
short routing_edge_remove_in_graph(igraph_t **G_ptr, igraph_integer_t node_a, igraph_integer_t node_b){
	//edge to remove
	igraph_integer_t eid;
	igraph_es_t		edges;
	
	//control
	int				code_ret;
	
	//error
	if ((node_a >= get_node_count()) || (node_b >= get_node_count()))
		tools_exit_short(2, "ERROR: bad link, routing_edge_remove()\n");
	
	
	//the link doesn't exist
	if (!routing_edge_exist_in_graph(G_ptr, node_a, node_b))
		return(FALSE);
    
	//get the eid and constructs the associated set (directed links, no error if not found)
	igraph_get_eid(*G_ptr, &eid, node_a, node_b, FALSE, TRUE);
	edges = igraph_ess_1(eid);
     
	//remove it
	if ((code_ret = igraph_delete_edges(*G_ptr, edges)) != 0)
		tools_exit_short(2, "ERROR: routing_edge_remove(), failed, code %d\n", code_ret);

    return(TRUE);
    
}

//remove a vertex for routing (it is not anymore present)
short routing_node_remove_in_graph(igraph_t **G_ptr, igraph_integer_t node_a){
	igraph_vs_t		vertices = igraph_vss_1(node_a);
	int             code_ret;
    
    if (node_a > igraph_vcount(*G_ptr)){
        printf("unexisting vertex: %d\n", (int)node_a);
        return(FALSE);
    }
    
	//remove it
	if ((code_ret = igraph_delete_vertices(*G_ptr, vertices)) != 0)
		tools_exit_short(2, "ERROR: routing_edge_remove(), failed, code %d\n", code_ret);
    
    return(TRUE);   //this node always exist
}


//----------------------------------------------
//		GRAPH INTERFACE FOR THE MAC
//----------------------------------------------

//add an edge for routing (i.e. link can be used to route packets)
void routing_edge_add_from_mac(call_t *c, uint16_t node_a, uint16_t node_b){
    
    if (routing_edge_add_in_graph(&G_, node_a, node_b)){
        routing_construct_shortest_routes_to_sink(c);
        tools_debug_print(c, "[ROUTING] link %d->%d inserted in the routing DAG\n", node_a, node_b);
    }      

}
//remove an edge for routing (it is not anymore present)
void routing_edge_remove_from_mac(call_t *c, uint16_t node_a, uint16_t node_b){
    
    //particular case
    if (G_ == NULL){
        tools_debug_print(c, "[GRAPH] the edge %d/%d was not removed from the graph since the graph was not inialized (this surely corresponds to a parent with a partial association\n");
        return;
    }
    
    //updates the route if the edge existed and was removed
 	if (routing_edge_remove_in_graph(&G_, node_a, node_b)){
        routing_construct_shortest_routes_to_sink(c);
        tools_debug_print(c, "[ROUTING] link %d->%d removed in the routing DAG\n", node_a, node_b);
    }
}


//----------------------------------------------
//				INFO
//----------------------------------------------



//distance from the sink
short routing_get_depth(uint16_t src){
	
	
	//the routing table does not yet exist (no associated node)
	if (routes_to_sink == NULL)
		return(-1);
		
	
	//walk in the tree toward the root, increasing the depth by one for each walk to one parent
	int			depth;
	uint16_t	current_node = src;
	for(depth=0; (current_node != ID_PAN_COORD) && (current_node != ADDR_INVALID_16); depth++){		
		current_node = routes_to_sink[current_node].nexthop;
	}
	
	//valid if the tree is ok
	if (current_node == ADDR_INVALID_16)
		return(-1);
	else
		return(depth);
}







//----------------------------------------------
//				NEXT HOP
//----------------------------------------------


//returns the next hop toward the corresponding node
//NB: this is a "god mode" -> it follows the tree assuming the tree topology is known a priori 
//(to evaluate the MAC with an ideal routing!!)
uint16_t routing_ideal_get_next_hop(call_t *c, uint16_t dest){
	nodedata_info	*nodedata = get_node_private_data(c);


	//error
	if (dest >= get_node_count())
		tools_exit(2, c, "ERROR: bad link,routing_ideal_get_next_hop()\n");
	
	
	//the routing table does not yet exist (no associated node)
	if (routes_to_sink == NULL){
		tools_debug_print(c, "[ROUTING] the routing table does not exist\n");
		return(ADDR_INVALID_16);
	}

	//I have no short addr (not yet associated) -> exit
	if (nodedata->addr_short == ADDR_INVALID_16){
		tools_debug_print(c, "[ROUTING] I don't have yet a short address\n");
		return(ADDR_INVALID_16);
	}
	
	tools_debug_print(c, "[ROUTING] destination %d\n", dest);
	
	//I walk in the tree until I find my address
	do{	
		tools_debug_print(c, "[ROUTING] link %d -> %d\n", dest, routes_to_sink[dest].nexthop);
		
		//I am the next hop for this ancestor of dest -> I use it as next hop
		if (routes_to_sink[dest].nexthop == nodedata->addr_short)
			return(dest);
		
		//else, I take the parent of this ancestor
		dest = routes_to_sink[dest].nexthop ;		
		
		//this node has no ancestor -> it is not connected to the cluster-tree
		if (dest == ADDR_INVALID_16)
			return(ADDR_INVALID_16);
		
	// if the current pointer is the coordinator (and I am not the coordinator),
	// this means I am not an ancestor of dest -> drop the packet
	} while(dest != ID_PAN_COORD);
	

	return(ADDR_INVALID_16);
}