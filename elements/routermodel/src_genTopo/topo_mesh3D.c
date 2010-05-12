/*
** $Id: topo_mesh3D.c,v 1.2 2010/04/27 23:17:19 rolf Exp $
**
** Rolf Riesen, April 2010, Sandia National Laboratories
**
*/
#include <stdio.h>
#include "gen.h"
#include "topo_mesh3D.h"


void
GenMesh3D(int dimX, int dimY, int dimZ, int doXwrap, int doYwrap, int doZwrap)
{

int x, y, z;
int r;
int me;


    printf("Generating a %d x %d x %d mesh, with ", dimX, dimY, dimZ);
    if (doXwrap)   {
	printf("X ");
    }
    if (doYwrap)   {
	printf("Y ");
    }
    if (doZwrap)   {
	printf("Z ");
    }

    if (!doXwrap && !doYwrap && !doZwrap)   {
	printf("no ");
    }
    printf("wrap-arounds\n");

    /* List the routers */
    for (r= 0; r < dimX * dimY * dimZ; r++)   {
	gen_router(r, 7);
    }

    /* Generate NICs and links between NICs and routers */
    for (r= 0; r < dimX * dimY * dimZ; r++)   {
	gen_nic(r, r, 0);
    }


    /* Generate links between routers */
    for (z= 0; z < dimZ; z++)   {
	for (y= 0; y < dimY; y++)   {
	    for (x= 0; x < dimX; x++)   {
		me= z * dimY * dimX + y * dimX + x;
		/* Go East */
		if ((me + 1) < (dimX * (y + 1) + dimX * dimY * z))   {
		    gen_link(me, 1, me + 1, 3);
		} else   {
		    /* Wrap around if specified */
		    if (doXwrap)   {
			gen_link(me, 1, (z * dimX * dimY) + y * dimX, 3);
		    }
		}

		/* Go South */
		if ((me + dimX) < (z + 1) * (dimX * dimY))   {
		    gen_link(me, 2, me + dimX, 4);
		} else   {
		    /* Wrap around if specified */
		    if (doYwrap)   {
			gen_link(me, 2, x + z * dimX * dimY, 4);
		    }
		}

		/* Go Back */
		if ((me + dimX * dimY) < (dimX * dimY * dimZ))   {
		    gen_link(me, 5, me + dimX * dimY, 6);
		} else   {
		    /* Wrap around if specified */
		    if (doZwrap)   {
			gen_link(me, 5, x + y * dimX, 6);
		    }
		}
	    }
	}
    }


}  /* end of GenMesh3D() */
