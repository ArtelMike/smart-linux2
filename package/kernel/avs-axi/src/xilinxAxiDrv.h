/*
************************************************************************
* Artel Video Systems, Inc. CONFIDENTIAL AND PROPRIETARY
*
* THIS WORK CONTAINS VALUABLE, CONFIDENTIAL AND PROPRIETARY
* INFORMATION.  DISCLOSURE, USE OR REPRODUCTION WITHOUT THE WRITTEN
* AUTHORIZATION OF ARTEL VIDEO SYSTEMS IS PROHIBITED.  THIS UNPUBLISHED
* WORK BY ARTEL VIDEO SYSTEMS IS PROTECTED BY THE LAWS OF THE UNITED STATES
* AND OTHER COUNTRIES.  IF PUBLICATION OF THE WORK SHOULD OCCUR THE FOLLOWING
* NOTICE SHALL APPLY:
*
* Copyright (c) 2016 Artel Video Systems
*
* Artel Video Systems, Inc., ALL RIGHTS RESERVED."
************************************************************************
*/

//============================================================================
// Name           : xilinxAxiDrv.h
// Author         : William Anderson
// Version        :
// Description : Xilinx PCI to AXI interface

//============================================================================

 

 
#ifndef _XILINXAXI_H_
#define _XILINXAXI_H_

#define PCI_VENDOR_ID_XILINX_AXI_BRIDGE 0x10EE
#define PCI_DEVICE_ID_XILINX_AXI_BRIDGE 0x8011

#define XILINXAXIMAJOR 2022  /* hardwire Major Device number */

#define AXI_BARO_MAP_OFFSET   0x20000000
//#define AXI_BARO_MAP_OFFSET 0x70E00000
//#define AXI_BARO_MAP_OFFSET 0x44A00000
#define AXI_BAR0_SIZE 0x100000 // 1M window
//#define AXI_BAR0_SIZE 0x40000 // 256K window
//#define AXI_BAR0_SIZE 0x1000 // 4K window
#define MIN_BAR0_AXI_ADDR AXI_BARO_MAP_OFFSET
#define MAX_BAR0_AXI_ADDR (AXI_BARO_MAP_OFFSET + AXI_BAR0_SIZE)   // 64MB window

#define AXI_BAR2_MAP_OFFSET 0xC0000000
#define AXI_BAR2_SIZE 0x2000 // 8K window
#define MIN_BAR2_AXI_ADDR AXI_BAR2_MAP_OFFSET
#define MAX_BAR2_AXI_ADDR (AXI_BAR2_MAP_OFFSET + AXI_BAR2_SIZE)   // 64MB window

#define MAX_NBR_AXI_CHAR_DEVICES 2

#include <linux/semaphore.h>


/* this gets allocated and initialized by driver probe routine */
typedef struct xilinx_axi_privData {
    uint32_t *register0;      /* physical address for start of BAR 0 */
    uint32_t *register2;      /* physical address for start of BAR 2 */
    uint32_t *register4;      /* physical address for start of BAR 4 */
} xilinx_axi_privData_t;

/* a list of xilinxAxi PCI devices scanned in system (should only be one for now) */
typedef struct xilinxAxi_dev {
    struct semaphore sem;         /* mutual exclusion semaphore */
    dev_t             dev;                   /*  Device numbers */
    struct cdev      *cdev;       /* Char device structure */
    struct pci_dev   *pcidev;     /* pci  device structure */
    struct xilinx_axi_privData * prvdata;  
} xilinxAxi_dev_t;



#endif /* _XILINXAXI_H_ */
