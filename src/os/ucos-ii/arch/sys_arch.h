/*
 * sys_arch.h
 *
 *  Created on: Sep 13, 2012
 *      Author: matthiasb
 */

#ifndef SYS_ARCH_H_
#define SYS_ARCH_H_

#include "ucos_ii.h"
#include "stdint.h"

#define SYS_MBOX_NULL   NULL
#define SYS_SEM_NULL    NULL

typedef OS_EVENT * sys_sem_t;
typedef OS_EVENT * sys_mbox_t;
typedef uint8_t sys_thread_t;

typedef OS_CPU_SR sys_prot_t;
sys_prot_t sys_arch_protect(void);
void sys_arch_unprotect(sys_prot_t pval);


/* Bit-Positions of Errors in the sysArchError Variable
 *
 */
#define SYS_INIT_ERR					0
#define SYS_SEM_NEW_ERR				1
#define SYS_SEM_FREE_ERR				2
#define SYS_SEM_SIGNAL_ERR			3
#define SYS_ARCH_SEM_WAIT_ERR		4
#define SYS_MBOX_NEW_MALLOC_ERR		5
#define SYS_MBOX_NEW_QCREATE_ERR		6
#define SYS_MBOX_FREE_ERR			7
#define SYS_MBOX_POST_ERR			8
#define SYS_ARCH_MBOX_FETCH_ERR		9
#define SYS_MBOX_TRYPOST_ERR			10
#define SYS_THREAD_NEW_PRIO_ERR		11
#define SYS_THREAD_NEW_STACK_ERR		12
#define SYS_THREAD_NEW_CREATE_ERR		13
#define SYS_THREAD_NEW_NAME_ERR		14

uint32_t getSysArchError();

#endif /* SYS_ARCH_H_ */
