#include <stdlib.h>
#include <stdio.h>
#include <system.h>
#include <sys/alt_irq.h>
#include <alt_types.h>
#include <altera_avalon_pio_regs.h>
#include <altera_avalon_timer_regs.h>

#include "interrupt.h"
#include "assembly.h"
#include "system_m.h"



typedef struct ListElem{

    Process p;
    struct ListElem* next;
    
} ListElem;

ListElem* interruptVector[2]={NULL,NULL};


Process removeHeadI(int i){
    
    ListElem* removed = interruptVector[i];
    if(interruptVector[i] != NULL){
        interruptVector[i] = interruptVector[i] -> next;
    }
    if(removed != NULL){
        Process result = removed -> p; 
		free(removed); 
		return result;
    }
    else{
        Process result = NULL;
        return result;
    }  
}

void insertTail(int i, Process toBeInserted){
    
    ListElem* elem = malloc(sizeof(ListElem));
    elem -> p = toBeInserted;
    elem -> next = NULL;
    
    ListElem* temp= NULL;
    if(interruptVector[i] == NULL){
        
       interruptVector[i] = elem; 
    }
    else{
        
        temp = interruptVector[i];
        while((temp -> next) != NULL){
        
            temp = temp -> next;
        }
        temp -> next = elem;  
    
    }
    
        
}

/* A variable to hold the value of the button pio edge capture register. */
volatile int edge_capture = 0;


void handle_button_interrupts(void* context, alt_u32 id)
{
    
    /* Cast context to edge_capture's type. It is important that this be 
     * declared volatile to avoid unwanted compiler optimization.
     */
    volatile int* edge_capture_ptr = (volatile int*) context;
    
    /* Store the value in the Button's edge capture register in *context. */
    *edge_capture_ptr = IORD_ALTERA_AVALON_PIO_EDGE_CAP(BUTTONS_BASE);
	/* Reset the edge capture register. */
    IOWR_ALTERA_AVALON_PIO_EDGE_CAP(BUTTONS_BASE, 0xf);
    
    /* Read the PIO to delay ISR exit. This is done to prevent a spurious interrupt in systems
     * with high processor -> pio latency and fast interrupts.  */
    IORD_ALTERA_AVALON_PIO_EDGE_CAP(BUTTONS_BASE);
    
    Process p2 = removeHeadI(1);
   
    if(p2 != NULL){
        
        transfer(p2);
       
    }
}

/* Initialize the button_pio. */

void init_button()
{
    /* Recast the edge_capture pointer to match the alt_irq_register() function
     * prototype. */
    void* edge_capture_ptr = (void*) &edge_capture;
    
    /* Enable all 4 button interrupts. */
    IOWR_ALTERA_AVALON_PIO_IRQ_MASK(BUTTONS_BASE, 0xf);
    
    /* Reset the edge capture register. */
    IOWR_ALTERA_AVALON_PIO_EDGE_CAP(BUTTONS_BASE, 0xf);
    
    /* Register the interrupt handler. */
    alt_irq_register (BUTTONS_IRQ, edge_capture_ptr, handle_button_interrupts);
}

/* A variable to set up context for timer interrupt. */
volatile int timer_capture = 0;

void handle_timer_interrupts(void* context, alt_u32 id)
{
	/* clear the interrupt */
	IOWR_ALTERA_AVALON_TIMER_STATUS (TIMER_BASE, 0);

	Process p2 = removeHeadI(0);
    if(p2 != NULL){
        transfer(p2);
    }
}

void init_clock()
{
    
  void* timer_capture_ptr = (void*) &timer_capture;  
  /* set to free running mode */
  IOWR_ALTERA_AVALON_TIMER_CONTROL (TIMER_BASE, 
            ALTERA_AVALON_TIMER_CONTROL_ITO_MSK  |
            ALTERA_AVALON_TIMER_CONTROL_CONT_MSK |
            ALTERA_AVALON_TIMER_CONTROL_START_MSK);

  /* register the interrupt handler, and enable the interrupt */ 
  alt_irq_register (TIMER_IRQ, timer_capture_ptr, handle_timer_interrupts);  
  
}







