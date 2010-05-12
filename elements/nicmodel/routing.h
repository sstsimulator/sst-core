#ifndef _NETSIM_ROUTING_H
#define _NETSIM_ROUTING_H

#ifdef __cplusplus
extern "C" {
#endif 

void init_NIC_table(int num_NICs);
int check_NIC_table(void);
void NIC_table_insert_router(int rank, int num_item);
void NIC_table_insert_port(int rank, int num_item);
void init_Adj_Matrix(int num_routers);
void Adj_Matrix_insert(int link, int left_router, int left_port, int right_router, int right_port);
void Adj_Matrix_print(void);
void gen_routes(int my_rank, int my_router, int debug);

#ifdef __cplusplus
}
#endif 

#endif /* _NETSIM_ROUTING_H */
