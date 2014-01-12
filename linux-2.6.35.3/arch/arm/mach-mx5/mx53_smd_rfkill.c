/*
 * Copyright (C) 2011 Freescale Semiconductor, Inc. All Rights Reserved.
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

/*!
 * @file mxc_bt_rfkill.c
 *
 * @brief This driver is implement a rfkill control interface of bluetooth
 * chip on i.MX serial boards. Register the power regulator function and
 * reset function in platform data.
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/rfkill.h>
#include <mach/hardware.h>
#include <mach/mxc_rfkill.h>

static int mxc_bt_set_block(void *rfkdata, bool blocked)
{
	struct mxc_bt_rfkill_platform_data *data = rfkdata;
	int ret;

	pr_info("rfkill: BT RF going to : %s\n", blocked ? "off" : "on");
	if (!blocked)
		ret = data->power_change(1);
	else
		ret = data->power_change(0);

	return ret;
}

static const struct rfkill_ops mxc_bt_rfkill_ops = {
	.set_block = mxc_bt_set_block,
};

static int mxc_bt_rfkill_probe(struct platform_device *dev)
{
	int rc;
	struct rfkill *rfk;

	struct mxc_bt_rfkill_platform_data *data = dev->dev.platform_data;

	if (data->power_change == NULL) {
		rc = -EINVAL;
		dev_err(&dev->dev, "no power_change function\n");
		goto error_check_func;
	}

	rfk = rfkill_alloc("mxc-bt", &dev->dev, RFKILL_TYPE_BLUETOOTH,
			   &mxc_bt_rfkill_ops, data);

	if (!rfk) {
		rc = -ENOMEM;
		goto error_rfk_alloc;
	}

	rfkill_set_led_trigger_name(rfk, "mxc_bt");

	rc = rfkill_register(rfk);
	if (rc)
		goto error_rfkill;

	platform_set_drvdata(dev, rfk);
	printk(KERN_INFO "mxc_bt_rfkill driver success loaded\n");
	return 0;

error_rfkill:
	rfkill_destroy(rfk);
error_rfk_alloc:
error_check_func:
	return rc;
}

static int __devexit mxc_bt_rfkill_remove(struct platform_device *dev)
{
	struct mxc_bt_rfkill_platform_data *data = dev->dev.platform_data;
	struct rfkill *rfk = platform_get_drvdata(dev);

	platform_set_drvdata(dev, NULL);

	if (rfk) {
		rfkill_unregister(rfk);
		rfkill_destroy(rfk);
	}

	data->power_change(0);

	return 0;
}

static struct platform_driver mxc_bt_rfkill_drv = {
	.driver = {
		.name = "mxc_bt_rfkill",
	},

	.probe	= mxc_bt_rfkill_probe,
	.remove = __devexit_p(mxc_bt_rfkill_remove),
};

static int __init mxc_bt_rfkill_init(void)
{
	return platform_driver_register(&mxc_bt_rfkill_drv);
}

module_init(mxc_bt_rfkill_init);

static void __exit mxc_bt_rfkill_exit(void)
{
	platform_driver_unregister(&mxc_bt_rfkill_drv);
}

module_exit(mxc_bt_rfkill_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_DESCRIPTION("RFKill control interface of BT on MX53 SMD");
