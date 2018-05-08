#include <stdio.h>
#include <xparameters.h>
#include <xuartlite.h>
#include <xuartlite_l.h>
#include <xio.h>
#include "ring_osc.h"
#include "xtmrctr.h"
#include "xintc_l.h"
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include "counter.h"
#include <stdbool.h>

// Date:       Version:  Developer:  Notes:
// 2014/01/06  1.0       aschmidt    Created and works with HW v1.0

#define SW_VERSION         "1.0"
#define UART_TX_FIFO_EMPTY 0x00000004

//the macros below correspond to bit locations specified in the user_logic.vhd file
#define SW_RESET           0x00000001
#define DUT_UART_REQUEST   0x00000002
#define DUT_UART_GRANTED   0x00000004
#define DUT_UART_RELEASE   0x00000008
#define DUT_TEST_DONE      0x00000010

#define RELOAD				1
#define CLK_TICKS			100000000
#define NUM_RING_OSCILLATORS 2
#define CHARACTERIZE_TIME	15
#define INTERVAL			(15*CLK_TICKS)
#define MAX_BUF 1024

//Timer defines
#define TIMER0_CONTROL	0x00000000
#define TIMER0_LOAD		0x00000004
#define TIMER0_COUNTER	0x00000008
#define TIMER1_CONTROL	0x00000010
#define TIMER1_LOAD		0x00000014
#define TIMER1_COUNTER	0x00000018

#define	TIMER1_LOAD_BIT		0x00000010
#define ENABLE_TIMERS_BIT	0x00000200

//Counter defines
#define ENABLE_COUNTER 0x80000000
#define RESET_COUNTER 0x40000000

#define NUM_COUNTERS 8

#define DESIRED_WAIT_TIME 1


extern char inbyte(void);
extern void outbyte(char c);

XTmrCtr timer0;
XTmrCtr* timer0ptr = &timer0;
int timerReloadCount = 0;
int seconds = 0;

int numIntervals = 0;
int currentInterval = 0;

volatile int done = 0;

unsigned long values[RELOAD][NUM_RING_OSCILLATORS];

void printValue(unsigned long value);
void determineIntervals(int seconds);
void printAllValues();

unsigned long addresses[NUM_COUNTERS] =
{
		XPAR_COUNTER_0_BASEADDR,
		XPAR_COUNTER_1_BASEADDR,
		XPAR_COUNTER_2_BASEADDR,
		XPAR_COUNTER_3_BASEADDR,
		XPAR_COUNTER_4_BASEADDR,
		XPAR_COUNTER_5_BASEADDR,
		XPAR_COUNTER_6_BASEADDR,
		XPAR_COUNTER_7_BASEADDR
};


typedef struct
{
	unsigned int ctrl_reg;
	unsigned int status_reg;
	unsigned int ver_reg;
} control_core_regs;

bool warmedUp = false;

void EnableCounter(unsigned long regAddress)
{
	unsigned int prevReg =  COUNTER_mReadSlaveReg2(regAddress, 0);
	unsigned int newReg = prevReg | ENABLE_COUNTER;
	COUNTER_mWriteSlaveReg2(regAddress, 0, newReg);
}

void DisableCounter(unsigned long regAddress)
{
	unsigned int prevReg =  COUNTER_mReadSlaveReg2(regAddress, 0);
	unsigned int newReg = prevReg & (~ENABLE_COUNTER);
	COUNTER_mWriteSlaveReg2(regAddress, 0, newReg);

}

void ResetCounter(unsigned long regAddress)
{
	unsigned int prevReg =  COUNTER_mReadSlaveReg2(regAddress, 0);
	unsigned int newReg = prevReg | RESET_COUNTER;
	COUNTER_mWriteSlaveReg2(regAddress, 0, newReg);
}

void DisableReset(unsigned long regAddress)
{
	unsigned int prevReg =  COUNTER_mReadSlaveReg2(regAddress, 0);
	unsigned int newReg = prevReg & (~RESET_COUNTER);
	COUNTER_mWriteSlaveReg2(regAddress, 0, newReg);
}


/*
 * Prints all of the counter values to the screen
 *
 */
void ReadCounters()
{
	int readReg0[NUM_COUNTERS];
	int readReg1[NUM_COUNTERS];
	for(int i = 0; i < NUM_COUNTERS; ++i)
	{
		readReg0[i] = COUNTER_mReadSlaveReg0(addresses[i], 0);
		readReg1[i] = COUNTER_mReadSlaveReg1(addresses[i], 0);
	}

	xil_printf("\r\nPrinting the counters: \r\n");
	for(int i = 0; i < NUM_COUNTERS; ++i)
	{
		xil_printf("reg0, counter%0d: ", i);
		xil_printf("%0d\r\n", readReg0[i]);
		xil_printf("reg1, counter%0d: ", i);
		xil_printf("%0d\r\n", readReg1[i]);

	}
}

/*
 * Sets the registers needed to start the ring oscillators indexed at first and second.
 * Not completely sure what the implementation will be.
 *
 */
void StartCounters(int first, int second)
{
	EnableCounter(addresses[first]);
	EnableCounter(addresses[second]);

}

/*
 * Returns the number of pairs that there will be based on the number of counters.
 *
 */
int GetNumPairs()
{
	int result = 0;
	for(int i = NUM_COUNTERS - 1; i > 0; --i)
	{
		result = i + result;
	}
	return result;
}


void GetCPR(int result[])
{
	//XTmrCtr_SetOptions(timer0ptr, XPAR_XPS_TIMER_0_DEVICE_ID, (XTC_AUTO_RELOAD_OPTION|XTC_INT_MODE_OPTION|XTC_DOWN_COUNT_OPTION));
	XTmrCtr_SetResetValue(timer0ptr, XPAR_XPS_TIMER_0_DEVICE_ID, (DESIRED_WAIT_TIME * CLK_TICKS));
	int firstCountShort = 0; // Short == slv_reg0
	int secondCountShort = 0;
	int firstCountLong = 0; // Long == slv_reg1
	int secondCountLong = 0;
	xil_printf("entered the cpr function\n\r");

	for(int i = 0; i < NUM_COUNTERS; ++i) // Resets all of the counters and sets them to 0;
	{
		ResetCounter(addresses[i]);
		DisableCounter(addresses[i]);
		DisableReset(addresses[i]);

	}
	int iteration = 0;
	for(int i = 0; i < NUM_COUNTERS; ++i)
	{
		for(int j = i + 1; j < NUM_COUNTERS; ++j)
		{
			ResetCounter(addresses[i]);
			ResetCounter(addresses[j]);
			DisableReset(addresses[i]);
			DisableReset(addresses[j]);

			EnableCounter(addresses[i]);
			EnableCounter(addresses[j]);
			XTmrCtr_Start(timer0ptr, XPAR_XPS_TIMER_0_DEVICE_ID);
			done = 0;
			while(!done)
			{
			}

			/*for(int l = 0; l < 1000; ++l)
			{
				print("");
			}
			DisableCounter(addresses[i]); // only needed because of the dummy loop.
			DisableCounter(addresses[j]);*/
			firstCountShort = COUNTER_mReadSlaveReg0(addresses[i], 0);
			secondCountShort = COUNTER_mReadSlaveReg0(addresses[j], 0);
			firstCountLong = COUNTER_mReadSlaveReg1(addresses[i], 0);
			secondCountLong = COUNTER_mReadSlaveReg1(addresses[j], 0);

			if (firstCountLong > secondCountLong)
			{
				result[iteration] = 1;
			}
			else if (firstCountLong == secondCountLong)
			{
				if(firstCountShort == secondCountShort )
				{
					result[iteration] = 19;
				}
				else if (firstCountShort > secondCountShort)
				{
					result[iteration] = 1;
				}
				else
				{
					result[iteration] = 2;
				}
			}
			else
			{
				result[iteration] = 2;
			}

			++iteration;

		}
	}

}

/*
 * Stops all of the counters
 *
 */
/*
 * Should this function look at all of the registers and find the 2 which have been enabled and only
 * Disable those 2 so that they are disable in sync with how they were enabled?
 */
void StopCounters()
{
	for(int i = 0; i < NUM_COUNTERS; ++i)
	{
		DisableCounter(addresses[i]);
	}
}

void interrupt_handler_dispatcher(void* ptr) {
	//Checking the timer interrupt
	xil_printf("made it into the interrupt.\r\n");
	if(!warmedUp) // warming up all of the counters
	{
		int intc_status = XIntc_GetIntrStatus(XPAR_INTC_0_BASEADDR);
		if (intc_status & XPAR_XPS_TIMER_0_INTERRUPT_MASK)
		{
			XIntc_AckIntr(XPAR_INTC_0_BASEADDR, XPAR_XPS_TIMER_0_INTERRUPT_MASK);
			currentInterval++;
			if (currentInterval == numIntervals)
			{
				currentInterval = 0;
				/*
				int slv0 = COUNTER_mReadSlaveReg0(XPAR_COUNTER_0_BASEADDR, 0);
				int slv1 = COUNTER_mReadSlaveReg1(XPAR_COUNTER_0_BASEADDR, 0);
				xil_printf("slv_reg0: %0d\n\r", slv0);
				xil_printf("slv_reg1: %0d\n\r", slv1);
				*/
				//ReadCounters();
				//EnableCounter(XPAR_COUNTER_0_BASEADDR);
				xil_printf("\r\n");
				ReadCounters();
				if (timerReloadCount < RELOAD)
				{
					timerReloadCount++;
					if (timerReloadCount == RELOAD)
					{
						//xil_printf("done.\r\n");
						warmedUp = true;
						done = 1;
					}
					else
					{
	//					RING_OSC_mWriteReg(XPAR_RING_OSC_1_BASEADDR, RING_OSC_SLV_REG2_OFFSET, 0xFFFFFFFF); //start
	//					RING_OSC_mWriteReg(XPAR_RING_OSC_2_BASEADDR, RING_OSC_SLV_REG2_OFFSET, 0xFFFFFFFF); //start
						XTmrCtr_Start(timer0ptr, XPAR_XPS_TIMER_0_DEVICE_ID);
					}
				}
				else
				{
					//xil_printf("done.\r\n");
					warmedUp = true;
					done = 1;
				}
			}
			else
			{
				XTmrCtr_Start(timer0ptr, XPAR_XPS_TIMER_0_DEVICE_ID);
			}
		}
	}
	else //generate the CPRs
	{
		done = 1;
		XIntc_AckIntr(XPAR_INTC_0_BASEADDR, XPAR_XPS_TIMER_0_INTERRUPT_MASK);
		StopCounters(); // stops all of the counters;

	}
}

int main()
{
	xil_printf("Hello World!\r\n");\


	for(int i = 0; i < NUM_COUNTERS; ++i) // This enables all of the counters so that they can start warming up.
	{
		EnableCounter(addresses[i]);
	}

	ReadCounters();

//	COUNTER_mWriteSlaveReg2(XPAR_COUNTER_0_BASEADDR, 0, 0x0000001);
//	EnableCounter(XPAR_COUNTER_0_BASEADDR);
	//Interrupts setup
	//
	microblaze_register_handler(interrupt_handler_dispatcher, NULL);
	XIntc_EnableIntr(XPAR_INTC_0_BASEADDR, XPAR_XPS_TIMER_0_INTERRUPT_MASK);
	XIntc_MasterEnable(XPAR_INTC_0_BASEADDR);
	microblaze_enable_interrupts();

	control_core_regs *core_regs = (control_core_regs*)(XPAR_DUT_CONTROL_CORE_0_BASEADDR);

	/* Assert DUT_UART_REQUEST to get access to Minicom */
	core_regs->ctrl_reg = DUT_UART_REQUEST;

	/* Wait until PUF Power Board Grants access to Minicom */
	while ((core_regs->status_reg & DUT_UART_GRANTED) != DUT_UART_GRANTED);
	xil_printf("made it this far 1.\r\n");
	//Initialize the timer
	XStatus Status;
	Status = XTmrCtr_Initialize(timer0ptr, XPAR_XPS_TIMER_0_DEVICE_ID);

	//Initialize the Software Registers
//	RING_OSC_mWriteReg(XPAR_RING_OSC_0_BASEADDR, RING_OSC_SLV_REG2_OFFSET, 0); //Reset

	if (Status != XST_SUCCESS)
	{
		xil_printf("\r\nTimer counter init failed\r\n");
		return XST_FAILURE;
	}


	Status = XTmrCtr_SelfTest(timer0ptr, 0);
	if (Status != XST_SUCCESS)
	{
		print("\r\nTimer counter self test failed\r\n");
		  return XST_FAILURE;
	}

	seconds = CHARACTERIZE_TIME;

	determineIntervals(seconds);

	//Configuring the timer
	XTmrCtr_SetOptions(timer0ptr, XPAR_XPS_TIMER_0_DEVICE_ID, (XTC_AUTO_RELOAD_OPTION|XTC_INT_MODE_OPTION|XTC_DOWN_COUNT_OPTION));
	XTmrCtr_SetResetValue(timer0ptr, XPAR_XPS_TIMER_0_DEVICE_ID, INTERVAL);

	XTmrCtr_Start(timer0ptr, XPAR_XPS_TIMER_0_DEVICE_ID);
	xil_printf("made it this far 2.\r\n");
	while (!done)
	{
		//Waiting for the timer interrupt
	}
	int numPairs = GetNumPairs();
	xil_printf("num pairs is: %0d\n\r", numPairs);
	int result[numPairs];
	GetCPR(result);
	xil_printf("printing the cpr results\n\r");
	for(int i = 0; i < numPairs; ++i)
	{
		xil_printf("%0d: ", i);
		xil_printf("%0d\n\r", result[i]);
	}

	xil_printf("made it this far 3.\r\n");

	printAllValues();
	xil_printf("done!\n\r");

	exit(1);
	return 0;
}

void printAllValues()
{
	int i;

	xil_printf("Test Region\r\n");
	for (i = 0; i < RELOAD; i++)
	{
		printValue(values[i][0]);
	}

	xil_printf("Control Region\r\n");
	for (i = 0; i < RELOAD; i++)
	{
		printValue(values[i][1]);
	}
	xil_printf("done!\r\n");
}

void printValue(unsigned long value)
{
	char buf[MAX_BUF];
	uint16_t digitIndex = 0;

	while (value != 0)
	{
		uint16_t digit = value % 10;
		buf[digitIndex] = '0' + digit;
		value -= digit;
		value /= 10;
		digitIndex++;
	}
	buf[digitIndex] = '\0';
	// Print out in backwards order.
	int i;
	for (i=digitIndex-1; i>=0; i--)
	{
		xil_printf("%c", buf[i]);
	}
	xil_printf("\r\n");
}

void determineIntervals(int seconds)
{
	numIntervals = seconds / 15;
}
