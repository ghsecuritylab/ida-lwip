/*
 * gi_ethernet_module.c
 *
 *  Created on: 15.05.2018
 *      Author: kaige
 */

#include "gi_modules/gi_ipv4_module.h"

static GI_LINK* links[4];

#define IPV4_TASK_STACK_SIZE				512
#define IPV4_TASK_PRIO						21
#define IPV4_TASK_NAME						"IPv4 Task"
static OS_STK ipv4_task_stk[IPV4_TASK_STACK_SIZE];

static uint32_t _ipv4InputBuffer[4*20];
static IPv4_SEND_DATA _ipv4SendDataMem[4*20];
static OS_MEM *_ipv4SendData;

static struct netif *ethDev;

static void* _ipv4GlobalProcessFunction(void* data, int inputId);
static int _ipv4Router(void* data, int inputId, void* gi_if);

static int _ipv4DownlinkOutputWrapper(void* p_arg1, void* p_arg2);
static int _ipv4Downlink2OutputWrapper(void* p_arg1, void* p_arg2);

static int _ipv4UdpLinkOutput(void* p_arg1, void* p_arg2);
static int _ipv4TcpLinkOutput(void* p_arg1, void* p_arg2);

typedef enum {
	DISCARD = 0,
	UDP,
	TCP
}IP_TYPE;
static IP_TYPE pbuf_data_type = DISCARD;
static struct pbuf *p_pbuf;
static struct netif *p_input;

/**
 * Init Function
 */
void GI_IPv4_Init(void *netif){
	uint8_t err;
	ethDev = netif;

	_ipv4SendData = OSMemCreate(_ipv4SendDataMem,4*20,sizeof(IPv4_SEND_DATA),&err);

	GI_TASK_DATA taskData;
	taskData.p_taskStack = ipv4_task_stk;
	taskData.prio = IPV4_TASK_PRIO;
	taskData.stackSize = IPV4_TASK_STACK_SIZE;

	GI_AddInterface(0, NULL, _ipv4GlobalProcessFunction, _ipv4Router, &taskData);

	/* Downlink, ETH Module */
	links[0] = GI_AddQueueLink(1,0,(void*)&_ipv4InputBuffer[0*20],20,_ipv4DownlinkOutputWrapper);
	links[1] = GI_AddQueueLink(1,1,(void*)&_ipv4InputBuffer[1*20],20,_ipv4Downlink2OutputWrapper);


	/* One uplink for UDP module ARP */
	links[2] = GI_AddQueueLink(1,1,(void*)&_ipv4InputBuffer[2*20],20, _ipv4UdpLinkOutput);

	/* One uplink for TCP module ARP */
	links[3] = GI_AddQueueLink(1,2,(void*)&_ipv4InputBuffer[3*20],20, _ipv4TcpLinkOutput);
}

/**
 * Process Function
 */
static void* _ipv4GlobalProcessFunction(void* data, int inputId){
	if(inputId == 0){
		/* IP to TCPUDP direction */
//		struct pbuf *p = low_level_input(ethDev);
//		ethernet_input(p,ethDev);
	} else {
		/* TCPUDP to IP direction */
//		ETH_SEND_DATA *p_txdata = (ETH_SEND_DATA *)data;
//		ethernet_output(p_txdata->netif,p_txdata->p,p_txdata->src,p_txdata->dst,p_txdata->eth_type);
	}
}

static int _ipv4Router(void* data, int inputId, void* gi_if){
	if(inputId == 0){
		/* IP to UDPTCP is routed */
		struct ip_hdr *iphdr = (struct ip_hdr *)p_pbuf;
		switch(iphdr->_proto){
		case IP_PROTO_UDP:
			return 1;
		case IP_PROTO_TCP:
			return 2;
		default:
			pbuf_free(p_pbuf);
			return -1;
		}
	} else {
		/* down is not routed??? */
		return -1;
	}
}

/**
 * Wrapper Functions called by ethernet module
 */


/**
 * Link Functions
 */

err_t ip4_input_wrapper(struct pbuf *p, struct netif *inp){
	return (err_t)links[0]->_inputFkt(p,inp);
}

err_t etharp_input_wrapper(struct pbuf *p, struct netif *inp){
	return (err_t)links[1]->_inputFkt(p,inp);
}

static int _ipv4DownlinkOutputWrapper(void* p_arg1, void* p_arg2){
	// Empty, called low-level-output in ethernet_output
}

static int _ipv4Downlink2OutputWrapper(void* p_arg1, void* p_arg2){
	// Empty, called low-level-output in ethernet_output
}


static int _ipv4UdpLinkOutput(void* p_arg1, void* p_arg2){
	struct netif *netif = ethDev;
	struct pbuf *p = (struct pbuf*)p_arg1;
	if (udp_input(p,netif) != ERR_OK )
	{
		pbuf_free(p);
	}
}

static int _ipv4TcpLinkOutput(void* p_arg1, void* p_arg2){
	struct netif *netif = ethDev;
	struct pbuf *p = (struct pbuf*)p_arg1;
	if (tcp_input(p,netif) != ERR_OK )
	{
		pbuf_free(p);
	}
}

err_t ipv4_udp_input_wrapper(struct pbuf *p, struct netif *inp){
	p_pbuf =  p;
	p_input = inp;
	pbuf_data_type = UDP;
}

err_t ipv4_tcp_input_wrapper(struct pbuf *p, struct netif *inp){
	p_pbuf = p;
	p_input = inp;
	pbuf_data_type = TCP;
}

err_t ip4_output_wrapper(struct pbuf *p, const ip4_addr_t *src, const ip4_addr_t *dest,
                  u8_t ttl, u8_t tos,
                  u8_t proto, GI_LINK* gi_link)
{
	uint8_t err;
	IPv4_SEND_DATA *p_data = OSMemGet(_ipv4SendData, &err);
	if(p_data != (IPv4_SEND_DATA*)0){
		p_data->p = p;
		p_data->src = src;
		p_data->dest = dest;
		p_data->ttl = ttl;
		p_data->tos = tos;
		p_data->proto = proto;
		gi_link->_inputFkt(gi_link,p_data);
		return ERR_OK;
	} else {
		pbuf_free(p);
		return ERR_MEM;
	}
}

err_t ip4_output_wrapper_udp(struct pbuf *p, const ip4_addr_t *src, const ip4_addr_t *dest,
                  u8_t ttl, u8_t tos,
                  u8_t proto, struct netif *netif){
	ip4_output_wrapper(p,src,dest,ttl,tos,proto,links[1]);
}

err_t ip4_output_wrapper_tcp(struct pbuf *p, const ip4_addr_t *src, const ip4_addr_t *dest,
        u8_t ttl, u8_t tos,
        u8_t proto, struct netif *netif){
	ip4_output_wrapper(p,src,dest,ttl,tos,proto,links[2]);
}


