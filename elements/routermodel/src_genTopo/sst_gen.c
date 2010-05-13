/*
** $Id: sst_gen.c,v 1.13 2010/05/13 19:27:23 rolf Exp $
**
** Rolf Riesen, March 2010, Sandia National Laboratories
**
*/
#include <stdio.h>
#include <stdlib.h>
#include "sst_gen.h"
#include "gen.h"

#define MAX_ID_LEN		(256)



void
sst_header(FILE *sstfile)
{

    if (sstfile == NULL)   {
	/* Nothing to output */
	return;
    }

    fprintf(sstfile, "<?xml version=\"1.0\"?>\n");
    fprintf(sstfile, "\n");
    fprintf(sstfile, "<config>\n");
    fprintf(sstfile, "    stopAtCycle=100000000000\n");
    fprintf(sstfile, "</config>\n");
    fprintf(sstfile, "\n");

}  /* end of sst_header() */



void
sst_cpu_param(FILE *sstfile, char *freq, char *exec, int cpu_verbose, int cpu_debug,
	char *cpu_nic_lat)
{

    if (sstfile == NULL)   {
	/* Nothing to output */
	return;
    }

    fprintf(sstfile, "<cpu_params>\n");
    fprintf(sstfile, "    <clock> %s </clock>\n", freq);
    fprintf(sstfile, "    <execFile> %s </execFile>\n", exec);
    fprintf(sstfile, "    <verbose> %d </verbose>\n", cpu_verbose);
    fprintf(sstfile, "    <debug> %d </debug>\n", cpu_debug);
    fprintf(sstfile, "</cpu_params>\n");
    fprintf(sstfile, "\n");
    fprintf(sstfile, "<cpu_link_params>\n");
    fprintf(sstfile, "    <lat> %s </lat>\n", cpu_nic_lat);
    fprintf(sstfile, "    <name> net0 </name>\n");
    fprintf(sstfile, "</cpu_link_params>\n");
    fprintf(sstfile, "\n");

}  /* end of sst_cpu_param() */



void
sst_nic_param_start(FILE *sstfile, int nic_debug)
{

    if (sstfile == NULL)   {
	/* Nothing to output */
	return;
    }

    fprintf(sstfile, "<nic_params>\n");
    fprintf(sstfile, "    <debug> %d </debug>\n", nic_debug);

}  /* end of sst_nic_param_start() */


void
sst_param_entry(FILE *sstfile, char *key, char *value)
{
    fprintf(sstfile, "    <%s> %s </%s>\n", key, value, key);
}  /* end of sst_param_entry() */


void
sst_nic_param_topology(FILE *sstfile)
{

int i;
int n, r, p;
int r1, r2, p1, p2;
char *label;
char str1[MAX_ID_LEN];
char str2[MAX_ID_LEN];


    if (sstfile == NULL)   {
	return;
    }

    /* How many NICs, routers, ports per router, and links? */
    snprintf(str1, MAX_ID_LEN, "%d", get_num_nics());
    sst_param_entry(sstfile, "num_NICs", str1);
    snprintf(str1, MAX_ID_LEN, "%d", get_num_routers());
    sst_param_entry(sstfile, "num_routers", str1);
    snprintf(str1, MAX_ID_LEN, "%d", get_num_ports());
    sst_param_entry(sstfile, "num_ports", str1);
    snprintf(str1, MAX_ID_LEN, "%d", get_num_links());
    sst_param_entry(sstfile, "num_links", str1);


    /* For each NIC we list the router and what port it is attached to */
    fprintf(sstfile, "\n");
    reset_nic_list();
    i= 0;
    while (next_nic(&n, &r, &p, &label))   {
	snprintf(str1, MAX_ID_LEN, "NIC%drouter", i);
	snprintf(str2, MAX_ID_LEN, "%d", r);
	sst_param_entry(sstfile, str1, str2);

	snprintf(str1, MAX_ID_LEN, "NIC%dport", i);
	snprintf(str2, MAX_ID_LEN, "%d", p);
	sst_param_entry(sstfile, str1, str2);
	i++;
    }

    if (i != get_num_nics())   {
	fprintf(stderr, "Something is wrong with the number of NICs! %d != %d\n", i, get_num_nics());
	exit(10);
    }

    /* List all links */
    fprintf(sstfile, "\n");
    reset_link_list();
    i= 0;
    while (next_link(&r1, &p1, &r2, &p2, &label))   {

	snprintf(str1, MAX_ID_LEN, "Link%dLeftRouter", i);
	snprintf(str2, MAX_ID_LEN, "%d", r1);
	sst_param_entry(sstfile, str1, str2);

	snprintf(str1, MAX_ID_LEN, "Link%dLeftPort", i);
	snprintf(str2, MAX_ID_LEN, "%d", p1);
	sst_param_entry(sstfile, str1, str2);

	snprintf(str1, MAX_ID_LEN, "Link%dRightRouter", i);
	snprintf(str2, MAX_ID_LEN, "%d", r2);
	sst_param_entry(sstfile, str1, str2);

	snprintf(str1, MAX_ID_LEN, "Link%dRightPort", i);
	snprintf(str2, MAX_ID_LEN, "%d", p2);
	sst_param_entry(sstfile, str1, str2);
	i++;
    }

    if (i != get_num_links())   {
	fprintf(stderr, "Something is wrong with the number of links! %d != %d\n", i, get_num_nics());
	exit(10);
    }

}  /* end of sst_nic_param_topology() */


void
sst_nic_param_end(FILE *sstfile, char *nic_cpu_lat, char *nic_net_lat)
{

    if (sstfile == NULL)   {
	/* Nothing to output */
	return;
    }

    fprintf(sstfile, "</nic_params>\n");
    fprintf(sstfile, "\n");

    fprintf(sstfile, "<nic_cpu_link>\n");
    fprintf(sstfile, "    <lat> %s </lat>\n", nic_cpu_lat);
    fprintf(sstfile, "    <name> CPU </name>\n");
    fprintf(sstfile, "</nic_cpu_link>\n");
    fprintf(sstfile, "\n");
    fprintf(sstfile, "<nic_net_link>\n");
    fprintf(sstfile, "    <lat> %s </lat>\n", nic_net_lat);
    fprintf(sstfile, "    <name> NETWORK </name>\n");
    fprintf(sstfile, "</nic_net_link>\n");
    fprintf(sstfile, "\n");

}  /* end of sst_nic_param_end() */



void
sst_router_param_start(FILE *sstfile, int num_ports)
{

    if (sstfile == NULL)   {
	/* Nothing to output */
	return;
    }

    fprintf(sstfile, "<router_params>\n");
    fprintf(sstfile, "    <hop_delay> 2us </hop_delay>\n");
    fprintf(sstfile, "    <debug> 0 </debug>\n");
    fprintf(sstfile, "    <num_ports> %d </num_ports>\n", num_ports);

}  /* end of sst_router_param_start() */



void
sst_router_param_end(FILE *sstfile)
{

    if (sstfile == NULL)   {
	/* Nothing to output */
	return;
    }

    fprintf(sstfile, "</router_params>\n");
    fprintf(sstfile, "\n");

}  /* end of sst_router_param_end() */



void
sst_body_start(FILE *sstfile)
{

    if (sstfile != NULL)   {
	fprintf(sstfile, "<sst>\n");
    }

}  /* end of sst_body_start() */



void
sst_body_end(FILE *sstfile)
{

    if (sstfile != NULL)   {
	fprintf(sstfile, "</sst>\n");
    }

}  /* end of sst_body_end() */



void
sst_cpu_component(char *cpu_id, char *link_id, float weight, FILE *sstfile)
{

    if (sstfile == NULL)   {
	/* Nothing to output */
	return;
    }

    fprintf(sstfile, "    <component id=\"%s\" weight=%f>\n", cpu_id, weight);
    fprintf(sstfile, "        <genericProc>\n");
    fprintf(sstfile, "            <params reference=cpu_params> </params>\n");
    fprintf(sstfile, "            <links>\n");
    fprintf(sstfile, "                <link id=\"%s\">\n", link_id);
    fprintf(sstfile, "                    <params reference=cpu_link_params> </params>\n");
    fprintf(sstfile, "                </link>\n");
    fprintf(sstfile, "            </links>\n");
    fprintf(sstfile, "        </genericProc>\n");
    fprintf(sstfile, "    </component>\n");
    fprintf(sstfile, "\n");

}  /* end of sst_cpu_component() */



void
sst_nic_component(char *nic_id, char *cpu_link_id, char *net_link_id, float weight,
	int nic_rank, FILE *sstfile)
{

    if (sstfile == NULL)   {
	/* Nothing to output */
	return;
    }

    fprintf(sstfile, "    <component id=\"%s\" weight=%f>\n", nic_id, weight);
    fprintf(sstfile, "        <nicmodel>\n");
    fprintf(sstfile, "            <params include=nic_params>\n");
    fprintf(sstfile, "               <rank> %d </rank>\n", nic_rank);
    fprintf(sstfile, "            </params>\n");
    fprintf(sstfile, "            <links>\n");
    fprintf(sstfile, "                <link id=\"%s\">\n", cpu_link_id);
    fprintf(sstfile, "                    <params reference=nic_cpu_link> </params>\n");
    fprintf(sstfile, "                </link>\n");
    fprintf(sstfile, "                <link id=\"%s\">\n", net_link_id);
    fprintf(sstfile, "                    <params reference=nic_net_link> </params>\n");
    fprintf(sstfile, "                </link>\n");
    fprintf(sstfile, "            </links>\n");
    fprintf(sstfile, "        </nicmodel>\n");
    fprintf(sstfile, "    </component>\n");
    fprintf(sstfile, "\n");

}  /* end of sst_nic_component() */



void
sst_router_component_start(char *id, float weight, char *cname, FILE *sstfile)
{

    if (sstfile == NULL)   {
	/* Nothing to output */
	return;
    }

    fprintf(sstfile, "    <component id=\"%s\" weight=%f>\n", id, weight);
    fprintf(sstfile, "        <routermodel>\n");
    fprintf(sstfile, "            <params include=router_params>\n");
    fprintf(sstfile, "                <component_name> %s </component_name>\n", cname);

}  /* end of sst_router_component_start() */



void
sst_router_component_link(char *id, char *link_lat, char *link_name, FILE *sstfile)
{

    if (sstfile == NULL)   {
	/* Nothing to output */
	return;
    }

    fprintf(sstfile, "            <link id=\"%s\">\n", id);
    fprintf(sstfile, "                <params>\n");
    fprintf(sstfile, "                    <lat>%s</lat>\n", link_lat);
    fprintf(sstfile, "                    <name>%s</name>\n", link_name);
    fprintf(sstfile, "                </params>\n");
    fprintf(sstfile, "            </link>\n");

}  /* end of sst_router_component_link() */



void
sst_router_component_end(FILE *sstfile)
{

    if (sstfile == NULL)   {
	/* Nothing to output */
	return;
    }

    fprintf(sstfile, "            </links>\n");
    fprintf(sstfile, "        </routermodel>\n");
    fprintf(sstfile, "    </component>\n");
    fprintf(sstfile, "\n");

}  /* end of sst_router_component_end() */



void
sst_footer(FILE *sstfile)
{

    if (sstfile != NULL)   {
	fprintf(sstfile, "\n");
    }

}  /* end of sst_footer() */



/*
** Generate the NIC components
*/
void
sst_nics(FILE *sstfile)
{

int n, r, p;
char nic_id[MAX_ID_LEN];
char cpu_link_id[MAX_ID_LEN];
char net_link_id[MAX_ID_LEN];
char *label;


    if (sstfile == NULL)   {
	return;
    }

    reset_nic_list();
    while (next_nic(&n, &r, &p, &label))   {
	snprintf(nic_id, MAX_ID_LEN, "nic%d", n);
	snprintf(cpu_link_id, MAX_ID_LEN, "cpu%dnicmodel", n);
	snprintf(net_link_id, MAX_ID_LEN, "Router%dPort%d", r, p);
	sst_nic_component(nic_id, cpu_link_id, net_link_id, 1.0, n, sstfile);
    }

}  /* end of sst_nics() */



/*
** Generate the router components
*/
void
sst_routers(FILE *sstfile)
{

int l, r, p;
char net_link_id[MAX_ID_LEN];
char router_id[MAX_ID_LEN];
char cname[MAX_ID_LEN];


    if (sstfile == NULL)   {
	return;
    }

    reset_router_list();
    while (next_router(&r))   {
	snprintf(router_id, MAX_ID_LEN, "router%d", r);
	snprintf(cname, MAX_ID_LEN, "R%d", r);
	sst_router_component_start(router_id, 0.3, cname, sstfile);
	/*
	** We have to list the links in order in the params section, so the router
	** componentn can get the names and create the appropriate links.
	*/

	/* Link to local NIC(s) */
	reset_router_nics(r);
	while (next_router_nic(r, &p))   {
	    snprintf(net_link_id, MAX_ID_LEN, "Router%dPort%d", r, p);
	    snprintf(cname, MAX_ID_LEN, "Link%dname", p);
	    fprintf(sstfile, "                <%s> %s </%s>\n", cname, net_link_id, cname);
	}

	/* Links to other routers */
	reset_router_links(r);
	while (next_router_link(r, &l, &p))   {
	    snprintf(net_link_id, MAX_ID_LEN, "L%d", l);
	    snprintf(cname, MAX_ID_LEN, "Link%dname", p);
	    fprintf(sstfile, "                <%s> %s </%s>\n", cname, net_link_id, cname);
	}
	fprintf(sstfile, "            </params>\n");
	fprintf(sstfile, "            <links>\n");



	/* Now do it again for the links section */

	/* Link to local NIC(s) */
	reset_router_nics(r);
	while (next_router_nic(r, &p))   {
	    snprintf(net_link_id, MAX_ID_LEN, "Router%dPort%d", r, p);
	    sst_router_component_link(net_link_id, "1ns", net_link_id, sstfile);
	}

	/* Links to other routers */
	reset_router_links(r);
	while (next_router_link(r, &l, &p))   {
	    snprintf(net_link_id, MAX_ID_LEN, "L%d", l);
	    sst_router_component_link(net_link_id, "1ns", net_link_id, sstfile);
	}

	sst_router_component_end(sstfile);
    }

}  /* end of sst_routers() */
