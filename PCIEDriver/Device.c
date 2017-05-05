/*++

Module Name:

    device.c - Device handling events for example driver.

Abstract:

   This file contains the device entry points and callbacks.
    
Environment:

    Kernel-mode Driver Framework

--*/

#include "driver.h"
#include "device.tmh"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, PcieEvtDeviceAdd)
#pragma alloc_text (PAGE, PcieEvtDevicePrepareHardware)
#pragma alloc_text (PAGE, PcieEvtDeviceReleaseHardware)
#pragma alloc_text (PAGE, PcieEvtDeviceD0Exit)
#pragma alloc_text (PAGE, PcieEvtDriverContextCleanup)
#pragma alloc_text (PAGE, PcieSetIdleAndWakeSettings)
#pragma alloc_text (PAGE, PcieInitializeDeviceContext)
#pragma alloc_text (PAGE, PcieInitializeDMA)
#endif


NTSTATUS
PcieEvtDeviceAdd(
	_In_    WDFDRIVER       Driver,
	_Inout_ PWDFDEVICE_INIT DeviceInit
	)
/*++
Routine Description:

EvtDeviceAdd is called by the framework in response to AddDevice
call from the PnP manager. We create and initialize a device object to
represent a new instance of the device.

Arguments:

Driver - Handle to a framework driver object created in DriverEntry

DeviceInit - Pointer to a framework-allocated WDFDEVICE_INIT structure.

Return Value:

NTSTATUS

--*/
{
	NTSTATUS                status;
	WDF_OBJECT_ATTRIBUTES   attributes;
	PDEVICE_CONTEXT         devExt;
	WDFDEVICE               device;
	WDF_PNPPOWER_EVENT_CALLBACKS  pnpPowerCallbacks;

	UNREFERENCED_PARAMETER(Driver); // hu 未引用参数(防止warning)

	PAGED_CODE();  // hu PAGED_CODE表示该代码占用分页内存，如果不说明，则占用系统的非分页内存

#ifdef DEBUG_HU
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "--> %!FUNC!");
#endif

	WdfDeviceInitSetIoType(DeviceInit, WdfDeviceIoBuffered);

	//
	// Zero out the PnpPowerCallbacks structure.
	//
	WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpPowerCallbacks);

	//
	// Set Callbacks for any of the functions we are interested in.
	// If no callback is set, Framework will take the default action
	// by itself.
	//
	pnpPowerCallbacks.EvtDevicePrepareHardware = PcieEvtDevicePrepareHardware;  // hu 设备硬件初始化函数
	pnpPowerCallbacks.EvtDeviceReleaseHardware = PcieEvtDeviceReleaseHardware;

	//
	// These two callbacks set up and tear down hardware state that must be
	// done every time the device moves in and out of the D0-working state.
	//
	pnpPowerCallbacks.EvtDeviceD0Entry = PcieEvtDeviceD0Entry;  // hu 进入和退出D0状态时候的回调函数
	pnpPowerCallbacks.EvtDeviceD0Exit = PcieEvtDeviceD0Exit;   // hu 在此状态下，计算机在全功耗和全功能下运行

	//
	// Register the PnP Callbacks..
	//
	WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &pnpPowerCallbacks);

	//
	// Initialize Fdo Attributes.
	// hu 分配一个DEVICE_CONTEXT的内存块，并且将内存块的指针保存到WDF_OBJECT_ATTRIBUTES里面
	WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, DEVICE_CONTEXT);
	//
	// By opting for SynchronizationScopeDevice, we tell the framework to
	// synchronize callbacks events of all the objects directly associated
	// with the device. In this driver, we will associate queues and
	// and DpcForIsr. By doing that we don't have to worrry about synchronizing
	// access to device-context by Io Events and DpcForIsr because they would
	// not concurrently ever. Framework will serialize them by using an
	// internal device-lock.
	//
	attributes.SynchronizationScope = WdfSynchronizationScopeDevice;

	//
	// Create the device
	//
	status = WdfDeviceCreate(&DeviceInit, &attributes, &device);

	if (!NT_SUCCESS(status)) {
		//
		// Device Initialization failed.
		//
#ifdef DEBUG_HU
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER,
			"WdfDeviceCreate failed %!STATUS!", status);
#endif
		return status;
	}

	//
	// Get the DeviceExtension and initialize it. PcieGetDeviceContext is an inline function
	// defined by WDF_DECLARE_CONTEXT_TYPE_WITH_NAME macro in the
	// private header file. This function will do the type checking and return
	// the device context. If you pass a wrong object a wrong object handle
	// it will return NULL and assert if run under framework verifier mode.
	//
	devExt = PcieGetDeviceContext(device);
	devExt->Device = device;

	//
	// Tell the Framework that this device will need an interface
	//
	// NOTE: See the note in Public.h concerning this GUID value.
	//
	status = WdfDeviceCreateDeviceInterface(device,
		(LPGUID)&GUID_XILINX_PCI_INTERFACE,
		NULL);

	if (!NT_SUCCESS(status)) {
#ifdef DEBUG_HU
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER,
			"WdfDeviceCreateDeviceInterface failed %!STATUS!", status);
#endif
		return status;
	}

	//
	// Set the idle and wait-wake policy for this device.
	//
	status = PcieSetIdleAndWakeSettings(devExt);

	if (!NT_SUCCESS(status)) {
		//
		// NOTE: The attempt to set the Idle and Wake options
		//       is a best-effort try. Failure is probably due to
		//       the non-driver environmentals, such as the system,
		//       bus or OS indicating that Wake is not supported for
		//       this case.
		//       All that being said, it probably not desirable to
		//       return the failure code as it would cause the
		//       AddDevice to fail and Idle and Wake are probably not
		//       "must-have" options.
		//
		//       You must decide for your case whether Idle/Wake are
		//       "must-have" options...but my guess is probably not.
		//
#ifdef DEBUG_HU
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER,
			"PcieSetIdleAndWakeSettings failed %!STATUS!", status);
#endif

#if 1
		status = STATUS_SUCCESS;
#else
		return status;
#endif
	}

	//
	// Initalize the Device Extension.
	//
	status = PcieInitializeDeviceContext(devExt);

	if (!NT_SUCCESS(status)) {
#ifdef DEBUG_HU
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER,
			"PcieInitializeDeviceContext failed %!STATUS!", status);
#endif
		return status;
	}
	//status = pcieDriverCreateDevice(DeviceInit);

#ifdef DEBUG_HU
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "<-- %!FUNC!");
#endif
	return status;
}

NTSTATUS
PcieEvtDevicePrepareHardware(
	WDFDEVICE       Device,
	WDFCMRESLIST   Resources,
	WDFCMRESLIST   ResourcesTranslated
	)
/*++

Routine Description:

Performs whatever initialization is needed to setup the device, setting up
a DMA channel or mapping any I/O port resources.  This will only be called
as a device starts or restarts, not every time the device moves into the D0
state.  Consequently, most hardware initialization belongs elsewhere.

Arguments:

Device - A handle to the WDFDEVICE

Resources - The raw PnP resources associated with the device.  Most of the
time, these aren't useful for a PCI device.

ResourcesTranslated - The translated PnP resources associated with the
device.  This is what is important to a PCI device.

Return Value:

NT status code - failure will result in the device stack being torn down

--*/
{
	NTSTATUS         status = STATUS_SUCCESS;
	PDEVICE_CONTEXT   devExt;
	ULONG i;
	PCM_PARTIAL_RESOURCE_DESCRIPTOR  desc;

	UNREFERENCED_PARAMETER(Resources);

	PAGED_CODE();

#ifdef DEBUG_HU
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "--> %!FUNC!");
#endif

	devExt = PcieGetDeviceContext(Device);

	//
	// Parse the resource list and save the resource information.
	//
	for (i = 0; i < 1/*WdfCmResourceListGetCount(ResourcesTranslated)*/; i++) {

		desc = WdfCmResourceListGetDescriptor(ResourcesTranslated, i);

		if (!desc) {
			status = STATUS_DEVICE_CONFIGURATION_ERROR;
#ifdef DEBUG_HU
			TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER,
				"WdfCmResourceListGetDescriptor failed %!STATUS!", status);
#endif
			return status;
		}

		switch (desc->Type) {
			case CmResourceTypeMemory:
				//
				// hu 将物理地址映射到虚拟地址
				// 
				devExt->MemBarBase = (PUCHAR)MmMapIoSpace(desc->u.Memory.Start,
					desc->u.Memory.Length,
					MmNonCached);
				devExt->MemBarLength = desc->u.Memory.Length;

#ifdef DEBUG_HU
				TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER,
					" - Memory Resource [%I64X-%I64X] BAR%d",
					desc->u.Memory.Start.QuadPart,
					desc->u.Memory.Start.QuadPart +
					desc->u.Memory.Length,
					i);
#endif

				if (!devExt->MemBarBase) {
					status = STATUS_INSUFFICIENT_RESOURCES;
#ifdef DEBUG_HU
					TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER,
						"Unable to map Registers memory %08I64X, length %d",
						desc->u.Memory.Start.QuadPart,
						desc->u.Memory.Length);
#endif
					return status;
				}
				break;
			case CmResourceTypePort:
				devExt->PortBarBase = ULongToPtr(desc->u.Port.Start.LowPart);
				devExt->PortBarLength = desc->u.Port.Length;

#ifdef DEBUG_HU
				TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER,
					" - Port Resource [%I64X-%I64X] BAR%d",
					desc->u.Port.Start.QuadPart,
					desc->u.Port.Start.QuadPart +
					desc->u.Port.Length,
					i);
#endif

				if (!devExt->PortBarBase) {
					status = STATUS_INSUFFICIENT_RESOURCES;
#ifdef DEBUG_HU
					TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER,
						"Unable to map Registers Port %08I64X, length %d",
						desc->u.Port.Start.QuadPart,
						desc->u.Port.Length);
#endif
					return status;
				}
				break;
			default:
				break;
		}
	}

	if (!devExt->MemBarBase) {
		status = STATUS_INSUFFICIENT_RESOURCES;
#ifdef DEBUG_HU
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER,
			"PcieMapResources: Missing resources BAR0");
#endif
		return status;
	}

	//
	// Init and Reset hardware
	//
	PcieDeviceResetHardWare(devExt->MemBarBase);

#ifdef DEBUG_HU
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "<-- %!FUNC!");
#endif
	return status;
}

NTSTATUS
PcieEvtDeviceReleaseHardware(
	_In_  WDFDEVICE Device,
	_In_  WDFCMRESLIST ResourcesTranslated
	)
/*++

Routine Description:

Unmap the resources that were mapped in PcieEvtDevicePrepareHardware.
This will only be called when the device stopped for resource rebalance,
surprise-removed or query-removed.

Arguments:

Device - A handle to the WDFDEVICE

ResourcesTranslated - The translated PnP resources associated with the
device.  This is what is important to a PCI device.

Return Value:

NT status code - failure will result in the device stack being torn down

--*/
{
	NTSTATUS status = STATUS_SUCCESS;
	PDEVICE_CONTEXT   devExt;

	UNREFERENCED_PARAMETER(ResourcesTranslated);

	PAGED_CODE();

#ifdef DEBUG_HU
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "--> %!FUNC!");
#endif

	devExt = PcieGetDeviceContext(Device);

	if (devExt->MemBarBase) {
		MmUnmapIoSpace(devExt->MemBarBase, devExt->MemBarLength);
		devExt->MemBarBase = NULL;
		devExt->MemBarLength = 0;
	}

#ifdef DEBUG_HU
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "<-- %!FUNC!");
#endif
	return status;
}

NTSTATUS
PcieEvtDeviceD0Entry(
	_In_  WDFDEVICE Device,
	_In_  WDF_POWER_DEVICE_STATE PreviousState
	)
/*++

Routine Description:

This routine prepares the device for use.  It is called whenever the device
enters the D0 state, which happens when the device is started, when it is
restarted, and when it has been powered off.

Note that interrupts will not be enabled at the time that this is called.
They will be enabled after this callback completes.

This function is not marked pageable because this function is in the
device power up path. When a function is marked pagable and the code
section is paged out, it will generate a page fault which could impact
the fast resume behavior because the client driver will have to wait
until the system drivers can service this page fault.

Arguments:

Device  - The handle to the WDF device object

PreviousState - The state the device was in before this callback was invoked.

Return Value:

NTSTATUS

Success implies that the device can be used.

Failure will result in the    device stack being torn down.

--*/
{
	NTSTATUS  status = STATUS_SUCCESS;

	UNREFERENCED_PARAMETER(Device);
	UNREFERENCED_PARAMETER(PreviousState);

#ifdef DEBUG_HU
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "--> %!FUNC!");
#endif



#ifdef DEBUG_HU
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "<-- %!FUNC!");
#endif
	return status;
}

NTSTATUS
PcieEvtDeviceD0Exit(
	_In_  WDFDEVICE Device,
	_In_  WDF_POWER_DEVICE_STATE TargetState
	)
/*++

Routine Description:

This routine undoes anything done in PcieEvtDeviceD0Entry.  It is called
whenever the device leaves the D0 state, which happens when the device
is stopped, when it is removed, and when it is powered off.

The device is still in D0 when this callback is invoked, which means that
the driver can still touch hardware in this routine.

Note that interrupts have already been disabled by the time that this
callback is invoked.

Arguments:

Device  - The handle to the WDF device object

TargetState - The state the device will go to when this callback completes.

Return Value:

Success implies that the device can be used.  Failure will result in the
device stack being torn down.

--*/
{
	NTSTATUS  status = STATUS_SUCCESS;
	PDEVICE_CONTEXT   devExt;

	PAGED_CODE();

#ifdef DEBUG_HU
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "--> %!FUNC!");
#endif

	devExt = PcieGetDeviceContext(Device);

	switch (TargetState) {
		case WdfPowerDeviceD1:
		case WdfPowerDeviceD2:
		case WdfPowerDeviceD3:

			//
			// Fill in any code to save hardware state here.
			//

			//
			// Fill in any code to put the device in a low-power state here.
			//
			break;

		case WdfPowerDevicePrepareForHibernation:

			//
			// Fill in any code to save hardware state here.  Do not put in any
			// code to shut the device off.  If this device cannot support being
			// in the paging path (or being a parent or grandparent of a paging
			// path device) then this whole case can be deleted.
			//

			break;

		case WdfPowerDeviceD3Final:
		default:
			//
			// Reset the hardware, as we're shutting down for the last time.
			//
			if (devExt->MemBarBase){
				PcieDeviceResetDMA(devExt->MemBarBase);
			}
			break;
	}

#ifdef DEBUG_HU
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "<-- %!FUNC!");
#endif
	return status;
}

VOID
PcieEvtDriverContextCleanup(
	_In_ WDFOBJECT DriverObject
	)
/*++
Routine Description:

Free all the resources allocated in DriverEntry.

Arguments:

DriverObject - handle to a WDF Driver object.

Return Value:

VOID.

--*/
{
	PAGED_CODE();

#ifdef DEBUG_HU
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "--> %!FUNC!");
	//
	// Stop WPP Tracing
	//
	WPP_CLEANUP(WdfDriverWdmGetDriverObject(DriverObject));
#else
	UNREFERENCED_PARAMETER(DriverObject);
#endif
}

NTSTATUS
PcieSetIdleAndWakeSettings(
	_In_ PDEVICE_CONTEXT FdoData
	)
/*++
Routine Description:

Called by EvtDeviceAdd to set the idle and wait-wake policy. Registering this policy
causes Power Management Tab to show up in the device manager. By default these
options are enabled and the user is provided control to change the settings.

Return Value:

NTSTATUS - Failure status is returned if the device is not capable of suspending
or wait-waking the machine by an external event. Framework checks the
capability information reported by the bus driver to decide whether the device is
capable of waking the machine.

--*/
{
	NTSTATUS    status = STATUS_SUCCESS;
	WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS idleSettings;
	WDF_DEVICE_POWER_POLICY_WAKE_SETTINGS wakeSettings;

	PAGED_CODE();

#ifdef DEBUG_HU
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "--> %!FUNC!");
#endif

	//
	// Init the idle policy structure.
	// hu 设置设备为闲时休眠.闲时超过 10S,自动进入休眠状态;并支持自唤醒
	WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS_INIT(&idleSettings, IdleCanWakeFromS0);
	idleSettings.IdleTimeout = 10000; // 10-sec

	status = WdfDeviceAssignS0IdleSettings(FdoData->Device, &idleSettings);
	if (!NT_SUCCESS(status)) {
#ifdef DEBUG_HU
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER,
			"WdfDeviceAssignS0IdleSettings failed %!STATUS!", status);
#endif
		return status;
	}

	//
	// Init wait-wake policy structure.
	//
	WDF_DEVICE_POWER_POLICY_WAKE_SETTINGS_INIT(&wakeSettings);

	status = WdfDeviceAssignSxWakeSettings(FdoData->Device, &wakeSettings);
	if (!NT_SUCCESS(status)) {
#ifdef DEBUG_HU
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER,
			"WdfDeviceAssignSxWakeSettings failed %!STATUS!", status);
#endif
		return status;
	}

#ifdef DEBUG_HU
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "<-- %!FUNC!");
#endif
	return status;
}

NTSTATUS
PcieInitializeDeviceContext(
	_In_ PDEVICE_CONTEXT DevExt
	)
/*++
Routine Description:

This routine is called by EvtDeviceAdd. Here the device context is
initialized and all the software resources required by the device is
allocated.

Arguments:

DevExt     Pointer to the Device Extension

Return Value:

NTSTATUS

--*/
{
	NTSTATUS    status = STATUS_SUCCESS;

	PAGED_CODE();

#ifdef DEBUG_HU
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "--> %!FUNC!");
#endif

	status = PcieQueueInitialize(DevExt);

	if (!NT_SUCCESS(status)) {
#ifdef DEBUG_HU
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER,
			"PcieQueueInitialize failed: %!STATUS!", status);
#endif
		return status;
	}

	//
	// Create a WDFINTERRUPT object.
	//
	status = PcieInterruptCreate(DevExt);
	if (!NT_SUCCESS(status)) {
#ifdef DEBUG_HU
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER,
			"PcieInterruptCreate failed: %!STATUS!", status);
#endif
		return status;
	}

	// 
	// Init DMA hardware
	//
	status = PcieInitializeDMA(DevExt);
	if (!NT_SUCCESS(status)) {
#ifdef DEBUG_HU
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER,
			"PcieInitializeDMA failed: %!STATUS!", status);
#endif
		return status;
	}

	//
	// Init Timers
	//
	status = PcieDMATimerCreate(
		&DevExt->WriteTimer,
		DevExt->Device,
		DmaWriteTimerEventFunc);

	if (!NT_SUCCESS(status)) {
#ifdef DEBUG_HU
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER,
			"PcieDMATimerCreate failed: %!STATUS!", status);
#endif
		return status;
	}

	status = PcieDMATimerCreate(
		&DevExt->ReadTimer,
		DevExt->Device,
		DmaReadTimerEventFunc);

	if (!NT_SUCCESS(status)) {
#ifdef DEBUG_HU
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER,
			"PcieDMATimerCreate failed: %!STATUS!", status);
#endif
		return status;
	}

#ifdef DEBUG_HU
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "<-- %!FUNC!");
#endif
	return status;
}

NTSTATUS
PcieInitializeDMA(
	_In_ PDEVICE_CONTEXT DevExt
	)
/*++
Routine Description:

Initializes the DMA adapter.

Arguments:

DevExt      Pointer to our DEVICE_EXTENSION

Return Value:

None

--*/
{
	NTSTATUS  status = STATUS_SUCCESS;
	WDF_DMA_ENABLER_CONFIG   	dmaConfig;

	PAGED_CODE();

#ifdef DEBUG_HU
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "--> %!FUNC!");
#endif

	//
	// DMA_TRANSFER_ELEMENTS must be 128-byte aligned
	//
	WdfDeviceSetAlignmentRequirement(DevExt->Device,
		/*PCI_DTE_ALIGNMENT_16 ); */
		PCI_DTE_ALIGNMENT_128);

	//
	// Create a new DMA Enabler instance.
	// Use Scatter/Gather, 32-bit Addresses, Duplex-type profile.
	//
	WDF_DMA_ENABLER_CONFIG_INIT(&dmaConfig,
#ifdef SUPPORT_DMA64
		WdfDmaProfileScatterGather64Duplex,
#else
		WdfDmaProfileScatterGatherDuplex,
#endif
		MAX_DMA_SIZE_COMMONBUFFER);

#ifdef DEBUG_HU
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER,
		"The DMA Profile is WdfDmaProfileScatterGatherDuplex");
#endif

	status = WdfDmaEnablerCreate(DevExt->Device,
		&dmaConfig,
		WDF_NO_OBJECT_ATTRIBUTES,
		&DevExt->DmaEnabler);

	if (!NT_SUCCESS(status)) {
#ifdef DEBUG_HU
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER,
			"WdfDmaEnablerCreate failed: %!STATUS!", status);
#endif
		return status;
	}

	//
	// Allocate common buffer
	//
	// NOTE: This common buffer will not be cached.
	//       Perhaps in some future revision, cached option could
	//       be used. This would have faster access, but requires
	//       flushing before starting the DMA in PcieStartReadDma.
	//
	DevExt->CommonBufferSize = MAX_DMA_SIZE_COMMONBUFFER;

	status = WdfCommonBufferCreate(DevExt->DmaEnabler,
		DevExt->CommonBufferSize,
		WDF_NO_OBJECT_ATTRIBUTES,
		&DevExt->CommonBuffer);

	if (!NT_SUCCESS(status)) {
#ifdef DEBUG_HU
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER,
			"WdfCommonBufferCreate failed %!STATUS!", status);
#endif
		return status;
	}

	DevExt->CommonBufferBase =
		WdfCommonBufferGetAlignedVirtualAddress(DevExt->CommonBuffer);

	DevExt->CommonBufferBaseLA =
		WdfCommonBufferGetAlignedLogicalAddress(DevExt->CommonBuffer);

	RtlZeroMemory(DevExt->CommonBufferBase,
		DevExt->CommonBufferSize);

#ifdef DEBUG_HU
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER,
		"CommonBuffer  0x%p  (#0x%I64X), length %I64d",
		DevExt->CommonBufferBase,
		DevExt->CommonBufferBaseLA.QuadPart,
		WdfCommonBufferGetLength(DevExt->CommonBuffer));
#endif

#ifdef DEBUG_HU
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "<-- %!FUNC!");
#endif
	return status;
}
