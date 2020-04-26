#ifndef __LINK_H__
#define __LINK_H__ 1
#include "pfm.h"
#include "pfm_comm.h"

int link_read(struct rte_mbuf *rx_pkts[],uint16_t burst_size,uint16_t *port);

/* Application needs to implement the following callback functions */
void link_state_change_callback(int por_id, ops_state_t ops_state);
void link_state_change(const int link_id,const ops_state_t desired_state);

#endif