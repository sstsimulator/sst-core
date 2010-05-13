#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "routing.h"

/*
** To Do
** Test whether up* / down* works correctly
** Implement heuristics from Sancho paper
** How well are redundant paths balanced? E.g. fat tree
*/

#define INFINITE	(999999999)


typedef struct NIC_table_t   {
    int router;
    int port;
} NIC_table_t;

typedef struct link_t   {
    int port;
    int in_tree;
    int main_branch;
} link_t;

typedef struct router_t   {
    int level;
    int visited;
    int label;
    int distance;
} router_t;

typedef struct route_info_t   {
    NIC_table_t *NIC_table;
    link_t **Links;
    router_t *Routers;
    int **Router_paths;
    int **NIC_routes;
    int *router_stack;
    int router_stack_pos;
    int max_router;
    int max_NIC;
    int current_label;
    int current_level;
    int on_main_branch;
    int me;
} route_info_t;


/* Local functions */
static void DFS(int v, route_info_t *rinfo);
static int pop_label(route_info_t *rinfo);
static void push_label(int value, route_info_t *rinfo);
static void main_down(int v, route_info_t *rinfo);
static void main_up(int v, route_info_t *rinfo);
static void Label_secondary(int v, route_info_t *rinfo);
static void clear_visited(route_info_t *rinfo);
static void clear_labels(route_info_t *rinfo);
static int down(int src, int dest, route_info_t *rinfo);
static int up(int src, int dest, route_info_t *rinfo);
static void init_NIC_table(int num_NICs, route_info_t *rinfo);



int *
get_route(int dst, void *vrinfo)
{

route_info_t *rinfo;


    rinfo= (route_info_t *)vrinfo;
    if ((dst < 0) || (dst >= rinfo->max_NIC))   {
	return NULL;
    }

    return rinfo->NIC_routes[dst];

}  /* end of get_route() */



static void
init_NIC_table(int num_NICs, route_info_t *rinfo)
{

int i;


    rinfo->NIC_table= (NIC_table_t *)malloc(num_NICs * sizeof(NIC_table_t));
    if (rinfo->NIC_table == NULL)   {
	fprintf(stderr, "Out of memory!\n");
	exit(0);
    }

    for (i= 0; i < num_NICs; i++)   {
	rinfo->NIC_table[i].router= -1;
	rinfo->NIC_table[i].port= -1;
    }

    rinfo->max_NIC= num_NICs;

}  /* end of init_NIC_table() */



int
check_NIC_table(void *vrinfo)
{

int i;
route_info_t *rinfo;


    rinfo= (route_info_t *)vrinfo;
    for (i= 0; i < rinfo->max_NIC; i++)   {
	if ((rinfo->NIC_table[i].router < 0) || (rinfo->NIC_table[i].port < 0))   {
	    fprintf(stderr, "NIC %d not properly initialized: router %d, port %d\n",
		i, rinfo->NIC_table[i].router, rinfo->NIC_table[i].port);
	    return -1;
	}
    }

    return 0;

}  /* end of check_NIC_table() */



void
NIC_table_insert_router(int rank, int router, void *vrinfo)
{

route_info_t *rinfo;


    rinfo= (route_info_t *)vrinfo;
    if (rinfo->NIC_table[rank].router >= 0)   {
	fprintf(stderr, "Already specified a router (%d) for NIC %d\n",
	    rinfo->NIC_table[rank].router, rank);
	exit(0);
    }
    if (router < 0)   {
	fprintf(stderr, "Invalid router ID (%d) for NIC %d\n",
	    router, rank);
	exit(0);
    }

    rinfo->NIC_table[rank].router= router;

}  /* end of NIC_table_insert_rank() */



void
NIC_table_insert_port(int rank, int num_item, void *vrinfo)
{

route_info_t *rinfo;


    rinfo= (route_info_t *)vrinfo;
    if (rinfo->NIC_table[rank].port >= 0)   {
	fprintf(stderr, "Already specified a port (%d) for NIC %d\n",
	    rinfo->NIC_table[rank].port, rank);
	exit(0);
    }
    if (num_item < 0)   {
	fprintf(stderr, "Invalid port number (%d) for NIC %d\n",
	    num_item, rank);
	exit(0);
    }

    rinfo->NIC_table[rank].port= num_item;

}  /* end of NIC_table_insert_rank() */



/*
** Each entry in the table is the exit port number.
** -1 means no connection
*/
void *
init_routing(int num_routers, int num_NICs)
{

int array_size, ptr_size;
link_t *ptr;
int *int_ptr;
int i, x, y;
route_info_t *rinfo;


    /*
    ** Allocate memory for the structure that holds all the routing info
    ** for this NIC.
    */
    rinfo= (route_info_t *)malloc(sizeof(route_info_t));
    if (rinfo == NULL)   {
        fprintf(stderr, "Memory allocation failed! Line %d\n", __LINE__);
        exit(0);
    }
    rinfo->max_router= -1;
    init_NIC_table(num_NICs, rinfo);

    /* Size for the data: n^2 ints */
    array_size= num_routers * num_routers * sizeof(link_t);

    /* Size for the pointers: n pointers to int */
    ptr_size= num_routers * sizeof(link_t *);

    /* Allocate all memory in one chunk */
    rinfo->Links= (link_t **)malloc(array_size + ptr_size);
    if (rinfo->Links == NULL)   {
        fprintf(stderr, "Memory allocation failed! Line %d\n", __LINE__);
        exit(0);
    }

    /* Fill in the pointer array */
    ptr= (link_t *)(rinfo->Links + num_routers);
    for (i= 0; i < num_routers; i++)   {
        rinfo->Links[i]= ptr;
        ptr= ptr + num_routers;
    }

    /* Clear it */
    for (y= 0; y < num_routers; y++)   {
	for (x= 0; x < num_routers; x++)   {
	    rinfo->Links[y][x].port= -1;
	    rinfo->Links[y][x].in_tree= 0;
	    rinfo->Links[y][x].main_branch= 0;
	}
    }

    rinfo->max_router= num_routers;

    /* Allocate memory for the routes from our router to all other routers */
    array_size= rinfo->max_router * rinfo->max_router * sizeof(int);
    ptr_size= rinfo->max_router * sizeof(int *);
    rinfo->Router_paths= (int **)malloc(array_size + ptr_size);
    if (rinfo->Router_paths == NULL)   {
        fprintf(stderr, "Memory allocation failed! Line %d\n", __LINE__);
        exit(0);
    }

    /* Fill in the pointer array */
    int_ptr= (int *)(rinfo->Router_paths + rinfo->max_router);
    for (i= 0; i < rinfo->max_router; i++)   {
        rinfo->Router_paths[i]= int_ptr;
        int_ptr= int_ptr + rinfo->max_router;
    }

    /* Clear it */
    for (y= 0; y < rinfo->max_router; y++)   {
	for (x= 0; x < rinfo->max_router; x++)   {
	    rinfo->Router_paths[y][x]= -1;
	}
    }



    /* Allocate memory for the routes from our NIC to all other NICs */
    /*
    ** The array is max_NIC x (max_router + 1). At worst, we visit each
    ** router once and need an exit port to the local NIC on the last router.
    */
    array_size= rinfo->max_NIC * (rinfo->max_router + 1) * sizeof(int);
    ptr_size= rinfo->max_NIC * sizeof(int *);
    rinfo->NIC_routes= (int **)malloc(array_size + ptr_size);
    if (rinfo->NIC_routes == NULL)   {
        fprintf(stderr, "Memory allocation failed! Line %d\n", __LINE__);
        exit(0);
    }

    /* Fill in the pointer array */
    int_ptr= (int *)(rinfo->NIC_routes + (rinfo->max_router + 1));
    for (i= 0; i < rinfo->max_NIC; i++)   {
        rinfo->NIC_routes[i]= int_ptr;
        int_ptr= int_ptr + (rinfo->max_router + 1);
    }

    /* Clear it */
    for (y= 0; y < rinfo->max_NIC; y++)   {
	for (x= 0; x < (rinfo->max_router + 1); x++)   {
	    rinfo->NIC_routes[y][x]= -1;
	}
    }



    /* Allocate memory to record visits in the DFS */
    rinfo->Routers= (router_t *)malloc(sizeof(router_t) * rinfo->max_router);
    if (rinfo->Routers == NULL)   {
        fprintf(stderr, "Memory allocation failed! Line %d\n", __LINE__);
        exit(0);
    }
    clear_visited(rinfo);
    clear_labels(rinfo);


    /* Allocate memory for a stack */
    rinfo->router_stack= (int *)malloc(sizeof(int) * rinfo->max_router);
    if (rinfo->router_stack == NULL)   {
        fprintf(stderr, "Memory allocation failed! Line %d\n", __LINE__);
        exit(0);
    }
    memset(rinfo->router_stack, 0, sizeof(int) * rinfo->max_router);
    rinfo->router_stack_pos= 0;

    return (void *)rinfo;

}  /* end of init_routing() */



static void
clear_visited(route_info_t *rinfo)
{

int i;


    for (i= 0; i < rinfo->max_router; i++)   {
	rinfo->Routers[i].visited= 0;
    }

}  /* end of clear_visited() */



static void
clear_labels(route_info_t *rinfo)
{

int i;


    for (i= 0; i < rinfo->max_router; i++)   {
	rinfo->Routers[i].label= 0;
    }

}  /* end of clear_labels() */



void
Adj_Matrix_insert(int link, int left_router, int left_port, int right_router, int right_port, void *vrinfo)
{

route_info_t *rinfo;


    rinfo= (route_info_t *)vrinfo;
    if ((left_router < 0) || (left_router >= rinfo->max_router))   {
	fprintf(stderr, "LeftRouter 0 <= %d < %d bad\n", left_router, rinfo->max_router);
	exit(0);
    }

    if ((right_router < 0) || (right_router >= rinfo->max_router))   {
	fprintf(stderr, "RightRouter 0 <= %d < %d bad\n", right_router, rinfo->max_router);
	exit(0);
    }

    if (left_port < 0)   {
	fprintf(stderr, "LeftPort %d < 0 bad\n", left_port);
	exit(0);
    }

    if (right_port < 0)   {
	fprintf(stderr, "RightPort %d < 0 bad\n", right_port);
	exit(0);
    }

    rinfo->Links[left_router][right_router].port= left_port;
    rinfo->Links[right_router][left_router].port= right_port;

}  /* end of Adj_Matrix_insert() */



void
Adj_Matrix_print(void *vrinfo)
{

int x, y;
route_info_t *rinfo;


    rinfo= (route_info_t *)vrinfo;
    printf("\nRouter Adjacency Table\n");
    printf("Entries are port numbers\n");
    printf("     ");
    for (x= 0; x < rinfo->max_router; x++)   {
	printf("|%2d", x);
    }
    printf("|\n");
    printf("-----");
    for (x= 0; x < rinfo->max_router; x++)   {
	printf("+--");
    }
    printf("|\n");

    for (y= 0; y < rinfo->max_router; y++)   {
	printf("%2d   ", y);
	for (x= 0; x < rinfo->max_router; x++)   {
	    if (rinfo->Links[y][x].port < 0)   {
		printf("|  ");
	    } else   {
		printf("|%2d", rinfo->Links[y][x].port);
	    }
	}
	printf("|\n");
    }

    printf("-----");
    for (x= 0; x < rinfo->max_router; x++)   {
	printf("+--");
    }
    printf("|\n");

}  /* end of Adj_Matrix_print() */



static void
print_routes(int me, route_info_t *rinfo)
{

int dst;
int i;


    printf("NIC to NIC routes for rank %d\n", me);
    for (dst= 0; dst < rinfo->max_NIC; dst++)   {
	printf("%4d --> %4d:   ", me, dst);
	i= 0;
	while (rinfo->NIC_routes[dst][i] >= 0)   {
	    printf("%2d ", rinfo->NIC_routes[dst][i]);
	    i++;
	}
	printf("\n");
    }

}  /* end of print_routes() */



/*
** me is the router this NIC is attached to.
*/
void
gen_routes(int my_rank, int my_router, int debug, void *vrinfo)
{

int root= 0;
int dst_router;
int dst_NIC;
int current_router;
int i;
int alt, min;
int going_up;
route_info_t *rinfo;


    rinfo= (route_info_t *)vrinfo;
    rinfo->me= my_rank;
    /*
    ** For now we are going to use the enhanced DFS-based up* / down* algorithm
    ** as described by Sancho, Robles, and Duato in "An Effective Methodology
    ** to Improve the Performance of the Up* / Down* Routing Algorithm."
    */

    /* Build the DFS */
    rinfo->on_main_branch= 1;
    rinfo->current_level= 0;
    if (my_rank == 0 && debug > 3) printf("--- Building the DFS\n");
    DFS(root, rinfo);


    /* Label the Routers */
    clear_visited(rinfo);
    rinfo->current_label= 1;
    if (my_rank == 0 && debug > 3) printf("--- Labelling the secondary branches\n");
    Label_secondary(root, rinfo);

    /*
    ** Label the main branch by collecting the exisiting labels and re-assigning them
    ** in reverse order.
    */
    clear_visited(rinfo);
    if (my_rank == 0 && debug > 3) printf("--- Record main branch\n");
    main_down(root, rinfo);
    if (my_rank == 0 && debug > 3) printf("--- Label main branch\n");
    main_up(root, rinfo);
    rinfo->Routers[root].label= 0;
    if (my_rank == 0 && debug > 3) printf("--- Done with Labels\n");


    /* Assign directions to the links is implicit: Up is toward the higher router label */


    /* 
    ** From here on down is Dijkstra's shortest path.
    ** For each path we make sure it is up* / down*; i.e.,
    ** avoid up to down reversals.
    ** We record the chosen paths in Routes;
    */
    clear_visited(rinfo);
    current_router= my_router;
    for (dst_router= 0; dst_router < rinfo->max_router; dst_router++)   {
	if (dst_router == current_router)   {
	    rinfo->Routers[dst_router].distance= 0;
	} else   {
	    rinfo->Routers[dst_router].distance= INFINITE;
	}
    }

    while (1)   {
	min= INFINITE;
	/* Find the router with smallest distance from us */
	for (dst_router= 0; dst_router < rinfo->max_router; dst_router++)   {
	    if ((rinfo->Routers[dst_router].distance < min) && (rinfo->Routers[dst_router].visited == 0))   {
		min= rinfo->Routers[dst_router].distance;
		current_router= dst_router;
	    }
	}
	if (my_rank == 0 && debug > 3) printf("    --- Processing router %d\n", current_router);

	/* If there are no paths, then we are done */
	if (min == INFINITE)   {
	    break;
	}

	rinfo->Routers[current_router].visited= 1;

	for (dst_router= 0; dst_router < rinfo->max_router; dst_router++)   {
	    if (rinfo->Links[current_router][dst_router].port >= 0)   {
		/* There is a path to router dst */
		alt= rinfo->Routers[current_router].distance + 1;
		if (my_rank == 0 && debug > 3)   {
		    printf("        --- Path from %d to %d using port %d\n",
			current_router, dst_router, rinfo->Links[current_router][dst_router].port);
		}

		/*
		** This could be it, but we need to make sure up* / down*
		** is not violated by this path.
		*/
		going_up= 1;
		i= 0;
		while (rinfo->Router_paths[dst_router][i] >= 0)   {
		    if (up(current_router, dst_router, rinfo) && going_up)   {
			/* Still going up */
		    } else   {
			/* Can't go up after we have gone down! */
			alt= INFINITE;
			if (my_rank == 0 && debug > 3)   {
			    printf("        --- Path from %d to %d is not in up*/down*\n",
				current_router, dst_router);
			}
			break;
		    }

		    if (down(current_router, dst_router, rinfo))   {
			going_up= 0;
		    }

		    i++;
		}

		if (alt < rinfo->Routers[dst_router].distance)   {
		    /* On the path */
		    rinfo->Routers[dst_router].distance= alt;

		    /* Copy route from my router to current_router to dst_router and add the last hop */
		    i= 0;
		    while (rinfo->Router_paths[current_router][i] >= 0)   {
			rinfo->Router_paths[dst_router][i]= rinfo->Router_paths[current_router][i];
			i++;
		    }
		    rinfo->Router_paths[dst_router][i]= rinfo->Links[current_router][dst_router].port;
		    if (my_rank == 0 && debug > 3)   {
			printf("        --- Using path from %d to %d, port %d at pos %d\n",
			    current_router, dst_router, rinfo->Links[current_router][dst_router].port, i);
		    }

		} else   {
		    if (my_rank == 0 && debug > 3)   {
			printf("        --- Path from %d to %d is too long\n", current_router, dst_router);
		    }
		}
	    }
	}
    }

    /*
    ** At this points I have routes from my router to all other routers.
    ** Now I need to generate routes to all NICs and add local router exit ports.
    */
    for (dst_NIC= 0; dst_NIC < rinfo->max_NIC; dst_NIC++)   {
	/* What is the destination router? */
	dst_router= rinfo->NIC_table[dst_NIC].router;

	/* Copy the path from my router to the destination router */
	i= 0;
	while (rinfo->Router_paths[dst_router][i] >= 0)   {
	    rinfo->NIC_routes[dst_NIC][i]= rinfo->Router_paths[dst_router][i];
	    i++;
	}

	/* Now add the exit port to the local NIC at the destination */
	rinfo->NIC_routes[dst_NIC][i]= rinfo->NIC_table[dst_NIC].port;
    }


    if (debug > 2)   {
	print_routes(my_rank, rinfo);
    }

}   /* end of gen_route() */



static int
down(int src, int dst, route_info_t *rinfo)
{
    if (rinfo->Routers[dst].level > rinfo->Routers[src].level)   {
	return 1;
    }
    if (rinfo->Routers[dst].level == rinfo->Routers[src].level)   {
	if (rinfo->Routers[dst].label > rinfo->Routers[dst].label)   {
	    return 1;
	}
    }

    return 0;

}  /* end of down() */



static int
up(int src, int dst, route_info_t *rinfo)
{
    return !down(src, dst, rinfo);
}  /* end of up() */



static void
push_label(int value, route_info_t *rinfo)
{
    if (rinfo->router_stack_pos >= rinfo->max_router)   {
	fprintf(stderr, "push_label() internal error!\n");
	exit(0);
    }

    rinfo->router_stack[rinfo->router_stack_pos++]= value;

}  /* end of push_label() */



static int
pop_label(route_info_t *rinfo)
{
    if (rinfo->router_stack_pos < 1)   {
	fprintf(stderr, "push_label() internal error!\n");
	exit(0);
    }

    return rinfo->router_stack[--rinfo->router_stack_pos];

}  /* end of pop_label() */



/*
** Go down the main branch and collect labels
*/
static void
main_down(int v, route_info_t *rinfo)
{

int w;


    rinfo->Routers[v].visited= 1;

    for (w= 0; w < rinfo->max_router; w++)   {
	if ((rinfo->Links[v][w].port >= 0) && !rinfo->Routers[w].visited && (rinfo->Links[v][w].main_branch))   {
	    push_label(rinfo->Routers[w].label, rinfo);
	    main_down(w, rinfo);
	}
    }

}  /* end of main_down() */



/*
** Go down the main branch and assign label on the way back up
*/
static void
main_up(int v, route_info_t *rinfo)
{

int w;


    rinfo->Routers[v].visited= 1;

    for (w= 0; w < rinfo->max_router; w++)   {
	if ((rinfo->Links[v][w].port >= 0) && !rinfo->Routers[w].visited && (rinfo->Links[v][w].main_branch))   {
	    main_up(w, rinfo);
	    rinfo->Routers[w].label= pop_label(rinfo);
	}
    }

}  /* end of main_up() */



static void
DFS(int v, route_info_t *rinfo)
{

int w;


    rinfo->Routers[v].visited= 1;
    rinfo->Routers[v].level= rinfo->current_level;

    for (w= 0; w < rinfo->max_router; w++)   {
	if ((rinfo->Links[v][w].port >= 0) && !rinfo->Routers[w].visited)   {
	    rinfo->current_level++;
	    DFS(w, rinfo);
	    /* add edge vw to tree T */
	    rinfo->Links[v][w].in_tree= 1;
	    if (rinfo->on_main_branch)   {
		rinfo->Links[v][w].main_branch= 1;
	    }
	    rinfo->current_level--;
	} else   {
	    rinfo->on_main_branch= 0;
	}
    }

}  /* end of DFS() */



/*
** Label the secondary branches
*/
static void
Label_secondary(int v, route_info_t *rinfo)
{

int w;


    rinfo->Routers[v].visited= 1;

    for (w= rinfo->max_router - 1; w >= 0; w--)   {
	if (rinfo->Links[v][w].in_tree && !rinfo->Routers[w].visited)   {
	    Label_secondary(w, rinfo);
	}
    }

    /* Assign a label coming back up */
    rinfo->Routers[v].label= rinfo->current_label++;

}  /* end of Label_secondary() */
