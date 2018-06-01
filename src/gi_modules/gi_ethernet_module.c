/*
 * gi_ethernet_module.c
 *
 *  Created on: 15.05.2018
 *      Author: kaige
 */

#include "gi_modules/gi_ethernet_module.h"

static GI_LINK* links[4];

#define ETH_TASK_STACK_SIZE				512
#define ETH_TASK_PRIO						20
#define ETH_TASK_NAME						"Eth Task"
static OS_STK eth_task_stk[ETH_TASK_STACK_SIZE];

static uint32_t _ethInputBuffer[4*20];
static ETH_SEND_DATA _ethSendDataMem[4*20];
static OS_MEM *_ethSendData;

static struct netif *ethDev;

static void* _ethGlobalProcessFunction(void* data, int inputId);
static int _ethRouter(void* data, int inputId, void* gi_if);

static int _ethDownlinkOutputWrapper(void* p_arg1, void* p_arg2);

static int _ethUplink1Output(void* p_arg1, void* p_arg2);
static int _ethUplink2Output(void* p_arg1, void* p_arg2);
static int _ethUplink3Output(void* p_arg1, void* p_arg2);

/* just to simulate a static arp ip module */
void testIPInput(struct pbuf *p, struct netif *inp){
	pbuf_free(p);
}

typedef enum {
	DISCARD = 0,
	IPv4,
	ARP
}ETH_TYPE;
static ETH_TYPE pbuf_data_type = DISCARD;
static struct pbuf *p_pbuf;
static struct netif *p_input;

/**
 * Init Function
 */
void GI_Ethernet_Init(void *netif){
	uint8_t err;
	ethDev = netif;

	_ethSendData = OSMemCreate(_ethSendDataMem,4*20,sizeof(ETH_SEND_DATA),&err);

	GI_TASK_DATA taskData;
	taskData.p_taskStack = eth_task_stk;
	taskData.prio = ETH_TASK_PRIO;
	taskData.stackSize = ETH_TASK_STACK_SIZE;

	GI_AddInterface(0, NULL, _ethGlobalProcessFunction, _ethRouter, &taskData);

	/* Downlink, Ethernet MAC */
	links[0] = GI_AddQueueLink(0,0,(void*)&_ethInputBuffer[0*20],20,_ethDownlinkOutputWrapper);

	/* One uplink for critical IP module with static ARP */
	links[1] = GI_AddQueueLink(0,1,(void*)&_ethInputBuffer[1*20],20, _ethUplink1Output);

	/* Two uplinks for non-critical IP module (links[2]) with dynamic ARP (links[3]) */
	links[2] = GI_AddQueueLink(0,2,(void*)&_ethInputBuffer[2*20],20, _ethUplink2Output);
	links[3] = GI_AddQueueLink(0,3,(void*)&_ethInputBuffer[3*20],20, _ethUplink3Output);
}

/**
 * Process Function
 */
static void* _ethGlobalProcessFunction(void* data, int inputId){
	if(inputId == 0){
		/* MAC to ip direction */
		struct pbuf *p = low_level_input(ethDev);
		ethernet_input(p,ethDev);
	} else {
		/* ip to mac direction */
		ETH_SEND_DATA *p_txdata = (ETH_SEND_DATA *)data;
		ethernet_output(p_txdata->netif,p_txdata->p,p_txdata->src,p_txdata->dst,p_txdata->eth_type);
		OSMemPut(_ethSendData,p_txdata);
	}
}

/**
 * Ouput Router Function
 */
static struct eth_addr addresses[] = {
		ETH_ADDR(MAC_ADDR0,MAC_ADDR1,MAC_ADDR2,MAC_ADDR3,0x00,0x02),
		ETH_ADDR(MAC_ADDR0,MAC_ADDR1,MAC_ADDR2,MAC_ADDR3,0x00,0x04),
};

static int _ethRouter(void* data, int inputId, void* gi_if){
	if(inputId == 0){
		/* MAC to IP is routed */
		struct eth_hdr *ethhdr = (struct eth_hdr *)p_pbuf;

		int uplinkMACId = 0;
		for (uplinkMACId = 0; uplinkMACId < 2; uplinkMACId++){
			if(eth_addr_cmp(&ethhdr->dest,&addresses[uplinkMACId]))
				break;
		}
		if(uplinkMACId == 0){
			/* critical traffic with static arp, ignore arp packets */
			if(pbuf_data_type == ARP){
				pbuf_free(p_pbuf);
				return -1;
			} else {
				return 1;
			}
		} else {
			if(pbuf_data_type == IPv4){
				return 2;
			} else {
				return 3;
			}
		}
	} else {
		/* IP to mac is not routed */
		return -1;
	}
}

/**
 * Wrapper Functions called by ethernet module
 */


/**
 * Link Functions
 */
int ethDownlinkInputFunction(void* p_arg1, void* p_arg2){
	links[0]->_inputFkt(links[0],NULL);
}

static int _ethDownlinkOutputWrapper(void* p_arg1, void* p_arg2){
	// Empty, called low-level-output in ethernet_output
}

static int _ethUplink1Output(void* p_arg1, void* p_arg2){
	struct pbuf *p = (struct pbuf*)p_arg1;
	pbuf_free(p);
}

static int _ethUplink2Output(void* p_arg1, void* p_arg2){
	struct netif *netif = ethDev;
	struct pbuf *p = (struct pbuf*)p_arg1;
	if (ip4_input_wrapper(p,netif) != ERR_OK )
	{
		pbuf_free(p);
	}
}

static int _ethUplink3Output(void* p_arg1, void* p_arg2){
	struct netif *netif = ethDev;
	struct pbuf *p = (struct pbuf*)p_arg1;
	if (etharp_input_wrapper(p,netif) != ERR_OK )
	{
		pbuf_free(p);
	}
}

err_t eth_ip4_input_wrapper(struct pbuf *p, struct netif *inp){
	p_pbuf =  p;
	p_input = inp;
	pbuf_data_type = IPv4;
}

err_t eth_etharp_input_wrapper(struct pbuf *p, struct netif *inp){
	p_pbuf = p;
	p_input = inp;
	pbuf_data_type = ARP;
}

err_t ethernet_output_wrapper(struct netif * netif, struct pbuf * p,
				const struct eth_addr * src, const struct eth_addr * dst,
				u16_t eth_type){
	uint8_t err;
	ETH_SEND_DATA *p_data = OSMemGet(_ethSendData, &err);
	if(p_data != (ETH_SEND_DATA*)0){
		p_data->netif = netif;
		p_data->p = p;
		p_data->src = src;
		p_data->dst = dst;
		p_data->eth_type = eth_type;
		links[1]->_inputFkt(links[1],p_data);
		return ERR_OK;
	} else {
		pbuf_free(p);
		return ERR_MEM;
	}
}
