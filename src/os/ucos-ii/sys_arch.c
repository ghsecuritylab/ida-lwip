/*
 * sys_arch.c
 *
 *  Created on: Dec 19, 2012
 *      Author: kaige
 */

/*sys_arch interface for lwIP 0.6++

 Author: Adam Dunkels

 The operating system emulation layer provides a common interface
 between the lwIP code and the underlying operating system kernel. The
 general idea is that porting lwIP to new architectures requires only
 small changes to a few header files and a new sys_arch
 implementation. It is also possible to do a sys_arch implementation
 that does not rely on any underlying operating system.

 The sys_arch provides semaphores and mailboxes to lwIP. For the full
 lwIP functionality, multiple threads support can be implemented in the
 sys_arch, but this is not required for the basic lwIP
 functionality. Previous versions of lwIP required the sys_arch to
 implement timer scheduling as well but as of lwIP 0.5 this is
 implemented in a higher layer.

 In addition to the source file providing the functionality of sys_arch,
 the OS emulation layer must provide several header files defining
 macros used throughout lwip.  The files required and the macros they
 must define are listed below the sys_arch description.

 Semaphores can be either counting or binary - lwIP works with both
 kinds. Mailboxes are used for message passing and can be implemented
 either as a queue which allows multiple messages to be posted to a
 mailbox, or as a rendez-vous point where only one message can be
 posted at a time. lwIP works with both kinds, but the former type will
 be more efficient. A message in a mailbox is just a pointer, nothing
 more.

 Semaphores are represented by the type "sys_sem_t" which is typedef'd
 in the sys_arch.h file. Mailboxes are equivalently represented by the
 type "sys_mbox_t". lwIP does not place any restrictions on how
 sys_sem_t or sys_mbox_t are represented internally.

 */

#include "arch/sys_arch.h"

#include "ucos_ii.h"
#include "lwip/err.h"
#include "tlsf.h"
#include "lwip/sys.h"
#include "lwip/def.h"
#include "lwip/sys.h"
#include "lwip/mem.h"

#include "arch/cc.h"
//#include "lwip/timers.h"

//Some ram-space we need to allocate memory for tasks and mailboxes
#define LWIP_TLSF_HEAP_SIZE 	4096

uint32_t tlsfHeap[LWIP_TLSF_HEAP_SIZE];

//This Variable stores one bit for every possible error that may occur. So you get an overview how many problems you have
uint32_t sysArchError = 0;


/*******************************************************************************************************/
/* System																											    */
/*******************************************************************************************************/
/*
 * --void sys_init(void) --
 *
 * lwIP system initialization. This function is called before the any other sys_arch-function is called
 * and is meant to be used to initialize anything that has to be up and running for the rest of the functions to work.
 * for example to set up a poo
 * l of semaphores.
 */
void sys_init(void)
{
	//Initialize TLSF Heap.
	//Be carefull: TLSF uses Bytes, ucosii is configured to use 32bit words.
	int i = 0;
	for(i = 0; i < 100; i++){
		tlsfHeap[i] = i;
	}
	size_t heapSize = init_memory_pool(LWIP_TLSF_HEAP_SIZE * sizeof(uint32_t), tlsfHeap);
	if (heapSize == 0)
		sysArchError |= (1 << SYS_INIT_ERR);
}
/*
 * -- uint32_t sys_now(void) --
 *
 * Returns the System-Time in Milliseconds
 */
u32_t sys_now(void)
{
	return OSTimeGet() / (OS_TICKS_PER_SEC / 1000);
}

/*******************************************************************************************************/
/* Mailboxes																											*/
/*
 * Semaphores can be either counting or binary - lwIP works with both kinds.
 * Semaphores are represented by the type sys_sem_t which is typedef'd in the sys_arch.h file.
 * lwIP does not place any restrictions on how sys_sem_t should be defined or represented internally,
 * but typically it is a pointer to an operating system semaphore or a struct wrapper
 * for an operating system semaphore.
 * Warning! Semaphores are used in lwip's memory-management functions (mem_malloc(), mem_free(), etc.)
 * and can thus not use these functions to create and destroy them.
 */
/*******************************************************************************************************/

/*
 * -- err_t sys_sem_new(sys_sem_t *sem, u8_t count) --
 *
 * Creates a new semaphore returns it through the sem pointer provided as argument to the function,
 * in addition the function returns ERR_MEM if a new semaphore could not be created
 * and ERR_OK if the semaphore was created.
 * The count argument specifies the initial state of the semaphore.
 */
err_t sys_sem_new(sys_sem_t *sem, u8_t count)
{
	*sem = (sys_sem_t) OSSemCreate(count);
	if (*sem == NULL){
		sysArchError |= (1 << SYS_SEM_NEW_ERR);
		return ERR_MEM;
	}
	return ERR_OK;
}

/*
 * 	-- void sys_sem_free(sys_sem_t *sem) --
 *
 * 	Frees a semaphore created by sys_sem_new.
 * 	Since these two functions provide the entry and exit point for all semaphores used by lwIP,
 * 	you have great flexibility in how these are allocated and deallocated
 * 	(for example, from the heap, a memory pool, a semaphore pool, etc).
 */
void sys_sem_free(sys_sem_t *sem)
{
	uint8_t err;
	OSSemDel(*sem, OS_DEL_ALWAYS, &err);
	if(err != OS_ERR_NONE)
		sysArchError |= (1 << SYS_SEM_FREE_ERR);
}

/*
 * -- void sys_sem_signal(sys_sem_t *sem) --
 *
 * Signals (or releases) a semaphore referenced by * sem.
 */
void sys_sem_signal(sys_sem_t *sem)
{
	uint8_t err = OSSemPost(*sem);
	if(err !=OS_ERR_NONE)
		sysArchError |= (1 << SYS_SEM_SIGNAL_ERR);
}

/*
 * -- u32_t sys_arch_sem_wait(sys_sem_t *sem, u32_t timeout) --
 *
 * Blocks the thread while waiting for the semaphore to be signaled.
 * The timeout parameter specifies how many milliseconds the function should block before returning;
 * if the function times out, it should return SYS_ARCH_TIMEOUT.
 * If timeout=0, then the function should block indefinitely.
 * If the function acquires the semaphore, it should return how many milliseconds expired while waiting for the semaphore.
 * The function may return 0 if the semaphore was immediately available.
 */
u32_t sys_arch_sem_wait(sys_sem_t *sem, u32_t timeout)
{
	uint32_t startTime = sys_now();
	uint8_t err;
	uint32_t ticks = timeout * (OS_TICKS_PER_SEC/1000);
	if(ticks > 0xFFFF)
		ticks = 0xFFFF;
	OSSemPend(*sem, (uint16_t)ticks, &err);
	if (err == OS_ERR_NONE)
		return sys_now() - startTime;
	else{
//		sysArchError |= (1 << SYS_ARCH_SEM_WAIT_ERR);
		return SYS_ARCH_TIMEOUT;
	}

}

/*
 * -- int sys_sem_valid(sys_sem_t *sem) --
 *
 * Checks if a given Pointer to a Semaphore is valid.
 */
int sys_sem_valid(sys_sem_t *sem)
{
	if (*sem != NULL)
	{
		return 1;
	}
	return 0;
}

/*
 * -- sys_sem_set_invalid(sys_sem_t *sem) --
 *
 * Sets a given Pointer to a Semaphore invalid by setting it to zero.
 */
void sys_sem_set_invalid(sys_sem_t *sem)
{
	*sem = NULL;
}

/*******************************************************************************************************/
/* Mailboxes																											*/
/*
 * Mailboxes are used for message passing and can be implemented either as a queue
 * which allows multiple messages to be posted to a mailbox,
 * or as a rendez-vous point where only one message can be posted at a time.
 * lwIP works with both kinds, but the former type will be more efficient.
 * A message in a mailbox is just a pointer, nothing more.
 * Mailboxes are equivalently represented by the type sys_mbox_t.
 * lwIP does not place any restrictions on how sys_mbox_t is represented internally,
 * but it is typically a structure pointer type to a structure
 * that wraps the OS-native mailbox type and its queue buffer.
 *
 */
/*******************************************************************************************************/

/*
 * -- err_t sys_mbox_new(sys_mbox_t * mbox, int size) --
 *
 * Trys to create a new mailbox and return it via the mbox pointer provided as argument to the function.
 * Returns ERR_OK if a mailbox was created and ERR_MEM if the mailbox on error.
 */
err_t sys_mbox_new(sys_mbox_t * mbox, int size)
{
	uint32_t *mem;
	mem = malloc_ex(size * sizeof(uint32_t), tlsfHeap);
	if (mem == NULL)
	{
		sysArchError |= (1 << SYS_MBOX_NEW_MALLOC_ERR);
		return ERR_MEM;
	}

	*mbox = OSQCreate((void*) mem, size);
	if (*mbox == NULL)
	{
		sysArchError |= (1 << SYS_MBOX_NEW_QCREATE_ERR);
		return ERR_MEM;
	}
	return ERR_OK;
}

/*
 * -- void sys_mbox_free(sys_mbox_t * mbox) --
 *
 * Deallocates a mailbox.
 * If there are messages still present in the mailbox when the mailbox is deallocated,
 * it is an indication of a programming error in lwIP and the developer should be notified.
 */
void sys_mbox_free(sys_mbox_t * mbox)
{
	uint8_t err;
	OS_EVENT *mbox_p = *mbox;
	OS_Q * event_p = mbox_p->OSEventPtr;
	uint32_t *mem = (uint32_t*) event_p->OSQStart;
	OSQDel(*mbox, OS_DEL_ALWAYS, &err);
	free_ex(mem, tlsfHeap);
	if(err != OS_ERR_NONE)
		sysArchError |= (1 << SYS_MBOX_FREE_ERR);
}

/*
 * -- void sys_mbox_post(sys_mbox_t * mbox, void *msg) --
 *
 * Posts the "msg" to the mailbox
 */
void sys_mbox_post(sys_mbox_t * mbox, void *msg)
{
	uint8_t err = OS_ERR_NONE;
	err = OSQPost(*mbox, msg);
	if(err != OS_ERR_NONE)
		sysArchError |= (1 << SYS_MBOX_POST_ERR);
}

/*
 * -- u32_t sys_arch_mbox_fetch(sys_mbox_t * mbox, void **msg, u32_t timeout) --
 *
 * Blocks the thread until a message arrives in the mailbox,
 * but does not block the thread longer than timeout milliseconds (similar to the sys_arch_sem_wait() function).
 * The msg argument is a pointer to the message in the mailbox and may be NULL to indicate that the message should be dropped.
 * This should return either SYS_ARCH_TIMEOUT or the number of milliseconds elapsed waiting for a message.
 */
u32_t sys_arch_mbox_fetch(sys_mbox_t * mbox, void **msg, u32_t timeout)
{
	uint32_t startTime = sys_now();
	uint8_t err;
	uint32_t ticks = (timeout * OS_TICKS_PER_SEC)/1000;
	if(ticks > 0xFFFF)
		ticks = 0xFFFF;
	*msg = OSQPend(*mbox, (uint16_t)ticks, &err);
	if (err == OS_ERR_NONE)
		return sys_now() - startTime;
	else{
//		sysArchError |= (1 << SYS_ARCH_MBOX_FETCH_ERR);
		return SYS_ARCH_TIMEOUT;
	}
}

/*
 * -- u32_t sys_arch_mbox_tryfetch(sys_mbox_t * mbox, void **msg) --
 *
 * This is similar to sys_arch_mbox_fetch, however if a message is not present in the mailbox,
 * it immediately returns with the code SYS_MBOX_EMPTY.
 * On success 0 is returned with msg pointing to the message retrieved from the mailbox.
 * If your implementation cannot support this functionality,
 * a simple workaround (but inefficient) is #define sys_arch_mbox_tryfetch(mbox,msg) sys_arch_mbox_fetch(mbox,msg,1).
 */
u32_t sys_arch_mbox_tryfetch(sys_mbox_t * mbox, void **msg)
{
	uint8_t err;
	*msg = OSQAccept(*mbox, &err);
	if (err == OS_ERR_NONE)
		return 0;
	else
		return SYS_MBOX_EMPTY;
}

/*
 * -- err_t sys_mbox_trypost(sys_mbox_t * mbox, void *msg) --
 *
 * Tries to post a message to mbox by polling (no timeout).
 * The function returns ERR_OK on success and ERR_MEM if it can't post at the moment.
 *
 */
err_t sys_mbox_trypost(sys_mbox_t * mbox, void *msg)
{
	int err = OSQPost(*mbox, msg);
	if(err == OS_ERR_NONE)
		return ERR_OK;
	else {
		sysArchError |= (1 << SYS_MBOX_TRYPOST_ERR);
		return ERR_MEM;
	}
}

/*
 * -- int sys_mbox_valid(sys_mbox_t *mbox) --
 *
 * Checks if a given Pointer to a MessageBox is valid or not
 */
int sys_mbox_valid(sys_mbox_t *mbox)
{
	if (*mbox != NULL)
	{
		return 1;
	}
	return 0;
}

/*
 * -- sys_mbox_set_invalid(sys_mbox_t *mbox) --
 *
 * Sets a given Pointer to a MessageBox invalid by setting it to zero
 */
void sys_mbox_set_invalid(sys_mbox_t *mbox)
{
	*mbox = NULL;
}

/*******************************************************************************************************/
/* Threads																											*/
/*
 * Threads are not required for lwIP, although lwIP is written to use them efficiently.
 * The following function will be used to instantiate a thread for lwIP
 *
 */
/*******************************************************************************************************/

/*
 * -- sys_thread_t sys_thread_new(char *name, void (* thread)(void *arg), void *arg, int stacksize, int prio) --
 *
 * name is the thread name. thread(arg) is the call made as the thread's entry point.
 * stacksize is the recommanded stack size for this thread. -> stacksize is in sizeof(OS_STCK) * Bytes
 * prio is the priority that lwIP asks for.
 * Stack size(s) and priority(ies) have to be are defined in lwipopts.h,
 * and so are completely customizable for your system.
 */
sys_thread_t sys_thread_new(const char *name, void(* thread)(void *arg), void *arg, int stacksize, int prio)
{
	OS_STK *taskStack;
	uint8_t err;

	if (prio == 0 || prio >= OS_TASK_IDLE_PRIO)
	{
		sysArchError |= (1 << SYS_THREAD_NEW_PRIO_ERR);
		return 0;
	}

	//Allocate Stack from LWIP-Heap
	taskStack = (OS_STK*) malloc_ex(stacksize * sizeof(OS_STK), tlsfHeap);
	if (taskStack == NULL)
	{
		sysArchError |= (1 << SYS_THREAD_NEW_STACK_ERR);
		return 0;
	}

	//Create Task
	err = OSTaskCreateExt(thread, arg, &taskStack[stacksize - 1], prio, prio, &taskStack[0], stacksize, (void *) 0, OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR);
	if(err != OS_ERR_NONE){
		sysArchError |= (1 << SYS_THREAD_NEW_CREATE_ERR);
		return 0;
	}

#if (OS_TASK_NAME_EN > 0)
    OSTaskNameSet(prio,(INT8U *)name,&err);
#endif

	if (err == OS_ERR_NONE)
	{
		return prio;
	} else
	{
		sysArchError |= (1 << SYS_THREAD_NEW_NAME_ERR);
		return 0;
	}
}

/*
 * This optional function does a "fast" critical region protection and returns
 the previous protection level. This function is only called during very short
 critical regions. An embedded system which supports ISR-based drivers might
 want to implement this function by disabling interrupts. Task-based systems
 might want to implement this by using a mutex or disabling tasking. This
 function should support recursive calls from the same task or interrupt. In
 other words, sys_arch_protect() could be called while already protected. In
 that case the return value indicates that it is already protected.

 sys_arch_protect() is only required if your port is supporting an operating
 system.
 */

sys_prot_t sys_arch_protect(void) {
#if OS_CRITICAL_METHOD == 3                            /* Allocate storage for CPU status register     */
    OS_CPU_SR  cpu_sr = 0;
#endif
	OS_ENTER_CRITICAL();
	return cpu_sr;
}

/*
 * This optional function does a "fast" set of critical region protection to the
 value specified by pval. See the documentation for sys_arch_protect() for
 more information. This function is only required if your port is supporting
 an operating system.
 */
void sys_arch_unprotect(sys_prot_t cpu_sr) {
	OS_EXIT_CRITICAL();
}

/*
 * This function gives you the Error-Flag-Word. Should be zero.
 */
uint32_t getSysArchError(){
	return sysArchError;
}
