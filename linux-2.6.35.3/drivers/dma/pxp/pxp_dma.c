/*
 * Copyright (C) 2010-2011 Freescale Semiconductor, Inc.
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */
/*
 * Based on STMP378X PxP driver
 * Copyright 2008-2009 Embedded Alley Solutions, Inc All Rights Reserved.
 */
#include <linux/dma-mapping.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/dmaengine.h>
#include <linux/pxp_dma.h>
#include <linux/timer.h>
#include <linux/clk.h>
#include <linux/workqueue.h>

#include "regs-pxp.h"

#define	PXP_DOWNSCALE_THRESHOLD		0x4000

static LIST_HEAD(head);
static int timeout_in_ms = 600;

struct pxp_dma {
	struct dma_device dma;
};

struct pxps {
	struct platform_device *pdev;
	struct clk *clk;
	void __iomem *base;
	int irq;		/* PXP IRQ to the CPU */

	spinlock_t lock;
	struct mutex clk_mutex;
	int clk_stat;
#define	CLK_STAT_OFF		0
#define	CLK_STAT_ON		1
	int pxp_ongoing;
	int lut_state;

	struct device *dev;
	struct pxp_dma pxp_dma;
	struct pxp_channel channel[NR_PXP_VIRT_CHANNEL];
	wait_queue_head_t done;
	struct work_struct work;

	/* describes most recent processing configuration */
	struct pxp_config_data pxp_conf_state;

	/* to turn clock off when pxp is inactive */
	struct timer_list clk_timer;
};

#define to_pxp_dma(d) container_of(d, struct pxp_dma, dma)
#define to_tx_desc(tx) container_of(tx, struct pxp_tx_desc, txd)
#define to_pxp_channel(d) container_of(d, struct pxp_channel, dma_chan)
#define to_pxp(id) container_of(id, struct pxps, pxp_dma)

#define PXP_DEF_BUFS	2
#define PXP_MIN_PIX	8

#define PXP_WAITCON	((__raw_readl(pxp->base + HW_PXP_STAT) & \
				BM_PXP_STAT_IRQ) != BM_PXP_STAT_IRQ)

static uint32_t pxp_s0_formats[] = {
	PXP_PIX_FMT_RGB24,
	PXP_PIX_FMT_RGB565,
	PXP_PIX_FMT_RGB555,
	PXP_PIX_FMT_YUV420P,
	PXP_PIX_FMT_YUV422P,
};

/*
 * PXP common functions
 */
static void dump_pxp_reg(struct pxps *pxp)
{
	dev_dbg(pxp->dev, "PXP_CTRL 0x%x",
		__raw_readl(pxp->base + HW_PXP_CTRL));
	dev_dbg(pxp->dev, "PXP_STAT 0x%x",
		__raw_readl(pxp->base + HW_PXP_STAT));
	dev_dbg(pxp->dev, "PXP_OUTBUF 0x%x",
		__raw_readl(pxp->base + HW_PXP_OUTBUF));
	dev_dbg(pxp->dev, "PXP_OUTBUF2 0x%x",
		__raw_readl(pxp->base + HW_PXP_OUTBUF2));
	dev_dbg(pxp->dev, "PXP_OUTSIZE 0x%x",
		__raw_readl(pxp->base + HW_PXP_OUTSIZE));
	dev_dbg(pxp->dev, "PXP_S0BUF 0x%x",
		__raw_readl(pxp->base + HW_PXP_S0BUF));
	dev_dbg(pxp->dev, "PXP_S0UBUF 0x%x",
		__raw_readl(pxp->base + HW_PXP_S0UBUF));
	dev_dbg(pxp->dev, "PXP_S0VBUF 0x%x",
		__raw_readl(pxp->base + HW_PXP_S0VBUF));
	dev_dbg(pxp->dev, "PXP_S0PARAM 0x%x",
		__raw_readl(pxp->base + HW_PXP_S0PARAM));
	dev_dbg(pxp->dev, "PXP_S0BACKGROUND 0x%x",
		__raw_readl(pxp->base + HW_PXP_S0BACKGROUND));
	dev_dbg(pxp->dev, "PXP_S0CROP 0x%x",
		__raw_readl(pxp->base + HW_PXP_S0CROP));
	dev_dbg(pxp->dev, "PXP_S0SCALE 0x%x",
		__raw_readl(pxp->base + HW_PXP_S0SCALE));
	dev_dbg(pxp->dev, "PXP_OLn 0x%x",
		__raw_readl(pxp->base + HW_PXP_OLn(0)));
	dev_dbg(pxp->dev, "PXP_OLnSIZE 0x%x",
		__raw_readl(pxp->base + HW_PXP_OLnSIZE(0)));
	dev_dbg(pxp->dev, "PXP_OLnPARAM 0x%x",
		__raw_readl(pxp->base + HW_PXP_OLnPARAM(0)));
	dev_dbg(pxp->dev, "PXP_CSCCOEF0 0x%x",
		__raw_readl(pxp->base + HW_PXP_CSCCOEF0));
	dev_dbg(pxp->dev, "PXP_CSCCOEF1 0x%x",
		__raw_readl(pxp->base + HW_PXP_CSCCOEF1));
	dev_dbg(pxp->dev, "PXP_CSCCOEF2 0x%x",
		__raw_readl(pxp->base + HW_PXP_CSCCOEF2));
	dev_dbg(pxp->dev, "PXP_CSC2CTRL 0x%x",
		__raw_readl(pxp->base + HW_PXP_CSC2CTRL));
	dev_dbg(pxp->dev, "PXP_CSC2COEF0 0x%x",
		__raw_readl(pxp->base + HW_PXP_CSC2COEF0));
	dev_dbg(pxp->dev, "PXP_CSC2COEF1 0x%x",
		__raw_readl(pxp->base + HW_PXP_CSC2COEF1));
	dev_dbg(pxp->dev, "PXP_CSC2COEF2 0x%x",
		__raw_readl(pxp->base + HW_PXP_CSC2COEF2));
	dev_dbg(pxp->dev, "PXP_CSC2COEF3 0x%x",
		__raw_readl(pxp->base + HW_PXP_CSC2COEF3));
	dev_dbg(pxp->dev, "PXP_CSC2COEF4 0x%x",
		__raw_readl(pxp->base + HW_PXP_CSC2COEF4));
	dev_dbg(pxp->dev, "PXP_CSC2COEF5 0x%x",
		__raw_readl(pxp->base + HW_PXP_CSC2COEF5));
	dev_dbg(pxp->dev, "PXP_LUT_CTRL 0x%x",
		__raw_readl(pxp->base + HW_PXP_LUT_CTRL));
	dev_dbg(pxp->dev, "PXP_LUT 0x%x", __raw_readl(pxp->base + HW_PXP_LUT));
	dev_dbg(pxp->dev, "PXP_HIST_CTRL 0x%x",
		__raw_readl(pxp->base + HW_PXP_HIST_CTRL));
	dev_dbg(pxp->dev, "PXP_HIST2_PARAM 0x%x",
		__raw_readl(pxp->base + HW_PXP_HIST2_PARAM));
	dev_dbg(pxp->dev, "PXP_HIST4_PARAM 0x%x",
		__raw_readl(pxp->base + HW_PXP_HIST4_PARAM));
	dev_dbg(pxp->dev, "PXP_HIST8_PARAM0 0x%x",
		__raw_readl(pxp->base + HW_PXP_HIST8_PARAM0));
	dev_dbg(pxp->dev, "PXP_HIST8_PARAM1 0x%x",
		__raw_readl(pxp->base + HW_PXP_HIST8_PARAM1));
	dev_dbg(pxp->dev, "PXP_HIST16_PARAM0 0x%x",
		__raw_readl(pxp->base + HW_PXP_HIST16_PARAM0));
	dev_dbg(pxp->dev, "PXP_HIST16_PARAM1 0x%x",
		__raw_readl(pxp->base + HW_PXP_HIST16_PARAM1));
	dev_dbg(pxp->dev, "PXP_HIST16_PARAM2 0x%x",
		__raw_readl(pxp->base + HW_PXP_HIST16_PARAM2));
	dev_dbg(pxp->dev, "PXP_HIST16_PARAM3 0x%x",
		__raw_readl(pxp->base + HW_PXP_HIST16_PARAM3));
}

static bool is_yuv(u32 pix_fmt)
{
	if ((pix_fmt == PXP_PIX_FMT_YUYV) |
	    (pix_fmt == PXP_PIX_FMT_UYVY) |
	    (pix_fmt == PXP_PIX_FMT_Y41P) |
	    (pix_fmt == PXP_PIX_FMT_YUV444) |
	    (pix_fmt == PXP_PIX_FMT_NV12) |
	    (pix_fmt == PXP_PIX_FMT_GREY) |
	    (pix_fmt == PXP_PIX_FMT_YVU410P) |
	    (pix_fmt == PXP_PIX_FMT_YUV410P) |
	    (pix_fmt == PXP_PIX_FMT_YVU420P) |
	    (pix_fmt == PXP_PIX_FMT_YUV420P) |
	    (pix_fmt == PXP_PIX_FMT_YUV420P2) |
	    (pix_fmt == PXP_PIX_FMT_YVU422P) |
	    (pix_fmt == PXP_PIX_FMT_YUV422P)) {
		return true;
	} else {
		return false;
	}
}

static void pxp_set_ctrl(struct pxps *pxp)
{
	struct pxp_config_data *pxp_conf = &pxp->pxp_conf_state;
	struct pxp_proc_data *proc_data = &pxp_conf->proc_data;
	u32 ctrl;
	u32 fmt_ctrl;

	/* Configure S0 input format */
	switch (pxp_conf->s0_param.pixel_fmt) {
	case PXP_PIX_FMT_RGB24:
		fmt_ctrl = BV_PXP_CTRL_S0_FORMAT__RGB888;
		break;
	case PXP_PIX_FMT_RGB565:
		fmt_ctrl = BV_PXP_CTRL_S0_FORMAT__RGB565;
		break;
	case PXP_PIX_FMT_RGB555:
		fmt_ctrl = BV_PXP_CTRL_S0_FORMAT__RGB555;
		break;
	case PXP_PIX_FMT_YUV420P:
	case PXP_PIX_FMT_GREY:
		fmt_ctrl = BV_PXP_CTRL_S0_FORMAT__YUV420;
		break;
	case PXP_PIX_FMT_YUV422P:
		fmt_ctrl = BV_PXP_CTRL_S0_FORMAT__YUV422;
		break;
	default:
		fmt_ctrl = 0;
	}
	ctrl = BF_PXP_CTRL_S0_FORMAT(fmt_ctrl);

	/* Configure output format based on out_channel format */
	switch (pxp_conf->out_param.pixel_fmt) {
	case PXP_PIX_FMT_RGB24:
		fmt_ctrl = BV_PXP_CTRL_OUTBUF_FORMAT__RGB888;
		break;
	case PXP_PIX_FMT_RGB565:
		fmt_ctrl = BV_PXP_CTRL_OUTBUF_FORMAT__RGB565;
		break;
	case PXP_PIX_FMT_RGB555:
		fmt_ctrl = BV_PXP_CTRL_OUTBUF_FORMAT__RGB555;
		break;
	case PXP_PIX_FMT_YUV420P:
		fmt_ctrl = BV_PXP_CTRL_OUTBUF_FORMAT__YUV2P420;
		break;
	case PXP_PIX_FMT_YUV422P:
		fmt_ctrl = BV_PXP_CTRL_OUTBUF_FORMAT__YUV2P422;
		break;
	case PXP_PIX_FMT_GREY:
		fmt_ctrl = BV_PXP_CTRL_OUTBUF_FORMAT__MONOC8;
		break;
	default:
		fmt_ctrl = 0;
	}
	ctrl |= BF_PXP_CTRL_OUTBUF_FORMAT(fmt_ctrl);

	ctrl |= BM_PXP_CTRL_CROP;

	if (proc_data->scaling)
		ctrl |= BM_PXP_CTRL_SCALE;
	if (proc_data->vflip)
		ctrl |= BM_PXP_CTRL_VFLIP;
	if (proc_data->hflip)
		ctrl |= BM_PXP_CTRL_HFLIP;
	if (proc_data->rotate)
		ctrl |= BF_PXP_CTRL_ROTATE(proc_data->rotate / 90);

	__raw_writel(ctrl, pxp->base + HW_PXP_CTRL);
}

static int pxp_start(struct pxps *pxp)
{
	__raw_writel(BM_PXP_CTRL_IRQ_ENABLE, pxp->base + HW_PXP_CTRL_SET);
	__raw_writel(BM_PXP_CTRL_ENABLE, pxp->base + HW_PXP_CTRL_SET);

	return 0;
}

static void pxp_set_outbuf(struct pxps *pxp)
{
	struct pxp_config_data *pxp_conf = &pxp->pxp_conf_state;
	struct pxp_layer_param *out_params = &pxp_conf->out_param;

	__raw_writel(out_params->paddr, pxp->base + HW_PXP_OUTBUF);

	__raw_writel(BF_PXP_OUTSIZE_WIDTH(out_params->width) |
		     BF_PXP_OUTSIZE_HEIGHT(out_params->height),
		     pxp->base + HW_PXP_OUTSIZE);
}

static void pxp_set_s0colorkey(struct pxps *pxp)
{
	struct pxp_config_data *pxp_conf = &pxp->pxp_conf_state;
	struct pxp_layer_param *s0_params = &pxp_conf->s0_param;

	/* Low and high are set equal. V4L does not allow a chromakey range */
	if (s0_params->color_key == -1) {
		/* disable color key */
		__raw_writel(0xFFFFFF, pxp->base + HW_PXP_S0COLORKEYLOW);
		__raw_writel(0, pxp->base + HW_PXP_S0COLORKEYHIGH);
	} else {
		__raw_writel(s0_params->color_key,
			     pxp->base + HW_PXP_S0COLORKEYLOW);
		__raw_writel(s0_params->color_key,
			     pxp->base + HW_PXP_S0COLORKEYHIGH);
	}
}

static void pxp_set_olcolorkey(int layer_no, struct pxps *pxp)
{
	struct pxp_config_data *pxp_conf = &pxp->pxp_conf_state;
	struct pxp_layer_param *ol_params = &pxp_conf->ol_param[layer_no];

	/* Low and high are set equal. V4L does not allow a chromakey range */
	if (ol_params->color_key_enable != 0 && ol_params->color_key != -1) {
		__raw_writel(ol_params->color_key,
			     pxp->base + HW_PXP_OLCOLORKEYLOW);
		__raw_writel(ol_params->color_key,
			     pxp->base + HW_PXP_OLCOLORKEYHIGH);
	} else {
		/* disable color key */
		__raw_writel(0xFFFFFF, pxp->base + HW_PXP_OLCOLORKEYLOW);
		__raw_writel(0, pxp->base + HW_PXP_OLCOLORKEYHIGH);
	}
}

static void pxp_set_oln(int layer_no, struct pxps *pxp)
{
	struct pxp_config_data *pxp_conf = &pxp->pxp_conf_state;
	struct pxp_layer_param *olparams_data = &pxp_conf->ol_param[layer_no];
	dma_addr_t phys_addr = olparams_data->paddr;
	__raw_writel(phys_addr, pxp->base + HW_PXP_OLn(layer_no));

	/* Fixme */
	__raw_writel(BF_PXP_OLnSIZE_WIDTH(olparams_data->width >> 3) |
		     BF_PXP_OLnSIZE_HEIGHT(olparams_data->height >> 3),
		     pxp->base + HW_PXP_OLnSIZE(layer_no));
}

static void pxp_set_olparam(int layer_no, struct pxps *pxp)
{
	struct pxp_config_data *pxp_conf = &pxp->pxp_conf_state;
	struct pxp_layer_param *olparams_data = &pxp_conf->ol_param[layer_no];
	u32 olparam;

	olparam = BF_PXP_OLnPARAM_ALPHA(olparams_data->global_alpha);
	if (olparams_data->pixel_fmt == PXP_PIX_FMT_RGB24)
		olparam |=
		    BF_PXP_OLnPARAM_FORMAT(BV_PXP_OLnPARAM_FORMAT__RGB888);
	else
		olparam |=
		    BF_PXP_OLnPARAM_FORMAT(BV_PXP_OLnPARAM_FORMAT__RGB565);
	if (olparams_data->global_alpha_enable)
		olparam |=
		    BF_PXP_OLnPARAM_ALPHA_CNTL
		    (BV_PXP_OLnPARAM_ALPHA_CNTL__Override);
	if (olparams_data->color_key_enable)
		olparam |= BM_PXP_OLnPARAM_ENABLE_COLORKEY;
	if (olparams_data->combine_enable)
		olparam |= BM_PXP_OLnPARAM_ENABLE;
	__raw_writel(olparam, pxp->base + HW_PXP_OLnPARAM(layer_no));
}

static void pxp_set_s0param(struct pxps *pxp)
{
	struct pxp_config_data *pxp_conf = &pxp->pxp_conf_state;
	struct pxp_layer_param *s0params_data = &pxp_conf->s0_param;
	struct pxp_proc_data *proc_data = &pxp_conf->proc_data;
	u32 s0param;

	s0param = BF_PXP_S0PARAM_XBASE(proc_data->drect.left >> 3);
	s0param |= BF_PXP_S0PARAM_YBASE(proc_data->drect.top >> 3);
	s0param |= BF_PXP_S0PARAM_WIDTH(s0params_data->width >> 3);
	s0param |= BF_PXP_S0PARAM_HEIGHT(s0params_data->height >> 3);
	__raw_writel(s0param, pxp->base + HW_PXP_S0PARAM);
}

static void pxp_set_s0crop(struct pxps *pxp)
{
	u32 s0crop;
	struct pxp_proc_data *proc_data = &pxp->pxp_conf_state.proc_data;

	s0crop = BF_PXP_S0CROP_XBASE(proc_data->srect.left >> 3);
	s0crop |= BF_PXP_S0CROP_YBASE(proc_data->srect.top >> 3);
	s0crop |= BF_PXP_S0CROP_WIDTH(proc_data->drect.width >> 3);
	s0crop |= BF_PXP_S0CROP_HEIGHT(proc_data->drect.height >> 3);
	__raw_writel(s0crop, pxp->base + HW_PXP_S0CROP);
}

static int pxp_set_scaling(struct pxps *pxp)
{
	int ret = 0;
	u32 xscale, yscale, s0scale;
	struct pxp_proc_data *proc_data = &pxp->pxp_conf_state.proc_data;
	struct pxp_layer_param *s0params_data = &pxp->pxp_conf_state.s0_param;

	if ((s0params_data->pixel_fmt != PXP_PIX_FMT_YUV420P) &&
	    (s0params_data->pixel_fmt != PXP_PIX_FMT_YUV422P)) {
		proc_data->scaling = 0;
		ret = -EINVAL;
		goto out;
	}

	if ((proc_data->srect.width == proc_data->drect.width) &&
	    (proc_data->srect.height == proc_data->drect.height)) {
		proc_data->scaling = 0;
		__raw_writel(0x10001000, pxp->base + HW_PXP_S0SCALE);
		goto out;
	}

	proc_data->scaling = 1;
	xscale = proc_data->srect.width * 0x1000 / proc_data->drect.width;
	yscale = proc_data->srect.height * 0x1000 / proc_data->drect.height;
	if (xscale > PXP_DOWNSCALE_THRESHOLD)
		xscale = PXP_DOWNSCALE_THRESHOLD;
	if (yscale > PXP_DOWNSCALE_THRESHOLD)
		yscale = PXP_DOWNSCALE_THRESHOLD;
	s0scale = BF_PXP_S0SCALE_YSCALE(yscale) | BF_PXP_S0SCALE_XSCALE(xscale);
	__raw_writel(s0scale, pxp->base + HW_PXP_S0SCALE);

out:
	pxp_set_ctrl(pxp);

	return ret;
}

static void pxp_set_bg(struct pxps *pxp)
{
	__raw_writel(pxp->pxp_conf_state.proc_data.bgcolor,
		     pxp->base + HW_PXP_S0BACKGROUND);
}

static void pxp_set_lut(struct pxps *pxp)
{
	struct pxp_config_data *pxp_conf = &pxp->pxp_conf_state;
	int lut_op = pxp_conf->proc_data.lut_transform;
	u32 reg_val;
	int i;

	/* If LUT already configured as needed, return */
	if (pxp->lut_state == lut_op)
		return;

	if (lut_op == PXP_LUT_NONE) {
		__raw_writel(BM_PXP_LUT_CTRL_BYPASS,
			     pxp->base + HW_PXP_LUT_CTRL);
	} else if (((lut_op & PXP_LUT_INVERT) != 0)
		&& ((lut_op & PXP_LUT_BLACK_WHITE) != 0)) {
		/* Fill out LUT table with inverted monochromized values */

		/* Initialize LUT address to 0 and clear bypass bit */
		__raw_writel(0, pxp->base + HW_PXP_LUT_CTRL);

		/* LUT address pointer auto-increments after each data write */
		for (i = 0; i < 256; i++) {
			reg_val =
			    __raw_readl(pxp->base +
					HW_PXP_LUT_CTRL) & BM_PXP_LUT_CTRL_ADDR;
			reg_val = (reg_val < 0x80) ? 0x00 : 0xFF;
			reg_val = ~reg_val & BM_PXP_LUT_DATA;
			__raw_writel(reg_val, pxp->base + HW_PXP_LUT);
		}
	} else if (lut_op == PXP_LUT_INVERT) {
		/* Fill out LUT table with 8-bit inverted values */

		/* Initialize LUT address to 0 and clear bypass bit */
		__raw_writel(0, pxp->base + HW_PXP_LUT_CTRL);

		/* LUT address pointer auto-increments after each data write */
		for (i = 0; i < 256; i++) {
			reg_val =
			    __raw_readl(pxp->base +
					HW_PXP_LUT_CTRL) & BM_PXP_LUT_CTRL_ADDR;
			reg_val = ~reg_val & BM_PXP_LUT_DATA;
			__raw_writel(reg_val, pxp->base + HW_PXP_LUT);
		}
	} else if (lut_op == PXP_LUT_BLACK_WHITE) {
		/* Fill out LUT table with 8-bit monochromized values */

		/* Initialize LUT address to 0 and clear bypass bit */
		__raw_writel(0, pxp->base + HW_PXP_LUT_CTRL);

		/* LUT address pointer auto-increments after each data write */
		for (i = 0; i < 256; i++) {
			reg_val =
			    __raw_readl(pxp->base +
					HW_PXP_LUT_CTRL) & BM_PXP_LUT_CTRL_ADDR;
			reg_val = (reg_val < 0x80) ? 0xFF : 0x00;
			reg_val = ~reg_val & BM_PXP_LUT_DATA;
			__raw_writel(reg_val, pxp->base + HW_PXP_LUT);
		}
	}

	pxp->lut_state = lut_op;
}

static void pxp_set_csc(struct pxps *pxp)
{
	struct pxp_config_data *pxp_conf = &pxp->pxp_conf_state;
	struct pxp_layer_param *s0_params = &pxp_conf->s0_param;
	struct pxp_layer_param *ol_params = &pxp_conf->ol_param[0];
	struct pxp_layer_param *out_params = &pxp_conf->out_param;

	bool input_is_YUV = is_yuv(s0_params->pixel_fmt);
	bool output_is_YUV = is_yuv(out_params->pixel_fmt);

	if (input_is_YUV && output_is_YUV) {
		/*
		 * Input = YUV, Output = YUV
		 * No CSC unless we need to do combining
		 */
		if (ol_params->combine_enable) {
			/* Must convert to RGB for combining with RGB overlay */

			/* CSC1 - YUV->RGB */
			__raw_writel(0x04030000, pxp->base + HW_PXP_CSCCOEF0);
			__raw_writel(0x01230208, pxp->base + HW_PXP_CSCCOEF1);
			__raw_writel(0x076b079c, pxp->base + HW_PXP_CSCCOEF2);

			/* CSC2 - RGB->YUV */
			__raw_writel(0x4, pxp->base + HW_PXP_CSC2CTRL);
			__raw_writel(0x0096004D, pxp->base + HW_PXP_CSC2COEF0);
			__raw_writel(0x05DA001D, pxp->base + HW_PXP_CSC2COEF1);
			__raw_writel(0x007005B6, pxp->base + HW_PXP_CSC2COEF2);
			__raw_writel(0x057C009E, pxp->base + HW_PXP_CSC2COEF3);
			__raw_writel(0x000005E6, pxp->base + HW_PXP_CSC2COEF4);
			__raw_writel(0x00000000, pxp->base + HW_PXP_CSC2COEF5);
		} else {
			/* Input & Output both YUV, so bypass both CSCs */

			/* CSC1 - Bypass */
			__raw_writel(0x40000000, pxp->base + HW_PXP_CSCCOEF0);

			/* CSC2 - Bypass */
			__raw_writel(0x1, pxp->base + HW_PXP_CSC2CTRL);
		}
	} else if (input_is_YUV && !output_is_YUV) {
		/*
		 * Input = YUV, Output = RGB
		 * Use CSC1 to convert to RGB
		 */

		/* CSC1 - YUV->RGB */
		__raw_writel(0x84ab01f0, pxp->base + HW_PXP_CSCCOEF0);
		__raw_writel(0x01230204, pxp->base + HW_PXP_CSCCOEF1);
		__raw_writel(0x0730079c, pxp->base + HW_PXP_CSCCOEF2);

		/* CSC2 - Bypass */
		__raw_writel(0x1, pxp->base + HW_PXP_CSC2CTRL);
	} else if (!input_is_YUV && output_is_YUV) {
		/*
		 * Input = RGB, Output = YUV
		 * Use CSC2 to convert to YUV
		 */

		/* CSC1 - Bypass */
		__raw_writel(0x40000000, pxp->base + HW_PXP_CSCCOEF0);

		/* CSC2 - RGB->YUV */
		__raw_writel(0x4, pxp->base + HW_PXP_CSC2CTRL);
		__raw_writel(0x0096004D, pxp->base + HW_PXP_CSC2COEF0);
		__raw_writel(0x05DA001D, pxp->base + HW_PXP_CSC2COEF1);
		__raw_writel(0x007005B6, pxp->base + HW_PXP_CSC2COEF2);
		__raw_writel(0x057C009E, pxp->base + HW_PXP_CSC2COEF3);
		__raw_writel(0x000005E6, pxp->base + HW_PXP_CSC2COEF4);
		__raw_writel(0x00000000, pxp->base + HW_PXP_CSC2COEF5);
	} else {
		/*
		 * Input = RGB, Output = RGB
		 * Input & Output both RGB, so bypass both CSCs
		 */

		/* CSC1 - Bypass */
		__raw_writel(0x40000000, pxp->base + HW_PXP_CSCCOEF0);

		/* CSC2 - Bypass */
		__raw_writel(0x1, pxp->base + HW_PXP_CSC2CTRL);
	}

	/* YCrCb colorspace */
	/* Not sure when we use this...no YCrCb formats are defined for PxP */
	/*
	   __raw_writel(0x84ab01f0, HW_PXP_CSCCOEFF0_ADDR);
	   __raw_writel(0x01230204, HW_PXP_CSCCOEFF1_ADDR);
	   __raw_writel(0x0730079c, HW_PXP_CSCCOEFF2_ADDR);
	 */

}

static void pxp_set_s0buf(struct pxps *pxp)
{
	struct pxp_config_data *pxp_conf = &pxp->pxp_conf_state;
	struct pxp_layer_param *s0_params = &pxp_conf->s0_param;
	dma_addr_t Y, U, V;

	Y = s0_params->paddr;
	__raw_writel(Y, pxp->base + HW_PXP_S0BUF);
	if ((s0_params->pixel_fmt == PXP_PIX_FMT_YUV420P) ||
	    (s0_params->pixel_fmt == PXP_PIX_FMT_YVU420P) ||
	    (s0_params->pixel_fmt == PXP_PIX_FMT_GREY)) {
		/* Set to 1 if YUV format is 4:2:2 rather than 4:2:0 */
		int s = 2;
		U = Y + (s0_params->width * s0_params->height);
		V = U + ((s0_params->width * s0_params->height) >> s);
		__raw_writel(U, pxp->base + HW_PXP_S0UBUF);
		__raw_writel(V, pxp->base + HW_PXP_S0VBUF);
	}
}

/**
 * pxp_config() - configure PxP for a processing task
 * @pxps:	PXP context.
 * @pxp_chan:	PXP channel.
 * @return:	0 on success or negative error code on failure.
 */
static int pxp_config(struct pxps *pxp, struct pxp_channel *pxp_chan)
{
	struct pxp_config_data *pxp_conf_data = &pxp->pxp_conf_state;
	int ol_nr;
	int i;

	/* Configure PxP regs */
	pxp_set_ctrl(pxp);
	pxp_set_s0param(pxp);
	pxp_set_s0crop(pxp);
	pxp_set_scaling(pxp);
	ol_nr = pxp_conf_data->layer_nr - 2;
	while (ol_nr > 0) {
		i = pxp_conf_data->layer_nr - 2 - ol_nr;
		pxp_set_oln(i, pxp);
		pxp_set_olparam(i, pxp);
		/* only the color key in higher overlay will take effect. */
		pxp_set_olcolorkey(i, pxp);
		ol_nr--;
	}
	pxp_set_s0colorkey(pxp);
	pxp_set_csc(pxp);
	pxp_set_bg(pxp);
	pxp_set_lut(pxp);

	pxp_set_s0buf(pxp);
	pxp_set_outbuf(pxp);

	return 0;
}

static void pxp_clk_enable(struct pxps *pxp)
{
	mutex_lock(&pxp->clk_mutex);

	if (pxp->clk_stat == CLK_STAT_ON) {
		mutex_unlock(&pxp->clk_mutex);
		return;
	}

	clk_enable(pxp->clk);
	pxp->clk_stat = CLK_STAT_ON;

	mutex_unlock(&pxp->clk_mutex);
}

static void pxp_clk_disable(struct pxps *pxp)
{
	unsigned long flags;

	mutex_lock(&pxp->clk_mutex);

	if (pxp->clk_stat == CLK_STAT_OFF) {
		mutex_unlock(&pxp->clk_mutex);
		return;
	}

	spin_lock_irqsave(&pxp->lock, flags);
	if ((pxp->pxp_ongoing == 0) && list_empty(&head)) {
		spin_unlock_irqrestore(&pxp->lock, flags);
		clk_disable(pxp->clk);
		pxp->clk_stat = CLK_STAT_OFF;
	} else
		spin_unlock_irqrestore(&pxp->lock, flags);

	mutex_unlock(&pxp->clk_mutex);
}

static inline void clkoff_callback(struct work_struct *w)
{
	struct pxps *pxp = container_of(w, struct pxps, work);

	pxp_clk_disable(pxp);
}

static void pxp_clkoff_timer(unsigned long arg)
{
	struct pxps *pxp = (struct pxps *)arg;

	if ((pxp->pxp_ongoing == 0) && list_empty(&head))
		schedule_work(&pxp->work);
	else
		mod_timer(&pxp->clk_timer,
			  jiffies + msecs_to_jiffies(timeout_in_ms));
}

static struct pxp_tx_desc *pxpdma_first_active(struct pxp_channel *pxp_chan)
{
	return list_entry(pxp_chan->active_list.next, struct pxp_tx_desc, list);
}

static struct pxp_tx_desc *pxpdma_first_queued(struct pxp_channel *pxp_chan)
{
	return list_entry(pxp_chan->queue.next, struct pxp_tx_desc, list);
}

/* called with pxp_chan->lock held */
static void __pxpdma_dostart(struct pxp_channel *pxp_chan)
{
	struct pxp_dma *pxp_dma = to_pxp_dma(pxp_chan->dma_chan.device);
	struct pxps *pxp = to_pxp(pxp_dma);
	struct pxp_tx_desc *desc;
	struct pxp_tx_desc *child;
	int i = 0;

	/* so far we presume only one transaction on active_list */
	/* S0 */
	desc = pxpdma_first_active(pxp_chan);
	memcpy(&pxp->pxp_conf_state.s0_param,
	       &desc->layer_param.s0_param, sizeof(struct pxp_layer_param));
	memcpy(&pxp->pxp_conf_state.proc_data,
	       &desc->proc_data, sizeof(struct pxp_proc_data));

	/* Save PxP configuration */
	list_for_each_entry(child, &desc->tx_list, list) {
		if (i == 0) {	/* Output */
			memcpy(&pxp->pxp_conf_state.out_param,
			       &child->layer_param.out_param,
			       sizeof(struct pxp_layer_param));
		} else {	/* Overlay */
			memcpy(&pxp->pxp_conf_state.ol_param[i - 1],
			       &child->layer_param.ol_param,
			       sizeof(struct pxp_layer_param));
		}

		i++;
	}
	pr_debug("%s:%d S0 w/h %d/%d paddr %08x\n", __func__, __LINE__,
		 pxp->pxp_conf_state.s0_param.width,
		 pxp->pxp_conf_state.s0_param.height,
		 pxp->pxp_conf_state.s0_param.paddr);
	pr_debug("%s:%d OUT w/h %d/%d paddr %08x\n", __func__, __LINE__,
		 pxp->pxp_conf_state.out_param.width,
		 pxp->pxp_conf_state.out_param.height,
		 pxp->pxp_conf_state.out_param.paddr);
}

static void pxpdma_dostart_work(struct pxps *pxp)
{
	struct pxp_channel *pxp_chan = NULL;
	unsigned long flags, flags1;

	while (__raw_readl(pxp->base + HW_PXP_CTRL) & BM_PXP_CTRL_ENABLE)
		;

	spin_lock_irqsave(&pxp->lock, flags);
	if (list_empty(&head)) {
		pxp->pxp_ongoing = 0;
		spin_unlock_irqrestore(&pxp->lock, flags);
		return;
	}

	pxp_chan = list_entry(head.next, struct pxp_channel, list);

	spin_lock_irqsave(&pxp_chan->lock, flags1);
	if (!list_empty(&pxp_chan->active_list)) {
		struct pxp_tx_desc *desc;
		/* REVISIT */
		desc = pxpdma_first_active(pxp_chan);
		__pxpdma_dostart(pxp_chan);
	}
	spin_unlock_irqrestore(&pxp_chan->lock, flags1);

	/* Configure PxP */
	pxp_config(pxp, pxp_chan);

	pxp_start(pxp);

	spin_unlock_irqrestore(&pxp->lock, flags);
}

static void pxpdma_dequeue(struct pxp_channel *pxp_chan, struct list_head *list)
{
	struct pxp_tx_desc *desc = NULL;
	do {
		desc = pxpdma_first_queued(pxp_chan);
		list_move_tail(&desc->list, list);
	} while (!list_empty(&pxp_chan->queue));
}

static dma_cookie_t pxp_tx_submit(struct dma_async_tx_descriptor *tx)
{
	struct pxp_tx_desc *desc = to_tx_desc(tx);
	struct pxp_channel *pxp_chan = to_pxp_channel(tx->chan);
	dma_cookie_t cookie;
	unsigned long flags;

	dev_dbg(&pxp_chan->dma_chan.dev->device, "received TX\n");

	mutex_lock(&pxp_chan->chan_mutex);

	cookie = pxp_chan->dma_chan.cookie;

	if (++cookie < 0)
		cookie = 1;

	/* from dmaengine.h: "last cookie value returned to client" */
	pxp_chan->dma_chan.cookie = cookie;
	tx->cookie = cookie;

	/* pxp_chan->lock can be taken under ichan->lock, but not v.v. */
	spin_lock_irqsave(&pxp_chan->lock, flags);

	/* Here we add the tx descriptor to our PxP task queue. */
	list_add_tail(&desc->list, &pxp_chan->queue);

	spin_unlock_irqrestore(&pxp_chan->lock, flags);

	dev_dbg(&pxp_chan->dma_chan.dev->device, "done TX\n");

	mutex_unlock(&pxp_chan->chan_mutex);
	return cookie;
}

/* Called with pxp_chan->chan_mutex held */
static int pxp_desc_alloc(struct pxp_channel *pxp_chan, int n)
{
	struct pxp_tx_desc *desc = vmalloc(n * sizeof(struct pxp_tx_desc));

	if (!desc)
		return -ENOMEM;

	pxp_chan->n_tx_desc = n;
	pxp_chan->desc = desc;
	INIT_LIST_HEAD(&pxp_chan->active_list);
	INIT_LIST_HEAD(&pxp_chan->queue);
	INIT_LIST_HEAD(&pxp_chan->free_list);

	while (n--) {
		struct dma_async_tx_descriptor *txd = &desc->txd;

		memset(txd, 0, sizeof(*txd));
		INIT_LIST_HEAD(&desc->tx_list);
		dma_async_tx_descriptor_init(txd, &pxp_chan->dma_chan);
		txd->tx_submit = pxp_tx_submit;

		list_add(&desc->list, &pxp_chan->free_list);

		desc++;
	}

	return 0;
}

/**
 * pxp_init_channel() - initialize a PXP channel.
 * @pxp_dma:   PXP DMA context.
 * @pchan:  pointer to the channel object.
 * @return      0 on success or negative error code on failure.
 */
static int pxp_init_channel(struct pxp_dma *pxp_dma,
			    struct pxp_channel *pxp_chan)
{
	unsigned long flags;
	struct pxps *pxp = to_pxp(pxp_dma);
	int ret = 0, n_desc = 0;

	/*
	 * We are using _virtual_ channel here.
	 * Each channel contains all parameters of corresponding layers
	 * for one transaction; each layer is represented as one descriptor
	 * (i.e., pxp_tx_desc) here.
	 */

	spin_lock_irqsave(&pxp->lock, flags);

	/* max desc nr: S0+OL+OUT = 1+8+1 */
	n_desc = 16;

	spin_unlock_irqrestore(&pxp->lock, flags);

	if (n_desc && !pxp_chan->desc)
		ret = pxp_desc_alloc(pxp_chan, n_desc);

	return ret;
}

/**
 * pxp_uninit_channel() - uninitialize a PXP channel.
 * @pxp_dma:   PXP DMA context.
 * @pchan:  pointer to the channel object.
 * @return      0 on success or negative error code on failure.
 */
static int pxp_uninit_channel(struct pxp_dma *pxp_dma,
			      struct pxp_channel *pxp_chan)
{
	int ret = 0;

	if (pxp_chan->desc)
		vfree(pxp_chan->desc);

	pxp_chan->desc = NULL;

	return ret;
}

static irqreturn_t pxp_irq(int irq, void *dev_id)
{
	struct pxps *pxp = dev_id;
	struct pxp_channel *pxp_chan;
	struct pxp_tx_desc *desc;
	dma_async_tx_callback callback;
	void *callback_param;
	unsigned long flags;
	u32 hist_status;

	dump_pxp_reg(pxp);

	hist_status =
	    __raw_readl(pxp->base + HW_PXP_HIST_CTRL) & BM_PXP_HIST_CTRL_STATUS;

	__raw_writel(BM_PXP_STAT_IRQ, pxp->base + HW_PXP_STAT_CLR);

	spin_lock_irqsave(&pxp->lock, flags);

	if (list_empty(&head)) {
		pxp->pxp_ongoing = 0;
		spin_unlock_irqrestore(&pxp->lock, flags);
		return IRQ_NONE;
	}

	pxp_chan = list_entry(head.next, struct pxp_channel, list);
	list_del_init(&pxp_chan->list);

	if (list_empty(&pxp_chan->active_list)) {
		pr_debug("PXP_IRQ pxp_chan->active_list empty. chan_id %d\n",
			 pxp_chan->dma_chan.chan_id);
		pxp->pxp_ongoing = 0;
		spin_unlock_irqrestore(&pxp->lock, flags);
		return IRQ_NONE;
	}

	/* Get descriptor and call callback */
	desc = pxpdma_first_active(pxp_chan);

	pxp_chan->completed = desc->txd.cookie;

	callback = desc->txd.callback;
	callback_param = desc->txd.callback_param;

	/* Send histogram status back to caller */
	desc->hist_status = hist_status;

	if ((desc->txd.flags & DMA_PREP_INTERRUPT) && callback)
		callback(callback_param);

	pxp_chan->status = PXP_CHANNEL_INITIALIZED;

	list_splice_init(&desc->tx_list, &pxp_chan->free_list);
	list_move(&desc->list, &pxp_chan->free_list);

	wake_up(&pxp->done);
	pxp->pxp_ongoing = 0;
	mod_timer(&pxp->clk_timer, jiffies + msecs_to_jiffies(timeout_in_ms));

	spin_unlock_irqrestore(&pxp->lock, flags);

	return IRQ_HANDLED;
}

/* called with pxp_chan->lock held */
static struct pxp_tx_desc *pxpdma_desc_get(struct pxp_channel *pxp_chan)
{
	struct pxp_tx_desc *desc, *_desc;
	struct pxp_tx_desc *ret = NULL;

	list_for_each_entry_safe(desc, _desc, &pxp_chan->free_list, list) {
		list_del_init(&desc->list);
		ret = desc;
		break;
	}

	return ret;
}

/* called with pxp_chan->lock held */
static void pxpdma_desc_put(struct pxp_channel *pxp_chan,
			    struct pxp_tx_desc *desc)
{
	if (desc) {
		struct device *dev = &pxp_chan->dma_chan.dev->device;
		struct pxp_tx_desc *child;
		unsigned long flags;

		list_for_each_entry(child, &desc->tx_list, list)
		    dev_info(dev, "moving child desc %p to freelist\n", child);
		list_splice_init(&desc->tx_list, &pxp_chan->free_list);
		dev_info(dev, "moving desc %p to freelist\n", desc);
		list_add(&desc->list, &pxp_chan->free_list);
	}
}

/* Allocate and initialise a transfer descriptor. */
static struct dma_async_tx_descriptor *pxp_prep_slave_sg(struct dma_chan *chan,
							 struct scatterlist
							 *sgl,
							 unsigned int sg_len,
							 enum dma_data_direction
							 direction,
							 unsigned long tx_flags)
{
	struct pxp_channel *pxp_chan = to_pxp_channel(chan);
	struct pxp_dma *pxp_dma = to_pxp_dma(chan->device);
	struct pxps *pxp = to_pxp(pxp_dma);
	struct pxp_tx_desc *desc = NULL;
	struct pxp_tx_desc *first = NULL, *prev = NULL;
	struct scatterlist *sg;
	unsigned long flags;
	dma_addr_t phys_addr;
	int i;

	if (direction != DMA_FROM_DEVICE && direction != DMA_TO_DEVICE) {
		dev_err(chan->device->dev, "Invalid DMA direction %d!\n",
			direction);
		return NULL;
	}

	if (unlikely(sg_len < 2))
		return NULL;

	spin_lock_irqsave(&pxp_chan->lock, flags);
	for_each_sg(sgl, sg, sg_len, i) {
		desc = pxpdma_desc_get(pxp_chan);
		if (!desc) {
			pxpdma_desc_put(pxp_chan, first);
			dev_err(chan->device->dev, "Can't get DMA desc.\n");
			spin_unlock_irqrestore(&pxp_chan->lock, flags);
			return NULL;
		}

		phys_addr = sg_dma_address(sg);

		if (!first) {
			first = desc;

			desc->layer_param.s0_param.paddr = phys_addr;
		} else {
			list_add_tail(&desc->list, &first->tx_list);
			prev->next = desc;
			desc->next = NULL;

			if (i == 1)
				desc->layer_param.out_param.paddr = phys_addr;
			else
				desc->layer_param.ol_param.paddr = phys_addr;
		}

		prev = desc;
	}
	spin_unlock_irqrestore(&pxp_chan->lock, flags);

	pxp->pxp_conf_state.layer_nr = sg_len;
	first->txd.flags = tx_flags;
	first->len = sg_len;
	pr_debug("%s:%d first %p, first->len %d, flags %08x\n",
		 __func__, __LINE__, first, first->len, first->txd.flags);

	return &first->txd;
}

static void pxp_issue_pending(struct dma_chan *chan)
{
	struct pxp_channel *pxp_chan = to_pxp_channel(chan);
	struct pxp_dma *pxp_dma = to_pxp_dma(chan->device);
	struct pxps *pxp = to_pxp(pxp_dma);
	unsigned long flags0, flags;

	spin_lock_irqsave(&pxp->lock, flags0);
	spin_lock_irqsave(&pxp_chan->lock, flags);

	if (!list_empty(&pxp_chan->queue)) {
		pxpdma_dequeue(pxp_chan, &pxp_chan->active_list);
		pxp_chan->status = PXP_CHANNEL_READY;
		list_add_tail(&pxp_chan->list, &head);
	} else {
		spin_unlock_irqrestore(&pxp_chan->lock, flags);
		spin_unlock_irqrestore(&pxp->lock, flags0);
		return;
	}
	spin_unlock_irqrestore(&pxp_chan->lock, flags);
	spin_unlock_irqrestore(&pxp->lock, flags0);

	pxp_clk_enable(pxp);
	if (!wait_event_interruptible_timeout(pxp->done, PXP_WAITCON, 2 * HZ) ||
		signal_pending(current)) {
		pxp_clk_disable(pxp);
		return;
	}

	spin_lock_irqsave(&pxp->lock, flags);
	pxp->pxp_ongoing = 1;
	spin_unlock_irqrestore(&pxp->lock, flags);
	pxpdma_dostart_work(pxp);
}

static void __pxp_terminate_all(struct dma_chan *chan)
{
	struct pxp_channel *pxp_chan = to_pxp_channel(chan);
	unsigned long flags;

	/* pchan->queue is modified in ISR, have to spinlock */
	spin_lock_irqsave(&pxp_chan->lock, flags);
	list_splice_init(&pxp_chan->queue, &pxp_chan->free_list);
	list_splice_init(&pxp_chan->active_list, &pxp_chan->free_list);

	spin_unlock_irqrestore(&pxp_chan->lock, flags);

	pxp_chan->status = PXP_CHANNEL_INITIALIZED;
}

static int pxp_control(struct dma_chan *chan, enum dma_ctrl_cmd cmd,
			unsigned long arg)
{
	struct pxp_channel *pxp_chan = to_pxp_channel(chan);

	/* Only supports DMA_TERMINATE_ALL */
	if (cmd != DMA_TERMINATE_ALL)
		return -ENXIO;

	mutex_lock(&pxp_chan->chan_mutex);
	__pxp_terminate_all(chan);
	mutex_unlock(&pxp_chan->chan_mutex);

	return 0;
}

static int pxp_alloc_chan_resources(struct dma_chan *chan)
{
	struct pxp_channel *pxp_chan = to_pxp_channel(chan);
	struct pxp_dma *pxp_dma = to_pxp_dma(chan->device);
	int ret;

	/* dmaengine.c now guarantees to only offer free channels */
	BUG_ON(chan->client_count > 1);
	WARN_ON(pxp_chan->status != PXP_CHANNEL_FREE);

	chan->cookie = 1;
	pxp_chan->completed = -ENXIO;

	pr_debug("%s dma_chan.chan_id %d\n", __func__, chan->chan_id);
	ret = pxp_init_channel(pxp_dma, pxp_chan);
	if (ret < 0)
		goto err_chan;

	pxp_chan->status = PXP_CHANNEL_INITIALIZED;

	dev_dbg(&chan->dev->device, "Found channel 0x%x, irq %d\n",
		chan->chan_id, pxp_chan->eof_irq);

	return ret;

err_chan:
	return ret;
}

static void pxp_free_chan_resources(struct dma_chan *chan)
{
	struct pxp_channel *pxp_chan = to_pxp_channel(chan);
	struct pxp_dma *pxp_dma = to_pxp_dma(chan->device);

	mutex_lock(&pxp_chan->chan_mutex);

	__pxp_terminate_all(chan);

	pxp_chan->status = PXP_CHANNEL_FREE;

	pxp_uninit_channel(pxp_dma, pxp_chan);

	mutex_unlock(&pxp_chan->chan_mutex);
}

static enum dma_status pxp_tx_status(struct dma_chan *chan,
				     dma_cookie_t cookie,
				     struct dma_tx_state *txstate)
{
	struct pxp_channel *pxp_chan = to_pxp_channel(chan);

	if (cookie != chan->cookie)
		return DMA_ERROR;

	if (txstate) {
		txstate->last = pxp_chan->completed;
		txstate->used = chan->cookie;
		txstate->residue = 0;
	}
	return DMA_SUCCESS;
}

static int pxp_hw_init(struct pxps *pxp)
{
	struct pxp_config_data *pxp_conf = &pxp->pxp_conf_state;
	struct pxp_proc_data *proc_data = &pxp_conf->proc_data;
	u32 reg_val;
	int i;

	/* Pull PxP out of reset */
	__raw_writel(0, pxp->base + HW_PXP_CTRL);

	/* Config defaults */

	/* Initialize non-channel-specific PxP parameters */
	proc_data->drect.left = proc_data->srect.left = 0;
	proc_data->drect.top = proc_data->srect.top = 0;
	proc_data->drect.width = proc_data->srect.width = 0;
	proc_data->drect.height = proc_data->srect.height = 0;
	proc_data->scaling = 0;
	proc_data->hflip = 0;
	proc_data->vflip = 0;
	proc_data->rotate = 0;
	proc_data->bgcolor = 0;

	/* Initialize S0 channel parameters */
	pxp_conf->s0_param.pixel_fmt = pxp_s0_formats[0];
	pxp_conf->s0_param.width = 0;
	pxp_conf->s0_param.height = 0;
	pxp_conf->s0_param.color_key = -1;
	pxp_conf->s0_param.color_key_enable = false;

	/* Initialize OL channel parameters */
	for (i = 0; i < 8; i++) {
		pxp_conf->ol_param[i].combine_enable = false;
		pxp_conf->ol_param[i].width = 0;
		pxp_conf->ol_param[i].height = 0;
		pxp_conf->ol_param[i].pixel_fmt = PXP_PIX_FMT_RGB565;
		pxp_conf->ol_param[i].color_key_enable = false;
		pxp_conf->ol_param[i].color_key = -1;
		pxp_conf->ol_param[i].global_alpha_enable = false;
		pxp_conf->ol_param[i].global_alpha = 0;
		pxp_conf->ol_param[i].local_alpha_enable = false;
	}

	/* Initialize Output channel parameters */
	pxp_conf->out_param.width = 0;
	pxp_conf->out_param.height = 0;
	pxp_conf->out_param.pixel_fmt = PXP_PIX_FMT_RGB565;

	proc_data->overlay_state = 0;

	/* Write default h/w config */
	pxp_set_ctrl(pxp);
	pxp_set_s0param(pxp);
	pxp_set_s0crop(pxp);
	for (i = 0; i < 8; i++) {
		pxp_set_oln(i, pxp);
		pxp_set_olparam(i, pxp);
		pxp_set_olcolorkey(i, pxp);
	}
	pxp_set_s0colorkey(pxp);
	pxp_set_csc(pxp);
	pxp_set_bg(pxp);
	pxp_set_lut(pxp);

	/* One-time histogram configuration */
	reg_val =
	    BF_PXP_HIST_CTRL_PANEL_MODE(BV_PXP_HIST_CTRL_PANEL_MODE__GRAY16);
	__raw_writel(reg_val, pxp->base + HW_PXP_HIST_CTRL);

	reg_val = BF_PXP_HIST2_PARAM_VALUE0(0x00) |
	    BF_PXP_HIST2_PARAM_VALUE1(0x00F);
	__raw_writel(reg_val, pxp->base + HW_PXP_HIST2_PARAM);

	reg_val = BF_PXP_HIST4_PARAM_VALUE0(0x00) |
	    BF_PXP_HIST4_PARAM_VALUE1(0x05) |
	    BF_PXP_HIST4_PARAM_VALUE2(0x0A) | BF_PXP_HIST4_PARAM_VALUE3(0x0F);
	__raw_writel(reg_val, pxp->base + HW_PXP_HIST4_PARAM);

	reg_val = BF_PXP_HIST8_PARAM0_VALUE0(0x00) |
	    BF_PXP_HIST8_PARAM0_VALUE1(0x02) |
	    BF_PXP_HIST8_PARAM0_VALUE2(0x04) | BF_PXP_HIST8_PARAM0_VALUE3(0x06);
	__raw_writel(reg_val, pxp->base + HW_PXP_HIST8_PARAM0);
	reg_val = BF_PXP_HIST8_PARAM1_VALUE4(0x09) |
	    BF_PXP_HIST8_PARAM1_VALUE5(0x0B) |
	    BF_PXP_HIST8_PARAM1_VALUE6(0x0D) | BF_PXP_HIST8_PARAM1_VALUE7(0x0F);
	__raw_writel(reg_val, pxp->base + HW_PXP_HIST8_PARAM1);

	reg_val = BF_PXP_HIST16_PARAM0_VALUE0(0x00) |
	    BF_PXP_HIST16_PARAM0_VALUE1(0x01) |
	    BF_PXP_HIST16_PARAM0_VALUE2(0x02) |
	    BF_PXP_HIST16_PARAM0_VALUE3(0x03);
	__raw_writel(reg_val, pxp->base + HW_PXP_HIST16_PARAM0);
	reg_val = BF_PXP_HIST16_PARAM1_VALUE4(0x04) |
	    BF_PXP_HIST16_PARAM1_VALUE5(0x05) |
	    BF_PXP_HIST16_PARAM1_VALUE6(0x06) |
	    BF_PXP_HIST16_PARAM1_VALUE7(0x07);
	__raw_writel(reg_val, pxp->base + HW_PXP_HIST16_PARAM1);
	reg_val = BF_PXP_HIST16_PARAM2_VALUE8(0x08) |
	    BF_PXP_HIST16_PARAM2_VALUE9(0x09) |
	    BF_PXP_HIST16_PARAM2_VALUE10(0x0A) |
	    BF_PXP_HIST16_PARAM2_VALUE11(0x0B);
	__raw_writel(reg_val, pxp->base + HW_PXP_HIST16_PARAM2);
	reg_val = BF_PXP_HIST16_PARAM3_VALUE12(0x0C) |
	    BF_PXP_HIST16_PARAM3_VALUE13(0x0D) |
	    BF_PXP_HIST16_PARAM3_VALUE14(0x0E) |
	    BF_PXP_HIST16_PARAM3_VALUE15(0x0F);
	__raw_writel(reg_val, pxp->base + HW_PXP_HIST16_PARAM3);

	return 0;
}

static int pxp_dma_init(struct pxps *pxp)
{
	struct pxp_dma *pxp_dma = &pxp->pxp_dma;
	struct dma_device *dma = &pxp_dma->dma;
	int i;

	dma_cap_set(DMA_SLAVE, dma->cap_mask);
	dma_cap_set(DMA_PRIVATE, dma->cap_mask);

	/* Compulsory common fields */
	dma->dev = pxp->dev;
	dma->device_alloc_chan_resources = pxp_alloc_chan_resources;
	dma->device_free_chan_resources = pxp_free_chan_resources;
	dma->device_tx_status = pxp_tx_status;
	dma->device_issue_pending = pxp_issue_pending;

	/* Compulsory for DMA_SLAVE fields */
	dma->device_prep_slave_sg = pxp_prep_slave_sg;
	dma->device_control = pxp_control;

	/* Initialize PxP Channels */
	INIT_LIST_HEAD(&dma->channels);
	for (i = 0; i < NR_PXP_VIRT_CHANNEL; i++) {
		struct pxp_channel *pxp_chan = pxp->channel + i;
		struct dma_chan *dma_chan = &pxp_chan->dma_chan;

		spin_lock_init(&pxp_chan->lock);
		mutex_init(&pxp_chan->chan_mutex);

		/* Only one EOF IRQ for PxP, shared by all channels */
		pxp_chan->eof_irq = pxp->irq;
		pxp_chan->status = PXP_CHANNEL_FREE;
		pxp_chan->completed = -ENXIO;
		snprintf(pxp_chan->eof_name, sizeof(pxp_chan->eof_name),
			 "PXP EOF %d", i);

		dma_chan->device = &pxp_dma->dma;
		dma_chan->cookie = 1;
		dma_chan->chan_id = i;
		list_add_tail(&dma_chan->device_node, &dma->channels);
	}

	return dma_async_device_register(&pxp_dma->dma);
}

static ssize_t clk_off_timeout_show(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", timeout_in_ms);
}

static ssize_t clk_off_timeout_store(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t count)
{
	int val;
	if (sscanf(buf, "%d", &val) > 0) {
		timeout_in_ms = val;
		return count;
	}
	return -EINVAL;
}

static DEVICE_ATTR(clk_off_timeout, 0644, clk_off_timeout_show,
		   clk_off_timeout_store);

static int pxp_probe(struct platform_device *pdev)
{
	struct pxps *pxp;
	struct resource *res;
	int irq;
	int err = 0;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	irq = platform_get_irq(pdev, 0);
	if (!res || irq < 0) {
		err = -ENODEV;
		goto exit;
	}

	pxp = kzalloc(sizeof(*pxp), GFP_KERNEL);
	if (!pxp) {
		dev_err(&pdev->dev, "failed to allocate control object\n");
		err = -ENOMEM;
		goto exit;
	}

	pxp->dev = &pdev->dev;

	platform_set_drvdata(pdev, pxp);
	pxp->irq = irq;

	pxp->pxp_ongoing = 0;
	pxp->lut_state = 0;

	spin_lock_init(&pxp->lock);
	mutex_init(&pxp->clk_mutex);

	if (!request_mem_region(res->start, resource_size(res), "pxp-mem")) {
		err = -EBUSY;
		goto freepxp;
	}

	pxp->base = ioremap(res->start, SZ_4K);
	pxp->pdev = pdev;

	pxp->clk = clk_get(NULL, "pxp_axi");
	clk_enable(pxp->clk);

	err = pxp_hw_init(pxp);
	if (err) {
		dev_err(&pdev->dev, "failed to initialize hardware\n");
		goto release;
	}
	clk_disable(pxp->clk);

	err = request_irq(pxp->irq, pxp_irq, 0, "pxp-irq", pxp);
	if (err)
		goto release;
	/* Initialize DMA engine */
	err = pxp_dma_init(pxp);
	if (err < 0)
		goto err_dma_init;

	if (device_create_file(&pdev->dev, &dev_attr_clk_off_timeout)) {
		dev_err(&pdev->dev,
			"Unable to create file from clk_off_timeout\n");
		goto err_dma_init;
	}

	INIT_WORK(&pxp->work, clkoff_callback);
	init_waitqueue_head(&pxp->done);
	init_timer(&pxp->clk_timer);
	pxp->clk_timer.function = pxp_clkoff_timer;
	pxp->clk_timer.data = (unsigned long)pxp;
exit:
	return err;
err_dma_init:
	free_irq(pxp->irq, pxp);
release:
	release_mem_region(res->start, resource_size(res));
freepxp:
	kfree(pxp);
	dev_err(&pdev->dev, "Exiting (unsuccessfully) pxp_probe function\n");
	return err;
}

static int __devexit pxp_remove(struct platform_device *pdev)
{
	struct pxps *pxp = platform_get_drvdata(pdev);

	cancel_work_sync(&pxp->work);
	del_timer_sync(&pxp->clk_timer);
	free_irq(pxp->irq, pxp);
	clk_disable(pxp->clk);
	clk_put(pxp->clk);
	iounmap(pxp->base);
	device_remove_file(&pdev->dev, &dev_attr_clk_off_timeout);

	kfree(pxp);

	return 0;
}

#ifdef CONFIG_PM
static int pxp_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct pxps *pxp = platform_get_drvdata(pdev);

	pxp_clk_enable(pxp);
	while (__raw_readl(pxp->base + HW_PXP_CTRL) & BM_PXP_CTRL_ENABLE)
		;

	__raw_writel(BM_PXP_CTRL_SFTRST, pxp->base + HW_PXP_CTRL);
	pxp_clk_disable(pxp);

	return 0;
}

static int pxp_resume(struct platform_device *pdev)
{
	struct pxps *pxp = platform_get_drvdata(pdev);

	pxp_clk_enable(pxp);
	/* Pull PxP out of reset */
	__raw_writel(0, pxp->base + HW_PXP_CTRL);
	pxp_clk_disable(pxp);

	return 0;
}
#else
#define	pxp_suspend	NULL
#define	pxp_resume	NULL
#endif

static struct platform_driver pxp_driver = {
	.driver = {
		   .name = "mxc-pxp",
		   },
	.probe = pxp_probe,
	.remove = __exit_p(pxp_remove),
	.suspend = pxp_suspend,
	.resume = pxp_resume,
};

static int __init pxp_init(void)
{
	return platform_driver_register(&pxp_driver);
}

subsys_initcall(pxp_init);

static void __exit pxp_exit(void)
{
	platform_driver_unregister(&pxp_driver);
}

module_exit(pxp_exit);

MODULE_DESCRIPTION("i.MX PxP driver");
MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_LICENSE("GPL");
