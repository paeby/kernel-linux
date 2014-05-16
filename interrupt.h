#ifndef INTERRUPT_H_
#define INTERRUPT_H_

#include "system_m.h"

/* Function that enables all 4 button interrupts and that resets the edge capture register. */
void init_button();

/* Function that enables clock interrupts. */
void init_clock();

/* Function used in implementation of iotransfer. */ 
void insertTail(int i, Process toBeInserted);

extern volatile int edge_capture;

/* Function that masks all interrupts. */
void maskInterrupts();

/* Function that allows all interrupts. */
void allowInterrupts();

#endif /*INTERRUPT_H_*/
