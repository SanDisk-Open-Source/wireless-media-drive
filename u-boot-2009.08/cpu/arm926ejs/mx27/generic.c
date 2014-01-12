/*
 *  Copyright (c) 2008 Eric Jarrige <eric.jarrige@armadeus.org>
 *  Copyright (c) 2009 Ilya Yanok <yanok@emcraft.com>
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
#include <div64.h>
#include <netdev.h>
#include <asm/io.h>
#include <asm/arch/imx-regs.h>
#ifdef CONFIG_MXC_MMC
#include <asm/arch/mxcmmc.h>
#endif

/*
 *  get the system pll clock in Hz
 *
 *                  mfi + mfn / (mfd +1)
 *  f = 2 * f_ref * --------------------
 *                        pd + 1
 */
unsigned int imx_decode_pll(unsigned int pll, unsigned int f_ref)
{
	unsigned int mfi = (pll >> 10) & 0xf;
	unsigned int mfn = pll & 0x3ff;
	unsigned int mfd = (pll >> 16) & 0x3ff;
	unsigned int pd =  (pll >> 26) & 0xf;

	mfi = mfi <= 5 ? 5 : mfi;

	return lldiv(2 * (u64)f_ref * (mfi * (mfd + 1) + mfn),
			(mfd + 1) * (pd + 1));
}

static ulong clk_in_32k(void)
{
	return 1024 * CONFIG_MX27_CLK32;
}

static ulong clk_in_26m(void)
{
	struct pll_regs *pll = (struct pll_regs *)IMX_PLL_BASE;

	if (readl(&pll->cscr) & CSCR_OSC26M_DIV1P5) {
		/* divide by 1.5 */
		return 26000000 * 2 / 3;
	} else {
		return 26000000;
	}
}

ulong imx_get_mpllclk(void)
{
	struct pll_regs *pll = (struct pll_regs *)IMX_PLL_BASE;
	ulong cscr = readl(&pll->cscr);
	ulong fref;

	if (cscr & CSCR_MCU_SEL)
		fref = clk_in_26m();
	else
		fref = clk_in_32k();

	return imx_decode_pll(readl(&pll->mpctl0), fref);
}

ulong imx_get_armclk(void)
{
	struct pll_regs *pll = (struct pll_regs *)IMX_PLL_BASE;
	ulong cscr = readl(&pll->cscr);
	ulong fref = imx_get_mpllclk();
	ulong div;

	if (!(cscr & CSCR_ARM_SRC_MPLL))
		fref = lldiv((fref * 2), 3);

	div = ((cscr >> 12) & 0x3) + 1;

	return lldiv(fref, div);
}

ulong imx_get_ahbclk(void)
{
	struct pll_regs *pll = (struct pll_regs *)IMX_PLL_BASE;
	ulong cscr = readl(&pll->cscr);
	ulong fref = imx_get_mpllclk();
	ulong div;

	div = ((cscr >> 8) & 0x3) + 1;

	return lldiv(fref * 2, 3 * div);
}

ulong imx_get_spllclk(void)
{
	struct pll_regs *pll = (struct pll_regs *)IMX_PLL_BASE;
	ulong cscr = readl(&pll->cscr);
	ulong fref;

	if (cscr & CSCR_SP_SEL)
		fref = clk_in_26m();
	else
		fref = clk_in_32k();

	return imx_decode_pll(readl(&pll->spctl0), fref);
}

static ulong imx_decode_perclk(ulong div)
{
	return lldiv((imx_get_mpllclk() * 2), (div * 3));
}

ulong imx_get_perclk1(void)
{
	struct pll_regs *pll = (struct pll_regs *)IMX_PLL_BASE;

	return imx_decode_perclk((readl(&pll->pcdr1) & 0x3f) + 1);
}

ulong imx_get_perclk2(void)
{
	struct pll_regs *pll = (struct pll_regs *)IMX_PLL_BASE;

	return imx_decode_perclk(((readl(&pll->pcdr1) >> 8) & 0x3f) + 1);
}

ulong imx_get_perclk3(void)
{
	struct pll_regs *pll = (struct pll_regs *)IMX_PLL_BASE;

	return imx_decode_perclk(((readl(&pll->pcdr1) >> 16) & 0x3f) + 1);
}

ulong imx_get_perclk4(void)
{
	struct pll_regs *pll = (struct pll_regs *)IMX_PLL_BASE;

	return imx_decode_perclk(((readl(&pll->pcdr1) >> 24) & 0x3f) + 1);
}

#if defined(CONFIG_DISPLAY_CPUINFO)
int print_cpuinfo (void)
{
	char buf[32];

	printf("CPU:   Freescale i.MX27 at %s MHz\n\n",
			strmhz(buf, imx_get_mpllclk()));
	return 0;
}
#endif

int cpu_eth_init(bd_t *bis)
{
#if defined(CONFIG_FEC_MXC)
	return fecmxc_initialize(bis);
#else
	return 0;
#endif
}

/*
 * Initializes on-chip MMC controllers.
 * to override, implement board_mmc_init()
 */
int cpu_mmc_init(bd_t *bis)
{
#ifdef CONFIG_MXC_MMC
	return mxc_mmc_init(bis);
#else
	return 0;
#endif
}

void imx_gpio_mode(int gpio_mode)
{
	struct gpio_regs *regs = (struct gpio_regs *)IMX_GPIO_BASE;
	unsigned int pin = gpio_mode & GPIO_PIN_MASK;
	unsigned int port = (gpio_mode & GPIO_PORT_MASK) >> GPIO_PORT_SHIFT;
	unsigned int ocr = (gpio_mode & GPIO_OCR_MASK) >> GPIO_OCR_SHIFT;
	unsigned int aout = (gpio_mode & GPIO_AOUT_MASK) >> GPIO_AOUT_SHIFT;
	unsigned int bout = (gpio_mode & GPIO_BOUT_MASK) >> GPIO_BOUT_SHIFT;
	unsigned int tmp;

	/* Pullup enable */
	if (gpio_mode & GPIO_PUEN) {
		writel(readl(&regs->port[port].puen) | (1 << pin),
				&regs->port[port].puen);
	} else {
		writel(readl(&regs->port[port].puen) & ~(1 << pin),
				&regs->port[port].puen);
	}

	/* Data direction */
	if (gpio_mode & GPIO_OUT) {
		writel(readl(&regs->port[port].ddir) | 1 << pin,
				&regs->port[port].ddir);
	} else {
		writel(readl(&regs->port[port].ddir) & ~(1 << pin),
				&regs->port[port].ddir);
	}

	/* Primary / alternate function */
	if (gpio_mode & GPIO_AF) {
		writel(readl(&regs->port[port].gpr) | (1 << pin),
				&regs->port[port].gpr);
	} else {
		writel(readl(&regs->port[port].gpr) & ~(1 << pin),
				&regs->port[port].gpr);
	}

	/* use as gpio? */
	if (!(gpio_mode & (GPIO_PF | GPIO_AF))) {
		writel(readl(&regs->port[port].gius) | (1 << pin),
				&regs->port[port].gius);
	} else {
		writel(readl(&regs->port[port].gius) & ~(1 << pin),
				&regs->port[port].gius);
	}

	/* Output / input configuration */
	if (pin < 16) {
		tmp = readl(&regs->port[port].ocr1);
		tmp &= ~(3 << (pin * 2));
		tmp |= (ocr << (pin * 2));
		writel(tmp, &regs->port[port].ocr1);

		writel(readl(&regs->port[port].iconfa1) & ~(3 << (pin * 2)),
				&regs->port[port].iconfa1);
		writel(readl(&regs->port[port].iconfa1) | aout << (pin * 2),
				&regs->port[port].iconfa1);
		writel(readl(&regs->port[port].iconfb1) & ~(3 << (pin * 2)),
				&regs->port[port].iconfb1);
		writel(readl(&regs->port[port].iconfb1) | bout << (pin * 2),
				&regs->port[port].iconfb1);
	} else {
		pin -= 16;

		tmp = readl(&regs->port[port].ocr2);
		tmp &= ~(3 << (pin * 2));
		tmp |= (ocr << (pin * 2));
		writel(tmp, &regs->port[port].ocr2);

		writel(readl(&regs->port[port].iconfa2) & ~(3 << (pin * 2)),
				&regs->port[port].iconfa2);
		writel(readl(&regs->port[port].iconfa2) | aout << (pin * 2),
				&regs->port[port].iconfa2);
		writel(readl(&regs->port[port].iconfb2) & ~(3 << (pin * 2)),
				&regs->port[port].iconfb2);
		writel(readl(&regs->port[port].iconfb2) | bout << (pin * 2),
				&regs->port[port].iconfb2);
	}
}
