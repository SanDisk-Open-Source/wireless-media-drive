/*
 * Copyright (C) 2008-2011 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

#include <linux/kernel.h>
#include <linux/clk.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/pmic_external.h>
#include <asm/io.h>
#include <mach/hardware.h>
#include <mach/clock.h>
#include <mach/common.h>
#include <asm/proc-fns.h>
#include <asm/system.h>
#include "crm_regs.h"

/*!
 * @defgroup MSL_MX51 i.MX51 Machine Specific Layer (MSL)
 */

/*!
 * @file mach-mx51/system.c
 * @brief This file contains idle and reset functions.
 *
 * @ingroup MSL_MX51
 */

extern int mxc_jtag_enabled;
extern int iram_ready;
extern int dvfs_core_is_active;
extern void __iomem *ccm_base;
extern void __iomem *databahn_base;
extern int low_bus_freq_mode;
extern void (*wait_in_iram)(void *ccm_addr, void *databahn_addr,
							u32 sys_clk_count);
extern void mx50_wait(u32 ccm_base, u32 databahn_addr,
						u32 sys_clk_count);
extern void stop_dvfs(void);
extern void *wait_in_iram_base;
extern void __iomem *apll_base;
extern void __iomem *arm_plat_base;

static struct clk *gpc_dvfs_clk;
static struct regulator *vpll;
static struct clk *pll1_sw_clk;
static struct clk *osc;
static struct clk *pll1_main_clk;
static struct clk *ddr_clk ;
static struct clk *sys_clk ;
static int dvfs_core_paused;

/* set cpu low power mode before WFI instruction */
void mxc_cpu_lp_set(enum mxc_cpu_pwr_mode mode)
{
	u32 plat_lpc, arm_srpgcr, ccm_clpcr;
	u32 empgc0, empgc1;
	int stop_mode = 0;

	/* always allow platform to issue a deep sleep mode request */
	plat_lpc = __raw_readl(arm_plat_base + MXC_CORTEXA8_PLAT_LPC) &
	    ~(MXC_CORTEXA8_PLAT_LPC_DSM);
	ccm_clpcr = __raw_readl(MXC_CCM_CLPCR) & ~(MXC_CCM_CLPCR_LPM_MASK);
	arm_srpgcr = __raw_readl(MXC_SRPG_ARM_SRPGCR) & ~(MXC_SRPGCR_PCR);
	empgc0 = __raw_readl(MXC_SRPG_EMPGC0_SRPGCR) & ~(MXC_SRPGCR_PCR);
	empgc1 = __raw_readl(MXC_SRPG_EMPGC1_SRPGCR) & ~(MXC_SRPGCR_PCR);

	switch (mode) {
	case WAIT_CLOCKED:
		break;
	case WAIT_UNCLOCKED:
		ccm_clpcr |= (0x1 << MXC_CCM_CLPCR_LPM_OFFSET);
		break;
	case WAIT_UNCLOCKED_POWER_OFF:
	case STOP_POWER_OFF:
		plat_lpc |= MXC_CORTEXA8_PLAT_LPC_DSM
			    | MXC_CORTEXA8_PLAT_LPC_DBG_DSM;
		if (mode == WAIT_UNCLOCKED_POWER_OFF) {
			ccm_clpcr |= (0x1 << MXC_CCM_CLPCR_LPM_OFFSET);
			ccm_clpcr &= ~MXC_CCM_CLPCR_VSTBY;
			ccm_clpcr &= ~MXC_CCM_CLPCR_SBYOS;
			stop_mode = 0;
		} else {
			ccm_clpcr |= (0x2 << MXC_CCM_CLPCR_LPM_OFFSET);
			ccm_clpcr |= (0x3 << MXC_CCM_CLPCR_STBY_COUNT_OFFSET);
			ccm_clpcr |= MXC_CCM_CLPCR_VSTBY;
			ccm_clpcr |= MXC_CCM_CLPCR_SBYOS;
			stop_mode = 1;
		}

		arm_srpgcr |= MXC_SRPGCR_PCR;
		if (stop_mode) {
			empgc0 |= MXC_SRPGCR_PCR;
			empgc1 |= MXC_SRPGCR_PCR;
		}

		if (tzic_enable_wake(1) != 0)
			return;
		break;
	case STOP_POWER_ON:
		ccm_clpcr |= (0x2 << MXC_CCM_CLPCR_LPM_OFFSET);
		break;
	default:
		printk(KERN_WARNING "UNKNOWN cpu power mode: %d\n", mode);
		return;
	}

	__raw_writel(plat_lpc, arm_plat_base + MXC_CORTEXA8_PLAT_LPC);
	__raw_writel(ccm_clpcr, MXC_CCM_CLPCR);
	if (cpu_is_mx51() || (mx53_revision() >= IMX_CHIP_REVISION_2_0)
		|| (mx50_revision() >= IMX_CHIP_REVISION_1_1))
		__raw_writel(arm_srpgcr, MXC_SRPG_ARM_SRPGCR);
	/* Enable NEON SRPG for all but MX50TO1.0. */
	if (!(mx50_revision() == IMX_CHIP_REVISION_1_0))
		__raw_writel(arm_srpgcr, MXC_SRPG_NEON_SRPGCR);
	if (stop_mode) {
		__raw_writel(empgc0, MXC_SRPG_EMPGC0_SRPGCR);
		__raw_writel(empgc1, MXC_SRPG_EMPGC1_SRPGCR);
	}
}

void mxc_pg_enable(struct platform_device *pdev)
{
	if (pdev == NULL)
		return;

	if (strcmp(pdev->name, "mxc_ipu") == 0) {
		__raw_writel(MXC_PGCR_PCR, MXC_PGC_IPU_PGCR);
		__raw_writel(MXC_PGSR_PSR, MXC_PGC_IPU_PGSR);
	} else if (strcmp(pdev->name, "mxc_vpu") == 0) {
		__raw_writel(MXC_PGCR_PCR, MXC_PGC_VPU_PGCR);
		__raw_writel(MXC_PGSR_PSR, MXC_PGC_VPU_PGSR);
	}
}

EXPORT_SYMBOL(mxc_pg_enable);

void mxc_pg_disable(struct platform_device *pdev)
{
	if (pdev == NULL)
		return;

	if (strcmp(pdev->name, "mxc_ipu") == 0) {
		__raw_writel(0x0, MXC_PGC_IPU_PGCR);
		if (__raw_readl(MXC_PGC_IPU_PGSR) & MXC_PGSR_PSR)
			dev_dbg(&pdev->dev, "power gating successful\n");
		__raw_writel(MXC_PGSR_PSR, MXC_PGC_IPU_PGSR);
	} else if (strcmp(pdev->name, "mxc_vpu") == 0) {
		__raw_writel(0x0, MXC_PGC_VPU_PGCR);
		if (__raw_readl(MXC_PGC_VPU_PGSR) & MXC_PGSR_PSR)
			dev_dbg(&pdev->dev, "power gating successful\n");
		__raw_writel(MXC_PGSR_PSR, MXC_PGC_VPU_PGSR);
	}
}

EXPORT_SYMBOL(mxc_pg_disable);

/* To change the idle power mode, need to set arch_idle_mode to a different
 * power mode as in enum mxc_cpu_pwr_mode.
 * May allow dynamically changing the idle mode.
 */
static int arch_idle_mode = WAIT_UNCLOCKED_POWER_OFF;
/*!
 * This function puts the CPU into idle mode. It is called by default_idle()
 * in process.c file.
 */
void arch_idle(void)
{
	if (likely(!mxc_jtag_enabled)) {
		if (ddr_clk == NULL)
			ddr_clk = clk_get(NULL, "ddr_clk");
		if (gpc_dvfs_clk == NULL)
			gpc_dvfs_clk = clk_get(NULL, "gpc_dvfs_clk");
		/* gpc clock is needed for SRPG */
		clk_enable(gpc_dvfs_clk);
		mxc_cpu_lp_set(arch_idle_mode);

		if (cpu_is_mx50() && (clk_get_usecount(ddr_clk) == 0)) {
			if (sys_clk == NULL)
				sys_clk = clk_get(NULL, "sys_clk");

			memcpy(wait_in_iram_base, mx50_wait, SZ_4K);
			wait_in_iram = (void *)wait_in_iram_base;
			if (low_bus_freq_mode) {
				u32 reg, cpu_podf;

				reg = __raw_readl(apll_base + 0x50);
				reg = 0x120490;
				__raw_writel(reg, apll_base + 0x50);
				reg = __raw_readl(apll_base + 0x80);
				reg |= 1;
				__raw_writel(reg, apll_base + 0x80);

				/* Move ARM to be sourced from 24MHz XTAL.
				 * when ARM is in WFI.
				 */
				if (pll1_sw_clk == NULL)
					pll1_sw_clk = clk_get(NULL,
							"pll1_sw_clk");
				if (osc == NULL)
					osc = clk_get(NULL, "lp_apm");
				if (pll1_main_clk == NULL)
					pll1_main_clk = clk_get(NULL,
							"pll1_main_clk");

				clk_set_parent(pll1_sw_clk, osc);
				/* Set the ARM-PODF divider to 1. */
				cpu_podf = __raw_readl(MXC_CCM_CACRR);
				__raw_writel(0x01, MXC_CCM_CACRR);

				wait_in_iram(ccm_base, databahn_base,
					clk_get_usecount(sys_clk));

				/* Set the ARM-POD divider back
				 * to the original.
				 */
				__raw_writel(cpu_podf, MXC_CCM_CACRR);
				clk_set_parent(pll1_sw_clk, pll1_main_clk);
			} else
				wait_in_iram(ccm_base, databahn_base,
					clk_get_usecount(sys_clk));
		} else
			cpu_do_idle();
		clk_disable(gpc_dvfs_clk);
		clk_put(ddr_clk);
	}
}

static int __mxs_reset_block(void __iomem *hwreg, int just_enable)
{
	u32 c;
	int timeout;

	/* the process of software reset of IP block is done
	   in several steps:

	   - clear SFTRST and wait for block is enabled;
	   - clear clock gating (CLKGATE bit);
	   - set the SFTRST again and wait for block is in reset;
	   - clear SFTRST and wait for reset completion.
	 */
	c = __raw_readl(hwreg);
	c &= ~(1 << 31);	/* clear SFTRST */
	__raw_writel(c, hwreg);
	for (timeout = 1000000; timeout > 0; timeout--)
		/* still in SFTRST state ? */
		if ((__raw_readl(hwreg) & (1 << 31)) == 0)
			break;
	if (timeout <= 0) {
		printk(KERN_ERR "%s(%p): timeout when enabling\n",
		       __func__, hwreg);
		return -ETIME;
	}

	c = __raw_readl(hwreg);
	c &= ~(1 << 30);	/* clear CLKGATE */
	__raw_writel(c, hwreg);

	if (!just_enable) {
		c = __raw_readl(hwreg);
		c |= (1 << 31);	/* now again set SFTRST */
		__raw_writel(c, hwreg);
		for (timeout = 1000000; timeout > 0; timeout--)
			/* poll until CLKGATE set */
			if (__raw_readl(hwreg) & (1 << 30))
				break;
		if (timeout <= 0) {
			printk(KERN_ERR "%s(%p): timeout when resetting\n",
			       __func__, hwreg);
			return -ETIME;
		}

		c = __raw_readl(hwreg);
		c &= ~(1 << 31);	/* clear SFTRST */
		__raw_writel(c, hwreg);
		for (timeout = 1000000; timeout > 0; timeout--)
			/* still in SFTRST state ? */
			if ((__raw_readl(hwreg) & (1 << 31)) == 0)
				break;
		if (timeout <= 0) {
			printk(KERN_ERR "%s(%p): timeout when enabling "
			       "after reset\n", __func__, hwreg);
			return -ETIME;
		}

		c = __raw_readl(hwreg);
		c &= ~(1 << 30);	/* clear CLKGATE */
		__raw_writel(c, hwreg);
	}
	for (timeout = 1000000; timeout > 0; timeout--)
		/* still in SFTRST state ? */
		if ((__raw_readl(hwreg) & (1 << 30)) == 0)
			break;

	if (timeout <= 0) {
		printk(KERN_ERR "%s(%p): timeout when unclockgating\n",
		       __func__, hwreg);
		return -ETIME;
	}

	return 0;
}

int mxs_reset_block(void __iomem *hwreg, int just_enable)
{
	int try = 10;
	int r;

	while (try--) {
		r = __mxs_reset_block(hwreg, just_enable);
		if (!r)
			break;
		pr_debug("%s: try %d failed\n", __func__, 10 - try);
	}
	return r;
}

