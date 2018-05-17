/*
 * main.c
 *
 *  Created on: May 15, 2018
 *      Author: adam
 */
#include <stdio.h>
#include <xuartlite.h>
#include <xuartlite_l.h>
#include <xio.h>
#include "ring_osc.h"
#include "xtmrctr.h"
#include "xintc_l.h"
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdint.h>
#include <xparameters.h>

//Adam B's Defines

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
#define NUM_RING_OSCILLATORS 8
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




extern char inbyte(void);
extern void outbyte(char c);

//My defines

#define WARM_UP_TIME 25 //in seconds
#define NUM_PUFS 8
#define DESIRED_WAIT_TIME 15 //in seconds
#define NUM_ATTEMPTS 10

//RingOsc defines
#define ENABLE_RING_OSC 0x80000000 //Still the backwards versions.
#define RESET_RING_OSC  0x40000000

#define DEBUG 0

//Global Variables needed
static uint32_t addresses[NUM_PUFS] =
{
		XPAR_RING_OSC_0_BASEADDR,
		XPAR_RING_OSC_1_BASEADDR,
		XPAR_RING_OSC_2_BASEADDR,
		XPAR_RING_OSC_3_BASEADDR,
		XPAR_RING_OSC_4_BASEADDR,
		XPAR_RING_OSC_5_BASEADDR,
		XPAR_RING_OSC_6_BASEADDR,
		XPAR_RING_OSC_7_BASEADDR
};

typedef struct
{
	unsigned int ctrl_reg;
	unsigned int status_reg;
	unsigned int ver_reg;
} control_core_regs;

typedef enum { FIRST_WINS, SECOND_WINS, SAME} pairResults;

typedef struct
{
	uint8_t index;
	uint32_t countHigh;
	uint32_t countLow;
} ringFrequency;

volatile int warmedUp = 0;

XTmrCtr timer0;
XTmrCtr* timer0ptr = &timer0;
int timerReloadCount = 0;
int seconds = 0;

int numIntervals = 0;
int currentInterval = 0;

volatile int done = 0;


/*
 * Enables the ring oscillator at regAddress
 * regAddrss is the base address of the ring that is to be turned on
 *
 * return nothing
 */
void EnableRingOsc(uint32_t regAddress)
{
	uint32_t prevReg =  RING_OSC_mReadSlaveReg2(regAddress, 0);
	uint32_t newReg = prevReg | ENABLE_RING_OSC;
	RING_OSC_mWriteSlaveReg2(regAddress, 0, newReg);
}

/*
 * Disables the ring oscillator at regAddress
 * regAddress is the base address of the ring that is to be turned off
 *
 * returns nothing
 */
void DisableRingOsc(uint32_t regAddress)
{
	uint32_t prevReg =  RING_OSC_mReadSlaveReg2(regAddress, 0);
	uint32_t newReg = prevReg & (~ENABLE_RING_OSC);
	RING_OSC_mWriteSlaveReg2(regAddress, 0, newReg);

}

/*
 * Turns off the reset of the ring oscillator at regAddress
 * regAddress is the base address of the ring whose reset is to be disabled.
 *
 * returns nothing.
 */
void DisableReset(uint32_t regAddress)
{
	uint32_t prevReg =  RING_OSC_mReadSlaveReg2(regAddress, 0);
	uint32_t newReg = prevReg & (~RESET_RING_OSC);
	RING_OSC_mWriteSlaveReg2(regAddress, 0, newReg);
}


/*
 * Resets the count of the Ring Oscillator at regAddress.
 * The RingOscillator will be disabled by the end of the function and will no longer be oscillating.
 * regAddress is the base address of the ring that is to be reset
 *
 * returns nothing
 */
void ResetRingOsc(uint32_t regAddress)
{

	EnableRingOsc(regAddress); //There is a slight bug in the hardware. It can only be reset if the enable is high.
	//This is where we change the register.
	uint32_t prevReg =  RING_OSC_mReadSlaveReg2(regAddress, 0);
	uint32_t newReg = prevReg | RESET_RING_OSC;
	RING_OSC_mWriteSlaveReg2(regAddress, 0, newReg); //This is where the reset actually takes place.
	DisableRingOsc(regAddress); //Stops the reset
	DisableReset(regAddress); //Stops the ring osc from continuously loading 0s.
}



/*
 * Enables all of the RingOscs
 *
 * returns nothing
 */

void EnableAll()
{
	for(int i = 0; i < NUM_PUFS; ++i)
	{
		EnableRingOsc(addresses[i]);
	}
}

/*
 * Disables all of the RingOscs
 */
void DisableAll()
{
	for(int i = 0; i < NUM_PUFS; ++i)
	{
		DisableRingOsc(addresses[i]);
	}
}

/*
 * Reset All of the ring oscillator counters.
 */
void ResetAll()
{
	for(int i = 0; i < NUM_PUFS; ++i)
	{
		ResetRingOsc(addresses[i]);
	}
}

/*
 * Disable the reset for all of the ring oscillator counters.
 */

void DisableResetAll()
{
	for(int i = 0; i < NUM_PUFS; ++i)
	{
		DisableReset(addresses[i]);
	}
}

/*
 * Prints the value of the ring oscillator at regAddress
 * regAddress is the base address of the ring whose value will be read.
 *
 * returns the value of the ring osc
 */
uint64_t ReadRingOsc(uint32_t regAddress)
{
	uint32_t upperBits = RING_OSC_mReadSlaveReg1(regAddress, 0);
	uint32_t lowerBits = RING_OSC_mReadSlaveReg0(regAddress, 0);
	uint64_t wholeCount = upperBits;
	wholeCount = wholeCount << 32;
	wholeCount = wholeCount + lowerBits;

	xil_printf("0x%8.8x%8.8x\n\r", upperBits, lowerBits);

	return wholeCount;
}
/*
 * Reads all of the ring oscillators
 */
void ReadAll()
{
	xil_printf("Reading all of the ring oscillators \n\r");
	for(int i = 0; i < NUM_PUFS; ++i)
	{
		xil_printf("%d: ", i);
		ReadRingOsc(addresses[i]);
	}
}

/*
 * Returns the number of pairs that there will be based on the number of counters.
 *
 */
int GetNumPairs()
{
	int result = 0;
	for(int i = NUM_PUFS - 1; i > 0; --i) //Sums up all of the possible combinations
	{
		result = i + result;
	}
	return result;
}

/*
 * Gets all of the CRP results for the ring oscillators. Collecting them on pair at a time.
 *
 * result is an array where the pairs will be stored
 *
 * returns nothing.
 *
 */
void GetCRP(pairResults result[])
{
	//XTmrCtr_SetOptions(timer0ptr, XPAR_XPS_TIMER_0_DEVICE_ID, (XTC_AUTO_RELOAD_OPTION|XTC_INT_MODE_OPTION|XTC_DOWN_COUNT_OPTION));
	XTmrCtr_SetResetValue(timer0ptr, XPAR_XPS_TIMER_0_DEVICE_ID, (DESIRED_WAIT_TIME * CLK_TICKS));
	int firstCountShort = 0; // Short == slv_reg0
	int secondCountShort = 0;
	int firstCountLong = 0; // Long == slv_reg1
	int secondCountLong = 0;
	xil_printf("entered the cpr gathering by pair function\n\r");

	ResetAll();
	int iteration = 0;
	for(int i = 0; i < NUM_PUFS; ++i)
	{
		for(int j = i + 1; j < NUM_PUFS; ++j)
		{
			ResetRingOsc(addresses[i]);
			ResetRingOsc(addresses[j]);

			//EnableRingOsc(addresses[i]);
			//EnableRingOsc(addresses[j]);
			EnableAll(); //Enabling all of the pufs in order to keep them warmed up and to keep the timing consistent when they are turned off.
			XTmrCtr_Start(timer0ptr, XPAR_XPS_TIMER_0_DEVICE_ID);
			done = 0;
			while(!done)
			{
				//Waiting for the timing interrupt
			}

			firstCountShort = RING_OSC_mReadSlaveReg0(addresses[i], 0);
			secondCountShort = RING_OSC_mReadSlaveReg0(addresses[j], 0);
			firstCountLong = RING_OSC_mReadSlaveReg1(addresses[i], 0);
			secondCountLong = RING_OSC_mReadSlaveReg1(addresses[j], 0);

			if (firstCountLong > secondCountLong) //First is faster
			{
				result[iteration] = FIRST_WINS;
			}
			else if (firstCountLong == secondCountLong) //the top bit are the same
			{
				if(firstCountShort == secondCountShort ) //they'r exactly the same
				{
					result[iteration] = SAME;
				}
				else if (firstCountShort > secondCountShort) //first is faster
				{
					result[iteration] = FIRST_WINS;
				}
				else //second is faster
				{
					result[iteration] = SECOND_WINS;
				}
			}
			else //second is faster
			{
				result[iteration] = SECOND_WINS;
			}

			++iteration;

		}
	}

}


/*
 * Characterize the PUFs in the 'traditional' way of challenge pairs.
 * It collects one pair result for each DESIRED_WAIT_TIME
 *
 */

void CharacterizeByChallenge()
{
	int numPairs = GetNumPairs();
	if(DEBUG)
	{
		xil_printf("num pairs is: %0d\n\r", numPairs);
	}
	pairResults crps[NUM_ATTEMPTS][numPairs]; //challenge response pairs.
	for(int i = 0; i < NUM_ATTEMPTS; ++i)
	{
		GetCRP(crps[i]);
	}

	xil_printf("printing the cpr results by pair\n\r");
	for(int i = 0; i < numPairs; ++i)
	{
		xil_printf("%0d: ", i);
		for(int j = 0; j < NUM_ATTEMPTS; ++j)
		{
			xil_printf("%0d, ", crps[j][i]);
		}
		xil_printf("\n\r");
	}
}


/*
 * Swaps 2 elements in array of ringFrequencies
 *
 * array is the array with the elements to swap
 * first is the index of the first element
 * second is the index of the second element
 *
 * returns nothing
 */
void Swap(ringFrequency array[], int first, int second)
{
	if(first != second)
	{
		ringFrequency temp = array[first];
		array[first] = array[second];
		array[second] = temp;
	}
}


/*
 * Sorts the results array from high frequency to low frequency.
 *
 */
void FrequencySort(ringFrequency results[])
{
	//This is using a selection sort algorithm. There are likely faster ways of doing this.
	for(int i = 0; i < NUM_PUFS; ++i)
	{
		int indexMax = i;
		uint32_t maxCountHigh = results[i].countHigh;
		uint32_t maxCountLow = results[i].countLow;
		for(int j = i+1; j < NUM_PUFS; ++j)
		{
			if((results[j].countHigh > maxCountHigh) ||
					( (results[j].countHigh == maxCountHigh) && (results[j].countLow > maxCountLow) ))
			{
				indexMax = j;
				maxCountHigh = results[j].countHigh;
				maxCountLow = results[j].countLow;
			}
		}
		Swap(results, i, indexMax);
	}
}



/*
 * Characterize the PUFs in the less traditional way of just measuring all of the frequencies and then
 * ranking them in terms of their frequencies.
 *
 */

void CharacterizeByFrequency()
{
	ringFrequency results[NUM_ATTEMPTS][NUM_PUFS];
	XTmrCtr_SetResetValue(timer0ptr, XPAR_XPS_TIMER_0_DEVICE_ID, (DESIRED_WAIT_TIME * CLK_TICKS));
	for(int j = 0; j < NUM_ATTEMPTS; ++j) //Run the same test 10 times.
	{
		xil_printf("Attempt %d: \n\r", j);
		ResetAll();
		EnableAll();
		XTmrCtr_Start(timer0ptr, XPAR_XPS_TIMER_0_DEVICE_ID);

		done = 0;
		while(!done)
		{
			//wait for the timer interrupt
		}
		//The ring oscillators should be stopped by the interrupt function.
		for(int i = 0; i < NUM_PUFS; ++i)
		{
			results[j][i].index = i;
			results[j][i].countLow = RING_OSC_mReadSlaveReg0(addresses[i], 0);
			results[j][i].countHigh = RING_OSC_mReadSlaveReg1(addresses[i], 0);
		}

		FrequencySort(results[j]);
	}
	xil_printf("Printing Frequency results from highest to lowest (Top to Bottom) \n\r");
	xil_printf("(Attempts Left to Right)\n\r");
	for(int i = 0; i < NUM_PUFS; ++i)
	{
		xil_printf("Ring Index: ");
		for(int j = 0; j < NUM_ATTEMPTS; ++j)
		{
			xil_printf("%d, ", results[j][i].index);
		}
		xil_printf("\n\r");
	}
	xil_printf("Individual counts (on the last attempt)\n\r");
	for(int i = 0; i < NUM_PUFS; ++i)
	{
		xil_printf("Ring Index: %d Count: 0x%8x%8x \n\r", results[NUM_ATTEMPTS - 1][i].index, results[NUM_ATTEMPTS - 1][i].countHigh, results[NUM_ATTEMPTS - 1][i].countLow);
	}

	xil_printf("Individual counts (on the middle attempt) to se the difference\n\r");
	for(int i = 0; i < NUM_PUFS; ++i)
	{
		xil_printf("Ring Index: %d Count: 0x%8x%8x \n\r", results[NUM_ATTEMPTS/2][i].index, results[NUM_ATTEMPTS/2][i].countHigh, results[NUM_ATTEMPTS/2][i].countLow);
	}



}


void TestRingOsc()
{
	xil_printf("Hello........... World!\r\n");
	xil_printf("Hello........... World!\r\n");
	xil_printf("Hello........... World!\r\n");
	xil_printf("Hello........... World!\r\n");
	xil_printf("Hello........... World!\r\n");

	xil_printf("At the start\n\r");
	ReadAll();
	EnableAll();
	for(int i = 0; i < 500; ++i)
	{
		xil_printf(" "); //Wait a little while.
	}
	xil_printf("\n\r");
	xil_printf("After a wait\n\r");
	ReadAll();
	DisableAll();
	xil_printf("After the disable\n\r");
	ReadAll();
	ResetAll();
	//DisableResetAll();
	xil_printf("After the reset\n\r");
	ReadAll();

	xil_printf("Goodbye........... World!\r\n");
	xil_printf("Goodbye........... World!\r\n");
	xil_printf("Goodbye........... World!\r\n");
	xil_printf("Goodbye........... World!\r\n");
	xil_printf("Goodbye........... World!\r\n");
}


/*
void determineIntervals(int seconds)
{
	numIntervals = seconds / 15;
}
*/

void interrupt_handler_dispatcher(void* ptr) {
	//Checking the timer interrupt
	if(DEBUG)
	{
		xil_printf("made it into the interrupt.\r\n");
	}
	if(!warmedUp) // warming up all of the counters
	{
		int intc_status = XIntc_GetIntrStatus(XPAR_INTC_0_BASEADDR);
		if (intc_status & XPAR_XPS_TIMER_0_INTERRUPT_MASK)
		{
			XIntc_AckIntr(XPAR_INTC_0_BASEADDR, XPAR_XPS_TIMER_0_INTERRUPT_MASK);
			if(DEBUG)
			{
				xil_printf("Warm up done\n\r");
			}
			warmedUp = 1;
			done = 1;
		}
		else
		{
			XTmrCtr_Start(timer0ptr, XPAR_XPS_TIMER_0_DEVICE_ID);
		}
	}
	else //generate the CRPs
	{
		int intc_status = XIntc_GetIntrStatus(XPAR_INTC_0_BASEADDR);
		if(intc_status & XPAR_XPS_TIMER_0_INTERRUPT_MASK)
		{
			done = 1;
			XIntc_AckIntr(XPAR_INTC_0_BASEADDR, XPAR_XPS_TIMER_0_INTERRUPT_MASK);
			DisableAll();// stops all of the PUFS
		}
		else
		{
			XTmrCtr_Start(timer0ptr, XPAR_XPS_TIMER_0_DEVICE_ID);
		}

	}
}


int main()
{
	//printf("Hello world");


	xil_printf("Hello world! \n\rProgram Begun\n\r");
	EnableAll(); //enable all of the oscillators so that they can start warming up.
	warmedUp = 0;

	microblaze_register_handler(interrupt_handler_dispatcher, NULL);
	XIntc_EnableIntr(XPAR_INTC_0_BASEADDR, XPAR_XPS_TIMER_0_INTERRUPT_MASK);
	XIntc_MasterEnable(XPAR_INTC_0_BASEADDR);
	microblaze_enable_interrupts();

	control_core_regs *core_regs = (control_core_regs*)(XPAR_DUT_CONTROL_CORE_0_BASEADDR);

	/* Assert DUT_UART_REQUEST to get access to Minicom */
	core_regs->ctrl_reg = DUT_UART_REQUEST;

	/* Wait until PUF Power Board Grants access to Minicom */
	while ((core_regs->status_reg & DUT_UART_GRANTED) != DUT_UART_GRANTED);
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

	//Configuring the timer
	XTmrCtr_SetOptions(timer0ptr, XPAR_XPS_TIMER_0_DEVICE_ID, (XTC_AUTO_RELOAD_OPTION|XTC_INT_MODE_OPTION|XTC_DOWN_COUNT_OPTION));
	XTmrCtr_SetResetValue(timer0ptr, XPAR_XPS_TIMER_0_DEVICE_ID, (WARM_UP_TIME*CLK_TICKS));

	XTmrCtr_Start(timer0ptr, XPAR_XPS_TIMER_0_DEVICE_ID);
	while (!done)
	{
		//Waiting for the timer interrupt
	}

	CharacterizeByFrequency();

	DisableAll(); //Make sure that they are all disabled
	//ReadAll();


	xil_printf("Clear the buffer!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\r");

	ResetAll(); //Make sure that all of the ring oscillators are reset. at the end.
	return 0;
}
