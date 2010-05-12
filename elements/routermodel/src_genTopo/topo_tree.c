/*
** $Id: topo_tree.c,v 1.4 2010/04/27 19:48:31 rolf Exp $
**
** Rolf Riesen, March 2010, Sandia National Laboratories
**
*/
#include <stdio.h>
#include "topo_tree.h"
#include "gen.h"


void
GenTree(int num_nodes, int fat)
{

int i;
int r;
int p1, p2;
int level;
int num_links;
int num_ports;


    /*
    ** We assume num_nodes is a power of 2
    */
    if (fat)   {
	printf("Generating a binary fat tree with %d nodes\n", num_nodes);
	num_ports= num_nodes;
    } else   {
	printf("Generating a binary tree with %d nodes\n", num_nodes);
	num_ports= 3;
    }

    /* List the routers */
    level= 1;
    while (level < num_nodes)   {
	for (r= 0; r < level; r++)   {
	    gen_router(r + (level - 1), num_ports);
	}
	level= level * 2;
    }

    /*
    ** Gen the NICs and the link to their routers
    ** Attach NICs to port 0 and 1 of the bottom routers
    */
    for (r= 0; r < num_nodes; r++)   {
	p1= r % 2;
	gen_nic(r, (r + num_nodes - 2) / 2, p1);
    }

    /* Generate links between routers */
    level= 2;
    num_links= num_nodes / 2;
    while (level < num_nodes)   {
	for (r= 0; r < level; r++)   {
	    if (fat)   {
		for (i= 0; i < num_links ; i++)   {
		    p1= (num_links + i);
		    p2= 2 * i + (((r + (level - 1)) - 1) % 2);
		    gen_link(r + (level - 1), p1, (r + (level - 1) - 1) / 2, p2);
		}
	    } else   {
		/* Going up, we always use port 2 */
		/* Going down we use port 0 or 1 */
		p1= ((r + (level - 1)) + 1) % 2;
		gen_link(r + (level - 1), 2, (r + (level - 1) - 1) / 2, p1);
	    }
	}
	num_links= num_links / 2;
	level= level * 2;
    }

}  /* end of GenTree() */
