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
#include <stdbool.h>
#include <stdint.h>
#include <xil_printf.h>
#include <xparameters.h>


//These are just temporary macros. Once we get the hardware file all worked out, we shouldn't need these anymore.
//I just want to start working now.
/*#define XPAR_RING_OSC_0_BASEADDR 0xC06E0000
#define XPAR_RING_OSC_1_BASEADDR 0xC06C0000
#define XPAR_RING_OSC_2_BASEADDR 0xC06A0000
#define XPAR_RING_OSC_3_BASEADDR 0xC0680000
#define XPAR_RING_OSC_4_BASEADDR 0xC0660000
#define XPAR_RING_OSC_5_BASEADDR 0xC0640000
#define XPAR_RING_OSC_6_BASEADDR 0xC0620000
#define XPAR_RING_OSC_7_BASEADDR 0xC0600000
*/
#define NUM_PUFS 8

//RingOsc defines
#define ENABLE_RING_OSC 0x80000000 //Still the backwards versions.
#define RESET_RING_OSC  0x40000000


static uint32_t addresses[NUM_PUFS] =
{
		/*XPAR_RING_OSC_0_BASEADDR,
		XPAR_RING_OSC_1_BASEADDR,
		XPAR_RING_OSC_2_BASEADDR,
		XPAR_RING_OSC_3_BASEADDR,
		XPAR_RING_OSC_4_BASEADDR,
		XPAR_RING_OSC_5_BASEADDR,
		XPAR_RING_OSC_6_BASEADDR,
		XPAR_RING_OSC_7_BASEADDR*/
};

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


int main()
{
	//printf("Hello world");

	xil_printf("Hello world! \n\rProgram Begun\n\r");
	EnableAll(); //enable all of the oscillators so that they can start warming up.
	ReadAll();
	DisableAll();
	ResetAll();




	xil_printf("Clear the buffer!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\r");


	return 0;
}
