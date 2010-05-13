/*
** $Id: gen.c,v 1.7 2010/05/13 19:27:22 rolf Exp $
**
** Rolf Riesen, April 2010, Sandia National Laboratories
**
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gen.h"

/* How many NICs per router max? */
/* FIXME: Some day I should make this dynamic */
#define MAX_NICS	(16)

/* Max length of link label */
#define MAX_LABEL	(32)


typedef struct nic_t   {
    int rank;
    int router_id;
    int router_port;
    char label[MAX_LABEL];
    struct nic_t *next;
} nic_t;


typedef struct link_t   {
    int id;
    int left_router;
    int right_router;
    int left_router_port;
    int right_router_port;
    char label[MAX_LABEL];
    struct link_t *next;
} link_t;


typedef struct router_t   {
    int id;
    int num_ports;
    int next_nic;
    int next_link;
    nic_t *nics[MAX_NICS];
    link_t **links;
    struct router_t *next;
} router_t;


static router_t *router_current= NULL;
static router_t *router_list= NULL;
static router_t *router_list_end= NULL;

static nic_t *nic_current= NULL;
static nic_t *nic_list= NULL;
static nic_t *nic_list_end= NULL;

static int next_link_id= 0;
static link_t *link_current= NULL;
static link_t *link_list= NULL;
static link_t *link_list_end= NULL;

static int g_num_routers= 0;
static int g_num_ports= 0;
static int g_num_nics= 0;
static int g_num_links= 0;



/*
** Add a Router
*/
void
gen_router(int id, int num_ports)
{

router_t *current;
int i;


    if (num_ports < 1)   {
	fprintf(stderr, "Routers without ports are useless!\n");
	exit(9);
    }

    if (g_num_ports < 1)   {
	/* First time setting this */
	g_num_ports= num_ports;
    } else   {
	if (num_ports != g_num_ports)   {
	    /*
	    ** In the SST xml file we only say once how many ports each router has.
	    ** We could change it to make it an individual setting per router, but
	    ** it seems of little value.
	    */
	    fprintf(stderr, "For now, all routers must have the same number of ports!\n");
	    exit(9);
	}
    }

    current= (router_t *)malloc(sizeof(router_t));
    if (current == NULL)   {
	fprintf(stderr, "Out of memory!\n");
	exit(9);
    }

    current->links= (link_t **)malloc(sizeof(link_t) * (num_ports + 1));
    if (current->links == NULL)   {
	fprintf(stderr, "Out of memory!\n");
	exit(9);
    }

    current->id= id;
    current->num_ports= num_ports;

    current->next_nic= 0;
    for (i= 0; i < MAX_NICS; i++)   {
	current->nics[i]= NULL;
    }

    current->next_link= 0;
    for (i= 0; i < num_ports; i++)   {
	current->links[i]= NULL;
    }

    if (router_list_end)   {
	/* Append */
	router_list_end->next= current;
	router_list_end= current;
    } else   {
	/* First entry */
	router_list= current;
	router_list_end= current;
    }

    g_num_routers++;

}  /* end of gen_router() */



/*
** Find the router with this id and return a pointer to it.
*/
static router_t *
find_router(int id)
{

router_t *current;


    current= router_list;
    while (current)   {
	if (id == current->id)   {
	    return current;
	}
	current= current->next;
    }

    return NULL;

}  /* end of find_router() */



/*
** Reset the nic port pointer inside a router
*/
void
reset_router_nics(int router)
{

router_t *r;


    r= find_router(router);
    if (!r)   {
	fprintf(stderr, "Cannot find router %d! Routers must be defined before NICs.\n", router);
	exit(8);
    }
    r->next_nic= 0;

}   /* end of reset_router_nics() */



/*
** Traverse the list of ports connected to NICs inside a router
*/
int
next_router_nic(int router, int *port)
{

router_t *r;
nic_t *n;


    r= find_router(router);
    if (!r)   {
	fprintf(stderr, "Cannot find router %d! Routers must be defined before NICs.\n", router);
	exit(8);
    }

    if (r->next_nic >= MAX_NICS)   {
	return 0;
    }

    n= r->nics[r->next_nic];
    if (n == NULL)   {
	return 0;
    }

    if (n->router_id != router)   {
	fprintf(stderr, "Inconsistency: Router link to a NIC with a link to another router!\n");
	exit(10);
    }

    *port= n->router_port;
    r->next_nic++;
    return 1;

}   /* end of next_router_nic() */



/*
** Reset the link port pointer inside a router
*/
void
reset_router_links(int router)
{

router_t *r;


    r= find_router(router);
    if (!r)   {
	fprintf(stderr, "Cannot find router %d! Routers must be defined before links.\n", router);
	exit(8);
    }
    r->next_link= 0;

}   /* end of reset_router_links() */



/*
** Traverse the list of ports connected to other routers
*/
int
next_router_link(int router, int *link_id, int *port)
{

router_t *r;
link_t *l;


    r= find_router(router);
    if (!r)   {
	fprintf(stderr, "Cannot find router %d! Routers must be defined before links.\n", router);
	exit(8);
    }

    if (r->next_link >= r->num_ports)   {
	return 0;
    }

    l= r->links[r->next_link];
    if (l == NULL)   {
	return 0;
    }

    if (router == l->left_router)   {
	/* router is connected to the left side */
	*port= l->left_router_port;
    } else if (router == l->right_router)   {
	*port= l->right_router_port;
    } else   {
	/* Something is wrong */
	fprintf(stderr, "Cannot find router %d on either end of this link!\n", router);
	exit(8);
    }

    *link_id= l->id;

    r->next_link++;
    return 1;

}   /* end of next_router_link() */



/*
** Add a NIC and the link to its router
*/
void
gen_nic(int rank, int router, int port)
{

nic_t *current;
router_t *r;
int i;


    current= (nic_t *)malloc(sizeof(nic_t));
    if (current == NULL)   {
	fprintf(stderr, "Out of memory!\n");
	exit(9);
    }

    current->rank= rank;
    current->next= NULL;
    current->router_id= router;
    current->router_port= port;
    snprintf(current->label, MAX_LABEL, "-- R%d/p%d", router, port);

    r= find_router(router);
    if (!r)   {
	fprintf(stderr, "Cannot find router %d! Routers must be defined before NICs.\n", router);
	exit(8);
    }

    for (i= 0; i < MAX_NICS; i++)   {
	if (r->nics[i] == NULL)   {
	    /* Unused slot */
	    r->nics[i]= current;
	    break;
	}
    }

    if (i >= MAX_NICS)   {
	fprintf(stderr, "Out of NIC port slots! Cannot handle more than %d NICs per router.\n", MAX_NICS);
	exit(8);
    }

    if (nic_list_end)   {
	/* Append */
	nic_list_end->next= current;
	nic_list_end= current;
    } else   {
	/* First entry */
	nic_list= current;
	nic_list_end= current;
    }

    g_num_nics++;

}  /* end of gen_nic() */



/*
** Add a link between routers
** Call this only once per link. It will hook into both routers, so
** don't call it for each router individually!
*/
void
gen_link(int Arouter, int Aport, int Brouter, int Bport)
{

link_t *current;
int i;
router_t *A, *B;


    current= (link_t *)malloc(sizeof(link_t));
    if (current == NULL)   {
	fprintf(stderr, "Out of memory!\n");
	exit(9);
    }

    current->id= next_link_id++;
    current->left_router= Arouter;
    current->right_router= Brouter;
    current->left_router_port= Aport;
    current->right_router_port= Bport;
    snprintf(current->label, MAX_LABEL, "R%d/p%d -- R%d/p%d", Arouter, Aport, Brouter, Bport);

    A= find_router(Arouter);
    if (!A)   {
	fprintf(stderr, "Cannot find router %d! Routers must be defined before NICs.\n", Arouter);
	exit(8);
    }

    B= find_router(Brouter);
    if (!B)   {
	fprintf(stderr, "Cannot find router %d! Routers must be defined before NICs.\n", Brouter);
	exit(8);
    }

    for (i= 0; i < A->num_ports; i++)   {
	if (A->links[i] == NULL)   {
	    /* Unused slot */
	    A->links[i]= current;
	    break;
	}
    }

    if (i >= A->num_ports)   {
	fprintf(stderr, "Out of router port slots on source router!\n");
	exit(8);
    }

    for (i= 0; i < B->num_ports; i++)   {
	if (B->links[i] == NULL)   {
	    /* Unused slot */
	    B->links[i]= current;
	    break;
	}
    }

    if (i >= B->num_ports)   {
	fprintf(stderr, "Out of router port slots on destination router!\n");
	exit(8);
    }



    if (link_list_end)   {
	/* Append */
	link_list_end->next= current;
	link_list_end= current;
    } else   {
	/* First entry */
	link_list= current;
	link_list_end= current;
    }

    g_num_links++;

}  /* end of gen_link() */



void
reset_router_list(void)
{
    router_current= router_list;
}  /* end of reset_router_list() */



int
next_router(int *id)
{

    if (!router_current)   {
	return 0;
    }
    *id= router_current->id;
    router_current= router_current->next;
    return 1;

}  /* end of next_router() */



void
reset_nic_list(void)
{
    nic_current= nic_list;
}  /* end of reset_nic_list() */



int
next_nic(int *id, int *router, int *port, char **label)
{

    if (!nic_current)   {
	return 0;
    }
    *id= nic_current->rank;
    *router= nic_current->router_id;
    *port= nic_current->router_port;
    *label= nic_current->label;
    nic_current= nic_current->next;
    return 1;

}  /* end of next_nic() */



void
reset_link_list(void)
{
    link_current= link_list;
}  /* end of reset_link_list() */



int
next_link(int *Arouter, int *Aport, int *Brouter, int *Bport, char **label)
{

    if (!link_current)   {
	return 0;
    }
    *Arouter= link_current->left_router;
    *Aport= link_current->left_router_port;
    *Brouter= link_current->right_router;
    *Bport= link_current->right_router_port;
    *label= link_current->label;
    link_current= link_current->next;
    return 1;

}  /* end of next_link() */



int
get_num_nics(void)
{
    return g_num_nics;
}  /* end of get_num_nics() */



int
get_num_routers(void)
{
    return g_num_routers;
}  /* end of get_num_routers() */



int
get_num_ports(void)
{
    return g_num_ports;
}  /* end of get_num_ports() */



int
get_num_links(void)
{
    return g_num_links;
}  /* end of get_num_links() */
