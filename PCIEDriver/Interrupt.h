
EVT_WDF_INTERRUPT_ISR PcieEvtInterruptIsr;
EVT_WDF_INTERRUPT_DPC PcieEvtInterruptDpc;
EVT_WDF_INTERRUPT_ENABLE PcieEvtInterruptEnable;
EVT_WDF_INTERRUPT_DISABLE PcieEvtInterruptDisable;

NTSTATUS
PcieInterruptCreate(
	_In_ PDEVICE_CONTEXT DevExt
	);