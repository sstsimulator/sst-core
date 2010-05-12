/*
** $Id: dot.c,v 1.8 2010/04/27 21:33:56 rolf Exp $
**
** Rolf Riesen, March 2010, Sandia National Laboratories
**
*/
#include <stdio.h>
#include "gen.h"
#include "dot.h"


void
dot_header(FILE *dotfile, char *graph_name)
{

    if (dotfile != NULL)   {
	fprintf(dotfile, "//\n");
	fprintf(dotfile, "// Run this using a command similar to this:\n");
	fprintf(dotfile, "//     dot|neato|circo|twopi|fdp -Tps mygraph.dot > mygraph.ps\n");
	fprintf(dotfile, "//\n");
	fprintf(dotfile, "graph \"%s\" {\n", graph_name);
	fprintf(dotfile, "    rankdir=LR;\n");
	fprintf(dotfile, "    node [shape = box];\n");
	fprintf(dotfile, "    // The following can be expensive to compute\n");
	fprintf(dotfile, "    // splines=true;\n");
	fprintf(dotfile, "    // nodesep=0.1;\n");
	fprintf(dotfile, "    // overlap=false;\n");
	/* fprintf(dotfile, "    rotate=90;\n"); */
	fprintf(dotfile, "\n");
    }

}  /* end of dot_header() */



void
dot_body(FILE *dotfile)
{

int n, r, p;
int r1, r2, p1, p2;
char *label;


    if (dotfile == NULL)   {
	return;
    }

    /* List the routers */
    fprintf(dotfile, "   // The routers\n");
    reset_router_list();
    while (next_router(&r))   {
	fprintf(dotfile, "    \"Rt_%06d\";\n", r);
    }

    /* List the nodes (NICs) */
    fprintf(dotfile, "\n");
    fprintf(dotfile, "   // The nodes (NICs)\n");
    reset_nic_list();
    while (next_nic(&n, &r, &p, &label))   {
	fprintf(dotfile, "    \"N_%06d\" [shape = oval];\n", n);
    }

    /* Generate links between nodes and routers */
    fprintf(dotfile, "\n");
    fprintf(dotfile, "   // Nodes (NIC) to router connections\n");
    reset_nic_list();
    while (next_nic(&n, &r, &p, &label))   {
	fprintf(dotfile, "    \"N_%06d\" -- \"Rt_%06d\" [weight = 10, label = \"%s\"];\n",
	    n, r, label);
    }

    /* Generate links between routers */
    fprintf(dotfile, "\n");
    fprintf(dotfile, "   // Router to router links\n");
    reset_link_list();
    while (next_link(&r1, &p1, &r2, &p2, &label))   {
	fprintf(dotfile, "    \"Rt_%06d\" -- \"Rt_%06d\" [weight = 2, label = \"%s\"];\n",
	    r1, r2, label);
    }

}  /* end of dot_body() */



void
dot_footer(FILE *dotfile)
{

    if (dotfile != NULL)   {
	fprintf(dotfile, "}\n");
    }

}  /* end of dot_footer() */
