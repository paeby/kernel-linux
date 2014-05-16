#ifndef SYSTEM_M_H_
#define SYSTEM_M_H_

typedef unsigned int *Process;


/* 
    newProcess is a procedure that creates a new process. Parameter f denotes the function that constitutes 
    the code of process, stack and stackSize define the process stack. Before newProcess is invoked, it is 
    necessary to allocate space (using malloc) for stack. 
     
 */ 


Process newProcess(void (*f), unsigned int* stack, int stackSize);

/*
   This procedure suspends currently running process and transfers control to process p. 
    
*/
void transfer(Process p);


/*
    This procedure registers that active process waits on interrupt interruptV, suspends active process and
    transfers control to process p.
  
 */
void iotransfer(Process p, int interruptV);



#endif /*SYSTEM_M_H_*/






