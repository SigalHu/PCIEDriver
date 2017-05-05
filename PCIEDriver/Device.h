/*++

Module Name:

    device.h

Abstract:

    This file contains the device definitions.

Environment:

    Kernel-mode Driver Framework

--*/

#include "public.h"
#include "RegPcie.h"
//
// The device context performs the same job as
// a WDM device extension in the driver frameworks
//
typedef struct _DEVICE_CONTEXT
{
	WDFDEVICE               Device;

	PUCHAR                  MemBarBase;      	// Registers base address (must be 32bit align)
	ULONG                   MemBarLength;    	// Registers base length
	PUCHAR                  PortBarBase;
	ULONG                   PortBarLength;

	WDFDMAENABLER           DmaEnabler;
	WDFCOMMONBUFFER         CommonBuffer;
	size_t                  CommonBufferSize;
	PUCHAR                  CommonBufferBase;
	PHYSICAL_ADDRESS        CommonBufferBaseLA;   // Logical Address

	WDFQUEUE                WriteQueue;
	WDFREQUEST				WriteRequest;
	ULONG					WriteDmaLength;
	WDFTIMER                WriteTimer;
	BOOLEAN                 WriteTimeout;

	WDFQUEUE                ReadQueue;
	WDFREQUEST				ReadRequest;
	ULONG					ReadDmaLength;
	PVOID					ReadBuffer;
	WDFTIMER                ReadTimer;
	BOOLEAN                 ReadTimeout;

	WDFQUEUE                IoDispatchQueue;

	WDFINTERRUPT            Interrupt;     	// Returned by InterruptCreate
	ULONG					IntStatus;

	union {
		DmaMode_t bits;
		ULONG   ulong;
	}                       DmaMode;

} DEVICE_CONTEXT, *PDEVICE_CONTEXT;

//
// This macro will generate an inline function called DeviceGetContext
// which will be used to get a pointer to the device context memory
// in a type safe manner.
//
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DEVICE_CONTEXT, PcieGetDeviceContext)

//
// WDFDRIVER Events
//

EVT_WDF_DRIVER_DEVICE_ADD PcieEvtDeviceAdd;
EVT_WDF_OBJECT_CONTEXT_CLEANUP PcieEvtDriverContextCleanup;

EVT_WDF_DEVICE_D0_ENTRY PcieEvtDeviceD0Entry;
EVT_WDF_DEVICE_D0_EXIT PcieEvtDeviceD0Exit;
EVT_WDF_DEVICE_PREPARE_HARDWARE PcieEvtDevicePrepareHardware;
EVT_WDF_DEVICE_RELEASE_HARDWARE PcieEvtDeviceReleaseHardware;

//
// Function to initialize the device and its callbacks
//
NTSTATUS
PcieSetIdleAndWakeSettings(
	_In_ PDEVICE_CONTEXT FdoData
	);

NTSTATUS
PcieInitializeDeviceContext(
	_In_ PDEVICE_CONTEXT DevExt
	);

NTSTATUS
PcieInitializeDMA(
	_In_ PDEVICE_CONTEXT DevExt
	);