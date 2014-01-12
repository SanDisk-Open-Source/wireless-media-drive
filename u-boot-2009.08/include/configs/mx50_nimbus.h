/*
 * Copyright (C) 2011 Freescale Semiconductor, Inc.
 *
 * Configuration settings for the MX50-RDP Freescale board.
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

#ifndef __CONFIG_H
#define __CONFIG_H

#include <asm/arch/mx50.h>

#define CONFIG_QSI_NIMBUS // Johnson 201204,iMX50 + MC34708

// for MGF u-boot.bin
//#define CONFIG_MFG
#ifdef CONFIG_MFG
#define QSI_NIMBUS_MFG "[MFG] "
#else
#define QSI_NIMBUS_MFG ""
#endif

#define QSI_PRODUCT "QSI: Wi-Fi Storage "
#define QSI_MODEL   "(Nimbus) " QSI_NIMBUS_MFG
#define QSI_VERSION "1.2.5"
#define QSI_NIMBUS_VERSION_STRING QSI_PRODUCT QSI_MODEL "Version: " QSI_VERSION

/*-----------------------------------------------------------------------------
 * U-Boot firmware release record.
 *-----------------------------------------------------------------------------
 * [20120530] version: 1.0.1 (EVT, LPDDR2, MC34708, AR6103G)
 * 1. Added command: pmic,poweroff
 * 2. Added QSI_NIMBUS_ME_DEMO. This is for ME demo LED effect. Included power
 *    button detection to power off device by USEROFFSPI.
 * 3. Merged MFG building by CONFIG_MFG.
 * 4. bootcmd for eMMC booting.
 * 5. PMIC SPI clksrc patch (keyword: QSI_NIMBUS_PMIC_SPI_PATCH_DEF)
 *=============================================================================
 * [20120601] version: 1.0.2
 * 1. Enabled SD & Wi-Fi power when power on.
 * 2. Adjust PMIC parameter for charger profile & battery.
 *-----------------------------------------------------------------------------
 * [20120629] Version: 1.1.0 (Pre-DVT, LPDDR1 + MC13892 + WG7311-2A)
 * 1. LPDDR1
 *
 * [2012070?] Version: 1.1.1     
 *
 * [20120706] Version: 1.1.2 
 * 1. Patched for MC13892 -> PATCHED_MC13892_BATTERY_20120706
 * 2. Correct VSBED setting of MC13892
 *      bat_check: for batter check when booting. Set as 0 to disable.
 *      bat_limited: for check voltage limmited setting range: 300-400.
 * 3. Added cleanenv command to restore setting to defalut.
 * 4. Added ethaddr setting.
 * 5. Added restore default setting in case CRC error in ENV.
 *
 * [20120712] Version: 1.1.3 
 * 1. Added rescue system firmware feature.
 *    shell cmd:
 *      sdreacue - rescue system by SD.
 *          Required: SD as FAT, files: rescue,nimbus.img
 *      sduboot - update uboot
 *          Required: SD as FAT, file: u-boot.bin
 * 2. Updated MFG bootargs.
 * [20120716] Version: 1.1.4 
 * 1. Revised MFG booting bootargs.
 * 2. Added rescue image TAG check for SD card rescue.
 * 3. Added VUSBIN=1 in PMIC' setting of ChargerUSB1 (50).
 * 4. Removed battery check when booting.
 * 5. Added command:
 *      batvoltage - measurment voltage of battery.
 *      batcurrent - measurment current of battery.
 * 6. Added parameter for version code of u-boot.
 *      example: ubootver=1.1.4
 * [20120730] Version: 1.1.5 
 * 1. Set WDOG_B and PWR_KEY_DET as Input pin.
 * 2. Added command:
 *      post - A test for memory and LED.
 * 3. Changed rescue mode checking keyword (fw_upgrade_rescue).
 * 4. Enabled battery check when booting.
 *     - Limited: 3500mv
 * 5. Revised post and pwrkey command for led behavior.
 * 6. Revised error code showing for sdrescue. 
 *     - Limited showing loop.
 *     - base from 1 for error code.
 * [20120822] Version: 1.1.6
 * 1. Changed power led to charger green led.
 * 2. Added low battery power on indicate by led.
 * 3. Changed sd-rescue behavior.
 *    a). Long pressed and held 10 sec. to enter rescue. Then the LED will chnge to RED.
 *    b). In case of failure, system will turn off.
 *    c). Detection SD first then to check rescue power event.
 * [20120822] Version: 1.1.7
 * 1. Revised low battery power on with USB plug-in to check charging status.
 * [20120927] Version: 1.1.8
 * 1. Changed power key debounce time to 125ms.
 * [20121011] Version: 1.1.9
 * 1. Disabled low power power on protect. (QSI_LOW_BAT_POWER_ON_PORTECT)
 * 2. Revised 125ms setting problem. 
 *
 * [20121112] Version: 1.1.10
 * 1. Turn on GREEN led as soon as possible.
 * 2. Enabled QSI_LOW_BAT_POWER_ON_PORTECT (Limit: 3V,Charging Timeout: 420 sec.)
 * [20121111] Version: 1.1.11
 * 1. Revised QSI_LOW_BAT_POWER_ON_PORTECT
 * [20121218] Version: 1.1.12
 * 1. adjust charging current setting. (Changed from 720mA to 480mA)
 * 2. Added vbus voltage detection.
 * [20130207] Version: 1.2.0
 * 1. Changed power led behavior.
 * 2. Changed bootdelay to 1 sec.
 * [20130221] Version: 1.2.1
 * 1. Added uboot version code checking.
 * 2. Fixed VCHRG setting issue. (found issue from 1.1.12 ,setting in 3.8V. we need to 4.2V)
 * 3. Pre-charging timeout 10 min.
 * [20130228] Version: 1.2.2
 * 1. Changed LED state from 3 state to 2 state.
 * 2. Changed Nomral state of LED boundary to 3.6V (Old: 3.71)
 * [20130301] Version: 1.2.3
 * 1. Reviewed CHARGE setting. 
 * 2. Disable HW charging controlling.
 * [20130322] Version: 1.2.4
 * 1. Added charging controlling in u-boot. (QSI_PWRST_ALIGN)
 *    If system power on by USB, then it will keep in u-boot until power key valid.
 * [20130409] Version: 1.2.5
 * 1. Refine  check_booting_case
 * 2. Regress function: QSI_PWRST_ALIGN (disabled)
 */


/* brent.tan add nimbus cmd & driver parameters for QSI Nimbus on 20120517*/
#define QSI_NIMBUS_IO_CONFIG_DEF  1
#define QSI_NIMBUS_PREDVT_PMIC_DEF  // Brent add for pmic 13892 in pre-DVT phase on 20120630, for 13892 setting*/

#ifndef CONFIG_MFG
//#define QSI_NIMBUS_PMIC_SPI_PATCH_DEF // V1.0.1, for MC34708
//#define QSI_NIMBUS_ME_DEMO // johnson 20120525,for LED checking
//#define QSI_NIMBUS_DRAM_16BIT

//Port by shawn 20120521 uBoot if condition shell supporting
#define CONFIG_SYS_HUSH_PARSER 
#define CONFIG_CMD_AUTOSCRIPT
#define CONFIG_SYS_PROMPT_HUSH_PS2 CONFIG_SYS_PROMPT
#define QSI_NIMBUS_FIRMWARE_RESCUE // 20120712, for rescue system from SD via u-boot.
#define QSI_NIMBUS_POWER_KEY_VALID_TIME 10 // unit: sec , time of power key event for QSI_NIMBUS_FIRMWARE_RESCUE
#define QSI_NIMBUS_RESCUE_TAG   "Nimbus-rescue-V"

#define QSI_NIMBUS_POWERON_IDENTIFY
#define QSI_NIMBUS_POWERON_EVENT_VALID_TIME 70 //unit: ms

#define PATCHED_MC13892_BATTERY_20120706
#define QSI_LOW_BAT_POWER_ON_PORTECT
#define QSI_LOW_BAT_POWER_ON_TIMEOUT    600 //unit: sec, timeout for lower-battery charging
//#define QSI_PWRST_ALIGN

#ifdef  QSI_PWRST_ALIGN
#define QSI_PWRBY_KEY       (0x01)           
#define QSI_PWRBY_USB       (0x02)
#define QSI_PWRBY_REBOOT    (0x04)
#endif
//#define QSI_NIMBUS_IOPULSE_CMD
#endif

 /* High Level Configuration Options */
#define CONFIG_MXC
#define CONFIG_MX50
//#define CONFIG_MX50_RD3
//#define CONFIG_LPDDR2 
#define CONFIG_FLASH_HEADER
#define CONFIG_FLASH_HEADER_OFFSET 0x400

#define CONFIG_SKIP_RELOCATE_UBOOT

/*
#define CONFIG_ARCH_CPU_INIT
#define CONFIG_ARCH_MMU
*/

#define CONFIG_MX50_HCLK_FREQ	24000000
#define CONFIG_SYS_PLL2_FREQ    400
#define CONFIG_SYS_AHB_PODF     2
#define CONFIG_SYS_AXIA_PODF    0
#define CONFIG_SYS_AXIB_PODF    1

#define CONFIG_DISPLAY_CPUINFO
#define CONFIG_DISPLAY_BOARDINFO

#define CONFIG_SYS_64BIT_VSPRINTF

#define BOARD_LATE_INIT
/*
 * Disabled for now due to build problems under Debian and a significant
 * increase in the final file size: 144260 vs. 109536 Bytes.
 */

#define CONFIG_CMDLINE_TAG		1	/* enable passing of ATAGs */
#define CONFIG_REVISION_TAG		1
#define CONFIG_SETUP_MEMORY_TAGS	1
#define CONFIG_INITRD_TAG		1

/*
 * Size of malloc() pool
 */
#define CONFIG_SYS_MALLOC_LEN		(CONFIG_ENV_SIZE + 2 * 1024 * 1024)
/* size in bytes reserved for initial data */
#define CONFIG_SYS_GBL_DATA_SIZE	128

/*
 * Hardware drivers
 */
#define CONFIG_MXC_UART
#define CONFIG_UART_BASE_ADDR	UART1_BASE_ADDR

/* allow to overwrite serial and ethaddr */
#define CONFIG_ENV_OVERWRITE
#define CONFIG_CONS_INDEX		1
#define CONFIG_BAUDRATE			115200
#define CONFIG_SYS_BAUDRATE_TABLE	{9600, 19200, 38400, 57600, 115200}

/***********************************************************
 * Command definition
 ***********************************************************/

#include <config_cmd_default.h>

#define CONFIG_CMD_PING
#define CONFIG_CMD_DHCP
#define CONFIG_CMD_MII
#define CONFIG_CMD_NET
#define CONFIG_NET_RETRY_COUNT  100
#define CONFIG_NET_MULTI 1
#define CONFIG_BOOTP_SUBNETMASK
#define CONFIG_BOOTP_GATEWAY
#define CONFIG_BOOTP_DNS

#ifndef CONFIG_MFG
#define CONFIG_CMD_MMC
#define CONFIG_CMD_ENV
#endif


/*#define CONFIG_CMD */
#define CONFIG_REF_CLK_FREQ CONFIG_MX50_HCLK_FREQ

#undef CONFIG_CMD_IMLS

#ifdef QSI_NIMBUS_ME_DEMO
#define CONFIG_BOOTDELAY	1
#else
#define CONFIG_BOOTDELAY	1
#endif

#define CONFIG_PRIME	"FEC0"

#define CONFIG_LOADADDR		0x70800000	/* loadaddr env var */
#define CONFIG_RD_LOADADDR	(CONFIG_LOADADDR + 0x300000)



#ifdef CONFIG_MFG // johnson 20120527

// johnson 20120715 for MFG
#define CONFIG_BOOTARGS         "console=ttymxc0,115200 root=/dev/ram0 rw "

				
#define CONFIG_BOOTCOMMAND      "bootm 0x70800000"
#define CONFIG_ENV_IS_EMBEDDED
#undef CONFIG_BOOTDELAY
#define CONFIG_BOOTDELAY	0
#endif

#define	CONFIG_EXTRA_ENV_SETTINGS					\
		"netdev=eth0\0"						\
		"ethprime=FEC0\0"					\
		"uboot=u-boot.bin\0"			\
		"kernel=uImage\0"				\
		"nfsroot=/home/qsi/freescale/rootfs/rootfs-imx50\0"				\
		"bootargs_base=setenv bootargs console=ttymxc0,115200\0"\
		"bootargs_nfs=setenv bootargs ${bootargs} root=/dev/nfs "\
		"ip=dhcp nfsroot=${serverip}:${nfsroot},v3,tcp\0"\
        "bootargs_rescue=setenv bootargs ${bootargs} root=/dev/ram0 rw\0"\
        "bootcmd_rescue=run bootargs_base bootargs_rescue; mmc read 2 ${loadaddr} 0x2800 0x5000; bootm\0"\
        "rescue_mode=0\0"\
		"bootcmd_net=run bootargs_base bootargs_nfs; " \
		"tftpboot ${loadaddr} ${kernel}; bootm\0"	\
		"bootargs_mmc=setenv bootargs ${bootargs} console=ttymxc0,115200 "     \
		"root=/dev/mmcblk0p2 rootwait rw\0"                \
		"bootcmd_mmc=run bootargs_base bootargs_mmc;mmc read 2 ${loadaddr} 0x800 0x1800;bootm\0"   \
		"bootcmd=run bootcmd_mmc\0"\
        "ipaddr=192.168.0.10\0" \
        "serverip=192.168.0.140\0" \
        "gatewayip=192.168.0.1\0"\
        "ethaddr=00:10:20:03:04:05\0" \
        "wifi_addr=00:01:02:03:04:05\0" \
        "tftpuboot=tftp u-boot.bin;mmcinfo 2;mmc write 2 70800400 2 400;\0" \
        "tftpuImage=tftp uImage;mmcinfo 2;mmc write 2 70800000 0x800 0x1800;\0" \
        "ubootver="QSI_VERSION"\0" \
        "bat_check=1\0" \
        "bat_limited=3500\0" \
        "lowbat_limited=3000\0"


#define CONFIG_ARP_TIMEOUT	200UL

/*
 * Miscellaneous configurable options
 */
#define CONFIG_SYS_LONGHELP		/* undef to save memory */
#define CONFIG_SYS_PROMPT		"Nimbus@" QSI_VERSION ": "
#define CONFIG_AUTO_COMPLETE
#define CONFIG_SYS_CBSIZE		256	/* Console I/O Buffer Size */
/* Print Buffer Size */
#define CONFIG_SYS_PBSIZE (CONFIG_SYS_CBSIZE + sizeof(CONFIG_SYS_PROMPT) + 16)
#define CONFIG_SYS_MAXARGS	16	/* max number of command args */
#define CONFIG_SYS_BARGSIZE CONFIG_SYS_CBSIZE /* Boot Argument Buffer Size */

#define CONFIG_SYS_MEMTEST_START	0	/* memtest works on */
#define CONFIG_SYS_MEMTEST_END		0x10000

#undef	CONFIG_SYS_CLKS_IN_HZ		/* everything, incl board info, in Hz */

#define CONFIG_SYS_LOAD_ADDR		CONFIG_LOADADDR

#define CONFIG_SYS_HZ				1000

#define CONFIG_CMDLINE_EDITING	1

#define CONFIG_FEC0_IOBASE	FEC_BASE_ADDR
#define CONFIG_FEC0_PINMUX	-1
#define CONFIG_FEC0_PHY_ADDR	-1
#define CONFIG_FEC0_MIIBASE	-1

#ifndef CONFIG_MFG
#define CONFIG_GET_FEC_MAC_ADDR_FROM_IIM
#endif

#define CONFIG_MXC_FEC
#define CONFIG_MII
#define CONFIG_MII_GASKET
#define CONFIG_DISCOVER_PHY

/*
 * DDR ZQ calibration
 */
//#define CONFIG_ZQ_CALIB


/*
 * I2C Configs
 */

//#define CONFIG_CMD_I2C          1


#ifdef CONFIG_CMD_I2C
	#define CONFIG_HARD_I2C         1
	#define CONFIG_I2C_MXC          1
	#define CONFIG_SYS_I2C_PORT             I2C2_BASE_ADDR
	#define CONFIG_SYS_I2C_SPEED            100000
	#define CONFIG_SYS_I2C_SLAVE            0xfe
#endif


/*
 * SPI Configs
 */
#define CONFIG_FSL_SF		1
#define CONFIG_CMD_SPI
#define CONFIG_CMD_SF
#define CONFIG_SPI_FLASH_IMX_M25PXX	1
#define CONFIG_SPI_FLASH_CS	1
#define CONFIG_IMX_CSPI
#define IMX_CSPI_VER_0_7        1
#define MAX_SPI_BYTES		(8 * 4)
#define CONFIG_IMX_SPI_PMIC
#define CONFIG_IMX_SPI_PMIC_CS 0

/*
 * MMC Configs
 */
#ifdef CONFIG_CMD_MMC
	#define CONFIG_MMC				1
	#define CONFIG_GENERIC_MMC
	#define CONFIG_IMX_MMC
	#define CONFIG_SYS_FSL_ESDHC_NUM        3
	#define CONFIG_SYS_FSL_ESDHC_ADDR       0
	#define CONFIG_SYS_MMC_ENV_DEV  0
	#define CONFIG_DOS_PARTITION	1
	#define CONFIG_CMD_FAT		1
	#define CONFIG_CMD_EXT2		1

	/* detect whether ESDHC1, ESDHC2, or ESDHC3 is boot device */
	#define CONFIG_DYNAMIC_MMC_DEVNO

	#define CONFIG_BOOT_PARTITION_ACCESS
#ifndef CONFIG_MFG
	#define CONFIG_EMMC_DDR_PORT_DETECT
	#define CONFIG_EMMC_DDR_MODE

	/* Indicate to esdhc driver which ports support 8-bit data */
	#define CONFIG_MMC_8BIT_PORTS		0x6   /* SD2 and SD3 */

	/* Uncomment the following define to enable uSDHC instead
	 * of eSDHC on SD3 port for SDR mode since eSDHC timing on MX50
	 * is borderline for SDR mode. DDR mode will be disabled when this
	 * define is enabled since the uSDHC timing on MX50 is borderline
	 * for DDR mode. */

	/*#define CONFIG_MX50_ENABLE_USDHC_SDR	1*/
#endif
#endif

/*
 * GPMI Nand Configs
 */
//#define CONFIG_CMD_NAND

#ifdef CONFIG_CMD_NAND
	#define CONFIG_NAND_GPMI
	#define CONFIG_GPMI_NFC_SWAP_BLOCK_MARK
	#define CONFIG_GPMI_NFC_V2

	#define CONFIG_GPMI_REG_BASE	GPMI_BASE_ADDR
	#define CONFIG_BCH_REG_BASE	BCH_BASE_ADDR

	#define NAND_MAX_CHIPS		8
	#define CONFIG_SYS_NAND_BASE		0x40000000
	#define CONFIG_SYS_MAX_NAND_DEVICE	1
#endif

/*
 * APBH DMA Configs
 */
#define CONFIG_APBH_DMA

#ifdef CONFIG_APBH_DMA
	#define CONFIG_APBH_DMA_V2
	#define CONFIG_MXS_DMA_REG_BASE	ABPHDMA_BASE_ADDR
#endif

/*-----------------------------------------------------------------------
 * Stack sizes
 *
 * The stack sizes are set up in start.S using the settings below
 */
#define CONFIG_STACKSIZE	(128 * 1024)	/* regular stack */

/*-----------------------------------------------------------------------
 * Physical Memory Map
 */
#define CONFIG_NR_DRAM_BANKS	1
#define PHYS_SDRAM_1		CSD0_BASE_ADDR
#ifdef QSI_NIMBUS_DRAM_16BIT
#define PHYS_SDRAM_1_SIZE	(128 * 1024 * 1024)
#else
#define PHYS_SDRAM_1_SIZE	(128 * 1024 * 1024)
#endif
#define iomem_valid_addr(addr, size) \
	(addr >= PHYS_SDRAM_1 && addr <= (PHYS_SDRAM_1 + PHYS_SDRAM_1_SIZE))

/*-----------------------------------------------------------------------
 * FLASH and environment organization
 */
#define CONFIG_SYS_NO_FLASH

/* Monitor at beginning of flash */
#ifndef CONFIG_MFG
#define CONFIG_FSL_ENV_IN_MMC
#endif

#define CONFIG_ENV_SECT_SIZE    (128 * 1024)
#define CONFIG_ENV_SIZE         CONFIG_ENV_SECT_SIZE

#if defined(CONFIG_FSL_ENV_IN_NAND)
	#define CONFIG_ENV_IS_IN_NAND 1
	#define CONFIG_ENV_OFFSET	0x100000
#elif defined(CONFIG_FSL_ENV_IN_MMC)
	#define CONFIG_ENV_IS_IN_MMC	1
	#define CONFIG_ENV_OFFSET	(768 * 1024)
#elif defined(CONFIG_FSL_ENV_IN_SF)
	#define CONFIG_ENV_IS_IN_SPI_FLASH	1
	#define CONFIG_ENV_SPI_CS		1
	#define CONFIG_ENV_OFFSET       (768 * 1024)
#else
	#define CONFIG_ENV_IS_NOWHERE	1
#endif
#endif				/* __CONFIG_H */
