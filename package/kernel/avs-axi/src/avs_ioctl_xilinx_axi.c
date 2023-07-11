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
// Name           : ioctl_xilinx_axi.c
// Author         : William Anderson
// Version        :
// Description : Xilinx PCI to AXI interface - IOCTL interface

//============================================================================



/* 
 ***********************************************************************
 *
 * xilinxAxiDrv
 ************************************************************************
 */

#include "xilinxAxiDrv.h"

#include <linux/module.h>
#include <linux/moduleparam.h>

#include <linux/kernel.h>       /* printk() */
#include <linux/slab.h>         /* kmalloc() */
#include <linux/fs.h>           /* everything... */
#include <linux/errno.h>        /* error codes */
#include <linux/types.h>        /* size_t */
#include <linux/fcntl.h>        /* O_ACCMODE */
#include <linux/seq_file.h>
#include <linux/cdev.h>
#include <linux/pci.h>
#include <linux/kdev_t.h>

#include <linux/semaphore.h>
#include <linux/uaccess.h>



static int   xilinxAxi_major = XILINXAXIMAJOR;
static int   xilinxAxi_minor = 0;
static int   xilinxAxi_nr_devs = 1;  // may add more later
static int   founddevs;
static int   Major;
static char  *driverName = "xilinxAxiDrv"; 
static int   xilinxAxi_nr_bars = 3;  // 1 used for production code other 2 used as debug only.

//module_param(xilinxAxi_major, int, S_IRUGO);
//module_param(xilinxAxi_minor, int, S_IRUGO);
//module_param(xilinxAxi_ int, S_IRUGO);

MODULE_AUTHOR( "Bill Anderson" );
MODULE_LICENSE( "GPL" );

struct xilinxAxi_dev *xilinxAxi_devices;        /* allocated in xilinxAxi_init_module */


static int xilinxAxi_find_devices( void )
{
    struct pci_dev  *pcidev = NULL;
    xilinx_axi_privData_t *privdata;
    founddevs = 0;
    do {
        //printk( KERN_INFO "pci_get_device( 0x%X 0x%X )\n", PCI_VENDOR_ID_XILINX_AXI_BRIDGE, PCI_DEVICE_ID_XILINX_AXI_BRIDGE );
        pcidev = pci_get_device( ( unsigned int ) PCI_VENDOR_ID_XILINX_AXI_BRIDGE,
                                 ( unsigned int ) PCI_DEVICE_ID_XILINX_AXI_BRIDGE,
                                  pcidev );
        if( pcidev ) {
            printk( KERN_INFO "pci_axi_ioctl_drv valid driver pci_dev found.\n" );
            xilinxAxi_devices[founddevs].pcidev = pcidev;   //Store pci_dev pointer
        } else {
            printk( KERN_ERR "pci_axi_ioctl_drv failed to find next pci device. %d devices found\n", founddevs );
            return founddevs;
        }
        privdata = ( xilinx_axi_privData_t * ) pci_get_drvdata( pcidev );
        if( pcidev && privdata ) {
            xilinxAxi_devices[founddevs].prvdata = privdata;
            founddevs++;
            printk( KERN_INFO "pci_axi_ioctl_drv valid dev & driver_data found.\n" );
         } else {
            printk( KERN_ERR "pci_axi_ioctl_drv valid driver_data not found.\n" );
            //return founddevs;
        }
        if(founddevs){  //  jump out early only have one device currently
            return founddevs;
        }
    } while ( pcidev != NULL ); // Find all devices

    return founddevs;
}



/*
 * Open and close
 */

int xilinxAxi_open( struct inode *inode, struct file *filp )
{
    int numFound;
    if( down_interruptible( &( xilinxAxi_devices[0].sem ) ) ) {
        printk( KERN_INFO " could not hold semaphore.  Device in use\n" );
        return -ERESTARTSYS;;
    }
    numFound = xilinxAxi_find_devices();
    if( numFound != 1 ) {
        printk( KERN_ERR "xilinxAxi_open failed to find pci resource.\n" );
        up( &( xilinxAxi_devices[0].sem ) );
        return -ERESTARTSYS;          /* gen failure */
    }

#if 0  // Have not implemented sysFS yet
    dev = container_of( inode->i_cdev, struct xilinxAxi_dev, cdev );
    filp->private_data = dev; /* for other methods */

    /* now trim to 0 the length of the device if open was write-only */
    if ( ( filp->f_flags & O_ACCMODE ) == O_WRONLY ) {
        if ( down_interruptible( &dev->sem ) ) {
            return -ERESTARTSYS;
        }
        scull_trim( dev ); /* ignore errors */
        up( &dev->sem );
    }
#endif
    up( &(xilinxAxi_devices[0].sem) );
    return 0;          /* success */
}

int xilinxAxi_release( struct inode *inode, struct file *filp )
{
    up( &( xilinxAxi_devices[0].sem ) );
    //printk( KERN_INFO " released semaphore" );
    return 0;
}


/*
 * Data management: read and write
 */

/* 
 *  The format of the data passed to theses special ioctl read/writes routines 
 *  encodes the address in the request,  Followed by the new Value for writes. 
 */

ssize_t xilinxAxi_read( struct file *filp, char __user *buff, size_t count, loff_t *offp )
{
    unsigned long ret = 0;
    struct xilinx_axi_privData *privdata ;
    uint32_t *data_p, data, reqAddr, windowOffset;
    struct pci_dev *pciDev;
    unsigned int minor; // take from inode
    unsigned int bar0Len;
    unsigned int minaddr, maxaddr;

    if( down_interruptible( &( xilinxAxi_devices[0].sem ) ) ) {
        printk( KERN_INFO " could not hold semaphore.  Device in use\n" );
        return -EBUSY;;
    }
    //printk( "Inside vectored read \n" );
    if(!( pciDev = xilinxAxi_devices[0].pcidev ) ) {
        printk( "xilinxAxi_read abort pcidev =  0x%p\n", xilinxAxi_devices[0].pcidev );
        up( &( xilinxAxi_devices[0].sem ) );
        return 0;
    }
    
    do{
        privdata = pci_get_drvdata(pciDev);
        if( !privdata )  {
            printk( "xilinxAxi_read abort privdata =  0x%p\n", privdata);
            ret = -EFAULT;
            break;
        }
        bar0Len = pci_resource_len (pciDev, 0);
    
        //printk( "xilinxAxi_read privdata->register0 =  0x%p length = 0x%08X\n",  privdata->register0, bar0Len);
        if (count != sizeof(uint32_t)){
            printk( "xilinxAxi illegal read size\n" );
            ret = -EFAULT;
            break;
        }

        minor = MINOR(filp->f_inode->i_rdev);
        if((minor != 0) && (minor != 1)){
            up( &(xilinxAxi_devices[0].sem) );
            return -EFAULT;
        }
        
        /* 
         * The requested address needs to be adjusted to align with the PCI memory mapped window
         *
         * Validate it is in word aligned, in range and normalize Address before using
         */
        ret = copy_from_user ((void*) &reqAddr, (void*)buff, sizeof(uint32_t));
        if( ret ){
            printk( "copy_from_user ret %lu\n", ret );
            ret = -EFAULT;
            break;
        }
        //printk( "read request address 0x%08X\n", reqAddr );
        switch ( minor ) {
            case 0: 
                minaddr = MIN_BAR0_AXI_ADDR; 
                maxaddr = MAX_BAR0_AXI_ADDR; 
                break;
            case 1: 
                minaddr = MIN_BAR2_AXI_ADDR; 
                maxaddr = MAX_BAR2_AXI_ADDR; 
                break;
            default: 
                break;
         }
        if( (reqAddr % sizeof(uint32_t)) || (reqAddr < minaddr) || (reqAddr > maxaddr) ){
            printk( "illegal address requested 0x%08X\n", reqAddr );
            ret = -EFAULT;
            break;
        }
        switch ( minor ) {
            case 0: 
                // Normalize the request address to BAR0 address range
                windowOffset = reqAddr - AXI_BARO_MAP_OFFSET; 
                //printk( "requested data from Bar0 offset 0x%08X\n", windowOffset );
                data_p = privdata->register0 + (windowOffset / sizeof(uint32_t));
                break;
            case 1: 
                // Normalize the request address to BAR0 address range
                windowOffset = reqAddr - AXI_BAR2_MAP_OFFSET; 
                //printk( "requested data from Bar0 offset 0x%08X\n", windowOffset );
                data_p = privdata->register2 + (windowOffset / sizeof(uint32_t));
                break;
            default: 
                break;
         }

    
        data = ioread32(data_p); // PCI only support 32 bit read/writes can't do bytewise copy
        //printk( "ioread32 %p = %08X\n", data_p, data );  
        ret = copy_to_user( buff, (void*) &data, sizeof(uint32_t));
        //printk( "xilinxAxi_read done\n" );
        break;
    } while(1);

    up( &( xilinxAxi_devices[0].sem ) );
    return ret;
}


ssize_t xilinxAxi_write( struct file *filp, const char __user *buff, size_t count, loff_t *offp )
{
    unsigned long ret = 0;
    struct xilinx_axi_privData *privdata ;
    struct pci_dev *pciDev;
    unsigned int minor; // take from inode
    unsigned int bar0Len;
    uint32_t *data_p;
    uint32_t  data, reqAddr, windowOffset;
    unsigned int minaddr, maxaddr;
//    uint32_t  data2;

    if( down_interruptible( &( xilinxAxi_devices[0].sem ) ) ) {
        printk( KERN_INFO " could not hold semaphore.  Device in use\n" );
        return -EBUSY;;
    }
    if(!( pciDev = xilinxAxi_devices[0].pcidev ) ) {
        printk( "xilinxAxi_read abort pcidev =  0x%p\n", xilinxAxi_devices[0].pcidev );
        up( &( xilinxAxi_devices[0].sem ) );
        return -EFAULT;
    }
    
    do{
        privdata = pci_get_drvdata(pciDev);
        bar0Len = pci_resource_len (pciDev, 0);
    
        if( !privdata )  {
            printk( "xilinxAxi_read abort privdata =  0x%p\n", privdata);
            ret = -EFAULT;
            break;
        }
        //printk( "xilinxAxi_write privdata->register0 =  0x%p length = 0x%08X\n",  privdata->register0, bar0Len);
    
        if (count != (2 * sizeof(uint32_t))){  // two parameters needed.
            printk( "xilinxAxi illegal write size cmd\n" );
            ret = -EFAULT;
            break;
        }

        minor = MINOR(filp->f_inode->i_rdev);
        if((minor != 0) && (minor != 1)){
            up( &(xilinxAxi_devices[0].sem) );
            return -EFAULT;
        }

        /* 
         * The requested address needs to be adjusted to align with the PCI memory mapped window
         * Validate it is in word aligned, in range and normalize Address before using
         */
    
        ret = copy_from_user ((void*) &reqAddr, (void*)buff, sizeof(uint32_t));
        if( ret ){
            printk( "copy_from_user ret %lu\n", ret );
            up( &( xilinxAxi_devices[0].sem ) );
            ret = -EFAULT;
            break;
        }
    
        //printk( "write request address 0x%08X\n", reqAddr );
        switch ( minor ) {
            case 0:
                minaddr = MIN_BAR0_AXI_ADDR;
                maxaddr = MAX_BAR0_AXI_ADDR;
                break;
            case 1:
                minaddr = MIN_BAR2_AXI_ADDR;
                maxaddr = MAX_BAR2_AXI_ADDR;
                break;
            default:
                break;
         }

        if( (reqAddr % sizeof(uint32_t)) || (reqAddr < minaddr) || (reqAddr > maxaddr) ){
            printk( "illegal address requested 0x%08X\n", reqAddr );
            ret = -EFAULT;
            break;
        }
    
        switch ( minor ) {
            case 0:
                // Normalize the request address to BAR0 address range
                windowOffset = reqAddr - AXI_BARO_MAP_OFFSET;
                //printk( "requested data from Bar0 offset 0x%08X\n", windowOffset );
                data_p = privdata->register0 + (windowOffset / sizeof(uint32_t));
                break;
            case 1:
                // Normalize the request address to BAR0 address range
                windowOffset = reqAddr - AXI_BAR2_MAP_OFFSET;
                //printk( "requested data from Bar0 offset 0x%08X\n", windowOffset );
                data_p = privdata->register2 + (windowOffset / sizeof(uint32_t));
                break;
            default:
                break;
         }


        ret = copy_from_user( ( void* ) &data, (void*)&buff[sizeof(uint32_t)], sizeof(uint32_t) );
        if(ret){
            printk( "xilinxAxi_write couldn't get new write value\n" );
            ret = -EFAULT;
            break;
        }
        iowrite32( data, data_p); // PCI only support 32 bit read/writes can't do bytewise copy
        //printk( "new iowrite32 %p = %08X\n", data_p, data );  
        break;
    } while (1); 


    /* DEBUG read PCI device to force completion of write request */
    //data2 = ioread32(data_p); // PCI only support 32 bit read/writes can't do bytewise copy
    //printk( "iowrite32  %08X to %p -- read back %08X\n" , data, data_p, data2 );
    //printk( "xilinxAxi_write done\n" );
    up( &(xilinxAxi_devices[0].sem) );
    return ret; // Number of ByTES
}

/*
 * The ioctl() implementation
 * Don't think we will need this long term unless we add a reset function ????
 * which I imagine we will need.
 */

long xilinxAxi_ioctl( struct file *filp, unsigned int cmd, unsigned long arg )
{

    int retval = 0;

    switch( cmd ) {

    case 1:
        return 0xDEADBEEF;
        break;

    default:
        return -ENOTTY;
    }

    return retval;
}



/*
 * The cleanup function is used to handle initialization failures as well.
 * Thefore, it must be careful to work correctly even if some of the items
 * have not been initialized
 */
void xilinxAxi_cleanup_module( void )
{
    int i;
    dev_t devno = MKDEV( xilinxAxi_major, xilinxAxi_minor );

    /* Get rid of our char dev entries */
    if ( xilinxAxi_devices ) {
        for ( i = 0; i < xilinxAxi_nr_devs; i++ ) {
            cdev_del( xilinxAxi_devices[i].cdev );
        }
        kfree( xilinxAxi_devices );
    }

    /* cleanup_module is never called if registering failed */
    unregister_chrdev_region( devno, xilinxAxi_nr_devs );
}


struct file_operations xilinxAxi_fops = {
    .owner  = THIS_MODULE,
//    .llseek = xilinxAxi_lseek,
    .read   = xilinxAxi_read,
    .write  = xilinxAxi_write,
//    .ioctl  = xilinxAxi_ioctl,
//    .mmap   = xilinxAxi_mmap,
    .open   = xilinxAxi_open,
    .release = xilinxAxi_release 
};


#if 0
/*
 * Set up the char_dev structure for this device.
 */
static void xilinxAxi_setup_cdev( xilinxAxi_dev_t *this_dev, int index )
{
    this_dev->cdev = cdev_alloc();
    cdev_init( this_dev->cdev, &xilinxAxi_fops );
    this_dev->cdev->owner = THIS_MODULE;
    this_dev->cdev->ops = &xilinxAxi_fops;
    printk( KERN_NOTICE "xilinxAxi_setup_cdev\n" );
}
#endif

static int __init xilinxAxi_init_module( void )
{
    int result = 0;
    int i;
    dev_t dev_no, dev;
    struct cdev * cdev;
    printk( KERN_DEBUG "xilinxAxi_init_module\n" );

    /*
     * allocate the devices -- normally they can't be static, as the number
     * can be specified at load time . Be general in this code for reuse.
     */

    xilinxAxi_devices = kzalloc( xilinxAxi_nr_devs * sizeof( xilinxAxi_dev_t ), GFP_KERNEL );
    if ( !xilinxAxi_devices ) {
        result = -ENOMEM;
        goto fail;  /* Make this more graceful */
    }

    /* Register your major. */
    //    result = register_chrdev_region(XILINXAXIMAJOR, 1, driverName);     // static assignment
    result = alloc_chrdev_region( &dev_no, 0, MAX_NBR_AXI_CHAR_DEVICES, driverName ); // dynamic allocation

    if ( result < 0 ) {
        //printk("xilinxAxi_init_module: register_chrdev_region err= %d\n", result);
        printk( "xilinxAxi_init_module: alloc_chrdev_region err= %d\n", result );
        return result;
    }

    //Major = result;
    Major = MAJOR( dev_no );

    cdev = cdev_alloc();
    cdev->ops = &xilinxAxi_fops;
    cdev->owner = THIS_MODULE;
    cdev_init( cdev, &xilinxAxi_fops );

    /* Initialize each device.  Only one device for now */
    for ( i = 0; i < MAX_NBR_AXI_CHAR_DEVICES; i++ ) {
        xilinxAxi_devices[i].cdev = cdev;

        sema_init( &xilinxAxi_devices[i].sem, 1 );

        dev = MKDEV( Major, i );
        xilinxAxi_devices[i].dev = dev;  //pci_dev
        printk( KERN_NOTICE "xilinxAxi driver Major number is %d, minor %d \n", Major, i );
        printk( KERN_NOTICE "xilinxAxi_init_module dev = %08X\n", dev );

        result = cdev_add ( cdev, dev, 1 );    // char dev

        if ( result < 0 ) {
            printk( "xilinxAxi_init_module: unable to add cdev %d\n", result );
            return result;
        }
    }
    /* At this point we are initialized */

    return 0;

fail:
    xilinxAxi_cleanup_module();
    return result;
}


module_init( xilinxAxi_init_module );
module_exit( xilinxAxi_cleanup_module );

