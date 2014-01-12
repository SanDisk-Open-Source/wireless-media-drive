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
#include <linux/regulator/consumer.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/fixed.h>
#include <linux/pmic_external.h>
#include <linux/pmic_status.h>
#include <linux/fec.h>
#include <linux/gpmi-nfc.h>
#include <linux/powerkey.h>
#ifdef CONFIG_PROC_FS
#include <linux/proc_fs.h>
#endif
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


#include "devices.h"
#include "usb.h"
#include "crm_regs.h"
#include "dma-apbh.h"


#define NIMBUS_KERNEL_VERSION   "1.1.8"
/* For QSI Nimbus project.
 * The hardware is based on i.MX50 RD3 EVB.
 * Johnson.Lee
 * ----------------------------------------
 * [20120607] Add /proc/nimbus entry.
 * [20120718] Added version code.
 *  - Changed pmic_battery -> mc13892_battery for module inactive issue.
 * [20120730] (V1.0.1)
 *  - Added MMC patched code 
 *  - Changed battery_module current setting to 720mA
 *  - Revised PMIC_Battery behavior.
 *  - Added ledctrl and batalarm in /proc/nimbus
 * [20120801] (V1.0.2)
 *  - Added voltage checking > 4.1V when chageing is full.
 * [20120811] (V1.0.3)
 *  - Separate LED controlled interface for kernel and user_space
 *  - Added link infomration of usb for g_file_storage
 * [20120821] (V1.0.4)
 *  - Added LED blink for low battery alarm.
 * [20120828] (V1.0.5)
 *  - Added low power handle for suspending mode.
 *  - Power down device in low power suspending.
 *  - Revised power led off after usb take off. LOWBATL=3.1V
 *  - Revised SD_CHARGER and SD_FOUND status.
 *  - disabled SD1 EJECT
 * [20120903] (V1.0.6)
 *  - correct led issue when power on by usb.
 * [20120917] (V1.0.7)
 *  - changed SDIO of Wi-Fi clock to 20MHz
 *  - Added time-stamp for kernel message
 * [20120917] (V1.0.7.0) Testing purpose
 *  - Added code heart led indication
 * [20120924] (V1.0.8) Testing purpose
 *  - Removed PMIC34708
 *  - Added xt_LED target
 *  - Added leds_gpio
 * [20120925] (V1.0.9)
 *  - Added battery capacity in power_now of mc13892
 *  - Apply USB patched code of STC for USB testing issue.
 *  - Adjust Wi-Fi enable delay time from 70ms to 200ms from TI suggestion.
 * [20121008] (V1.0.10)
 *  - Added /proc/nimbus/suspending for system status
 *    0: Normal
 *    1: Suspending
 *    2: Suspended
 *  - Revised the lower power handle of PMIC.
 *  - Added FUSE module.
 * [20121018] (V1.0.11)
 *  - Adjust configuration
 * [20121026] (V1.0.12)
 *  - Enabled SRTC module.
 *  - Added RTC wakeup for suspending to poweroff.
 *  - Changed Wi-Fi SDIO to 15MHz
 * [20121026] (V1.0.13)
 *  - Adjust configuration of netfilter 
 * [20121115] (V1.0.14)
 *  - Fixed /proc/nimbus/ledctrl no function.
 *  - Improved battery power_now response time.
 * [20121207] (V1.0.15)
 *  - removed mx5_usbh1_init due to no use in platfrom and system will hang up if 
 *    R17 removed.
 * [20121222] (V1.0.16)
 *  - Reivsed charging current flow, from 480 mA step up to 720 mA 
 *    (workaround for low current issue. <500mA)
 *  - Removed: mxc_register_low_power_event
 * [20130108] (V1.0.17)
 *  - Removed some debug message of pmic_battery.c
 *  - Changed MSC device name from Nimbus to Scotti
 *  - Added workaround for USBIF issue,EL_9/8. (Turn off charging in USB test mode)
 * [20130117] (V1.0.18)
 *  - Change Wi-Fi SDIO to 20MHz
 *  - Change iNAND and SD clock to 45MHz, found sometime error on SD
 *  - Added power key handle in kernel for quick response.
 * [20130118] (V1.0.19)
 *  - enhance power key handle, detection key status during init.
 * [20130120] (V1.0.20)
 *  - Revised PMIC controlling.
 * [20130208] (V1.1.0)
 *  - Revised LED behavior for discharge/charging.
 *  - Changed SDIO clk: Wi-Fi:21MHz,SD/MMC: 50MHz
 * [20130223] (V1.1.1)
 *  - Revised battery level & precentage issue
 * [20130223] (V1.1.2)
 *  - Improved full charging condition.
 * [20130225] (V1.1.3)
 *  - Improved full charging condition.
 * [20130228] (V1.1.4)
 *  - Chnage LED from 3 state to 2 state, 3.6V is boundary.
 *  - Removed battery alarm info from /proc.
 * [20130228] (V1.1.5)
 *  - Disabled very low battery workaround.
 *  (NIMBUS_VERY_LOWBAT_WORKAROUND)
 *  - Disable CHARGE setting CHGAUTOB.
 *  - Adjust battery precentage mapping table.
 * [20130314] (V1.1.6)
 *  - Added IDLE timeout to power off system feature
 *    (POWEROFF_SYSTEM_IN_IDLE_TIMEOUT)
 * [20130409] (V1.1.7)
 *  - Changed idle time from 10 Min. to 20 Min.
 * [20130528] (V1.1.8)
 *  - Revised g_file_storage driver for EJECT from OS.
 */

#define NO_SDEJECT_SOFTWARE
#define POWEROFF_SYSTEM_IN_IDLE_TIMEOUT
#define POWERKEY_HANDLE

#define LED_WIFI            (2*32+1) // output,On-low,Off-high,port3.1
#define LED_POWER_GREEN     (2*32+2) // output,On-low,Off-high
#define LED_WLAN            LED_POWER_GREEN
#define LED_LAN             LED_WIFI
#define LED_CHARGER_RED     (2*32+3) // output,On-low,Off-high
#define LED_CHARGER_GREEN   (2*32+4) // output,On-low,Off-high
#define iNAND_RESET         (2*32+5) // output,Reset-Low,Normal-high
#define SD_POWER            (2*32+6) // output,On-Low,Off-high
#define WIFI_RESET          (2*32+7) // output,Reset-low,Nomrl-high
#define SD1_CHANGE_CLEAR    (2*32+8) // output,Clear-high,Normal-low
#define SD1_CHANGE_FOUND    (2*32+9) // input, Found-low,Normal-high

#define WIFI_WAKEUP         (3*32+1)  // Input,
#define WIFI_POWER          (3*32+16) // output,Enable-high,Disable-low
#define SYSTEM_DOWN         (3*32+17) // input,

#define POWER_KET_DET       (5*32+24) // input,
#define WDOG_B              (5*32+28) // non-use set to input,Output,Normal-High,Low-WDI active

#define SD1_WP	            (3*32 + 19)	/*GPIO_4_19 */
#define SD1_CD	            (0*32 + 27)	/*GPIO_1_27,there is reserve detction, normal in low, after card inser will change to high */

// Non-used for SD2 in case of wi-fi design on board
#define SD2_WP	            (4*32 + 16)	/*GPIO_5_16 */
#define SD2_CD	            (4*32 + 17) /*GPIO_5_17 */

#define CSPI_CS1	        (3*32 + 13)	/*GPIO_4_13 */
#define CSPI_CS2	        (3*32 + 11) /*GPIO_4_11*/
#define USB_OTG_PWR	        (5*32 + 25) /*GPIO_6_25*/
#define DCDC_EN             WIFI_POWER  /*GPIO_4_16*/

#ifdef CONFIG_MACH_MX50_NIMBUS_TI_WIFI
#include <linux/wl12xx.h>
#endif

extern int __init mx50_rdp_init_mc13892(void);
#ifdef CONFIG_MXC_PMIC_MC34708
extern int __init mx50_rdp_init_mc34708(void);
#endif
extern struct cpu_wp *(*get_cpu_wp)(int *wp);
extern void (*set_num_cpu_wp)(int num);
extern struct dvfs_wp *(*get_dvfs_core_wp)(int *wp);
extern void __iomem *apll_base;

// For manual issue sd detction.
extern void mxc_mmc_force_detect(int id);
int sd1_cd_manual;
//extern int system_status,rtc_wakeup_poweroff;
#ifdef CONFIG_MXC_MC13892_BATTERY
extern int low_bat_alarm;
#endif

extern int orderly_poweroff(bool force);
#ifdef CONFIG_MXC_MC13892_BATTERY
extern void battery_update_status(void);
#endif
void nimbus_poweroff(void);


static void mx50_suspend_enter(void);
static void mx50_suspend_exit(void);
static void nimbus_gpio_iomux_init(void);
static void nimbus_gpio_iomux_deinit(void);

static int num_cpu_wp;

static iomux_v3_cfg_t mx50_rdp[] = {
	/* SD1 */
	MX50_PAD_ECSPI2_SS0__GPIO_4_19, // SD1_WP
	MX50_PAD_EIM_CRE__GPIO_1_27,    // SD1_CD
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

    /* GPIOs */
    MX50_PAD_EPDC_D1__GPIO_3_1,
    MX50_PAD_EPDC_D2__GPIO_3_2,
    MX50_PAD_EPDC_D3__GPIO_3_3,
    MX50_PAD_EPDC_D4__GPIO_3_4,
    MX50_PAD_EPDC_D5__GPIO_3_5,
    MX50_PAD_EPDC_D6__GPIO_3_6,
    MX50_PAD_EPDC_D7__GPIO_3_7,
    MX50_PAD_EPDC_D8__GPIO_3_8,
    MX50_PAD_EPDC_D9__GPIO_3_9,
    MX50_PAD_ECSPI2_SCLK__GPIO_4_16,
    MX50_PAD_ECSPI2_MOSI__GPIO_4_17,

	/* UART pad setting */
	MX50_PAD_UART1_TXD__UART1_TXD,
	MX50_PAD_UART1_RXD__UART1_RXD,
	MX50_PAD_UART1_RTS__UART1_RTS,
	MX50_PAD_UART2_TXD__UART2_TXD,
	MX50_PAD_UART2_RXD__UART2_RXD,
	MX50_PAD_UART2_CTS__UART2_CTS,
	MX50_PAD_UART2_RTS__UART2_RTS,

	//MX50_PAD_I2C1_SCL__I2C1_SCL,
	//MX50_PAD_I2C1_SDA__I2C1_SDA,
	//MX50_PAD_I2C2_SCL__I2C2_SCL,
	//MX50_PAD_I2C2_SDA__I2C2_SDA,

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

	MX50_PAD_CSPI_SS0__CSPI_SS0,
	MX50_PAD_ECSPI1_MOSI__CSPI_SS1,
	MX50_PAD_CSPI_MOSI__CSPI_MOSI,
	MX50_PAD_CSPI_MISO__CSPI_MISO,

    
   	MX50_PAD_PWM1__GPIO_6_24, // for POWER_KET_DET
   	MX50_PAD_WDOG__GPIO_6_28, // for WDOG_B

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

static struct fec_platform_data fec_data = {
	.phy = PHY_INTERFACE_MODE_RMII,
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

/* Fixed voltage regulator DCDC_3V15 */
static struct regulator_consumer_supply fixed_volt_reg_consumers[] = {
	{
		/* Wi-Fi module */
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

static int sdhc_write_protect(struct device *dev)
{
	unsigned short rc = 0;

	if (to_platform_device(dev)->id == 0)
		rc = gpio_get_value(SD1_WP);
	else if (to_platform_device(dev)->id == 1)
        rc=0;
	else if (to_platform_device(dev)->id == 2)
		rc=0;
    
	return rc;
}
static unsigned int sdhc_get_card_det_status(struct device *dev)
{
	int ret = 0;
	if (to_platform_device(dev)->id == 0)
	{
	    /*
	     * The CD of SD1 is reverse detction desgin.
	     * Nomral: Low, Card-Insert: High
	     * We also need to patched esdhc_cd_callback() of mx_sdhc.c
	     * for irq trigger edge. For logic, the CD is low active.
	     * But, we need to change trigger edge to fit physical signal.
	     */
	    ret = gpio_get_value(SD1_CD);
        ret= (ret==1)?0:1;

        if(sd1_cd_manual)
        {
            ret=1;
            //printk(KERN_INFO "sdhc_get_card_det_status> sd1_cd_manual=%d\n",sd1_cd_manual);
        }

        //gpio_direction_output(SD1_CHANGE_CLEAR,1); 
        //mdelay(1);
        //gpio_direction_output(SD1_CHANGE_CLEAR,0);
    }
	else if (to_platform_device(dev)->id == 1)
        ret=1;
	else if (to_platform_device(dev)->id == 2)
		ret=1;
    //printk(KERN_INFO "sdhc_get_card_det_status>id=%d,ret=%d",to_platform_device(dev)->id,ret);
	return ret;
}

// For external SD slot
static struct mxc_mmc_platform_data mmc1_data = {
	.ocr_mask = MMC_VDD_27_28 | MMC_VDD_28_29 | MMC_VDD_29_30
		| MMC_VDD_31_32,
	.caps = MMC_CAP_4_BIT_DATA ,
	.min_clk = 400000,
	.max_clk = 50000000, 
	.card_inserted_state = 0,
	.status = sdhc_get_card_det_status,
	.wp_status = sdhc_write_protect,
	.clock_mmc = "esdhc_clk",
	.power_mmc = NULL,
};

// For Wi-Fi module
static struct mxc_mmc_platform_data mmc2_data = {
	.ocr_mask = MMC_VDD_27_28 | MMC_VDD_28_29 | MMC_VDD_29_30
		| MMC_VDD_31_32,
#ifdef CONFIG_MACH_MX50_NIMBUS_TI_WIFI_PM_PATCH
	.caps = MMC_CAP_4_BIT_DATA | MMC_CAP_NONREMOVABLE | MMC_CAP_POWER_OFF_CARD,
#else
	.caps = MMC_CAP_4_BIT_DATA,
#endif
	.min_clk = 400000,
	.max_clk = 21500000, 
    .card_inserted_state = 1,
	.status = sdhc_get_card_det_status,
	.wp_status = sdhc_write_protect,
	.clock_mmc = "esdhc_clk",
};

/*
 * NOTE: Due to possible timing issue, it is not recommended to use usdhc
 * with DDR mode enabled. Instead, we use esdhc for DDR mode by default.
 */
// For iNAND 
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
static void mx50_arm2_usb_set_vbus(bool enable)
{
	gpio_set_value(USB_OTG_PWR, enable);
}
void nimbus_led_ctrl(int led,int action,int isOn);

#ifdef CONFIG_MXC_MC13892_BATTERY0
void nimbus_low_power_handle(void *action)
{
    
    printk("Low power found: system power down! (%d)\n",system_status);
    if(system_status!=0)
    {
        //mc13892_power_off();
        orderly_poweroff(1);
    }
}

static void mxc_register_low_power_event(void)
{
	pmic_event_callback_t low_power_event;
    printk("Register low power event handle.\n");
	low_power_event.param = (void *)1;
	low_power_event.func = (void *)nimbus_low_power_handle;
	pmic_event_subscribe(EVENT_LOBATLI, low_power_event);
}
#endif


#ifdef POWERKEY_HANDLE
static int mxc_pwrkey_getstatus(int id);
int powerkey_disabled,powerkey_found,do_powerdown;

#define POWERKEY_ACTIVE_TIME (2*HZ)
static struct timer_list nimbus_powerkey_timer;

static void nimbus_powerkey_timer_handler(unsigned long ptr)
{
    printk("<INFO> Power Key is valid.(timeout) -> Power down!\n");
    do_powerdown=1;
}
#endif


static void mxc_register_powerkey(pwrkey_callback pk_cb)
{
	pmic_event_callback_t power_key_event;

	power_key_event.param = (void *)1;
	power_key_event.func = (void *)pk_cb;
	pmic_event_subscribe(EVENT_PWRONI, power_key_event);
	power_key_event.param = (void *)3;
	pmic_event_subscribe(EVENT_PWRON3I, power_key_event);
#ifdef CONFIG_MXC_MC13892_BATTERY0
    mxc_register_low_power_event();
#endif

#ifdef POWERKEY_HANDLE
    printk("<INFO> Power key detect...\n");
    mxc_pwrkey_getstatus(1);
#endif

}



static int mxc_pwrkey_getstatus(int id)
{
	int sense, off = 3;
    int key_pressed=1;

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
		key_pressed=0;

    //printk("mxc_pwrkey_getstatus> off=%d,id=%d,key_pressed=%d\n",off,id,key_pressed);

#ifdef POWERKEY_HANDLE
    if(powerkey_disabled==0 && do_powerdown==0)
    {
        if(key_pressed)
        {
            printk("Power key found...\n");
            init_timer(&nimbus_powerkey_timer);
            nimbus_powerkey_timer.function=nimbus_powerkey_timer_handler;
            nimbus_powerkey_timer.data=(unsigned long)key_pressed;
            nimbus_powerkey_timer.expires=jiffies + POWERKEY_ACTIVE_TIME;
            add_timer(&nimbus_powerkey_timer);
        }
        else
        {
            printk("Power key released.\n");
            if(timer_pending(&nimbus_powerkey_timer))
            {
                //printk("del timer.\n");
                del_timer(&nimbus_powerkey_timer);
            }
        }
    }
#endif
    

	return key_pressed;
}

static struct power_key_platform_data pwrkey_data = {
	.key_value = KEY_F4,
	.register_pwrkey = mxc_register_powerkey,
	.get_key_status = mxc_pwrkey_getstatus,
};


static struct mxs_dma_plat_data dma_apbh_data = {
	.chan_base = MXS_DMA_CHANNEL_AHB_APBH,
	.chan_num = MXS_MAX_DMA_CHANNELS,
};

#ifdef POWEROFF_SYSTEM_IN_IDLE_TIMEOUT
#define IDLE_TIMEOUT_MIN            300 // 5 min
#define IDLE_TIMEOUT_DEFAULT        1200 // 20 min.
#define IDLE_CHECKING_TIME_UNIT     30  // unit sec.

#define IDLE_CHECK_DELAY (IDLE_CHECKING_TIME_UNIT*HZ)
static struct timer_list nimbus_idle_timer;
static int system_idle_cnt,system_idle_cnt_limited;
static int system_idle_timeout=IDLE_TIMEOUT_DEFAULT;
static int system_idle_check_disabled=1;


static void nimbus_idle_check_timer(unsigned long ptr)
{

    nimbus_idle_timer.expires=jiffies + IDLE_CHECK_DELAY;

    if(system_idle_cnt < system_idle_cnt_limited)
    {
        system_idle_cnt++;
    }
    else
    {
        printk("<INFO> System idle timeout, system auto-power off. (limite: %d ,unit:30 sec)\n",system_idle_cnt_limited);
        nimbus_poweroff();
        
    }
    //printk("<INFO> %d,%d\n",system_idle_cnt,system_idle_cnt_limited);
    add_timer(&nimbus_idle_timer);
}

void nimbus_idle_check_init()
{
    if(system_idle_check_disabled!=0)
    {
        init_timer(&nimbus_idle_timer);
        nimbus_idle_timer.function=nimbus_idle_check_timer;
        nimbus_idle_timer.data=(unsigned long)&system_idle_cnt;
        system_idle_cnt_limited=system_idle_timeout/IDLE_CHECKING_TIME_UNIT;
        nimbus_idle_timer.expires=jiffies + IDLE_CHECK_DELAY;
        add_timer(&nimbus_idle_timer);
        system_idle_check_disabled=0;
        system_idle_cnt=0;
        printk("<INFO> Start system idle checking. (%d)\n",system_idle_cnt_limited);
    }
    
}

void nimbus_idle_check_stop()
{
    if(timer_pending(&nimbus_idle_timer))
    {
        del_timer(&nimbus_idle_timer);
        printk("<INFO> Stop system idle checking. (%d)\n",system_idle_cnt_limited);
        system_idle_check_disabled=1;
    }
    else
    {
        printk("<WARNING> System idle checking is stop.\n");
    }
}
void nimbus_idle_check_reset()
{
    system_idle_cnt=0;
}
#else
void nimbus_idle_check_reset();
void nimbus_idle_check_init();
void nimbus_idle_check_stop();
#endif



#define LEDID_WLAN 0
#define LEDID_WIFI  1
#define LEDID_CHARGER_RED   2
#define LEDID_CHARGER_GREEN 3
static int led_status=0;
static int led_ctrl_disabled=0;
void nimbus_led_ctrl(int led,int action,int isOn)
{

    int io_level;

    if(led_ctrl_disabled)
    {

        printk("<INFO> LED ctrl is disabled!(id:%d,action:%d)\n",led,isOn);
        return;

    }
        
    switch(led)
    {
        case LEDID_WLAN:
            io_level = (isOn==1)?0:1;
            gpio_request(LED_WLAN, "wlan-led");
            gpio_direction_output(LED_WLAN, io_level);
            break;
        case LEDID_WIFI:   
            io_level = (isOn==1)?0:1;
            gpio_request(LED_LAN, "lan-led");
            gpio_direction_output(LED_LAN, io_level);
            break;
        case LEDID_CHARGER_RED: 
            io_level = (isOn==1)?0:1;
            gpio_request(LED_CHARGER_RED, "charger-led_red");
            gpio_direction_output(LED_CHARGER_RED, io_level);
            break;
        case LEDID_CHARGER_GREEN:
            io_level = (isOn==1)?0:1;
            gpio_request(LED_CHARGER_GREEN, "charger-led_green");
            gpio_direction_output(LED_CHARGER_GREEN, io_level);
            break;
        default:
            printk("<WARNING> LED unknown.(id:%d,action:%d)\n",led,isOn);
            return;
    }
    if(isOn)
    {
        led_status|=(0x1<< led);
    }
    else
    {
        led_status&= ~(0x1<< led);
    }
}
EXPORT_SYMBOL_GPL(nimbus_led_ctrl);


void nimbus_led_ctrl_userspace(int led,int action,int isOn)
{

    int io_level;
#if 0
    if(rtc_wakeup_poweroff)
    {

        printk("<INFO> LED ctrl is disabled (rtc_wakeup_poweroff)!(id:%d,action:%d)\n",led,isOn);
        return;

    }
#endif    
    
    switch(led)
    {
        case LEDID_WLAN:
            io_level = (isOn==1)?0:1;
            gpio_request(LED_WLAN, "wlan-led");
            gpio_direction_output(LED_WLAN, io_level);
            break;
        case LEDID_WIFI:   
            io_level = (isOn==1)?0:1;
            gpio_request(LED_WIFI, "lan-led");
            gpio_direction_output(LED_WIFI, io_level);
            break;
        case LEDID_CHARGER_RED: 
            io_level = (isOn==1)?0:1;
            gpio_request(LED_CHARGER_RED, "charger-led_red");
            gpio_direction_output(LED_CHARGER_RED, io_level);
            break;
        case LEDID_CHARGER_GREEN:
            io_level = (isOn==1)?0:1;
            gpio_request(LED_CHARGER_GREEN, "charger-led_green");
            gpio_direction_output(LED_CHARGER_GREEN, io_level);
            break;
        default:
            printk("<WARNING> LED unknown.(id:%d,action:%d)\n",led,isOn);
            return;
    }
    if(isOn)
    {
        led_status|=(0x1<< led);
    }
    else
    {
        led_status&= ~(0x1<< led);
    }
}
EXPORT_SYMBOL_GPL(nimbus_led_ctrl_userspace);


void nimbus_poweroff(void)
{
    led_ctrl_disabled=1;
    nimbus_led_ctrl_userspace(LEDID_WLAN,0,0);
    nimbus_led_ctrl_userspace(LEDID_WIFI,0,0);
    nimbus_led_ctrl_userspace(LEDID_CHARGER_RED,0,0);
    nimbus_led_ctrl_userspace(LEDID_CHARGER_GREEN,0,0);
    orderly_poweroff(1);
}

#if 1 // ndef CONFIG_FSL_UTP

static struct gpio_led gpio_leds[] = {
    {
        .name   = "lan_led",
        .default_trigger = "none",
        .gpio   = LED_LAN,
        .active_low = true,
        .default_state = LEDS_GPIO_DEFSTATE_OFF,
    },
    {
        .name   = "wlan_led",
        .default_trigger = "none",
        .gpio   =  LED_WLAN, 
        .active_low = true,
        .default_state = LEDS_GPIO_DEFSTATE_OFF,
    },

};

static struct gpio_led_platform_data gpio_led_info = {
	.leds		= gpio_leds,
	.num_leds	= ARRAY_SIZE(gpio_leds),
};

static struct platform_device leds_gpio = {
	.name	= "leds-gpio",
	.id	= -1,
	.dev	= {
		.platform_data	= &gpio_led_info,
	},
};

#endif


int nimbus_get_sd1_cd(void)
{
    return gpio_get_value(SD1_CD);
}
EXPORT_SYMBOL_GPL(nimbus_get_sd1_cd);

void nimbus_sd1_fake_eject_on(void)
{
    printk("nimbus_sd1_fake_eject_on>\n");
    // SD1_CD normal in low
    if(gpio_get_value(SD1_CD) && sd1_cd_manual==0)
    {
        printk("nimbus_sd1_fake_eject_on> Card found!\n");
        sd1_cd_manual=1;
        mxc_mmc_force_detect(0);
        //mdelay(1500);
    }    
}

EXPORT_SYMBOL_GPL(nimbus_sd1_fake_eject_on);

void nimbus_sd1_fake_eject_off(void)
{
    printk("nimbus_sd1_fake_eject_off>\n");
    if(sd1_cd_manual)
    {
        sd1_cd_manual=0;
        mxc_mmc_force_detect(0);
    }
}

EXPORT_SYMBOL_GPL(nimbus_sd1_fake_eject_off);

 
static void nimbus_gpio_iomux_init()
{
	iomux_v3_cfg_t iomux_setting;

	iomux_setting = (MX50_PAD_ECSPI2_SCLK__GPIO_4_16 & \
				~MUX_PAD_CTRL_MASK) | \
				MUX_PAD_CTRL(PAD_CTL_PKE | PAD_CTL_DSE_HIGH);

	/* Enable the Pull/keeper */
	mxc_iomux_v3_setup_pad(iomux_setting);

    nimbus_led_ctrl(LEDID_WLAN,0,0);
    nimbus_led_ctrl(LEDID_WIFI,0,0);
    nimbus_led_ctrl(LEDID_CHARGER_RED,0,0);
    nimbus_led_ctrl(LEDID_CHARGER_GREEN,0,0);
    
    #if 0
    if( gpio_get_value(SD1_CHANGE_FOUND)==0)
    {
        printk("Found SD changed during suspending.\n");
        
    }

    gpio_direction_output(SD1_CHANGE_CLEAR,1); 
    mdelay(1);
    gpio_direction_output(SD1_CHANGE_CLEAR,0); 
    #endif
}

static void nimbus_gpio_iomux_deinit()
{
	iomux_v3_cfg_t iomux_setting;

	iomux_setting = (MX50_PAD_ECSPI2_SCLK__GPIO_4_16 & \
					~MUX_PAD_CTRL_MASK) | MUX_PAD_CTRL(0x4);

	mxc_iomux_v3_setup_pad(iomux_setting);

    nimbus_led_ctrl(LEDID_WLAN,0,0);
    nimbus_led_ctrl(LEDID_WIFI,0,0);
    nimbus_led_ctrl(LEDID_CHARGER_RED,0,0);
    nimbus_led_ctrl(LEDID_CHARGER_GREEN,0,0);
    
    #if 0
    printk("Clear SD changer before suspending.\n");
    gpio_direction_output(SD1_CHANGE_CLEAR,1); 
    mdelay(1);
    gpio_direction_output(SD1_CHANGE_CLEAR,0); 
    #endif

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

	//if (board_is_mx50_rd3()) {
		/* Enable the Pull/keeper */
		mxc_iomux_v3_setup_pad(iomux_setting);
		gpio_request(DCDC_EN, "dcdc-en");
		gpio_direction_output(DCDC_EN, 0);
	//}

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

    nimbus_gpio_iomux_deinit();

    //system_status=2;

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

	//if (board_is_mx50_rd3()) {
		/* Enable the Pull/keeper */
		mxc_iomux_v3_setup_pad(iomux_setting);
		gpio_request(DCDC_EN, "dcdc-en");
		gpio_direction_output(DCDC_EN, 1);
	//}

	mxc_iomux_v3_setup_multiple_pads(suspend_exit_pads,
			ARRAY_SIZE(suspend_exit_pads));

    nimbus_gpio_iomux_init();

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

#ifdef CONFIG_PROC_FS
/***********************************************
 * The proc for nimbus as following:
 * ---------------------------------
 * /proc/nimbus
 *         +- sd1eject : for software fake eject of SD1.
 *            This is used to manual eject in software.But card is still in system.
 *            0: fake eject off.
 *            1: fake eject on.
 *            Note: Please make sure the sd1_cd_manual=0 for pyhsical action of SD1 slot.
 */

static struct proc_dir_entry *nimbus_proc_dir;

static int nimbus_sd1eject_read_proc(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    int num=0;

    num += sprintf(page + num, "sd1_cd_manual: %d\n", sd1_cd_manual);

    //printk(KERN_INFO "nimbus_sd1eject_read_proc> sd1_cd_manual=%d\n",sd1_cd_manual);
    
    return num;
}

static int nimbus_sd1eject_write_proc(struct file *file, const char *buffer, unsigned long count, void *data)
{
    #ifndef NO_SDEJECT_SOFTWARE
    if(count>0)
    {
        switch(buffer[0])
        {
            case '0':
                sd1_cd_manual=0;
                 break;
            case '1':
                sd1_cd_manual=1;
                 break;
        }

        mxc_mmc_force_detect(0);
        //printk(KERN_INFO "nimbus_sd1eject_write_proc>count=%d\n",count);             
    }

    //printk(KERN_INFO "nimbus_sd1eject_write_proc> sd1_cd_manual=%d\n",sd1_cd_manual);
    #endif
    return count;
}

static int nimbus_leds_read_proc(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    int num=0;
    int isOff;

    num +=sprintf(page + num, "led_status: 0x%x\n", led_status);
    isOff=led_status&(0x1<<LEDID_WLAN);
    num += sprintf(page + num, "LED POWER: %s\n", ((isOff==0)?"OFF":"ON"));
    isOff=led_status&(0x1<<LEDID_WIFI);
    num += sprintf(page + num, "LED WIFI: %s\n", ((isOff==0)?"OFF":"ON"));
    isOff=led_status&(0x1<<LEDID_CHARGER_RED);
    num += sprintf(page + num, "LED CHARGER RED: %s\n", ((isOff==0)?"OFF":"ON"));
    isOff=led_status&(0x1<<LEDID_CHARGER_GREEN);
    num += sprintf(page + num, "LED CHARGER GREEN: %s\n", ((isOff==0)?"OFF":"ON"));
    
    return num;
}

static int nimbus_leds_write_proc(struct file *file, const char *buffer, unsigned long count, void *data)
{
    if(count>0)
    {
        switch(buffer[0])
        {
            case '0':
                nimbus_led_ctrl_userspace(LEDID_WLAN,0,1); 
                 break;
            case '1':
                nimbus_led_ctrl_userspace(LEDID_WLAN,0,0); 
                 break;
            case '2':
                nimbus_led_ctrl_userspace(LEDID_WIFI,0,1); 
                 break;
            case '3':
                nimbus_led_ctrl_userspace(LEDID_WIFI,0,0); 
                 break;
            case '4':
                nimbus_led_ctrl_userspace(LEDID_CHARGER_RED,0,1); 
                 break;
            case '5':
                nimbus_led_ctrl_userspace(LEDID_CHARGER_RED,0,0); 
                 break;
            case '6':
                nimbus_led_ctrl_userspace(LEDID_CHARGER_GREEN,0,1); 
                 break;
            case '7':
                nimbus_led_ctrl_userspace(LEDID_CHARGER_GREEN,0,0); 
                 break;                 
        }

    }

    
    return count;
}


static int nimbus_ledctrl_read_proc(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    int num=0;

    num += sprintf(page + num, "led_ctrl_disabled: %d\n", led_ctrl_disabled);

    return num;
}

static int nimbus_ledctrl_write_proc(struct file *file, const char *buffer, unsigned long count, void *data)
{
    if(count>0)
    {
        switch(buffer[0])
        {
            case '0':
                led_ctrl_disabled=0;
                #ifdef CONFIG_MXC_MC13892_BATTERY
                battery_update_status();
                #endif
                 break;
            case '1':
                led_ctrl_disabled=1;
                 break;
        }

    }
    return count;
}

static int nimbus_kernel_version_read_proc(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    int num=0;
    
    num += sprintf(page + num, "Revision: %s\n", NIMBUS_KERNEL_VERSION);
    
    return num;
}

#if 0
static int nimbus_suspending_read_proc(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    int num=0;

    //num += sprintf(page + num, "%d\n", system_status);

    return num;
}
#endif

#ifdef POWERKEY_HANDLE
static int nimbus_powerkeyctrl_read_proc(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    int num=0;

    num += sprintf(page + num, "powerkey_disabled: %d (0:processed by kernel.)\n", powerkey_disabled);

    return num;
}

static int nimbus_powerkeyctrl_write_proc(struct file *file, const char *buffer, unsigned long count, void *data)
{
    if(count>0)
    {
        switch(buffer[0])
        {
            case '0':
                powerkey_disabled=0;
                 break;
            case '1':
                powerkey_disabled=1;
                 break;
        }
    }
    return count;
}

static int nimbus_powerdown_read_proc(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    int num=0;

    num += sprintf(page + num, "%d\n", do_powerdown);

    return num;
}

#endif


#ifdef POWEROFF_SYSTEM_IN_IDLE_TIMEOUT
static int nimbus_idlecheckctrl_read_proc(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    int num=0;

    num += sprintf(page + num, "idle checking disabled: %d (0: Enabled.)\n", system_idle_check_disabled);

    return num;
}

static int nimbus_idlecheckctrl_write_proc(struct file *file, const char *buffer, unsigned long count, void *data)
{
    if(count>0)
    {
        switch(buffer[0])
        {
            case '0':
                if(system_idle_check_disabled)
                {
                    nimbus_idle_check_init();
                    system_idle_check_disabled=0;
                }
                break;
            case '1':
                if(system_idle_check_disabled==0)
                {
                    nimbus_idle_check_stop();
                    system_idle_check_disabled=1;
                }
                break;
        }
    }
    return count;
}

static int nimbus_idlecheck_timeout_read_proc(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    int num=0;

    num += sprintf(page + num, "idle checking timeout: %d (sec) (cnt:%d)\n", system_idle_timeout,system_idle_cnt);

    return num;
}

#define PROCFS_MAX_SIZE 1024
static int nimbus_idlecheck_timeout_write_proc(struct file *file, const char *buffer, unsigned long count, void *data)
{
    if(count>0)
    {
        char procfs_buffer[PROCFS_MAX_SIZE];
        int new_timeout;

        /* get buffer size */
        unsigned long procfs_buffer_size = count;
        if (procfs_buffer_size > PROCFS_MAX_SIZE ) {
           procfs_buffer_size = PROCFS_MAX_SIZE;
        }

        /* write data to the buffer */
        if ( copy_from_user(procfs_buffer, buffer, procfs_buffer_size) ) {
           return -EFAULT;
        }
        
        new_timeout = simple_strtol(procfs_buffer, NULL, 10);
        if(new_timeout>0)
        {
            if(new_timeout<IDLE_TIMEOUT_MIN)
            {   
                printk("<WARNING> Setting (%d) < 300 sec, reset to 300 sec.\n",new_timeout);
                new_timeout=IDLE_TIMEOUT_MIN;
            }
            printk("<INFO> idlecheck timeout (old): %d sec.\n",system_idle_timeout);
            printk("<INFO> idlecheck timeout (new): %d sec.\n",new_timeout);
            system_idle_timeout=new_timeout;
            system_idle_cnt=0;
            system_idle_cnt_limited=system_idle_timeout/IDLE_CHECKING_TIME_UNIT;
        }
    }
    return count;
}


#endif
static int nimbus_setup_proc_entry(void)
{
	struct proc_dir_entry *res;

	nimbus_proc_dir = proc_mkdir("nimbus",NULL);
	if (!nimbus_proc_dir) {
		printk(KERN_ERR "Failed to create proc/nimbus\n");
		return -ENOMEM;
	}

    res=create_proc_entry("sd1eject",S_IFREG | S_IRUGO | S_IWUGO ,nimbus_proc_dir);
    res->read_proc=nimbus_sd1eject_read_proc;
    res->write_proc=nimbus_sd1eject_write_proc;
    res=create_proc_entry("leds",S_IFREG | S_IRUGO | S_IWUGO ,nimbus_proc_dir);
    res->read_proc=nimbus_leds_read_proc;
    res->write_proc=nimbus_leds_write_proc;
    res=create_proc_entry("ledctrl",S_IFREG | S_IRUGO | S_IWUGO ,nimbus_proc_dir);
    res->read_proc=nimbus_ledctrl_read_proc;
    res->write_proc=nimbus_ledctrl_write_proc;
    res=create_proc_entry("version",S_IFREG | S_IRUGO ,nimbus_proc_dir);
    res->read_proc=nimbus_kernel_version_read_proc;
    //res=create_proc_entry("suspended",S_IFREG | S_IRUGO ,nimbus_proc_dir);
    //res->read_proc=nimbus_suspending_read_proc;
    #ifdef POWERKEY_HANDLE
    res=create_proc_entry("powerkeyctrl",S_IFREG | S_IRUGO | S_IWUGO,nimbus_proc_dir);
    res->read_proc=nimbus_powerkeyctrl_read_proc;
    res->write_proc=nimbus_powerkeyctrl_write_proc;
    res=create_proc_entry("powerdown",S_IFREG | S_IRUGO ,nimbus_proc_dir);
    res->read_proc=nimbus_powerdown_read_proc;
    #endif

    #ifdef POWEROFF_SYSTEM_IN_IDLE_TIMEOUT
    res=create_proc_entry("idlecheck_disabled",S_IFREG | S_IRUGO | S_IWUGO,nimbus_proc_dir);
    res->read_proc=nimbus_idlecheckctrl_read_proc;
    res->write_proc=nimbus_idlecheckctrl_write_proc;
    res=create_proc_entry("idlecheck_timeout",S_IFREG | S_IRUGO | S_IWUGO,nimbus_proc_dir);
    res->read_proc=nimbus_idlecheck_timeout_read_proc;
    res->write_proc=nimbus_idlecheck_timeout_write_proc;
    #endif
	
	return 0;
}

EXPORT_SYMBOL(nimbus_proc_dir);
#endif

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


	gpio_request(SD1_WP, "sdhc1-wp");
	gpio_direction_input(SD1_WP);

	gpio_request(SD1_CD, "sdhc1-cd");
	gpio_direction_input(SD1_CD);

    //gpio_request(WIFI_RESET, "wifi-reset");
	//gpio_direction_output(WIFI_RESET,1);

    gpio_request(SD_POWER, "sd-power");
	gpio_direction_output(SD_POWER,0);

    // Power on, clean the SD1_CHANGE_FOUND.
    //printk("Clean SD changed event.\n");
    gpio_request(SD1_CHANGE_CLEAR, "sd-clear");
	gpio_direction_output(SD1_CHANGE_CLEAR,1); 
    mdelay(1);
    gpio_direction_output(SD1_CHANGE_CLEAR,0); 

    gpio_request(SD1_CHANGE_FOUND, "sd-found");
    gpio_direction_input(SD1_CHANGE_FOUND);

    #ifdef CONFIG_MACH_MX50_NIMBUS_RESCUE
    nimbus_led_ctrl(LEDID_CHARGER_RED,0,1);
    nimbus_led_ctrl(LEDID_CHARGER_GREEN,0,0);
    #endif
    gpio_request(POWER_KET_DET, "power-key-detect");
	gpio_direction_input(POWER_KET_DET);

    gpio_request(WDOG_B, "wdog-b");
	gpio_direction_input(WDOG_B);

	/* USB OTG PWR */
	gpio_request(USB_OTG_PWR, "usb otg power");
	gpio_direction_output(USB_OTG_PWR, 0);

	/* Disable all keepers */
	mxc_iomux_v3_setup_pad(cspi_keeper);
}

#ifdef CONFIG_MACH_MX50_NIMBUS_TI_WIFI
// Note: IRQ & EN are 1.8V interface
#define WLAN_IRQ_IO	(3*32 + 1)	/* GPIO_4_1 */
#define WLAN_EN_IO 	(3*32 + 3)	/* GPIO_4_3 */
#define WLAN_IRQ 	gpio_to_irq(WLAN_IRQ_IO)

void wlan_set_power(int on)
{
    if (on) {
        gpio_set_value(WLAN_EN_IO, 1);
        mdelay(200);
        printk("WLAN-EN:ON\n");
    } else {
        mdelay(200);
        gpio_set_value(WLAN_EN_IO, 0);
        printk("WLAN-EN:OFF\n");
    }
}
EXPORT_SYMBOL(wlan_set_power);

static struct wl12xx_platform_data ti_wlan_data __initdata = {
        .irq = WLAN_IRQ,
        .board_ref_clock = WL12XX_REFCLOCK_38_XTAL,//127x
};

static void ti_wifi_init(void)
{
    // Make sure your GPIO & PAD setting is match
	mxc_iomux_v3_setup_pad(MX50_PAD_KEY_ROW0__GPIO_4_1);
	gpio_request(WLAN_IRQ_IO, "wlan_irq");
	gpio_direction_input(WLAN_IRQ_IO);

	mxc_iomux_v3_setup_pad(MX50_PAD_KEY_ROW1__GPIO_4_3);
	gpio_request(WLAN_EN_IO, "wlan_en");
#ifdef CONFIG_MACH_MX50_NIMBUS_TI_WIFI_PM_PATCH
	gpio_direction_output(WLAN_EN_IO , 0);
#else
	gpio_direction_output(WLAN_EN_IO , 1);
#endif

    if (wl12xx_set_platform_data(&ti_wlan_data))
    {
        pr_err("Error setting wl12xx data\n");
    }
}
#endif


/*!
 * Board specific initialization.
 */
static void __init mxc_board_init(void)
{
    //system_status=0;

	/* SD card detect irqs */
	mxcsdhc1_device.resource[2].start = gpio_to_irq(SD1_CD);
	mxcsdhc1_device.resource[2].end = gpio_to_irq(SD1_CD);

    // Wi-Fi module is always present in system.
    mmc2_data.card_inserted_state=1;
    mmc2_data.status=NULL;
    mmc2_data.wp_status=NULL;

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

    // make sure eMMC will be first scan , the order is important
	mxc_register_device(&mxcsdhc3_device, &mmc3_data); 
	mxc_register_device(&mxcsdhc2_device, &mmc2_data);
    mxc_register_device(&mxcsdhc1_device, &mmc1_data);

	mxc_register_device(&mxc_ssi1_device, NULL);
	mxc_register_device(&mxc_ssi2_device, NULL);
	mxc_register_device(&mxc_fec_device, &fec_data);
	mxc_register_device(&mxc_pwm1_device, NULL);
	mxc_register_device(&mxc_rngb_device, NULL);
//	mxc_register_device(&dcp_device, NULL);
	mxc_register_device(&mxc_powerkey_device, &pwrkey_data);
	mxc_register_device(&fixed_volt_reg_device, &fixed_volt_reg_pdata);
	if (mx50_revision() >= IMX_CHIP_REVISION_1_1)
		mxc_register_device(&mxc_zq_calib_device, NULL);
#ifdef CONFIG_MXC_PMIC_MC34708
	if (board_is_mx50_rd3())
		mx50_rdp_init_mc34708();
	else
#endif        
		mx50_rdp_init_mc13892();

/*
	pm_power_off = mxc_power_off;
	*/
	mx5_set_otghost_vbus_func(mx50_arm2_usb_set_vbus);
	mx5_usb_dr_init();
	//mx5_usbh1_init(); // we didn't use this port.
	mxc_register_device(&mxc_perfmon, &mxc_perfmon_data);
    #ifdef CONFIG_PROC_FS
    nimbus_setup_proc_entry();
    #endif

    // LEDS
	mxc_register_device(&leds_gpio,&gpio_led_info);



#ifdef CONFIG_MACH_MX50_NIMBUS_TI_WIFI
    /*Jorjin added*/
	ti_wifi_init();
#endif

#ifdef POWEROFF_SYSTEM_IN_IDLE_TIMEOUT
    nimbus_idle_check_init();
#endif  

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
MACHINE_START(MX50_RDP, "Freescale MX50 Platform - Nimbus@QSI(WG7311-2A) Ver: "NIMBUS_KERNEL_VERSION)
	/* Maintainer: Freescale Semiconductor, Inc. */
	.fixup = fixup_mxc_board,
	.map_io = mx5_map_io,
	.init_irq = mx5_init_irq,
	.init_machine = mxc_board_init,
	.timer = &mxc_timer,
MACHINE_END
