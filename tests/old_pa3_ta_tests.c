/* user.c - main */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <stdio.h>
#include <paging.h>

void halt();

int total_points = 0;

#define add_points(x, y) \
		if (x) {\
			total_points += y;\
			kprintf("--> PASSED: %d (cumulative points)\n", total_points);\
		} else{\
			kprintf("--> FAILED\n");\
		}

/*------------------------------------------------------------------------
 *  main  --  user main program
 *------------------------------------------------------------------------
 */

#define PROC1_VADDR	0x40000000
#define PROC1_VPNO      0x40000
#define PROC2_VADDR     0x80000000
#define PROC2_VPNO      0x80000
#define TEST1_BS	1

/*********************************************************************************/
int test1() {
	kprintf("\n----- Test 1: get_bs(), xmmap(), xmunmap() ----- \n");
	int ret;
	char *addr = (char*) 0x40000000; //1G
	bsd_t bs = 1;

	int i = ((unsigned long) addr) >> 12;	// the ith page

	ret = get_bs(bs, 200);

	add_points(ret == OK, 5);

	if (xmmap(i, bs, 200) == SYSERR) {
		kprintf("xmmap call failed\n");
		return 0;
	}

	add_points(OK, 5);

	for (i = 0; i < 10; i++) {
		*addr = 'A' + i;
		add_points(OK, 1);
		addr += NBPG;	//increment by one page each time
	}

	addr = (char*) 0x40000000; //1G
	for (i = 0; i < 10; i++) {
		kprintf("0x%08x: %c\n", addr, *addr);

		add_points((*addr) == ('A' + i), 1);
		addr += 4096;       //increment by one page each time
	}

	ret = xmunmap(0x40000000 >> 12);

	sleep(1);
	kprintf("----- Test 1 finished! ----- \n");
	return 0;
}

/*********************************************************************************/
void proc1_test2(char *msg, int lck) {
	long j = 0;
	pd_t *addr = (pd_t*) FN2PA(1024);
	for (j = 0; j < 4; j++) {
		kprintf("Frame[%d]: 0x%08x: %d\n", j, addr,
				(unsigned int) addr->pd_base);
		add_points(((unsigned int) addr->pd_base) == 1025 + j, 2);
		addr += 1;
	}

	return;
}

void test2() {
	int pid1;

	kprintf("\n----- Test 2: Null Process Page Directory Test ----- \n");

	pid1 = create(proc1_test2, 2000, 20, "proc1_test2", 0, NULL);

	resume(pid1);

	sleep(1);
	kprintf("----- Test 2 finished! ----- \n");
	return;
}

/*********************************************************************************/
void proc1_test3(char *msg, int lck) {
	long j = 0;
	pt_t *addr = (pt_t*) 0x400000;
	for (j = 0; j < 12; j++) {
		kprintf("Frame[%d]: 0x%08x: %d\n", j + 4, addr,
				(unsigned int) addr->pt_base);
		add_points(((unsigned int) addr->pt_base) == 3072 + j, 1);
		addr += 1;
	}

	return;
}

void test3() {
	int pid1;

	kprintf("\n----- Test 3: Null Process Page Table Test ----- \n");

	pid1 = create(proc1_test3, 2000, 20, "proc1_test2", 0, NULL);

	resume(pid1);

	sleep(1);
	kprintf("----- Test 3 finished! ----- \n");
	return;
}

/*********************************************************************************/
void proc1_test4(char *msg, int lck) {
	char *addr;
	int i;

	get_bs(TEST1_BS, 200);

	if (xmmap(PROC1_VPNO, TEST1_BS, 200) == SYSERR) {
		kprintf("xmmap call failed\n");
		sleep(1);
		return;
	}

	addr = (char*) PROC1_VADDR;
	for (i = 0; i < 20; i++) {
		*(addr + i * NBPG) = 'A' + i;
	}

	sleep(4);

	for (i = 0; i < 20; i++) {
		kprintf("0x%08x: %c\n", addr + i * NBPG, *(addr + i * NBPG));
		add_points(*(addr + i * NBPG) == 'a' + i, 1)
	}

	xmunmap(PROC1_VPNO);
	return;
}

void proc2_test4(char *msg, int lck) {
	char *addr;
	int i;

	get_bs(TEST1_BS, 200);

	if (xmmap(PROC2_VPNO, TEST1_BS, 200) == SYSERR) {
		kprintf("xmmap call failed\n");
		sleep(1);
		return;
	}

	addr = (char*) PROC2_VADDR;

	for (i = 0; i < 20; i++) {
		//kprintf("0x%08x: %c\n", addr + i * NBPG, *(addr + i * NBPG));
	}

	/*Update the content, proc1 should see it*/
	for (i = 0; i < 20; i++) {
		*(addr + i * NBPG) = 'a' + i;
	}

	xmunmap(PROC2_VPNO);
	return;
}

void test4() {
	int pid1;
	int pid2;

	kprintf("\n----- Test 4: shared memory ----- \n");

	pid1 = create(proc1_test4, 2000, 20, "proc1_test4", 0, NULL);
	pid2 = create(proc2_test4, 2000, 20, "proc2_test4", 0, NULL);

	resume(pid1);
	sleep(2);
	resume(pid2);

	sleep(7);
	kprintf("----- Test 4 finished! ----- \n");
}

/*********************************************************************************/
void proc1_test5(char *msg, int lck) {
	int *x;

	kprintf("ready to allocate heap space\n");
	x = vgetmem(1024);
	kprintf("heap allocated at %x\n", x);
	*x = 100;
	add_points(*x == 100, 10);

	*(x + 400) = 4231;
	add_points(*(x + 400) == 4231, 10);

	//kprintf("heap variable: %d %d\n", *x, *(x + 1));

	vfreemem(x, 1024);
}

void test5() {
	int pid1;
	kprintf("\n----- Test 5: vgetmem/vfreemem ----- \n");
	pid1 = vcreate(proc1_test5, 2000, 100, 20, "proc1_test2", 0, NULL);

	kprintf("pid %d has private heap\n", pid1);
	//assert(pid1 != SYSERR);
	resume(pid1);

	sleep(1);
	kprintf("----- Test 5 finished! ----- \n");
	return;
}

/*********************************************************************************/

void proc1_test6(char *msg, int lck) {

	char *vaddr, *vaddr2, *addr0, *addr_lastframe, *addr_last;
	int i, j, k, l;
	int tempaddr;

	int vaddr_beg = 0x40000000;
	int vpno;

	//kprintf("\n---FIFO---\n");
	srpolicy(FIFO);

	// 0x40000000 ~ 0x40578000
	for (i = 0; i < 6; i++) {
		tempaddr = vaddr_beg + 200 * NBPG * i; // 200*4096 = OxC8000
		vaddr = (char *) tempaddr;
		vpno = tempaddr >> 12;
		get_bs(i, 200);
		if (xmmap(vpno, i, 200) == SYSERR) {
			kprintf("xmmap call failed\n");
			sleep(3);
			return;
		}

		for (j = 0; j < 200; j++) {
			*(vaddr + j * NBPG) = 'A' + i;
			//kprintf("0x%08x: %c\n", vaddr + j * NBPG, *(vaddr + j * NBPG));
		}
	}

	//kprintf("\n---AGIN---G\n");
	srpolicy(AGING);

	for (l = 0; l < 2; l++) {
		for (i = 0; i < 6; i++) {
			tempaddr = vaddr_beg + 200 * NBPG * i; // 200*4096 = OxC8000
			vaddr = (char *) tempaddr;
			vpno = tempaddr >> 12;

			for (j = 0; j < 200; j++) {
				*(vaddr + j * NBPG) = 'A' + i;

				for (k = 0; k < 4; k++) {
					tempaddr = vaddr_beg + k * NBPG;
					vaddr2 = (char *) tempaddr;
					*vaddr2 = 'K' + i;
				}
				//kprintf("0x%08x: %c\n", vaddr + j * NBPG, *(vaddr + j * NBPG));
			}
		}
	}

	for (i = 0; i < 6; i++) {
		tempaddr = vaddr_beg + 200 * NBPG * i;
		vpno = tempaddr >> 12;
		xmunmap(vpno);
	}

	add_points(OK, 10);

	return;
}

void test6() {
	int pid1;

	kprintf("\n----- Test 6: Page Replacement Policy: FIFO/AGING ----- \n");

	pid1 = create(proc1_test6, 2000, 20, "proc1_test6", 0, NULL);

	resume(pid1);

	sleep(1);
	kprintf("----- Test 6 finished! ----- \n");
}

int main() {

	kprintf("\n\nHello World, Xinu lives\n\n");

	test1();
	test2();
	test3();
	//test4();
	//test5();
	//test6();

	sleep(3);
	return 0;
}
