/*****************************************************************************
* Filename:          /home/adam/RO-PUF/XPS_workspace/hw/drivers/counter_v1_01_a/src/counter.h
* Version:           1.01.a
* Description:       counter Driver Header File
* Date:              Mon Apr 30 13:08:57 2018 (by Create and Import Peripheral Wizard)
*****************************************************************************/

#ifndef COUNTER_H
#define COUNTER_H

/***************************** Include Files *******************************/

#include "xbasic_types.h"
#include "xstatus.h"
#include "xil_io.h"

/************************** Constant Definitions ***************************/


/**
 * User Logic Slave Space Offsets
 * -- SLV_REG0 : user logic slave module register 0
 * -- SLV_REG1 : user logic slave module register 1
 * -- SLV_REG2 : user logic slave module register 2
 */
#define COUNTER_USER_SLV_SPACE_OFFSET (0x00000000)
#define COUNTER_SLV_REG0_OFFSET (COUNTER_USER_SLV_SPACE_OFFSET + 0x00000000)
#define COUNTER_SLV_REG1_OFFSET (COUNTER_USER_SLV_SPACE_OFFSET + 0x00000004)
#define COUNTER_SLV_REG2_OFFSET (COUNTER_USER_SLV_SPACE_OFFSET + 0x00000008)

/**
 * Software Reset Space Register Offsets
 * -- RST : software reset register
 */
#define COUNTER_SOFT_RST_SPACE_OFFSET (0x00000100)
#define COUNTER_RST_REG_OFFSET (COUNTER_SOFT_RST_SPACE_OFFSET + 0x00000000)

/**
 * Software Reset Masks
 * -- SOFT_RESET : software reset
 */
#define SOFT_RESET (0x0000000A)

/**************************** Type Definitions *****************************/


/***************** Macros (Inline Functions) Definitions *******************/

/**
 *
 * Write a value to a COUNTER register. A 32 bit write is performed.
 * If the component is implemented in a smaller width, only the least
 * significant data is written.
 *
 * @param   BaseAddress is the base address of the COUNTER device.
 * @param   RegOffset is the register offset from the base to write to.
 * @param   Data is the data written to the register.
 *
 * @return  None.
 *
 * @note
 * C-style signature:
 * 	void COUNTER_mWriteReg(Xuint32 BaseAddress, unsigned RegOffset, Xuint32 Data)
 *
 */
#define COUNTER_mWriteReg(BaseAddress, RegOffset, Data) \
 	Xil_Out32((BaseAddress) + (RegOffset), (Xuint32)(Data))

/**
 *
 * Read a value from a COUNTER register. A 32 bit read is performed.
 * If the component is implemented in a smaller width, only the least
 * significant data is read from the register. The most significant data
 * will be read as 0.
 *
 * @param   BaseAddress is the base address of the COUNTER device.
 * @param   RegOffset is the register offset from the base to write to.
 *
 * @return  Data is the data from the register.
 *
 * @note
 * C-style signature:
 * 	Xuint32 COUNTER_mReadReg(Xuint32 BaseAddress, unsigned RegOffset)
 *
 */
#define COUNTER_mReadReg(BaseAddress, RegOffset) \
 	Xil_In32((BaseAddress) + (RegOffset))


/**
 *
 * Write/Read 32 bit value to/from COUNTER user logic slave registers.
 *
 * @param   BaseAddress is the base address of the COUNTER device.
 * @param   RegOffset is the offset from the slave register to write to or read from.
 * @param   Value is the data written to the register.
 *
 * @return  Data is the data from the user logic slave register.
 *
 * @note
 * C-style signature:
 * 	void COUNTER_mWriteSlaveRegn(Xuint32 BaseAddress, unsigned RegOffset, Xuint32 Value)
 * 	Xuint32 COUNTER_mReadSlaveRegn(Xuint32 BaseAddress, unsigned RegOffset)
 *
 */
#define COUNTER_mWriteSlaveReg0(BaseAddress, RegOffset, Value) \
 	Xil_Out32((BaseAddress) + (COUNTER_SLV_REG0_OFFSET) + (RegOffset), (Xuint32)(Value))
#define COUNTER_mWriteSlaveReg1(BaseAddress, RegOffset, Value) \
 	Xil_Out32((BaseAddress) + (COUNTER_SLV_REG1_OFFSET) + (RegOffset), (Xuint32)(Value))
#define COUNTER_mWriteSlaveReg2(BaseAddress, RegOffset, Value) \
 	Xil_Out32((BaseAddress) + (COUNTER_SLV_REG2_OFFSET) + (RegOffset), (Xuint32)(Value))

#define COUNTER_mReadSlaveReg0(BaseAddress, RegOffset) \
 	Xil_In32((BaseAddress) + (COUNTER_SLV_REG0_OFFSET) + (RegOffset))
#define COUNTER_mReadSlaveReg1(BaseAddress, RegOffset) \
 	Xil_In32((BaseAddress) + (COUNTER_SLV_REG1_OFFSET) + (RegOffset))
#define COUNTER_mReadSlaveReg2(BaseAddress, RegOffset) \
 	Xil_In32((BaseAddress) + (COUNTER_SLV_REG2_OFFSET) + (RegOffset))

/**
 *
 * Reset COUNTER via software.
 *
 * @param   BaseAddress is the base address of the COUNTER device.
 *
 * @return  None.
 *
 * @note
 * C-style signature:
 * 	void COUNTER_mReset(Xuint32 BaseAddress)
 *
 */
#define COUNTER_mReset(BaseAddress) \
 	Xil_Out32((BaseAddress)+(COUNTER_RST_REG_OFFSET), SOFT_RESET)

/************************** Function Prototypes ****************************/


/**
 *
 * Run a self-test on the driver/device. Note this may be a destructive test if
 * resets of the device are performed.
 *
 * If the hardware system is not built correctly, this function may never
 * return to the caller.
 *
 * @param   baseaddr_p is the base address of the COUNTER instance to be worked on.
 *
 * @return
 *
 *    - XST_SUCCESS   if all self-test code passed
 *    - XST_FAILURE   if any self-test code failed
 *
 * @note    Caching must be turned off for this function to work.
 * @note    Self test may fail if data memory and device are not on the same bus.
 *
 */
XStatus COUNTER_SelfTest(void * baseaddr_p);

#endif /** COUNTER_H */
