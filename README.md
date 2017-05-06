# PCIEDriver
基于WDK的PCIE驱动程序及测试软件
## 开发工具
* VS2013
* WDK7600
## 介绍
* PCIEDriver工程：PCIE驱动程序，实现了中断、寄存器读写与DMA传输等功能
* PCIEDriverHelper工程：使用C++/CLI对驱动相关功能的调用细节进行封装，生成dll，用户通过接口函数对PCIE设备进行操作
* PCIEDriverTest工程：PCIE驱动测试软件，对PCIEDriver工程生成的PCIE驱动进行测试
## 实现功能
支持中断、寄存器读写与DMA传输等操作
