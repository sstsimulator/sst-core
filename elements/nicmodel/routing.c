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

static NIC_table_t *NIC_table= NULL;
static link_t **Links;
static router_t *Routers;
static int **Router_paths;
static int **NIC_routes;
static int *router_stack;
static int router_stack_pos;
static int max_router= -1;
static int max_NIC= -1;
static int current_label;
static int current_level;
static int on_main_branch;


/* Local functions */
static void DFS(int v);
static int pop_label(void);
static void push_label(int value);
static void main_down(int v);
static void main_up(int v);
static void Label_secondary(int v);
static void clear_visited(void);
static void clear_labels(void);
static int down(int src, int dest);
static int up(int src, int dest);



void
init_NIC_table(int num_NICs)
{

int i;


    NIC_table= (NIC_table_t *)malloc(num_NICs * sizeof(NIC_table_t));
    if (NIC_table == NULL)   {
	fprintf(stderr, "Out of memory!\n");
	exit(0);
    }

    for (i= 0; i < num_NICs; i++)   {
	NIC_table[i].router= -1;
	NIC_table[i].port= -1;
    }

    max_NIC= num_NICs;

}  /* end of init_NIC_table() */



int
check_NIC_table(void)
{

int i;


    for (i= 0; i < max_NIC; i++)   {
	if ((NIC_table[i].router < 0) || (NIC_table[i].port < 0))   {
	    fprintf(stderr, "NIC %d not properly initialized: router %d, port %d\n",
		i, NIC_table[i].router, NIC_table[i].port);
	    return -1;
	}
    }

    return 0;

}  /* end of check_NIC_table() */



void
NIC_table_insert_router(int rank, int router)
{

    if (NIC_table[rank].router >= 0)   {
	fprintf(stderr, "Already specified a router (%d) for NIC %d\n",
	    NIC_table[rank].router, rank);
	exit(0);
    }
    if (router < 0)   {
	fprintf(stderr, "Invalid router ID (%d) for NIC %d\n",
	    router, rank);
	exit(0);
    }

    NIC_table[rank].router= router;

}  /* end of NIC_table_insert_rank() */



void
NIC_table_insert_port(int rank, int num_item)
{

    if (NIC_table[rank].port >= 0)   {
	fprintf(stderr, "Already specified a port (%d) for NIC %d\n",
	    NIC_table[rank].port, rank);
	exit(0);
    }
    if (num_item < 0)   {
	fprintf(stderr, "Invalid port number (%d) for NIC %d\n",
	    num_item, rank);
	exit(0);
    }

    NIC_table[rank].port= num_item;

}  /* end of NIC_table_insert_rank() */



/*
** Each entry in the table is the exit port number.
** -1 means no connection
*/
void
init_Adj_Matrix(int n)
{

int array_size, ptr_size;
link_t *ptr;
int *int_ptr;
int i, x, y;


    /* Size for the data: n^2 ints */
    array_size= n * n * sizeof(link_t);

    /* Size for the pointers: n pointers to int */
    ptr_size= n * sizeof(link_t *);

    /* Allocate all memory in one chunk */
    Links= (link_t **)malloc(array_size + ptr_size);
    if (Links == NULL)   {
        fprintf(stderr, "Memory allocation failed!\n");
        exit(0);
    }

    /* Fill in the pointer array */
    ptr= (link_t *)(Links + n);
    for (i= 0; i < n; i++)   {
        Links[i]= ptr;
        ptr= ptr + n;
    }

    /* Clear it */
    for (y= 0; y < n; y++)   {
	for (x= 0; x < n; x++)   {
	    Links[y][x].port= -1;
	    Links[y][x].in_tree= 0;
	    Links[y][x].main_branch= 0;
	}
    }

    max_router= n;

    /* Allocate memory for the routes from our router to all other routers */
    array_size= max_router * max_router * sizeof(int);
    ptr_size= max_router * sizeof(int *);
    Router_paths= (int **)malloc(array_size + ptr_size);
    if (Router_paths == NULL)   {
        fprintf(stderr, "Memory allocation failed!\n");
        exit(0);
    }

    /* Fill in the pointer array */
    int_ptr= (int *)(Router_paths + max_router);
    for (i= 0; i < max_router; i++)   {
        Router_paths[i]= int_ptr;
        int_ptr= int_ptr + max_router;
    }

    /* Clear it */
    for (y= 0; y < max_router; y++)   {
	for (x= 0; x < max_router; x++)   {
	    Router_paths[y][x]= -1;
	}
    }



    /* Allocate memory for the routes from our NIC to all other NICs */
    /*
    ** The array is max_NIC x (max_router + 1). At worst, we visit each
    ** router once and need an exit port to the local NIC on the last router.
    */
    array_size= max_NIC * (max_router + 1) * sizeof(int);
    ptr_size= max_NIC * sizeof(int *);
    NIC_routes= (int **)malloc(array_size + ptr_size);
    if (NIC_routes == NULL)   {
        fprintf(stderr, "Memory allocation failed!\n");
        exit(0);
    }

    /* Fill in the pointer array */
    int_ptr= (int *)(NIC_routes + (max_router + 1));
    for (i= 0; i < max_NIC; i++)   {
        NIC_routes[i]= int_ptr;
        int_ptr= int_ptr + (max_router + 1);
    }

    /* Clear it */
    for (y= 0; y < max_NIC; y++)   {
	for (x= 0; x < (max_router + 1); x++)   {
	    NIC_routes[y][x]= -1;
	}
    }



    /* Allocate memory to record visits in the DFS */
    Routers= (router_t *)malloc(sizeof(router_t) * max_router);
    if (Routers == NULL)   {
        fprintf(stderr, "Memory allocation failed!\n");
        exit(0);
    }
    clear_visited();
    clear_labels();


    /* Allocate memory for a stack */
    router_stack= (int *)malloc(sizeof(int) * max_router);
    if (router_stack == NULL)   {
        fprintf(stderr, "Memory allocation failed!\n");
        exit(0);
    }
    memset(router_stack, 0, sizeof(int) * max_router);
    router_stack_pos= 0;


}  /* end of init_Adj_Matrix() */



static void
clear_visited(void)
{

int i;


    for (i= 0; i < max_router; i++)   {
	Routers[i].visited= 0;
    }

}  /* end of clear_visited() */



static void
clear_labels(void)
{

int i;


    for (i= 0; i < max_router; i++)   {
	Routers[i].label= 0;
    }

}  /* end of clear_labels() */



void
Adj_Matrix_insert(int link, int left_router, int left_port, int right_router, int right_port)
{

    if ((left_router < 0) || (left_router >= max_router))   {
	fprintf(stderr, "LeftRouter 0 <= %d < %d bad\n", left_router, max_router);
	exit(0);
    }

    if ((right_router < 0) || (right_router >= max_router))   {
	fprintf(stderr, "RightRouter 0 <= %d < %d bad\n", right_router, max_router);
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

    Links[left_router][right_router].port= left_port;
    Links[right_router][left_router].port= right_port;

}  /* end of Adj_Matrix_insert() */



void
Adj_Matrix_print(void)
{

int x, y;


    printf("\nRouter Adjacency Table\n");
    printf("Entries are port numbers\n");
    printf("     ");
    for (x= 0; x < max_router; x++)   {
	printf("|%2d", x);
    }
    printf("|\n");
    printf("-----");
    for (x= 0; x < max_router; x++)   {
	printf("+--");
    }
    printf("|\n");

    for (y= 0; y < max_router; y++)   {
	printf("%2d   ", y);
	for (x= 0; x < max_router; x++)   {
	    if (Links[y][x].port < 0)   {
		printf("|  ");
	    } else   {
		printf("|%2d", Links[y][x].port);
	    }
	}
	printf("|\n");
    }

    printf("-----");
    for (x= 0; x < max_router; x++)   {
	printf("+--");
    }
    printf("|\n");

}  /* end of Adj_Matrix_print() */



static void
print_routes(int me)
{

int dst;
int i;


    printf("NIC to NIC routes for rank %d\n", me);
    for (dst= 0; dst < max_NIC; dst++)   {
	printf("%4d --> %4d:   ", me, dst);
	i= 0;
	while (NIC_routes[dst][i] >= 0)   {
	    printf("%2d ", NIC_routes[dst][i]);
	    i++;
	}
	printf("\n");
    }

}  /* end of print_routes() */



/*
** me is the router this NIC is attached to.
*/
void
gen_routes(int my_rank, int my_router, int debug)
{

int root= 0;
int dst_router;
int dst_NIC;
int current_router;
int i;
int alt, min;
int going_up;


    /*
    ** For now we are going to use the enhanced DFS-based up* / down* algorithm
    ** as described by Sancho, Robles, and Duato in "An Effective Methodology
    ** to Improve the Performance of the Up* / Down* Routing Algorithm."
    */

    /* Build the DFS */
    on_main_branch= 1;
    current_level= 0;
    if (my_rank == 0 && debug > 3) printf("--- Building the DFS\n");
    DFS(root);


    /* Label the Routers */
    clear_visited();
    current_label= 1;
    if (my_rank == 0 && debug > 3) printf("--- Labelling the secondary branches\n");
    Label_secondary(root);

    /*
    ** Label the main branch by collecting the exisiting labels and re-assigning them
    ** in reverse order.
    */
    clear_visited();
    if (my_rank == 0 && debug > 3) printf("--- Record main branch\n");
    main_down(root);
    if (my_rank == 0 && debug > 3) printf("--- Label main branch\n");
    main_up(root);
    Routers[root].label= 0;
    if (my_rank == 0 && debug > 3) printf("--- Done with Labels\n");


    /* Assign directions to the links is implicit: Up is toward the higher router label */


    /* 
    ** From here on down is Dijkstra's shortest path.
    ** For each path we make sure it is up* / down*; i.e.,
    ** avoid up to down reversals.
    ** We record the chosen paths in Routes;
    */
    clear_visited();
    current_router= my_router;
    for (dst_router= 0; dst_router < max_router; dst_router++)   {
	if (dst_router == current_router)   {
	    Routers[dst_router].distance= 0;
	} else   {
	    Routers[dst_router].distance= INFINITE;
	}
    }

    while (1)   {
	min= INFINITE;
	/* Find the router with smallest distance from us */
	for (dst_router= 0; dst_router < max_router; dst_router++)   {
	    if ((Routers[dst_router].distance < min) && (Routers[dst_router].visited == 0))   {
		min= Routers[dst_router].distance;
		current_router= dst_router;
	    }
	}
	if (my_rank == 0 && debug > 3) printf("    --- Processing router %d\n", current_router);

	/* If there are no paths, then we are done */
	if (min == INFINITE)   {
	    break;
	}

	Routers[current_router].visited= 1;

	for (dst_router= 0; dst_router < max_router; dst_router++)   {
	    if (Links[current_router][dst_router].port >= 0)   {
		/* There is a path to router dst */
		alt= Routers[current_router].distance + 1;
		if (my_rank == 0 && debug > 3)   {
		    printf("        --- Path from %d to %d using port %d\n",
			current_router, dst_router, Links[current_router][dst_router].port);
		}

		/*
		** This could be it, but we need to make sure up* / down*
		** is not violated by this path.
		*/
		going_up= 1;
		i= 0;
		while (Router_paths[dst_router][i] >= 0)   {
		    if (up(current_router, dst_router) && going_up)   {
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

		    if (down(current_router, dst_router))   {
			going_up= 0;
		    }

		    i++;
		}

		if (alt < Routers[dst_router].distance)   {
		    /* On the path */
		    Routers[dst_router].distance= alt;

		    /* Copy route from my router to current_router to dst_router and add the last hop */
		    i= 0;
		    while (Router_paths[current_router][i] >= 0)   {
			Router_paths[dst_router][i]= Router_paths[current_router][i];
			i++;
		    }
		    Router_paths[dst_router][i]= Links[current_router][dst_router].port;
		    if (my_rank == 0 && debug > 3)   {
			printf("        --- Using path from %d to %d, port %d at pos %d\n",
			    current_router, dst_router, Links[current_router][dst_router].port, i);
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
    for (dst_NIC= 0; dst_NIC < max_NIC; dst_NIC++)   {
	/* What is the destination router? */
	dst_router= NIC_table[dst_NIC].router;

	/* Copy the path from my router to the destination router */
	i= 0;
	while (Router_paths[dst_router][i] >= 0)   {
	    NIC_routes[dst_NIC][i]= Router_paths[dst_router][i];
	    i++;
	}

	/* Now add the exit port to the local NIC at the destination */
	NIC_routes[dst_NIC][i]= NIC_table[dst_NIC].port;
    }


    if (debug > 2)   {
	print_routes(my_rank);
    }

}   /* end of gen_route() */



static int
down(int src, int dst)
{
    if (Routers[dst].level > Routers[src].level)   {
	return 1;
    }
    if (Routers[dst].level == Routers[src].level)   {
	if (Routers[dst].label > Routers[dst].label)   {
	    return 1;
	}
    }

    return 0;

}  /* end of down() */



static int
up(int src, int dst)
{
    return !down(src, dst);
}  /* end of up() */



static void
push_label(int value)
{
    if (router_stack_pos >= max_router)   {
	fprintf(stderr, "push_label() internal error!\n");
	exit(0);
    }

    router_stack[router_stack_pos++]= value;

}  /* end of push_label() */



static int
pop_label(void)
{
    if (router_stack_pos < 1)   {
	fprintf(stderr, "push_label() internal error!\n");
	exit(0);
    }

    return router_stack[--router_stack_pos];

}  /* end of pop_label() */



/*
** Go down the main branch and collect labels
*/
static void
main_down(int v)
{

int w;


    Routers[v].visited= 1;

    for (w= 0; w < max_router; w++)   {
	if ((Links[v][w].port >= 0) && !Routers[w].visited && (Links[v][w].main_branch))   {
	    push_label(Routers[w].label);
	    main_down(w);
	}
    }

}  /* end of main_down() */



/*
** Go down the main branch and assign label on the way back up
*/
static void
main_up(int v)
{

int w;


    Routers[v].visited= 1;

    for (w= 0; w < max_router; w++)   {
	if ((Links[v][w].port >= 0) && !Routers[w].visited && (Links[v][w].main_branch))   {
	    main_up(w);
	    Routers[w].label= pop_label();
	}
    }

}  /* end of main_up() */



static void
DFS(int v)
{

int w;


    Routers[v].visited= 1;
    Routers[v].level= current_level;

    for (w= 0; w < max_router; w++)   {
	if ((Links[v][w].port >= 0) && !Routers[w].visited)   {
	    current_level++;
	    DFS(w);
	    /* add edge vw to tree T */
	    Links[v][w].in_tree= 1;
	    if (on_main_branch)   {
		Links[v][w].main_branch= 1;
	    }
	    current_level--;
	} else   {
	    on_main_branch= 0;
	}
    }

}  /* end of DFS() */



/*
** Label the secondary branches
*/
static void
Label_secondary(int v)
{

int w;


    Routers[v].visited= 1;

    for (w= max_router - 1; w >= 0; w--)   {
	if (Links[v][w].in_tree && !Routers[w].visited)   {
	    Label_secondary(w);
	}
    }

    /* Assign a label coming back up */
    Routers[v].label= current_label++;

}  /* end of Label_secondary() */
