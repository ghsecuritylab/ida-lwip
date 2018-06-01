/*
 * gi_ipv4_module.h
 *
 *  Created on: 22.05.2018
 *      Author: kaige
 */

#ifndef SRC_MODULES_IDA_LWIP_SRC_INCLUDE_GI_MODULES_GI_IPV4_MODULE_H_
#define SRC_MODULES_IDA_LWIP_SRC_INCLUDE_GI_MODULES_GI_IPV4_MODULE_H_

#include "gi.h"
#include "lwip/ip.h"
#include "lwip/ip4.h"

typedef struct{
	struct pbuf *p;
	const ip4_addr_t *src;
	const ip4_addr_t *dest;
	u8_t ttl;
	u8_t tos;
	u8_t proto;
	struct netif *netif;
}IPv4_SEND_DATA;

void GI_IPv4_Init(void *netif);

#endif /* SRC_MODULES_IDA_LWIP_SRC_INCLUDE_GI_MODULES_GI_IPV4_MODULE_H_ */
