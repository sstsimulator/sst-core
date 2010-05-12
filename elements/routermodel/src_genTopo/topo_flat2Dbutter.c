/*
** $Id: topo_flat2Dbutter.c,v 1.1 2010/04/28 21:19:51 rolf Exp $
**
** Rolf Riesen, April 2010, Sandia National Laboratories
**
*/
#include <stdio.h>
#include "gen.h"
#include "topo_flat2Dbutter.h"


void
GenFlat2Dbutter(int dimX, int dimY)
{

int x, y;
int i;
int r;
int me;
int num_ports;
int src_port, dst_port;


    num_ports= dimX - 1 + dimY - 1 + 1;
    printf("Generating a flatened 2-D butterfly %d x %d topology\n", dimX, dimY);
    printf("Each router has %d ports including one to the local NIC\n", num_ports);

    /* List the routers */
    for (r= 0; r < dimX * dimY; r++)   {
	gen_router(r, num_ports);
    }

    /* Generate NICs and links between NICs and routers */
    for (r= 0; r < dimX * dimY; r++)   {
	gen_nic(r, r, 0);
    }


    /* Generate links between routers */
    for (y= 0; y < dimY; y++)   {
	for (x= 0; x < dimX; x++)   {
	    me= y * dimX + x;

	    src_port= x + 1;
	    dst_port= x + 1;
	    for (i= x + 1; i < dimX; i++)   {
		/* Connect to each router in the X dimension */
		gen_link(me, src_port, y * dimX + i, dst_port);
		src_port++;
	    }

	    src_port= y + dimX;
	    dst_port= y + dimX;
	    for (i= x + dimX * (y + 1); i < dimX * dimY; i= i + dimX)   {
		/* Connect to each router in the Y dimension */
		gen_link(me, src_port, i, dst_port);
		src_port++;
	    }

	}
    }

}  /* end of GenFlat2Dbutter() */
