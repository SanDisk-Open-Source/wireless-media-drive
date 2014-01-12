/*
 * Copyright (C) 2007, Guennadi Liakhovetski <lg@denx.de>
 *
 * (C) Copyright 2009-2011 Freescale Semiconductor, Inc.
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
#include <asm/arch/mx50.h>
#include <asm/arch/mx50_pins.h>
#include <asm/arch/iomux.h>
#include <asm/errno.h>

#ifdef CONFIG_IMX_CSPI
#include <imx_spi.h>
#include <asm/arch/imx_spi_pmic.h>
#endif

#if CONFIG_I2C_MXC
#include <i2c.h>
#endif

#ifdef CONFIG_CMD_MMC
#include <mmc.h>
#include <fsl_esdhc.h>
#endif

#ifdef CONFIG_ARCH_MMU
#include <asm/mmu.h>
#include <asm/arch/mmu.h>
#endif

#ifdef CONFIG_CMD_CLOCK
#include <asm/clock.h>
#endif

#ifdef CONFIG_MXC_EPDC
#include <lcd.h>
#endif

#ifdef CONFIG_ANDROID_RECOVERY
#include "../common/recovery.h"
#include <part.h>
#include <ext2fs.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <ubi_uboot.h>
#include <jffs2/load_kernel.h>
#endif

DECLARE_GLOBAL_DATA_PTR;

static u32 system_rev;
static enum boot_device boot_dev;

#ifdef  QSI_PWRST_ALIGN
int qwifi_booting_st;
int bat_cap_st;

#define BATCAP_UNKNOWN  0
#define BATCAP_LOW      1
#define BATCAP_NORMAL   2
#define BATCAP_FULL     3
#endif

static inline void setup_boot_device(void)
{
	uint soc_sbmr = readl(SRC_BASE_ADDR + 0x4);
	uint bt_mem_ctl = (soc_sbmr & 0x000000FF) >> 4 ;
	uint bt_mem_type = (soc_sbmr & 0x00000008) >> 3;

	switch (bt_mem_ctl) {
	case 0x0:
		if (bt_mem_type)
			boot_dev = ONE_NAND_BOOT;
		else
			boot_dev = WEIM_NOR_BOOT;
		break;
	case 0x2:
		if (bt_mem_type)
			boot_dev = SATA_BOOT;
		else
			boot_dev = PATA_BOOT;
		break;
	case 0x3:
		if (bt_mem_type)
			boot_dev = SPI_NOR_BOOT;
		else
			boot_dev = I2C_BOOT;
		break;
	case 0x4:
	case 0x5:
		boot_dev = SD_BOOT;
		break;
	case 0x6:
	case 0x7:
		boot_dev = MMC_BOOT;
		break;
	case 0x8 ... 0xf:
		boot_dev = NAND_BOOT;
		break;
	default:
		boot_dev = UNKNOWN_BOOT;
		break;
	}
}

enum boot_device get_boot_device(void)
{
	return boot_dev;
}

u32 get_board_rev(void)
{
	return system_rev;
}

static inline void setup_soc_rev(void)
{
	int reg = __REG(ROM_SI_REV);

	switch (reg) {
	case 0x10:
		system_rev = 0x50000 | CHIP_REV_1_0;
		break;
	case 0x11:
		system_rev = 0x50000 | CHIP_REV_1_1_1;
		break;
	default:
		system_rev = 0x50000 | CHIP_REV_1_1_1;
	}
}

static inline void set_board_rev(int rev)
{
	system_rev |= (rev & 0xF) << 8;
}

static inline void setup_board_rev(void)
{
#if defined(CONFIG_MX50_RD3)
	set_board_rev(0x3);
#endif
}

static inline void setup_arch_id(void)
{
#if defined(CONFIG_MX50_RDP) || defined(CONFIG_MX50_RD3) || defined(CONFIG_QSI_NIMBUS)
	gd->bd->bi_arch_number = MACH_TYPE_MX50_RDP;
#elif defined(CONFIG_MX50_ARM2)
	gd->bd->bi_arch_number = MACH_TYPE_MX50_ARM2;
#else
#	error "Unsupported board!"
#endif
}

inline int is_soc_rev(int rev)
{
	return (system_rev & 0xFF) - rev;
}

#ifdef CONFIG_ARCH_MMU
void board_mmu_init(void)
{
	unsigned long ttb_base = PHYS_SDRAM_1 + 0x4000;
	unsigned long i;

	/*
	* Set the TTB register
	*/
	asm volatile ("mcr  p15,0,%0,c2,c0,0" : : "r"(ttb_base) /*:*/);

	/*
	* Set the Domain Access Control Register
	*/
	i = ARM_ACCESS_DACR_DEFAULT;
	asm volatile ("mcr  p15,0,%0,c3,c0,0" : : "r"(i) /*:*/);

	/*
	* First clear all TT entries - ie Set them to Faulting
	*/
	memset((void *)ttb_base, 0, ARM_FIRST_LEVEL_PAGE_TABLE_SIZE);
	/* Actual   Virtual  Size   Attributes          Function */
	/* Base     Base     MB     cached? buffered?  access permissions */
	/* xxx00000 xxx00000 */
	X_ARM_MMU_SECTION(0x000, 0x000, 0x10,
			ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* ROM, 16M */
	X_ARM_MMU_SECTION(0x070, 0x070, 0x010,
			ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* IRAM */
	X_ARM_MMU_SECTION(0x100, 0x100, 0x040,
			ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* SATA */
	X_ARM_MMU_SECTION(0x180, 0x180, 0x100,
			ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* IPUv3M */
	X_ARM_MMU_SECTION(0x200, 0x200, 0x200,
			ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* GPU */
	X_ARM_MMU_SECTION(0x400, 0x400, 0x300,
			ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* periperals */
	X_ARM_MMU_SECTION(0x700, 0x700, 0x400,
			ARM_CACHEABLE, ARM_BUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* CSD0 1G */
	X_ARM_MMU_SECTION(0x700, 0xB00, 0x400,
			ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* CSD0 1G */
	X_ARM_MMU_SECTION(0xF00, 0xF00, 0x100,
			ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* CS1 EIM control*/
	X_ARM_MMU_SECTION(0xF80, 0xF80, 0x001,
			ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* iRam */

	/* Workaround for arm errata #709718 */
	/* Setup PRRR so device is always mapped to non-shared */
	asm volatile ("mrc p15, 0, %0, c10, c2, 0" : "=r"(i) : /*:*/);
	i &= (~(3 << 0x10));
	asm volatile ("mcr p15, 0, %0, c10, c2, 0" : : "r"(i) /*:*/);

	/* Enable MMU */
	MMU_ON();
}
#endif

int dram_init(void)
{
	gd->bd->bi_dram[0].start = PHYS_SDRAM_1;
	gd->bd->bi_dram[0].size = PHYS_SDRAM_1_SIZE;
	return 0;
}

static void setup_uart(void)
{
	unsigned int reg;

#if defined(CONFIG_MX50_RD3) || defined(CONFIG_QSI_NIMBUS)
	/* UART3 TXD */
	mxc_request_iomux(MX50_PIN_UART3_TXD, IOMUX_CONFIG_ALT1);
	mxc_iomux_set_pad(MX50_PIN_UART3_TXD, 0x1E4);
	/* Enable UART1 */
	reg = readl(GPIO6_BASE_ADDR + 0x0);
	reg |= (1 << 14);
	writel(reg, GPIO6_BASE_ADDR + 0x0);

	reg = readl(GPIO6_BASE_ADDR + 0x4);
	reg |= (1 << 14);
	writel(reg, GPIO6_BASE_ADDR + 0x4);
#endif
	/* UART1 RXD */
	mxc_request_iomux(MX50_PIN_UART1_RXD, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX50_PIN_UART1_RXD, 0x1E4);
	mxc_iomux_set_input(MUX_IN_UART1_IPP_UART_RXD_MUX_SELECT_INPUT, 0x1);

	/* UART1 TXD */
	mxc_request_iomux(MX50_PIN_UART1_TXD, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX50_PIN_UART1_TXD, 0x1E4);

#ifdef CONFIG_QSI_NIMBUS
    /* VCC_SD2: gpio6-9 set to 1 to enable */
	mxc_request_iomux(MX50_PIN_UART1_RTS, IOMUX_CONFIG_ALT1);

	reg = readl(GPIO6_BASE_ADDR + 0x0);
	reg |= (1 << 9);
	writel(reg, GPIO6_BASE_ADDR + 0x0);

	reg = readl(GPIO6_BASE_ADDR + 0x4);
	reg |= (1 << 9);
	writel(reg, GPIO6_BASE_ADDR + 0x4);
#endif
}

#ifdef CONFIG_I2C_MXC
static void setup_i2c(unsigned int module_base)
{
	switch (module_base) {
	case I2C1_BASE_ADDR:
		/* i2c1 SDA */
		mxc_request_iomux(MX50_PIN_I2C1_SDA,
				IOMUX_CONFIG_ALT0 | IOMUX_CONFIG_SION);
		mxc_iomux_set_pad(MX50_PIN_I2C1_SDA, PAD_CTL_SRE_FAST |
				PAD_CTL_ODE_OPENDRAIN_ENABLE |
				PAD_CTL_DRV_HIGH | PAD_CTL_100K_PU |
				PAD_CTL_HYS_ENABLE);
		/* i2c1 SCL */
		mxc_request_iomux(MX50_PIN_I2C1_SCL,
				IOMUX_CONFIG_ALT0 | IOMUX_CONFIG_SION);
		mxc_iomux_set_pad(MX50_PIN_I2C1_SCL, PAD_CTL_SRE_FAST |
				PAD_CTL_ODE_OPENDRAIN_ENABLE |
				PAD_CTL_DRV_HIGH | PAD_CTL_100K_PU |
				PAD_CTL_HYS_ENABLE);
		break;
	case I2C2_BASE_ADDR:
		/* i2c2 SDA */
		mxc_request_iomux(MX50_PIN_I2C2_SDA,
				IOMUX_CONFIG_ALT0 | IOMUX_CONFIG_SION);
		mxc_iomux_set_pad(MX50_PIN_I2C2_SDA,
				PAD_CTL_SRE_FAST |
				PAD_CTL_ODE_OPENDRAIN_ENABLE |
				PAD_CTL_DRV_HIGH | PAD_CTL_100K_PU |
				PAD_CTL_HYS_ENABLE);

		/* i2c2 SCL */
		mxc_request_iomux(MX50_PIN_I2C2_SCL,
				IOMUX_CONFIG_ALT0 | IOMUX_CONFIG_SION);
		mxc_iomux_set_pad(MX50_PIN_I2C2_SCL,
				PAD_CTL_SRE_FAST |
				PAD_CTL_ODE_OPENDRAIN_ENABLE |
				PAD_CTL_DRV_HIGH | PAD_CTL_100K_PU |
				PAD_CTL_HYS_ENABLE);
		break;
	case I2C3_BASE_ADDR:
		/* i2c3 SDA */
		mxc_request_iomux(MX50_PIN_I2C3_SDA,
				IOMUX_CONFIG_ALT0 | IOMUX_CONFIG_SION);
		mxc_iomux_set_pad(MX50_PIN_I2C3_SDA,
				PAD_CTL_SRE_FAST |
				PAD_CTL_ODE_OPENDRAIN_ENABLE |
				PAD_CTL_DRV_HIGH | PAD_CTL_100K_PU |
				PAD_CTL_HYS_ENABLE);

		/* i2c3 SCL */
		mxc_request_iomux(MX50_PIN_I2C3_SCL,
				IOMUX_CONFIG_ALT0 | IOMUX_CONFIG_SION);
		mxc_iomux_set_pad(MX50_PIN_I2C3_SCL,
				PAD_CTL_SRE_FAST |
				PAD_CTL_ODE_OPENDRAIN_ENABLE |
				PAD_CTL_DRV_HIGH | PAD_CTL_100K_PU |
				PAD_CTL_HYS_ENABLE);
		break;
	default:
		printf("Invalid I2C base: 0x%x\n", module_base);
		break;
	}
}

#endif

#ifdef CONFIG_IMX_CSPI
s32 spi_get_cfg(struct imx_spi_dev_t *dev)
{
	switch (dev->slave.cs) {
	case 0:
		/* PMIC */
		dev->base = CSPI3_BASE_ADDR;

#if 0 //ndef QSI_NIMBUS_PMIC_SPI_PATCH_DEF  ,johnson 20121109     
		dev->freq = 25000000; 
#else
        dev->freq = 10000000;
#endif
		dev->ss_pol = IMX_SPI_ACTIVE_HIGH;
		dev->ss = 0;
		dev->fifo_sz = 32;
		dev->us_delay = 0;
		break;
	case 1:
		/* SPI-NOR */
		dev->base = CSPI3_BASE_ADDR;
#if 0 //ndef QSI_NIMBUS_PMIC_SPI_PATCH_DEF  , johnson 20121109    
		dev->freq = 25000000;
#else
        dev->freq = 10000000;
#endif
		dev->ss_pol = IMX_SPI_ACTIVE_LOW;
		dev->ss = 1;
		dev->fifo_sz = 32;
		dev->us_delay = 0;
		break;
	default:
		printf("Invalid Bus ID!\n");
	}

	return 0;
}

void spi_io_init(struct imx_spi_dev_t *dev)
{
	switch (dev->base) {
	case CSPI3_BASE_ADDR:
		mxc_request_iomux(MX50_PIN_CSPI_MOSI, IOMUX_CONFIG_ALT0);
		mxc_iomux_set_pad(MX50_PIN_CSPI_MOSI, 0x4);

		mxc_request_iomux(MX50_PIN_CSPI_MISO, IOMUX_CONFIG_ALT0);
		mxc_iomux_set_pad(MX50_PIN_CSPI_MISO, 0x4);

		if (dev->ss == 0) {
			/* de-select SS1 of instance: cspi */
			mxc_request_iomux(MX50_PIN_ECSPI1_MOSI,
						IOMUX_CONFIG_ALT1);

			mxc_request_iomux(MX50_PIN_CSPI_SS0, IOMUX_CONFIG_ALT0);
			mxc_iomux_set_pad(MX50_PIN_CSPI_SS0, 0xE4);
		} else if (dev->ss == 1) {
			/* de-select SS0 of instance: cspi */
			mxc_request_iomux(MX50_PIN_CSPI_SS0, IOMUX_CONFIG_ALT1);

			mxc_request_iomux(MX50_PIN_ECSPI1_MOSI,
						IOMUX_CONFIG_ALT2);
			mxc_iomux_set_pad(MX50_PIN_ECSPI1_MOSI, 0xE4);
			mxc_iomux_set_input(
			MUX_IN_CSPI_IPP_IND_SS1_B_SELECT_INPUT, 0x1);
		}

		mxc_request_iomux(MX50_PIN_CSPI_SCLK, IOMUX_CONFIG_ALT0);
		mxc_iomux_set_pad(MX50_PIN_CSPI_SCLK, 0x4);
		break;
	case CSPI2_BASE_ADDR:
	case CSPI1_BASE_ADDR:
		/* ecspi1-2 fall through */
		break;
	default:
		break;
	}
}
#endif

#ifdef CONFIG_NAND_GPMI
void setup_gpmi_nand(void)
{
	u32 src_sbmr = readl(SRC_BASE_ADDR + 0x4);

	/* Fix for gpmi gatelevel issue */
	mxc_iomux_set_pad(MX50_PIN_SD3_CLK, 0x00e4);

	/* RESETN,WRN,RDN,DATA0~7 Signals iomux*/
	/* Check if 1.8v NAND is to be supported */
	if ((src_sbmr & 0x00000004) >> 2)
		*(u32 *)(IOMUXC_BASE_ADDR + PAD_GRP_START + 0x58) = (0x1 << 13);

	/* RESETN */
	mxc_request_iomux(MX50_PIN_SD3_WP, IOMUX_CONFIG_ALT2);
	mxc_iomux_set_pad(MX50_PIN_SD3_WP, PAD_CTL_DRV_HIGH);

	/* WRN */
	mxc_request_iomux(MX50_PIN_SD3_CMD, IOMUX_CONFIG_ALT2);
	mxc_iomux_set_pad(MX50_PIN_SD3_CMD, PAD_CTL_DRV_HIGH);

	/* RDN */
	mxc_request_iomux(MX50_PIN_SD3_CLK, IOMUX_CONFIG_ALT2);
	mxc_iomux_set_pad(MX50_PIN_SD3_CLK, PAD_CTL_DRV_HIGH);

	/* D0 */
	mxc_request_iomux(MX50_PIN_SD3_D4, IOMUX_CONFIG_ALT2);
	mxc_iomux_set_pad(MX50_PIN_SD3_D4, PAD_CTL_DRV_HIGH);

	/* D1 */
	mxc_request_iomux(MX50_PIN_SD3_D5, IOMUX_CONFIG_ALT2);
	mxc_iomux_set_pad(MX50_PIN_SD3_D5, PAD_CTL_DRV_HIGH);

	/* D2 */
	mxc_request_iomux(MX50_PIN_SD3_D6, IOMUX_CONFIG_ALT2);
	mxc_iomux_set_pad(MX50_PIN_SD3_D6, PAD_CTL_DRV_HIGH);

	/* D3 */
	mxc_request_iomux(MX50_PIN_SD3_D7, IOMUX_CONFIG_ALT2);
	mxc_iomux_set_pad(MX50_PIN_SD3_D7, PAD_CTL_DRV_HIGH);

	/* D4 */
	mxc_request_iomux(MX50_PIN_SD3_D0, IOMUX_CONFIG_ALT2);
	mxc_iomux_set_pad(MX50_PIN_SD3_D0, PAD_CTL_DRV_HIGH);

	/* D5 */
	mxc_request_iomux(MX50_PIN_SD3_D1, IOMUX_CONFIG_ALT2);
	mxc_iomux_set_pad(MX50_PIN_SD3_D1, PAD_CTL_DRV_HIGH);

	/* D6 */
	mxc_request_iomux(MX50_PIN_SD3_D2, IOMUX_CONFIG_ALT2);
	mxc_iomux_set_pad(MX50_PIN_SD3_D2, PAD_CTL_DRV_HIGH);

	/* D7 */
	mxc_request_iomux(MX50_PIN_SD3_D3, IOMUX_CONFIG_ALT2);
	mxc_iomux_set_pad(MX50_PIN_SD3_D3, PAD_CTL_DRV_HIGH);

	/*CE0~3,and other four controls signals muxed on KPP*/
	switch ((src_sbmr & 0x00000018) >> 3) {
	case  0:
		/* Muxed on key */
		if ((src_sbmr & 0x00000004) >> 2)
			*(u32 *)(IOMUXC_BASE_ADDR + PAD_GRP_START + 0x20) =
								(0x1 << 13);

		/* CLE */
		mxc_request_iomux(MX50_PIN_KEY_COL0, IOMUX_CONFIG_ALT2);
		mxc_iomux_set_pad(MX50_PIN_KEY_COL0, PAD_CTL_DRV_HIGH);

		/* ALE */
		mxc_request_iomux(MX50_PIN_KEY_ROW0, IOMUX_CONFIG_ALT2);
		mxc_iomux_set_pad(MX50_PIN_KEY_ROW0, PAD_CTL_DRV_HIGH);

		/* READY0 */
		mxc_request_iomux(MX50_PIN_KEY_COL3, IOMUX_CONFIG_ALT2);
		mxc_iomux_set_pad(MX50_PIN_KEY_COL3,
				PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_PULL |
				PAD_CTL_100K_PU);
		mxc_iomux_set_input(
			MUX_IN_RAWNAND_U_GPMI_INPUT_GPMI_RDY0_IN_SELECT_INPUT,
			INPUT_CTL_PATH0);

		/* DQS */
		mxc_request_iomux(MX50_PIN_KEY_ROW3, IOMUX_CONFIG_ALT2);
		mxc_iomux_set_pad(MX50_PIN_KEY_ROW3, PAD_CTL_DRV_HIGH);
		mxc_iomux_set_input(
			MUX_IN_RAWNAND_U_GPMI_INPUT_GPMI_DQS_IN_SELECT_INPUT,
			INPUT_CTL_PATH0);

		/* CE0 */
		mxc_request_iomux(MX50_PIN_KEY_COL1, IOMUX_CONFIG_ALT2);
		mxc_iomux_set_pad(MX50_PIN_KEY_COL1, PAD_CTL_DRV_HIGH);

		/* CE1 */
		mxc_request_iomux(MX50_PIN_KEY_ROW1, IOMUX_CONFIG_ALT2);
		mxc_iomux_set_pad(MX50_PIN_KEY_ROW1, PAD_CTL_DRV_HIGH);

		/* CE2 */
		mxc_request_iomux(MX50_PIN_KEY_COL2, IOMUX_CONFIG_ALT2);
		mxc_iomux_set_pad(MX50_PIN_KEY_COL2, PAD_CTL_DRV_HIGH);

		/* CE3 */
		mxc_request_iomux(MX50_PIN_KEY_ROW2, IOMUX_CONFIG_ALT2);
		mxc_iomux_set_pad(MX50_PIN_KEY_ROW2, PAD_CTL_DRV_HIGH);

		break;
	case 1:
		if ((src_sbmr & 0x00000004) >> 2)
			*(u32 *)(IOMUXC_BASE_ADDR + PAD_GRP_START + 0xc) =
								(0x1 << 13);

		/* CLE */
		mxc_request_iomux(MX50_PIN_EIM_DA8, IOMUX_CONFIG_ALT2);
		mxc_iomux_set_pad(MX50_PIN_EIM_DA8, PAD_CTL_DRV_HIGH);

		/* ALE */
		mxc_request_iomux(MX50_PIN_EIM_DA9, IOMUX_CONFIG_ALT2);
		mxc_iomux_set_pad(MX50_PIN_EIM_DA9, PAD_CTL_DRV_HIGH);

		/* READY0 */
		mxc_request_iomux(MX50_PIN_EIM_DA14, IOMUX_CONFIG_ALT2);
		mxc_iomux_set_pad(MX50_PIN_EIM_DA14,
				PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_PULL |
				PAD_CTL_100K_PU);
		mxc_iomux_set_input(
			MUX_IN_RAWNAND_U_GPMI_INPUT_GPMI_RDY0_IN_SELECT_INPUT,
			INPUT_CTL_PATH2);

		/* DQS */
		mxc_request_iomux(MX50_PIN_EIM_DA15, IOMUX_CONFIG_ALT2);
		mxc_iomux_set_pad(MX50_PIN_EIM_DA15, PAD_CTL_DRV_HIGH);
		mxc_iomux_set_input(
			MUX_IN_RAWNAND_U_GPMI_INPUT_GPMI_DQS_IN_SELECT_INPUT,
			INPUT_CTL_PATH2);

		/* CE0 */
		mxc_request_iomux(MX50_PIN_EIM_DA10, IOMUX_CONFIG_ALT2);
		mxc_iomux_set_pad(MX50_PIN_EIM_DA10, PAD_CTL_DRV_HIGH);

		/* CE1 */
		mxc_request_iomux(MX50_PIN_EIM_DA11, IOMUX_CONFIG_ALT2);
		mxc_iomux_set_pad(MX50_PIN_EIM_DA11, PAD_CTL_DRV_HIGH);

		/* CE2 */
		mxc_request_iomux(MX50_PIN_EIM_DA12, IOMUX_CONFIG_ALT2);
		mxc_iomux_set_pad(MX50_PIN_EIM_DA12, PAD_CTL_DRV_HIGH);

		/* CE3 */
		mxc_request_iomux(MX50_PIN_EIM_DA13, IOMUX_CONFIG_ALT2);
		mxc_iomux_set_pad(MX50_PIN_EIM_DA13, PAD_CTL_DRV_HIGH);

		break;
	case 2:
		if ((src_sbmr & 0x00000004) >> 2)
			*(u32 *)(IOMUXC_BASE_ADDR + PAD_GRP_START + 0x48) =
								(0x1 << 13);

		/* CLE */
		mxc_request_iomux(MX50_PIN_DISP_D8, IOMUX_CONFIG_ALT2);
		mxc_iomux_set_pad(MX50_PIN_DISP_D8, PAD_CTL_DRV_HIGH);

		/* ALE */
		mxc_request_iomux(MX50_PIN_DISP_D9, IOMUX_CONFIG_ALT2);
		mxc_iomux_set_pad(MX50_PIN_DISP_D9, PAD_CTL_DRV_HIGH);

		/* READY0 */
		mxc_request_iomux(MX50_PIN_DISP_D14, IOMUX_CONFIG_ALT2);
		mxc_iomux_set_pad(MX50_PIN_DISP_D14,
				PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_PULL |
				PAD_CTL_100K_PU);
		mxc_iomux_set_input(
			MUX_IN_RAWNAND_U_GPMI_INPUT_GPMI_RDY0_IN_SELECT_INPUT,
			INPUT_CTL_PATH1);

		/* DQS */
		mxc_request_iomux(MX50_PIN_DISP_D15, IOMUX_CONFIG_ALT2);
		mxc_iomux_set_pad(MX50_PIN_DISP_D15, PAD_CTL_DRV_HIGH);
		mxc_iomux_set_input(
			MUX_IN_RAWNAND_U_GPMI_INPUT_GPMI_DQS_IN_SELECT_INPUT,
			INPUT_CTL_PATH1);

		/* CE0 */
		mxc_request_iomux(MX50_PIN_DISP_D10, IOMUX_CONFIG_ALT2);
		mxc_iomux_set_pad(MX50_PIN_DISP_D10, PAD_CTL_DRV_HIGH);

		/* CE1 */
		mxc_request_iomux(MX50_PIN_EIM_DA11, IOMUX_CONFIG_ALT2);
		mxc_iomux_set_pad(MX50_PIN_EIM_DA11, PAD_CTL_DRV_HIGH);

		/* CE2 */
		mxc_request_iomux(MX50_PIN_DISP_D12, IOMUX_CONFIG_ALT2);
		mxc_iomux_set_pad(MX50_PIN_DISP_D12, PAD_CTL_DRV_HIGH);

		/* CE3 */
		mxc_request_iomux(MX50_PIN_DISP_D13, IOMUX_CONFIG_ALT2);
		mxc_iomux_set_pad(MX50_PIN_DISP_D13, PAD_CTL_DRV_HIGH);

		break;
	default:
		break;
	}
}
#endif

#ifdef CONFIG_MXC_FEC

#ifdef CONFIG_GET_FEC_MAC_ADDR_FROM_IIM

#define HW_OCOTP_MACn(n)	(0x00000250 + (n) * 0x10)

int fec_get_mac_addr(unsigned char *mac)
{
	u32 *ocotp_mac_base =
		(u32 *)(OCOTP_CTRL_BASE_ADDR + HW_OCOTP_MACn(0));
	int i;

	for (i = 0; i < 6; ++i, ++ocotp_mac_base)
		mac[6 - 1 - i] = readl(++ocotp_mac_base);

	return 0;
}
#endif

static void setup_fec(void)
{
	volatile unsigned int reg;

#if defined(CONFIG_MX50_RDP)
	/* FEC_EN: gpio6-23 set to 0 to enable FEC */
	mxc_request_iomux(MX50_PIN_I2C3_SDA, IOMUX_CONFIG_ALT1);

	reg = readl(GPIO6_BASE_ADDR + 0x0);
	reg &= ~(1 << 23);
	writel(reg, GPIO6_BASE_ADDR + 0x0);

	reg = readl(GPIO6_BASE_ADDR + 0x4);
	reg |= (1 << 23);
	writel(reg, GPIO6_BASE_ADDR + 0x4);

#elif defined(CONFIG_MX50_RD3) || defined(CONFIG_QSI_NIMBUS)
	/* FEC_EN: gpio4-15 set to 1 to enable FEC */
	mxc_request_iomux(MX50_PIN_ECSPI1_SS0, IOMUX_CONFIG_ALT1);

	reg = readl(GPIO4_BASE_ADDR + 0x0);
	reg |= (1 << 15);
	writel(reg, GPIO4_BASE_ADDR + 0x0);

	reg = readl(GPIO4_BASE_ADDR + 0x4);
	reg |= (1 << 15);
	writel(reg, GPIO4_BASE_ADDR + 0x4);

	/* DCDC_PWREN(GP4_16) set to 0 to enable DCDC_3V15 */
	mxc_request_iomux(MX50_PIN_ECSPI2_SCLK, IOMUX_CONFIG_ALT1);
	reg = readl(GPIO4_BASE_ADDR + 0x0);
    #ifdef CONFIG_QSI_NIMBUS
    /* This is patched for EVB to enable DCDC_3V15. The GP4_16 need to set 1. */
    reg |= (1 << 16);
    #else
	reg &= ~(1 << 16);
    #endif
	writel(reg, GPIO4_BASE_ADDR + 0x0);

	reg = readl(GPIO4_BASE_ADDR + 0x4);
	reg |= (1 << 16);
	writel(reg, GPIO4_BASE_ADDR + 0x4);

	/* Isolate EIM signals and boot configuration signals. - GPIO6_11 to 1*/
	mxc_request_iomux(MX50_PIN_UART2_RXD, IOMUX_CONFIG_ALT1);
	reg = readl(GPIO6_BASE_ADDR + 0x0);
	reg |= (1 << 11);
	writel(reg, GPIO6_BASE_ADDR + 0x0);

	reg = readl(GPIO6_BASE_ADDR + 0x4);
	reg |= (1 << 11);
	writel(reg, GPIO6_BASE_ADDR + 0x4);
#endif

	/*FEC_MDIO*/
	mxc_request_iomux(MX50_PIN_SSI_RXC, IOMUX_CONFIG_ALT6);
	mxc_iomux_set_pad(MX50_PIN_SSI_RXC, 0xC);
	mxc_iomux_set_input(MUX_IN_FEC_FEC_MDI_SELECT_INPUT, 0x1);

	/*FEC_MDC*/
	mxc_request_iomux(MX50_PIN_SSI_RXFS, IOMUX_CONFIG_ALT6);
	mxc_iomux_set_pad(MX50_PIN_SSI_RXFS, 0x004);

	/* FEC RXD1 */
	mxc_request_iomux(MX50_PIN_DISP_D3, IOMUX_CONFIG_ALT2);
	mxc_iomux_set_pad(MX50_PIN_DISP_D3, 0x0);
	mxc_iomux_set_input(MUX_IN_FEC_FEC_RDATA_1_SELECT_INPUT, 0x0);

	/* FEC RXD0 */
	mxc_request_iomux(MX50_PIN_DISP_D4, IOMUX_CONFIG_ALT2);
	mxc_iomux_set_pad(MX50_PIN_DISP_D4, 0x0);
	mxc_iomux_set_input(MUX_IN_FEC_FEC_RDATA_0_SELECT_INPUT, 0x0);

	 /* FEC TXD1 */
	mxc_request_iomux(MX50_PIN_DISP_D6, IOMUX_CONFIG_ALT2);
	mxc_iomux_set_pad(MX50_PIN_DISP_D6, 0x004);

	/* FEC TXD0 */
	mxc_request_iomux(MX50_PIN_DISP_D7, IOMUX_CONFIG_ALT2);
	mxc_iomux_set_pad(MX50_PIN_DISP_D7, 0x004);

	/* FEC TX_EN */
	mxc_request_iomux(MX50_PIN_DISP_D5, IOMUX_CONFIG_ALT2);
	mxc_iomux_set_pad(MX50_PIN_DISP_D5, 0x004);

	/* FEC TX_CLK */
	mxc_request_iomux(MX50_PIN_DISP_D0, IOMUX_CONFIG_ALT2);
	mxc_iomux_set_pad(MX50_PIN_DISP_D0, 0x0);
	mxc_iomux_set_input(MUX_IN_FEC_FEC_TX_CLK_SELECT_INPUT, 0x0);

	/* FEC RX_ER */
	mxc_request_iomux(MX50_PIN_DISP_D1, IOMUX_CONFIG_ALT2);
	mxc_iomux_set_pad(MX50_PIN_DISP_D1, 0x0);
	mxc_iomux_set_input(MUX_IN_FEC_FEC_RX_ER_SELECT_INPUT, 0);

	/* FEC CRS */
	mxc_request_iomux(MX50_PIN_DISP_D2, IOMUX_CONFIG_ALT2);
	mxc_iomux_set_pad(MX50_PIN_DISP_D2, 0x0);
	mxc_iomux_set_input(MUX_IN_FEC_FEC_RX_DV_SELECT_INPUT, 0);

#if defined(CONFIG_MX50_RDP) || defined(CONFIG_MX50_RD3) || defined(CONFIG_QSI_NIMBUS)
	/* FEC_RESET_B: gpio4-12 */
	mxc_request_iomux(MX50_PIN_ECSPI1_SCLK, IOMUX_CONFIG_ALT1);

	reg = readl(GPIO4_BASE_ADDR + 0x0);
	reg &= ~(1 << 12);
	writel(reg, GPIO4_BASE_ADDR + 0x0);

	reg = readl(GPIO4_BASE_ADDR + 0x4);
	reg |= (1 << 12);
	writel(reg, GPIO4_BASE_ADDR + 0x4);

	udelay(500);

	reg = readl(GPIO4_BASE_ADDR + 0x0);
	reg |= (1 << 12);
	writel(reg, GPIO4_BASE_ADDR + 0x0);
#elif defined(CONFIG_MX50_ARM2)
	/* phy reset: gpio4-6 */
	mxc_request_iomux(MX50_PIN_KEY_COL3, IOMUX_CONFIG_ALT1);

	reg = readl(GPIO4_BASE_ADDR + 0x0);
	reg &= ~0x40;
	writel(reg, GPIO4_BASE_ADDR + 0x0);

	reg = readl(GPIO4_BASE_ADDR + 0x4);
	reg |= 0x40;
	writel(reg, GPIO4_BASE_ADDR + 0x4);

	udelay(500);

	reg = readl(GPIO4_BASE_ADDR + 0x0);
	reg |= 0x40;
	writel(reg, GPIO4_BASE_ADDR + 0x0);
#else
#	error "Unsupported board!"
#endif
}
#endif

#ifdef CONFIG_CMD_MMC

struct fsl_esdhc_cfg esdhc_cfg[3] = {
	{MMC_SDHC1_BASE_ADDR, 1, 1},
	{MMC_SDHC2_BASE_ADDR, 1, 1},
	{MMC_SDHC3_BASE_ADDR, 1, 1},
};


#ifdef CONFIG_DYNAMIC_MMC_DEVNO
int get_mmc_env_devno(void)
{
	uint soc_sbmr = readl(SRC_BASE_ADDR + 0x4);
	int mmc_devno = 0;

	switch (soc_sbmr & 0x00300000) {
	default:
	case 0x0:
		mmc_devno = 0;
		break;
	case 0x00100000:
		mmc_devno = 1;
		break;
	case 0x00200000:
		mmc_devno = 2;
		break;
	}

	return mmc_devno;
}
#endif

#ifdef CONFIG_EMMC_DDR_PORT_DETECT
int detect_mmc_emmc_ddr_port(struct fsl_esdhc_cfg *cfg)
{
	return (MMC_SDHC3_BASE_ADDR == cfg->esdhc_base) ? 1 : 0;
}
#endif

/* The following function enables uSDHC instead of eSDHC
 * on SD3 port for SDR mode since eSDHC timing on MX50
 * is borderline for SDR mode. DDR mode will be disabled when this
 * define is enabled since the uSDHC timing on MX50 is borderline
 * for DDR mode. */
#ifdef CONFIG_MX50_ENABLE_USDHC_SDR
void enable_usdhc()
{
	/* Bring DIGCTL block out of reset and ungate clock */
	writel(0xC0000000, DIGCTL_BASE_ADDR + 0x8);
	/* Set bit 0 to select uSDHC */
	writel(1, DIGCTL_BASE_ADDR + 0x4);
}
#endif

int esdhc_gpio_init(bd_t *bis)
{
	s32 status = 0;
	u32 index = 0;

	for (index = 0; index < CONFIG_SYS_FSL_ESDHC_NUM;
		++index) {
		switch (index) {
		case 0:
			mxc_request_iomux(MX50_PIN_SD1_CMD, IOMUX_CONFIG_ALT0);
			mxc_request_iomux(MX50_PIN_SD1_CLK, IOMUX_CONFIG_ALT0);
			mxc_request_iomux(MX50_PIN_SD1_D0,  IOMUX_CONFIG_ALT0);
			mxc_request_iomux(MX50_PIN_SD1_D1,  IOMUX_CONFIG_ALT0);
			mxc_request_iomux(MX50_PIN_SD1_D2,  IOMUX_CONFIG_ALT0);
			mxc_request_iomux(MX50_PIN_SD1_D3,  IOMUX_CONFIG_ALT0);

			mxc_iomux_set_pad(MX50_PIN_SD1_CMD, 0x1E4);
			mxc_iomux_set_pad(MX50_PIN_SD1_CLK, 0xD4);
			mxc_iomux_set_pad(MX50_PIN_SD1_D0,  0x1D4);
			mxc_iomux_set_pad(MX50_PIN_SD1_D1,  0x1D4);
			mxc_iomux_set_pad(MX50_PIN_SD1_D2,  0x1D4);
			mxc_iomux_set_pad(MX50_PIN_SD1_D3,  0x1D4);

			break;
		case 1:
			mxc_request_iomux(MX50_PIN_SD2_CMD, IOMUX_CONFIG_ALT0);
			mxc_request_iomux(MX50_PIN_SD2_CLK, IOMUX_CONFIG_ALT0);
			mxc_request_iomux(MX50_PIN_SD2_D0,  IOMUX_CONFIG_ALT0);
			mxc_request_iomux(MX50_PIN_SD2_D1,  IOMUX_CONFIG_ALT0);
			mxc_request_iomux(MX50_PIN_SD2_D2,  IOMUX_CONFIG_ALT0);
			mxc_request_iomux(MX50_PIN_SD2_D3,  IOMUX_CONFIG_ALT0);
			mxc_request_iomux(MX50_PIN_SD2_D4,  IOMUX_CONFIG_ALT0);
			mxc_request_iomux(MX50_PIN_SD2_D5,  IOMUX_CONFIG_ALT0);
			mxc_request_iomux(MX50_PIN_SD2_D6,  IOMUX_CONFIG_ALT0);
			mxc_request_iomux(MX50_PIN_SD2_D7,  IOMUX_CONFIG_ALT0);

			mxc_iomux_set_pad(MX50_PIN_SD2_CMD, 0x14);
			mxc_iomux_set_pad(MX50_PIN_SD2_CLK, 0xD4);
			mxc_iomux_set_pad(MX50_PIN_SD2_D0,  0x1D4);
			mxc_iomux_set_pad(MX50_PIN_SD2_D1,  0x1D4);
			mxc_iomux_set_pad(MX50_PIN_SD2_D2,  0x1D4);
			mxc_iomux_set_pad(MX50_PIN_SD2_D3,  0x1D4);
			mxc_iomux_set_pad(MX50_PIN_SD2_D4,  0x1D4);
			mxc_iomux_set_pad(MX50_PIN_SD2_D5,  0x1D4);
			mxc_iomux_set_pad(MX50_PIN_SD2_D6,  0x1D4);
			mxc_iomux_set_pad(MX50_PIN_SD2_D7,  0x1D4);

			break;
		case 2:
#ifndef CONFIG_NAND_GPMI
			mxc_request_iomux(MX50_PIN_SD3_CMD, IOMUX_CONFIG_ALT0);
			mxc_request_iomux(MX50_PIN_SD3_CLK, IOMUX_CONFIG_ALT0);
			mxc_request_iomux(MX50_PIN_SD3_D0,  IOMUX_CONFIG_ALT0);
			mxc_request_iomux(MX50_PIN_SD3_D1,  IOMUX_CONFIG_ALT0);
			mxc_request_iomux(MX50_PIN_SD3_D2,  IOMUX_CONFIG_ALT0);
			mxc_request_iomux(MX50_PIN_SD3_D3,  IOMUX_CONFIG_ALT0);
			mxc_request_iomux(MX50_PIN_SD3_D4,  IOMUX_CONFIG_ALT0);
			mxc_request_iomux(MX50_PIN_SD3_D5,  IOMUX_CONFIG_ALT0);
			mxc_request_iomux(MX50_PIN_SD3_D6,  IOMUX_CONFIG_ALT0);
			mxc_request_iomux(MX50_PIN_SD3_D7,  IOMUX_CONFIG_ALT0);

			mxc_iomux_set_pad(MX50_PIN_SD3_CMD, 0x1E4);
			mxc_iomux_set_pad(MX50_PIN_SD3_CLK, 0xD4);
			mxc_iomux_set_pad(MX50_PIN_SD3_D0,  0x1D4);
			mxc_iomux_set_pad(MX50_PIN_SD3_D1,  0x1D4);
			mxc_iomux_set_pad(MX50_PIN_SD3_D2,  0x1D4);
			mxc_iomux_set_pad(MX50_PIN_SD3_D3,  0x1D4);
			mxc_iomux_set_pad(MX50_PIN_SD3_D4,  0x1D4);
			mxc_iomux_set_pad(MX50_PIN_SD3_D5,  0x1D4);
			mxc_iomux_set_pad(MX50_PIN_SD3_D6,  0x1D4);
			mxc_iomux_set_pad(MX50_PIN_SD3_D7,  0x1D4);
#endif
			break;
		default:
			printf("Warning: you configured more ESDHC controller"
				"(%d) as supported by the board(2)\n",
				CONFIG_SYS_FSL_ESDHC_NUM);
			return status;
			break;
		}
		status |= fsl_esdhc_initialize(bis, &esdhc_cfg[index]);
	}

	return status;
}

int board_mmc_init(bd_t *bis)
{
	if (!esdhc_gpio_init(bis))
		return 0;
	else
		return -1;
}

#endif

#ifdef CONFIG_MXC_EPDC
#ifdef CONFIG_SPLASH_SCREEN
int setup_splash_img()
{
#ifdef CONFIG_SPLASH_IS_IN_MMC
	int mmc_dev = get_mmc_env_devno();
	ulong offset = CONFIG_SPLASH_IMG_OFFSET;
	ulong size = CONFIG_SPLASH_IMG_SIZE;
	ulong addr = 0;
	char *s = NULL;
	struct mmc *mmc = find_mmc_device(mmc_dev);
	uint blk_start, blk_cnt, n;

	s = getenv("splashimage");

	if (NULL == s) {
		puts("env splashimage not found!\n");
		return -1;
	}
	addr = simple_strtoul(s, NULL, 16);

	if (!mmc) {
		printf("MMC Device %d not found\n",
			mmc_dev);
		return -1;
	}

	if (mmc_init(mmc)) {
		puts("MMC init failed\n");
		return  -1;
	}

	blk_start = ALIGN(offset, mmc->read_bl_len) / mmc->read_bl_len;
	blk_cnt   = ALIGN(size, mmc->read_bl_len) / mmc->read_bl_len;
	n = mmc->block_dev.block_read(mmc_dev, blk_start,
					blk_cnt, (u_char *)addr);
	flush_cache((ulong)addr, blk_cnt * mmc->read_bl_len);

	return (n == blk_cnt) ? 0 : -1;
#endif
}
#endif

vidinfo_t panel_info = {
	.vl_refresh = 60,
	.vl_col = 800,
	.vl_row = 600,
	.vl_pixclock = 17700000,
	.vl_left_margin = 8,
	.vl_right_margin = 142,
	.vl_upper_margin = 4,
	.vl_lower_margin = 10,
	.vl_hsync = 20,
	.vl_vsync = 4,
	.vl_sync = 0,
	.vl_mode = 0,
	.vl_flag = 0,
	.vl_bpix = 3,
	cmap:0,
};

static void setup_epdc_power()
{
	unsigned int reg;

	/* Setup epdc voltage */

	/* EPDC PWRSTAT - GPIO3[28] for PWR_GOOD status */
	mxc_request_iomux(MX50_PIN_EPDC_PWRSTAT, IOMUX_CONFIG_ALT1);

	/* EPDC VCOM0 - GPIO4[21] for VCOM control */
	mxc_request_iomux(MX50_PIN_EPDC_VCOM0, IOMUX_CONFIG_ALT1);
	/* Set as output */
	reg = readl(GPIO4_BASE_ADDR + 0x4);
	reg |= (1 << 21);
	writel(reg, GPIO4_BASE_ADDR + 0x4);

	/* UART4 TXD - GPIO6[16] for EPD PMIC WAKEUP */
	mxc_request_iomux(MX50_PIN_UART4_TXD, IOMUX_CONFIG_ALT1);
	/* Set as output */
	reg = readl(GPIO6_BASE_ADDR + 0x4);
	reg |= (1 << 16);
	writel(reg, GPIO6_BASE_ADDR + 0x4);
}

void epdc_power_on()
{
	unsigned int reg;

	/* Set PMIC Wakeup to high - enable Display power */
	reg = readl(GPIO6_BASE_ADDR + 0x0);
	reg |= (1 << 16);
	writel(reg, GPIO6_BASE_ADDR + 0x0);

	/* Wait for PWRGOOD == 1 */
	while (1) {
		reg = readl(GPIO3_BASE_ADDR + 0x0);
		if (!(reg & (1 << 28)))
			break;

		udelay(100);
	}

	/* Enable VCOM */
	reg = readl(GPIO4_BASE_ADDR + 0x0);
	reg |= (1 << 21);
	writel(reg, GPIO4_BASE_ADDR + 0x0);

	reg = readl(GPIO4_BASE_ADDR + 0x0);

	udelay(500);
}

void  epdc_power_off()
{
	unsigned int reg;
	/* Set PMIC Wakeup to low - disable Display power */
	reg = readl(GPIO6_BASE_ADDR + 0x0);
	reg |= 0 << 16;
	writel(reg, GPIO6_BASE_ADDR + 0x0);

	/* Disable VCOM */
	reg = readl(GPIO4_BASE_ADDR + 0x0);
	reg |= 0 << 21;
	writel(reg, GPIO4_BASE_ADDR + 0x0);
}

int setup_waveform_file()
{
#ifdef CONFIG_WAVEFORM_FILE_IN_MMC
	int mmc_dev = get_mmc_env_devno();
	ulong offset = CONFIG_WAVEFORM_FILE_OFFSET;
	ulong size = CONFIG_WAVEFORM_FILE_SIZE;
	ulong addr = CONFIG_WAVEFORM_BUF_ADDR;
	char *s = NULL;
	struct mmc *mmc = find_mmc_device(mmc_dev);
	uint blk_start, blk_cnt, n;

	if (!mmc) {
		printf("MMC Device %d not found\n",
			mmc_dev);
		return -1;
	}

	if (mmc_init(mmc)) {
		puts("MMC init failed\n");
		return -1;
	}

	blk_start = ALIGN(offset, mmc->read_bl_len) / mmc->read_bl_len;
	blk_cnt   = ALIGN(size, mmc->read_bl_len) / mmc->read_bl_len;
	n = mmc->block_dev.block_read(mmc_dev, blk_start,
		blk_cnt, (u_char *)addr);
	flush_cache((ulong)addr, blk_cnt * mmc->read_bl_len);

	return (n == blk_cnt) ? 0 : -1;
#else
	return -1;
#endif
}

static void setup_epdc()
{
	unsigned int reg;

	/* epdc iomux settings */
	mxc_request_iomux(MX50_PIN_EPDC_D0, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX50_PIN_EPDC_D1, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX50_PIN_EPDC_D2, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX50_PIN_EPDC_D3, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX50_PIN_EPDC_D4, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX50_PIN_EPDC_D5, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX50_PIN_EPDC_D6, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX50_PIN_EPDC_D7, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX50_PIN_EPDC_GDCLK, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX50_PIN_EPDC_GDSP, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX50_PIN_EPDC_GDOE, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX50_PIN_EPDC_GDRL, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX50_PIN_EPDC_SDCLK, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX50_PIN_EPDC_SDOE, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX50_PIN_EPDC_SDLE, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX50_PIN_EPDC_SDSHR, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX50_PIN_EPDC_BDR0, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX50_PIN_EPDC_SDCE0, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX50_PIN_EPDC_SDCE1, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX50_PIN_EPDC_SDCE2, IOMUX_CONFIG_ALT0);


	/*** epdc Maxim PMIC settings ***/

	/* EPDC PWRSTAT - GPIO3[28] for PWR_GOOD status */
	mxc_request_iomux(MX50_PIN_EPDC_PWRSTAT, IOMUX_CONFIG_ALT1);

	/* EPDC VCOM0 - GPIO4[21] for VCOM control */
	mxc_request_iomux(MX50_PIN_EPDC_VCOM0, IOMUX_CONFIG_ALT1);

	/* UART4 TXD - GPIO6[16] for EPD PMIC WAKEUP */
	mxc_request_iomux(MX50_PIN_UART4_TXD, IOMUX_CONFIG_ALT1);


	/*** Set pixel clock rates for EPDC ***/

	/* EPDC AXI clk and EPDC PIX clk from PLL1 */
	reg = readl(CCM_BASE_ADDR + CLKCTL_CLKSEQ_BYPASS);
	reg &= ~(0x3 << 4);
	reg |= (0x2 << 4) | (0x2 << 12);
	writel(reg, CCM_BASE_ADDR + CLKCTL_CLKSEQ_BYPASS);

	/* EPDC AXI clk enable and set to 200MHz (800/4) */
	reg = readl(CCM_BASE_ADDR + 0xA8);
	reg &= ~((0x3 << 30) | 0x3F);
	reg |= (0x2 << 30) | 0x4;
	writel(reg, CCM_BASE_ADDR + 0xA8);

	/* EPDC PIX clk enable and set to 20MHz (800/40) */
	reg = readl(CCM_BASE_ADDR + 0xA0);
	reg &= ~((0x3 << 30) | (0x3 << 12) | 0x3F);
	reg |= (0x2 << 30) | (0x1 << 12) | 0x2D;
	writel(reg, CCM_BASE_ADDR + 0xA0);

	panel_info.epdc_data.working_buf_addr = CONFIG_WORKING_BUF_ADDR;
	panel_info.epdc_data.waveform_buf_addr = CONFIG_WAVEFORM_BUF_ADDR;

	panel_info.epdc_data.wv_modes.mode_init = 0;
	panel_info.epdc_data.wv_modes.mode_du = 1;
	panel_info.epdc_data.wv_modes.mode_gc4 = 3;
	panel_info.epdc_data.wv_modes.mode_gc8 = 2;
	panel_info.epdc_data.wv_modes.mode_gc16 = 2;
	panel_info.epdc_data.wv_modes.mode_gc32 = 2;

	setup_epdc_power();

	/* Assign fb_base */
	gd->fb_base = CONFIG_FB_BASE;
}
#endif

#ifdef CONFIG_IMX_CSPI

#ifdef PATCHED_MC13892_BATTERY_20120706
#if defined(CONFIG_MX50_RD3)
void check_battery(void){}
#else
// check_battery for MC13892

#define MC13892_REG_ISENSE0			2
#define MC13892_REG_24				24
#define MC13892_REG_25				25
#define MC13892_REG_POWER_CTL0	    13
#define MC13892_REG_ADC0			43
#define MC13892_REG_ADC1			44
#define MC13892_REG_ADC2			45
#define MC13892_REG_ADC3			46
#define MC13892_REG_ADC4			47
#define MC13892_REG_CHARGE0			48

#define MC13892_ISENSE0_VBUSVALIDS	(1<<3)	
#define MC13892_ISENSE0_CHGDETS		(1<<6)	
#define MC13892_ISENSE0_CHGCURRS	(1<<11)
#define IsChargerPresence(a)	( ((a) & (MC13892_ISENSE0_VBUSVALIDS|MC13892_ISENSE0_CHGDETS)) ==  \
								  (MC13892_ISENSE0_VBUSVALIDS|MC13892_ISENSE0_CHGDETS) )

#define IsVBusOn(a)	( ((a) & (MC13892_ISENSE0_VBUSVALIDS|MC13892_ISENSE0_CHGDETS)) ==  \
								  (MC13892_ISENSE0_VBUSVALIDS|MC13892_ISENSE0_CHGDETS) )

#define MC13892_ADC0_CHRGICON_MASK		(1<<1)
#define MC13892_ADC0_BATICON_MASK		(1<<2)
#define MC13892_ADC0_ADRESET		    (1 << 8)
#define MC13892_ADC0_ADREFEN		    (1 << 10)
#define MC13892_ADC0_ADREFMODE		    (1 << 11)
#define MC13892_ADC0_TSMOD0		        (1 << 12)
#define MC13892_ADC0_TSMOD1		        (1 << 13)
#define MC13892_ADC0_TSMOD2		        (1 << 14)
#define MC13892_ADC0_ADINC1		        1 << 16)
#define MC13892_ADC0_ADINC2		        (1 << 17)

#define MC13892_ADC0_TSMOD_MASK		(MC13892_ADC0_TSMOD0 | \
					MC13892_ADC0_TSMOD1 | \
					MC13892_ADC0_TSMOD2)


#define MC13892_ADC1_ADEN		(1 << 0)
#define MC13892_ADC1_RAND		(1 << 1)
#define MC13892_ADC1_ADCCAL		(1 << 2)
#define MC13892_ADC1_ADSEL		(1 << 3)
#define MC13892_ADC1_ASC		(1 << 20)
#define MC13892_ADC1_ADTRIGIGN		(1 << 21)
#define MC13892_ADC1_DEFAULT_ATO	(0x080800)

#define MC13892_ADC1_ADA1_SHITT      5
#define MC13892_ADC1_ADA1_MASK       0xE0
#define MC13892_ADC1_ADA2_SHITT      8
#define MC13892_ADC1_ADA2_MASK       0x700								

#define MC13892_ADC2_ADD1_SHIFT		    2
#define MC13892_ADC2_ADD1_MASK		    0x3FF
#define MC13892_ADC2_ADD2_SHIFT		    14
#define MC13892_ADC2_ADD2_MASK		    0x3FF

#define MC13892_ADA1(ch)  ((ch&0x7)<<5)
#define MC13892_ADA2(ch)  ((ch&0x7)<<8)
#define GET_ADD1(val)     ((val>>MC13892_ADC2_ADD1_SHIFT)&MC13892_ADC2_ADD1_MASK)
#define GET_ADD2(val)     ((val>>MC13892_ADC2_ADD2_SHIFT)&MC13892_ADC2_ADD2_MASK)

#define MC13892_CHARGE0_VCHRG_42V		(3)
#define MC13892_CHARGE0_TREN			(1<<7)
#define MC13892_CHARGE0_ACKLPB			(1<<8)
#define MC13892_CHARGE0_CHGRESTART		(1<<20)
#define MC13892_CHARGE0_CHGAUTOB	    (1<<21)
#define MC13892_CHARGE0_CYCLB		    (1<<22)
#define MC13892_CHARGE0_CHGAUTOVIB		(1<<23)
#define MC13892_CHARGE0_ICHRG_SHIFT		3
#define MC13892_CHARGE0_ICHRG_MASK		0x78 
#define MC13892_CHARGE0_PLIM_SHIFT		(15)
#define MC13892_CHARGE0_PLIM_1200mW		(3)

// define for Current setting:
#define MC13892_ICHRG_OFF       0
#define MC13892_ICHRG_80mA      1
#define MC13892_ICHRG_240mA     2
#define MC13892_ICHRG_320mA     3
#define MC13892_ICHRG_400mA     4
#define MC13892_ICHRG_480mA     5
#define MC13892_ICHRG_560mA     6
#define MC13892_ICHRG_640mA     7
#define MC13892_ICHRG_720mA     8
#define MC13892_ICHRG_800mA     9
#define MC13892_ICHRG_880mA     10
#define MC13892_ICHRG_960mA     11
#define MC13892_ICHRG_1040mA    12
#define MC13892_ICHRG_1200mA    13
#define MC13892_ICHRG_1600mA    14
#define MC13892_ICHRG_FULLON    15

#define SET_ICHRG(val)  ((val<<MC13892_CHARGE0_ICHRG_SHIFT)&MC13892_CHARGE0_ICHRG_MASK)

#define BATLOW_LIMITED_WITHUSB_DEFAULT  3000
#define BATLOW_LIMITED_DEFAULT  3500

#define BATLOW_LIMITED_TOP      4000
#define BATLOW_LIMITED_BOTTOM  2650

#define ADC_INPUT_BAT_VOLTAGE   0
#define ADC_INPUT_BAT_CURRENT   1
#define ADC_INPUT_VBUS_VOLTAGE   3

void nimbus_low_battery_indication()
{
    int i;

    nimbus_led_switch_fun(3,0);
    nimbus_led_switch_fun(4,0);    
    for(i=0;i<3; i++)
    {
        nimbus_led_switch_fun(4,1);
        udelay(500000); // 500ms
        nimbus_led_switch_fun(4,0);
        udelay(500000); // 500ms
    }
}

int nimbus_get_battery_voltage(void)
{
    struct spi_slave *slave;
    unsigned int val,volt_bat;
	unsigned int adc1,adc1_cur;
	int  chg_curr, ii, iDelay;
    
    slave = spi_pmic_probe();
    
    // reset ADC
	val = pmic_reg(slave, MC13892_REG_ADC0, 0, 0);
	pmic_reg(slave, MC13892_REG_ADC0, (val|MC13892_ADC0_ADRESET), 1);
	udelay(100);
	val &= (~MC13892_ADC0_ADRESET);
	pmic_reg(slave, MC13892_REG_ADC0, val, 1);
	udelay(1000);

	// enable ADC calibration once,ADA1=0
	adc1 = MC13892_ADC1_ADEN | MC13892_ADC1_ADCCAL | MC13892_ADC1_ADTRIGIGN | MC13892_ADC1_ASC | MC13892_ADC1_DEFAULT_ATO;
	
	for(ii=0; ii<3; ii++)
	{
		//arbitrarily set ADA2 to channel 3 
		adc1 |= MC13892_ADA2(3);
		pmic_reg(slave, MC13892_REG_ADC1, adc1, 1);

		/* Wait for conversation complete */
		for (iDelay=0; iDelay<10000; iDelay++) {
			adc1_cur = pmic_reg(slave, MC13892_REG_ADC1, 0, 0);
			if(ii==0)	// calibration loop
			{
				if ( !(adc1_cur & (MC13892_ADC1_ADCCAL|MC13892_ADC1_ASC)) )
					break;
			}
			else  // ADC conversion loop
			{
				if (!(adc1_cur & MC13892_ADC1_ASC))
					break;
			}
			udelay(100);
		}

		// no ADCCAL here
		adc1 = MC13892_ADC1_ADEN | MC13892_ADC1_ADTRIGIGN | MC13892_ADC1_ASC | MC13892_ADC1_DEFAULT_ATO;

		//read ADC2
		val = pmic_reg(slave, MC13892_REG_ADC2, 0, 0);
	}
	// always take the 2nd ADC read and ignore the 1 ADC read after calibration

	//get Battery voltage from ADD1
	volt_bat = GET_ADD1(val);

	volt_bat = (volt_bat+1)*4800/1024;
    spi_pmic_free(slave);

    return(volt_bat);
}

int nimbus_get_vbus_voltage(void)
{
    struct spi_slave *slave;
    unsigned int val,volt_bat;
	unsigned int adc1,adc1_cur;
	int  chg_curr, ii, iDelay;
    
    slave = spi_pmic_probe();
    // reset ADC
	val = pmic_reg(slave, MC13892_REG_ADC0, 0, 0);
	pmic_reg(slave, MC13892_REG_ADC0, (val|MC13892_ADC0_ADRESET), 1);
	udelay(100);
	val &= (~MC13892_ADC0_ADRESET);
	pmic_reg(slave, MC13892_REG_ADC0, val, 1);
	udelay(1000);

	// enable ADC calibration once
	adc1 = MC13892_ADC1_ADEN | MC13892_ADC1_ADCCAL | MC13892_ADC1_ADTRIGIGN | MC13892_ADC1_ASC | MC13892_ADC1_DEFAULT_ATO;
	for(ii=0; ii<3; ii++)
	{
		//arbitrarily set ADA2 to channel 3 
		adc1 |= MC13892_ADA2(3);
		pmic_reg(slave, MC13892_REG_ADC1, adc1, 1);

		/* Wait for conversation complete */
		for (iDelay=0; iDelay<10000; iDelay++) {
			adc1_cur = pmic_reg(slave, MC13892_REG_ADC1, 0, 0);
			if(ii==0)	// calibration loop
			{
				if ( !(adc1_cur & (MC13892_ADC1_ADCCAL|MC13892_ADC1_ASC)) )
					break;
			}
			else  // ADC conversion loop
			{
				if (!(adc1_cur & MC13892_ADC1_ASC))
					break;
			}
			udelay(100);
		}

		// no ADCCAL here
		adc1 = MC13892_ADC1_ADEN | MC13892_ADC1_ADTRIGIGN | MC13892_ADC1_ASC | MC13892_ADC1_DEFAULT_ATO;

		//read ADC2
		val = pmic_reg(slave, MC13892_REG_ADC2, 0, 0);
	}
	// always take the 2nd ADC read and ignore the 1 ADC read after calibration

	volt_bat = GET_ADD2(val);
	volt_bat = (volt_bat*10000)/852;
    spi_pmic_free(slave);

    return(volt_bat);
}

int nimbus_get_current(int isChanrge,int limited)
{
    struct spi_slave *slave;
    unsigned int val,volt_bat;
	unsigned int adc1,adc1_cur;
	int  chg_curr, ii, iDelay,adc_ch;
    char *current_limited[16]={"OFF","80","240","320","400","480","560",
                             "640","720","800","880","960","1040","1200","1600","FULL"};

    adc_ch=(isChanrge)?4:1;
    
    slave = spi_pmic_probe();
    // reset ADC
	val = pmic_reg(slave, MC13892_REG_ADC0, 0, 0);
	pmic_reg(slave, MC13892_REG_ADC0, (val|MC13892_ADC0_ADRESET), 1);
	udelay(100);
	val &= (~MC13892_ADC0_ADRESET);
	pmic_reg(slave, MC13892_REG_ADC0, val, 1);
	udelay(1000);

	printf("Setting charge current: %s (mA)\n",current_limited[limited]);

    chg_curr=0;
	val = pmic_reg(slave, MC13892_REG_CHARGE0, 0, 0);
	val |= (3<< MC13892_CHARGE0_PLIM_SHIFT);
    pmic_reg(slave,  MC13892_REG_CHARGE0, val, 1);


	// set CHARGE0_CHGAUTOVIB = 1
	val |= MC13892_CHARGE0_CHGAUTOVIB;
	
	val &= ~MC13892_CHARGE0_ICHRG_MASK;

	val |= SET_ICHRG(limited);
	pmic_reg(slave,  MC13892_REG_CHARGE0, val, 1);

	udelay(1000000); // change to 1000 ms

	// READ CHARGE CURRENT

	// enable charge current reading
	val = pmic_reg(slave, MC13892_REG_ADC0, 0, 0);
	val |= MC13892_ADC0_CHRGICON_MASK | MC13892_ADC0_BATICON_MASK;
	pmic_reg(slave, MC13892_REG_ADC0, val, 1);

	//set ADA1 to channel 1 && 
	//ADA2 to channel 4 for charge current
	adc1 = MC13892_ADC1_ADEN | MC13892_ADC1_ADTRIGIGN | MC13892_ADC1_ASC | MC13892_ADC1_DEFAULT_ATO;
	adc1 |=  MC13892_ADA2(adc_ch);
	pmic_reg(slave, MC13892_REG_ADC1, adc1, 1);

	/* Wait for conversation complete */
	for (iDelay=0; iDelay<10000;iDelay++) {
		adc1_cur = pmic_reg(slave, MC13892_REG_ADC1, 0, 0);
		if (!(adc1_cur & MC13892_ADC1_ASC))
			break;

		udelay(100);
	}

	//read ADC2
	val = pmic_reg(slave, MC13892_REG_ADC2, 0, 0);

	// get charging current from ADD2
	chg_curr = GET_ADD2(val);
	if((chg_curr & 0x200))
	{
		// negative value
		chg_curr = 0 - ((~chg_curr + 1) & 0x1FF);
	}
	chg_curr = (chg_curr * 5865)/1000;

	printf("Reading current:%d (mA)\n", chg_curr);

	// disable charge current measurement
	val = pmic_reg(slave, MC13892_REG_ADC0, 0, 0);
	val &= (~MC13892_ADC0_CHRGICON_MASK | MC13892_ADC0_BATICON_MASK);
	pmic_reg(slave, MC13892_REG_ADC0, val, 1);
	
    spi_pmic_free(slave);

    return(chg_curr);
}



int nimbus_get_sys_current(int isChanrge)
{
    struct spi_slave *slave;
    unsigned int val;
	unsigned int adc1,adc1_cur;
	int  chg_curr, ii, iDelay,adc_ch;

    adc_ch=(isChanrge)?4:1;
    
    slave = spi_pmic_probe();
    // reset ADC
	val = pmic_reg(slave, MC13892_REG_ADC0, 0, 0);
	pmic_reg(slave, MC13892_REG_ADC0, (val|MC13892_ADC0_ADRESET), 1);
	udelay(100);
	val &= (~MC13892_ADC0_ADRESET);
	pmic_reg(slave, MC13892_REG_ADC0, val, 1);
	udelay(1000);

    chg_curr=0;
	// READ CHARGE CURRENT

	// enable charge current reading
	val = pmic_reg(slave, MC13892_REG_ADC0, 0, 0);
	val |= MC13892_ADC0_CHRGICON_MASK | MC13892_ADC0_BATICON_MASK;
	pmic_reg(slave, MC13892_REG_ADC0, val, 1);

	//set ADA1 to channel 1 && 
	//ADA2 to channel 4 for charge current
	adc1 = MC13892_ADC1_ADEN | MC13892_ADC1_ADTRIGIGN | MC13892_ADC1_ASC | MC13892_ADC1_DEFAULT_ATO;
	adc1 |=  MC13892_ADA2(adc_ch);
	pmic_reg(slave, MC13892_REG_ADC1, adc1, 1);

	/* Wait for conversation complete */
	for (iDelay=0; iDelay<10000;iDelay++) {
		adc1_cur = pmic_reg(slave, MC13892_REG_ADC1, 0, 0);
		if (!(adc1_cur & MC13892_ADC1_ASC))
			break;

		udelay(100);
	}

	//read ADC2
	val = pmic_reg(slave, MC13892_REG_ADC2, 0, 0);

	// get charging current from ADD2
	chg_curr = GET_ADD2(val);
	if((chg_curr & 0x200))
	{
		// negative value
		chg_curr = 0 - ((~chg_curr + 1) & 0x1FF);
	}
	chg_curr = (chg_curr * 5865)/1000;

	// disable charge current measurement
	val = pmic_reg(slave, MC13892_REG_ADC0, 0, 0);
	val &= (~MC13892_ADC0_CHRGICON_MASK | MC13892_ADC0_BATICON_MASK);
	pmic_reg(slave, MC13892_REG_ADC0, val, 1);
	
    spi_pmic_free(slave);

    return(chg_curr);
}


void check_battery(void)
{
	struct spi_slave *slave;
	u32 val,val1;
    unsigned int volt_bat,volt_vbus,bat_limited,bat_limited_usb;
	u32 adc1,adc1_cur;
	int  chg_curr, ii, iDelay,bat_check;
    int isLedOn;
    char *s=NULL;
    ulong tmp,bat_timer,bat_led_timer,low_charging_timeout;

    slave = spi_pmic_probe();

    s=getenv("bat_check");
    if( s==NULL) printf("<INFO> no bat_check setting!\n");
    if( s==NULL || strcmp(s,"1")!=0)
    {
        printf("Battery check is disabled!\n");
        bat_check=0;
    }
    else bat_check=1;

    s=getenv("bat_limited");
    if( s==NULL) printf("<INFO> no bat_limited setting!(default: %d)\n",BATLOW_LIMITED_DEFAULT);
    if( s!=NULL)
    {
        bat_limited=simple_strtoul(s, NULL, 10);
        if(bat_limited>BATLOW_LIMITED_TOP && bat_limited<BATLOW_LIMITED_WITHUSB_DEFAULT)
        {
            printf("Battery Limited setting out of range(%d~%d mV): %d\n",BATLOW_LIMITED_TOP,BATLOW_LIMITED_WITHUSB_DEFAULT,bat_limited);
            bat_limited=BATLOW_LIMITED_DEFAULT;
        }
        
        printf("Battary Booting Limited: %d mV.\n",bat_limited);
    }
    else
    {
        bat_limited=BATLOW_LIMITED_DEFAULT;
    }

    s=getenv("lowbat_limited");
    if(s==NULL) printf("<INFO> no lowbat_limited setting!(default: %d)\n",BATLOW_LIMITED_WITHUSB_DEFAULT);
    if(s!=NULL)
    {
        bat_limited_usb=simple_strtoul(s, NULL, 10);
        if( bat_limited_usb==0)
        {
            printf("Disable pre-charging.\n");
        }
        else if(bat_limited_usb>BATLOW_LIMITED_TOP && bat_limited_usb<BATLOW_LIMITED_BOTTOM)
        {
            printf("Battary pre-charging limited setting out of range(%d~%d mV): %d\n",BATLOW_LIMITED_TOP,BATLOW_LIMITED_BOTTOM,bat_limited_usb);
            bat_limited_usb=BATLOW_LIMITED_WITHUSB_DEFAULT;
        }
        
        printf("Pre-charging Limited: %d mV. (For very low booting with USB. 0:Disabled)\n",bat_limited_usb);
    }
    else
    {
        bat_limited_usb=BATLOW_LIMITED_WITHUSB_DEFAULT;
    }

    // check USB if present
	volt_vbus = nimbus_get_vbus_voltage();
    volt_vbus = (nimbus_get_vbus_voltage()+ volt_vbus)/2;
	printf("VBUS voltage: %d\n",volt_vbus);

    // get battery voltage
	volt_bat = nimbus_get_battery_voltage();
    volt_bat = (nimbus_get_battery_voltage()+volt_bat)/2;
	printf("Battery voltage: %d\n",volt_bat);

	if(bat_check==1 && volt_bat<bat_limited)
	{
	    // RED led on
        nimbus_led_switch_fun(4,1);
        nimbus_led_switch_fun(3,0);

	    ii=0;
	    do
		{
    		val = pmic_reg(slave, MC13892_REG_ISENSE0, 0, 0);
            udelay(1000);
            val1 = pmic_reg(slave, MC13892_REG_ISENSE0, 0, 0);
            udelay(1000);
            if( IsChargerPresence(val) == IsChargerPresence(val1) )
            {
                break;
            }
        }while(ii++ <3);
        
        printf("SENSE0: 0x%04X\n",val);
		if( IsChargerPresence(val) )
		{
			val = ((MC13892_CHARGE0_PLIM_1200mW<< MC13892_CHARGE0_PLIM_SHIFT)|MC13892_CHARGE0_VCHRG_42V);

			// set CHARGE0_ICHRG = 560mA , same as hw setting
			val &= ~MC13892_CHARGE0_ICHRG_MASK;
            val |= (MC13892_ICHRG_560mA<<MC13892_CHARGE0_ICHRG_SHIFT);
            val |= MC13892_CHARGE0_CHGAUTOB; // disable auto-charging
            val |= MC13892_CHARGE0_CYCLB;
            val |= MC13892_CHARGE0_CHGAUTOVIB;
            val |= MC13892_CHARGE0_CHGRESTART;
			pmic_reg(slave,  MC13892_REG_CHARGE0, val, 1);

			udelay(500000);

			// READ CHARGE CURRENT

			// enable charge current reading
			val = pmic_reg(slave, MC13892_REG_ADC0, 0, 0);
			val |= MC13892_ADC0_CHRGICON_MASK;
			pmic_reg(slave, MC13892_REG_ADC0, val, 1);

			//set ADA1 to channel 1 && 
			//ADA2 to channel 4 for charge current
			adc1 = MC13892_ADC1_ADEN | MC13892_ADC1_ADTRIGIGN | MC13892_ADC1_ASC | MC13892_ADC1_DEFAULT_ATO;
			adc1 |= (4<< MC13892_ADC1_ADA2_SHITT);
			pmic_reg(slave, MC13892_REG_ADC1, adc1, 1);

			/* Wait for conversation complete */
			for (iDelay=0; iDelay<10000;iDelay++) {
				adc1_cur = pmic_reg(slave, MC13892_REG_ADC1, 0, 0);
				if (!(adc1_cur & MC13892_ADC1_ASC))
				{
				    break;
                }
				udelay(100);
			}

			//read ADC2
			val = pmic_reg(slave, MC13892_REG_ADC2, 0, 0);

			// get charging current from ADC2
			chg_curr = (val >> MC13892_ADC2_ADD2_SHIFT) & MC13892_ADC2_ADD2_MASK;
			if((chg_curr & 0x200))
			{
				// negative value
				chg_curr = 0 - ((~chg_curr + 1) & 0x1FF);
			}
			chg_curr = (chg_curr * 5865)/1000;

			printf("Charger current:%d mA\n", chg_curr);
            isLedOn=1;
            bat_timer=0;
            bat_led_timer=0;
            ii=0;
            iDelay=0;
            #ifdef QSI_LOW_BAT_POWER_ON_PORTECT
            if(bat_limited_usb>0 && volt_bat<bat_limited_usb )
            {
                int precharge_check=0,precharge_ok;
                precharge_ok=0;
                tmp=get_timer(0);
                low_charging_timeout=QSI_LOW_BAT_POWER_ON_TIMEOUT*CONFIG_SYS_HZ;
                do
                {
                    tmp=get_timer(0);
                    if(bat_led_timer < tmp)
                    {
                        bat_led_timer=tmp+CONFIG_SYS_HZ/2;
                        if(isLedOn)
                        {
                            nimbus_led_switch_fun(4,0);                        
                        }
                        else
                        {
                            nimbus_led_switch_fun(4,1);
                        }
                        isLedOn^=1;
                    }
                    if(bat_timer < tmp)
                    {
                        bat_timer=tmp+2*CONFIG_SYS_HZ;
                        volt_bat = (nimbus_get_battery_voltage() + volt_bat)/2;
                        if( volt_bat>bat_limited_usb )
                        {
                            if(++precharge_check > 3)
                            {
                                precharge_ok=1;
                            }
                        }
                        else precharge_check=0;
                        iDelay=(tmp+QSI_LOW_BAT_POWER_ON_TIMEOUT*CONFIG_SYS_HZ-low_charging_timeout)/CONFIG_SYS_HZ;
                        printf("Battery voltage checking: %d ,%d sec (%d)\n",volt_bat,iDelay,QSI_LOW_BAT_POWER_ON_TIMEOUT);
                    }

                    val = pmic_reg(slave, MC13892_REG_ISENSE0, 0, 0);
                    if(!IsVBusOn(val))
                    {
                        if(ii++ > 3)
                        {
                            printf("Low battery: USB removed!(power down)\n");
                            nimbus_low_battery_indication();
                            val = pmic_reg(slave, MC13892_REG_POWER_CTL0, 0, 0);
            				val |= 0x8;
        	    			pmic_reg(slave,  MC13892_REG_POWER_CTL0, val, 1);
                            while(1);
                        }
                    }
                    else ii=0;

                    if(low_charging_timeout < tmp)
                    {
                        printf("Lower battery charging timeout: BAT-%d\n",volt_bat);
                        precharge_ok=1;
                    }
                }while( precharge_ok==0 );
            }
                
            // RED LED for low, < 3.6V
            nimbus_led_switch_fun(4,1);
            #ifdef  QSI_PWRST_ALIGN
            bat_cap_st=BATCAP_LOW;
            #endif
            #endif
		}
		else
		{
            nimbus_low_battery_indication();
			//no charger, shut down
			printf("No found power adaptor!\nBattery is lower than %d mV, MC13982 power off\n",bat_limited);
			udelay(1000);
			val = pmic_reg(slave, MC13892_REG_POWER_CTL0, 0, 0);
			val |= 0x8;
			pmic_reg(slave,  MC13892_REG_POWER_CTL0, val, 1);
            while(1);
		}
	}
    else
    {

        if(volt_vbus>4700)
	    {
    	    val = ((MC13892_CHARGE0_PLIM_1200mW<< MC13892_CHARGE0_PLIM_SHIFT)|MC13892_CHARGE0_VCHRG_42V);

    		// set CHARGE0_ICHRG = 560mA , same as hw setting
    		val &= ~MC13892_CHARGE0_ICHRG_MASK;
            val |= (MC13892_ICHRG_560mA<<MC13892_CHARGE0_ICHRG_SHIFT);
            val |= MC13892_CHARGE0_CHGAUTOB; // disable auto-charging
            val |= MC13892_CHARGE0_CYCLB;
            val |= MC13892_CHARGE0_CHGAUTOVIB;
            val |= MC13892_CHARGE0_CHGRESTART;
    		pmic_reg(slave,  MC13892_REG_CHARGE0, val, 1);
            printf("Reset CHARG and Restart charging...(0x%x)\n",val);
        }

        // for battery status
        if((volt_vbus>4700 && volt_bat < 3750) || (volt_bat < 3600))
        {
            // RED for low battery.
            nimbus_led_switch_fun(4,1);
            nimbus_led_switch_fun(3,0);
            #ifdef  QSI_PWRST_ALIGN
            bat_cap_st=BATCAP_LOW;
            #endif

        }
        #ifdef QSI_PWRST_ALIGN
        else
        {
            bat_cap_st=BATCAP_NORMAL;
        }
        #endif
        
    }

	spi_pmic_free(slave);

}
#endif //defined(CONFIG_MX50_RD3)
#endif

#ifdef QSI_PWRST_ALIGN
#define PWRKEY_VALID_TIME 2
#define LED_BLINK_TIME 650
#define LOWTONOMRAL_TIMER (120*CONFIG_SYS_HZ)
#define FULLDROP_TIMER    (60*CONFIG_SYS_HZ)
#define BAT_CHARGE_NORMAL_BTM       3750
#define CHARGING_TIMEOUT    (3*60*60)
#define BAT_FULL_LIMITM         4100
#define CHGR_VOLTAGE_MIN        4700

#define CHG_IN_FULL_TIME1   (3*60*CONFIG_SYS_HZ)
#define CHG_IN_FULL_TIME2   (30*60*CONFIG_SYS_HZ)
#define CHG_IN_FULL_TIME3   (60*60*CONFIG_SYS_HZ)

void qsi_check_booting_case()
{
    printf("Booting ST: 0x%x , bat_st=%d\n",qwifi_booting_st,bat_cap_st);

    if(qwifi_booting_st&QSI_PWRBY_USB)
    {
        /* we checked: power key to booting.
         * 1. charging device
         * 2. Take off USB to power down.
         * 3. power key valide to boot system.
         */
         int booting=0;
         struct spi_slave *slave;
         ulong tmp,pwrkey_timer,led_timer,batst_timer,sec_timer,sec_timeout,charging_timeout;
         ulong bat_charging_in_small_current_timestamp;
         int nousb,chk_charger,volt_bat,volt_bat_avg;
         u32 val,isLedOn;
         int chg_cur,bat_cur;
         int sec_checked,ICHRG_limited,chk_charger_st;
         int isChgFinish;

         slave = spi_pmic_probe();

         chk_charger=1;

         tmp=get_timer(0);
         led_timer=tmp+LED_BLINK_TIME;
         pwrkey_timer=tmp+PWRKEY_VALID_TIME*CONFIG_SYS_HZ; // 2 sec for power key booting
         charging_timeout=tmp+CHARGING_TIMEOUT;
         sec_timeout=tmp+CONFIG_SYS_HZ;
         sec_timer=0;
         charging_timeout=sec_timer+CHARGING_TIMEOUT;
         ICHRG_limited=MC13892_ICHRG_560mA;
         chk_charger_st=0;
         isChgFinish=0;
         volt_bat_avg=0;
         isLedOn=0;
         bat_charging_in_small_current_timestamp=tmp;
         do
         {
            tmp=get_timer(0);

            if(sec_timeout<tmp)
            {
                sec_timeout=tmp+CONFIG_SYS_HZ;
                sec_timer++;
                sec_checked=1;
            }

            // handle led
            if(led_timer<tmp)
            {
                led_timer=tmp+LED_BLINK_TIME;
                if(bat_cap_st==BATCAP_NORMAL)
                {
                    nimbus_led_switch_fun(4,0); 
                    if(isLedOn)
                    {
                        nimbus_led_switch_fun(3,0);                        
                    }
                    else
                    {
                        nimbus_led_switch_fun(3,1);
                    }
                }
                else if(sec_checked)
                {
                    if(bat_cap_st==BATCAP_LOW)
                    {
                        nimbus_led_switch_fun(3,0); 
                        nimbus_led_switch_fun(4,1); 
                    }
                    else 
                    {
                        nimbus_led_switch_fun(3,1); 
                        nimbus_led_switch_fun(4,0); 
                    }
                }

                if(isChgFinish==0)
                {
                    if(isLedOn)
                    {
                        chg_cur=nimbus_get_sys_current(1);
                    }
                    else
                    {
                        bat_cur=(-nimbus_get_sys_current(0));
                    }
                }
                isLedOn^=1;
                //printf("<INFO> led updated. isLedOn=%d,%d\n",isLedOn,bat_cap_st);
            }

            // if need to adjust charging current
            if(chk_charger && sec_checked)
            {
                if(chk_charger_st==0 && ICHRG_limited<=MC13892_ICHRG_720mA)
                {
                    val=pmic_reg(slave,  MC13892_REG_CHARGE0, 0, 0);
                    // set CHARGE0_CHGAUTOVIB = 1
                	val |= MC13892_CHARGE0_CHGAUTOVIB;
                	
                	val &= ~MC13892_CHARGE0_ICHRG_MASK;

                	val |= SET_ICHRG(ICHRG_limited);
                	pmic_reg(slave,  MC13892_REG_CHARGE0, val, 1);
                    chk_charger_st=1;
                }

                if(chk_charger_st==1)
                {
                    if(nimbus_get_vbus_voltage()<CHGR_VOLTAGE_MIN)
                    {
                        if(ICHRG_limited > MC13892_ICHRG_560mA)
                        {
                            ICHRG_limited--;
                            printf("<WARNING> VBUS drop! down step.(%d)\n",ICHRG_limited);
                            val=pmic_reg(slave,  MC13892_REG_CHARGE0, 0, 0);
                            // set CHARGE0_CHGAUTOVIB = 1
                        	val |= MC13892_CHARGE0_CHGAUTOVIB;
                        	
                        	val &= ~MC13892_CHARGE0_ICHRG_MASK;

                        	val |= SET_ICHRG(ICHRG_limited);
                        	pmic_reg(slave,  MC13892_REG_CHARGE0, val, 1);
                        }
                        chk_charger_st=0;
                        chk_charger=0;
                    }

                    if(ICHRG_limited<MC13892_ICHRG_720mA)
                    {
                        ICHRG_limited++;
                        chk_charger_st=0;
                    }
                    else
                    {
                        chk_charger_st=0;
                        chk_charger=0;
                    }
                    
                }
            }
            
            // Monitor battery voltage
            if(sec_checked)
            {
                volt_bat = nimbus_get_battery_voltage();
                if(volt_bat>0 && volt_bat<4300)
                {
                    if(volt_bat_avg==0)  volt_bat_avg=volt_bat;
                    volt_bat_avg= (volt_bat_avg+volt_bat)/2;
                }

                if(isChgFinish==0)
                {
                    printf("CHG: %d mA, BAT=%d mA (%d mV),time: %d sec.(st:%d)\n",chg_cur,bat_cur,volt_bat_avg,sec_timer,bat_cap_st);
                    if(charging_timeout<sec_timer)
                    {
                        isChgFinish=1;
                    }
                    else if(volt_bat_avg>BAT_FULL_LIMITM && bat_cur<160 && (chg_cur<380))
                    {
                        if((tmp-bat_charging_in_small_current_timestamp) > CHG_IN_FULL_TIME1 ) // 3 min.
                        {
                            printf("<INFO> Charging: very small current timeout!\n");
                            isChgFinish=1;
                        }
                    }
                    else if(volt_bat_avg>BAT_FULL_LIMITM && bat_cur<180 && (chg_cur<400))
                    {
                        if((tmp-bat_charging_in_small_current_timestamp) > CHG_IN_FULL_TIME2 ) // 30 min.
                        {
                            printf("<INFO> Charging: small current timeout!\n");
                            isChgFinish=1;
                        }
                    }
                    else
                    {
                        // maybe in wi-fi accessing casue in large current on VBUS
                        if(volt_bat_avg>BAT_FULL_LIMITM && bat_cur<180)
                        {
                            if((tmp-bat_charging_in_small_current_timestamp) > CHG_IN_FULL_TIME3 ) // 60 min.
                            {
                                printf("<INFO> Charging: small current timeout! (VBUS current changes)\n");
                                isChgFinish=1;
                            }
                        }
                        else bat_charging_in_small_current_timestamp=tmp;
                    }
                }
            }

            if(bat_cap_st==BATCAP_LOW)
            {
                if(volt_bat_avg>BAT_CHARGE_NORMAL_BTM)
                {
                    if(batst_timer<tmp)
                    {
                        bat_cap_st=BATCAP_NORMAL;
                        printf("<INFO> Battery from LOW to NORMAL.\n");
                    }
                }
                else
                {
                    batst_timer=tmp+LOWTONOMRAL_TIMER;
                }
            }
            else if(bat_cap_st==BATCAP_NORMAL)
            {
                if(isChgFinish==1)
                {
                    // change ICHRG to default.
                    val=pmic_reg(slave,  MC13892_REG_CHARGE0, 0, 0);
                    // set CHARGE0_CHGAUTOVIB = 1
                	val &= ~MC13892_CHARGE0_CHGAUTOVIB;
                	
                	val &= ~MC13892_CHARGE0_ICHRG_MASK;

                	val |= SET_ICHRG(MC13892_ICHRG_OFF);
                	pmic_reg(slave,  MC13892_REG_CHARGE0, val, 1);
                    bat_cap_st=BATCAP_FULL;
                }
            }
            else if(bat_cap_st==BATCAP_FULL)
            {
                if(volt_bat_avg<4050)
                {
                    if(isChgFinish && tmp>batst_timer)
                    {
                        isChgFinish=0;
                        sec_timer=0;
                        charging_timeout=sec_timer+CHARGING_TIMEOUT;
                        bat_charging_in_small_current_timestamp=tmp;
                        printf("Re-start CHG!\n");
                        // change ICHRG to default.
                        val=pmic_reg(slave,  MC13892_REG_CHARGE0, 0, 0);
                        // set CHARGE0_CHGAUTOVIB = 1
                    	val |= MC13892_CHARGE0_CHGAUTOVIB;
                        val |= MC13892_CHARGE0_CHGRESTART;
                    	val &= ~MC13892_CHARGE0_ICHRG_MASK;

                    	val |= SET_ICHRG(MC13892_ICHRG_560mA);
                    	pmic_reg(slave,  MC13892_REG_CHARGE0, val, 1);
                    }
                }
                else if(isChgFinish)
                {
                    batst_timer=tmp+FULLDROP_TIMER;
                }
            }

            // check power key
            val = pmic_reg(slave, 5, 0, 0); //Interrupt Sense1
            if(val & 0x8)
            {
                pwrkey_timer=tmp+PWRKEY_VALID_TIME*CONFIG_SYS_HZ;
            }
            else if(pwrkey_timer<tmp)
            {
                printf("<INFO> Found valid power key! booting...\n");
                booting=1;
            }

            // check usb
            val = pmic_reg(slave, MC13892_REG_ISENSE0, 0, 0);
            if(!IsVBusOn(val))
            {
                if(nousb++ > 3)
                {
                    printf("USB booting: USB removed!(power down)\n");
                    val = pmic_reg(slave, MC13892_REG_POWER_CTL0, 0, 0);
    				val |= 0x8;
	    			pmic_reg(slave,  MC13892_REG_POWER_CTL0, val, 1);
                    while(1);
                }
            }
            else nousb=0;

            sec_checked=0;
        }while(booting==0);

        spi_pmic_free(slave);

        if(bat_cap_st==BATCAP_LOW)
        {
            nimbus_led_switch_fun(3,0); 
            nimbus_led_switch_fun(4,1); 
        }
        else 
        {
            nimbus_led_switch_fun(3,1); 
            nimbus_led_switch_fun(4,0); 
        }
         
    }
    else if(qwifi_booting_st&QSI_PWRBY_KEY)
    {
        printf("<INFO> Power key boot!\n");
    }
    else if(qwifi_booting_st&QSI_PWRBY_REBOOT)
    {
        printf("<INFO> System Reboot!\n");
    }
    
}
#endif

static void setup_power(void)
{
	struct spi_slave *slave;
	unsigned int val;

	puts("PMIC Mode: SPI\n");

	/* Enable VGEN1 to enable ethernet */
	slave = spi_pmic_probe();

#if defined(CONFIG_MX50_RD3) || defined(CONFIG_QSI_NIMBUS)

#if 0 //def QSI_NIMBUS_PMIC_SPI_PATCH_DEF 
    // setup SW1A/B power
    pmic_reg(slave, 24, 0x420, 1);
    // reset PMIC IRQ mask
    pmic_reg(slave, 1, 0xffffff, 1);
    pmic_reg(slave, 4, 0xffffff, 1);
#endif /*QSI_NIMBUS_PMIC_SPI_PATCH_DEF*/

	/* Set global reset time to 0s*/
	val = pmic_reg(slave, 15, 0, 0);
	val &= ~(0x300);
	pmic_reg(slave, 15, val, 1);
    
#if 1 //ndef CONFIG_MFG
    val = pmic_reg(slave, 7, 0, 0);
	printf("PMIC ID: 0x%08x\n", val);
    
    val = pmic_reg(slave, 8, 0, 0);
	printf("PMIC REGULAR FAULT: 0x%08x\n", val);
    
    val = pmic_reg(slave, 6, 0, 0);
    printf("PMIC: ICTEST=%d, PUMS=0x%x\n",val&1,(val&0x3E)/2);
    
#ifdef QSI_NIMBUS_PREDVT_PMIC_DEF 
    val = pmic_reg(slave, 15, 0, 0);
    printf("PMIC: PWR CTRL2 = 0x%x\n",val);
    val &= ~(0xF<<11); // clear CHRCC[3:0]
    //printf("PMIC: Changed CHRCC -> 750mA\n");
    val |= (5<<11);
    val = pmic_reg(slave, 15, val, 1);
    
    /* VGEN1= 0x3, VSD=0x1C0, 20120629,*/
	val = pmic_reg(slave, 30, 0, 0);  //
	val |= 0x3;
	pmic_reg(slave, 30, val, 1);

	val = pmic_reg(slave, 32, 0, 0);  //
	val |= 0x1;
	pmic_reg(slave, 32, val, 1);    

    /* VSDEN: Enable */
	val = pmic_reg(slave, 33, 0, 0);
	val |= (0x1 << 18);
	pmic_reg(slave, 33, val, 1);

    /* STARTCC: Enable */
	val = pmic_reg(slave, 9, 0, 0);
	val |= 0x1;
	pmic_reg(slave, 9, val, 1);

    // johnson add SWBSTEN=1
	val = pmic_reg(slave, 29, 0, 0);
	val |= (0x1<<20);
	pmic_reg(slave, 29, val, 1);

    // johnson add VUSBIN=1
	val = pmic_reg(slave, 50, 0, 0);
	val |= 0x1;
	pmic_reg(slave, 50, val, 1);

    // johnson 20120927 PWRON1BNC = 125ms
    // 0: 0ms
    // 1: 31.5ms
    // 2: 125ms
    // 3: 750ms
	val = pmic_reg(slave, 15, 0, 0);
    val &= ~(0x3<<4);
	val |= (0x2<<4);
	pmic_reg(slave, 15, val, 1);

#ifdef PATCHED_MC13892_BATTERY_20120706
    // for charge checking
    val = pmic_reg(slave, 2, 0, 0);
    if(IsChargerPresence(val))
    {
        printf("<INFO> Found charger!\n");
        val>>=8;
        val&=3;

        switch(val)
        {
            case 0:
                printf("\tNormal!\n");
                break;
            case 1:
                printf("<Warning> Charger source fault!\n");
                break;
            case 2:
                printf("<Warning> battery fault!\n");
                break;
            case 3:
                printf("<Warning> battery temperature!\n");
                break;
        }
    }
   
#endif /*PATCHED_MC13892_BATTERY_20120706*/

#endif /*20120630*/
#endif /*CONFIG_MFG*/
#else  /*defined(CONFIG_MX50_RD3) || defined(CONFIG_QSI_NIMBUS)*/
	val = pmic_reg(slave, 30, 0, 0);
	val |= 0x3;
	pmic_reg(slave, 30, val, 1);

	val = pmic_reg(slave, 32, 0, 0);
	val |= 0x1;
	pmic_reg(slave, 32, val, 1);

	/* Enable VCAM   */
	val = pmic_reg(slave, 33, 0, 0);
	val |= 0x40;
	pmic_reg(slave, 33, val, 1);
#endif /*defined(CONFIG_MX50_RD3)*/

	spi_pmic_free(slave);
}

void setup_voltage_cpu(void)
{
	/* Currently VDDGP 1.05v
	 * no one tell me we need increase the core
	 * voltage to let CPU run at 800Mhz, not do it
	 */

	/* Raise the core frequency to 800MHz */
	writel(0x0, CCM_BASE_ADDR + CLKCTL_CACRR);

}
#endif

#ifdef QSI_NIMBUS_IO_CONFIG_DEF
/*************************************************************
* Nimbus Parameter Definition
**************************************************************/
/* gpio setting */
#define NIMBUS_SD_INSERT_DEF                 MX50_PIN_EIM_CRE     // GPIO1.27, *input
#define NIMBUS_LED_GREEN_WIFI_DEF            MX50_PIN_EPDC_D1     // GPIO3.1, output, high, On:low,OFF:high
#define NIMBUS_LED_GREEN_POWER_DEF           MX50_PIN_EPDC_D2     // GPIO3.2, output, low, On:low,OFF:high
#define NIMBUS_LED_GREEN_DISCHARGE_DEF       MX50_PIN_EPDC_D3     // GPIO3.3, output, high, On:low,OFF:high
#define NIMBUS_LED_RED_CHARGE_DEF            MX50_PIN_EPDC_D4     // GPIO3.4, output, high, On:low,OFF:high
#define NIMBUS_iNAND_RESET_DEF               MX50_PIN_EPDC_D5     // GPIO3.5, output, high, Reset:low
#define NIMBUS_SD_POWER_DEF                  MX50_PIN_EPDC_D6     // GPIO3.6, output, low, On:low,OFF:high
#define NIMBUS_SD_CHG_CLR_DEF                MX50_PIN_EPDC_D8     // GPIO3.8, output, low, Normal:low,Clear:high
#define NIMBUS_SD_CHG_STATUS_DEF             MX50_PIN_EPDC_D9     // GPIO3.9, *input, low, Not_change:low,Change:high
#define NIMBUS_WAKE_UP_LAN_DEF               MX50_PIN_KEY_ROW0    // GPIO4.1, *input, low, Normal:low,WakeUp:high
#define NIMBUS_WIFI_ENABLE_DEF               MX50_PIN_KEY_ROW1    // GPIO4.3, output, high??, OFF;low,On:high
#define NIMBUS_WIFI_POWER_DEF                MX50_PIN_ECSPI2_SCLK // GPIO4-16, output, high, OFF:low,On:high
#define NIMBUS_PWR_KEY_DET_DEF               MX50_PIN_PWM1        // GPIO6-24, *input, low, OFF:low,On:high,          
#define NIMBUS_WDOG_B_DEF                    MX50_PIN_WDOG        // GPIO6-28, output, high, active:low,normal:high
/* pin mask */
#define NIMBUS_SD_INSERT_MASK_DEF            (0X1 << 27)  /* GPIO1.27 */
#define NIMBUS_LED_GREEN_WIFI_MASK_DEF       (0x1 << 1)   /* GPIO3.1 */
#define NIMBUS_LED_GREEN_POWER_MASK_DEF      (0x1 << 2)   /* GPIO3.2 */
#define NIMBUS_LED_GREEN_DISCHARGE_MASK_DEF  (0x1 << 3)   /* GPIO3.3 */
#define NIMBUS_LED_RED_CHARGE_MASK_DEF       (0x1 << 4)   /* GPIO3.4 */
#define NIMBUS_LED_ALL_MASK_DEF              (NIMBUS_LED_GREEN_WIFI_MASK_DEF | NIMBUS_LED_GREEN_POWER_MASK_DEF | \
                                              NIMBUS_LED_RED_CHARGE_MASK_DEF | NIMBUS_LED_GREEN_DISCHARGE_MASK_DEF)
#define NIMBUS_iNAND_RESET_MASK_DEF          (0x1 << 5)   /* GPIO3.5 */                                              
#define NIMBUS_SD_POWER_MASK_DEF             (0x1 << 6)   /* GPIO3.6 */
#define NIMBUS_SD_CHG_CLR_MASK_DEF           (0x1 << 8)   /* GPIO3.8 */
#define NIMBUS_SD_CHG_STATUS_MASK_DEF        (0x1 << 9)   /* GPIO3.9 */
#define NIMBUS_WAKE_UP_LAN_MASK_DEF          (0x1 << 1)   /* GPIO4.1 */
#define NIMBUS_WIFI_ENABLE_MASK_DEF          (0x1 << 3)   /* GPIO4.3 */
#define NIMBUS_WIFI_POWER_MASK_DEF           (0x1 << 16)  /* GPIO4.16 */
#define NIMBUS_PWR_KEY_DET_MASK_DEF          (0x1 << 24)  /* GPIO6.24 */
#define NIMBUS_WDOG_B_MASK_DEF               (0x1 << 28)  /* GPIO6.28 */

/*************************************************************
* purpose: Initialize iomux for LED and 
* params : None.
* log    : Created by Brent.Tan on 20120517
*          Change PWR_KEY_DET & WDOG_B to input-pin
**************************************************************/
static void nimbus_ioconfig_fun(void)
{
  unsigned int reg;

  /* IOMUX setting */
  // GPIO 3
  mxc_request_iomux(NIMBUS_SD_INSERT_DEF, IOMUX_CONFIG_ALT1);           // GPIO3.1, output, On:low,OFF:high
  mxc_request_iomux(NIMBUS_LED_GREEN_WIFI_DEF, IOMUX_CONFIG_ALT1);      // GPIO3.1, output, On:low,OFF:high
  mxc_request_iomux(NIMBUS_LED_GREEN_POWER_DEF, IOMUX_CONFIG_ALT1);     // GPIO3.2, output, On:low,OFF:high
  mxc_request_iomux(NIMBUS_LED_GREEN_DISCHARGE_DEF, IOMUX_CONFIG_ALT1); // GPIO3.3, output, On:low,OFF:high
  mxc_request_iomux(NIMBUS_LED_RED_CHARGE_DEF, IOMUX_CONFIG_ALT1);      // GPIO3.4, output, On:low,OFF:high
  mxc_request_iomux(NIMBUS_iNAND_RESET_DEF, IOMUX_CONFIG_ALT1);         // GPIO3.5, output, Reset:low
  mxc_request_iomux(NIMBUS_SD_POWER_DEF, IOMUX_CONFIG_ALT1);            // GPIO3.6, output, On:low,OFF:high
  mxc_request_iomux(NIMBUS_SD_CHG_CLR_DEF, IOMUX_CONFIG_ALT1);          // GPIO3.8, output, Normal:low,Clear:high
  mxc_request_iomux(NIMBUS_SD_CHG_STATUS_DEF, IOMUX_CONFIG_ALT1);       // GPIO3.9, input, Not_change:low,Change:high
  // GPIO 4
  mxc_request_iomux(NIMBUS_WAKE_UP_LAN_DEF, IOMUX_CONFIG_ALT1);         // GPIO4.1, input, Normal:low,WakeUp:high
  mxc_request_iomux(NIMBUS_WIFI_ENABLE_DEF, IOMUX_CONFIG_ALT1);         // GPIO4.3, output, OFF;low,On:high
  mxc_request_iomux(NIMBUS_WIFI_POWER_DEF, IOMUX_CONFIG_ALT1);          // GPIO4-16, output, OFF:low,On:high
  // GPIO 5
  mxc_request_iomux(NIMBUS_PWR_KEY_DET_DEF, IOMUX_CONFIG_ALT1);         // GPIO6-24, input, OFF:low,On:high
  mxc_request_iomux(NIMBUS_WDOG_B_DEF, IOMUX_CONFIG_ALT1);              // GPIO6-28, input, active:low,normal:high, always high

  /* GPIO1 DIR, 1:gpo, 0:gpi */
  reg = readl(GPIO1_BASE_ADDR + 0x4);
  reg &= ~(NIMBUS_SD_INSERT_MASK_DEF);
  writel(reg, GPIO1_BASE_ADDR + 0x4);

  /* GPIO3 Data */
  reg = readl(GPIO3_BASE_ADDR + 0x0);
  // setting for high
  reg |= (NIMBUS_LED_ALL_MASK_DEF | NIMBUS_iNAND_RESET_MASK_DEF);
  // setting for low 
  reg &= ~(NIMBUS_SD_POWER_MASK_DEF | NIMBUS_SD_CHG_CLR_MASK_DEF);
  writel(reg, GPIO3_BASE_ADDR + 0x0);
  /* GPIO3 DIR, 1:gpo, 0:gpi */
  reg = readl(GPIO3_BASE_ADDR + 0x4);
  // setting for output
  reg |= (NIMBUS_LED_ALL_MASK_DEF | NIMBUS_iNAND_RESET_MASK_DEF | \
          NIMBUS_SD_POWER_MASK_DEF | NIMBUS_SD_CHG_CLR_MASK_DEF);
  // setting for input
  reg &= ~(NIMBUS_SD_CHG_STATUS_MASK_DEF);
  writel(reg, GPIO3_BASE_ADDR + 0x4);
  
  /* GPIO4 Data */
  reg = readl(GPIO4_BASE_ADDR + 0x0);
  reg |= (NIMBUS_WIFI_ENABLE_MASK_DEF | NIMBUS_WIFI_POWER_MASK_DEF);
  writel(reg, GPIO4_BASE_ADDR + 0x0);
  /* GPIO4 DIR, 1:gpo, 0:gpi gpio4_3, gpio4_16*/
  reg = readl(GPIO4_BASE_ADDR + 0x4);
  reg |= (NIMBUS_WIFI_ENABLE_MASK_DEF | NIMBUS_WIFI_POWER_MASK_DEF);
  writel(reg, GPIO4_BASE_ADDR + 0x4);
   
  /* GPIO6 DIR, 1:gpo, 0:gpi */  
  reg = readl(GPIO6_BASE_ADDR + 0x4);
  reg &= ~(NIMBUS_WDOG_B_MASK_DEF | NIMBUS_PWR_KEY_DET_MASK_DEF);
  writel(reg, GPIO6_BASE_ADDR + 0x4);

  return;
}

/*************************************************************
* purpose: LED On/Off switch
* params : LedId: LED #1 ~ #4
*          isOn : 1: on, 0: off
* log    : Created by Brent.Tan on 20120517
**************************************************************/
void nimbus_led_switch_fun(unsigned int LedId, int isOn)
{ 
  unsigned int reg;
  unsigned int mask = 0;
  
  switch(LedId) {
    case 1:
        mask = NIMBUS_LED_GREEN_WIFI_MASK_DEF;
        break;
    case 2:
        mask = NIMBUS_LED_GREEN_POWER_MASK_DEF;
        break;
    case 3:
        mask = NIMBUS_LED_RED_CHARGE_MASK_DEF;
        break;
    case 4:
        mask = NIMBUS_LED_GREEN_DISCHARGE_MASK_DEF;
        break;
    case 5:
        mask = NIMBUS_LED_ALL_MASK_DEF;
        break;
  }

  reg = readl(GPIO3_BASE_ADDR + 0x0);
    
  if(isOn){
    reg &= ~(mask); /* LED: On:low, Off:high */
  }
  else {
    reg |= (mask);    
  }

  writel(reg, GPIO3_BASE_ADDR + 0x0);
  
}

void nimbus_power_led_on()
{
#ifndef CONFIG_MFG

#if 0
    struct spi_slave *slave;
	unsigned int val;

    
	/* Enable VGEN1 to enable ethernet */
	slave = spi_pmic_probe();

    val = pmic_reg(slave, MC13892_REG_ISENSE0, 0, 0);
    if(IsChargerPresence(val))
    {
        nimbus_led_switch_fun(3,0);
        nimbus_led_switch_fun(4,1);
    }
    else
    {
        nimbus_led_switch_fun(3,1);
    }

    spi_pmic_free(slave);
#endif    
#endif
}

#ifdef QSI_NIMBUS_ME_DEMO
#define KEYST_NONE 1
#define KEYST_PRESS 2

void nimbus_check_power_key(void)
{
    struct spi_slave *slave;
    static int key_st;
	unsigned int val,key_found=0;

	slave = spi_pmic_probe();


	/* Set global reset time to 0s*/
	val = pmic_reg(slave, 5, 0, 0); //Interrupt Sense1
    if((val&0x8)==0) // PWNON1S, 1 if high
    {
        key_found=1;
    }

    if(key_st==0 && key_found)
    {
        printf("Power Key pressed!\n");
        key_st=1;
    }

    if(key_st == 1 && key_found == 0)
    {
        unsigned int reg;
        nimbus_led_switch_fun(5, 1);
        printf("Power Key release!\n");
        key_st = 0;
        
        printf("Do Power down...by: USEROFFSPI ON.\n");
        val = pmic_reg(slave, 13, 0, 0); //Power Control 1
        val |= 0x8; //USEROFFSPI & PCEN
        pmic_reg(slave, 13, val, 1);
    }
	spi_pmic_free(slave);
}

void nimbus_led_demo(void)
{
    static ulong led_timer;
    static int toggle;
    ulong tmp;
    
    tmp=get_timer (0);
    if(tmp>led_timer)
    {
        unsigned int reg,leds;
        
        //printf("Timeout: Toggle=%d,%u,%u\n",toggle,tmp,led_timer);
        nimbus_led_switch_fun(5,1);
        
        led_timer=tmp+2*CONFIG_SYS_HZ;
        if(toggle==1)
        {
            toggle=0;
            nimbus_led_switch_fun(3,0);
            
        }
        else
        {
            toggle=1;
            nimbus_led_switch_fun(4,0);
        }
    }
    nimbus_check_power_key();
    
}


#endif  /* QSI_NIMBUS_ME_DEMO */

/*************************************************************
* purpose: nimbus gpio test
* params : 
* log    : Created by Brent.Tan on 20120517
**************************************************************/
void nimbus_gpio_test_fun(unsigned int gpio_port, unsigned int pin_id, unsigned int mux_index,
                                unsigned int pad_index, unsigned int io_mode, int onoff)
{ 
  unsigned int reg, cur_gpio_base_addr, iomux_pin;
  /*printf("MX50_PIN_ECSPI2_SCLK = 0x%8.8x, gpio_port = 0x%8.8x, pin_id= %d\n", MX50_PIN_EPDC_D1, gpio_port, pin_id);*/
  switch(gpio_port) {
   case 1:
     cur_gpio_base_addr = GPIO1_BASE_ADDR;
     break;
   case 2:
     cur_gpio_base_addr = GPIO2_BASE_ADDR;
     break;
   case 3:
     cur_gpio_base_addr = GPIO3_BASE_ADDR;
     break;
   case 4:
     cur_gpio_base_addr = GPIO4_BASE_ADDR;
     break;
   case 5:
     cur_gpio_base_addr = GPIO5_BASE_ADDR;
     break;
   case 6:
     cur_gpio_base_addr = GPIO6_BASE_ADDR;
     break;
  }

  iomux_pin = _MXC_BUILD_PIN((gpio_port - 1), pin_id, 1, mux_index, pad_index);
  
  mxc_request_iomux(iomux_pin, IOMUX_CONFIG_ALT1);
  
  /* Enable gpio */
  if (onoff == 0) { /* off */
    reg = readl(cur_gpio_base_addr + 0x0);
    reg |= (1 << pin_id);
    writel(reg, cur_gpio_base_addr + 0x0);
  }
  else {
    reg = readl(cur_gpio_base_addr + 0x0);
    reg &= ~(1 << pin_id);
    writel(reg, cur_gpio_base_addr + 0x0);    
  }

  /* Set gpi / gpo */
  if (io_mode == 0) {  /* gpi*/
    reg = readl(cur_gpio_base_addr + 0x4);
    reg &= ~(1 << pin_id);
    writel(reg, cur_gpio_base_addr + 0x4);
  }
  else { /* gpo */
    reg = readl(cur_gpio_base_addr + 0x4);
    reg |= (1 << pin_id);
    writel(reg, cur_gpio_base_addr + 0x4);
  }   

  return;
}

/*************************************************************
* purpose: nimbus_is_power_key_pressed
* params : N/A
* return :
*           1- Power key is pressed.
*           0- Nothing.
* log    : Created by Brent.Tan on 20120620
* [20120712] -Revised by Johnson
*           1. Rename function name.
**************************************************************/
int nimbus_is_power_key_pressed(void)
{
    struct spi_slave *slave;
	unsigned int val, key_found = 0;
	slave = spi_pmic_probe();
	/* Set global reset time to 0s*/
	val = pmic_reg(slave, 5, 0, 0); //Interrupt Sense1
    if((val & 0x8) == 0) // PWNON1S, 1 if high
    {
        key_found = 1;
    }
    else
    {
        key_found = 0;
    }
	spi_pmic_free(slave);

    return key_found;    
}

#endif /* QSI_NIMBUS_IO_CONFIG_DEF */

int board_init(void)
{
#ifdef CONFIG_MFG
/* MFG firmware need reset usb to avoid host crash firstly */
#define USBCMD 0x140
	int val = readl(OTG_BASE_ADDR + USBCMD);
	val &= ~0x1; /*RS bit*/
	writel(val, OTG_BASE_ADDR + USBCMD);
#endif
	/* boot device */
	setup_boot_device();

	/* soc rev */
	setup_soc_rev();

	/* board rev */
	setup_board_rev();

	/* arch id for linux */
	setup_arch_id();

	/* boot parameters */
	gd->bd->bi_boot_params = PHYS_SDRAM_1 + 0x100;

	/* iomux for uart */
	setup_uart();

#ifdef CONFIG_MXC_FEC
	/* iomux for fec */
	setup_fec();
#endif

#ifdef CONFIG_NAND_GPMI
	setup_gpmi_nand();
#endif

#ifdef CONFIG_MXC_EPDC
	setup_epdc();
#endif

#ifdef QSI_NIMBUS_IO_CONFIG_DEF
    /* iomux for led & buzzer of Nimbus IO */
    nimbus_ioconfig_fun();
#endif /* QSI_NIMBUS_IO_CONFIG_DEF */

	return 0;
}

#ifdef CONFIG_ANDROID_RECOVERY
struct reco_envs supported_reco_envs[BOOT_DEV_NUM] = {
	{
	 .cmd = NULL,
	 .args = NULL,
	 },
	{
	 .cmd = NULL,
	 .args = NULL,
	 },
	{
	 .cmd = NULL,
	 .args = NULL,
	 },
	{
	 .cmd = NULL,
	 .args = NULL,
	 },
	{
	 .cmd = NULL,
	 .args = NULL,
	 },
	{
	 .cmd = NULL,
	 .args = NULL,
	 },
	{
	 .cmd = CONFIG_ANDROID_RECOVERY_BOOTCMD_MMC,
	 .args = CONFIG_ANDROID_RECOVERY_BOOTARGS_MMC,
	 },
	{
	 .cmd = CONFIG_ANDROID_RECOVERY_BOOTCMD_MMC,
	 .args = CONFIG_ANDROID_RECOVERY_BOOTARGS_MMC,
	 },
	{
	 .cmd = NULL,
	 .args = NULL,
	 },
};

int check_recovery_cmd_file(void)
{
	disk_partition_t info;
	ulong part_length;
	int filelen;

	switch (get_boot_device()) {
	case MMC_BOOT:
	case SD_BOOT:
		{
			block_dev_desc_t *dev_desc = NULL;
			struct mmc *mmc = find_mmc_device(0);

			dev_desc = get_dev("mmc", 0);

			if (NULL == dev_desc) {
				puts("** Block device MMC 0 not supported\n");
				return 0;
			}

			mmc_init(mmc);

			if (get_partition_info(dev_desc,
					CONFIG_ANDROID_CACHE_PARTITION_MMC,
					&info)) {
				printf("** Bad partition %d **\n",
					CONFIG_ANDROID_CACHE_PARTITION_MMC);
				return 0;
			}

			part_length = ext2fs_set_blk_dev(dev_desc,
							CONFIG_ANDROID_CACHE_PARTITION_MMC);
			if (part_length == 0) {
				printf("** Bad partition - mmc 0:%d **\n",
					CONFIG_ANDROID_CACHE_PARTITION_MMC);
				ext2fs_close();
				return 0;
			}

			if (!ext2fs_mount(part_length)) {
				printf("** Bad ext2 partition or disk - mmc 0:%d **\n",
					CONFIG_ANDROID_CACHE_PARTITION_MMC);
				ext2fs_close();
				return 0;
			}

			filelen = ext2fs_open(CONFIG_ANDROID_RECOVERY_CMD_FILE);

			ext2fs_close();
		}
		break;
	case NAND_BOOT:
		return 0;
		break;
	case SPI_NOR_BOOT:
		return 0;
		break;
	case UNKNOWN_BOOT:
	default:
		return 0;
		break;
	}

	return (filelen > 0) ? 1 : 0;

}
#endif

#ifdef QSI_NIMBUS_POWERON_IDENTIFY
int nimbus_is_valid_poweron()
{
    ulong tmp,poweron_timer;
    struct spi_slave *slave;
    unsigned int val,usb_val,i;

    #ifdef  QSI_PWRST_ALIGN
    qwifi_booting_st=0;
    #endif

    if(__REG(SRC_BASE_ADDR + 0x8)!=0x1)
    {
        printf("Not POR booting! <BYPASS>\n");
        nimbus_led_switch_fun(3,1);
        #ifdef  QSI_PWRST_ALIGN
        qwifi_booting_st|=QSI_PWRBY_REBOOT;
        #endif
        return(0);
    }
    i=0;
    slave = spi_pmic_probe();
    tmp=get_timer (0);
    poweron_timer=tmp+QSI_NIMBUS_POWERON_EVENT_VALID_TIME;
    do
    {
        tmp=get_timer (0);
        val = pmic_reg(slave, 5, 0, 0); //Interrupt Sense1

        usb_val = pmic_reg(slave, MC13892_REG_ISENSE0, 0, 0);
        if(IsVBusOn(usb_val))
        {
            if(i++ > 3)
            {
                printf("POR by USB plug-in.\n");
                #ifdef  QSI_PWRST_ALIGN
                qwifi_booting_st|=QSI_PWRBY_USB;
                #endif
                break;
            }
        }
        else if(val & 0x8)
        {
            printf("Invalid power on event!(%d) -> power off.\n",tmp);
            val = pmic_reg(slave, MC13892_REG_POWER_CTL0, 0, 0);
			val |= 0x8;
			pmic_reg(slave,  MC13892_REG_POWER_CTL0, val, 1);
            while(1);
        }
    }while(tmp<poweron_timer);

    #ifdef QSI_PWRST_ALIGN
    if((qwifi_booting_st&QSI_PWRBY_USB)==0)
    {
        qwifi_booting_st|=QSI_PWRBY_KEY;
    }
    #endif

    /* VGEN1= 0x3, VSD=0x1C0, 20120629,*/
	val = pmic_reg(slave, 30, 0, 0);  //
	val |= 0x3;
	pmic_reg(slave, 30, val, 1);
    val = pmic_reg(slave, 32, 0, 0);  //
	val |= 0x1;
	pmic_reg(slave, 32, val, 1); 
    nimbus_led_switch_fun(3,1);

    spi_pmic_free(slave);

    return(0);
}
#endif


int board_late_init(void)
{

#ifdef PATCHED_MC13892_BATTERY_20120706
    #ifdef CONFIG_IMX_CSPI
    check_battery();
    #endif
#endif

#ifdef CONFIG_IMX_CSPI
    setup_power();
#endif




#ifdef QSI_NIMBUS_IO_CONFIG_DEF
    /* iomux for led & buzzer of Nimbus IO */
    nimbus_power_led_on();
#endif /* QSI_NIMBUS_IO_CONFIG_DEF */

	return 0;
}

int checkboard(void)
{
#if defined(CONFIG_QSI_NIMBUS)
    printf("Board: QSI Nimbus board\n");
    #ifdef QSI_NIMBUS_DRAM_16BIT
    printf("Board: DRAM 16 BIT\n");
    #endif
#elif defined(CONFIG_MX50_RDP)
	printf("Board: MX50 RDP board\n");
#elif defined(CONFIG_MX50_RD3)
	printf("Board: MX50 RD3 board\n");
#elif defined(CONFIG_MX50_ARM2)
	printf("Board: MX50 ARM2 board\n");
#else
#	error "Unsupported board!"
#endif

	printf("Boot Reason: [");

	switch (__REG(SRC_BASE_ADDR + 0x8)) {
	case 0x0001:
		printf("POR");
		break;
	case 0x0009:
		printf("RST");
		break;
	case 0x0010:
	case 0x0011:
		printf("WDOG");
		break;
	default:
		printf("unknown");
	}
	printf("]\n");

	printf("Boot Device: ");
	switch (get_boot_device()) {
	case WEIM_NOR_BOOT:
		printf("NOR\n");
		break;
	case ONE_NAND_BOOT:
		printf("ONE NAND\n");
		break;
	case PATA_BOOT:
		printf("PATA\n");
		break;
	case SATA_BOOT:
		printf("SATA\n");
		break;
	case I2C_BOOT:
		printf("I2C\n");
		break;
	case SPI_NOR_BOOT:
		printf("SPI NOR\n");
		break;
	case SD_BOOT:
		printf("SD\n");
		break;
	case MMC_BOOT:
		printf("MMC\n");
		break;
	case NAND_BOOT:
		printf("NAND\n");
		break;
	case UNKNOWN_BOOT:
	default:
		printf("UNKNOWN\n");
		break;
	}

	return 0;
}
