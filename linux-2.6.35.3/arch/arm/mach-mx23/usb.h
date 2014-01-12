/*
 * Copyright (C) 2009-2010 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

extern int usbotg_init(struct platform_device *pdev);
extern void usbotg_uninit(struct fsl_usb2_platform_data *pdata);
extern int gpio_usbotg_utmi_active(void);
extern void gpio_usbotg_utmi_inactive(void);
extern struct platform_device *host_pdev_register(struct resource *res,
						  int n_res,
						  struct fsl_usb2_platform_data
						  *config);
extern void fsl_platform_set_usb_phy_dis(struct fsl_usb2_platform_data *pdata,
					 bool enable);
extern int fsl_usb_host_init(struct platform_device *pdev);
extern void fsl_usb_host_uninit(struct fsl_usb2_platform_data *pdata);
extern int gpio_usbh2_active(void);
extern void gpio_usbh2_inactive(void);

/*
 * Determine which platform_data struct to use for the DR controller,
 * based on which transceiver is configured.
 * PDATA is a pointer to it.
 */
static struct fsl_usb2_platform_data __maybe_unused dr_utmi_config;
#define PDATA (&dr_utmi_config)

/*
 * Used to set pdata->operating_mode before registering the platform_device.
 * If OTG is configured, the controller operates in OTG mode,
 * otherwise it's either host or device.
 */
#ifdef CONFIG_USB_OTG
#define DR_UDC_MODE	FSL_USB2_DR_OTG
#define DR_HOST_MODE	FSL_USB2_DR_OTG
#else
#define DR_UDC_MODE	FSL_USB2_DR_DEVICE
#define DR_HOST_MODE	FSL_USB2_DR_HOST
#endif

#ifdef CONFIG_USB_EHCI_ARC_OTG
static inline void dr_register_host(struct resource *r, int rs)
{
	PDATA->operating_mode = DR_HOST_MODE;
	host_pdev_register(r, rs, PDATA);
}
#else
static inline void dr_register_host(struct resource *r, int rs)
{
}
#endif

#ifdef CONFIG_USB_GADGET_ARC
static struct platform_device dr_udc_device;

static inline void dr_register_udc(void)
{
	PDATA->operating_mode = DR_UDC_MODE;
	dr_udc_device.dev.platform_data = PDATA;

	if (platform_device_register(&dr_udc_device))
		printk(KERN_ERR "usb: can't register DR gadget\n");
	else
		printk(KERN_INFO "usb: DR gadget (%s) registered\n",
		       PDATA->transceiver);
}
#else
static inline void dr_register_udc(void)
{
}
#endif

#ifdef CONFIG_USB_OTG
static struct platform_device dr_otg_device;

/*
 * set the proper operating_mode and
 * platform_data pointer, then register the
 * device.
 */
static inline void dr_register_otg(void)
{
	PDATA->operating_mode = FSL_USB2_DR_OTG;
	dr_otg_device.dev.platform_data = PDATA;

	if (platform_device_register(&dr_otg_device))
		printk(KERN_ERR "usb: can't register otg device\n");
	else
		printk(KERN_INFO "usb: DR OTG registered\n");
}
#else
static inline void dr_register_otg(void)
{
}
#endif
