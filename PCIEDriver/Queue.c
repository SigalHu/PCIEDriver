/*++

Module Name:

    queue.c

Abstract:

    This file contains the queue entry points and callbacks.

Environment:

    Kernel-mode Driver Framework

--*/

#include "driver.h"
#include "queue.tmh"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, PcieQueueInitialize)
#endif

NTSTATUS
PcieQueueInitialize(
	_In_ PDEVICE_CONTEXT DevExt
    )
/*++

Routine Description:


     The I/O dispatch callbacks for the frameworks device object
     are configured in this function.

     A single default I/O Queue is configured for parallel request
     processing, and a driver context memory allocation is created
     to hold our structure QUEUE_CONTEXT.

Arguments:

    Device - Handle to a framework device object.

Return Value:

    VOID

--*/
{
    NTSTATUS status;
    WDF_IO_QUEUE_CONFIG    queueConfig;

    PAGED_CODE();

#ifdef DEBUG_HU
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "--> %!FUNC!");
#endif

	//
	// Setup a queue to handle only IRP_MJ_WRITE requests in Sequential
	// dispatch mode. This mode ensures there is only one write request
	// outstanding in the driver at any time. Framework will present the next
	// request only if the current request is completed.
	// Since we have configured the queue to dispatch all the specific requests
	// we care about, we don't need a default queue.  A default queue is
	// used to receive requests that are not preconfigured to goto
	// a specific queue.
	// hu 初始化缺省队列配置，设置I/O请求分发处理方式为串行
	WDF_IO_QUEUE_CONFIG_INIT(&queueConfig, WdfIoQueueDispatchSequential);

	queueConfig.EvtIoWrite = PcieEvtIoWrite;

	status = WdfIoQueueCreate(DevExt->Device,
		&queueConfig,
		WDF_NO_OBJECT_ATTRIBUTES,
		&DevExt->WriteQueue);

	if (!NT_SUCCESS(status)) {
#ifdef DEBUG_HU
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER,
			"WdfIoQueueCreate failed: %!STATUS!", status);
#endif
		return status;
	}

	//
	// Set the Write Queue forwarding for IRP_MJ_WRITE requests.
	//
	status = WdfDeviceConfigureRequestDispatching(DevExt->Device,
		DevExt->WriteQueue,
		WdfRequestTypeWrite);

	if (!NT_SUCCESS(status)) {
#ifdef DEBUG_HU
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER,
			"WdfDeviceConfigureRequestDispatching failed: %!STATUS!", status);
#endif
		return status;
	}

	//
	// Create a new IO Queue for IRP_MJ_READ requests in sequential mode.
	//
	WDF_IO_QUEUE_CONFIG_INIT(&queueConfig, WdfIoQueueDispatchSequential);

	queueConfig.EvtIoRead = PcieEvtIoRead;

	status = WdfIoQueueCreate(DevExt->Device,
		&queueConfig,
		WDF_NO_OBJECT_ATTRIBUTES,
		&DevExt->ReadQueue);

	if (!NT_SUCCESS(status)) {
#ifdef DEBUG_HU
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER,
			"WdfIoQueueCreate failed: %!STATUS!", status);
#endif
		return status;
	}

	//
	// Set the Read Queue forwarding for IRP_MJ_READ requests.
	//
	status = WdfDeviceConfigureRequestDispatching(DevExt->Device,
		DevExt->ReadQueue,
		WdfRequestTypeRead);

	if (!NT_SUCCESS(status)) {
#ifdef DEBUG_HU
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER,
			"WdfDeviceConfigureRequestDispatching failed: %!STATUS!", status);
#endif
		return status;
	}


	//
	// Create a new IO Dispatch Queue for IRP_MJ_DEVICE_CONTROL  requests in sequential mode.
	//
	WDF_IO_QUEUE_CONFIG_INIT(&queueConfig, WdfIoQueueDispatchSequential);

	queueConfig.EvtIoDeviceControl = PcieEvtIoDeviceControl;

	status = WdfIoQueueCreate(DevExt->Device,
		&queueConfig,
		WDF_NO_OBJECT_ATTRIBUTES,
		&DevExt->IoDispatchQueue);

	if (!NT_SUCCESS(status)) {
#ifdef DEBUG_HU
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER,
			"WdfIoQueueCreate failed: %!STATUS!", status);
#endif
		return status;
	}

	//
	// Set the IO Dispatch Queue forwarding for IRP_MJ_DEVICE_CONTROL requests.
	//
	status = WdfDeviceConfigureRequestDispatching(DevExt->Device,
		DevExt->IoDispatchQueue,
		WdfRequestTypeDeviceControl);

	if (!NT_SUCCESS(status)) {
#ifdef DEBUG_HU
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER,
			"WdfDeviceConfigureRequestDispatching failed: %!STATUS!", status);
#endif
		return status;
	}

#ifdef DEBUG_HU
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "<-- %!FUNC!");
#endif
    return status;
}

VOID
PcieEvtIoWrite(
	_In_ WDFQUEUE         Queue,
	_In_ WDFREQUEST       Request,
	_In_ size_t           Length
	)
/*++

Routine Description:

Called by the framework as soon as it receives a write request.
If the device is not ready, fail the request.
Otherwise get scatter-gather list for this request and send the
packet to the hardware for DMA.

Arguments:

Queue - Handle to the framework queue object that is associated
with the I/O request.
Request - Handle to a framework request object.

Length - Length of the IO operation
The default property of the queue is to not dispatch
zero lenght read & write requests to the driver and
complete is with status success. So we will never get
a zero length request.

Return Value:

--*/
{
	NTSTATUS status = STATUS_SUCCESS;
	PDEVICE_CONTEXT devExt;
	PVOID 	in_buffer;
	size_t 	in_bufsize;

#ifdef DEBUG_HU
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "--> %!FUNC!: Request %p", Request);
#endif

	//
	// Get the DevExt from the Queue handle
	//
	devExt = PcieGetDeviceContext(WdfIoQueueGetDevice(Queue));

	//
	// Validate the Length parameter.
	//
	if (Length > MAX_DMA_SIZE_COMMONBUFFER)  {
		status = STATUS_INVALID_BUFFER_SIZE;
#ifdef DEBUG_HU
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER,
			"%!FUNC! failed: %!STATUS!", status);
#endif
		WdfRequestComplete(Request, status);
		return;
	}

	status = WdfRequestRetrieveInputBuffer(Request, 1, &in_buffer, &in_bufsize);
	if (!NT_SUCCESS(status)){
#ifdef DEBUG_HU
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER,
			"WdfRequestRetrieveInputBuffer failed: %!STATUS!", status);
#endif
		WdfRequestComplete(Request, status);
		return;
	}

	RtlCopyMemory(devExt->CommonBufferBase, in_buffer, in_bufsize);
	devExt->WriteDmaLength = in_bufsize;
	KeMemoryBarrier();

	devExt->WriteRequest = Request;
	devExt->DmaMode.bits.RdWr = TRUE;

	if (devExt->MemBarBase){
		PcieDeviceSetupDMA(devExt->MemBarBase,
			devExt->Interrupt,
			devExt->CommonBufferBaseLA,
			devExt->WriteDmaLength,
			DIRECTION_TO_DEVICE,
			1);

		KeMemoryBarrier();

		status = PcieDeviceStartDMA(devExt->MemBarBase, devExt->Interrupt);
		if (!NT_SUCCESS(status)){
#ifdef DEBUG_HU
			TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER,
				"PcieDeviceStartDMA failed: %!STATUS!", status);
#endif
			WdfRequestComplete(Request, status);
			return;
		}
	}

	devExt->WriteTimeout = FALSE;
	PcieDMATimerStart(devExt->WriteTimer);

#ifdef DEBUG_HU
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "<-- %!FUNC!: Request %p", Request);
#endif
}

VOID
PcieEvtIoRead(
	_In_ WDFQUEUE         Queue,
	_In_ WDFREQUEST       Request,
	_In_ size_t           Length
	)
/*++

Routine Description:

Called by the framework as soon as it receives a read request.
If the device is not ready, fail the request.
Otherwise get scatter-gather list for this request and send the
packet to the hardware for DMA.

Arguments:

Queue   	- Default queue handle
Request  	- Handle to the write request
Lenght 		- Length of the data buffer associated with the request.
The default property of the queue is to not dispatch
zero lenght read & write requests to the driver and
complete is with status success. So we will never get
a zero length request.

Return Value:

--*/
{
	NTSTATUS status = STATUS_SUCCESS;
	PDEVICE_CONTEXT devExt;
	PVOID 	out_buffer;
	size_t 	out_bufsize;

#ifdef DEBUG_HU
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "--> %!FUNC!: Request %p", Request);
#endif

	devExt = PcieGetDeviceContext(WdfIoQueueGetDevice(Queue));

	//
	// Validate the Length parameter.
	//
	if (Length > MAX_DMA_SIZE_COMMONBUFFER)  {
		status = STATUS_INVALID_BUFFER_SIZE;
#ifdef DEBUG_HU
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER,
			"%!FUNC! failed: %!STATUS!", status);
#endif
		WdfRequestComplete(Request, status);
		return;
	}

	status = WdfRequestRetrieveOutputBuffer(Request, 1, &out_buffer, &out_bufsize);
	if (!NT_SUCCESS(status)){
#ifdef DEBUG_HU
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER,
			"WdfRequestRetrieveOutputBuffer failed: %!STATUS!", status);
#endif
		WdfRequestComplete(Request, status);
		return;
	}

	devExt->ReadRequest = Request;
	devExt->DmaMode.bits.RdWr = FALSE;
	devExt->ReadDmaLength = out_bufsize;
	devExt->ReadBuffer = out_buffer;

	if (devExt->MemBarBase){
		PcieDeviceSetupDMA(devExt->MemBarBase,
			devExt->Interrupt,
			devExt->CommonBufferBaseLA,
			devExt->ReadDmaLength,
			DIRECTION_FROM_DEVICE,
			1);

		KeMemoryBarrier();

		status = PcieDeviceStartDMA(devExt->MemBarBase, devExt->Interrupt);
		if (!NT_SUCCESS(status)){
#ifdef DEBUG_HU
			TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER,
				"PcieDeviceStartDMA failed: %!STATUS!", status);
#endif
			WdfRequestComplete(Request, status);
			return;
		}
	}

	devExt->ReadTimeout = FALSE;
	PcieDMATimerStart(devExt->ReadTimer);

#ifdef DEBUG_HU
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "<-- %!FUNC!: Request %p", Request);
#endif
}

VOID
PcieEvtIoDeviceControl(
	_In_ WDFQUEUE   Queue,
	_In_ WDFREQUEST Request,
	_In_ size_t     OutputBufferLength,
	_In_ size_t     InputBufferLength,
	_In_ ULONG      IoControlCode
	)
{
	NTSTATUS  status = STATUS_SUCCESS;
	PDEVICE_CONTEXT devExt = NULL;

	int ret_length = 0;
	PVOID out_buffer;
	size_t out_bufsize;
	PVOID in_buffer;
	size_t in_bufsize;

	UNREFERENCED_PARAMETER(OutputBufferLength);
	UNREFERENCED_PARAMETER(InputBufferLength);

#ifdef DEBUG_HU
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "--> %!FUNC!: Request %p", Request);
#endif

	devExt = PcieGetDeviceContext(WdfIoQueueGetDevice(Queue));

	status = WdfRequestRetrieveOutputBuffer(Request, 1, &out_buffer, &out_bufsize);
	if (!NT_SUCCESS(status)){
		WdfRequestCompleteWithInformation(Request, status, ret_length);
#ifdef DEBUG_HU
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER,
			"WdfRequestRetrieveOutputBuffer failed: %!STATUS!", status);
#endif
		return;
	}

	status = WdfRequestRetrieveInputBuffer(Request, 1, &in_buffer, &in_bufsize);
	if (!NT_SUCCESS(status)){
		WdfRequestCompleteWithInformation(Request, status, ret_length);
#ifdef DEBUG_HU
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER,
			"WdfRequestRetrieveInputBuffer failed: %!STATUS!", status);
#endif
		return;
	}

#ifdef DEBUG_HU
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER,
		"PcieEvtIoDeviceControl: in_buffer 0x%x in_bufsize 0x%x",
		(unsigned int)in_buffer, in_bufsize);

	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER,
		"PcieEvtIoDeviceControl: out_buffer 0x%x out_bufsize 0x%x",
		(unsigned int)out_buffer, out_bufsize);
#endif

	switch (IoControlCode) {
		case PCIeDMA_IOCTL_GET_TIME:
		{
			ULONG time;
			if (devExt->MemBarBase){
				time = PcieDeviceGetDMATime(devExt->MemBarBase);
				*(PULONG)out_buffer = time;
				ret_length = sizeof(ULONG);
				status = STATUS_SUCCESS;
			}
			break;
		}
		case PCIeDMA_IOCTL_GET_VERSION:
		{
			ULONG64 version;
			if (devExt->MemBarBase){
				version = PcieDeviceGetVersion(devExt->MemBarBase);
				*(PULONG64)out_buffer = version;
				ret_length = sizeof(ULONG64);
				status = STATUS_SUCCESS;
			}
			break;
		}
		case PCIeDMA_IOCTL_TRIG_INT:
		{
			if (devExt->MemBarBase){
				PcieDeviceTriggerInterrupt(devExt->MemBarBase);
				status = STATUS_SUCCESS;
			}
			break;
		}
		case PCIeDMA_IOCTL_WRITE_REG:
		{
			ULONG *ptr = (PULONG)in_buffer;
			ULONG address = ptr[0];
			ULONG size = ptr[1] / sizeof(ULONG);
			PULONG data = &ptr[2];
			ULONG i;

			if (devExt->MemBarBase){
				for (i = 0; i < size; i++){
					PcieDeviceWriteReg(devExt->MemBarBase, address + i*sizeof(ULONG), data[i]);
				}
				status = STATUS_SUCCESS;
			}
			break;
		}
		case PCIeDMA_IOCTL_READ_REG:
		{
			PULONG ptr = (PULONG)in_buffer;
			ULONG address = ptr[0];
			ULONG size = ptr[1] / sizeof(ULONG);
			PULONG ptrOut = (PULONG)out_buffer;
			ULONG i;

			if (devExt->MemBarBase){
				for (i = 0; i < size; i++){
					ptrOut[i] = PcieDeviceReadReg(devExt->MemBarBase, address + i*sizeof(ULONG));
				}
				ret_length = ptr[1];
				status = STATUS_SUCCESS;
			}
			break;
		}
		default:
			status = STATUS_INVALID_DEVICE_REQUEST;
			break;
	}

	WdfRequestCompleteWithInformation(Request, status, ret_length);

#ifdef DEBUG_HU
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "<-- %!FUNC!: Request %p", Request);
#endif
}