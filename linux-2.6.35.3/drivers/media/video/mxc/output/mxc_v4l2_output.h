/*
 * Copyright 2005-2010 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

/*!
 * @defgroup MXC_V4L2_OUTPUT MXC V4L2 Video Output Driver
 */
/*!
 * @file mxc_v4l2_output.h
 *
 * @brief MXC V4L2 Video Output Driver Header file
 *
 * Video4Linux2 Output Device using MXC IPU Post-processing functionality.
 *
 * @ingroup MXC_V4L2_OUTPUT
 */
#ifndef __MXC_V4L2_OUTPUT_H__
#define __MXC_V4L2_OUTPUT_H__

#include <media/v4l2-dev.h>

#ifdef __KERNEL__

#include <linux/ipu.h>
#include <linux/mxc_v4l2.h>
#include <linux/videodev2.h>

#define MIN_FRAME_NUM 2
#define MAX_FRAME_NUM 30

#define MXC_V4L2_OUT_NUM_OUTPUTS        6
#define MXC_V4L2_OUT_2_SDC              0


typedef struct {
	int list[MAX_FRAME_NUM + 1];
	int head;
	int tail;
} v4l_queue;

/*!
 * States for the video stream
 */
typedef enum {
	STATE_STREAM_OFF,
	STATE_STREAM_ON,
	STATE_STREAM_PAUSED,
	STATE_STREAM_STOPPING,
} v4lout_state;

/*!
 * common v4l2 driver structure.
 */
typedef struct _vout_data {
	struct video_device *video_dev;
	/*!
	 * semaphore guard against SMP multithreading
	 */
	struct semaphore busy_lock;

	/*!
	 * number of process that have device open
	 */
	int open_count;

	/*!
	 * params lock for this camera
	 */
	struct semaphore param_lock;

	struct timer_list output_timer;
	struct workqueue_struct *v4l_wq;
	struct work_struct icbypass_work;
	int disp_buf_num;
	int fb_blank;
	unsigned long start_jiffies;
	u32 frame_count;

	v4l_queue ready_q;
	v4l_queue done_q;

	s8 next_rdy_ipu_buf;
	s8 next_done_ipu_buf;
	s8 next_disp_ipu_buf;
	s8 ipu_buf[2];
	s8 ipu_buf_p[2];
	s8 ipu_buf_n[2];
	volatile v4lout_state state;

	int cur_disp_output;
	int output_fb_num[MXC_V4L2_OUT_NUM_OUTPUTS];
	int output_enabled[MXC_V4L2_OUT_NUM_OUTPUTS];
	struct v4l2_framebuffer v4l2_fb;
	int ic_bypass;
	u32 work_irq;
	ipu_channel_t display_ch;
	ipu_channel_t post_proc_ch;
	ipu_channel_t display_input_ch;

	/*!
	 * FRAME_NUM-buffering, so we need a array
	 */
	int buffer_cnt;
	dma_addr_t queue_buf_paddr[MAX_FRAME_NUM];
	void *queue_buf_vaddr[MAX_FRAME_NUM];
	u32 queue_buf_size;
	struct v4l2_buffer v4l2_bufs[MAX_FRAME_NUM];
	u32 display_buf_size;
	dma_addr_t display_bufs[2];
	void *display_bufs_vaddr[2];
	dma_addr_t rot_pp_bufs[2];
	void *rot_pp_bufs_vaddr[2];

	/*!
	 * Poll wait queue
	 */
	wait_queue_head_t v4l_bufq;

	/*!
	 * v4l2 format
	 */
	struct v4l2_format v2f;
	struct v4l2_mxc_offset offset;
	ipu_rotate_mode_t rotate;

	/* crop */
	struct v4l2_rect crop_bounds[MXC_V4L2_OUT_NUM_OUTPUTS];
	struct v4l2_rect crop_current;
	u32 bytesperline;
	enum v4l2_field field_fmt;
	ipu_motion_sel motion_sel;

	/* PP split fot two stripes*/
	int pp_split; /* 0,1 */
	struct stripe_param pp_left_stripe;
	struct stripe_param pp_right_stripe; /* struct for split parameters */
	struct stripe_param pp_up_stripe;
	struct stripe_param pp_down_stripe;
	/* IC ouput buffer number. Counting from 0 to 7 */
	int pp_split_buf_num; /*  0..7 */
	u16 bpp ; /* bit per pixel */
	u16 xres; /* width of physical frame (BGs) */
	u16 yres; /* heigth of physical frame (BGs)*/

} vout_data;

#endif
#endif				/* __MXC_V4L2_OUTPUT_H__ */
