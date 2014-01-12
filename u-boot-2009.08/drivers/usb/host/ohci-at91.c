/*
 * (C) Copyright 2006
 * DENX Software Engineering <mk@denx.de>
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

#if defined(CONFIG_USB_OHCI_NEW) && defined(CONFIG_SYS_USB_OHCI_CPU_INIT)

#include <asm/arch/hardware.h>
#include <asm/arch/io.h>
#include <asm/arch/at91_pmc.h>
#include <asm/arch/clk.h>

int usb_cpu_init(void)
{

#if defined(CONFIG_AT91CAP9) || defined(CONFIG_AT91SAM9260) || \
    defined(CONFIG_AT91SAM9263) || defined(CONFIG_AT91SAM9G20) || \
    defined(CONFIG_AT91SAM9261)
	/* Enable PLLB */
	at91_sys_write(AT91_CKGR_PLLBR, get_pllb_init());
	while ((at91_sys_read(AT91_PMC_SR) & AT91_PMC_LOCKB) != AT91_PMC_LOCKB)
		;
#endif

	/* Enable USB host clock. */
	at91_sys_write(AT91_PMC_PCER, 1 << AT91_ID_UHP);
#ifdef CONFIG_AT91SAM9261
	at91_sys_write(AT91_PMC_SCER, AT91_PMC_UHP | AT91_PMC_HCK0);
#else
	at91_sys_write(AT91_PMC_SCER, AT91_PMC_UHP);
#endif

	return 0;
}

int usb_cpu_stop(void)
{
	/* Disable USB host clock. */
	at91_sys_write(AT91_PMC_PCDR, 1 << AT91_ID_UHP);
#ifdef CONFIG_AT91SAM9261
	at91_sys_write(AT91_PMC_SCDR, AT91_PMC_UHP | AT91_PMC_HCK0);
#else
	at91_sys_write(AT91_PMC_SCDR, AT91_PMC_UHP);
#endif

#if defined(CONFIG_AT91CAP9) || defined(CONFIG_AT91SAM9260) || \
    defined(CONFIG_AT91SAM9263) || defined(CONFIG_AT91SAM9G20)
	/* Disable PLLB */
	at91_sys_write(AT91_CKGR_PLLBR, 0);
	while ((at91_sys_read(AT91_PMC_SR) & AT91_PMC_LOCKB) != 0)
		;
#endif

	return 0;
}

int usb_cpu_init_fail(void)
{
	return usb_cpu_stop();
}

#endif /* defined(CONFIG_USB_OHCI) && defined(CONFIG_SYS_USB_OHCI_CPU_INIT) */
