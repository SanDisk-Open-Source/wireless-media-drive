/*
 * (C) Copyright 2003
 * Texas Instruments <www.ti.com>
 *
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Marius Groeger <mgroeger@sysgo.de>
 *
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Alex Zuepke <azu@sysgo.de>
 *
 * (C) Copyright 2002-2004
 * Gary Jennejohn, DENX Software Engineering, <garyj@denx.de>
 *
 * (C) Copyright 2004
 * Philippe Robin, ARM Ltd. <philippe.robin@arm.com>
 *
 * Copyright (C) 2007 Sergey Kubushyn <ksi@koi8.net>
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>

typedef volatile struct {
	u_int32_t	pid12;
	u_int32_t	emumgt;
	u_int32_t	na1;
	u_int32_t	na2;
	u_int32_t	tim12;
	u_int32_t	tim34;
	u_int32_t	prd12;
	u_int32_t	prd34;
	u_int32_t	tcr;
	u_int32_t	tgcr;
	u_int32_t	wdtcr;
} davinci_timer;

davinci_timer		*timer = (davinci_timer *)CONFIG_SYS_TIMERBASE;

#define TIMER_LOAD_VAL	(CONFIG_SYS_HZ_CLOCK / CONFIG_SYS_HZ)
#define TIM_CLK_DIV	16

static ulong timestamp;
static ulong lastinc;

int timer_init(void)
{
	/* We are using timer34 in unchained 32-bit mode, full speed */
	timer->tcr = 0x0;
	timer->tgcr = 0x0;
	timer->tgcr = 0x06 | ((TIM_CLK_DIV - 1) << 8);
	timer->tim34 = 0x0;
	timer->prd34 = TIMER_LOAD_VAL;
	lastinc = 0;
	timestamp = 0;
	timer->tcr = 2 << 22;

	return(0);
}

void reset_timer(void)
{
	timer->tcr = 0x0;
	timer->tim34 = 0;
	lastinc = 0;
	timestamp = 0;
	timer->tcr = 2 << 22;
}

static ulong get_timer_raw(void)
{
	ulong now = timer->tim34;

	if (now >= lastinc) {
		/* normal mode */
		timestamp += now - lastinc;
	} else {
		/* overflow ... */
		timestamp += now + TIMER_LOAD_VAL - lastinc;
	}
	lastinc = now;
	return timestamp;
}

ulong get_timer(ulong base)
{
	return((get_timer_raw() / (TIMER_LOAD_VAL / TIM_CLK_DIV)) - base);
}

void set_timer(ulong t)
{
	timestamp = t;
}

void udelay(unsigned long usec)
{
	ulong tmo;
	ulong endtime;
	signed long diff;

	tmo = CONFIG_SYS_HZ_CLOCK / 1000;
	tmo *= usec;
	tmo /= (1000 * TIM_CLK_DIV);

	endtime = get_timer_raw() + tmo;

	do {
		ulong now = get_timer_raw();
		diff = endtime - now;
	} while (diff >= 0);
}

/*
 * This function is derived from PowerPC code (read timebase as long long).
 * On ARM it just returns the timer value.
 */
unsigned long long get_ticks(void)
{
	return(get_timer(0));
}

/*
 * This function is derived from PowerPC code (timebase clock frequency).
 * On ARM it returns the number of timer ticks per second.
 */
ulong get_tbclk(void)
{
	return CONFIG_SYS_HZ;
}
