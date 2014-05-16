#include <stdio.h>
#include <stdlib.h>
#include "system_m.h"
#include "assembly.h"
#include "interrupt.h"


Process running = NULL;  // pointer to the current process.
Process nextP = NULL;  // variable used internally to implement transfer and iotransfer procedures

Process newProcess(void (*f), unsigned int* stack, int stackSize){
    
    unsigned int* newPC = f;
    int size = stackSize - 4;
    
    Process process = _createStack(stack,newPC,size);
    return process;
}

/**
 * Called mainly from interrupt routine.
 * (Except for the first call)
 */
void transfer(Process p){
    
    if(running == NULL){
        running = malloc(sizeof(Process));
    }
    nextP = p ;
    _transfer();
   
}

/**
 * Called from kernel thread.
 */
void iotransfer(Process p, int interruptV){
    
    insertTail(interruptV, running);
    nextP = p;
    _transfer();
   
}
    
    
