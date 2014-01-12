/*
 * (C) Copyright 2007
 * Sascha Hauer, Pengutronix
 *
 * (C) Copyright 2009 Freescale Semiconductor, Inc.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <asm/io.h>
#include <asm/arch/mx51.h>

/* nothing really to do with interrupts, just starts up a counter. */
int interrupt_init(void)
{
	return 0;
}

void reset_cpu(ulong addr)
{
	/* workaround for ENGcm09397 - Fix SPI NOR reset issue*/
	/* de-select SS0 of instance: eCSPI1 */
	writel(0x3, IOMUXC_BASE_ADDR + 0x218);
	writel(0x85, IOMUXC_BASE_ADDR + 0x608);
	/* de-select SS1 of instance: ecspi1 */
	writel(0x3, IOMUXC_BASE_ADDR + 0x21C);
	writel(0x85, IOMUXC_BASE_ADDR + 0x60C);

	__REG16(WDOG1_BASE_ADDR) = 4;
}
