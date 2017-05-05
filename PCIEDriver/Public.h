/*++

Module Name:

    public.h

Abstract:

    This module contains the common declarations shared by driver
    and user applications.

Environment:

    user and kernel

--*/

#include "initguid.h"

//
// Define an Interface Guid so that app can find the device and talk to it.
//

DEFINE_GUID(GUID_XILINX_PCI_INTERFACE,
    0x76c326d4,0xd26b,0x4c7e,0x91,0x53,0x70,0xef,0xfc,0xe2,0x2d,0x4f);
// {76c326d4-d26b-4c7e-9153-70effce22d4f}

//// {29D2A384-2E47-49b5-AEBF-6962C22BD7C2}
//DEFINE_GUID(GUID_XILINX_PCI_INTERFACE,
//	0x29d2a384, 0x2e47, 0x49b5, 0xae, 0xbf, 0x69, 0x62, 0xc2, 0x2b, 0xd7, 0xc2);

#define MAX_DMABUFFER_SIZE         (16*1024*1024)

#define PCIeDMA_IOCTL_GET_TIME				CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define PCIeDMA_IOCTL_GET_VERSION			CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define PCIeDMA_IOCTL_TRIG_INT				CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define PCIeDMA_IOCTL_WRITE_REG		    	CTL_CODE(FILE_DEVICE_UNKNOWN, 0x803, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define PCIeDMA_IOCTL_READ_REG  			CTL_CODE(FILE_DEVICE_UNKNOWN, 0x804, METHOD_BUFFERED, FILE_ANY_ACCESS)