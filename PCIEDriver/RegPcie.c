#include "driver.h"
#include "RegPcie.tmh"

ULONG64
PcieDeviceGetVersion(
	_In_ PUCHAR BarXBase
	)
// hu 获取版本号
{
	ULONG code;
	ULONG version;
	ULONG64 outVer;
	PDMA_REGS	dmaRegs;

#ifdef DEBUG_HU
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "--> %!FUNC!");
#endif

	dmaRegs = (PCIE_DMA_REGS *)BarXBase;
	code = READ_REGISTER_ULONG((PULONG)&dmaRegs->ProjCode);
	version = READ_REGISTER_ULONG((PULONG)&dmaRegs->ProjVer);

	*(PULONG)&outVer = code;
	*((PULONG)&outVer + 1) = version;

#ifdef DEBUG_HU
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, 
		"Project code 0x%x", code);
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, 
		"Project version 0x%x", version);

	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "<-- %!FUNC!");
#endif
	return outVer;
}

ULONG
PcieDeviceReadReg(
	_In_ PUCHAR BarXBase,
	_In_ ULONG Address
	)
// hu 读取PCIE设备寄存器值
{
	ULONG ret = 0;

#ifdef DEBUG_HU
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "--> %!FUNC!");
#endif

	ret = READ_REGISTER_ULONG((PULONG)((ULONG_PTR)BarXBase + Address));

#ifdef DEBUG_HU
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, 
		"address 0x%x data 0x%x", (ULONG_PTR)BarXBase + Address, ret);

	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "<-- %!FUNC!");
#endif
	return ret;
}

VOID
PcieDeviceWriteReg(
	_In_ PUCHAR BarXBase,
	_In_ ULONG Address,
	_In_ ULONG Data
	)
// hu 写入PCIE设备寄存器值
{
#ifdef DEBUG_HU
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "--> %!FUNC!");
#endif

	WRITE_REGISTER_ULONG((PULONG)((ULONG_PTR)BarXBase + Address), Data);

#ifdef DEBUG_HU
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER,
		"address 0x%x data 0x%x", (ULONG_PTR)BarXBase + Address, Data);

	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "<-- %!FUNC!");
#endif
}

VOID
PcieDeviceResetHardWare(
	_In_ PUCHAR BarXBase
	)
// hu 初始化PCIE设备
{
	ULONG* pcieRegGPIO;
	PCIE_DMA_EXT1_REGS* pcieDmaExt1;

#ifdef DEBUG_HU
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "--> %!FUNC!");
#endif

	pcieRegGPIO = (PULONG)(BarXBase + REG_GPIO_OFFSET);
	WRITE_REGISTER_ULONG(pcieRegGPIO, 0);

	pcieDmaExt1 = (PCIE_DMA_EXT1_REGS *)(BarXBase + REG_EXT1_OFFSET);
	WRITE_REGISTER_ULONG((PULONG)&pcieDmaExt1->hostAccess, HOST_ACCESS_OK); // PC host is able to access dma registers
	WRITE_REGISTER_ULONG((PULONG)&pcieDmaExt1->opMode, OPMODE_NORMAL); // dma is working in normal mode

	PcieDeviceResetDMA(BarXBase);

#ifdef DEBUG_HU
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "<-- %!FUNC!");
#endif
}

VOID
PcieDeviceResetDMA(
	_In_ PUCHAR BarXBase
	)
// hu 复位DMA寄存器
{
	DmaCtl_u_t DmaCtl;
	PCIE_DMA_REGS *dmaRegs;

#ifdef DEBUG_HU
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "--> %!FUNC!");
#endif

	dmaRegs = (PCIE_DMA_REGS *)BarXBase;
	DmaCtl.ulong = 0;
	DmaCtl.bits.DmaRst = TRUE;
	WRITE_REGISTER_ULONG((PULONG)&dmaRegs->DmaCtl, DmaCtl.ulong);
	DmaCtl.bits.DmaRst = FALSE;
	WRITE_REGISTER_ULONG((PULONG)&dmaRegs->DmaCtl, DmaCtl.ulong);
	KeStallExecutionProcessor(10);

#ifdef DEBUG_HU
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "<-- %!FUNC!");
#endif
}

VOID
PcieDeviceSetupDMA(
	_In_ PUCHAR BarXBase,
	_In_ WDFINTERRUPT interrupt,
	_In_ PHYSICAL_ADDRESS hostAddress,
	_In_ ULONG size,
	_In_ ULONG direction,
	_In_ BOOLEAN descLoc	// Descriptor location : 0 - external 1 - internal
	)
// hu 设置DMA寄存器
{
	PCIE_DMA_REGS *dmaRegs;
	PDMA_TRANSFER_ELEMENT dteVA;

	dte_field3 descParamTmp;
	DmaDSa_u_t DmaDSa;

#ifdef DEBUG_HU
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "--> %!FUNC!");
#endif

	dmaRegs = (PCIE_DMA_REGS *)BarXBase;
	dteVA = (PDMA_TRANSFER_ELEMENT)(BarXBase + ITERN_DESC_OFFSET);

	WdfInterruptAcquireLock(interrupt);

	WRITE_REGISTER_ULONG((PULONG)&dteVA->Param, 0);
#ifdef SUPPORT_DMA64
	WRITE_REGISTER_ULONG((PULONG)&dteVA->DevAddressHigh, 0);
#endif
	// pcie device dma address (in scope of DMA engine)
	WRITE_REGISTER_ULONG((PULONG)&dteVA->DevAddressLow, ONBOARD_MEM_OFFSET);
#ifdef SUPPORT_DMA64
	WRITE_REGISTER_ULONG((PULONG)&dteVA->PciAddressHigh, hostAddress.HighPart);
#endif
	// dma buffer address in PC host side (physical address)
	WRITE_REGISTER_ULONG((PULONG)&dteVA->PciAddressLow, hostAddress.LowPart); 
	WRITE_REGISTER_ULONG((PULONG)&dteVA->NextElement, 0);
	// program size, direction and validate this descriptor
	descParamTmp.bits.DmaDVal = 1;
	descParamTmp.bits.DmaTDir = direction;
	descParamTmp.bits.DmaSize = size;
	WRITE_REGISTER_ULONG((PULONG)&dteVA->Param, descParamTmp.ulong);

	// Write the base address (physical) of the DMA_TRANSFER_ELEMENT list.
	if (descLoc){
		DmaDSa.ulong = ITERN_DESC_ADDR | 0x1; // indicate internal dte
	}
	else {
		DmaDSa.ulong = ITERN_DESC_ADDR; // indicate external dte
	}
	WRITE_REGISTER_ULONG((PULONG)&dmaRegs->DmaDSa, DmaDSa.ulong);

	WdfInterruptReleaseLock(interrupt);

#ifdef DEBUG_HU
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "<-- %!FUNC!");
#endif
}

NTSTATUS
PcieDeviceStartDMA(
	_In_ PUCHAR BarXBase,
	_In_ WDFINTERRUPT interrupt
	)
// hu 开始DMA传输
{
	NTSTATUS status = STATUS_SUCCESS;
	PCIE_DMA_REGS *dmaRegs;
	DmaCtl_u_t DmaCtl;

#ifdef DEBUG_HU
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "--> %!FUNC!");
#endif

	dmaRegs = (PCIE_DMA_REGS *)BarXBase;

	//
	// Start the DMA operation.
	// Acquire this device's InterruptSpinLock.
	// Note: ISR would access registers of DMA engine also
	WdfInterruptAcquireLock(interrupt);

	// Check if DMA busy or not?
	DmaCtl.ulong = READ_REGISTER_ULONG((PULONG)&dmaRegs->DmaCtl);
	if (DmaCtl.bits.DmaEna){
		WdfInterruptReleaseLock(interrupt);
		status = STATUS_UNSUCCESSFUL;
#ifdef DEBUG_HU
		TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "DMA is busy, reject Start request"); // should not reach here
#endif
		return status;
	}

	// Clear all the interrupt status flags
	PcieDeviceClearInterrupt(BarXBase);

	// Enable interrupt
	PcieDeviceEnableInterrupt(BarXBase);

	// Start the DMA operation: Set Enable bit
	DmaCtl.ulong = READ_REGISTER_ULONG((PULONG)&dmaRegs->DmaCtl);
	DmaCtl.bits.DmaEna = TRUE;
	WRITE_REGISTER_ULONG((PULONG)&dmaRegs->DmaCtl, DmaCtl.ulong);

	WdfInterruptReleaseLock(interrupt);

#ifdef DEBUG_HU
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER,
		"DmaInt %08X DmaSta %08X DmaDSa %08X DmaCtl %08X\n",
		dmaRegs->DmaInt,
		dmaRegs->DmaSta,
		dmaRegs->DmaDSa,
		dmaRegs->DmaCtl);

	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "<-- %!FUNC!");
#endif
	return status;
}

ULONG
PcieDeviceGetDMATime(
	_In_ PUCHAR BarXBase
	)
// hu 获取DMA传输所花时间
{
	ULONG time;
	PDMA_REGS	dmaRegs;

#ifdef DEBUG_HU
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "--> %!FUNC!");
#endif

	dmaRegs = (PCIE_DMA_REGS *)BarXBase;
	time = READ_REGISTER_ULONG((PULONG)&dmaRegs->Timer0);

#ifdef DEBUG_HU
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, 
		"DMA elapse time 0x%x", time);

	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "<-- %!FUNC!");
#endif
	return time;
}

ULONG
PcieDeviceGetInterrupt(
	_In_ PUCHAR BarXBase
	)
// hu 获取中断标志位
{
	PCIE_DMA_REGS *dmaRegs;

	ULONG PcieIntStatus;
	ULONG PcieIntMask;

#ifdef DEBUG_HU
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "--> %!FUNC!");
#endif

	dmaRegs = (PCIE_DMA_REGS *)BarXBase;

	//
	// Check interrupt status
	//
	PcieIntMask = READ_REGISTER_ULONG((PULONG)&dmaRegs->DmaInt);
	PcieIntStatus = READ_REGISTER_ULONG((PULONG)&dmaRegs->DmaSta);
#ifdef DEBUG_HU
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER,
		"PcieIntMask 0x%x PcieIntStatus 0x%x",
		PcieIntMask,
		PcieIntStatus);

	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "<-- %!FUNC!");
#endif
	return PcieIntMask & PcieIntStatus;
}

VOID
PcieDeviceClearInterrupt(
	_In_ PUCHAR BarXBase
	)
// hu 清除中断标志位
{
	PCIE_DMA_REGS *dmaRegs;
	ULONG		  PcieIntStatus;

#ifdef DEBUG_HU
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "--> %!FUNC!");
#endif

	dmaRegs = (PCIE_DMA_REGS *)BarXBase;

	PcieIntStatus = READ_REGISTER_ULONG((PULONG)&dmaRegs->DmaSta);
	WRITE_REGISTER_ULONG((PULONG)&dmaRegs->DmaSta, PcieIntStatus);

#ifdef DEBUG_HU
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "<-- %!FUNC!");
#endif
}

VOID
PcieDeviceTriggerInterrupt(
	_In_ PUCHAR BarXBase
	)
// hu 触发逻辑中断
{
	PCIE_MAILBOX_REGS *mb;
	PCIE_DMA_REGS *dmaRegs;
	DmaInt_u_t DmaInt;

#ifdef DEBUG_HU
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "--> %!FUNC!");
#endif

	dmaRegs = (PCIE_DMA_REGS *)BarXBase;

	DmaInt.ulong = 0;
	DmaInt.bits.TrigInt = TRUE;
	WRITE_REGISTER_ULONG((PULONG)&dmaRegs->DmaInt, DmaInt.ulong);

	// send command
	mb = (PCIE_MAILBOX_REGS *)(BarXBase + MAILBOX_OFFSET);
	WRITE_REGISTER_ULONG((PULONG)&mb->trig_int, TRIGGER_INT_CMD);

#ifdef DEBUG_HU
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, 
		"data at mb->trig_int 0x%x\n", 
		READ_REGISTER_ULONG((PULONG)&mb->trig_int));

	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "<-- %!FUNC!");
#endif
}

VOID
PcieDeviceEnableInterrupt(
	_In_ PUCHAR BarXBase
	)
// hu 使能中断
{
	PCIE_DMA_REGS *dmaRegs;

#ifdef DEBUG_HU
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "--> %!FUNC!");
#endif

	dmaRegs = (PCIE_DMA_REGS *)BarXBase;

	WRITE_REGISTER_ULONG((PULONG)&dmaRegs->DmaInt, INT_MASK_ENABLE);

#ifdef DEBUG_HU
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "<-- %!FUNC!");
#endif
}

VOID
PcieDeviceDisableInterrupt(
	_In_ PUCHAR BarXBase
	)
// hu 屏蔽中断
{
	PCIE_DMA_REGS *dmaRegs;

#ifdef DEBUG_HU
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "--> %!FUNC!");
#endif

	dmaRegs = (PCIE_DMA_REGS *)BarXBase;

	WRITE_REGISTER_ULONG((PULONG)&dmaRegs->DmaInt, INT_MASK_DISABLE_ALL);

#ifdef DEBUG_HU
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "<-- %!FUNC!");
#endif
}