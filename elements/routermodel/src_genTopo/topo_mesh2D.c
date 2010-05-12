/*
** $Id: topo_mesh2D.c,v 1.5 2010/04/27 23:17:19 rolf Exp $
**
** Rolf Riesen, March 2010, Sandia National Laboratories
**
*/
#include <stdio.h>
#include "gen.h"
#include "topo_mesh2D.h"


void
GenMesh2D(int dimX, int dimY, int doXwrap, int doYwrap)
{

int x, y;
int r;
int me;


    printf("Generating a %d x %d mesh, with ", dimX, dimY);
    if (doXwrap)   {
	printf("X ");
    }
    if (doYwrap)   {
	printf("Y ");
    }

    if (!doXwrap && !doYwrap)   {
	printf("no ");
    }
    printf("wrap-arounds\n");

    /* List the routers */
    for (r= 0; r < dimX * dimY; r++)   {
	gen_router(r, 5);
    }

    /* Generate NICs and links between NICs and routers */
    for (r= 0; r < dimX * dimY; r++)   {
	gen_nic(r, r, 0);
    }


    /* Generate links between routers */
    for (y= 0; y < dimY; y++)   {
	for (x= 0; x < dimX; x++)   {
	    me= y * dimX + x;
	    /* Go East */
	    if ((me + 1) < (dimX * (y + 1)))   {
		gen_link(me, 1, me + 1, 3);
	    } else   {
		/* Wrap around if specified */
		if (doXwrap)   {
		    gen_link(me, 1, y * dimX, 3);
		}
	    }

	    /* Go South */
	    if ((me + dimX) < (dimX * dimY))   {
		gen_link(me, 2, me + dimX, 4);
	    } else   {
		/* Wrap around if specified */
		if (doYwrap)   {
		    gen_link(me, 2, x, 4);
		}
	    }
	}
    }


}  /* end of GenMesh2D() */
