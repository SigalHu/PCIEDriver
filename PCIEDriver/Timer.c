#include "driver.h"
#include "Timer.tmh"

NTSTATUS
PcieDMATimerCreate(
	_In_ WDFTIMER *Timer,
	_In_ WDFDEVICE Device,
	_In_ PFN_WDF_TIMER EvtTimerFunc
	)
{
	NTSTATUS status;
	WDF_TIMER_CONFIG timerConfig;
	WDF_OBJECT_ATTRIBUTES timerAttributes;

#ifdef DEBUG_HU
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "--> %!FUNC!");
#endif

	WDF_TIMER_CONFIG_INIT(&timerConfig, EvtTimerFunc);

	WDF_OBJECT_ATTRIBUTES_INIT(&timerAttributes);
	timerAttributes.ParentObject = Device;

	status = WdfTimerCreate(
		&timerConfig,
		&timerAttributes,
		Timer
		);

#ifdef DEBUG_HU
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "<-- %!FUNC!");
#endif
	return status;
}

BOOLEAN
PcieDMATimerStart(
	_In_ WDFTIMER Timer
	)
{
	BOOLEAN status;

#ifdef DEBUG_HU
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "--> %!FUNC!");
#endif

	status = WdfTimerStart(Timer, WDF_REL_TIMEOUT_IN_MS(5000));

#ifdef DEBUG_HU
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "<-- %!FUNC!");
#endif
	return status;
}

BOOLEAN
PcieDMATimerStop(
	_In_ WDFTIMER Timer
	)
{
	BOOLEAN status;

#ifdef DEBUG_HU
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "--> %!FUNC!");
#endif

	status = WdfTimerStop(Timer, FALSE);

#ifdef DEBUG_HU
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "<-- %!FUNC!");
#endif
	return status;
}

VOID 
DmaWriteTimerEventFunc(
	_In_ WDFTIMER Timer
	)
{
	PDEVICE_CONTEXT devExt;

#ifdef DEBUG_HU
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "--> %!FUNC!");

	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "DmaWrite timeout\n");
#endif

	devExt = PcieGetDeviceContext(WdfTimerGetParentObject(Timer));

	// Disable DMA interrupt
	WdfInterruptAcquireLock(devExt->Interrupt);

	devExt->WriteTimeout = TRUE;
	if (devExt->MemBarBase){
		PcieDeviceResetDMA(devExt->MemBarBase);
		PcieDeviceDisableInterrupt(devExt->MemBarBase);
	}

	WdfInterruptReleaseLock(devExt->Interrupt);

	WdfRequestCompleteWithInformation(devExt->WriteRequest, STATUS_INVALID_DEVICE_STATE, 0);

#ifdef DEBUG_HU
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "<-- %!FUNC!");
#endif
}

VOID
	DmaReadTimerEventFunc(
	_In_ WDFTIMER Timer
	)
{
	PDEVICE_CONTEXT devExt;

#ifdef DEBUG_HU
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "--> %!FUNC!");

	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "DmaRead timeout\n");
#endif

	devExt = PcieGetDeviceContext(WdfTimerGetParentObject(Timer));

	// Disable DMA interrupt
	WdfInterruptAcquireLock(devExt->Interrupt);

	devExt->ReadTimeout = TRUE;
	if (devExt->MemBarBase){
		PcieDeviceResetDMA(devExt->MemBarBase);
		PcieDeviceDisableInterrupt(devExt->MemBarBase);
	}

	WdfInterruptReleaseLock(devExt->Interrupt);

	WdfRequestCompleteWithInformation(devExt->ReadRequest, STATUS_INVALID_DEVICE_STATE, 0);

#ifdef DEBUG_HU
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "<-- %!FUNC!");
#endif
}