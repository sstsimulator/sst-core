/*
** $Id: topo_hyper.c,v 1.2 2010/04/28 17:15:50 rolf Exp $
**
** Rolf Riesen, April 2010, Sandia National Laboratories
**
*/
#include <stdio.h>
#include "topo_hyper.h"
#include "gen.h"



void
GenHyper(int num_nodes, int dimension)
{

int r, d;
int bit;
int dest;


    printf("Generating a %d dimensional Hypercube with %d nodes\n",
	dimension, num_nodes);

    /*
    ** One NIC per router. NIC id = Router id. NIC is always
    ** on port 0 of its router.
    */

    /* Gen the routers with D + 1 ports each */
    for (r= 0; r < num_nodes; r++)   {
	gen_router(r, dimension + 1);
    }

    /* Gen the NICs and the link to their routers */
    for (r= 0; r < num_nodes; r++)   {
	gen_nic(r, r, 0);
    }

    /* Gen links between routers */
    for (r= 0; r < num_nodes; r++)   {
	bit= 1;
	for (d= 0; d < dimension; d++)   {
	    dest= r ^ bit;
	    if (dest > r)   {
		gen_link(r, d + 1, r ^ bit, d + 1);
	    }
	    bit= bit << 1;
	}
    }

}  /* end of GenHyper() */
