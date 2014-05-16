#include <stdio.h>
#include <stdlib.h>
#include "system.h"
#include "interrupt.h"
#include "altera_avalon_pio_regs.h"
#include "kernel2.h"

#define STACK_SIZE		10000
#define INTERVAL		100
#define FREEZE_FOR		3000

#define RESET			0x1111
#define START			0x2222
#define STOP			0x3333
#define TIMEOUT			0xFFFF

/*********************** Buffer implemented using monitors *********************/
typedef struct {
	int message;
	int full;
	int monitor;
} Buffer;

void initBuffer(Buffer* b) {
	b->monitor = createMonitor();
	b->full = 0;
}

void put(Buffer* b, int m) {
	enterMonitor(b->monitor);
	while(b->full) {
		wait();
	}
	b->message = m;
	b->full = 1;
	notify();
	exitMonitor();

	return;
}

int get(Buffer* b) {
	int m;

	enterMonitor(b->monitor);
	while (!b->full) {
		wait();
	}
	m = b->message;
	b->full = 0;
	notifyAll();
	exitMonitor();

	return m;
}

int timedGet(Buffer *b, int timeout) {
	int m, ret;

	enterMonitor(b->monitor);
	if (!b->full) {
		ret = timedWait(timeout);
	}
	if (ret) {
		m = b->message;
		b->full = 0;
		notifyAll();
	} else {
		m = TIMEOUT;
	}
	exitMonitor();
	return m;
}
/*****************************************************************************/

/* global variables */
Buffer b0;
int reset = 0;
int displayOn = 1;
int started = 0;

/* arrays used to convert digits to their LCD representations */
int digitCodes[] = {0x3E223E00, 0x203E2400, 0x2E2A3A00, 0x3E2A2A00, 0x3E080E00,
		0x3A2A2E00, 0x3A2A3E00, 0x3E020200, 0x3E2A3E00, 0x3E2A2E00, 0};
int lcdZones[] = {LED_0_BASE, LED_1_BASE, LED_2_BASE};

/* displays digit "no" in LCD zone "zone" */
void displayDigit(int zone, int no) {
	IOWR_ALTERA_AVALON_PIO_DATA(lcdZones[zone], digitCodes[no]);
}

void displayNumber(int no) {
	displayDigit(2, no % 10);
	displayDigit(1, (no / 10) % 10);
	displayDigit(0, no / 100);
}

void producer(){
	int temp;

	printf("Producer starting...\n");

	while(1) {
		waitInterrupt(1);
		temp = edge_capture;
		if (temp != 0) {

			/* check button 0 */
			if (temp%2==1) {
				printf("Reset.\n");
				put(&b0, RESET);
			}

			/* check button 1 */
			temp = temp >> 1;
			if (temp%2==1) {
				printf("Start/Freeze.\n");
				put(&b0, START);
			}

			/* check button 2 */
			temp = temp >> 1;
			if (temp%2==1) {
				printf("Stop.\n");
				put(&b0, STOP);
			}

			/* button 3 ignored */
		}
	}
}


void consumer(){
	int m;

	printf("Consumer starting...\n");
	while (1) {
		m = displayOn ? get(&b0) : timedGet(&b0, FREEZE_FOR);

		switch (m) {
		case RESET:
			displayOn = 1;
			reset = 1;
			break;
		case START:
			if (started)
				displayOn = 1 - displayOn;
			else
				started = 1;
			break;
		case STOP:
			started = 0;
			break;
		case TIMEOUT:
			displayOn = 1;
			break;
		default:
			printf("Wrong command!\n");
			break;
		}
	}
}

void countAndDisplay() {
	int counter = 0;

	displayNumber(counter);

	while(1) {
		if (displayOn) {
			displayNumber(counter);
		}
		if (started)
			counter = (counter + 1) % 1000;
		if (reset) {
			counter = 0;
			reset = 0;
		}

		sleep(INTERVAL);
	}
}

int main() {
	IOWR_ALTERA_AVALON_PIO_DATA(LED_COLOR_BASE, LED_COLOR_RESET_VALUE);
	initBuffer(&b0);
	createProcess(producer, STACK_SIZE);
	createProcess(consumer, STACK_SIZE);
	createProcess(countAndDisplay, STACK_SIZE);

	start();
	return 0;
}
