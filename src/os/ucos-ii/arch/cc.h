/*
 * cc.h
 *
 *  Created on: Sep 13, 2012
 *      Author: matthiasb
 */

#ifndef CC_H_
#define CC_H_

#include <sys/time.h>
//#include <stdio.h>
//#include <stdlib.h>
//#include <stdint.h>
//#include <string.h>
//#include <sys/time.h>
//#include <cpu.h>
//
//// Includes definition of mch_printf macro to do printf
////#include <lpc_types.h>
//#include <ucos_ii.h>

//#ifdef DEBUG
//#undef DEBUG
//#endif
//#define DEBUG DEBUG_LVL_INFO
////#include "utilities/debug.h"
//
#ifndef BYTE_ORDER
#define BYTE_ORDER  LITTLE_ENDIAN
#endif

/**
 * Randon number
 * returns 4, choosen by fair dice role
 */
#define LWIP_RAND() ((u32_t)4)
//
//typedef CPU_INT08U     u8_t;
//typedef CPU_INT08S     s8_t;
//typedef CPU_INT16U     u16_t;
//typedef CPU_INT16S     s16_t;
//typedef CPU_INT32U     u32_t;
//typedef CPU_INT32S     s32_t;
//
//typedef uintptr_t   mem_ptr_t;
//
//#define LWIP_ERR_T  int
//
///* Define (sn)printf formatters for these lwIP types */
//#define U16_F "hu"
//#define S16_F "d"
//#define X16_F "hx"
//#define U32_F "u"
//#define S32_F "d"
//#define X32_F "x"
//#define SZT_F "uz"
//
///* Define the checksum-algorithm to use {1...3} */
//#define LWIP_CHKSUM_ALGORITHM 2
//
///* Compiler hints for packing structures */
//#define PACK_STRUCT_FIELD(x)    x
//#define PACK_STRUCT_STRUCT  __attribute__((packed))
//#define PACK_STRUCT_BEGIN
//#define PACK_STRUCT_END
//
///* Plaform specific diagnostic output */
//#define LWIP_PLATFORM_DIAG(x)			;
//#define LWIP_PLATFORM_ASSERT(X)			;

//#define LWIP_PLATFORM_DIAG(x)   do {                \
//		DEBUG_STR_LVL(DEBUG_LVL_INFO,x);                   \
//    } while (0)
//
#define LWIP_PLATFORM_ASSERT(x) do {                \
    } while (0)

#endif /* CC_H_ */
