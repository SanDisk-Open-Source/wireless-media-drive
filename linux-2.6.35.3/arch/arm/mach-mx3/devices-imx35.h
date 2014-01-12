/*
 * Copyright (C) 2010 Pengutronix
 * Copyright (C) 2010 Freescale Semiconductor, Inc.
 * Uwe Kleine-Koenig <u.kleine-koenig@pengutronix.de>
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License version 2 as published by the
 * Free Software Foundation.
 */
#include <mach/mx35.h>
#include <mach/devices-common.h>

#define imx35_add_flexcan0(pdata)	\
	imx_add_flexcan(0, MX35_CAN1_BASE_ADDR, SZ_16K, MX35_INT_CAN1, pdata)
#define imx35_add_flexcan1(pdata)	\
	imx_add_flexcan(1, MX35_CAN2_BASE_ADDR, SZ_16K, MX35_INT_CAN2, pdata)

#define imx35_add_imx_i2c0(pdata)	\
	imx_add_imx_i2c(0, MX35_I2C1_BASE_ADDR, SZ_4K, MX35_INT_I2C1, pdata)
#define imx35_add_imx_i2c1(pdata)	\
	imx_add_imx_i2c(1, MX35_I2C2_BASE_ADDR, SZ_4K, MX35_INT_I2C2, pdata)
#define imx35_add_imx_i2c2(pdata)	\
	imx_add_imx_i2c(2, MX35_I2C3_BASE_ADDR, SZ_4K, MX35_INT_I2C3, pdata)

extern const struct imx_imx_uart_1irq_data imx35_imx_uart_data[] __initconst;
#define imx35_add_imx_uart(id, pdata)	\
	imx_add_imx_uart_1irq(&imx35_imx_uart_data[id], pdata)
#define imx35_add_imx_uart0(pdata)	imx35_add_imx_uart(0, pdata)
#define imx35_add_imx_uart1(pdata)	imx35_add_imx_uart(1, pdata)
#define imx35_add_imx_uart2(pdata)	imx35_add_imx_uart(2, pdata)

#define imx35_add_mxc_nand(pdata)	\
	imx_add_mxc_nand_v21(MX35_NFC_BASE_ADDR, MX35_INT_NANDFC, pdata)

#define imx35_add_spi_imx0(pdata)	\
	imx_add_spi_imx(0, MX35_CSPI1_BASE_ADDR, SZ_4K, MX35_INT_CSPI1, pdata)
#define imx35_add_spi_imx1(pdata)	\
	imx_add_spi_imx(1, MX35_CSPI2_BASE_ADDR, SZ_4K, MX35_INT_CSPI2, pdata)

#define imx35_add_imx_mmc0(pdata)       \
	imx_add_imx_mmc(0, MX35_MMC_SDHC1_BASE_ADDR, SZ_4K, MX35_INT_MMC_SDHC1, MXC_PSEUDO_IRQ_SD1_CD, pdata)
#define imx35_add_imx_mmc1(pdata)	\
	imx_add_imx_mmc(1, MX35_MMC_SDHC2_BASE_ADDR, SZ_4K, MX35_INT_MMC_SDHC2, 0, pdata)
