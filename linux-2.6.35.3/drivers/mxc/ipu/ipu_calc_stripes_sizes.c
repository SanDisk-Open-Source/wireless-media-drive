/*
 * Copyright 2009-2010 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

/*
 * @file ipu_calc_stripes_sizes.c
 *
 * @brief IPU IC functions
 *
 * @ingroup IPU
 */

#include <linux/module.h>
#include <linux/ipu.h>
#include <asm/div64.h>

#define BPP_32 0
#define BPP_16 3
#define BPP_8 5
#define BPP_24 1
#define BPP_12 4
#define BPP_18 2

static u64 _do_div(u64 a, u32 b)
{
	u64 div;
	div = a;
	do_div(div, b);
	return div;
}

static u32 truncate(u32 up, /* 0: down; else: up */
					u64 a, /* must be non-negative */
					u32 b)
{
	u32 d;
	u64 div;
	div = _do_div(a, b);
	d = b * (div >> 32);
	if (up && (a > (((u64)d) << 32)))
		return d+b;
	else
		return d;
}

static unsigned int f_calc(unsigned int pfs, unsigned int bpp, unsigned int *write)
{/* return input_f */
	unsigned int f_calculated = 0;
	switch (pfs) {
	case IPU_PIX_FMT_YVU422P:
	case IPU_PIX_FMT_YUV422P:
	case IPU_PIX_FMT_YUV420P2:
	case IPU_PIX_FMT_YUV420P:
		f_calculated = 16;
		break;

	case IPU_PIX_FMT_NV12:
		f_calculated = 8;
		break;

	default:
		f_calculated = 0;
		break;

	}
	if (!f_calculated) {
		switch (bpp) {
		case BPP_32:
			f_calculated = 2;
			break;

		case BPP_16:
			f_calculated = 4;
			break;

		case BPP_8:
		case BPP_24:
			f_calculated = 8;
			break;

		case BPP_12:
			f_calculated = 16;
			break;

		case BPP_18:
			f_calculated = 32;
			break;

		default:
			f_calculated = 0;
			break;
			}
		}
	return f_calculated;
}


static unsigned int m_calc(unsigned int pfs)
{
	unsigned int m_calculated = 0;
	switch (pfs) {
	case IPU_PIX_FMT_YUV420P2:
	case IPU_PIX_FMT_YUV420P:
	case IPU_PIX_FMT_YVU422P:
	case IPU_PIX_FMT_YUV422P:
	case IPU_PIX_FMT_YVU420P:
	case IPU_PIX_FMT_NV12:
		m_calculated = 8;
		break;

	case IPU_PIX_FMT_YUYV:
	case IPU_PIX_FMT_UYVY:
		m_calculated = 2;
		break;

	default:
		m_calculated = 1;
		break;

	}
	return m_calculated;
}


/* Stripe parameters calculator */
/**************************************************************************
Notes:
MSW = the maximal width allowed for a stripe
	i.MX31: 720, i.MX35: 800, i.MX37/51/53: 1024
cirr = the maximal inverse resizing ratio for which overlap in the input
	is requested; typically cirr~2
equal_stripes:
	0: each stripe is allowed to have independent parameters
		for maximal image quality
	1: the stripes are requested to have identical parameters
	(except the base address), for maximal performance
If performance is the top priority (above image quality)
	Avoid overlap, by setting CIRR = 0
		This will also force effectively identical_stripes = 1
	Choose IF & OF that corresponds to the same IOX/SX for both stripes
	Choose IFW & OFW such that
	IFW/IM, IFW/IF, OFW/OM, OFW/OF are even integers
	The function returns an error status:
	0: no error
	1: invalid input parameters -> aborted without result
		Valid parameters should satisfy the following conditions
		IFW <= OFW, otherwise downsizing is required
					 - which is not supported yet
		4 <= IFW,OFW, so some interpolation may be needed even without overlap
		IM, OM, IF, OF should not vanish
		2*IF <= IFW
		so the frame can be split to two equal stripes, even without overlap
		2*(OF+IF/irr_opt) <= OFW
		so a valid positive INW exists even for equal stripes
		OF <= MSW, otherwise, the left stripe cannot be sufficiently large
		MSW < OFW, so splitting to stripes is required
		OFW <= 2*MSW, so two stripes are sufficient
		(this also implies that 2<=MSW)
	2: OF is not a multiple of OM - not fully-supported yet
	Output is produced but OW is not guaranited to be a multiple of OM
	4: OFW reduced to be a multiple of OM
	8: CIRR > 1: truncated to 1
	Overlap is not supported (and not needed) y for upsizing)
**************************************************************************/
int ipu_calc_stripes_sizes(const unsigned int input_frame_width,
			   /* input frame width;>1 */
			   unsigned int output_frame_width, /* output frame width; >1 */
			   const unsigned int maximal_stripe_width,
			   /* the maximal width allowed for a stripe */
			   const unsigned long long cirr, /* see above */
			   const unsigned int equal_stripes, /* see above */
			   u32 input_pixelformat,/* pixel format after of read channel*/
			   u32 output_pixelformat,/* pixel format after of write channel*/
			   struct stripe_param *left,
			   struct stripe_param *right)
{
	const unsigned int irr_frac_bits = 13;
	const unsigned long irr_steps = 1 << irr_frac_bits;
	const u64 dirr = ((u64)1) << (32 - 2);
	/* The maximum relative difference allowed between the irrs */
	const u64 cr = ((u64)4) << 32;
	/* The importance ratio between the two terms in the cost function below */

	unsigned int status;
	unsigned int temp;
	unsigned int onw_min;
	unsigned int inw, onw, inw_best = 0;
	/* number of pixels in the left stripe NOT hidden by the right stripe */
	u64 irr_opt; /* the optimal inverse resizing ratio */
	u64 rr_opt; /* the optimal resizing ratio = 1/irr_opt*/
	u64 dinw; /* the misalignment between the stripes */
	/* (measured in units of input columns) */
	u64 difwl, difwr;
	/* The number of input columns not reflected in the output */
	/* the resizing ratio used for the right stripe is */
	/*   left->irr and right->irr respectively */
	u64 cost, cost_min;
	u64 div; /* result of division */

	unsigned int input_m, input_f, output_m, output_f; /* parameters for upsizing by stripes */

	status = 0;

	/* M, F calculations */
	/* read back pfs from params */

	input_f = f_calc(input_pixelformat, 0, NULL);
	input_m = 16;
	/* BPP should be used in the out_F calc */
	/* Temporarily not used */
	/* out_F = F_calc(idmac->pfs, idmac->bpp, NULL); */

	output_f = 16;
	output_m = m_calc(output_pixelformat);


	if ((output_frame_width < input_frame_width) || (input_frame_width < 4)
	    || (output_frame_width < 4))
		return 1;

	irr_opt = _do_div((((u64)(input_frame_width - 1)) << 32),
			  (output_frame_width - 1));
	rr_opt = _do_div((((u64)(output_frame_width - 1)) << 32),
			 (input_frame_width - 1));

	if ((input_m == 0) || (output_m == 0) || (input_f == 0) || (output_f == 0)
	    || (input_frame_width < (2 * input_f))
	    || ((((u64)output_frame_width) << 32) <
		(2 * ((((u64)output_f) << 32) + (input_f * rr_opt))))
	    || (maximal_stripe_width < output_f)
	    || (output_frame_width <= maximal_stripe_width)
	    || ((2 * maximal_stripe_width) < output_frame_width))
		return 1;

	if (output_f % output_m)
		status += 2;

	temp = truncate(0, (((u64)output_frame_width) << 32), output_m);
	if (temp < output_frame_width) {
		output_frame_width = temp;
		status += 4;
	}

	if (equal_stripes) {
		if ((irr_opt > cirr) /* overlap in the input is not requested */
		    && ((input_frame_width % (input_m << 1)) == 0)
		    && ((input_frame_width % (input_f << 1)) == 0)
		    && ((output_frame_width % (output_m << 1)) == 0)
		    && ((output_frame_width % (output_f << 1)) == 0)) {
			/* without overlap */
			left->input_width = right->input_width = right->input_column =
				input_frame_width >> 1;
			left->output_width = right->output_width = right->output_column =
				output_frame_width >> 1;
			left->input_column = right->input_column = 0;
			div = _do_div(((((u64)irr_steps) << 32) *
				       (right->input_width - 1)), (right->output_width - 1));
			left->irr = right->irr = truncate(0, div, 1);
		} else { /* with overlap */
			onw = truncate(0, (((u64)output_frame_width) << 32) >> 1,
				       output_f);
			inw = truncate(0, onw * irr_opt, input_f);
			/* this is the maximal inw which allows the same resizing ratio */
			/* in both stripes */
			onw = truncate(1, (inw * rr_opt), output_f);
			div = _do_div((((u64)(irr_steps * inw)) <<
				       32), onw);
			left->irr = right->irr = truncate(0, div, 1);
			left->output_width = right->output_width =
				output_frame_width - onw;
			/* These are valid assignments for output_width, */
			/* assuming output_f is a multiple of output_m */
			div = (((u64)(left->output_width-1) * (left->irr)) << 32);
			div = (((u64)1) << 32) + _do_div(div, irr_steps);

			left->input_width = right->input_width = truncate(1, div, input_m);

			div = _do_div((((u64)((right->output_width - 1) * right->irr)) <<
				       32), irr_steps);
			difwr = (((u64)(input_frame_width - 1 - inw)) << 32) - div;
			div = _do_div((difwr + (((u64)input_f) << 32)), 2);
			left->input_column = truncate(0, div, input_f);


			/* This splits the truncated input columns evenly */
			/*    between the left and right margins */
			right->input_column = left->input_column + inw;
			left->output_column = 0;
			right->output_column = onw;
		}
	} else { /* independent stripes */
		onw_min = output_frame_width - maximal_stripe_width;
		/* onw is a multiple of output_f, in the range */
		/* [max(output_f,output_frame_width-maximal_stripe_width),*/
		/*min(output_frame_width-2,maximal_stripe_width)] */
		/* definitely beyond the cost of any valid setting */
		cost_min = (((u64)input_frame_width) << 32) + cr;
		onw = truncate(0, ((u64)maximal_stripe_width), output_f);
		if (output_frame_width - onw == 1)
			onw -= output_f; /*  => onw and output_frame_width-1-onw are positive */
		inw = truncate(0, onw * irr_opt, input_f);
		/* this is the maximal inw which allows the same resizing ratio */
		/* in both stripes */
		onw = truncate(1, inw * rr_opt, output_f);
		do {
			div = _do_div((((u64)(irr_steps * inw)) << 32), onw);
			left->irr = truncate(0, div, 1);
			div = _do_div((((u64)(onw * left->irr)) << 32),
				      irr_steps);
			dinw = (((u64)inw) << 32) - div;

			div = _do_div((((u64)((output_frame_width - 1 - onw) * left->irr)) <<
				       32), irr_steps);

			difwl = (((u64)(input_frame_width - 1 - inw)) << 32) - div;

			cost = difwl + (((u64)(cr * dinw)) >> 32);

			if (cost < cost_min) {
				inw_best = inw;
				cost_min = cost;
			}

			inw -= input_f;
			onw = truncate(1, inw * rr_opt, output_f);
			/* This is the minimal onw which allows the same resizing ratio */
			/*     in both stripes */
		} while (onw >= onw_min);

		inw = inw_best;
		onw = truncate(1, inw * rr_opt, output_f);
		div = _do_div((((u64)(irr_steps * inw)) << 32), onw);
		left->irr = truncate(0, div, 1);

		left->output_width = onw;
		right->output_width = output_frame_width - onw;
		/* These are valid assignments for output_width, */
		/* assuming output_f is a multiple of output_m */
		left->input_width = truncate(1, ((u64)(inw + 1)) << 32, input_m);
		right->input_width = truncate(1, ((u64)(input_frame_width - inw)) <<
					      32, input_m);

		div = _do_div((((u64)(irr_steps * (input_frame_width - 1 - inw))) <<
			       32), (right->output_width - 1));
		right->irr = truncate(0, div, 1);
		temp = truncate(0, ((u64)left->irr) * ((((u64)1) << 32) + dirr), 1);
		if (temp < right->irr)
			right->irr = temp;
		div = _do_div(((u64)((right->output_width - 1) * right->irr) <<
			       32), irr_steps);
		difwr = (u64)(input_frame_width - 1 - inw) - div;


		div = _do_div((difwr + (((u64)input_f) << 32)), 2);
		left->input_column = truncate(0, div, input_f);

		/* This splits the truncated input columns evenly */
		/*    between the left and right margins */
		right->input_column = left->input_column + inw;
		left->output_column = 0;
		right->output_column = onw;
	}
	return status;
}
EXPORT_SYMBOL(ipu_calc_stripes_sizes);
