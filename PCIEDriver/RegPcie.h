#ifndef __REGPCIE_H_
#define __REGPCIE_H_

#define MAX_DMA_SIZE_COMMONBUFFER	MAX_DMABUFFER_SIZE
// hu PCI设备物理地址
#define ONBOARD_MEM_OFFSET			(0x10000000)
#define ITERN_DESC_ADDR				(0x1000)
// hu 寄存器偏移地址
#define REG_DMA_OFFSET              (0x0000)
#define REG_EXT1_OFFSET				(0x0E00)
#define ITERN_DESC_OFFSET			(0x1000)
#define MAILBOX_OFFSET				(0x2000)
#define REG_GPIO_OFFSET				(0x4000)

//-----------------------------------------------------------------------------   
// The DMA_TRANSFER_ELEMENTS
//-----------------------------------------------------------------------------   
#define PCI_DTE_ALIGNMENT_128		 FILE_128_BYTE_ALIGNMENT

//-----------------------------------------------------------------------------   
// DMA Transfer Element (DTE)
//-----------------------------------------------------------------------------   
#define DIRECTION_FROM_DEVICE        1
#define DIRECTION_TO_DEVICE          0

typedef struct _DESCPARAM_ {
	unsigned int 	DmaSize : 30;  	// bit 0-29
	unsigned int	DmaTDir : 1;   	// bit 30
	unsigned int	DmaDVal : 1;   	// bit 31
} DescParam_t;

typedef union {
	DescParam_t   	bits;
	unsigned int	ulong;
} dte_field3;

typedef struct _DMA_TRANSFER_ELEMENT {
#ifdef SUPPORT_DMA64
	volatile unsigned int	DevAddressHigh;
#endif
	volatile unsigned int	DevAddressLow;
#ifdef SUPPORT_DMA64
	volatile unsigned int	PciAddressHigh;
#endif	
	volatile unsigned int	PciAddressLow;
	volatile unsigned int	Param;
	volatile unsigned int	NextElement;
} DMA_TRANSFER_ELEMENT, *PDMA_TRANSFER_ELEMENT;

typedef struct _DMAMODE_ {
	unsigned int  	RdWr : 1;  // bit 0 (0-read 1-write)
	unsigned int	Reserved : 31; // bit 1-31	
} DmaMode_t;

//-----------------------------------------------------------------------------   
// Define the DMA Descriptor Source Address Register (DMADSA)
//-----------------------------------------------------------------------------   
typedef struct _DMADSA_ {
	unsigned int  	Bit0 : 1;  // bit 0
	unsigned int 	Bit1 : 1;  // bit 1
	unsigned int	Reserved : 30; // bit 2-31	
} DmaDSa_t;

typedef union {
	DmaDSa_t   		bits;
	unsigned int	ulong;
} DmaDSa_u_t;

//-----------------------------------------------------------------------------   
// Define the DMA Control Register (DMACTL)
//-----------------------------------------------------------------------------   
typedef struct _DMACTL_ {
	unsigned int  	DmaEna : 1;  // bit 0 	- '1' enable
	unsigned int 	DmaRst : 1;  // bit 1	- '1' reset
	unsigned int	Reserved : 30; // bit 2-31	
} DmaCtl_t;

typedef union {
	DmaCtl_t   		bits;
	unsigned int	ulong;
} DmaCtl_u_t;

//-----------------------------------------------------------------------------   
// Define the DMA Status Register (DMASTA)
//-----------------------------------------------------------------------------   
typedef struct _DMASTA_ {
	unsigned int  	Res0 : 1;  // bit 0
	unsigned int  	Res1 : 1;  // bit 1
	unsigned int  	DmaEnd : 1;  // bit 2
	unsigned int  	Res3 : 1;  // bit 3
	unsigned int  	DmaDesc : 1;  // bit 4
	unsigned int  	TrigInt : 1;  // bit 5	
	unsigned int	Reserved : 26; // bit 6-31	
} DmaSta_t;


typedef union {
	DmaSta_t   		bits;
	unsigned int	ulong;
} DmaSta_u_t;

//-----------------------------------------------------------------------------   
// Define the DMA Interrupt Mask Register (DMAINT)
//-----------------------------------------------------------------------------   
typedef struct _DMAINT_ {
	unsigned int  	Res0 : 1;  // bit 0
	unsigned int  	Res1 : 1;  // bit 1
	unsigned int  	DmaEnd : 1;  // bit 2
	unsigned int  	Res3 : 1;  // bit 3
	unsigned int  	DmaDesc : 1;  // bit 4
	unsigned int  	TrigInt : 1;  // bit 5
	unsigned int	Reserved : 26; // bit 6-31	
} DmaInt_t;

typedef union {
	DmaInt_t   		bits;
	unsigned int	ulong;
} DmaInt_u_t;

#define INT_MASK_ENABLE			0x4 		// enable DmaEnd only by default
#define INT_MASK_DISABLE_ALL	0x0			// disable all interrupts
#define INT_FLAG_FPGA_TRIG		(1<<5)		// interrupt triggered by user FPGA
#define INT_FLAG_DMA_END		(1<<2)		// interrupt by DMA end

//-----------------------------------------------------------------------------   
// DMA_REGS structure
//-----------------------------------------------------------------------------   
typedef struct _DMA_REGS_ {
	volatile unsigned int  	DmaDSa;  		// 0x00 
	volatile unsigned int  	DmaCtl;  		// 0x04 
	volatile unsigned int   DmaSta;  		// 0x08 
	volatile unsigned int  	DmaInt; 		// 0x0c
	volatile unsigned int 	Timer0;		// 0x10 - start-end per transfer
	volatile unsigned int 	dummy0;		// 0x14 - read timer / descriptor
	volatile unsigned int 	dummy1;		// 0x18 - write timer / descriptor
	volatile unsigned int 	dummy2;		// 0x1c
	volatile unsigned int 	reserve0;		// 0x20 - don't use
	volatile unsigned int 	ToDevAttr;		// 0x24
	volatile unsigned int 	FromDevAttr;	// 0x28
	volatile unsigned int 	reg1;			// 0x2c - don't use (debug)
	volatile unsigned int 	reg2;			// 0x30 - don't use (debug)
	volatile unsigned int 	reg3;			// 0x34 - don't use (debug)
	volatile unsigned int 	reg4;			// 0x38 - don't use (debug)
	volatile unsigned int 	reg5;			// 0x3c - don't use (debug)
	volatile unsigned int 	ProjCode;		// 0x40
	volatile unsigned int 	ProjVer;		// 0x44

} DMA_REGS, *PDMA_REGS;

typedef DMA_REGS PCIE_DMA_REGS;

#define HOST_ACCESS_OK		0x01
#define HOST_ACCESS_NOK		0
#define OPMODE_NORMAL		0
#define OPMODE_SIUI			0x01

typedef struct _DMA_EXT1_REGS_ {
	volatile unsigned int  	hostAccess; 	// 0x0E00 
	volatile unsigned int  	opMode;  		// 0x0E04 
} DMA_EXT1_REGS, *PDMA_EXT1_REGS;

typedef DMA_EXT1_REGS PCIE_DMA_EXT1_REGS;

typedef struct _MAILBOX_REGS_ {
	volatile unsigned int  	res0;	  		// 0x00
	volatile unsigned int	res1;			// 0x04
	volatile unsigned int	res2;			// 0x08
	volatile unsigned int	res3;			// 0x0c
	volatile unsigned int	trig_int;		// 0x10

} MAILBOX_REGS, *PMAILBOX_REGS;

typedef MAILBOX_REGS PCIE_MAILBOX_REGS;

#define TRIGGER_INT_CMD		0xffffffff

// hu ////////////////////////////////////////////////
VOID
PcieDeviceResetHardWare(
	_In_ PUCHAR BarXBase
	);

VOID
PcieDeviceResetDMA(
	_In_ PUCHAR BarXBase
	);

ULONG
PcieDeviceGetInterrupt(
	_In_ PUCHAR BarXBase
	);

VOID
PcieDeviceClearInterrupt(
	_In_ PUCHAR BarXBase
	);

VOID
PcieDeviceEnableInterrupt(
	_In_ PUCHAR BarXBase
	);

VOID
PcieDeviceDisableInterrupt(
	_In_ PUCHAR BarXBase
	);

VOID
PcieDeviceSetupDMA(
	_In_ PUCHAR BarXBase,
	_In_ WDFINTERRUPT interrupt,
	_In_ PHYSICAL_ADDRESS hostAddress,
	_In_ ULONG size,
	_In_ ULONG direction,
	_In_ BOOLEAN descLoc	// Descriptor location : 0 - external 1 - internal
	);

NTSTATUS
PcieDeviceStartDMA(
	_In_ PUCHAR BarXBase,
	_In_ WDFINTERRUPT interrupt
	);

ULONG
PcieDeviceGetDMATime(
	_In_ PUCHAR BarXBase
	);

ULONG64
PcieDeviceGetVersion(
	_In_ PUCHAR BarXBase
	);

ULONG
PcieDeviceReadReg(
	_In_ PUCHAR BarXBase,
	_In_ ULONG Address
	);

VOID
PcieDeviceWriteReg(
	_In_ PUCHAR BarXBase,
	_In_ ULONG Address,
	_In_ ULONG Data
	);

VOID
PcieDeviceTriggerInterrupt(
	_In_ PUCHAR BarXBase
	);
//////////////////////////////////////////////////////
#endif  // __REGPCIE_H_