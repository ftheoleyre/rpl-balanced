/**
 *  \file   shadowing.c
 *  \brief  Log-Normal Shadowing propagation model
 *  \author Guillaume Chelius and Elyes Ben Hamida
 *  \date   2007
 **/
#include <include/modelutils.h>


/* ************************************************** */
/* ************************************************** */
model_t model =  {
    "Log-Normal Shadowing propagation model",
    "Fabrice Theoleyre",
    "0.2",
    MODELTYPE_PROPAGATION, 
    {NULL, 0}
};

/* ************************************************** */
/* ************************************************** */
struct entitydata {
    double pathloss;    /* pathloss exponent */
    double deviation;   /* Shadowing deviation (dB) */
    double dist0;       /* reference distance (m) */
    double Pr_dBm0;     /* receiving power at distance dist0 */

    /* optimize computation */
    double factor;
    double last_rxdBm;
    double Pr0;
};


/* ************************************************** */
/* ************************************************** */
int init(call_t *c, void *params) {
    struct entitydata *entitydata = malloc(sizeof(struct entitydata));
    param_t *param;
    double frequency = 868;

    /* default values */
    entitydata->pathloss   = 2.0;
    entitydata->deviation  = 4.0;
    entitydata->dist0      = 1.0;
    entitydata->Pr_dBm0    = 0.0;
    entitydata->last_rxdBm = 9999;
    entitydata->Pr0        = 0.0;

     /* get parameters */
    das_init_traverse(params);
    while ((param = (param_t *) das_traverse(params)) != NULL) {
        
        if (!strcmp(param->key, "pathloss")) {
            if (get_param_distance(param->value, &(entitydata->pathloss))) {
                goto error;
            }
        }
        if (!strcmp(param->key, "frequency_MHz")) {
            if (get_param_double(param->value, &(frequency))) {
                goto error;
            }
        }
        if (!strcmp(param->key, "deviation")) {
            if (get_param_double(param->value, &(entitydata->deviation))) {
                goto error;
            }
        }
        if (!strcmp(param->key, "dist0")) {
            if (get_param_double(param->value, &(entitydata->dist0))) {
                goto error;
            }
          }
        if (!strcmp(param->key, "Pr_dBm0")) {
            if (get_param_double(param->value, &(entitydata->Pr_dBm0))) {
                goto error;
            }
        }
    }
    
    /* update factor */
    entitydata->factor = 300 / (4 * M_PI * frequency);

    set_entity_private_data(c, entitydata);
    return 0;

 error:
    free(entitydata);
    return -1;
}

int destroy(call_t *c) {
    free(get_entity_private_data(c));
    return 0;
}


/* ************************************************** */
/* ************************************************** */
double normal (double avg, double deviation) {
    return (avg + deviation * cos(2*M_PI*get_random_double()) * sqrt(-2.0 * log(get_random_double())));
}



/* ************************************************** */
/* ************************************************** */
double propagation(call_t *c, packet_t *packet, nodeid_t src, nodeid_t dst, double rxdBm) {
    struct entitydata *entitydata = get_entity_private_data(c);
    double dist, powerloss_dbm, X, Pr;
    /*
     *  Pr_dBm(d) = Pr_dBm(d0) - 10 * beta * log10(d/d0) + X
     *
     *  Note: rxdBm = [Pt + Gt + Gr]_dBm, L = 1, and X a normal distributed RV (in dBm)
     *
     *  cf p104-105 ref "Wireless Communications: Principles and Practice", Theodore Rappaport, 1996.
     *
     */
    if (rxdBm != entitydata->last_rxdBm) {
        
        entitydata->Pr0 = dBm2mW(rxdBm + entitydata->Pr_dBm0);
        entitydata->last_rxdBm = rxdBm;
    }
    
    dist = distance(get_node_position(src), get_node_position(dst));

    X = normal(0.0, entitydata->deviation);
    powerloss_dbm = -10.0 * entitydata->pathloss * log10(dist/entitydata->dist0) + X;
    Pr = mW2dBm(entitydata->Pr0) + powerloss_dbm;
    
    
   // printf("POWER - %d->%d - dist=%f - Pr= %f = %f + %f + %f)\n",  \
           src, dst, \
           dist, \
           Pr,\
           mW2dBm(entitydata->Pr0) , \
           -10.0 * entitydata->pathloss * log10(dist/entitydata->dist0),\
           X);
    
    return Pr;
}


/* ************************************************** */
/* ************************************************** */
propagation_methods_t methods = {propagation};
