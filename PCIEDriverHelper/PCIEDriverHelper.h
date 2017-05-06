// PCIEDriverHelper.h

#pragma once

#include "Public.h"
#include "PCIEDriver.h"

using namespace System;
using namespace System::Runtime::InteropServices;

namespace PCIEDriverHelper {

	public ref class PcieDriver
	{
		// TODO: Add your methods for this class here.
	public:
		literal unsigned int MAX_BUF_SIZE = MAX_DMABUFFER_SIZE;
		
		static String^ GetLastDeviceError();

		static bool OpenPcieDevice();

		static void ClosePcieDevice();

		static bool DmaToDevice(array<byte> ^inBuf);

		static bool DmaFromDevice(array<byte> ^inBuf);

		static bool SetDeviceRegister(unsigned int devRegAddr,array<unsigned int> ^regData);

		static bool SetDeviceRegister(unsigned int devRegAddr,unsigned int regData);
	};
}
