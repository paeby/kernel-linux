#ifndef KERNEL2_H_
#define KERNEL2_H_

void createProcess(void (*f)(), int stackSize);

void start();

int createMonitor();

void enterMonitor(int monitorID);

void exitMonitor();

void wait();

int timedWait(int msec);

void notify();

void notifyAll();

void sleep(int msec);

void yield();

void waitInterrupt(int per);

#endif /*KERNEL2_H_*/
