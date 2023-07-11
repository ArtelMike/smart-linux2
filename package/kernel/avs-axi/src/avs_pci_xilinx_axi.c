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
// Name           : pci_xilinx_axi.c
// Author         : William Anderson
// Version        :
// Description : Xilinx PCI to AXI interface - Low level PCIe IF

//============================================================================


#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/pci.h>
#include <linux/init.h>
#include "xilinxAxiDrv.h"

/*
 * From linux driver developmwnt example (3rd edition )
*/

static struct pci_device_id xilinx_axi_ids[] = {
    { PCI_DEVICE( PCI_VENDOR_ID_XILINX, PCI_DEVICE_ID_XILINX_AXI_BRIDGE ), },
    { 0, }
};

static int pci_xilinx_axi_probe(struct pci_dev *dev, const struct pci_device_id *id)
{
    void __iomem* bar0;
    unsigned long bar0_len;
    void __iomem* bar2;
    unsigned long bar2_len;
#ifdef READY4MORE
    void __iomem* bar4;
    unsigned long bar4_len;
#endif

    xilinx_axi_privData_t *privdata;
    privdata = kzalloc( sizeof(xilinx_axi_privData_t), GFP_KERNEL);
    if(!privdata){ 
        printk(KERN_ERR "pci_xilinx_axi_drv failed to allocate privdata memory.\n");
        return -ENOMEM;
    }

    if(pci_enable_device(dev)) {
        dev_err(&dev->dev, "can't enable PCI device\n");
         printk(KERN_ERR "pci_xilinx_axi_drv can't enable PCI device\n");
       return -ENODEV;
    }
     pci_set_drvdata(dev, privdata);

    // Check that BAR 0 is defined and memory mapped.
    if(!(pci_resource_flags(dev, 0) & IORESOURCE_MEM)){
        printk(KERN_ERR "pci_xilinx_axi_drv incorrect BAR 0 configuration.\n");
        return -ENODEV;
    }
         
    if(!(pci_resource_flags(dev, 2) & IORESOURCE_MEM)){
        printk(KERN_ERR "pci_xilinx_axi_drv no BAR 2 configuration.\n");
        //return -ENODEV;  // may not be present since this is a debug area
    }
         
#ifdef READY4MORE
    if(!(pci_resource_flags(dev, 4) & IORESOURCE_MEM)){
        printk(KERN_ERR "pci_xilinx_axi_drv no BAR 3 configuration.\n");
        //return -ENODEV;
    }
#endif         

    if(pci_request_regions(dev, "xilinc_axi_pci_drv")){
        printk(KERN_ERR "pci_xilinx_axi_drv can't request PCI region(s)\n");
        dev_err(&dev->dev, "can't request PCI region(s)\n");
        return -ENODEV;
    }

    bar0_len = pci_resource_len (dev, 0);
    bar0 = pci_iomap (dev, 0, bar0_len); // or map the entire BAR 0 by setting  len=0
    if(!bar0 || !bar0_len){
        printk(KERN_ERR "pci_xilinx_axi_drv failed to map BAR 0.\n");
        return -ENODEV;
    }

    privdata->register0 = bar0;
    printk(KERN_INFO "Xilinx_axi_pci BAR 0 io_map %ld bytes.\n", bar0_len);
    printk(KERN_INFO "Xilinx_axi_pci privdata->register0 BAR 0 %p .\n", bar0);
    

    bar2_len = pci_resource_len (dev, 2);
    bar2 = pci_iomap ( dev, 2, bar2_len); // map the entire BAR 2 by setting  len=0
    if(!bar2){
        printk(KERN_ERR "pci_xilinx_axi_drv failed to map BAR 2.\n");
        //return -ENODEV;
    }
    privdata->register2 = bar2;
    printk(KERN_INFO "Xilinx_axi_pci BAR 2 io_map %ld bytes.\n", bar2_len);
    printk(KERN_INFO "Xilinx_axi_pci privdata->register2 BAR 0 %p .\n", bar0);

#ifdef READY4MORE

    bar4 = pci_iomap ( dev, 4, 0); // map the entire BAR 4 by setting  len=0
    if(!bar4){
        printk(KERN_ERR "pci_xilinx_axi_drv failed to map BAR 4.\n");
        return -ENODEV;
    }
    printk(KERN_INFO "Xilinx_axi_pci BAR 4 0x%p\n", bar4);
#endif

#if 0
{
    // DUMP first 256 words 
    int i;
    uint32_t *data_p = privdata->register0;
    printk(KERN_INFO "pci_xilinx_axi Bar0 0x%p\n", privdata->register0);
    for( i=0; i<0xFF; i++){
        
        printk(KERN_INFO "pci_xilinx_axi Bar0 0x%p 0x%x = 0x%x \n", (void*) data_p, i, ioread32(data_p++));
        //printk(KERN_INFO "pci_xilinx_axi Bar0 0x%p 0x%x = 0x%x \n", privdata->register0[i], i, ioread32(&privdata->register0[i]));
    }
}
#endif

//    maybe only needed by root complex.  Not need by Xilinx_axi yet?
//    pci_set_master(dev);

#if 0
    if(pci_enable_msi(dev) ){
        printk(KERN_ERR "pci_xilinx_axi_drv failed to enable MSI.\n");
        return -ENODEV;
    }
#endif

#ifdef USINGDMA
    if(!pci_set_dma_mask(dev, DMA_BIT_MASK(64))){
        printk(KERN_INFO "pci_xilinx_axi set DMA MASK 64\n");
        privdata->dma_using_dac = 1;
    } else if(!pci_set_dma_mask(dev, DMA_BIT_MASK(32))){
        printk(KERN_INFO "Xilinx_axi_pci set DMA MASK 32\n");
        privdata->dma_using_dac = 0;
    } else {
        printk(KERN_ERR "Xilinx_axi_pci Failed to set DMA MASK\n");
        return -ENODEV;
    }
#endif

    return 0;
}

static void remove(struct pci_dev *dev)
{
    /* clean up any allocated resources and stuff here.
     * like call release_region();
     */
}

MODULE_DEVICE_TABLE(pci, xilinx_axi_ids);

static struct pci_driver avs_pci_driver = {
    .name = "pci_xilinx_axi",
    .id_table = xilinx_axi_ids,
    .probe = pci_xilinx_axi_probe,
    .remove = remove
};

static int __init pci_xilinx_axi_init(void)
{
    int ret = 0;
    struct pci_dev *dev = NULL;
    ret = pci_register_driver(&avs_pci_driver);
    dev = pci_get_device(PCI_VENDOR_ID_XILINX, PCI_DEVICE_ID_XILINX_AXI_BRIDGE, dev);
    if (dev)
    {
	printk(KERN_INFO "pci_xilinx_axi init called and device found, ret = %d\n", ret);
    }
    else
    {
	printk(KERN_ERR "pci_xilinx_axi init called and DEVICE NOT FOUND, ret = %d\n", ret);
    }
    return ret;
}

static void __exit pci_xilinx_axi_exit(void)
{
    pci_unregister_driver(&avs_pci_driver);
}

MODULE_LICENSE("GPL");

module_init(pci_xilinx_axi_init);
module_exit(pci_xilinx_axi_exit);
