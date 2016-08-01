// lwipk66.h
#include <string.h>
#include <stdio.h>
#include "lwip/netif.h"
#include "lwip/timers.h"
#include "lwip/inet.h"
#include "lwip/dhcp.h"
#include "lwip/stats.h"
#include "netif/etharp.h"
#include "kinetis.h"
#include "core_pins.h"

#ifdef __cplusplus
extern "C" {
#endif
void ether_init(const char *ipaddr, const char *netmask, const char *gw);
int ether_init_dhcp();
void ether_poll();
void ether_delay(uint32_t ms);
uint32_t ether_get_ipaddr();
#ifdef __cplusplus
}
#endif
