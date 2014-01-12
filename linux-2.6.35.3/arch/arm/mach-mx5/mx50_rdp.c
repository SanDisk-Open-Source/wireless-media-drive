/*
 * Copyright (C) 2010-2011 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <linux/types.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/nodemask.h>
#include <linux/clk.h>
#include <linux/platform_device.h>
#include <linux/fsl_devices.h>
#include <linux/spi/spi.h>
#include <linux/spi/flash.h>
#include <linux/i2c.h>
#include <linux/ata.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/map.h>
#include <linux/mtd/partitions.h>
#include <linux/regulator/consumer.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/fixed.h>
#include <linux/mfd/max17135.h>
#include <linux/pmic_external.h>
#include <linux/pmic_status.h>
#include <linux/videodev2.h>
#include <linux/mxcfb.h>
#include <linux/pwm_backlight.h>
#include <linux/fec.h>
#include <linux/gpmi-nfc.h>
#include <linux/powerkey.h>
#include <asm/irq.h>
#include <asm/setup.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/mach/time.h>
#include <asm/mach/keypad.h>
#include <mach/common.h>
#include <mach/hardware.h>
#include <mach/memory.h>
#include <mach/arc_otg.h>
#include <mach/gpio.h>
#include <mach/mmc.h>
#include <mach/mxc_dvfs.h>
#include <mach/iomux-mx50.h>
#include <mach/i2c.h>

#include "devices.h"
#include "usb.h"
#include "crm_regs.h"
#include "dma-apbh.h"

#define SD1_WP	(3*32 + 19)	/*GPIO_4_19 */
#define SD1_CD	(0*32 + 27)	/*GPIO_1_27 */
#define SD2_WP	(4*32 + 16)	/*GPIO_5_16 */
#define SD2_CD	(4*32 + 17) /*GPIO_5_17 */
#define HP_DETECT	(3*32 + 15)	/*GPIO_4_15 */
#define PWR_INT		(3*32 + 18)	/*GPIO_4_18 */
#define EPDC_D0		(2*32 + 0)	/*GPIO_3_0 */
#define EPDC_D1		(2*32 + 1)	/*GPIO_3_1 */
#define EPDC_D2		(2*32 + 2)	/*GPIO_3_2 */
#define EPDC_D3		(2*32 + 3)	/*GPIO_3_3 */
#define EPDC_D4		(2*32 + 4)	/*GPIO_3_4 */
#define EPDC_D5		(2*32 + 5)	/*GPIO_3_5 */
#define EPDC_D6		(2*32 + 6)	/*GPIO_3_6 */
#define EPDC_D7		(2*32 + 7)	/*GPIO_3_7 */
#define EPDC_GDCLK	(2*32 + 16)	/*GPIO_3_16 */
#define EPDC_GDSP	(2*32 + 17)	/*GPIO_3_17 */
#define EPDC_GDOE	(2*32 + 18)	/*GPIO_3_18 */
#define EPDC_GDRL	(2*32 + 19)	/*GPIO_3_19 */
#define EPDC_SDCLK	(2*32 + 20)	/*GPIO_3_20 */
#define EPDC_SDOEZ	(2*32 + 21)	/*GPIO_3_21 */
#define EPDC_SDOED	(2*32 + 22)	/*GPIO_3_22 */
#define EPDC_SDOE	(2*32 + 23)	/*GPIO_3_23 */
#define EPDC_SDLE	(2*32 + 24)	/*GPIO_3_24 */
#define EPDC_SDCLKN	(2*32 + 25)	/*GPIO_3_25 */
#define EPDC_SDSHR	(2*32 + 26)	/*GPIO_3_26 */
#define EPDC_PWRCOM	(2*32 + 27)	/*GPIO_3_27 */
#define EPDC_PWRSTAT	(2*32 + 28)	/*GPIO_3_28 */
#define EPDC_PWRCTRL0	(2*32 + 29)	/*GPIO_3_29 */
#define EPDC_PWRCTRL1	(2*32 + 30)	/*GPIO_3_30 */
#define EPDC_PWRCTRL2	(2*32 + 31)	/*GPIO_3_31 */
#define EPDC_PWRCTRL3	(3*32 + 20)	/*GPIO_4_20 */
#define EPDC_BDR0	(3*32 + 23)	/*GPIO_4_23 */
#define EPDC_BDR1	(3*32 + 24)	/*GPIO_4_24 */
#define EPDC_SDCE0	(3*32 + 25)	/*GPIO_4_25 */
#define EPDC_SDCE1	(3*32 + 26)	/*GPIO_4_26 */
#define EPDC_SDCE2	(3*32 + 27)	/*GPIO_4_27 */
#define EPDC_SDCE3	(3*32 + 28)	/*GPIO_4_28 */
#define EPDC_SDCE4	(3*32 + 29)	/*GPIO_4_29 */
#define EPDC_SDCE5	(3*32 + 30)	/*GPIO_4_30 */
#define EPDC_PMIC_WAKE		(5*32 + 16)	/*GPIO_6_16 */
#define EPDC_PMIC_INT		(5*32 + 17)	/*GPIO_6_17 */
#define EPDC_VCOM	(3*32 + 21)	/*GPIO_4_21 */
#define EPDC_PWRSTAT	(2*32 + 28)	/*GPIO_3_28 */
#define ELCDIF_PWR_ON (board_is_mx50_rd3() ? (1*32 + 18) : (1*32 + 21))
#define ELCDIF_DAT0_DUMMY	(0*32 + 0)	/*GPIO_1_0 */
#define ELCDIF_DAT1_DUMMY	(0*32 + 1)	/*GPIO_1_1 */
#define ELCDIF_DAT2_DUMMY	(0*32 + 2)	/*GPIO_1_2 */
#define ELCDIF_DAT8_DUMMY	(0*32 + 3)	/*GPIO_1_3 */
#define ELCDIF_DAT9_DUMMY	(0*32 + 4)	/*GPIO_1_4 */
#define ELCDIF_DAT16_DUMMY	(0*32 + 5)	/*GPIO_1_5 */
#define ELCDIF_DAT17_DUMMY	(0*32 + 6)	/*GPIO_1_6 */
#define ELCDIF_DAT18_DUMMY	(0*32 + 7)	/*GPIO_1_7 */
#define CSPI_CS1	(3*32 + 13)	/*GPIO_4_13 */
#define CSPI_CS2	(3*32 + 11) /*GPIO_4_11*/
#define SGTL_OSCEN (5*32 + 8) /*GPIO_6_8*/
#define SGTL_AMP_SHDN		(5*32 + 15) /*GPIO_6_15*/
#define FEC_EN (board_is_mx50_rd3() ? (3*32 + 15) : (5*32 + 23))
#define FEC_RESET_B (3*32 + 12) /*GPIO_4_12*/
#define USB_OTG_PWR	(5*32 + 25) /*GPIO_6_25*/
#define DCDC_EN (3*32 + 16) /*GPIO_4_16*/
#define UART1_RTS (5*32 + 9) /*GPIO_6_9*/
#define UART2_RX (5*32 + 11) /*GPIO_6_11*/
#define HDMI_DETECT		(0*32 + 24)	/* GPIO_1_24 */
#define HDMI_PWR_ENABLE		(0*32 + 25)	/* GPIO_1_25 */
#define HDMI_RESET		(0*32 + 26)	/* GPIO_1_26 */

#define LCD_PWR_EN	(3*32 + 1) /* GPIO_4_1, KEY_ROW0 */

extern int __init mx50_rdp_init_mc13892(void);
extern int __init mx50_rdp_init_mc34708(void);
extern struct cpu_wp *(*get_cpu_wp)(int *wp);
extern void (*set_num_cpu_wp)(int num);
extern struct dvfs_wp *(*get_dvfs_core_wp)(int *wp);
extern int lcdif_sel_lcd;
extern int lcd_seiko_on_j12;
extern void __iomem *apll_base;

static void mx50_suspend_enter(void);
static void mx50_suspend_exit(void);
static void fec_gpio_iomux_init(void);
static void fec_gpio_iomux_deinit(void);

static int max17135_regulator_init(struct max17135 *max17135);
static int num_cpu_wp;

static iomux_v3_cfg_t mx50_rdp[] = {
	/* SD1 */
	MX50_PAD_ECSPI2_SS0__GPIO_4_19,
	MX50_PAD_EIM_CRE__GPIO_1_27,
	MX50_PAD_SD1_CMD__SD1_CMD,

	MX50_PAD_SD1_CLK__SD1_CLK,
	MX50_PAD_SD1_D0__SD1_D0,
	MX50_PAD_SD1_D1__SD1_D1,
	MX50_PAD_SD1_D2__SD1_D2,
	MX50_PAD_SD1_D3__SD1_D3,

	/* SD2 */
	MX50_PAD_SD2_CD__GPIO_5_17,
	MX50_PAD_SD2_WP__GPIO_5_16,
	MX50_PAD_SD2_CMD__SD2_CMD,
	MX50_PAD_SD2_CLK__SD2_CLK,
	MX50_PAD_SD2_D0__SD2_D0,
	MX50_PAD_SD2_D1__SD2_D1,
	MX50_PAD_SD2_D2__SD2_D2,
	MX50_PAD_SD2_D3__SD2_D3,
	MX50_PAD_SD2_D4__SD2_D4,
	MX50_PAD_SD2_D5__SD2_D5,
	MX50_PAD_SD2_D6__SD2_D6,
	MX50_PAD_SD2_D7__SD2_D7,

	/* SD3 */
	MX50_PAD_SD3_CMD__SD3_CMD,
	MX50_PAD_SD3_CLK__SD3_CLK,
	MX50_PAD_SD3_D0__SD3_D0,
	MX50_PAD_SD3_D1__SD3_D1,
	MX50_PAD_SD3_D2__SD3_D2,
	MX50_PAD_SD3_D3__SD3_D3,
	MX50_PAD_SD3_D4__SD3_D4,
	MX50_PAD_SD3_D5__SD3_D5,
	MX50_PAD_SD3_D6__SD3_D6,
	MX50_PAD_SD3_D7__SD3_D7,

	MX50_PAD_SSI_RXD__SSI_RXD,
	MX50_PAD_SSI_TXD__SSI_TXD,
	MX50_PAD_SSI_TXC__SSI_TXC,
	MX50_PAD_SSI_TXFS__SSI_TXFS,

	/* HP_DET_B (headphone detect) */
	MX50_PAD_ECSPI1_SS0__GPIO_4_15,

	/* PWR_INT */
	MX50_PAD_ECSPI2_MISO__GPIO_4_18,

	/* UART pad setting */
	MX50_PAD_UART1_TXD__UART1_TXD,
	MX50_PAD_UART1_RXD__UART1_RXD,
	MX50_PAD_UART1_RTS__UART1_RTS,
	MX50_PAD_UART2_TXD__UART2_TXD,
	MX50_PAD_UART2_RXD__UART2_RXD,
	MX50_PAD_UART2_CTS__UART2_CTS,
	MX50_PAD_UART2_RTS__UART2_RTS,

    MX50_PAD_I2C1_SCL__I2C1_SCL,
	MX50_PAD_I2C1_SDA__I2C1_SDA,
	MX50_PAD_I2C2_SCL__I2C2_SCL,
	MX50_PAD_I2C2_SDA__I2C2_SDA,

	/* EPDC pins */
	MX50_PAD_EPDC_PWRSTAT__GPIO_3_28,
	MX50_PAD_EPDC_VCOM0__GPIO_4_21,
	MX50_PAD_EPDC_PWRCTRL0__GPIO_3_29,

	MX50_PAD_DISP_D8__DISP_D8,
	MX50_PAD_DISP_D9__DISP_D9,
	MX50_PAD_DISP_D10__DISP_D10,
	MX50_PAD_DISP_D11__DISP_D11,
	MX50_PAD_DISP_D12__DISP_D12,
	MX50_PAD_DISP_D13__DISP_D13,
	MX50_PAD_DISP_D14__DISP_D14,
	MX50_PAD_DISP_D15__DISP_D15,
	MX50_PAD_DISP_RS__ELCDIF_VSYNC,

	/* ELCDIF contrast */
	MX50_PAD_PWM1__PWMO,

	/* ELCDIF power on */
	MX50_PAD_DISP_CS__GPIO_2_21,

	MX50_PAD_DISP_BUSY__ELCDIF_HSYNC,
	MX50_PAD_DISP_RD__ELCDIF_EN,
	MX50_PAD_DISP_WR__ELCDIF_PIXCLK,

	/* EPD PMIC WAKEUP */
	MX50_PAD_UART4_TXD__GPIO_6_16,

	/* EPD PMIC intr */
	MX50_PAD_UART4_RXD__GPIO_6_17,

	/* EPD PMIC powerup */
	MX50_PAD_EPDC_PWRCTRL0__GPIO_3_29,

	MX50_PAD_EPITO__USBH1_PWR,
	/* Need to comment below line if
	 * one needs to debug owire.
	 */
	MX50_PAD_OWIRE__USBH1_OC,
	/* using gpio to control otg pwr */
	MX50_PAD_PWM2__GPIO_6_25,
	MX50_PAD_I2C3_SCL__USBOTG_OC,

	MX50_PAD_SSI_RXC__FEC_MDIO,
	MX50_PAD_SSI_RXC__FEC_MDIO,
	MX50_PAD_DISP_D0__FEC_TXCLK,
	MX50_PAD_DISP_D1__FEC_RX_ER,
	MX50_PAD_DISP_D2__FEC_RX_DV,
	MX50_PAD_DISP_D3__FEC_RXD1,
	MX50_PAD_DISP_D4__FEC_RXD0,
	MX50_PAD_DISP_D5__FEC_TX_EN,
	MX50_PAD_DISP_D6__FEC_TXD1,
	MX50_PAD_DISP_D7__FEC_TXD0,
	MX50_PAD_SSI_RXFS__FEC_MDC,
	MX50_PAD_I2C3_SDA__GPIO_6_23,
	MX50_PAD_ECSPI1_SCLK__GPIO_4_12,

	MX50_PAD_CSPI_SS0__CSPI_SS0,
	MX50_PAD_ECSPI1_MOSI__CSPI_SS1,
	MX50_PAD_CSPI_MOSI__CSPI_MOSI,
	MX50_PAD_CSPI_MISO__CSPI_MISO,

	/* SGTL500_OSC_EN */
	MX50_PAD_UART1_CTS__GPIO_6_8,

	/* SGTL_AMP_SHDN */
	MX50_PAD_UART3_RXD__GPIO_6_15,

	/* Keypad */
	MX50_PAD_KEY_COL0__KEY_COL0,
	MX50_PAD_KEY_ROW0__KEY_ROW0,
	MX50_PAD_KEY_COL1__KEY_COL1,
	MX50_PAD_KEY_ROW1__KEY_ROW1,
	MX50_PAD_KEY_COL2__KEY_COL2,
	MX50_PAD_KEY_ROW2__KEY_ROW2,
	MX50_PAD_KEY_COL3__KEY_COL3,
	MX50_PAD_KEY_ROW3__KEY_ROW3,
	MX50_PAD_EIM_DA0__KEY_COL4,
	MX50_PAD_EIM_DA1__KEY_ROW4,
	MX50_PAD_EIM_DA2__KEY_COL5,
	MX50_PAD_EIM_DA3__KEY_ROW5,
	MX50_PAD_EIM_DA4__KEY_COL6,
	MX50_PAD_EIM_DA5__KEY_ROW6,
	MX50_PAD_EIM_DA6__KEY_COL7,
	MX50_PAD_EIM_DA7__KEY_ROW7,
	/*EIM pads */
	MX50_PAD_EIM_DA8__GPIO_1_8,
	MX50_PAD_EIM_DA9__GPIO_1_9,
	MX50_PAD_EIM_DA10__GPIO_1_10,
	MX50_PAD_EIM_DA11__GPIO_1_11,
	MX50_PAD_EIM_DA12__GPIO_1_12,
	MX50_PAD_EIM_DA13__GPIO_1_13,
	MX50_PAD_EIM_DA14__GPIO_1_14,
	MX50_PAD_EIM_DA15__GPIO_1_15,
	MX50_PAD_EIM_CS2__GPIO_1_16,
	MX50_PAD_EIM_CS1__GPIO_1_17,
	MX50_PAD_EIM_CS0__GPIO_1_18,
	MX50_PAD_EIM_EB0__GPIO_1_19,
	MX50_PAD_EIM_EB1__GPIO_1_20,
	MX50_PAD_EIM_WAIT__GPIO_1_21,
	MX50_PAD_EIM_BCLK__GPIO_1_22,
	MX50_PAD_EIM_RDY__GPIO_1_23,
	MX50_PAD_EIM_OE__GPIO_1_24,
	MX50_PAD_EIM_RW__GPIO_1_25,
	MX50_PAD_EIM_LBA__GPIO_1_26,
};

/* POWER_EN and DISP_VSYNC pin switched */
static iomux_v3_cfg_t mx50_rd3_adjust[] = {
	MX50_PAD_DISP_CS__ELCDIF_HSYNC,
	MX50_PAD_DISP_BUSY__GPIO_2_18,
	MX50_PAD_UART1_RTS__GPIO_6_9,	/* SD2 VDD */
	MX50_PAD_UART2_RXD__GPIO_6_11,
};

static iomux_v3_cfg_t mx50_gpmi_nand[] = {
	MX50_PIN_EIM_DA8__NANDF_CLE,
	MX50_PIN_EIM_DA9__NANDF_ALE,
	MX50_PIN_EIM_DA10__NANDF_CE0,
	MX50_PIN_EIM_DA11__NANDF_CE1,
	MX50_PIN_EIM_DA12__NANDF_CE2,
	MX50_PIN_EIM_DA13__NANDF_CE3,
	MX50_PIN_EIM_DA14__NANDF_READY,
	MX50_PIN_EIM_DA15__NANDF_DQS,
	MX50_PIN_SD3_D4__NANDF_D0,
	MX50_PIN_SD3_D5__NANDF_D1,
	MX50_PIN_SD3_D6__NANDF_D2,
	MX50_PIN_SD3_D7__NANDF_D3,
	MX50_PIN_SD3_D0__NANDF_D4,
	MX50_PIN_SD3_D1__NANDF_D5,
	MX50_PIN_SD3_D2__NANDF_D6,
	MX50_PIN_SD3_D3__NANDF_D7,
	MX50_PIN_SD3_CLK__NANDF_RDN,
	MX50_PIN_SD3_CMD__NANDF_WRN,
	MX50_PIN_SD3_WP__NANDF_RESETN,
};

static iomux_v3_cfg_t suspend_enter_pads[] = {
	MX50_PAD_EIM_DA0__GPIO_1_0,
	MX50_PAD_EIM_DA1__GPIO_1_1,
	MX50_PAD_EIM_DA2__GPIO_1_2,
	MX50_PAD_EIM_DA3__GPIO_1_3,
	MX50_PAD_EIM_DA4__GPIO_1_4,
	MX50_PAD_EIM_DA5__GPIO_1_5,
	MX50_PAD_EIM_DA6__GPIO_1_6,
	MX50_PAD_EIM_DA7__GPIO_1_7,

	MX50_PAD_EIM_DA8__GPIO_1_8,
	MX50_PAD_EIM_DA9__GPIO_1_9,
	MX50_PAD_EIM_DA10__GPIO_1_10,
	MX50_PAD_EIM_DA11__GPIO_1_11,
	MX50_PAD_EIM_DA12__GPIO_1_12,
	MX50_PAD_EIM_DA13__GPIO_1_13,
	MX50_PAD_EIM_DA14__GPIO_1_14,
	MX50_PAD_EIM_DA15__GPIO_1_15,
	MX50_PAD_EIM_CS2__GPIO_1_16,
	MX50_PAD_EIM_CS1__GPIO_1_17,
	MX50_PAD_EIM_CS0__GPIO_1_18,
	MX50_PAD_EIM_EB0__GPIO_1_19,
	MX50_PAD_EIM_EB1__GPIO_1_20,
	MX50_PAD_EIM_WAIT__GPIO_1_21,
	MX50_PAD_EIM_BCLK__GPIO_1_22,
	MX50_PAD_EIM_RDY__GPIO_1_23,
	MX50_PAD_EIM_OE__GPIO_1_24,
	MX50_PAD_EIM_RW__GPIO_1_25,
	MX50_PAD_EIM_LBA__GPIO_1_26,
	MX50_PAD_EIM_CRE__GPIO_1_27,

	/* NVCC_NANDF pads */
	MX50_PAD_DISP_D8__GPIO_2_8,
	MX50_PAD_DISP_D9__GPIO_2_9,
	MX50_PAD_DISP_D10__GPIO_2_10,
	MX50_PAD_DISP_D11__GPIO_2_11,
	MX50_PAD_DISP_D12__GPIO_2_12,
	MX50_PAD_DISP_D13__GPIO_2_13,
	MX50_PAD_DISP_D14__GPIO_2_14,
	MX50_PAD_DISP_D15__GPIO_2_15,
	MX50_PAD_SD3_CMD__GPIO_5_18,
	MX50_PAD_SD3_CLK__GPIO_5_19,
	MX50_PAD_SD3_D0__GPIO_5_20,
	MX50_PAD_SD3_D1__GPIO_5_21,
	MX50_PAD_SD3_D2__GPIO_5_22,
	MX50_PAD_SD3_D3__GPIO_5_23,
	MX50_PAD_SD3_D4__GPIO_5_24,
	MX50_PAD_SD3_D5__GPIO_5_25,
	MX50_PAD_SD3_D6__GPIO_5_26,
	MX50_PAD_SD3_D7__GPIO_5_27,
	MX50_PAD_SD3_WP__GPIO_5_28,

	/* NVCC_LCD pads */
	MX50_PAD_DISP_D0__GPIO_2_0,
	MX50_PAD_DISP_D1__GPIO_2_1,
	MX50_PAD_DISP_D2__GPIO_2_2,
	MX50_PAD_DISP_D3__GPIO_2_3,
	MX50_PAD_DISP_D4__GPIO_2_4,
	MX50_PAD_DISP_D5__GPIO_2_5,
	MX50_PAD_DISP_D6__GPIO_2_6,
	MX50_PAD_DISP_D7__GPIO_2_7,
	MX50_PAD_DISP_WR__GPIO_2_16,
	MX50_PAD_DISP_RS__GPIO_2_17,
	MX50_PAD_DISP_BUSY__GPIO_2_18,
	MX50_PAD_DISP_RD__GPIO_2_19,
	MX50_PAD_DISP_RESET__GPIO_2_20,
	MX50_PAD_DISP_CS__GPIO_2_21,

	/* CSPI pads */
	MX50_PAD_CSPI_SCLK__GPIO_4_8,
	MX50_PAD_CSPI_MOSI__GPIO_4_9,
	MX50_PAD_CSPI_MISO__GPIO_4_10,
	MX50_PAD_CSPI_SS0__GPIO_4_11,

	/*NVCC_MISC pins as GPIO */
	MX50_PAD_I2C1_SCL__GPIO_6_18,
	MX50_PAD_I2C1_SDA__GPIO_6_19,
	MX50_PAD_I2C2_SCL__GPIO_6_20,
	MX50_PAD_I2C2_SDA__GPIO_6_21,
	MX50_PAD_I2C3_SCL__GPIO_6_22,
	MX50_PAD_I2C3_SDA__GPIO_6_23,

	/* NVCC_MISC_PWM_USB_OTG pins */
	MX50_PAD_PWM1__GPIO_6_24,
	MX50_PAD_PWM2__GPIO_6_25,
	MX50_PAD_EPITO__GPIO_6_27,
	MX50_PAD_WDOG__GPIO_6_28,

	/* FEC related. */
	MX50_PAD_EPDC_D10__GPIO_3_10,
	MX50_PAD_SSI_RXC__GPIO_6_5,
	MX50_PAD_SSI_RXFS__GPIO_6_4,
};

static iomux_v3_cfg_t suspend_exit_pads[ARRAY_SIZE(suspend_enter_pads)];

static struct mxc_dvfs_platform_data dvfs_core_data = {
	.reg_id = "SW1",
	.clk1_id = "cpu_clk",
	.clk2_id = "gpc_dvfs_clk",
	.gpc_cntr_offset = MXC_GPC_CNTR_OFFSET,
	.gpc_vcr_offset = MXC_GPC_VCR_OFFSET,
	.ccm_cdcr_offset = MXC_CCM_CDCR_OFFSET,
	.ccm_cacrr_offset = MXC_CCM_CACRR_OFFSET,
	.ccm_cdhipr_offset = MXC_CCM_CDHIPR_OFFSET,
	.prediv_mask = 0x1F800,
	.prediv_offset = 11,
	.prediv_val = 3,
	.div3ck_mask = 0xE0000000,
	.div3ck_offset = 29,
	.div3ck_val = 2,
	.emac_val = 0x08,
	.upthr_val = 25,
	.dnthr_val = 9,
	.pncthr_val = 33,
	.upcnt_val = 10,
	.dncnt_val = 10,
	.delay_time = 80,
};

static struct mxc_bus_freq_platform_data bus_freq_data = {
	.gp_reg_id = "SW1",
	.lp_reg_id = "SW2",
};

static struct dvfs_wp dvfs_core_setpoint[] = {
	{33, 13, 33, 10, 10, 0x08}, /* 800MHz*/
	{28, 8, 33, 10, 10, 0x08},   /* 400MHz */
	{20, 0, 33, 20, 10, 0x08},   /* 160MHz*/
	{28, 8, 33, 20, 30, 0x08},   /*160MHz, AHB 133MHz, LPAPM mode*/
	{29, 0, 33, 20, 10, 0x08},}; /* 160MHz, AHB 24MHz */

/* working point(wp): 0 - 800MHz; 1 - 400MHz, 2 - 160MHz; */
static struct cpu_wp cpu_wp_auto[] = {
	{
	 .pll_rate = 800000000,
	 .cpu_rate = 800000000,
	 .pdf = 0,
	 .mfi = 8,
	 .mfd = 2,
	 .mfn = 1,
	 .cpu_podf = 0,
	 .cpu_voltage = 1050000,},
	{
	 .pll_rate = 800000000,
	 .cpu_rate = 400000000,
	 .cpu_podf = 1,
	 .cpu_voltage = 1050000,},
	{
	 .pll_rate = 800000000,
	 .cpu_rate = 160000000,
	 .cpu_podf = 4,
	 .cpu_voltage = 850000,},
};

static struct dvfs_wp *mx50_rdp_get_dvfs_core_table(int *wp)
{
	*wp = ARRAY_SIZE(dvfs_core_setpoint);
	return dvfs_core_setpoint;
}

static struct cpu_wp *mx50_rdp_get_cpu_wp(int *wp)
{
	*wp = num_cpu_wp;
	return cpu_wp_auto;
}

static void mx50_rdp_set_num_cpu_wp(int num)
{
	num_cpu_wp = num;
	return;
}

static struct mxc_w1_config mxc_w1_data = {
	.search_rom_accelerator = 1,
};

static struct fec_platform_data fec_data = {
	.phy = PHY_INTERFACE_MODE_RMII,
};

static u16 keymapping[] = {
	KEY_9, KEY_0, KEY_O, KEY_P, KEY_L, KEY_DELETE, KEY_SLASH, KEY_ENTER,
	KEY_F4, KEY_F1, KEY_F6, KEY_F9, KEY_F5, KEY_BACKSPACE, KEY_MENU, 0,
	KEY_PREVIOUS, KEY_NEXT, KEY_HOME, KEY_NEXT, KEY_F2, KEY_F3, KEY_F8, KEY_F7,
	KEY_F11, KEY_CAPSLOCK, KEY_SPACE, KEY_SPACE, KEY_LEFTALT, KEY_LEFTSHIFT, 0, 0,
	KEY_COMMA, KEY_M, KEY_N, KEY_B, KEY_V, KEY_C, KEY_X, KEY_Z,
	KEY_K, KEY_J, KEY_H, KEY_G, KEY_F, KEY_D, KEY_S, KEY_A,
	KEY_I, KEY_U, KEY_Y, KEY_T, KEY_R, KEY_E, KEY_W, KEY_Q,
	KEY_8, KEY_7, KEY_6, KEY_5, KEY_4, KEY_3, KEY_2, KEY_1
};

static struct keypad_data keypad_plat_data = {
	.rowmax = 8,
	.colmax = 8,
	.irq = MXC_INT_KPP,
	.learning = 0,
	.delay = 2,
	.matrix = keymapping,
};


/* workaround for cspi chipselect pin may not keep correct level when idle */
static void mx50_rdp_gpio_spi_chipselect_active(int cspi_mode, int status,
					     int chipselect)
{
	switch (cspi_mode) {
	case 1:
		break;
	case 2:
		break;
	case 3:
		switch (chipselect) {
		case 0x1:
			{
			iomux_v3_cfg_t cspi_ss0 = MX50_PAD_CSPI_SS0__CSPI_SS0;
			iomux_v3_cfg_t cspi_cs1 = MX50_PAD_ECSPI1_MOSI__GPIO_4_13;

			/* pull up/down deassert it */
			mxc_iomux_v3_setup_pad(cspi_ss0);
			mxc_iomux_v3_setup_pad(cspi_cs1);

			gpio_request(CSPI_CS1, "cspi-cs1");
			gpio_direction_input(CSPI_CS1);
			}
			break;
		case 0x2:
			{
			iomux_v3_cfg_t cspi_ss1 = MX50_PAD_ECSPI1_MOSI__CSPI_SS1;
			iomux_v3_cfg_t cspi_ss0 = MX50_PAD_CSPI_SS0__GPIO_4_11;

			/*disable other ss */
			mxc_iomux_v3_setup_pad(cspi_ss1);
			mxc_iomux_v3_setup_pad(cspi_ss0);

			/* pull up/down deassert it */
			gpio_request(CSPI_CS2, "cspi-cs2");
			gpio_direction_input(CSPI_CS2);
			}
			break;
		default:
			break;
		}
		break;

	default:
		break;
	}
}

static void mx50_rdp_gpio_spi_chipselect_inactive(int cspi_mode, int status,
					       int chipselect)
{
	switch (cspi_mode) {
	case 1:
		break;
	case 2:
		break;
	case 3:
		switch (chipselect) {
		case 0x1:
			gpio_free(CSPI_CS1);
			break;
		case 0x2:
			gpio_free(CSPI_CS2);
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}

}

static struct mxc_spi_master mxcspi1_data = {
	.maxchipselect = 4,
	.spi_version = 23,
	.chipselect_active = mx50_rdp_gpio_spi_chipselect_active,
	.chipselect_inactive = mx50_rdp_gpio_spi_chipselect_inactive,
};

static struct mxc_spi_master mxcspi3_data = {
	.maxchipselect = 4,
	.spi_version = 7,
	.chipselect_active = mx50_rdp_gpio_spi_chipselect_active,
	.chipselect_inactive = mx50_rdp_gpio_spi_chipselect_inactive,
};

#define mV_to_uV(mV) (mV * 1000)
#define uV_to_mV(uV) (uV / 1000)
#define V_to_uV(V) (mV_to_uV(V * 1000))
#define uV_to_V(uV) (uV_to_mV(uV) / 1000)

static struct regulator_init_data max17135_init_data[] = {
	{
		.constraints = {
			.name = "DISPLAY",
			.valid_ops_mask =  REGULATOR_CHANGE_STATUS,
		},
	}, {
		.constraints = {
			.name = "GVDD",
			.min_uV = V_to_uV(20),
			.max_uV = V_to_uV(20),
		},
	}, {
		.constraints = {
			.name = "GVEE",
			.min_uV = V_to_uV(-22),
			.max_uV = V_to_uV(-22),
		},
	}, {
		.constraints = {
			.name = "HVINN",
			.min_uV = V_to_uV(-22),
			.max_uV = V_to_uV(-22),
		},
	}, {
		.constraints = {
			.name = "HVINP",
			.min_uV = V_to_uV(20),
			.max_uV = V_to_uV(20),
		},
	}, {
		.constraints = {
			.name = "VCOM",
			.min_uV = mV_to_uV(-4325),
			.max_uV = mV_to_uV(-500),
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE |
			REGULATOR_CHANGE_STATUS,
		},
	}, {
		.constraints = {
			.name = "VNEG",
			.min_uV = V_to_uV(-15),
			.max_uV = V_to_uV(-15),
		},
	}, {
		.constraints = {
			.name = "VPOS",
			.min_uV = V_to_uV(15),
			.max_uV = V_to_uV(15),
		},
	}, {
		.constraints = {
			.name = "V3P3",
			.valid_ops_mask =  REGULATOR_CHANGE_STATUS,
		},
	},
};

/* Fixed voltage regulator DCDC_3V15 */
static struct regulator_consumer_supply fixed_volt_reg_consumers[] = {
	{
		/* sgtl5000 */
		.supply = "VDDIO",
		.dev_name = "1-000a",
	},
};

static struct regulator_init_data fixed_volt_reg_init_data = {
	.constraints = {
		.always_on = 1,
	},
	.num_consumer_supplies = ARRAY_SIZE(fixed_volt_reg_consumers),
	.consumer_supplies = fixed_volt_reg_consumers,
};

static struct fixed_voltage_config fixed_volt_reg_pdata = {
	.supply_name = "DCDC_3V15",
	.microvolts = 3150000,
	.init_data = &fixed_volt_reg_init_data,
	.gpio = -EINVAL,
};

static int epdc_get_pins(void)
{
	int ret = 0;

	/* Claim GPIOs for EPDC pins - used during power up/down */
	ret |= gpio_request(EPDC_D0, "epdc_d0");
	ret |= gpio_request(EPDC_D1, "epdc_d1");
	ret |= gpio_request(EPDC_D2, "epdc_d2");
	ret |= gpio_request(EPDC_D3, "epdc_d3");
	ret |= gpio_request(EPDC_D4, "epdc_d4");
	ret |= gpio_request(EPDC_D5, "epdc_d5");
	ret |= gpio_request(EPDC_D6, "epdc_d6");
	ret |= gpio_request(EPDC_D7, "epdc_d7");
	ret |= gpio_request(EPDC_GDCLK, "epdc_gdclk");
	ret |= gpio_request(EPDC_GDSP, "epdc_gdsp");
	ret |= gpio_request(EPDC_GDOE, "epdc_gdoe");
	ret |= gpio_request(EPDC_GDRL, "epdc_gdrl");
	ret |= gpio_request(EPDC_SDCLK, "epdc_sdclk");
	ret |= gpio_request(EPDC_SDOE, "epdc_sdoe");
	ret |= gpio_request(EPDC_SDLE, "epdc_sdle");
	ret |= gpio_request(EPDC_SDSHR, "epdc_sdshr");
	ret |= gpio_request(EPDC_BDR0, "epdc_bdr0");
	ret |= gpio_request(EPDC_SDCE0, "epdc_sdce0");
	ret |= gpio_request(EPDC_SDCE1, "epdc_sdce1");
	ret |= gpio_request(EPDC_SDCE2, "epdc_sdce2");

	return ret;
}

static void epdc_put_pins(void)
{
	gpio_free(EPDC_D0);
	gpio_free(EPDC_D1);
	gpio_free(EPDC_D2);
	gpio_free(EPDC_D3);
	gpio_free(EPDC_D4);
	gpio_free(EPDC_D5);
	gpio_free(EPDC_D6);
	gpio_free(EPDC_D7);
	gpio_free(EPDC_GDCLK);
	gpio_free(EPDC_GDSP);
	gpio_free(EPDC_GDOE);
	gpio_free(EPDC_GDRL);
	gpio_free(EPDC_SDCLK);
	gpio_free(EPDC_SDOE);
	gpio_free(EPDC_SDLE);
	gpio_free(EPDC_SDSHR);
	gpio_free(EPDC_BDR0);
	gpio_free(EPDC_SDCE0);
	gpio_free(EPDC_SDCE1);
	gpio_free(EPDC_SDCE2);
}

static iomux_v3_cfg_t mx50_epdc_pads_enabled[] = {
	MX50_PAD_EPDC_D0__EPDC_D0,
	MX50_PAD_EPDC_D1__EPDC_D1,
	MX50_PAD_EPDC_D2__EPDC_D2,
	MX50_PAD_EPDC_D3__EPDC_D3,
	MX50_PAD_EPDC_D4__EPDC_D4,
	MX50_PAD_EPDC_D5__EPDC_D5,
	MX50_PAD_EPDC_D6__EPDC_D6,
	MX50_PAD_EPDC_D7__EPDC_D7,
	MX50_PAD_EPDC_GDCLK__EPDC_GDCLK,
	MX50_PAD_EPDC_GDSP__EPDC_GDSP,
	MX50_PAD_EPDC_GDOE__EPDC_GDOE,
	MX50_PAD_EPDC_GDRL__EPDC_GDRL,
	MX50_PAD_EPDC_SDCLK__EPDC_SDCLK,
	MX50_PAD_EPDC_SDOE__EPDC_SDOE,
	MX50_PAD_EPDC_SDLE__EPDC_SDLE,
	MX50_PAD_EPDC_SDSHR__EPDC_SDSHR,
	MX50_PAD_EPDC_BDR0__EPDC_BDR0,
	MX50_PAD_EPDC_SDCE0__EPDC_SDCE0,
	MX50_PAD_EPDC_SDCE1__EPDC_SDCE1,
	MX50_PAD_EPDC_SDCE2__EPDC_SDCE2,
};

static iomux_v3_cfg_t mx50_epdc_pads_disabled[] = {
	MX50_PAD_EPDC_D0__GPIO_3_0,
	MX50_PAD_EPDC_D1__GPIO_3_1,
	MX50_PAD_EPDC_D2__GPIO_3_2,
	MX50_PAD_EPDC_D3__GPIO_3_3,
	MX50_PAD_EPDC_D4__GPIO_3_4,
	MX50_PAD_EPDC_D5__GPIO_3_5,
	MX50_PAD_EPDC_D6__GPIO_3_6,
	MX50_PAD_EPDC_D7__GPIO_3_7,
	MX50_PAD_EPDC_GDCLK__GPIO_3_16,
	MX50_PAD_EPDC_GDSP__GPIO_3_17,
	MX50_PAD_EPDC_GDOE__GPIO_3_18,
	MX50_PAD_EPDC_GDRL__GPIO_3_19,
	MX50_PAD_EPDC_SDCLK__GPIO_3_20,
	MX50_PAD_EPDC_SDOE__GPIO_3_23,
	MX50_PAD_EPDC_SDLE__GPIO_3_24,
	MX50_PAD_EPDC_SDSHR__GPIO_3_26,
	MX50_PAD_EPDC_BDR0__GPIO_4_23,
	MX50_PAD_EPDC_SDCE0__GPIO_4_25,
	MX50_PAD_EPDC_SDCE1__GPIO_4_26,
	MX50_PAD_EPDC_SDCE2__GPIO_4_27,
};

static void epdc_enable_pins(void)
{
	/* Configure MUX settings to enable EPDC use */
	mxc_iomux_v3_setup_multiple_pads(mx50_epdc_pads_enabled, \
				ARRAY_SIZE(mx50_epdc_pads_enabled));

	gpio_direction_input(EPDC_D0);
	gpio_direction_input(EPDC_D1);
	gpio_direction_input(EPDC_D2);
	gpio_direction_input(EPDC_D3);
	gpio_direction_input(EPDC_D4);
	gpio_direction_input(EPDC_D5);
	gpio_direction_input(EPDC_D6);
	gpio_direction_input(EPDC_D7);
	gpio_direction_input(EPDC_GDCLK);
	gpio_direction_input(EPDC_GDSP);
	gpio_direction_input(EPDC_GDOE);
	gpio_direction_input(EPDC_GDRL);
	gpio_direction_input(EPDC_SDCLK);
	gpio_direction_input(EPDC_SDOE);
	gpio_direction_input(EPDC_SDLE);
	gpio_direction_input(EPDC_SDSHR);
	gpio_direction_input(EPDC_BDR0);
	gpio_direction_input(EPDC_SDCE0);
	gpio_direction_input(EPDC_SDCE1);
	gpio_direction_input(EPDC_SDCE2);
}

static void epdc_disable_pins(void)
{
	/* Configure MUX settings for EPDC pins to
	 * GPIO and drive to 0. */
	mxc_iomux_v3_setup_multiple_pads(mx50_epdc_pads_disabled, \
				ARRAY_SIZE(mx50_epdc_pads_disabled));

	gpio_direction_output(EPDC_D0, 0);
	gpio_direction_output(EPDC_D1, 0);
	gpio_direction_output(EPDC_D2, 0);
	gpio_direction_output(EPDC_D3, 0);
	gpio_direction_output(EPDC_D4, 0);
	gpio_direction_output(EPDC_D5, 0);
	gpio_direction_output(EPDC_D6, 0);
	gpio_direction_output(EPDC_D7, 0);
	gpio_direction_output(EPDC_GDCLK, 0);
	gpio_direction_output(EPDC_GDSP, 0);
	gpio_direction_output(EPDC_GDOE, 0);
	gpio_direction_output(EPDC_GDRL, 0);
	gpio_direction_output(EPDC_SDCLK, 0);
	gpio_direction_output(EPDC_SDOE, 0);
	gpio_direction_output(EPDC_SDLE, 0);
	gpio_direction_output(EPDC_SDSHR, 0);
	gpio_direction_output(EPDC_BDR0, 0);
	gpio_direction_output(EPDC_SDCE0, 0);
	gpio_direction_output(EPDC_SDCE1, 0);
	gpio_direction_output(EPDC_SDCE2, 0);
}

static struct fb_videomode e60_v110_mode = {
	.name = "E60_V110",
	.refresh = 50,
	.xres = 800,
	.yres = 600,
	.pixclock = 18604700,
	.left_margin = 8,
	.right_margin = 178,
	.upper_margin = 4,
	.lower_margin = 10,
	.hsync_len = 20,
	.vsync_len = 4,
	.sync = 0,
	.vmode = FB_VMODE_NONINTERLACED,
	.flag = 0,
};

static struct fb_videomode e60_v220_mode = {
	.name = "E60_V220",
	.refresh = 85,
	.xres = 800,
	.yres = 600,
	.pixclock = 30000000,
	.left_margin = 8,
	.right_margin = 164,
	.upper_margin = 4,
	.lower_margin = 8,
	.hsync_len = 4,
	.vsync_len = 1,
	.sync = 0,
	.vmode = FB_VMODE_NONINTERLACED,
	.flag = 0,
};

static struct fb_videomode e97_v110_mode = {
	.name = "E97_V110",
	.refresh = 50,
	.xres = 1200,
	.yres = 825,
	.pixclock = 32000000,
	.left_margin = 12,
	.right_margin = 128,
	.upper_margin = 4,
	.lower_margin = 10,
	.hsync_len = 20,
	.vsync_len = 4,
	.sync = 0,
	.vmode = FB_VMODE_NONINTERLACED,
	.flag = 0,
};

static struct mxc_epdc_fb_mode panel_modes[] = {
	{
		&e60_v110_mode,
		4,	/* vscan_holdoff */
		10,	/* sdoed_width */
		20,	/* sdoed_delay */
		10,	/* sdoez_width */
		20,	/* sdoez_delay */
		428,	/* gdclk_hp_offs */
		20,	/* gdsp_offs */
		0,	/* gdoe_offs */
		1,	/* gdclk_offs */
		1,	/* num_ce */
	},
	{
		&e60_v220_mode,
		4,	/* vscan_holdoff */
		10,	/* sdoed_width */
		20,	/* sdoed_delay */
		10,	/* sdoez_width */
		20,	/* sdoez_delay */
		465,	/* gdclk_hp_offs */
		20,	/* gdsp_offs */
		0,	/* gdoe_offs */
		9,	/* gdclk_offs */
		1,	/* num_ce */
	},
	{
		&e97_v110_mode,
		8,	/* vscan_holdoff */
		10,	/* sdoed_width */
		20,	/* sdoed_delay */
		10,	/* sdoez_width */
		20,	/* sdoez_delay */
		632,	/* gdclk_hp_offs */
		20,	/* gdsp_offs */
		0,	/* gdoe_offs */
		1,	/* gdclk_offs */
		3,	/* num_ce */
	}
};

static struct mxc_epdc_fb_platform_data epdc_data = {
	.epdc_mode = panel_modes,
	.num_modes = ARRAY_SIZE(panel_modes),
	.get_pins = epdc_get_pins,
	.put_pins = epdc_put_pins,
	.enable_pins = epdc_enable_pins,
	.disable_pins = epdc_disable_pins,
};

static struct platform_device max17135_sensor_device = {
	.name = "max17135_sensor",
	.id = 0,
};

static struct max17135_platform_data max17135_pdata __initdata = {
	.vneg_pwrup = 1,
	.gvee_pwrup = 1,
	.vpos_pwrup = 2,
	.gvdd_pwrup = 1,
	.gvdd_pwrdn = 1,
	.vpos_pwrdn = 2,
	.gvee_pwrdn = 1,
	.vneg_pwrdn = 1,
	.gpio_pmic_pwrgood = EPDC_PWRSTAT,
	.gpio_pmic_vcom_ctrl = EPDC_VCOM,
	.gpio_pmic_wakeup = EPDC_PMIC_WAKE,
	.gpio_pmic_v3p3 = EPDC_PWRCTRL0,
	.gpio_pmic_intr = EPDC_PMIC_INT,
	.regulator_init = max17135_init_data,
	.init = max17135_regulator_init,
};

static int max17135_regulator_init(struct max17135 *max17135)
{
	struct max17135_platform_data *pdata = &max17135_pdata;
	int i, ret;

	max17135->gvee_pwrup = pdata->gvee_pwrup;
	max17135->vneg_pwrup = pdata->vneg_pwrup;
	max17135->vpos_pwrup = pdata->vpos_pwrup;
	max17135->gvdd_pwrup = pdata->gvdd_pwrup;
	max17135->gvdd_pwrdn = pdata->gvdd_pwrdn;
	max17135->vpos_pwrdn = pdata->vpos_pwrdn;
	max17135->vneg_pwrdn = pdata->vneg_pwrdn;
	max17135->gvee_pwrdn = pdata->gvee_pwrdn;

	max17135->max_wait = pdata->vpos_pwrup + pdata->vneg_pwrup +
		pdata->gvdd_pwrup + pdata->gvee_pwrup;

	max17135->gpio_pmic_pwrgood = pdata->gpio_pmic_pwrgood;
	max17135->gpio_pmic_vcom_ctrl = pdata->gpio_pmic_vcom_ctrl;
	max17135->gpio_pmic_wakeup = pdata->gpio_pmic_wakeup;
	max17135->gpio_pmic_v3p3 = pdata->gpio_pmic_v3p3;
	max17135->gpio_pmic_intr = pdata->gpio_pmic_intr;

	max17135->vcom_setup = false;
	max17135->init_done = false;

	for (i = 0; i < MAX17135_NUM_REGULATORS; i++) {
		ret = max17135_register_regulator(max17135, i,
			&pdata->regulator_init[i]);
		if (ret != 0) {
			printk(KERN_ERR"max17135 regulator init failed: %d\n",
				ret);
			return ret;
		}
	}

	regulator_has_full_constraints();

	return 0;
}

static struct imxi2c_platform_data mxci2c_data = {
       .bitrate = 100000,
};

static struct i2c_board_info mxc_i2c0_board_info[] __initdata = {
	{
	 I2C_BOARD_INFO("max17135", 0x48),
	 .platform_data = &max17135_pdata,
	 },
	 {
	 .type = "mma8450",
	 .addr = 0x1c,
	 },
	 {
	 .type = "eeprom",
	 .addr = 0x50,
	 },
};

static void sii902x_hdmi_reset(void)
{
	gpio_set_value(HDMI_PWR_ENABLE, 1);
	msleep(10);
	gpio_set_value(HDMI_RESET, 0);
	msleep(10);
	gpio_set_value(HDMI_RESET, 1);
	msleep(10);
}

static int sii902x_hdmi_get_pins(void)
{
	int ret = 0;

	ret |= gpio_request(HDMI_RESET, "hdmi_reset");
	ret |= gpio_request(HDMI_DETECT, "hdmi_detect");
	ret |= gpio_request(EPDC_SDCE5, "hdmi_d0");
	ret |= gpio_request(EPDC_SDCE4, "hdmi_d1");
	ret |= gpio_request(EPDC_SDCE3, "hdmi_d2");
	ret |= gpio_request(EPDC_SDCE2, "hdmi_d3");
	ret |= gpio_request(EPDC_SDCE1, "hdmi_d4");
	ret |= gpio_request(EPDC_SDCE0, "hdmi_d5");
	ret |= gpio_request(EPDC_BDR1, "hdmi_d6");
	ret |= gpio_request(EPDC_BDR0, "hdmi_d7");
	ret |= gpio_request(EPDC_SDLE, "hdmi_d8");
	ret |= gpio_request(EPDC_SDCLKN, "hdmi_d9");
	ret |= gpio_request(EPDC_SDSHR, "hdmi_d10");
	ret |= gpio_request(EPDC_PWRCOM, "hdmi_d11");
	ret |= gpio_request(EPDC_PWRSTAT, "hdmi_d12");
	ret |= gpio_request(EPDC_PWRCTRL0, "hdmi_d13");
	ret |= gpio_request(EPDC_PWRCTRL1, "hdmi_d14");
	ret |= gpio_request(EPDC_PWRCTRL2, "hdmi_d15");
	ret |= gpio_request(EPDC_GDCLK, "hdmi_d16");
	ret |= gpio_request(EPDC_GDSP, "hdmi_d17");
	ret |= gpio_request(EPDC_GDOE, "hdmi_d18");
	ret |= gpio_request(EPDC_GDRL, "hdmi_d19");
	ret |= gpio_request(EPDC_SDCLK, "hdmi_d20");
	ret |= gpio_request(EPDC_SDOEZ, "hdmi_d21");
	ret |= gpio_request(EPDC_SDOED, "hdmi_d22");
	ret |= gpio_request(EPDC_SDOE, "hdmi_d23");
	ret |= gpio_request(EPDC_D0, "hdmi_dclk");
	ret |= gpio_request(EPDC_D1, "hdmi_drdy");
	ret |= gpio_request(EPDC_D2, "hdmi_vsync");
	ret |= gpio_request(EPDC_D3, "hdmi_hsync");

	return ret;
}

static void sii902x_hdmi_put_pins(void)
{
	gpio_free(HDMI_RESET);
	gpio_free(HDMI_DETECT);
	gpio_free(EPDC_SDCE5);
	gpio_free(EPDC_SDCE4);
	gpio_free(EPDC_SDCE3);
	gpio_free(EPDC_SDCE2);
	gpio_free(EPDC_SDCE1);
	gpio_free(EPDC_SDCE0);
	gpio_free(EPDC_BDR1);
	gpio_free(EPDC_BDR0);
	gpio_free(EPDC_SDLE);
	gpio_free(EPDC_SDCLKN);
	gpio_free(EPDC_SDSHR);
	gpio_free(EPDC_PWRCOM);
	gpio_free(EPDC_PWRSTAT);
	gpio_free(EPDC_PWRCTRL0);
	gpio_free(EPDC_PWRCTRL1);
	gpio_free(EPDC_PWRCTRL2);
	gpio_free(EPDC_GDCLK);
	gpio_free(EPDC_GDSP);
	gpio_free(EPDC_GDOE);
	gpio_free(EPDC_GDRL);
	gpio_free(EPDC_SDCLK);
	gpio_free(EPDC_SDOEZ);
	gpio_free(EPDC_SDOED);
	gpio_free(EPDC_SDOE);
	gpio_free(EPDC_D0);
	gpio_free(EPDC_D1);
	gpio_free(EPDC_D2);
	gpio_free(EPDC_D3);
}

static iomux_v3_cfg_t mx50_sii902x_hdmi_pads_enabled[] = {
	MX50_PAD_EPDC_SDCE5__ELCDIF_D0,
	MX50_PAD_EPDC_SDCE4__ELCDIF_D1,
	MX50_PAD_EPDC_SDCE3__ELCDIF_D2,
	MX50_PAD_EPDC_SDCE2__ELCDIF_D3,
	MX50_PAD_EPDC_SDCE1__ELCDIF_D4,
	MX50_PAD_EPDC_SDCE0__ELCDIF_D5,
	MX50_PAD_EPDC_BDR1__ELCDIF_D6,
	MX50_PAD_EPDC_BDR0__ELCDIF_D7,
	MX50_PAD_EPDC_SDLE__ELCDIF_D8,
	MX50_PAD_EPDC_SDCLKN__ELCDIF_D9,
	MX50_PAD_EPDC_SDSHR__ELCDIF_D10,
	MX50_PAD_EPDC_PWRCOM__ELCDIF_D11,
	MX50_PAD_EPDC_PWRSTAT__ELCDIF_D12,
	MX50_PAD_EPDC_PWRCTRL0__ELCDIF_D13,
	MX50_PAD_EPDC_PWRCTRL1__ELCDIF_D14,
	MX50_PAD_EPDC_PWRCTRL2__ELCDIF_D15,
	MX50_PAD_EPDC_GDCLK__ELCDIF_D16,
	MX50_PAD_EPDC_GDSP__ELCDIF_D17,
	MX50_PAD_EPDC_GDOE__ELCDIF_D18,
	MX50_PAD_EPDC_GDRL__ELCDIF_D19,
	MX50_PAD_EPDC_SDCLK__ELCDIF_D20,
	MX50_PAD_EPDC_SDOEZ__ELCDIF_D21,
	MX50_PAD_EPDC_SDOED__ELCDIF_D22,
	MX50_PAD_EPDC_SDOE__ELCDIF_D23,
	MX50_PAD_EPDC_D2__ELCDIF_VSYNC,
	MX50_PAD_EPDC_D3__ELCDIF_HSYNC,
	MX50_PAD_EPDC_D1__ELCDIF_EN,
	MX50_PAD_EPDC_D0__ELCDIF_DCLK,
	MX50_PAD_EIM_LBA__GPIO_1_26,
	MX50_PAD_EIM_RW__GPIO_1_25,
	MX50_PAD_EIM_OE__GPIO_1_24,
};

static iomux_v3_cfg_t mx50_sii902x_hdmi_pads_disabled[] = {
	MX50_PAD_EPDC_SDCE5__GPIO_4_30,
	MX50_PAD_EPDC_SDCE4__GPIO_4_29,
	MX50_PAD_EPDC_SDCE3__GPIO_4_28,
	MX50_PAD_EPDC_SDCE2__GPIO_4_27,
	MX50_PAD_EPDC_SDCE1__GPIO_4_26,
	MX50_PAD_EPDC_SDCE0__GPIO_4_25,
	MX50_PAD_EPDC_BDR1__GPIO_4_24,
	MX50_PAD_EPDC_BDR0__GPIO_4_23,
	MX50_PAD_EPDC_SDLE__GPIO_3_24,
	MX50_PAD_EPDC_SDCLKN__GPIO_3_25,
	MX50_PAD_EPDC_SDSHR__GPIO_3_26,
	MX50_PAD_EPDC_PWRCOM__GPIO_3_27,
	MX50_PAD_EPDC_PWRSTAT__GPIO_3_28,
	MX50_PAD_EPDC_PWRCTRL0__GPIO_3_29,
	MX50_PAD_EPDC_PWRCTRL1__GPIO_3_30,
	MX50_PAD_EPDC_PWRCTRL2__GPIO_3_31,
	MX50_PAD_EPDC_GDCLK__GPIO_3_16,
	MX50_PAD_EPDC_GDSP__GPIO_3_17,
	MX50_PAD_EPDC_GDOE__GPIO_3_18,
	MX50_PAD_EPDC_GDRL__GPIO_3_19,
	MX50_PAD_EPDC_SDCLK__GPIO_3_20,
	MX50_PAD_EPDC_SDOEZ__GPIO_3_21,
	MX50_PAD_EPDC_SDOED__GPIO_3_22,
	MX50_PAD_EPDC_SDOE__GPIO_3_23,
	MX50_PAD_EPDC_D2__GPIO_3_2,
	MX50_PAD_EPDC_D3__GPIO_3_3,
	MX50_PAD_EPDC_D1__GPIO_3_1,
	MX50_PAD_EPDC_D0__GPIO_3_0,
	MX50_PAD_EIM_LBA__GPIO_1_26,
	MX50_PAD_EIM_RW__GPIO_1_25,
	MX50_PAD_EIM_OE__GPIO_1_24,
};

static void sii902x_hdmi_enable_pins(void)
{
	/* Configure MUX settings to enable HDMI use */
	mxc_iomux_v3_setup_multiple_pads(mx50_sii902x_hdmi_pads_enabled, \
				ARRAY_SIZE(mx50_sii902x_hdmi_pads_enabled));
}

static void sii902x_hdmi_disable_pins(void)
{
	/* Configure MUX settings for HDMI pins to
	 * GPIO mode and drive to 0. */
	mxc_iomux_v3_setup_multiple_pads(mx50_sii902x_hdmi_pads_disabled, \
				ARRAY_SIZE(mx50_sii902x_hdmi_pads_disabled));

	/* Setting as GPIO input should reduce power consumption */
	gpio_direction_input(EPDC_SDCE5);
	gpio_direction_input(EPDC_SDCE4);
	gpio_direction_input(EPDC_SDCE3);
	gpio_direction_input(EPDC_SDCE2);
	gpio_direction_input(EPDC_SDCE1);
	gpio_direction_input(EPDC_SDCE0);
	gpio_direction_input(EPDC_BDR1);
	gpio_direction_input(EPDC_BDR0);
	gpio_direction_input(EPDC_SDLE);
	gpio_direction_input(EPDC_SDCLKN);
	gpio_direction_input(EPDC_SDSHR);
	gpio_direction_input(EPDC_PWRCOM);
	gpio_direction_input(EPDC_PWRSTAT);
	gpio_direction_input(EPDC_PWRCTRL0);
	gpio_direction_input(EPDC_PWRCTRL1);
	gpio_direction_input(EPDC_PWRCTRL2);
	gpio_direction_input(EPDC_GDCLK);
	gpio_direction_input(EPDC_GDSP);
	gpio_direction_input(EPDC_GDOE);
	gpio_direction_input(EPDC_GDRL);
	gpio_direction_input(EPDC_SDCLK);
	gpio_direction_input(EPDC_SDOEZ);
	gpio_direction_input(EPDC_SDOED);
	gpio_direction_input(EPDC_SDOE);
	gpio_direction_input(EPDC_D0);
	gpio_direction_input(EPDC_D1);
	gpio_direction_input(EPDC_D2);
	gpio_direction_input(EPDC_D3);
}

static struct mxc_lcd_platform_data sii902x_hdmi_data = {
	.reset = sii902x_hdmi_reset,
	.get_pins = sii902x_hdmi_get_pins,
	.put_pins = sii902x_hdmi_put_pins,
	.enable_pins = sii902x_hdmi_enable_pins,
	.disable_pins = sii902x_hdmi_disable_pins,
};

static struct i2c_board_info mxc_i2c1_board_info[] __initdata = {
	{
	 .type = "sgtl5000-i2c",
	 .addr = 0x0a,
	 },
	{
	.type = "sii902x",
	.addr = 0x39,
	.irq = gpio_to_irq(HDMI_DETECT),
	.platform_data = &sii902x_hdmi_data,
	},
};

static struct mtd_partition mxc_dataflash_partitions[] = {
	{
	 .name = "bootloader",
	 .offset = 0,
	 .size = 0x000100000,},
	{
	 .name = "kernel",
	 .offset = MTDPART_OFS_APPEND,
	 .size = MTDPART_SIZ_FULL,},
};

static struct flash_platform_data mxc_spi_flash_data[] = {
	{
	 .name = "mxc_dataflash",
	 .parts = mxc_dataflash_partitions,
	 .nr_parts = ARRAY_SIZE(mxc_dataflash_partitions),
	 .type = "at45db321d",},
	 {
	 .name = "m25p80",
	 .parts = mxc_dataflash_partitions,
	 .nr_parts = ARRAY_SIZE(mxc_dataflash_partitions),
	 .type = "m25p32",}
};


static struct spi_board_info mxc_dataflash_device[] __initdata = {
	{
	 .modalias = "mxc_dataflash",
	 .max_speed_hz = 25000000,	/* max spi clock (SCK) speed in HZ */
	 .bus_num = 3,
	 .chip_select = 1,
	 .platform_data = &mxc_spi_flash_data[0],},
};

static struct spi_board_info m25pxx_dataflash_device[] __initdata = {
	{
	 .modalias = "m25p80",
	 .max_speed_hz = 20000000,	/* max spi clock (SCK) speed in HZ */
	 .bus_num = 3,
	 .chip_select = 1,
	 .platform_data = &mxc_spi_flash_data[1],},
};

static int sdhc_write_protect(struct device *dev)
{
	unsigned short rc = 0;

	if (to_platform_device(dev)->id == 0)
		rc = gpio_get_value(SD1_WP);
	else if (to_platform_device(dev)->id == 1)
		rc = gpio_get_value(SD2_WP);
	else if (to_platform_device(dev)->id == 2)
		rc = 0;

	return rc;
}

static unsigned int sdhc_get_card_det_status(struct device *dev)
{
	int ret = 0;
	if (to_platform_device(dev)->id == 0)
		ret = gpio_get_value(SD1_CD);
	else if (to_platform_device(dev)->id == 1)
		ret = gpio_get_value(SD2_CD);
	else if (to_platform_device(dev)->id == 2)
		ret = 1;

	return ret;
}

static struct mxc_mmc_platform_data mmc1_data = {
	.ocr_mask = MMC_VDD_27_28 | MMC_VDD_28_29 | MMC_VDD_29_30
		| MMC_VDD_31_32,
	.caps = MMC_CAP_4_BIT_DATA,
	.min_clk = 400000,
	.max_clk = 50000000,
	.card_inserted_state = 0,
	.status = sdhc_get_card_det_status,
	.wp_status = sdhc_write_protect,
	.clock_mmc = "esdhc_clk",
	.power_mmc = NULL,
};

static struct mxc_mmc_platform_data mmc2_data = {
	.ocr_mask = MMC_VDD_27_28 | MMC_VDD_28_29 | MMC_VDD_29_30
		| MMC_VDD_31_32,
	.caps = MMC_CAP_4_BIT_DATA | MMC_CAP_8_BIT_DATA,
	.min_clk = 400000,
	.max_clk = 50000000,
	.card_inserted_state = 0,
	.status = sdhc_get_card_det_status,
	.wp_status = sdhc_write_protect,
	.clock_mmc = "esdhc_clk",
	.power_mmc = "VSD"
};

/*
 * NOTE: Due to possible timing issue, it is not recommended to use usdhc
 * with DDR mode enabled. Instead, we use esdhc for DDR mode by default.
 */
static struct mxc_mmc_platform_data mmc3_data = {
	.ocr_mask = MMC_VDD_27_28 | MMC_VDD_28_29 | MMC_VDD_29_30
		| MMC_VDD_31_32,
	.caps = MMC_CAP_4_BIT_DATA | MMC_CAP_8_BIT_DATA | MMC_CAP_DATA_DDR,
	.min_clk = 400000,
	.max_clk = 50000000,
	.card_inserted_state = 1,
	.status = sdhc_get_card_det_status,
	.wp_status = sdhc_write_protect,
	.clock_mmc = "esdhc_clk",
};

static int mxc_sgtl5000_amp_enable(int enable)
{
	gpio_set_value(SGTL_AMP_SHDN, !enable);
	return 0;
}

static int mxc_sgtl5000_clock_enable(int enable)
{
	gpio_set_value(SGTL_OSCEN, enable);
	return 0;
}

static int headphone_det_status(void)
{
	return (gpio_get_value(HP_DETECT) == 0);
}

static struct mxc_audio_platform_data sgtl5000_data = {
	.ssi_num = 1,
	.src_port = 2,
	.ext_port = 3,
	.hp_irq = gpio_to_irq(HP_DETECT),
	.hp_status = headphone_det_status,
	.amp_enable = mxc_sgtl5000_amp_enable,
	.clock_enable = mxc_sgtl5000_clock_enable,
	.sysclk = 12288000,
};

static struct platform_device mxc_sgtl5000_device = {
	.name = "imx-3stack-sgtl5000",
};

static iomux_v3_cfg_t rdp_wvga_pads[] = {
	MX50_PAD_DISP_D0__DISP_D0,
	MX50_PAD_DISP_D1__DISP_D1,
	MX50_PAD_DISP_D2__DISP_D2,
	MX50_PAD_DISP_D3__DISP_D3,
	MX50_PAD_DISP_D4__DISP_D4,
	MX50_PAD_DISP_D5__DISP_D5,
	MX50_PAD_DISP_D6__DISP_D6,
	MX50_PAD_DISP_D7__DISP_D7,
	MX50_PAD_EIM_DA0__GPIO_1_0,
	MX50_PAD_EIM_DA1__GPIO_1_1,
	MX50_PAD_EIM_DA2__GPIO_1_2,
	MX50_PAD_EIM_DA3__GPIO_1_3,
	MX50_PAD_EIM_DA4__GPIO_1_4,
	MX50_PAD_EIM_DA5__GPIO_1_5,
	MX50_PAD_EIM_DA6__GPIO_1_6,
	MX50_PAD_EIM_DA7__GPIO_1_7,
};

static void wvga_reset(void)
{
	mxc_iomux_v3_setup_multiple_pads(rdp_wvga_pads, \
				ARRAY_SIZE(rdp_wvga_pads));

	if (board_is_mx50_rd3())
		gpio_direction_output(FEC_EN, 0);
	else
		gpio_direction_output(FEC_EN, 1);

	gpio_request(ELCDIF_DAT0_DUMMY, "elcdif-data0");
	gpio_direction_output(ELCDIF_DAT0_DUMMY, 0);
	gpio_request(ELCDIF_DAT1_DUMMY, "elcdif-data1");
	gpio_direction_output(ELCDIF_DAT1_DUMMY, 0);
	gpio_request(ELCDIF_DAT2_DUMMY, "elcdif-data2");
	gpio_direction_output(ELCDIF_DAT2_DUMMY, 0);
	gpio_request(ELCDIF_DAT8_DUMMY, "elcdif-data8");
	gpio_direction_output(ELCDIF_DAT8_DUMMY, 0);
	gpio_request(ELCDIF_DAT9_DUMMY, "elcdif-data9");
	gpio_direction_output(ELCDIF_DAT9_DUMMY, 0);
	gpio_request(ELCDIF_DAT16_DUMMY, "elcdif-data16");
	gpio_direction_output(ELCDIF_DAT16_DUMMY, 0);
	gpio_request(ELCDIF_DAT17_DUMMY, "elcdif-data17");
	gpio_direction_output(ELCDIF_DAT17_DUMMY, 0);

	return;
}

static void wvga_reset__on_j12(void)
{
	mxc_iomux_v3_setup_pad(MX50_PAD_KEY_ROW0__GPIO_4_1);

	gpio_request(LCD_PWR_EN, "power_en");
	gpio_direction_output(LCD_PWR_EN, 1);
}

/* Use same pinmux on HDMI */
static int claa_wvga_get_pins(void)
{
	return sii902x_hdmi_get_pins();
}

static void claa_wvga_put_pins(void)
{
	sii902x_hdmi_put_pins();
}

static void claa_wvga_enable_pins(void)
{
	sii902x_hdmi_enable_pins();
}

static void claa_wvga_disable_pins(void)
{
	sii902x_hdmi_disable_pins();
}

static struct mxc_lcd_platform_data lcd_wvga_data = {
	.reset = wvga_reset,
};

static struct platform_device lcd_wvga_device = {
	.name = "lcd_seiko",
	.dev = {
		.platform_data = &lcd_wvga_data,
		},
};


static struct mxc_lcd_platform_data lcd_wvga_data2 = {
	.get_pins = claa_wvga_get_pins,
	.put_pins = claa_wvga_put_pins,
	.enable_pins = claa_wvga_enable_pins,
	.disable_pins = claa_wvga_disable_pins,
};


static struct platform_device lcd_wvga_device2 = {
	.name = "lcd_claa",
	.dev = {
		.platform_data = &lcd_wvga_data2,
		},
};

static struct fb_videomode video_modes[] = {
	{
	 /* 800x480 @ 57 Hz , pixel clk @ 32MHz */
	 "SEIKO-WVGA", 60, 800, 480, 29850, 99, 164, 33, 10, 10, 10,
	 FB_SYNC_CLK_LAT_FALL,
	 FB_VMODE_NONINTERLACED,
	 0,},
	{
	 /* 800x480 @ 57 Hz , pixel clk @ 27MHz */
	 "CLAA-WVGA", 57, 800, 480, 37037, 40, 60, 10, 10, 20, 10,
	 FB_SYNC_CLK_LAT_FALL,
	 FB_VMODE_NONINTERLACED,
	 0,},
	{
	/* 1600x1200 @ 60 Hz 162M pixel clk*/
	"UXGA", 60, 1600, 1200, 6172,
	304, 64,
	1, 46,
	192, 3,
	FB_SYNC_HOR_HIGH_ACT|FB_SYNC_VERT_HIGH_ACT,
	FB_VMODE_NONINTERLACED,
	0,},
};

static struct mxc_fb_platform_data fb_data[] = {
	{
	 .interface_pix_fmt = V4L2_PIX_FMT_RGB24,
	 .mode_str = "1024x768M-16@60",
	 .mode = video_modes,
	 .num_modes = ARRAY_SIZE(video_modes),
	 },
	{
	 .interface_pix_fmt = V4L2_PIX_FMT_RGB565,
	 .mode_str = "SEIKO-WVGA",
	 .mode = video_modes,
	 .num_modes = ARRAY_SIZE(video_modes),
	 },
	{
	 .interface_pix_fmt = V4L2_PIX_FMT_RGB565,
	 .mode_str = "CLAA-WVGA",
	 .mode = video_modes,
	 .num_modes = ARRAY_SIZE(video_modes),
	 },
};

static struct platform_pwm_backlight_data mxc_pwm_backlight_data = {
	.pwm_id = 0,
	.max_brightness = 255,
	.dft_brightness = 128,
	.pwm_period_ns = 2000000,
};

static void mx50_arm2_usb_set_vbus(bool enable)
{
	gpio_set_value(USB_OTG_PWR, enable);
}

static void mxc_register_powerkey(pwrkey_callback pk_cb)
{
	pmic_event_callback_t power_key_event;

	power_key_event.param = (void *)1;
	power_key_event.func = (void *)pk_cb;
	pmic_event_subscribe(EVENT_PWRONI, power_key_event);
	power_key_event.param = (void *)3;
	pmic_event_subscribe(EVENT_PWRON3I, power_key_event);
}

static int mxc_pwrkey_getstatus(int id)
{
	int sense, off = 3;

	pmic_read_reg(REG_INT_SENSE1, &sense, 0xffffffff);
	switch (id) {
	case 2:
		off = 4;
		break;
	case 3:
		off = 2;
		break;
	}

	if (sense & (1 << off))
		return 0;

	return 1;
}

static struct power_key_platform_data pwrkey_data = {
	.key_value = KEY_F4,
	.register_pwrkey = mxc_register_powerkey,
	.get_key_status = mxc_pwrkey_getstatus,
};


static int __initdata enable_w1 = { 0 };
static int __init w1_setup(char *__unused)
{
	enable_w1 = 1;
	return cpu_is_mx50();
}

__setup("w1", w1_setup);

static int __initdata enable_keypad = {0};
static int __init keypad_setup(char *__unused)
{
	enable_keypad = 1;
	return cpu_is_mx50();
}

__setup("keypad", keypad_setup);

static struct mxs_dma_plat_data dma_apbh_data = {
	.chan_base = MXS_DMA_CHANNEL_AHB_APBH,
	.chan_num = MXS_MAX_DMA_CHANNELS,
};

static int gpmi_nfc_platform_init(unsigned int max_chip_count)
{
	mxc_iomux_v3_setup_multiple_pads(mx50_gpmi_nand,
				ARRAY_SIZE(mx50_gpmi_nand));

	return 0;
}

static void gpmi_nfc_platform_exit(unsigned int max_chip_count)
{
}

static const char *gpmi_nfc_partition_source_types[] = { "cmdlinepart", 0 };

static struct gpmi_nfc_platform_data  gpmi_nfc_platform_data = {
	.nfc_version             = 2,
	.boot_rom_version        = 1,
	.clock_name              = "gpmi-nfc",
	.platform_init           = gpmi_nfc_platform_init,
	.platform_exit           = gpmi_nfc_platform_exit,
	.min_prop_delay_in_ns    = 5,
	.max_prop_delay_in_ns    = 9,
	.max_chip_count          = 2,
	.boot_area_size_in_bytes = 20 * SZ_1M,
	.partition_source_types  = gpmi_nfc_partition_source_types,
	.partitions              = 0,
	.partition_count         = 0,
};

static void fec_gpio_iomux_init()
{
	iomux_v3_cfg_t iomux_setting;

	if (board_is_mx50_rd3())
		iomux_setting = (MX50_PAD_ECSPI1_SS0__GPIO_4_15 & \
				~MUX_PAD_CTRL_MASK) | \
				MUX_PAD_CTRL(PAD_CTL_PKE | PAD_CTL_DSE_HIGH);
	else
		iomux_setting = (MX50_PAD_I2C3_SDA__GPIO_6_23 & \
				~MUX_PAD_CTRL_MASK) | \
				MUX_PAD_CTRL(PAD_CTL_PKE | PAD_CTL_DSE_HIGH);

	/* Enable the Pull/keeper */
	mxc_iomux_v3_setup_pad(iomux_setting);
	gpio_request(FEC_EN, "fec-en");
	if (board_is_mx50_rd3())
		gpio_direction_output(FEC_EN, 1);
	else
		gpio_direction_output(FEC_EN, 0);
	gpio_request(FEC_RESET_B, "fec-reset_b");
	gpio_direction_output(FEC_RESET_B, 0);
	udelay(500);
	gpio_set_value(FEC_RESET_B, 1);
}

static void fec_gpio_iomux_deinit()
{
	iomux_v3_cfg_t iomux_setting;

	if (board_is_mx50_rd3())
		iomux_setting = (MX50_PAD_ECSPI1_SS0__GPIO_4_15 & \
					~MUX_PAD_CTRL_MASK) | MUX_PAD_CTRL(0x4);
	else
		iomux_setting = (MX50_PAD_I2C3_SDA__GPIO_6_23 & \
					~MUX_PAD_CTRL_MASK) | MUX_PAD_CTRL(0x4);

	mxc_iomux_v3_setup_pad(iomux_setting);
	gpio_request(FEC_EN, "fec-en");
	gpio_direction_input(FEC_EN);
	gpio_request(FEC_RESET_B, "fec-reset_b");
	gpio_direction_input(FEC_RESET_B);
}

static void mx50_suspend_enter()
{
	iomux_v3_cfg_t *p = suspend_enter_pads;
	int i;
	iomux_v3_cfg_t iomux_setting =
			(MX50_PAD_ECSPI2_SCLK__GPIO_4_16 &
			~MUX_PAD_CTRL_MASK) | MUX_PAD_CTRL(0x84);

	/* Clear the SELF_BIAS bit and power down
	 * the band-gap.
	 */
	__raw_writel(MXC_ANADIG_REF_SELFBIAS_OFF,
			apll_base + MXC_ANADIG_MISC_CLR);
	__raw_writel(MXC_ANADIG_REF_PWD,
			apll_base + MXC_ANADIG_MISC_SET);

	if (board_is_mx50_rd3()) {
		/* Enable the Pull/keeper */
		mxc_iomux_v3_setup_pad(iomux_setting);
		gpio_request(DCDC_EN, "dcdc-en");
		gpio_direction_output(DCDC_EN, 0);
	}

	/* Set PADCTRL to 0 for all IOMUX. */
	for (i = 0; i < ARRAY_SIZE(suspend_enter_pads); i++) {
		suspend_exit_pads[i] = *p;
		*p &= ~MUX_PAD_CTRL_MASK;
		p++;
	}
	mxc_iomux_v3_get_multiple_pads(suspend_exit_pads,
			ARRAY_SIZE(suspend_exit_pads));
	mxc_iomux_v3_setup_multiple_pads(suspend_enter_pads,
			ARRAY_SIZE(suspend_enter_pads));

	fec_gpio_iomux_deinit();
}

static void mx50_suspend_exit()
{
	iomux_v3_cfg_t iomux_setting =
			(MX50_PAD_ECSPI2_SCLK__GPIO_4_16 &
			~MUX_PAD_CTRL_MASK) | MUX_PAD_CTRL(0x84);

	/* Power Up the band-gap and set the SELFBIAS bit. */
	__raw_writel(MXC_ANADIG_REF_PWD,
			apll_base + MXC_ANADIG_MISC_CLR);
	udelay(100);
	__raw_writel(MXC_ANADIG_REF_SELFBIAS_OFF,
			apll_base + MXC_ANADIG_MISC_SET);

	if (board_is_mx50_rd3()) {
		/* Enable the Pull/keeper */
		mxc_iomux_v3_setup_pad(iomux_setting);
		gpio_request(DCDC_EN, "dcdc-en");
		gpio_direction_output(DCDC_EN, 1);
	}

	mxc_iomux_v3_setup_multiple_pads(suspend_exit_pads,
			ARRAY_SIZE(suspend_exit_pads));
	fec_gpio_iomux_init();
}

static struct mxc_pm_platform_data mx50_pm_data = {
	.suspend_enter = mx50_suspend_enter,
	.suspend_exit = mx50_suspend_exit,
};

/*!
 * Board specific fixup function. It is called by \b setup_arch() in
 * setup.c file very early on during kernel starts. It allows the user to
 * statically fill in the proper values for the passed-in parameters. None of
 * the parameters is used currently.
 *
 * @param  desc         pointer to \b struct \b machine_desc
 * @param  tags         pointer to \b struct \b tag
 * @param  cmdline      pointer to the command line
 * @param  mi           pointer to \b struct \b meminfo
 */
static void __init fixup_mxc_board(struct machine_desc *desc, struct tag *tags,
				   char **cmdline, struct meminfo *mi)
{
	mxc_set_cpu_type(MXC_CPU_MX50);

	get_cpu_wp = mx50_rdp_get_cpu_wp;
	set_num_cpu_wp = mx50_rdp_set_num_cpu_wp;
	get_dvfs_core_wp = mx50_rdp_get_dvfs_core_table;
	num_cpu_wp = ARRAY_SIZE(cpu_wp_auto);
}

static void __init mx50_rdp_io_init(void)
{
	iomux_v3_cfg_t cspi_keeper = (MX50_PAD_ECSPI1_SCLK__GPIO_4_12 & ~MUX_PAD_CTRL_MASK);

	iomux_v3_cfg_t *p = mx50_rdp;
	int i;

	/* Set PADCTRL to 0 for all IOMUX. */
	for (i = 0; i < ARRAY_SIZE(mx50_rdp); i++) {
		iomux_v3_cfg_t pad_ctl = *p;
		pad_ctl &= ~MUX_PAD_CTRL_MASK;
		mxc_iomux_v3_setup_pad(pad_ctl);
		p++;
	}

	mxc_iomux_v3_setup_multiple_pads(mx50_rdp, \
			ARRAY_SIZE(mx50_rdp));

	if (board_is_mx50_rd3())
		mxc_iomux_v3_setup_multiple_pads(mx50_rd3_adjust, \
			ARRAY_SIZE(mx50_rd3_adjust));

	gpio_request(SD1_WP, "sdhc1-wp");
	gpio_direction_input(SD1_WP);

	gpio_request(SD1_CD, "sdhc1-cd");
	gpio_direction_input(SD1_CD);

	//gpio_request(SD2_WP, "sdhc2-wp");
	//gpio_direction_input(SD2_WP);

	//gpio_request(SD2_CD, "sdhc2-cd");
	//gpio_direction_input(SD2_CD);

	gpio_request(HP_DETECT, "hp-det");
	gpio_direction_input(HP_DETECT);

	gpio_request(PWR_INT, "pwr-int");
	gpio_direction_input(PWR_INT);

	gpio_request(EPDC_PMIC_WAKE, "epdc-pmic-wake");
	gpio_direction_output(EPDC_PMIC_WAKE, 0);

	gpio_request(EPDC_VCOM, "epdc-vcom");
	gpio_direction_output(EPDC_VCOM, 0);

	gpio_request(EPDC_PWRCTRL0, "epdc-powerup");
	gpio_direction_output(EPDC_PWRCTRL0, 0);

	gpio_request(EPDC_PMIC_INT, "epdc-pmic-int");
	gpio_direction_input(EPDC_PMIC_INT);

	gpio_request(EPDC_PWRSTAT, "epdc-pwrstat");
	gpio_direction_input(EPDC_PWRSTAT);

	gpio_request(ELCDIF_PWR_ON, "elcdif-power-on");
	gpio_direction_output(ELCDIF_PWR_ON, 1);

	if (board_is_mx50_rd3()) {
		gpio_request(UART1_RTS, "sd2-vdd");
		gpio_direction_output(UART1_RTS, 1);
		/* isolate EIM signals and boot configuration signals. */
		gpio_request(UART2_RX, "eim-bootcfg-iso");
		gpio_direction_output(UART2_RX, 1);
	}

	/* Sii902x HDMI controller */
	gpio_request(HDMI_RESET, "hdmi-reset");
	gpio_direction_output(HDMI_RESET, 1);
	gpio_request(HDMI_PWR_ENABLE, "hdmi-pwr-enable");
	gpio_direction_output(HDMI_PWR_ENABLE, 1);
	gpio_set_value(HDMI_PWR_ENABLE, 0);
	gpio_request(HDMI_DETECT, "hdmi-detect");
	gpio_direction_input(HDMI_DETECT);

	if (enable_w1) {
		iomux_v3_cfg_t one_wire = MX50_PAD_OWIRE__OWIRE;
		mxc_iomux_v3_setup_pad(one_wire);
	}

	/* SGTL5000_OSC_EN */
	gpio_request(SGTL_OSCEN, "sgtl5000-osc-en");
	gpio_direction_output(SGTL_OSCEN, 1);

	gpio_request(SGTL_AMP_SHDN, "sgtl5000-amp-shdn");
	gpio_direction_output(SGTL_AMP_SHDN, 1);

	fec_gpio_iomux_init();

	/* USB OTG PWR */
	gpio_request(USB_OTG_PWR, "usb otg power");
	gpio_direction_output(USB_OTG_PWR, 0);

	/* Disable all keepers */
	mxc_iomux_v3_setup_pad(cspi_keeper);
}

/*!
 * Board specific initialization.
 */
static void __init mxc_board_init(void)
{
	/* SD card detect irqs */
	mxcsdhc1_device.resource[2].start = gpio_to_irq(SD1_CD);
	mxcsdhc1_device.resource[2].end = gpio_to_irq(SD1_CD);
	mxcsdhc2_device.resource[2].start = gpio_to_irq(SD2_CD);
	mxcsdhc2_device.resource[2].end = gpio_to_irq(SD2_CD);


	mxc_cpu_common_init();
	mx50_rdp_io_init();

	mxc_register_device(&mxcspi1_device, &mxcspi1_data);
	mxc_register_device(&mxcspi3_device, &mxcspi3_data);

	if (board_is_mx50_rd3())
		dvfs_core_data.reg_id = "SW1A";
	mxc_register_device(&mxc_dvfs_core_device, &dvfs_core_data);
	if (board_is_mx50_rd3())
		bus_freq_data.gp_reg_id = "SW1A";
	mxc_register_device(&busfreq_device, &bus_freq_data);

	mxc_register_device(&mxc_dma_device, NULL);
	mxc_register_device(&mxs_dma_apbh_device, &dma_apbh_data);
	mxc_register_device(&mxc_wdt_device, NULL);
	
        mxc_register_device(&mxc_rtc_device, NULL);
        mxc_register_device(&pm_device, &mx50_pm_data);

   // make sure eMMC will be first scan , the ordre is important
	mxc_register_device(&mxcsdhc3_device, &mmc3_data); 
	//mxc_register_device(&mxcsdhc2_device, &mmc2_data);
        mxc_register_device(&mxcsdhc1_device, &mmc1_data);

        if (enable_keypad)
		mxc_register_device(&mxc_keypad_device, &keypad_plat_data);

	mxc_register_device(&mxc_ssi1_device, NULL);
	mxc_register_device(&mxc_ssi2_device, NULL);
	mxc_register_device(&mxc_fec_device, &fec_data);
	if (board_is_mx50_rd3())
		spi_register_board_info(m25pxx_dataflash_device,
				ARRAY_SIZE(m25pxx_dataflash_device));
	else
		spi_register_board_info(mxc_dataflash_device,
				ARRAY_SIZE(mxc_dataflash_device));
	i2c_register_board_info(0, mxc_i2c0_board_info,
				ARRAY_SIZE(mxc_i2c0_board_info));
	i2c_register_board_info(1, mxc_i2c1_board_info,
				ARRAY_SIZE(mxc_i2c1_board_info));

	mxc_register_device(&max17135_sensor_device, NULL);
	mxc_register_device(&epdc_device, &epdc_data);
	if (!board_is_mx50_rd3())
		lcdif_sel_lcd = 1;

	/* If choose "j12", but not "seiko wvga"; then abandon "j12" option */
	if (lcd_seiko_on_j12 == 1 && lcdif_sel_lcd != 1)
		lcd_seiko_on_j12 = 0;
	if (lcd_seiko_on_j12 == 1 && lcdif_sel_lcd == 1) {
		lcd_wvga_data.reset = wvga_reset__on_j12;
		lcd_wvga_data.get_pins = claa_wvga_get_pins,
		lcd_wvga_data.put_pins = claa_wvga_put_pins,
		lcd_wvga_data.enable_pins = claa_wvga_enable_pins,
		lcd_wvga_data.disable_pins = claa_wvga_disable_pins,
		fb_data[1].interface_pix_fmt = V4L2_PIX_FMT_RGB24;
	}
	if (lcdif_sel_lcd == 1)
		mxc_register_device(&lcd_wvga_device, &lcd_wvga_data);
	if (lcdif_sel_lcd == 2)
		mxc_register_device(&lcd_wvga_device2, &lcd_wvga_data2);
	//mxc_register_device(&elcdif_device, &fb_data[lcdif_sel_lcd]);
	mxc_register_device(&mxc_pwm1_device, NULL);
	//mxc_register_device(&mxc_pwm1_backlight_device,
	//	&mxc_pwm_backlight_data);
	//mxc_register_device(&mxs_viim, NULL);
	mxc_register_device(&mxc_rngb_device, NULL);
	//mxc_register_device(&dcp_device, NULL);
	mxc_register_device(&mxc_powerkey_device, &pwrkey_data);
	mxc_register_device(&fixed_volt_reg_device, &fixed_volt_reg_pdata);
	if (mx50_revision() >= IMX_CHIP_REVISION_1_1)
		mxc_register_device(&mxc_zq_calib_device, NULL);
	//if (board_is_mx50_rd3())
	//	mx50_rdp_init_mc34708();
	//else
		mx50_rdp_init_mc13892();
/*
	pm_power_off = mxc_power_off;
	*/
	mx5_set_otghost_vbus_func(mx50_arm2_usb_set_vbus);
	//mxc_register_device(&mxc_sgtl5000_device, &sgtl5000_data);
	//mxc_register_device(&gpmi_nfc_device, &gpmi_nfc_platform_data);
	mx5_usb_dr_init();
	mx5_usbh1_init();
	mxc_register_device(&mxc_perfmon, &mxc_perfmon_data);
}

static void __init mx50_rdp_timer_init(void)
{
	struct clk *uart_clk;

	mx50_clocks_init(32768, 24000000, 22579200);

	uart_clk = clk_get_sys("mxcintuart.0", NULL);
	early_console_setup(MX53_BASE_ADDR(UART1_BASE_ADDR), uart_clk);
}

static struct sys_timer mxc_timer = {
	.init	= mx50_rdp_timer_init,
};

/*
 * The following uses standard kernel macros define in arch.h in order to
 * initialize __mach_desc_MX50_RDP data structure.
 */
MACHINE_START(MX50_RDP, "Freescale MX50 Reference Design Platform")
	/* Maintainer: Freescale Semiconductor, Inc. */
	.fixup = fixup_mxc_board,
	.map_io = mx5_map_io,
	.init_irq = mx5_init_irq,
	.init_machine = mxc_board_init,
	.timer = &mxc_timer,
MACHINE_END
