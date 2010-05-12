/*
** $Id: topo_full.c,v 1.4 2010/04/27 19:48:31 rolf Exp $
**
** Rolf Riesen, March 2010, Sandia National Laboratories
**
*/
#include <stdio.h>
#include "gen.h"
#include "topo_full.h"


void
GenFull(int num_nodes)
{

int src, dst;
int p1, p2;


    printf("Generating a fully connected graph with %d nodes\n", num_nodes);

    /* List the routers */
    for (src= 0; src < num_nodes; src++)   {
	gen_router(src, num_nodes);
    }

    /* Connect all NICs to port 0 of their router */
    for (src= 0; src < num_nodes; src++)   {
	gen_nic(src, src, 0);
    }

    /* Generate links between routers */
    for (src= 0; src < num_nodes; src++)   {
	for (dst= src + 1; dst < num_nodes; dst++)   {
	    p1= dst;
	    p2= src + 1;
	    gen_link(src, p1, dst, p2);
	}
    }

}  /* end of GenFull() */
