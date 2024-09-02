/*----------------------------------------------------------------------------------------
 *      RL-ARM - RTX
 *----------------------------------------------------------------------------------------
 *      Name:    RTX-CHATTING.c  
 *      Purpose: RTX Task passing a message from one task to other using Mailbox concept
 *---------------------------------------------------------------------------------------*/

#include <RTL.h>                      		// RTX kernel functions & defines      
#include <LPC17xx.h>                  		// LPC17xx definitions                 
#include <stdio.h>
#include <string.h>
#include "lcd.h"

OS_TID tsk1;                          		// assigned identification for send_task - task1  
OS_TID tsk2;                          		// assigned identification for rec_task - task 2  
	
int statuss = 0;
int k, motor, pirr,v;
int value;
//int *ptr = &status;



os_mbx_declare (MsgBox,1);  
U32 mpool[(sizeof(U32))/4 + 3];// Declare an RTX mailbox             
//_declare_box (mpool,sizeof(T_MEAS),1); // Dynamic memory pool 
// tsk1, tsk2, tsk3, tsk4 will contain task identifications at run-time 
OS_TID tsk1, tsk2, tsk3, tsk4;

U16 counter1;          // counter for task 1                  
U16 counter2;          // counter for task 2                  
U16 counter3;          // counter for task 3                  
U16 counter4;          // counter for task 4                  

char arr1[20],arr2[20],arr3[20],arr4[20];
/* -------------------------------------------------------------------------------------------------------------------------
*/

extern void lcd_init(void);
extern void lcd_com(void);
extern void wr_cn(void);
extern void lcd_data(void);
extern void wr_dn(void);
extern void delay_lcd(unsigned int r1);
extern void clr_disp(void);
extern void clear_ports(void);
extern void lcd_puts(unsigned char *buf1);
extern void lcd(void);


/* -------------------------------------------------------------------------------------------------------------------------
*/

unsigned int i = 0;

// Function prototypes
__task void init_task(void); 
__task void task1 (void);
__task void task3 (void);
__task void task4 (void);
void UART0_Init(void);




void init_gpio()
{
	LPC_PINCON->PINSEL3 = 0x00000000;
	LPC_GPIO1->FIODIR |= 0x0c000000; // setting p1.26,27 as output for dc and p1.25 for pir
	LPC_GPIO1->FIODIR |= 0x00000000;
	LPC_GPIO2->FIOCLR = 0x00200000;	 
}
void dc(void)
{
	      int x;
			 	LPC_GPIO1->FIOSET |= 0X08000000;
				for(x=0;x<65000;x++);
	      LPC_GPIO1->FIOCLR |= 0X08000000;
				for(x=0;x<65000;x++);
				LPC_GPIO1->FIOSET |= 0X04000000;
				for(x=0;x<65000;x++);
				LPC_GPIO1->FIOCLR |= 0X04000000;
				for(x=0;x<65000;x++);
				LPC_GPIO1->FIOSET |= 0X08000000;
				for(x=0;x<65000;x++);
				LPC_GPIO1->FIOSET |= 0X08000000;
				for(x=0;x<65000;x++);
				LPC_GPIO1->FIOSET |= 0X04000000;
				for(x=0;x<65000;x++);	
				LPC_GPIO1->FIOCLR |= 0X04000000;
				motor++;
		
}


int pir(void)
{
				pirr++;
				value  = LPC_GPIO1->FIOPIN & 0x02000000;
				return value;
}




/*--------------------------------------------------------------------------------------------------------------------------------------
 *   init_task:  RTX Kernel starts this task with 	os_sys_init_prio(init_task, 10);
 *------------------------------------------------------------------------------------------------------------------------------------*/
__task void init_task(void)
{
	//tsk1 = os_tsk_create(task1,1);
	tsk3 = os_tsk_create(task3,1); 			// task3 at priority 1 
	tsk4 = os_tsk_create(task4,1); 			// task4 at priority 1
	os_tsk_delete_self(); 					// must delete itself before exiting
}



/* ----------------------------------------------------------------------------------------------------------------------------------

*/

/*--------------------------------------------------------------------------------------------------------------------------------------
 *   Task 3 :  RTX Kernel starts this task with os_tsk_create (task3,1)
 *------------------------------------------------------------------------------------------------------------------------------------*/
__task void task3 (void) //send task
{

	U32 *mptr;
	os_mbx_init (MsgBox, sizeof(MsgBox));// initialize the mailbox    

	while (1)  {                        	// endless loop                        
      	 lcd();
	    	mptr = _alloc_box (mpool);
				*mptr=pir();
		//statuss= pir();
		//mptr->status=statuss;
				
		  i=0;
    if(pir())	 {           // signal overflow of counter 3       
			   counter3 ++;	
				
				//mptr -> status=statuss;
				os_mbx_send (MsgBox, mptr, 0xffff); // Send the message to the mailbox     
    		os_evt_set (0x0002,tsk4);       // to task 4   
				os_dly_wait(2);
			  os_tsk_pass ();               	// because of same priority task3                         
    	}
  	}
}

/*--------------------------------------------------------------------------------------------------------------------------------------
 *   Task 4 :  RTX Kernel starts this task with os_tsk_create (task4,1)
 *------------------------------------------------------------------------------------------------------------------------------------*/
__task void task4 (void) {
	  
	U32 *rptr;
	while (1) { 

		           counter4++;                                       
    	os_evt_wait_or (0x0002, 0x00ff);  	// wait for signal event               
      os_mbx_wait (MsgBox,(void *)&rptr, 0xffff);
			v=*rptr;
		 	while (!(LPC_UART0->LSR & 0x20));
			LPC_UART0->THR = (*rptr)+48;
			dc();
			_free_box (mpool, rptr); 
	
	}
}	

/*----------------------------------------------------------------------------
 *  Task 2: RTX Kernel starts this task with os_tsk_create (rec_task, 2)
 *---------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------------------------------------------
 *        Initialize serial interface										 
 *------------------------------------------------------------------------------------------------------------*/
void UART0_Init(void) {
	LPC_SC->PCONP |= 0x00000008;			// UART0 peripheral enable
	LPC_PINCON->PINSEL0 &= ~0x000000F0;
	LPC_PINCON->PINSEL0 |= 0x00000050;		// P0.2 - TXD0 and P0.3 - RXD0
	LPC_UART0->LCR = 0x00000083;			// enable divisor latch, parity disable, 1 stop bit, 8bit word length
	LPC_UART0->DLM = 0X00; 
	LPC_UART0->DLL = 0xA2;      			// select baud rate 9600 bps @25MHz
	LPC_UART0->LCR = 0X00000003;
	LPC_UART0->FCR = 0x07;
	LPC_UART0->IER = 0X03;	   				// select Transmit and receive interrupt
}

/*------------------------------------------------------------------------------
 *        Main: Initialize and start RTX Kernel
 *----------------------------------------------------------------------------*/
int main(void) {
		SystemInit();
	  SystemCoreClockUpdate();
 	  UART0_Init ();          	// initialize the serial interface   
    init_gpio() ;  
   	_init_box (mpool, sizeof(mpool),    	// initialize the 'mpool' memory for   
              sizeof(U32));        		// the membox dynamic allocation    
	
		os_sys_init_prio(init_task, 10);		// Initialize init_task and start RTX Kernel
}
/*---------------------------------------------------------------------------------------------------------------
 * end of file
 *-------------------------------------------------------------------------------------------------------------*/

