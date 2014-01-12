/*
 * (C) Copyright 2007-2008
 * Stelian Pop <stelian.pop@leadtechdesign.com>
 * Lead Tech Design <www.leadtechdesign.com>
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <asm/arch/at91_common.h>
#include <asm/arch/at91_pmc.h>
#include <asm/arch/gpio.h>
#include <asm/arch/io.h>

void at91_serial0_hw_init(void)
{
	at91_set_A_periph(AT91_PIN_PB19, 1);	/* TXD0 */
	at91_set_A_periph(AT91_PIN_PB18, 0);	/* RXD0 */
	at91_sys_write(AT91_PMC_PCER, 1 << AT91SAM9G45_ID_US0);
}

void at91_serial1_hw_init(void)
{
	at91_set_A_periph(AT91_PIN_PB4, 1);		/* TXD1 */
	at91_set_A_periph(AT91_PIN_PB5, 0);		/* RXD1 */
	at91_sys_write(AT91_PMC_PCER, 1 << AT91SAM9G45_ID_US1);
}

void at91_serial2_hw_init(void)
{
	at91_set_A_periph(AT91_PIN_PD6, 1);		/* TXD2 */
	at91_set_A_periph(AT91_PIN_PD7, 0);		/* RXD2 */
	at91_sys_write(AT91_PMC_PCER, 1 << AT91SAM9G45_ID_US2);
}

void at91_serial3_hw_init(void)
{
	at91_set_A_periph(AT91_PIN_PB12, 0);	/* DRXD */
	at91_set_A_periph(AT91_PIN_PB13, 1);	/* DTXD */
	at91_sys_write(AT91_PMC_PCER, 1 << AT91_ID_SYS);;
}

void at91_serial_hw_init(void)
{
#ifdef CONFIG_USART0
	at91_serial0_hw_init();
#endif

#ifdef CONFIG_USART1
	at91_serial1_hw_init();
#endif

#ifdef CONFIG_USART2
	at91_serial2_hw_init();
#endif

#ifdef CONFIG_USART3	/* DBGU */
	at91_serial3_hw_init();
#endif
}

#ifdef CONFIG_ATMEL_SPI
void at91_spi0_hw_init(unsigned long cs_mask)
{
	at91_set_A_periph(AT91_PIN_PB0, 0);	/* SPI0_MISO */
	at91_set_A_periph(AT91_PIN_PB1, 0);	/* SPI0_MOSI */
	at91_set_A_periph(AT91_PIN_PB2, 0);	/* SPI0_SPCK */

	/* Enable clock */
	at91_sys_write(AT91_PMC_PCER, 1 << AT91SAM9G45_ID_SPI0);

	if (cs_mask & (1 << 0)) {
		at91_set_A_periph(AT91_PIN_PB3, 0);
	}
	if (cs_mask & (1 << 1)) {
		at91_set_B_periph(AT91_PIN_PB18, 0);
	}
	if (cs_mask & (1 << 2)) {
		at91_set_B_periph(AT91_PIN_PB19, 0);
	}
	if (cs_mask & (1 << 3)) {
		at91_set_B_periph(AT91_PIN_PD27, 0);
	}
	if (cs_mask & (1 << 4)) {
		at91_set_gpio_output(AT91_PIN_PB3, 0);
	}
	if (cs_mask & (1 << 5)) {
		at91_set_gpio_output(AT91_PIN_PB18, 0);
	}
	if (cs_mask & (1 << 6)) {
		at91_set_gpio_output(AT91_PIN_PB19, 0);
	}
	if (cs_mask & (1 << 7)) {
		at91_set_gpio_output(AT91_PIN_PD27, 0);
	}
}

void at91_spi1_hw_init(unsigned long cs_mask)
{
	at91_set_A_periph(AT91_PIN_PB14, 0);	/* SPI1_MISO */
	at91_set_A_periph(AT91_PIN_PB15, 0);	/* SPI1_MOSI */
	at91_set_A_periph(AT91_PIN_PB16, 0);	/* SPI1_SPCK */

	/* Enable clock */
	at91_sys_write(AT91_PMC_PCER, 1 << AT91SAM9G45_ID_SPI1);

	if (cs_mask & (1 << 0)) {
		at91_set_A_periph(AT91_PIN_PB17, 0);
	}
	if (cs_mask & (1 << 1)) {
		at91_set_B_periph(AT91_PIN_PD28, 0);
	}
	if (cs_mask & (1 << 2)) {
		at91_set_A_periph(AT91_PIN_PD18, 0);
	}
	if (cs_mask & (1 << 3)) {
		at91_set_A_periph(AT91_PIN_PD19, 0);
	}
	if (cs_mask & (1 << 4)) {
		at91_set_gpio_output(AT91_PIN_PB17, 0);
	}
	if (cs_mask & (1 << 5)) {
		at91_set_gpio_output(AT91_PIN_PD28, 0);
	}
	if (cs_mask & (1 << 6)) {
		at91_set_gpio_output(AT91_PIN_PD18, 0);
	}
	if (cs_mask & (1 << 7)) {
		at91_set_gpio_output(AT91_PIN_PD19, 0);
	}

}
#endif

#ifdef CONFIG_MACB
void at91_macb_hw_init(void)
{
	at91_set_A_periph(AT91_PIN_PA17, 0);	/* ETXCK_EREFCK */
	at91_set_A_periph(AT91_PIN_PA15, 0);	/* ERXDV */
	at91_set_A_periph(AT91_PIN_PA12, 0);	/* ERX0 */
	at91_set_A_periph(AT91_PIN_PA13, 0);	/* ERX1 */
	at91_set_A_periph(AT91_PIN_PA16, 0);	/* ERXER */
	at91_set_A_periph(AT91_PIN_PA14, 0);	/* ETXEN */
	at91_set_A_periph(AT91_PIN_PA10, 0);	/* ETX0 */
	at91_set_A_periph(AT91_PIN_PA11, 0);	/* ETX1 */
	at91_set_A_periph(AT91_PIN_PA19, 0);	/* EMDIO */
	at91_set_A_periph(AT91_PIN_PA18, 0);	/* EMDC */
#ifndef CONFIG_RMII
	at91_set_B_periph(AT91_PIN_PA29, 0);	/* ECRS */
	at91_set_B_periph(AT91_PIN_PA30, 0);	/* ECOL */
	at91_set_B_periph(AT91_PIN_PA8,  0);	/* ERX2 */
	at91_set_B_periph(AT91_PIN_PA9,  0);	/* ERX3 */
	at91_set_B_periph(AT91_PIN_PA28, 0);	/* ERXCK */
	at91_set_B_periph(AT91_PIN_PA6,  0);	/* ETX2 */
	at91_set_B_periph(AT91_PIN_PA7,  0);	/* ETX3 */
	at91_set_B_periph(AT91_PIN_PA27, 0);	/* ETXER */
#endif
}
#endif
