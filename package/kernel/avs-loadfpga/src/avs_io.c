/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/firmware.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <asm/io.h>


#include "avs_io.h"


static inline void byte0_out(unsigned char data);
static inline void byte1_out(unsigned char data);

int i;
static unsigned int gpioControlBase;
static unsigned int gpioDataBase;
static unsigned int configBase;
static unsigned int* control_pins;
static int start_bit_pos, end_bit_pos;


static inline unsigned int __bswap32( unsigned int val )
{
#ifndef DONT_SWAP
    val = ((val << 8) & 0xFF00FF00 ) | ((val >> 8) & 0xFF00FF );
    return (val << 16) | (val >> 16);
#else
    return val;
#endif

}

/* Assert and Deassert CCLK */
void inline xl_shift_cclk(int count)
{
	int i;

	for (i = 0; i < count; i++) {
		xl_cclk_b(1);
		xl_cclk_b(0);
	}
}

// Obsolete - set based on board type

int xl_supported_prog_bus_width(enum wbus bus_bytes)
{
	switch (bus_bytes) {
	case bus_1byte:
		break;
	case bus_2byte:
		break;
	default:
		pr_err("unsupported program bus width %d\n",
				bus_bytes);
		return 0;
	}

	return 1;
}

/* Serialize byte and clock each bit on target's DIN and CCLK pins */
void inline xl_shift_bytes_out(enum wbus bus_byte, unsigned char *pdata)
{
  //volatile unsigned long int current_gpio;

#if 1
	//printk("Output ------->> %0x %0x \n", pdata[0], pdata[1]);

	/*
	 * supports 1 and 2 bytes programming mode
	 */
	if (likely(bus_byte == bus_2byte))
	  {
	    byte0_out(pdata[0]);   // upper bits
	    byte1_out(pdata[1]);   // lower 8 bits
	  }
	else
	  {
	    byte1_out(pdata[0]);
	  }
#endif
#if 0
	unsigned char rev_byte;

		current_gpio = ioread32((void *)(gpioDataBase+GPIO_GPDAT));
		current_gpio = __bswap32(current_gpio);
		printk("gpio32 bits%x\n", current_gpio);
#endif
	// This is for 8 bit transfers or the second byte of 16 bits.   Data goes on the lower 8 bits to the Xilinx
#ifdef SLOW_WAY
	xl_shift_cclk(1);
#else

	// This lowers then raises the clock

	xl_clock_pulse(control_pins[CCLK]);
#endif
;
}

// Set a GPIO bit  - utility functions
// This only should be used for control pins

// NB: GPIO pins are 0 to 31 and pin 0 corresponds to Bit 0, the MSB (leftmost) in the GPIO registers per NXP convention

inline void xl_set_bit(unsigned int bit, unsigned int value)
{
    volatile unsigned long int current_gpio;
	current_gpio = ioread32((void *)(gpioControlBase+GPIO_GPDAT));
	current_gpio = __bswap32(current_gpio);
	if (value ==1)
	{
		current_gpio |= (0x80000000>>bit);
	}
	else
	{
		current_gpio &= ~(0x80000000>>bit);
	}
	current_gpio = __bswap32(current_gpio);
	iowrite32(current_gpio, ((void *)(gpioControlBase+GPIO_GPDAT)));
}

inline void xl_data_set_bit(unsigned int bit, unsigned int value)
{
    volatile unsigned long int current_gpio;
	current_gpio = ioread32((void *)(gpioDataBase+GPIO_GPDAT));
	current_gpio = __bswap32(current_gpio);
	if (value ==1)
	{
		current_gpio |= (0x80000000>>bit);
	}
	else
	{
		current_gpio &= ~(0x80000000>>bit);
	}
	current_gpio = __bswap32(current_gpio);
	iowrite32(current_gpio, ((void *)(gpioDataBase+GPIO_GPDAT)));
}

inline void xl_clock_pulse(unsigned int bit)
{

	// Note: In NXP doc, the MSB is numbered 0 and the LSB 31, so to set bit N, you shift 0x800000 N
	// to the right.
	// Also note CPU registers are byte addressable, so reading in 32bits starting at the specified address
	// puts the MOST signicant byte at the right side of a 32bit word, so you have to swap them into LE order
	// to make everything work.

	volatile unsigned long int current_gpio, save_gpio;
	current_gpio = ioread32((void *) (gpioControlBase + GPIO_GPDAT));
	current_gpio = __bswap32(current_gpio);

	current_gpio &= ~(0x80000000u >> bit);   // lower clock
	save_gpio = __bswap32(current_gpio);
	iowrite32(save_gpio, ((void *) (gpioControlBase + GPIO_GPDAT)));

	// Delay here if pulse needs to be longer
	current_gpio |= (0x80000000u >> bit);           // raise clock
	current_gpio = __bswap32(current_gpio);
	iowrite32(current_gpio, ((void *) (gpioControlBase + GPIO_GPDAT)));
}
// Get a GPIO input bit - utility functions

inline const unsigned int xl_get_bit(unsigned int bit)
{
    volatile unsigned long int current_gpio;
	current_gpio = ioread32((void *)(gpioControlBase+GPIO_GPDAT));
	current_gpio = __bswap32(current_gpio);
	return ( current_gpio << bit) & 0x80000000;
}
/*
 * generic bit swap for xilinx SYSTEMMAP FPGA programming
 */
void xl_program_b(int32_t i)
{
	xl_set_bit(control_pins[PROGN], i);
}

void xl_rdwr_b(int32_t i)
{
	xl_set_bit(control_pins[RDRWB], i);
}

void xl_csi_b(int32_t i)
{
	xl_set_bit(control_pins[CSIN], i);
}

int xl_get_init_b(void)
{
	return xl_get_bit(control_pins[INITN]);
}

int xl_get_done_b(void)
{
	return xl_get_bit(control_pins[DONE]);
}

// Reverse a byte
unsigned char reverse(unsigned char b)
{
   b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
   b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
   b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
   return b;
}


// Insert byte c starting at bitpos (0 based) into integer x
unsigned long int inline replaceByte(unsigned long int x, int bitpos, unsigned char c)
{
    unsigned int old_c = c<<24;
    unsigned long int shift = (old_c >> bitpos);
    unsigned long int mask = 0xff000000 >> bitpos;
    return (~mask & x) | shift;
}


// Second byte of 16 bit data path
static inline void byte0_out(unsigned char data)
{
    static volatile unsigned long int current_gpio;
    //static unsigned char rev_byte;

	current_gpio = ioread32((void *)(gpioDataBase+GPIO_GPDAT));
	current_gpio = __bswap32(current_gpio);

	// Reverse the bits so D0 aligns with the starting bit and goes towards higher pins numbers
	//rev_byte = reverse(data);

        // put the data onto the upper 8 bits (D[15:8] of the Xilinx interface
        current_gpio = replaceByte(current_gpio, start_bit_pos+8, data);  // Start bit is D[0], so start_bit+8 = D[8] to Xilinx
        //current_gpio = replaceByte(current_gpio, start_bit_pos+8, rev_byte);  // Start bit is D[0], so start_bit+8 = D[8] to Xilinx
	current_gpio = __bswap32(current_gpio);
	iowrite32(current_gpio, ((void *)(gpioDataBase+GPIO_GPDAT)));
}


// 8 bit path outputter (1st byte)
static inline void byte1_out(unsigned char data)
{
    volatile unsigned long int current_gpio;
    //unsigned char rev_byte;

	current_gpio = ioread32((void *)(gpioDataBase+GPIO_GPDAT));
	current_gpio = __bswap32(current_gpio);

	// Reverse the bits so D0 aligns with the starting bit and goes towards higher pins numbers
	//rev_byte = reverse(data);

        current_gpio = replaceByte(current_gpio, start_bit_pos, data);               // <<< 2nd byte of the word
        //current_gpio = replaceByte(current_gpio, start_bit_pos, rev_byte);               // <<< 2nd byte of the word
	current_gpio = __bswap32(current_gpio);
	iowrite32(current_gpio, ((void *)(gpioDataBase+GPIO_GPDAT)));

}

static inline void bxl_cclk_b(unsigned char data)
{
	// This does 8 bits at a whack?  gpio_set_value(gpioTest, i);

}

inline void xl_cclk_b(unsigned int i)
{
	xl_set_bit(control_pins[CCLK], i);

}



// Function to cleanup after all I/O is complete

int xl_exit_io(void)
{
	// Free up the GPIO pins for use
#if 0
        // Release is not needed as we never reserved the GPIO registers. Rhe reserve fails,likely because GPIO driver is using them
        // for sysfs interface. Still seems to work ok
	release_mem_region( gpioControlBase, GPIO_LEN);
	release_mem_region( gpioDataBase, GPIO_LEN);
#endif
	return 0;

}
// Passed type of board (iot or dlc4555)

int xl_init_io(board_t board)
{
	unsigned long int current_ddr, ddr_in_mask, ddr_out_mask, ddr_out_mask_data, configValue;
        //int k;


#if 0
	// Reserving the GPIO register space fails - likely already rerved by the GPIO sysfs driver, but it still work
	int ierr;
	// Reserve the memory area for this - these always fail - probably sysfs driver consumed them already
	if( (ierr==request_mem_region( GPIO_XILINX, GPIO_LEN, "XILINX PINS" )) == NULL)
	  {
	    printk( "GPIO pins for Xilins download not available %x\n",ierr);
#warning "Error path removed -  this always fails - sysfs owns it?"
	    //return -EBUSY;
	  }
	//xilinx_gpio = (unsigned long int)ioremap( GPIO_XILINX, GPIO_LEN);

#endif


	// From board type, set up base address of GPIO control and data pin banks and pins numbers from the io.h tables

	if (board==iot)
	{
              printk("Defining signals for IOT board\n");
	      gpioControlBase = (unsigned long int)ioremap(gpio_base_iot[CONTROL], GPIO_LEN);
	      gpioDataBase = (unsigned long int)ioremap(gpio_base_iot[DATA], GPIO_LEN);
	      control_pins = iot_control_pins;
	      start_bit_pos = iot_bit0_pos;
              end_bit_pos = iot_bit0_pos+(8-1);
	}
	else  // dlc - 16 bit interface
	{
            printk("Defining signals for DLC4555 board\n");
	    gpioControlBase = (unsigned long int)ioremap(gpio_base_dlc[CONTROL], GPIO_LEN);
	    gpioDataBase = (unsigned long int)ioremap(gpio_base_dlc[DATA], GPIO_LEN);
	    control_pins = dlc_control_pins;
	    start_bit_pos = dlc_bit0_pos;
	    end_bit_pos = dlc_bit0_pos+(16-1);
	    //end_bit_pos = dlc_bit0_pos+(8-1);


	    // Set up system config to all GPIOs shared with TDMA clocks to be used ad GPIOs
	    configBase =  (unsigned long int)ioremap(SCFG_QEIOCLKCR , 4);
	    configValue = ioread32(((void *)(configBase)));
	    configValue = __bswap32(configValue);

	    // Set the configs for the 4 GPIO pins
	    configValue &= 0x00FFFFFF;
	    configValue |= 0x55000000;   // set b'01' in bits 0, 2 ,4 ,6 2 bit fields
	    configValue = __bswap32(configValue);
	    iowrite32(configValue, (void *)(configBase));

	}



	// Form a Data Direction Register mask. First the ins (0) then the outs (1)

        // Input in the DDR need to be 0  enum control_pin_index = {INITN=0, RDRWB, CCLK, PROGN, CSIN, DONE};

	ddr_in_mask = 0x00000000;
        ddr_in_mask |= 0x80000000 >>  control_pins[INITN];
	ddr_in_mask |= 0x80000000 >>  control_pins[DONE];   	// clear these bits in the ddr
//	ddr_in_mask = ~ddr_in_mask;        			// And this into DDR

	// Output pins in the DDR need to be 1
	ddr_out_mask = 0x00000000;
	ddr_out_mask |= 0x80000000 >>  control_pins[CSIN];
	ddr_out_mask |= 0x80000000 >>  control_pins[RDRWB];
	ddr_out_mask |= 0x80000000 >>  control_pins[CCLK];
	ddr_out_mask |= 0x80000000 >>  control_pins[PROGN];

        // Set the DDR for the control pin bank - mix of ins and outs
	current_ddr = ioread32(((void *)(gpioControlBase+GPIO_GPDIR)));
	current_ddr = __bswap32(current_ddr);
	current_ddr |= ddr_in_mask;     			// set the input bits
	current_ddr = __bswap32(current_ddr);
        msleep(1);
	current_ddr &= ~ddr_in_mask;     			// clear the input bits
	current_ddr |= ddr_out_mask;    			// set the output bits
	current_ddr = __bswap32(current_ddr);
	iowrite32(current_ddr, (void *)(gpioControlBase+GPIO_GPDIR));
	//printk("DDR control register dump setting  %lx    readback %0x \n\n", current_ddr, ioread32((void *)(gpioControlBase+GPIO_GPDIR)));


        // Data output mask for the gpio data pin outputs - set 8 or 16 pins
        ddr_out_mask_data= 0x00000000;
	for (i=start_bit_pos; i<=end_bit_pos; i++)
	{
	    ddr_out_mask_data |= 0x80000000 >>  i;
	}

	// Set the DDR for the data bank - all output pins
	current_ddr = ioread32(((void *)(gpioDataBase+GPIO_GPDIR)));
	current_ddr = __bswap32(current_ddr);
	current_ddr |= ddr_out_mask_data;
	current_ddr = __bswap32(current_ddr);
	iowrite32(current_ddr, (void *)(gpioDataBase+GPIO_GPDIR));


	printk("DDR data register dump setting  %lx    readback %0x \n\n", current_ddr, ioread32((void *)(gpioDataBase+GPIO_GPDIR)));

#ifdef WANT_SCOPE_PROBES
        for (k=0; k<10; k++) {
	// Test all the output pins individually - good for scope or analyzer probes
        printk("Strobing pins 1hz\n");
	xl_program_b(1); xl_program_b(0); xl_program_b(1);
	xl_rdwr_b(1); xl_rdwr_b(0); xl_rdwr_b(1);
	xl_cclk_b(1); xl_cclk_b(0); xl_cclk_b(1);
	xl_csi_b(1); xl_csi_b(0); xl_csi_b(1);

	// Strobe each data bit quickly
	for (i=start_bit_pos; i<=end_bit_pos; i++)
	{
	    ddr_out_mask_data |= 0x80000000 >>  i;
	    xl_set_bit(i, 1); xl_set_bit(i, 0);xl_set_bit(i, 1);
	}
        msleep(1000);
      
       } // remove this

#endif


   // Set up each of the required IO pins for SYSFS access

	return 0;
}

#ifndef WANT_EXPORT_SYMBOLS
EXPORT_SYMBOL(xl_shift_cclk);
EXPORT_SYMBOL(xl_supported_prog_bus_width);
EXPORT_SYMBOL(xl_shift_bytes_out);
EXPORT_SYMBOL(xl_csi_b);
EXPORT_SYMBOL(xl_program_b);
EXPORT_SYMBOL(xl_rdwr_b);
EXPORT_SYMBOL(xl_get_init_b);
EXPORT_SYMBOL(xl_get_done_b);
EXPORT_SYMBOL(byte0_out);
EXPORT_SYMBOL(bxl_cclk_b);
EXPORT_SYMBOL(xl_cclk_b);
EXPORT_SYMBOL(xl_init_io);
EXPORT_SYMBOL(xl_exit_io);

#endif
MODULE_AUTHOR("Larry C");
MODULE_DESCRIPTION("Xlinix FPGA firmware download - Platform");
MODULE_LICENSE("GPL");
