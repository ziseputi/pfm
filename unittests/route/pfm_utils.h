#ifndef __PFM_UTILS_H__
#define __PFM_UTILS_H__ 1
#include "pfm.h"

ipv4_addr_t pfm_str2ip(const char *ip_str);
const char *pfm_ip2str(ipv4_addr_t iph,char *ip_addr_str);

#endif


