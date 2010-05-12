/*
** $Id: topo_ring.c,v 1.4 2010/04/27 19:48:31 rolf Exp $
**
** Rolf Riesen, March 2010, Sandia National Laboratories
**
*/
#include <stdio.h>
#include "topo_ring.h"
#include "gen.h"



void
GenRing(int num_nodes)
{

int r;
int next;


    printf("Generating a Ring with %d nodes\n", num_nodes);

    /*
    ** One NIC per router. NIC id = Router id. NIC is always
    ** on port 0 of its router.
    */

    /* Gen the routers with three ports each */
    for (r= 0; r < num_nodes; r++)   {
	gen_router(r, 3);
    }

    /* Gen the NICs and the link to their routers */
    for (r= 0; r < num_nodes; r++)   {
	gen_nic(r, r, 0);
    }

    /* Gen links between routers */
    for (r= 0; r < num_nodes; r++)   {
	next= (r + 1) % num_nodes;
	gen_link(r, 1, next, 2);
    }

}  /* end of GenRing() */
