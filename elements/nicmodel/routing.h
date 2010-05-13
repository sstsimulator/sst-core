#ifndef _NETSIM_ROUTING_H
#define _NETSIM_ROUTING_H

#ifdef __cplusplus
extern "C" {
#endif 

int *get_route(int dst, void *vrinfo);
int check_NIC_table(void *vrinfo);
void NIC_table_insert_router(int rank, int num_item, void *vrinfo);
void NIC_table_insert_port(int rank, int num_item, void *vrinfo);
void *init_routing(int num_routers, int num_NICs);
void Adj_Matrix_insert(int link, int left_router, int left_port, int right_router, int right_port, void *vrinfo);
void Adj_Matrix_print(void *vrinfo);
void gen_routes(int my_rank, int my_router, int debug, void *vrinfo);

#ifdef __cplusplus
}
#endif 

#endif /* _NETSIM_ROUTING_H */
