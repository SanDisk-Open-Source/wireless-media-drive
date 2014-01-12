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
#ifdef CONFIG_QSI_NIMBUS

// for MGF u-boot.bin
//#define CONFIG_MFG
#ifdef CONFIG_MFG
#define QSI_NIMBUS_MFG "[MFG] "
#else
#define QSI_NIMBUS_MFG ""
#endif

#define QSI_PRODUCT "QSI: Wi-Fi Storage "
#define QSI_MODEL   "(Nimbus) " QSI_NIMBUS_MFG
#define QSI_VERSION "1.1.0"
#define QSI_NIMBUS_VERSION_STRING QSI_PRODUCT QSI_MODEL "Version: " QSI_VERSION

/*-----------------------------------------------------------------------------
 * U-Boot firmware release record.
 *-----------------------------------------------------------------------------
 * [20120530] version: 1.0.1
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
 * [20120629] Version: 1.1.0
 * 1. LPDDR1
 */
#ifndef CONFIG_MFG
// for code run in EVB
//#define QSI_NIMBUS_EVB_HW

/* brent.tan add nimbus cmd & driver parameters for QSI Nimbus on 20120517*/
#define QSI_NIMBUS_IO_CONFIG_DEF
//#define QSI_NIMBUS_PMIC_SPI_PATCH_DEF // V1.0.1
//#define QSI_NIMBUS_ME_DEMO // johnson 20120525,for LED checking
//#define QSI_NIMBUS_DRAM_16BIT

//Port by shawn 20120521 uBoot if condition shell supporting
#define CONFIG_SYS_HUSH_PARSER 
#define CONFIG_CMD_AUTOSCRIPT
#define CONFIG_SYS_PROMPT_HUSH_PS2 CONFIG_SYS_PROMPT


#endif
#endif

 /* High Level Configuration Options */
#define CONFIG_MXC
#define CONFIG_MX50
#define CONFIG_MX50_RD3
#ifndef CONFIG_QSI_NIMBUS
#define CONFIG_LPDDR2 
#endif
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
#define CONFIG_BOOTDELAY	3
#endif

#define CONFIG_PRIME	"FEC0"

#define CONFIG_LOADADDR		0x70800000	/* loadaddr env var */
#define CONFIG_RD_LOADADDR	(CONFIG_LOADADDR + 0x300000)

#ifdef CONFIG_QSI_NIMBUS

#ifdef CONFIG_MFG // johnson 20120527

#define CONFIG_BOOTARGS         "console=ttymxc0,115200 "\
				"rdinit=/linuxrc"
#define CONFIG_BOOTCOMMAND      "bootm 0x70800000 0x70B00000"
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
        "updateuboot=mmcinfo 0;fatload mmc 0 70800000 u-boot.bin;mmcinfo 2;mmc write 2 70800400 2 400;\0"
        
		/*"bootcmd=if test -n ${rescue_mode}; then if test ${rescue_mode} -eq 1; then echo Sandisk Rescue Booting...; run bootcmd_rescue; else run bootcmd_mmc; fi; else run bootcmd_mmc; fi;\0"*/

#else
#define	CONFIG_EXTRA_ENV_SETTINGS					\
		"netdev=eth0\0"						\
		"ethprime=FEC0\0"					\
		"uboot=u-boot.bin\0"			\
		"kernel=uImage\0"				\
		"nfsroot=/opt/eldk/arm\0"				\
		"bootargs_base=setenv bootargs console=ttymxc0,115200\0"\
		"bootargs_nfs=setenv bootargs ${bootargs} root=/dev/nfs "\
		"ip=dhcp nfsroot=${serverip}:${nfsroot},v3,tcp\0"\
		"bootcmd_net=run bootargs_base bootargs_nfs; "		\
		"tftpboot ${loadaddr} ${kernel}; bootm\0"	\
		"bootargs_mmc=setenv bootargs ${bootargs} ip=dhcp "     \
		"root=/dev/mmcblk0p2 rootwait\0"                \
		"bootcmd_mmc=run bootargs_base bootargs_mmc; bootm\0"   \
		"bootcmd=run bootcmd_net\0"                             \

#endif

#define CONFIG_ARP_TIMEOUT	200UL

/*
 * Miscellaneous configurable options
 */
#define CONFIG_SYS_LONGHELP		/* undef to save memory */
#ifdef CONFIG_QSI_NIMBUS
#define CONFIG_SYS_PROMPT		"Nimbus@" QSI_VERSION ": "
#else
#define CONFIG_SYS_PROMPT		"MX50_RD3 U-Boot > "
#endif
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
#ifndef CONFIG_QSI_NIMBUS
/*
 * DDR ZQ calibration
 */
#define CONFIG_ZQ_CALIB


/*
 * I2C Configs
 */

#define CONFIG_CMD_I2C          1
#endif

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
#ifndef CONFIG_QSI_NIMBUS
#define CONFIG_CMD_NAND
#endif

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
#ifdef CONFIG_QSI_NIMBUS
#ifdef QSI_NIMBUS_DRAM_16BIT
#define PHYS_SDRAM_1_SIZE	(128 * 1024 * 1024)
#else
#define PHYS_SDRAM_1_SIZE	(128 * 1024 * 1024)
#endif
#else
#define PHYS_SDRAM_1_SIZE	(512 * 1024 * 1024)
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
