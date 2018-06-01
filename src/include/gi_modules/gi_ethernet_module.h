/*
 * gi_ethernet_module.h
 *
 *  Created on: 15.05.2018
 *      Author: kaige
 */

#ifndef SRC_MODULES_IDA_LWIP_SRC_INCLUDE_GI_MODULES_GI_ETHERNET_MODULE_H_
#define SRC_MODULES_IDA_LWIP_SRC_INCLUDE_GI_MODULES_GI_ETHERNET_MODULE_H_

#include "gi.h"
#include "ethernetif.h"
#include "netif/ethernet.h"
#include "stm32f7xx_hal_conf.h"

typedef struct{
	struct netif * netif;
	struct pbuf * p;
	const struct eth_addr * src;
	const struct eth_addr * dst;
	u16_t eth_type;
}ETH_SEND_DATA;


void GI_Ethernet_Init(void *netif);

#endif /* SRC_MODULES_IDA_LWIP_SRC_INCLUDE_GI_MODULES_GI_ETHERNET_MODULE_H_ */
