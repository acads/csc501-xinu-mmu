/* user.c - main */

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

int main() {
		testSC();
		testNRU();
		return 0;
}




