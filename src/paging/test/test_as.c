#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <stdio.h>
#include <paging.h>

void halt();

/*------------------------------------------------------------------------
 *  main  --  user main program
 *------------------------------------------------------------------------
 */

void testNRU_func()
{
		STATWORD ps;
		int PAGE0 = 0x40000;
		int i,j,temp=0;
		int addrs[1200];
		int cnt = 0; 

		//can go up to  (NFRAMES - 5 frames for null prc - 1pd for main - 1pd + 1pt frames for this proc)
		int maxpage = (NFRAMES - (5 + 1 + 1 + 1)); //=1016


		srpolicy(NRU); 

		for (i=0;i<=maxpage/100;i++){
				if(get_bs(i,100) == SYSERR)
				{
						kprintf("get_bs call failed \n");
						return;
				}
				if (xmmap(PAGE0+i*100, i, 100) == SYSERR) {
						kprintf("xmmap call failed\n");
						return;
				}
				for(j=0;j < 100;j++)
				{  
						//store the virtual addresses
						addrs[cnt++] = (PAGE0+(i*100) + j) << 12;
				}			
		}


		/* all of these should generate page fault, no page replacement yet
		   acquire all free frames, starting from 1032 to 2047, lower frames are acquired first
		   */
		for(i=0; i < maxpage; i++)
		{  
				*((int *)addrs[i]) = i + 1;  //bring all pages in, only referece bits are set

		}

		sleep(3); //after sleep, all reference bits should be cleared

		disable(ps); //reduce the possibility of trigger reference bit clearing routine while testing

		for(i=0; i < maxpage/2; i++)
		{  
				*((int *)addrs[i]) = i + 1; //set both ref bits and dirty bits for these pages			
		}		


		enable(ps);  //to allow page fault
		// trigger page replace ment, expected output: frame 1032+maxpage/2=1540 will be swapped out
		// this test might have a different result (with very very low possibility) if bit clearing routine is called before executing the following instruction		
		*((int *)addrs[maxpage]) = maxpage + 1; 

		for (i=0;i<=maxpage/100;i++){
				xmunmap(PAGE0+(i*100));
				release_bs(i);			
		}


}
void testNRU(){
		int pid1;

		kprintf("\nTest NRU page replacement policy\n");

		pid1 = create(testNRU_func, 2000, 20, "testNRU_func", 0, NULL);

		resume(pid1);
		sleep(10);
		kill(pid1);


}


void testSC_func()
{
		int PAGE0 = 0x40000;
		int i,j,temp;
		int addrs[1200];
		int cnt = 0; 
		//can go up to  (NFRAMES - 5 frames for null prc - 1pd for main - 1pd + 1pt frames for this proc)
		//frame for pages will be from 1032-2047
		int maxpage = (NFRAMES - (5 + 1 + 1 + 1));

		srpolicy(SC); 

		for (i=0;i<=maxpage/100;i++){
				if(get_bs(i,100) == SYSERR)
				{
						kprintf("get_bs call failed \n");
						return;
				}
				if (xmmap(PAGE0+i*100, i, 100) == SYSERR) {
						kprintf("xmmap call failed\n");
						return;
				}
				for(j=0;j < 100;j++)
				{  
						//store the virtual addresses
						addrs[cnt++] = (PAGE0+(i*100) + j) << 12;
				}			
		}

		/* all of these should generate page fault, no page replacement yet
		   acquire all free frames, starting from 1032 to 2047, lower frames are acquired first
		   */
		for(i=0; i < maxpage; i++)
		{  
				*((int *)addrs[i]) = i + 1; 
		}


		//trigger page replacement, this should clear all access bits of all pages
		//expected output: frame 1032 will be swapped out
		*((int *)addrs[maxpage]) = maxpage + 1; 

		for(i=1; i <= maxpage; i++)
		{

				if (i != 500)  //reset access bits of all pages except this
					*((int *)addrs[i])= i+1;

		}
		//Expected page to be swapped: 1032+500 = 1532
		*((int *)addrs[maxpage+1]) = maxpage + 2; 
		temp = *((int *)addrs[maxpage+1]);


		for (i=0;i<=maxpage/100;i++){
				xmunmap(PAGE0+(i*100));
				release_bs(i);			
		}

}
void testSC(){
		int pid1;


		kprintf("\nTest SC page replacement policy\n");

		pid1 = create(testSC_func, 2000, 20, "testSC_func5", 0 , NULL);

		resume(pid1);
		sleep(10);
		kill(pid1);

}

void func4() {

        kprintf("\n test 4 has started\n");
        char *addr = (char*)0x40000000; //1G
	bsd_t bs=1;

	int  i= ((unsigned long)addr)>>12;	// the ith page

	kprintf("\n\nHello World, Xinu lives\n\n");

	get_bs(bs, 100);

	if (xmmap(i, bs, 100) == SYSERR) {
	    kprintf("xmmap call failed\n");
	    return 0;
	}

	for (i=0; i<16; i++){
	    *addr = 'A'+i;
	    addr += NBPG;	//increment by one page each time
	}


        addr = (char*)0x40000000; //1G
        for (i=0; i<16; i++){
            kprintf("0x%08x: %c\n", addr, *addr);
            addr += NBPG;       //increment by one page each time
        }

	xmunmap(0x40000000>>12);
        release_bs(bs);
#ifdef DBG_ON
        print_bsm_tab(TRUE);
#endif
}

void test4() {

        int pid4 = create(func4,2000,20,"func4",1,'a');
        resume(pid4);
}


void func3(char a) {

	    kprintf("\n func3 has started");
	    char *x;
            char temp;
            get_bs(4, 100);
            xmmap(9000, 4, 100);  
            x = 9000*4096;
            *x = 'Z';
            temp = *x;    
            kprintf("\n temp value = %c",temp);
}




void test3() {
            kprintf("\n entering test 3"); 
            int pid1 =create(func3,2000,20,"func3",1,'a');
            char *x;
            char temp;
            get_bs(4, 100);
            xmmap(7000, 4, 100);  
            x = 7000*4096;
            *x = 'Y';       
            temp = *x;   
            kprintf("\n temp value = %c",temp);
            resume(pid1);
            temp = *x;
            kprintf("\n final temp value = %c",temp);

}
          
   
   

void func2(char a) {

   int *x;
   x = vgetmem(100);
   kprintf("\n x points to %08x",x);
   int temp;
   *x =100;
   temp = *x;
   kprintf("\n temp = %d",temp); 
   return 0;
}


void test2() {
    kprintf("\n entering TEST 2 !!!!!!!! ");
    int pid1 =vcreate(func2,2000,100,20,"func2",1,'a');
#ifdef DBG_ON
    print_bsm_tab(TRUE);
#endif
        
    // doa get_bs on the same bs
    int r = get_bs(1,100);
    if (r ==SYSERR) {
       kprintf("\n error with get_bs");
    }
    r = xmmap(6000,1,100);
#ifdef DBG_ON
    print_bsm_tab(TRUE);
#endif
    char *x;
    char temp; 
    x = 6000*4096;
    *x = 'Y';
    temp = *x;
    kprintf("\n temp = %c",temp);
    resume(pid1);
}


void secfunc(char a) {

   int p1 = get_bs(4,10);
   if (p1 == SYSERR) 
      kprintf("\n get_bs failure!!!!!!!!");
   int p2 = get_bs(1,20);
   if (p2 == SYSERR)
      kprintf("\n get_bs failure!!!!!!!!");
   int p3 = get_bs(6,50);
   if (p3 == SYSERR)
      kprintf("\n get_bs failure!!!!!!!!");
   int p4 = get_bs(8,100);
   if (p4 == SYSERR)
      kprintf("\n get_bs failure!!!!!!!!");
   int rc = xmmap(6000,4,p1);
   if (rc == SYSERR)
      kprintf("\n xmmap1 failure!!!!!!!!");
   rc = xmmap(7000,1,40);
    if (rc == SYSERR)
      kprintf("\n xmmap2 failure!!!!!!!!");
   kprintf("\n secfunc going to slep");
   rc = xmmap(8000,6,p3);
   if (rc == SYSERR)
      kprintf("\n xmmap3 failure!!!!!!!!");
   rc = xmmap(9000,8,p4);
   if (rc == SYSERR)
      kprintf("\n xmmap4 failure!!!!!!!!");
#ifdef DBG_ON
    print_bsm_tab(TRUE);
#endif
   kprintf("\n sleep is taking over!!!!!!!!!!");
   sleep(1);
   release_bs(4);
   release_bs(1);
   release_bs(6);
   release_bs(8);
}


void test1() {
   
   kprintf("\n entering Test 1 !!!!!!!!");
   int pid1 = create(secfunc,2000,20,"func1",1,'a');
      
   int p1 = get_bs(0,100);
   if (p1 == SYSERR) 
      kprintf("\n get_bs failure!!!!!!!!");
   int p2 = get_bs(1,80);
   if (p2 == SYSERR) 
      kprintf("\n get_bs failure!!!!!!!!");
   
   int p3 = get_bs(2,70);
   if (p3 == SYSERR) 
      kprintf("\n get_bs failure!!!!!!!!");
   int p4 = get_bs(7,120);
   if (p4 == SYSERR) 
      kprintf("\n get_bs failure!!!!!!!!");
   int rc = xmmap(6000,0,p1);
   if (rc == SYSERR) 
      kprintf("\n xmmap1 failure!!!!!!!!");
   rc = xmmap(7000,1,p2);
    if (rc == SYSERR) 
      kprintf("\n xmmap2 failure!!!!!!!!");
   resume(pid1);
   rc = xmmap(8000,2,p3);
   if (rc == SYSERR) 
      kprintf("\n xmmap3 failure!!!!!!!!");
   rc = xmmap(9000,7,p4);
   if (rc == SYSERR) 
      kprintf("\n xmmap4 failure!!!!!!!!");
   release_bs(0);
   release_bs(1);
   release_bs(2);
   release_bs(7);
#ifdef DBG_ON
    print_bsm_tab(TRUE);
#endif
}

int main() {
   //test1();
   //kprintf("\n RETURNING BACK FROM TEST 1");
   //sleep(5);
   //test2();
   //kprintf("\n RETURNING FROM TEST 2");
   //sleep(5);
   //test3();
   //kprintf("\n RETURNING FROM TEST 3");
   //sleep(5);

    kprintf("\n\nBefore test 4\n\n");
   test4();
   sleep(5);
   kprintf("\n\nAfter test 4\n\n");



   kprintf("\n\nBefore test SC\n\n");
   testSC();
   kprintf("\n\nAfter test SC\n\n");
   //print_sc_tab();
   sleep(9);
   //kprintf("\n RETURNING FROM TEST SC");


   testNRU();
   return 0;
}
   

