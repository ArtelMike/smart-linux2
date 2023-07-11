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
#ifndef GSIO_H
#define GSIO_H

#include "avs_loadfpga.h"


// CPU GPIO Regiter base addresses
#define GPIO1_BASE		0x02300000
#define GPIO2_BASE		0x02310000
#define GPIO3_BASE		0x02320000
#define GPIO4_BASE		0x02330000

#define SCFG_QEIOCLKCR          0x01570400
// Offset for DDR registers
#define GPDIR	0
#define GPDAT	8

//static const unsigned int gpio_base[4]= {0x02300000, 0x02310000, 0x02310000, 0x02320000 };
// GPIO controller base address for control (first address) and data pins (second address)

enum gpio_base_index {DATA=0, CONTROL};
//
static const unsigned int gpio_base_iot[2]= {GPIO3_BASE, GPIO3_BASE };   // IOT uses GPIO3 for data and control
static const unsigned int gpio_base_dlc[2]= {GPIO4_BASE, GPIO2_BASE };  // DLC use GPIO2 for control and GPIO4 for data

// Lnngth of each region of GPIO memory (per GPIO bank)
#define GPIO_LEN		32

#define GPIO_GPDIR		0
#define GPIO_GPDAT		8


//
// Pin definitions for the XILINX programming interface for the IOT and DLC boards
//

enum control_pin_index  {INITN=0, RDRWB, CCLK, PROGN, CSIN, DONE};
// Next two indexed by above
static unsigned int iot_control_pins[6]= {16, 19, 17, 15, 18, 16 };
static unsigned int dlc_control_pins[6]= {25, 15, 11, 13, 12, 14 };

// Data pins numbers.  Remember MSB is bit 0 in NXP world, and it's the MSB
static const unsigned int iot_bit0_pos = 20;   // D0= 20 21 22 23 .... 27 (8 bits)
static const unsigned int dlc_bit0_pos = 9;    // D0 = 9 10 11 ...24 (16 bits)


/*
 * program bus width in bytes
 */
enum wbus {
	bus_1byte	= 1,
	bus_2byte	= 2,
};


// How long to wait for the DONE to assert after a FPGA load - no units
#define MAX_WAIT_DONE	10000

struct gpiobus {
	int	ngpio;
	void __iomem *r[4];
};

int xl_supported_prog_bus_width(enum wbus );

void xl_program_b(int32_t i);
void xl_rdwr_b(int32_t i);
void xl_csi_b(int32_t i);

int xl_get_init_b(void);
int xl_get_done_b(void);

void xl_shift_cclk(int count);
void xl_shift_bytes_out(enum wbus bus_byte, unsigned char *pdata);

int xl_init_io(board_t board);
int xl_exit_io(void);
void xl_clock_pulse(unsigned int bit);
void xl_cclk_b(unsigned int bit);



inline void xl_set_bit(unsigned int bit, unsigned int value);
inline const unsigned int xl_get_bit(unsigned int bit);

#endif
