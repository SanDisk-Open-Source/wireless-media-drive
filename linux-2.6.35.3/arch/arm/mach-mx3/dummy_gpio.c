/*
 * Copyright 2007-2010 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

#include <linux/errno.h>
#include <linux/module.h>

void gpio_uart_active(int port, int no_irda) {}
EXPORT_SYMBOL(gpio_uart_active);

void gpio_uart_inactive(int port, int no_irda) {}
EXPORT_SYMBOL(gpio_uart_inactive);

void gpio_gps_active(void) {}
EXPORT_SYMBOL(gpio_gps_active);

void gpio_gps_inactive(void) {}
EXPORT_SYMBOL(gpio_gps_inactive);

void config_uartdma_event(int port) {}
EXPORT_SYMBOL(config_uartdma_event);

void gpio_spi_active(int cspi_mod) {}
EXPORT_SYMBOL(gpio_spi_active);

void gpio_spi_inactive(int cspi_mod) {}
EXPORT_SYMBOL(gpio_spi_inactive);

void gpio_owire_active(void) {}
EXPORT_SYMBOL(gpio_owire_active);

void gpio_owire_inactive(void) {}
EXPORT_SYMBOL(gpio_owire_inactive);

void gpio_i2c_active(int i2c_num) {}
EXPORT_SYMBOL(gpio_i2c_active);

void gpio_i2c_inactive(int i2c_num) {}
EXPORT_SYMBOL(gpio_i2c_inactive);

void gpio_i2c_hs_active(void) {}
EXPORT_SYMBOL(gpio_i2c_hs_active);

void gpio_i2c_hs_inactive(void) {}
EXPORT_SYMBOL(gpio_i2c_hs_inactive);

void gpio_pmic_active(void) {}
EXPORT_SYMBOL(gpio_pmic_active);

void gpio_activate_audio_ports(void) {}
EXPORT_SYMBOL(gpio_activate_audio_ports);

void gpio_sdhc_active(int module) {}
EXPORT_SYMBOL(gpio_sdhc_active);

void gpio_sdhc_inactive(int module) {}
EXPORT_SYMBOL(gpio_sdhc_inactive);

void gpio_sensor_select(int sensor) {}

void gpio_sensor_active(unsigned int csi) {}
EXPORT_SYMBOL(gpio_sensor_active);

void gpio_sensor_inactive(unsigned int csi) {}
EXPORT_SYMBOL(gpio_sensor_inactive);

void gpio_ata_active(void) {}
EXPORT_SYMBOL(gpio_ata_active);

void gpio_ata_inactive(void) {}
EXPORT_SYMBOL(gpio_ata_inactive);

void gpio_nand_active(void) {}
EXPORT_SYMBOL(gpio_nand_active);

void gpio_nand_inactive(void) {}
EXPORT_SYMBOL(gpio_nand_inactive);

void gpio_keypad_active(void) {}
EXPORT_SYMBOL(gpio_keypad_active);

void gpio_keypad_inactive(void) {}
EXPORT_SYMBOL(gpio_keypad_inactive);

int gpio_usbotg_hs_active(void)
{
	return 0;
}
EXPORT_SYMBOL(gpio_usbotg_hs_active);

void gpio_usbotg_hs_inactive(void) {}
EXPORT_SYMBOL(gpio_usbotg_hs_inactive);

void gpio_fec_active(void) {}
EXPORT_SYMBOL(gpio_fec_active);

void gpio_fec_inactive(void) {}
EXPORT_SYMBOL(gpio_fec_inactive);

void gpio_spdif_active(void) {}
EXPORT_SYMBOL(gpio_spdif_active);

void gpio_spdif_inactive(void) {}
EXPORT_SYMBOL(gpio_spdif_inactive);

void gpio_mlb_active(void) {}
EXPORT_SYMBOL(gpio_mlb_active);

void gpio_mlb_inactive(void) {}
EXPORT_SYMBOL(gpio_mlb_inactive);
