/*
 * (C) Copyright 2008-2011 Freescale Semiconductor, Inc.
 * Terry Lv
 *
 * Copyright 2008, Freescale Semiconductor, Inc
 * Andy Fleming
 *
 * Based vaguely on the Linux code
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <config.h>
#include <common.h>
#include <command.h>
#include <mmc.h>
#include <part.h>
#include <malloc.h>
#include <linux/list.h>
#include <mmc.h>
#include <div64.h>

static struct list_head mmc_devices;
static int cur_dev_num = -1;

int __board_mmc_getcd(u8 *cd, struct mmc *mmc)
{
	return -1;
}

int board_mmc_getcd(u8 *cd, struct mmc *mmc)__attribute__((weak,
	alias("__board_mmc_getcd")));

static int mmc_send_cmd(struct mmc *mmc,
	struct mmc_cmd *cmd, struct mmc_data *data)
{
	return mmc->send_cmd(mmc, cmd, data);
}

static int mmc_set_blocklen(struct mmc *mmc, int len)
{
	struct mmc_cmd cmd;

	cmd.cmdidx = MMC_CMD_SET_BLOCKLEN;
	cmd.resp_type = MMC_RSP_R1;
	cmd.cmdarg = len;
	cmd.flags = 0;

	return mmc_send_cmd(mmc, &cmd, NULL);
}

struct mmc *find_mmc_device(int dev_num)
{
	struct mmc *m;
	struct list_head *entry;

	list_for_each(entry, &mmc_devices) {
		m = list_entry(entry, struct mmc, link);

		if (m->block_dev.dev == dev_num)
			return m;
	}

	printf("MMC Device %d not found\n", dev_num);

	return NULL;
}

static ulong
mmc_write_blocks(struct mmc *mmc, ulong start, lbaint_t blkcnt, const void*src)
{
	struct mmc_cmd cmd;
	struct mmc_data data;

	if ((start + blkcnt) > mmc->block_dev.lba) {
		printf("MMC: block number 0x%lx exceeds max(0x%lx)\n",
			start + blkcnt, mmc->block_dev.lba);
		return 0;
	}

	if (blkcnt > 1)
		cmd.cmdidx = MMC_CMD_WRITE_MULTIPLE_BLOCK;
	else
		cmd.cmdidx = MMC_CMD_WRITE_SINGLE_BLOCK;

	if (mmc->high_capacity)
		cmd.cmdarg = start;
	else
		cmd.cmdarg = start * mmc->write_bl_len;

	cmd.resp_type = MMC_RSP_R1;
	cmd.flags = 0;

	data.src = src;
	data.blocks = blkcnt;
	data.blocksize = mmc->write_bl_len;
	data.flags = MMC_DATA_WRITE;

	if (mmc_send_cmd(mmc, &cmd, &data)) {
		printf("mmc write failed\n");
		return 0;
	}

	if (blkcnt > 1) {
		cmd.cmdidx = MMC_CMD_STOP_TRANSMISSION;
		cmd.cmdarg = 0;
		cmd.resp_type = MMC_RSP_R1b;
		cmd.flags = 0;
		if (mmc_send_cmd(mmc, &cmd, NULL)) {
			printf("mmc fail to send stop cmd\n");
			return 0;
		}
	}

	return blkcnt;
}

static ulong
mmc_bwrite(int dev_num, ulong start, lbaint_t blkcnt, const void *src)
{
	lbaint_t cur, blocks_todo = blkcnt;
	int err = 0, blklen;

	struct mmc *mmc = find_mmc_device(dev_num);
	if (!mmc)
		return 0;

	blklen = mmc->write_bl_len;

	if (mmc->bus_width == EMMC_MODE_4BIT_DDR ||
		mmc->bus_width == EMMC_MODE_8BIT_DDR) {
		err = 0;
		blklen = 512;
	} else
		err = mmc_set_blocklen(mmc, mmc->write_bl_len);

	if (err) {
		puts("set write bl len failed\n\r");
		return err;
	}

	do {
		/*
		 * The 65535 constraint comes from some hardware has
		 * only 16 bit width block number counter
		 */
		cur = (blocks_todo > 65535) ? 65535 : blocks_todo;
		if (mmc_write_blocks(mmc, start, cur, src) != cur)
			return 0;
		blocks_todo -= cur;
		start += cur;
		src += cur * mmc->write_bl_len;
	} while (blocks_todo > 0);

	return blkcnt;
}

static int mmc_read_blocks(struct mmc *mmc, void *dst,
				ulong start, lbaint_t blkcnt)
{
	struct mmc_cmd cmd;
	struct mmc_data data;

	if (blkcnt > 1)
		cmd.cmdidx = MMC_CMD_READ_MULTIPLE_BLOCK;
	else
		cmd.cmdidx = MMC_CMD_READ_SINGLE_BLOCK;

	if (mmc->high_capacity)
		cmd.cmdarg = start;
	else
		cmd.cmdarg = start * mmc->read_bl_len;

	cmd.resp_type = MMC_RSP_R1;
	cmd.flags = 0;

	data.dest = dst;
	data.blocks = blkcnt;
	data.blocksize = mmc->read_bl_len;
	data.flags = MMC_DATA_READ;

	if (mmc_send_cmd(mmc, &cmd, &data))
		return 0;

	if (blkcnt > 1) {
		cmd.cmdidx = MMC_CMD_STOP_TRANSMISSION;
		cmd.cmdarg = 0;
		cmd.resp_type = MMC_RSP_R1b;
		cmd.flags = 0;
		if (mmc_send_cmd(mmc, &cmd, NULL)) {
			printf("mmc fail to send stop cmd\n");
			return 0;
		}
	}

	return blkcnt;
}

static ulong mmc_bread(int dev_num, ulong start, lbaint_t blkcnt, void *dst)
{
	lbaint_t cur, blocks_todo = blkcnt;
	int err = 0, blklen;

	if (blkcnt == 0)
		return 0;

	struct mmc *mmc = find_mmc_device(dev_num);
	if (!mmc)
		return 0;

	if ((start + blkcnt) > mmc->block_dev.lba) {
		printf("MMC: block number 0x%lx exceeds max(0x%lx)\n",
			start + blkcnt, mmc->block_dev.lba);
		return 0;
	}

	if (mmc->bus_width == EMMC_MODE_4BIT_DDR ||
		mmc->bus_width == EMMC_MODE_8BIT_DDR) {
		blklen = 512;
		err = 0;
	} else {
		blklen = mmc->read_bl_len;
		err = mmc_set_blocklen(mmc, blklen);
	}

	if (err) {
		puts("set read bl len failed\n\r");
		return err;
	}

	do {
		/*
		 * The 65535 constraint comes from some hardware has
		 * only 16 bit width block number counter
		 */
		cur = (blocks_todo > 65535) ? 65535 : blocks_todo;
		if (mmc_read_blocks(mmc, dst, start, cur) != cur)
			return 0;
		blocks_todo -= cur;
		start += cur;
		dst += cur * mmc->read_bl_len;
	} while (blocks_todo > 0);

	return blkcnt;
}

static int mmc_go_idle(struct mmc *mmc)
{
	struct mmc_cmd cmd;
	int err;

	udelay(1000);

	cmd.cmdidx = MMC_CMD_GO_IDLE_STATE;
	cmd.cmdarg = 0;
	cmd.resp_type = MMC_RSP_NONE;
	cmd.flags = 0;

	err = mmc_send_cmd(mmc, &cmd, NULL);

	if (err)
		return err;

	udelay(2000);

	return 0;
}

static int
sd_send_op_cond(struct mmc *mmc)
{
	int timeout = 1000;
	int err;
	struct mmc_cmd cmd;

	do {
		cmd.cmdidx = MMC_CMD_APP_CMD;
		cmd.resp_type = MMC_RSP_R1;
		cmd.cmdarg = 0;
		cmd.flags = 0;

		err = mmc_send_cmd(mmc, &cmd, NULL);

		if (err)
			return err;

		cmd.cmdidx = SD_CMD_APP_SEND_OP_COND;
		cmd.resp_type = MMC_RSP_R3;

		/*
		 * Most cards do not answer if some reserved bits
		 * in the ocr are set. However, Some controller
		 * can set bit 7 (reserved for low voltages), but
		 * how to manage low voltages SD card is not yet
		 * specified.
		 */
		cmd.cmdarg = mmc->voltages & 0xff8000;

		if (mmc->version == SD_VERSION_2)
			cmd.cmdarg |= OCR_HCS;

		err = mmc_send_cmd(mmc, &cmd, NULL);

		if (err)
			return err;

		udelay(1000);
	} while ((!(cmd.response[0] & OCR_BUSY)) && timeout--);

	if (timeout <= 0)
		return UNUSABLE_ERR;

	if (mmc->version != SD_VERSION_2)
		mmc->version = SD_VERSION_1_0;

	mmc->ocr = cmd.response[0];

	mmc->high_capacity = ((mmc->ocr & OCR_HCS) == OCR_HCS);
	mmc->rca = 0;

	return 0;
}

static int mmc_send_op_cond(struct mmc *mmc)
{
	int timeout = 1000;
	struct mmc_cmd cmd;
	int err;

	/* Some cards seem to need this */
	mmc_go_idle(mmc);

	do {
		cmd.cmdidx = MMC_CMD_SEND_OP_COND;
		cmd.resp_type = MMC_RSP_R3;
		cmd.cmdarg = OCR_HCS | mmc->voltages;
		cmd.flags = 0;

		err = mmc_send_cmd(mmc, &cmd, NULL);

		if (err)
			return err;

		udelay(1000);
	} while (!(cmd.response[0] & OCR_BUSY) && timeout--);

	if (timeout <= 0)
		return UNUSABLE_ERR;

	mmc->version = MMC_VERSION_UNKNOWN;
	mmc->ocr = cmd.response[0];

	mmc->high_capacity = ((mmc->ocr & OCR_HCS) == OCR_HCS);
	mmc->rca = 0;

	return 0;
}


static int mmc_send_ext_csd(struct mmc *mmc, char *ext_csd)
{
	struct mmc_cmd cmd;
	struct mmc_data data;
	int err;

	/* Get the Card Status Register */
	cmd.cmdidx = MMC_CMD_SEND_EXT_CSD;
	cmd.resp_type = MMC_RSP_R1;
	cmd.cmdarg = 0;
	cmd.flags = 0;

	data.dest = ext_csd;
	data.blocks = 1;
	data.blocksize = 512;
	data.flags = MMC_DATA_READ;

	err = mmc_send_cmd(mmc, &cmd, &data);

	return err;
}


static int mmc_switch(struct mmc *mmc, u8 set, u8 index, u8 value)
{
	struct mmc_cmd cmd;

	cmd.cmdidx = MMC_CMD_SWITCH;
	cmd.resp_type = MMC_RSP_R1b;
	cmd.cmdarg = (MMC_SWITCH_MODE_WRITE_BYTE << 24) |
		(index << 16) |
		(value << 8);
	cmd.flags = 0;

	return mmc_send_cmd(mmc, &cmd, NULL);
}

static int mmc_change_freq(struct mmc *mmc)
{
	char *ext_csd;
	char cardtype;
	int err;

	mmc->card_caps = 0;

	/* Only version 4 supports high-speed */
	if (mmc->version < MMC_VERSION_4)
		return 0;

	mmc->card_caps |= ((mmc->host_caps & MMC_MODE_8BIT)
		? MMC_MODE_8BIT : MMC_MODE_4BIT);

	ext_csd = (char *)malloc(512);

	if (!ext_csd) {
		puts("Could not allocate buffer for MMC ext csd!\n");
		return -1;
	}

	err = mmc_send_ext_csd(mmc, ext_csd);

	if (err)
		goto err_rtn;

	/* Cards with density > 2GiB are sector addressed */
	if (ext_csd[212] || ext_csd[213] || ext_csd[214] || ext_csd[215] &&
			(mmc->capacity > (2u * 1024 * 1024 * 1024) / 512))
		mmc->high_capacity = 1;

	cardtype = ext_csd[EXT_CSD_CARD_TYPE] & 0xf;

	err = mmc_switch(mmc, EXT_CSD_CMD_SET_NORMAL, EXT_CSD_HS_TIMING, 1);

	if (err)
		goto err_rtn;

	/* Now check to see that it worked */
	err = mmc_send_ext_csd(mmc, ext_csd);

	if (err)
		goto err_rtn;

	/* No high-speed support */
	if (!ext_csd[EXT_CSD_HS_TIMING])
		goto no_err_rtn;

	/* High Speed is set, there are two types: 52MHz and 26MHz */
	if (cardtype & MMC_HS_52MHZ)
		mmc->card_caps |= MMC_MODE_HS_52MHz | MMC_MODE_HS;
	else
		mmc->card_caps |= MMC_MODE_HS;

	if (cardtype & EMMC_MODE_DDR_3V) {
		if (mmc->card_caps & MMC_MODE_8BIT)
			mmc->card_caps |= EMMC_MODE_8BIT_DDR;
		else
			mmc->card_caps |= EMMC_MODE_4BIT_DDR;
	}

no_err_rtn:
	free(ext_csd);
	return 0;

err_rtn:
	free(ext_csd);
	return err;
}

static int sd_switch(struct mmc *mmc, int mode, int group, u8 value, u8 *resp)
{
	struct mmc_cmd cmd;
	struct mmc_data data;

	/* Switch the frequency */
	cmd.cmdidx = SD_CMD_SWITCH_FUNC;
	cmd.resp_type = MMC_RSP_R1;
	cmd.cmdarg = (mode << 31) | 0xffffff;
	cmd.cmdarg &= ~(0xf << (group * 4));
	cmd.cmdarg |= value << (group * 4);
	cmd.flags = 0;

	data.dest = (char *)resp;
	data.blocksize = 64;
	data.blocks = 1;
	data.flags = MMC_DATA_READ;

	return mmc_send_cmd(mmc, &cmd, &data);
}


static int sd_change_freq(struct mmc *mmc)
{
	int err;
	struct mmc_cmd cmd;
	uint scr[2];
	uint switch_status[16];
	struct mmc_data data;
	int timeout;

	mmc->card_caps = 0;

	/* Read the SCR to find out if this card supports higher speeds */
	cmd.cmdidx = MMC_CMD_APP_CMD;
	cmd.resp_type = MMC_RSP_R1;
	cmd.cmdarg = mmc->rca << 16;
	cmd.flags = 0;

	err = mmc_send_cmd(mmc, &cmd, NULL);

	if (err)
		return err;

	cmd.cmdidx = SD_CMD_APP_SEND_SCR;
	cmd.resp_type = MMC_RSP_R1;
	cmd.cmdarg = 0;
	cmd.flags = 0;

	timeout = 3;

retry_scr:
	data.dest = (char *)&scr;
	data.blocksize = 8;
	data.blocks = 1;
	data.flags = MMC_DATA_READ;

	err = mmc_send_cmd(mmc, &cmd, &data);

	if (err) {
		if (timeout--)
			goto retry_scr;

		return err;
	}

	mmc->scr[0] = __be32_to_cpu(scr[0]);
	mmc->scr[1] = __be32_to_cpu(scr[1]);

	switch ((mmc->scr[0] >> 24) & 0xf) {
		case 0:
			mmc->version = SD_VERSION_1_0;
			break;
		case 1:
			mmc->version = SD_VERSION_1_10;
			break;
		case 2:
			mmc->version = SD_VERSION_2;
			break;
		default:
			mmc->version = SD_VERSION_1_0;
			break;
	}

	/* Version 1.0 doesn't support switching */
	if (mmc->version == SD_VERSION_1_0)
		return 0;

	timeout = 4;
	while (timeout--) {
		err = sd_switch(mmc, SD_SWITCH_CHECK, 0, 1,
				(u8 *)&switch_status);

		if (err)
			return err;

		/* The high-speed function is busy.  Try again */
		if (!(__be32_to_cpu(switch_status[7]) & SD_HIGHSPEED_BUSY))
			break;
	}

	if (mmc->scr[0] & SD_DATA_4BIT)
		mmc->card_caps |= MMC_MODE_4BIT;

	/* If high-speed isn't supported, we return */
	if (!(__be32_to_cpu(switch_status[3]) & SD_HIGHSPEED_SUPPORTED))
		return 0;

	err = sd_switch(mmc, SD_SWITCH_SWITCH, 0, 1, (u8 *)&switch_status);

	if (err)
		return err;

	if ((__be32_to_cpu(switch_status[4]) & 0x0f000000) == 0x01000000)
		mmc->card_caps |= MMC_MODE_HS;

	return 0;
}

/* frequency bases */
/* divided by 10 to be nice to platforms without floating point */
static int fbase[] = {
	10000,
	100000,
	1000000,
	10000000,
};

/* Multiplier values for TRAN_SPEED.  Multiplied by 10 to be nice
 * to platforms without floating point.
 */
static int multipliers[] = {
	0,	/* reserved */
	10,
	12,
	13,
	15,
	20,
	25,
	30,
	35,
	40,
	45,
	50,
	55,
	60,
	70,
	80,
};

static void mmc_set_ios(struct mmc *mmc)
{
	mmc->set_ios(mmc);
}

void mmc_set_clock(struct mmc *mmc, uint clock)
{
	if (clock > mmc->f_max)
		clock = mmc->f_max;

	if (clock < mmc->f_min)
		clock = mmc->f_min;

	mmc->clock = clock;

	mmc_set_ios(mmc);
}

static void mmc_set_bus_width(struct mmc *mmc, uint width)
{
	mmc->bus_width = width;

	mmc_set_ios(mmc);
}

#ifdef CONFIG_BOOT_PARTITION_ACCESS
/* Return 0/1/2 for partition id before switch; Return -1 if fail to switch */
int mmc_switch_partition(struct mmc *mmc, uint part, uint enable_boot)
{
	char *ext_csd;
	int err;
	uint old_part, new_part;
	char boot_config;
	char boot_bus_width, card_boot_bus_width;

	/* partition must be -
		0 - user area
		1 - boot partition 1
		2 - boot partition 2
	*/
	if (part > 2) {
		printf("\nWrong partition id - "
			"0 (user area), 1 (boot1), 2 (boot2)\n");
		return 1;
	}

	/* Before calling this func, "mmc" struct must have been initialized */
	if (mmc->version < MMC_VERSION_4) {
		puts("\nError: invalid mmc version! "
			"mmc version is below version 4!");
		return -1;
	}

	if (mmc->boot_size_mult <= 0) {
		/* it's a normal SD/MMC but user request to boot partition */
		printf("\nError: This is a normal SD/MMC card but you"
			"request to access boot partition\n");
		return -1;
	}

	/*
	 * Part must be 0 (user area), 1 (boot partition1)
	 * or 2 (boot partition2)
	 */
	if (part > 2) {
		puts("\nError: partition id must be 0(user area), "
			"1(boot partition1) or 2(boot partition2)\n");
		return -1;
	}

	ext_csd = (char *)malloc(512);
	if (!ext_csd) {
		puts("\nError: Could not allocate buffer for MMC ext csd!\n");
		return -1;
	}

	err = mmc_send_ext_csd(mmc, ext_csd);
	if (err) {
		puts("\nWarning: fail to get ext csd for MMC!\n");
		goto err_rtn;
	}

	old_part = ext_csd[EXT_CSD_BOOT_CONFIG] &
			EXT_CSD_BOOT_PARTITION_ACCESS_MASK;

	/* Send SWITCH command to change partition for access */
	boot_config = (ext_csd[EXT_CSD_BOOT_CONFIG] &
			~EXT_CSD_BOOT_PARTITION_ACCESS_MASK) |
			(char)part;

	/* enable access plus boot from that partition and boot_ack bit */
	if (enable_boot != 0)
		boot_config = (char)(part) | (char)(part << 3) | (char)(1 << 6);

	err = mmc_switch(mmc, EXT_CSD_CMD_SET_NORMAL,
			EXT_CSD_BOOT_CONFIG, boot_config);
	if (err) {
		puts("\nError: fail to send SWITCH command to card "
			"to swich partition for access!\n");
		goto err_rtn;
	}

	/* Now check whether it works */
	err = mmc_send_ext_csd(mmc, ext_csd);
	if (err) {
		puts("\nWarning: fail to get ext csd for MMC!\n");
		goto err_rtn;
	}

	new_part = ext_csd[EXT_CSD_BOOT_CONFIG] &
			EXT_CSD_BOOT_PARTITION_ACCESS_MASK;
	if ((char)part != new_part) {
		printf("\nWarning: after SWITCH, current part id %d is "
			"not same as requested partition %d!\n",
			new_part, part);
		goto err_rtn;
	}

	/* Program boot_bus_width field for eMMC 4.4 boot mode */
	if ((ext_csd[EXT_CSD_CARD_TYPE] & 0xC) && enable_boot != 0) {

		/* Configure according to this host's capabilities */
		if (mmc->host_caps & EMMC_MODE_8BIT_DDR)
			boot_bus_width =  EXT_CSD_BOOT_BUS_WIDTH_DDR |
				EXT_CSD_BOOT_BUS_WIDTH_8BIT;
		else if (mmc->host_caps & EMMC_MODE_4BIT_DDR)
			boot_bus_width =  EXT_CSD_BOOT_BUS_WIDTH_DDR |
				EXT_CSD_BOOT_BUS_WIDTH_4BIT;
		else if (mmc->host_caps & MMC_MODE_8BIT)
			boot_bus_width = EXT_CSD_BOOT_BUS_WIDTH_8BIT;
		else if (mmc->host_caps & MMC_MODE_4BIT)
			boot_bus_width = EXT_CSD_BOOT_BUS_WIDTH_4BIT;
		else
			boot_bus_width = 0;

		err = mmc_switch(mmc, EXT_CSD_CMD_SET_NORMAL,
			EXT_CSD_BOOT_BUS_WIDTH, boot_bus_width);

		/* Ensure that it programmed properly */
		err = mmc_send_ext_csd(mmc, ext_csd);
		if (err) {
			puts("\nWarning: fail to get ext csd for MMC!\n");
			goto err_rtn;
		}

		card_boot_bus_width = ext_csd[EXT_CSD_BOOT_BUS_WIDTH];
		if (card_boot_bus_width != boot_bus_width) {
			printf("\nWarning: current boot_bus_width, 0x%x, is "
				"not same as requested boot_bus_width 0x%x!\n",
				card_boot_bus_width, boot_bus_width);
			goto err_rtn;
		}
	}

	/* Seems everything is ok, return the partition id before switch */
	free(ext_csd);
	return old_part;

err_rtn:
	free(ext_csd);
	return -1;
}

int sd_switch_partition(struct mmc *mmc, uint part)
{
	struct mmc_cmd cmd;
	int err;

	if (part > 1) {
		printf("\nWrong partition id - 0 (user area), 1 (boot1)\n");
		return 1;
	}

	cmd.cmdidx = SD_CMD_SELECT_PARTITION;
	cmd.resp_type = MMC_RSP_R1;
	cmd.cmdarg = part << 24;
	cmd.flags = 0;

	err = mmc_send_cmd(mmc, &cmd, NULL);

	if (err)
		return -1;

	return 0;
}

static int mmc_get_cur_boot_partition(struct mmc *mmc)
{
	char *ext_csd;
	int err;

	ext_csd = (char *)malloc(512);

	if (!ext_csd) {
		puts("\nError! Could not allocate buffer for MMC ext csd!\n");
		return -1;
	}

	err = mmc_send_ext_csd(mmc, ext_csd);

	if (err) {
		mmc->boot_config = 0;
		mmc->boot_size_mult = 0;
		/* continue since it's not a fatal error */
	} else {
		mmc->boot_config = ext_csd[EXT_CSD_BOOT_CONFIG];
		mmc->boot_size_mult = ext_csd[EXT_CSD_BOOT_SIZE_MULT];
	}

	free(ext_csd);

	return err;
}

#endif

static int mmc_startup(struct mmc *mmc)
{
	int err;
	uint mult, freq;
	u64 cmult, csize;
	struct mmc_cmd cmd;
	char ext_csd[512];

	/* Put the Card in Identify Mode */
	cmd.cmdidx = MMC_CMD_ALL_SEND_CID;
	cmd.resp_type = MMC_RSP_R2;
	cmd.cmdarg = 0;
	cmd.flags = 0;

	err = mmc_send_cmd(mmc, &cmd, NULL);

	if (err)
		return err;

	memcpy(mmc->cid, cmd.response, 16);

	/*
	 * For MMC cards, set the Relative Address.
	 * For SD cards, get the Relatvie Address.
	 * This also puts the cards into Standby State
	 */
	cmd.cmdidx = SD_CMD_SEND_RELATIVE_ADDR;
	cmd.cmdarg = mmc->rca << 16;
	cmd.resp_type = MMC_RSP_R6;
	cmd.flags = 0;

	err = mmc_send_cmd(mmc, &cmd, NULL);

	if (err)
		return err;

	if (IS_SD(mmc))
		mmc->rca = (cmd.response[0] >> 16) & 0xffff;

	/* Get the Card-Specific Data */
	cmd.cmdidx = MMC_CMD_SEND_CSD;
	cmd.resp_type = MMC_RSP_R2;
	cmd.cmdarg = mmc->rca << 16;
	cmd.flags = 0;

	err = mmc_send_cmd(mmc, &cmd, NULL);

	if (err)
		return err;

	mmc->csd[0] = cmd.response[0];
	mmc->csd[1] = cmd.response[1];
	mmc->csd[2] = cmd.response[2];
	mmc->csd[3] = cmd.response[3];

	if (mmc->version == MMC_VERSION_UNKNOWN) {
		int version = (cmd.response[0] >> 26) & 0xf;

		switch (version) {
			case 0:
				mmc->version = MMC_VERSION_1_2;
				break;
			case 1:
				mmc->version = MMC_VERSION_1_4;
				break;
			case 2:
				mmc->version = MMC_VERSION_2_2;
				break;
			case 3:
				mmc->version = MMC_VERSION_3;
				break;
			case 4:
				mmc->version = MMC_VERSION_4;
				break;
			default:
				mmc->version = MMC_VERSION_1_2;
				break;
		}
	}

	/* divide frequency by 10, since the mults are 10x bigger */
	freq = fbase[(cmd.response[0] & 0x7)];
	mult = multipliers[((cmd.response[0] >> 3) & 0xf)];

	mmc->tran_speed = freq * mult;

	mmc->read_bl_len = 1 << ((cmd.response[1] >> 16) & 0xf);

	if (IS_SD(mmc))
		mmc->write_bl_len = mmc->read_bl_len;
	else
		mmc->write_bl_len = 1 << ((cmd.response[3] >> 22) & 0xf);

	if (mmc->high_capacity) {
		csize = (mmc->csd[1] & 0x3f) << 16
			| (mmc->csd[2] & 0xffff0000) >> 16;
		cmult = 8;
	} else {
		csize = (mmc->csd[1] & 0x3ff) << 2
			| (mmc->csd[2] & 0xc0000000) >> 30;
		cmult = (mmc->csd[2] & 0x00038000) >> 15;
	}

	mmc->capacity = (csize + 1) << (cmult + 2);
	mmc->capacity *= mmc->read_bl_len;

	if (mmc->read_bl_len > 512)
		mmc->read_bl_len = 512;

	if (mmc->write_bl_len > 512)
		mmc->write_bl_len = 512;

	/* Select the card, and put it into Transfer Mode */
	cmd.cmdidx = MMC_CMD_SELECT_CARD;
	cmd.resp_type = MMC_RSP_R1b;
	cmd.cmdarg = mmc->rca << 16;
	cmd.flags = 0;
	err = mmc_send_cmd(mmc, &cmd, NULL);

	if (err)
		return err;

	if (!IS_SD(mmc) && (mmc->version >= MMC_VERSION_4)) {
		/* check  ext_csd version and capacity */
		err = mmc_send_ext_csd(mmc, ext_csd);
		if (!err & (ext_csd[192] >= 2)) {
			mmc->capacity = ext_csd[212] << 0 | ext_csd[213] << 8 |
					ext_csd[214] << 16 | ext_csd[215] << 24;
			mmc->capacity *= 512;
		}
	}

	if (IS_SD(mmc))
		err = sd_change_freq(mmc);
	else
		err = mmc_change_freq(mmc);

	if (err)
		return err;

	/* Restrict card's capabilities by what the host can do */
	mmc->card_caps &= mmc->host_caps;

	if (IS_SD(mmc)) {
		if (mmc->card_caps & MMC_MODE_4BIT) {
			cmd.cmdidx = MMC_CMD_APP_CMD;
			cmd.resp_type = MMC_RSP_R1;
			cmd.cmdarg = mmc->rca << 16;
			cmd.flags = 0;

			err = mmc_send_cmd(mmc, &cmd, NULL);
			if (err)
				return err;

			cmd.cmdidx = SD_CMD_APP_SET_BUS_WIDTH;
			cmd.resp_type = MMC_RSP_R1;
			cmd.cmdarg = 2;
			cmd.flags = 0;
			err = mmc_send_cmd(mmc, &cmd, NULL);
			if (err)
				return err;

			mmc_set_bus_width(mmc, 4);
		}

		if (mmc->card_caps & MMC_MODE_HS)
			mmc_set_clock(mmc, 50000000);
		else
			mmc_set_clock(mmc, 25000000);
	} else {
		if (mmc->card_caps & MMC_MODE_4BIT) {
			/* Set the card to use 4 bit*/
			err = mmc_switch(mmc, EXT_CSD_CMD_SET_NORMAL,
					EXT_CSD_BUS_WIDTH,
					EXT_CSD_BUS_WIDTH_4);

			if (err)
				return err;

			mmc_set_bus_width(mmc, 4);
		} else if (mmc->card_caps & MMC_MODE_8BIT) {
			/* Set the card to use 8 bit*/
			err = mmc_switch(mmc, EXT_CSD_CMD_SET_NORMAL,
					EXT_CSD_BUS_WIDTH,
					EXT_CSD_BUS_WIDTH_8);

			if (err)
				return err;

			mmc_set_bus_width(mmc, 8);
		}

		if (mmc->card_caps & EMMC_MODE_8BIT_DDR) {
			/* Set the card to use 8 bit DDR mode */
			err = mmc_switch(mmc, EXT_CSD_CMD_SET_NORMAL,
					EXT_CSD_BUS_WIDTH,
					EXT_CSD_BUS_WIDTH_8_DDR);
			if (err)
				return err;


			/* Setup the host controller for DDR mode */
			mmc_set_bus_width(mmc, EMMC_MODE_8BIT_DDR);
		} else if (mmc->card_caps & EMMC_MODE_4BIT_DDR) {
			/* Set the card to use 4 bit DDR mode */
			err = mmc_switch(mmc, EXT_CSD_CMD_SET_NORMAL,
					EXT_CSD_BUS_WIDTH,
					EXT_CSD_BUS_WIDTH_4_DDR);
			if (err)
				return err;

			/* Setup the host controller for DDR mode */
			mmc_set_bus_width(mmc, EMMC_MODE_4BIT_DDR);
		}

		if (mmc->card_caps & MMC_MODE_HS) {
			if (mmc->card_caps & MMC_MODE_HS_52MHz)
				mmc_set_clock(mmc, 52000000);
			else
				mmc_set_clock(mmc, 26000000);
		} else
			mmc_set_clock(mmc, 20000000);

#ifdef CONFIG_BOOT_PARTITION_ACCESS
		mmc_get_cur_boot_partition(mmc);
#endif
	}

	/* fill in device description */
	mmc->block_dev.lun = 0;
	mmc->block_dev.type = 0;
	mmc->block_dev.blksz = mmc->read_bl_len;
	mmc->block_dev.lba = lldiv(mmc->capacity, mmc->read_bl_len);
	sprintf(mmc->block_dev.vendor, "Man %06x Snr %08x", mmc->cid[0] >> 8,
			(mmc->cid[2] << 8) | (mmc->cid[3] >> 24));
	sprintf(mmc->block_dev.product, "%c%c%c%c%c", mmc->cid[0] & 0xff,
			(mmc->cid[1] >> 24), (mmc->cid[1] >> 16) & 0xff,
			(mmc->cid[1] >> 8) & 0xff, mmc->cid[1] & 0xff);
	sprintf(mmc->block_dev.revision, "%d.%d", mmc->cid[2] >> 28,
			(mmc->cid[2] >> 24) & 0xf);
	init_part(&mmc->block_dev);

	return 0;
}

static int mmc_send_if_cond(struct mmc *mmc)
{
	struct mmc_cmd cmd;
	int err;

	cmd.cmdidx = SD_CMD_SEND_IF_COND;
	/* We set the bit if the host supports voltages between 2.7 and 3.6 V */
	cmd.cmdarg = ((mmc->voltages & 0xff8000) != 0) << 8 | 0xaa;
	cmd.resp_type = MMC_RSP_R7;
	cmd.flags = 0;

	err = mmc_send_cmd(mmc, &cmd, NULL);

	if (err)
		return err;

	if ((cmd.response[0] & 0xff) != 0xaa)
		return UNUSABLE_ERR;
	else
		mmc->version = SD_VERSION_2;

	return 0;
}

int mmc_register(struct mmc *mmc)
{
	/* Setup the universal parts of the block interface just once */
	mmc->block_dev.if_type = IF_TYPE_MMC;
	mmc->block_dev.dev = cur_dev_num++;
	mmc->block_dev.removable = 1;
	mmc->block_dev.block_read = mmc_bread;
	mmc->block_dev.block_write = mmc_bwrite;

	INIT_LIST_HEAD (&mmc->link);

	list_add_tail (&mmc->link, &mmc_devices);

	return 0;
}

block_dev_desc_t *mmc_get_dev(int dev)
{
	struct mmc *mmc = find_mmc_device(dev);

	return mmc ? &mmc->block_dev : NULL;
}

int mmc_init(struct mmc *mmc)
{
	int err;

	err = mmc->init(mmc);

	if (err)
		return err;

	mmc_set_bus_width(mmc, 1);
	mmc_set_clock(mmc, 1);

	/* Reset the Card */
	err = mmc_go_idle(mmc);

	if (err)
		return err;

	/* Test for SD version 2 */
	err = mmc_send_if_cond(mmc);

	/* Now try to get the SD card's operating condition */
	err = sd_send_op_cond(mmc);

	/* If the command timed out, we check for an MMC card */
	if (err == TIMEOUT) {
		err = mmc_send_op_cond(mmc);

		if (err) {
			printf("Card did not respond to voltage select!\n");
			return UNUSABLE_ERR;
		}
	}

	return mmc_startup(mmc);
}

/*
 * CPU and board-specific MMC initializations.  Aliased function
 * signals caller to move on
 */
static int __def_mmc_init(bd_t *bis)
{
	return -1;
}

int cpu_mmc_init(bd_t *bis) __attribute__((weak, alias("__def_mmc_init")));
int board_mmc_init(bd_t *bis) __attribute__((weak, alias("__def_mmc_init")));

void print_mmc_devices(char separator)
{
	struct mmc *m;
	struct list_head *entry;

	list_for_each(entry, &mmc_devices) {
		m = list_entry(entry, struct mmc, link);

		printf("%s: %d", m->name, m->block_dev.dev);

		if (entry->next != &mmc_devices)
			printf("%c ", separator);
	}

	printf("\n");
}

int mmc_initialize(bd_t *bis)
{
	INIT_LIST_HEAD (&mmc_devices);
	cur_dev_num = 0;

	if (board_mmc_init(bis) < 0)
		cpu_mmc_init(bis);

	print_mmc_devices(',');

	return 0;
}

