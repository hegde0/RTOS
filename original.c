/*----------------------------------------------------------------------------------------
 *      RL-ARM - RTX
 *----------------------------------------------------------------------------------------
 *      Name:    TOKEN GENERATION  
 *      Purpose: COUNTING NUMBER OF PEOPLE ENTERING USING PIR SENSOR
 *---------------------------------------------------------------------------------------*/

#include <RTL.h>                      		// RTX kernel functions & defines      
#include <LPC17xx.h>                  		// LPC17xx definitions                 
#include <stdio.h>
#include <string.h>
#include "lcd.h"

#define PRESCALE (12000-1)  //25000 PCLK clock cycles to increment TC by 1 


OS_TID tsk1;                          		// assigned identification for send_task - task1  
OS_TID tsk2;                          		// assigned identification for rec_task - task 2  
	
int status = 0;
int k, motor, pirr,v;
int value;
int cur,prev;


typedef struct {                        // Message object structure            
  	int status;// A counter value                    
} T_MEAS;

os_mbx_declare (MsgBox,1);             // Declare an RTX mailbox             
_declare_box (mpool,sizeof(T_MEAS),1); // Dynamic memory pool 
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
void delayMS(unsigned int milliseconds);
void initTimer0(void);

void initTimer0(void)
{
	/*Assuming that PLL0 has been setup with CCLK = 100Mhz and PCLK = 25Mhz.*/
	LPC_SC->PCONP |= (1<<1); //Power up Timer0. By default Timer0 and Timer1 are enabled.
	LPC_SC->PCLKSEL0 &= ~(0x3<<2); //Set PCLK for timer = CCLK/4 = 100/4 (default)
	
	LPC_TIM0->CTCR = 0x0;
	LPC_TIM0->PR = PRESCALE; //Increment LPC_TIM0->TC at every 24999+1 clock cycles
	//25000 clock cycles @25Mhz = 1 mS

	LPC_TIM0->TCR = 0x02; //Reset Timer
	//LPC_TIM0->TCR = 0x01; //Enable timer, done inside delayMS()
}

void delayMS(unsigned int milliseconds) //Using Timer0
{
	LPC_TIM0->TCR = 0x02; //Reset Timer
	LPC_TIM0->TCR = 0x01; //Enable timer

	while(LPC_TIM0->TC < milliseconds); //wait until timer counter reaches the desired delay

	LPC_TIM0->TCR = 0x00; //Disable timer
}


void init_gpio()
{
	LPC_PINCON->PINSEL3 = 0x00000000;
	LPC_GPIO1->FIODIR |= 0x0c000000; // setting p1.26,27 as output for dc and p1.25 for pir
	LPC_GPIO1->FIODIR |= 0x00000000;
	LPC_GPIO2->FIOCLR = 0x00200000;	
LPC_GPIO1->FIOSET|=0X10000000;	
}
void dc(void)
{
	      int x;
			 	LPC_GPIO1->FIOSET |= 0X08000000;
				delayMS(50);
	      LPC_GPIO1->FIOCLR |= 0X08000000;
				delayMS(100);
				LPC_GPIO1->FIOSET |= 0X04000000;
				delayMS(500);
				//LPC_GPIO1->FIOCLR |= 0X04000000;
				delayMS(50);
				LPC_GPIO1->FIOSET |= 0X08000000;
				delayMS(100);
				/*LPC_GPIO1->FIOSET |= 0X08000000;
				delayMS(500);
				//for(x=0;x<65000;x++);
				LPC_GPIO1->FIOSET |= 0X04000000;
				delayMS(500);
				//for(x=0;x<65000;x++);	
				LPC_GPIO1->FIOCLR |= 0X04000000;*/
				motor++;
		
}


int pir(void)
{
	//int sattava;	
				pirr++;
				cur = LPC_GPIO1->FIOPIN &0x02000000;
			if(cur==00)
				return cur;
			else
			{
				while(LPC_GPIO1->FIOPIN &0x02000000);
				return 0x02000000;
		}

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
__task void task3 (void) 
{

	T_MEAS *mptr;
	os_mbx_init (MsgBox, sizeof(MsgBox));// initialize the mailbox    

	while (1)  {                        	// endless loop                        
      	 lcd();
	    	mptr = _alloc_box (mpool);
				k = pir();
				
		  i=0;
    	if (k ==  0x02000000) {           // signal overflow of counter 3       
			   counter3 ++;
					dc();				
			}	
				mptr -> status=counter3;
				os_mbx_send (MsgBox, mptr, 0xffff); // Send the message to the mailbox     
    		os_evt_set (0x0002,tsk4);       // to task 4   
				os_dly_wait(2);
			  os_tsk_pass ();               	// because of same priority task3                         
    	
  	}
}

/*--------------------------------------------------------------------------------------------------------------------------------------
 *   Task 4 :  RTX Kernel starts this task with os_tsk_create (task4,1)
 *------------------------------------------------------------------------------------------------------------------------------------*/
__task void task4 (void) {
	  
	T_MEAS *mtr;
	while (1) { 
					
		           counter4++;                                       
    	os_evt_wait_or (0x0002, 0x00ff);  	// wait for signal event               
      os_mbx_wait (MsgBox, (void **)&mtr, 0xffff);
			v=mtr->status;
		 	while (!(LPC_UART0->LSR & 0x20));
			LPC_UART0->THR = v+48;
		//for(sattava=0;sattava<12000;sattava++);
			
			_free_box (mpool, mtr); 
	
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
	LPC_UART0->DLL = 0x4e;      			// select baud rate 9600 bps @25MHz
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
 	  UART0_Init (); 
		initTimer0();	// initialize the serial interface   
    init_gpio() ;  
   	_init_box (mpool, sizeof(mpool),    	// initialize the 'mpool' memory for   
              sizeof(status));        		// the membox dynamic allocation    
	
		os_sys_init_prio(init_task, 10);		// Initialize init_task and start RTX Kernel
}
/*---------------------------------------------------------------------------------------------------------------
 * end of file
 *-------------------------------------------------------------------------------------------------------------*/
/*...............................................................................................................
BY TEAM 3
(ROHIT,CHINMAYEE,VINAYA,SHREESHA)
.................................................................................................................*/
