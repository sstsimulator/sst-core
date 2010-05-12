/*
** $Id: sst_gen.h,v 1.9 2010/04/27 23:17:19 rolf Exp $
**
** Rolf Riesen, March 2010, Sandia National Laboratories
**
*/
#ifndef _SST_GEN_H_
#define _SST_GEN_H_

#include <stdio.h>



void sst_header(FILE *sstfile);
void sst_footer(FILE *dotfile);

void sst_cpu_param(FILE *sstfile, char *freq, char *exec, int
	cpu_verbose, int cpu_debug, char *nic_lat);

void sst_router_param_start(FILE *sstfile, int num_ports);
void sst_router_param_end(FILE *sstfile);

void sst_nic_param_start(FILE *sstfile, int nic_debug);
void sst_param_entry(FILE *sstfile, char *key, char *value);
void sst_nic_param_topology(FILE *sstfile);
void sst_nic_param_end(FILE *sstfile, char *nic_cpu_lat, char *nic_net_lat);

void sst_body_end(FILE *sstfile);
void sst_body_start(FILE *sstfile);

void sst_cpu_component(char *cpu_id, char *link_id, float weight, FILE *sstfile);
void sst_nic_component(char *nic_id, char *cpu_link_id, char *net_link_id, float weight,
	int nic_rank, FILE *sstfile);
void sst_router_component_start(char *id, float weight, char *cname, FILE *sstfile);
void sst_router_component_end(FILE *sstfile);
void sst_router_component_link(char *id, char *link_lat, char *link_name, FILE *sstfile);

void sst_nics(FILE *sstfile);
void sst_routers(FILE *sstfile);

#endif /* _SST_GEN_H_ */
