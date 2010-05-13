/*
** $Id: gen.h,v 1.7 2010/05/13 19:27:23 rolf Exp $
**
** Rolf Riesen, April 2010, Sandia National Laboratories
**
*/
#ifndef _GEN_H_
#define _GEN_H_

#define FALSE		(0)
#define TRUE		(1)

void gen_nic(int rank, int router, int port);
void gen_router(int id, int num_ports);
void gen_link(int Arouter, int Aport, int Brouter, int Bport);

void reset_router_list(void);
int next_router(int *id);
void reset_router_nics(int router);
int next_router_nic(int router, int *port);
void reset_router_links(int router);
int next_router_link(int router, int *link_id, int *port);

void reset_nic_list(void);
int next_nic(int *id, int *router, int *port, char **label);
void reset_link_list(void);
int next_link(int *Arouter, int *Aport, int *Brouter, int *Bport, char **label);

int get_num_nics(void);
int get_num_routers(void);
int get_num_ports(void);
int get_num_links(void);

#endif /* _GEN_H_ */
