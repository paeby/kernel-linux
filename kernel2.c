#include <stdio.h>
#include <stdlib.h>
#include "system_m.h"
#include "interrupt.h"
#include "kernel2.h"

/************* Symbolic constants and macros ************/
#define MAX_PROC 10
#define MAX_MONITORS 10

#define DPRINTA(text, ...) printf("[%d] " text "\n", head(&readyList), __VA_ARGS__)
#define DPRINT(text) DPRINTA(text, 0)
#define ERRA(text, ...) fprintf(stderr, "[%d] Error: " text "\n", head(&readyList), __VA_ARGS__)
#define ERR(text) ERRA(text, 0)

#define STACK_SIZE 10000

void idleFunction();

void clockFunction();

/************* Data structures **************/
typedef struct {
	int next;
	Process p;
	int currentMonitor;			/* points to the monitors array */
	int monitors[MAX_MONITORS + 1]; /* used for nested calls; monitors[0] is always -1 */
	int notified; // flag that tracks whether the process has been notified or not. Initial value 0
	int counter; // -1 if inactive, 0 if active and counting
} ProcessDescriptor; 

typedef struct {
	int timesTaken;
	int takenBy;
	int entryList;
	int waitingList;
} MonitorDescriptor;

/********************** Global variables **********************/

/* Pointer to the head of the ready list */
static int readyList = -1;

/* List of process descriptors */
ProcessDescriptor processes[MAX_PROC];
static int nextProcessId = 0;

/* List of monitor descriptors */
MonitorDescriptor monitors[MAX_MONITORS];
static int nextMonitorId = 0;

static int idleIndex;
static int clockIndex;

/*************** Functions for process list manipulation **********/

/* add element to the tail of the list */
static void addLast(int* list, int processId) {
	if (*list == -1){
		*list = processId;
	}
	else {
		int temp = *list;
		while (processes[temp].next != -1){
			temp = processes[temp].next;
		}
		processes[temp].next = processId;
	}
	processes[processId].next = -1;
}

/* add element to the head of list */
static void addFirst(int* list, int processId){
	processes[processId].next = *list;
	*list = processId;
}

/* remove an element from the head of the list */
static int removeHead(int* list){
	if (*list == -1){
		return(-1);
	}
	else {
		int head = *list;
		int next = processes[*list].next;
		processes[*list].next = -1;
		*list = next;
		return head;
	}
}

/* returns the head of the list */
static int head(int* list){
	return *list;
}

/* checks if the list is empty */
static int isEmpty(int* list) {
	return *list < 0;
}

static int removeProc(int* list, int proc) { //removes a specific process in a list
	if(processes[*list].next != proc)
		removeProc(&processes[*list].next, proc);
	else return removeHead(&processes[*list].next);
}

/***********************************************************
 ***********************************************************
                    Kernel functions
************************************************************
* **********************************************************/

void createProcess (void (*f)(), int stackSize) {
	maskInterrupts();
	if (nextProcessId == MAX_PROC){
		ERR("Maximum number of processes reached!");
		exit(1);
	}
	unsigned int* stack = malloc(stackSize);
	if (stack==NULL) {
		ERR("Could not allocate stack. Exiting...");
		exit(1);
	}
	processes[nextProcessId].p = newProcess(f, stack, stackSize);
	processes[nextProcessId].next = -1;
	processes[nextProcessId].currentMonitor = 0;
	processes[nextProcessId].monitors[0] = -1;
	processes[nextProcessId].notified = 0;
	processes[nextProcessId].counter = -1;
	processes[nextProcessId].sleeping = 0;

	addLast(&readyList, nextProcessId);
	nextProcessId++;
	allowInterrupts();
}

static void checkAndTransfer() {
	if (isEmpty(&readyList)){
		ERR("No processes in the ready list! Exiting...");
		exit(1);
	}
	Process process = processes[head(&readyList)].p;
	transfer(process);
}

void start(){
	maskInterrupts();
	DPRINT("Starting kernel...");
	if(isEmpty(&readyList))
		printf("ERROR: readyList is empty.");
	idleIndex = nextProcessId;
	createProcess(idleFunction, STACK_SIZE);
	clockIndex = nextProcessId;
	createProcess(clockFunction, STACK_SIZE);
	init_button();
	transfer(processes[clockIndex].p);
	allowInterrupts();
}

void yield(){
	maskInterrupts();
	int pid = removeHead(&readyList);
	addLast(&readyList, pid);
	checkAndTransfer();
	allowInterrupts();
}

int createMonitor(){
	maskInterrupts();
	if (nextMonitorId == MAX_MONITORS){
		ERR("Maximum number of monitors reached!\n");
		exit(1);
	}
	monitors[nextMonitorId].timesTaken = 0;
	monitors[nextMonitorId].takenBy = -1;
	monitors[nextMonitorId].entryList = -1;
	monitors[nextMonitorId].waitingList = -1;
	return nextMonitorId++;
	allowInterrupts();
}

static int getCurrentMonitor(int pid) {
	return processes[pid].monitors[processes[pid].currentMonitor];
}

void enterMonitor(int monitorID) {
	maskInterrupts();
	int myID = head(&readyList);

	if (monitorID > nextMonitorId || monitorID < 0) {
		ERRA("Monitor %d does not exist.", nextMonitorId);
		exit(1);
	}

	if (processes[myID].currentMonitor >= MAX_MONITORS) {
		ERR("Too many nested calls.");
		exit(1);
	}

	if (monitors[monitorID].timesTaken > 0 && monitors[monitorID].takenBy != myID) {
		removeHead(&readyList);
		addLast(&(monitors[monitorID].entryList), myID);
		checkAndTransfer();

		/* I am woken up by exitMonitor -- check if the monitor state is consistent */
		if ((monitors[monitorID].timesTaken != 1) || (monitors[monitorID].takenBy != myID)) {
			ERR("The kernel has performed an illegal operation. Please contact customer support.");
			exit(1);
		}
	}
	else {
		monitors[monitorID].timesTaken++;
		monitors[monitorID].takenBy = myID;
	}

	/* push the new call onto the call stack */
	processes[myID].monitors[++processes[myID].currentMonitor] = monitorID;
	allowInterrupts();
}

void exitMonitor() {
	maskInterrupts();
	int myID = head(&readyList);
	int myMonitor = getCurrentMonitor(myID);

	if (myMonitor < 0) {
		ERRA("Process %d called exitMonitor outside of a monitor.", myID);
		exit(1);
	}

	/* go backwards in the stack of called monitors */
	processes[myID].currentMonitor--;

	if (--monitors[myMonitor].timesTaken == 0) {
		/* see if someone is waiting, and if yes, let the next process in */
		if (!isEmpty(&(monitors[myMonitor].entryList))) {
			int pid = removeHead(&(monitors[myMonitor].entryList));
			addLast(&readyList, pid);
			monitors[myMonitor].timesTaken = 1;
			monitors[myMonitor].takenBy = pid;
		} else {
			monitors[myMonitor].takenBy = -1;
		}
	}
	allowInterrupts();
}

void wait() {
	maskInterrupts();
	int myID = head(&readyList);
	int myMonitor = getCurrentMonitor(myID);
	int myTaken;
	processes[myID].notified = 0;

	if (myMonitor < 0) {
		ERRA("Process %d called wait outside of a monitor.", myID);
		exit(1);
	}

	removeHead(&readyList);
	addLast(&monitors[myMonitor].waitingList, myID);

	/* save timesTaken so we can restore it later */
	myTaken = monitors[myMonitor].timesTaken;

	/* let the next process in, if any */
	if (!isEmpty(&(monitors[myMonitor].entryList))) {
		int pid = removeHead(&(monitors[myMonitor].entryList));
		addLast(&readyList, pid);
		monitors[myMonitor].timesTaken = 1;
		monitors[myMonitor].takenBy = pid;
	} else {
		monitors[myMonitor].timesTaken = 0;
		monitors[myMonitor].takenBy = -1;
	}
	checkAndTransfer();

	/* I am woken up by exitMonitor -- check if the monitor state is consistent */
	if ((monitors[myMonitor].timesTaken != 1) || (monitors[myMonitor].takenBy != myID)) {
		ERR("The kernel has performed an illegal operation. Please contact customer support.");
		exit(1);
	}

	/* we're back, restore timesTaken */
	monitors[myMonitor].timesTaken = myTaken;
	allowInterrupts();
}

void notify() {
	maskInterrupts();
	int myID = head(&readyList);
	int myMonitor = getCurrentMonitor(myID);

	if (myMonitor < 0) {
		ERRA("Process %d called notify outside of a monitor.", myID);
		exit(1);	
	}

	if (!isEmpty(&(monitors[myMonitor].waitingList))) {
		int pid = removeHead(&monitors[myMonitor].waitingList);
		processes[pid].notified = 1;
		addLast(&monitors[myMonitor].entryList, pid);
	}
	allowInterrupts();
}

void notifyAll() {
	maskInterrupts();
	int myID = head(&readyList);
	int myMonitor = getCurrentMonitor(myID);

	if (myMonitor < 0) {
		ERRA("Process %d called notify outside of a monitor.", myID);
		exit(1);
	}

	while (!isEmpty(&(monitors[myMonitor].waitingList))) {
		int pid = removeHead(&monitors[myMonitor].waitingList);
		processes[pid].notified = 1;
		addLast(&monitors[myMonitor].entryList, pid);
	}
	allowInterrupts();
}

void idleFunction() {
	while(1){}
}

int checkandDecrement() {
	for(int i = 0; i < nextProcessId - 3; i++) { //-next -clock -idle
		ProcessDescriptor proc = processes[i]; 
		if(proc.notified) { //the process got the notify before timeout
			proc.counter = -1;
		}
		if(proc.counter >= 0){
			proc.counter--;
			if(proc.counter < 0){
				//removes the process in question from the waitingList and
				//adds it to the end of the entryList of the monitor
				//when the timer expires
				if(!proc.sleeping)
				addLast(&getCurrentMonitor(i).entryList,
				 removeProc(&getCurrentMonitor(i).waitingList, i));
				else {
					proc.sleeping = 0;
					addLast(&readyList ,i);
				}
			}
		}
	}
}

void clockFunction() {
	maskInterrupts();
	init_clock();
	while(1) {
		if(!isEmpty(&readyList)) {
			Process p = processes[head(&readyList)].p;
			iotransfer(p, 0);
			checkandDecrement(); 
		}
		else iotransfer(processes[idleIndex].p, 0);
		int q = removeHead(&readyList);
		addLast(&readyList, q);
	}
	//timeslice each 20msec transfer(head(&readyList).p)????
	if(slice % 20 != 0){
		slice++;
	} else {
		slice = 0;
		transfer(processes[head(&readyList)].p); // or iotransfer?!
	}
}


int timedWait(int msec) {
	maskInterrupts();
	int myID = head(&readyList);
	
	if(msec < 0) {
		ERR("Negative time given in timedWait");
		exit(1);
	}

	processes[myID].notified = 0;
	if(msec == 0) wait();
	else {
		processes[myID].counter = msec; /*activate counter*/
		wait(); //real start of the waiting and countdown
		}
		processes[myID].counter = -1;
		if(processes[myID].notified == 1) {
			allowInterrupts();
			return 1;
		} else {
			allowInterrupts();
			return 0;
		}
	}
}

void sleep(int time) { //blocking or transfer()
	maskInterrupts();
	int processId = removeHead(&readyList);
	processes[processId].counter = time;
	processes[processId].sleeping = 1;
	allowInterrupts();
}

void waitInterrupt(int peripherique) {
	int elem = removeHead(&readyList);
	Process p;
	if(!isEmpty(&readyList))
		p = processes[head(&readyList)].p;
	else
		p = processes[idleIndex].p;
	iotransfer(p, peripherique);
	addFirst(&readyList, elem);
}