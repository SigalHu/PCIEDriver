/*++

Module Name:

    queue.h

Abstract:

    This file contains the queue definitions.

Environment:

    Kernel-mode Driver Framework

--*/

//
// This is the context that can be placed per queue
// and would contain per queue information.
//
typedef struct _QUEUE_CONTEXT {

    ULONG PrivateDeviceData;  // just a placeholder

} QUEUE_CONTEXT, *PQUEUE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(QUEUE_CONTEXT, QueueGetContext)

NTSTATUS
PcieQueueInitialize(
	_In_ PDEVICE_CONTEXT DevExt
    );

//
// Events from the IoQueue object
//
EVT_WDF_IO_QUEUE_IO_READ PcieEvtIoRead;
EVT_WDF_IO_QUEUE_IO_WRITE PcieEvtIoWrite;
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL PcieEvtIoDeviceControl;

