/*
 * fec.h -- Fast Ethernet Controller definitions
 *
 * Some definitions copied from commproc.h for MPC8xx:
 * MPC8xx Communication Processor Module.
 * Copyright (c) 1997 Dan Malek (dmalek@jlc.net)
 *
 * Add FEC Structure and definitions
 * Copyright 2004-2010 Freescale Semiconductor, Inc.
 * TsiChung Liew (Tsi-Chung.Liew@freescale.com)
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

#ifndef	fec_h
#define	fec_h

#include <net.h>

/* Buffer descriptors used FEC.
*/
typedef struct cpm_buf_desc {
	ushort cbd_datlen;	/* Data length in buffer */
	ushort cbd_sc;		/* Status and Control */
	uint cbd_bufaddr;	/* Buffer address in host memory */
} cbd_t;

#define BD_SC_EMPTY		((ushort)0x8000)	/* Recieve is empty */
#define BD_SC_READY		((ushort)0x8000)	/* Transmit is ready */
#define BD_SC_WRAP		((ushort)0x2000)/* Last buffer descriptor */
#define BD_SC_INTRPT		((ushort)0x1000)/* Interrupt on change */
#define BD_SC_LAST		((ushort)0x0800)/* Last buffer in frame */
#define BD_SC_TC		((ushort)0x0400)	/* Transmit CRC */
#define BD_SC_CM		((ushort)0x0200)	/* Continous mode */
#define BD_SC_ID		((ushort)0x0100)/* Rec'd too many idles */
#define BD_SC_P			((ushort)0x0100)	/* xmt preamble */
#define BD_SC_BR		((ushort)0x0020)	/* Break received */
#define BD_SC_FR		((ushort)0x0010)	/* Framing error */
#define BD_SC_PR		((ushort)0x0008)	/* Parity error */
#define BD_SC_OV		((ushort)0x0002)	/* Overrun */
#define BD_SC_CD		((ushort)0x0001)/* Carrier Detect lost */

/* Buffer descriptor control/status used by Ethernet receive.
*/
#define BD_ENET_RX_EMPTY	((ushort)0x8000)
#define BD_ENET_RX_RO1		((ushort)0x4000)
#define BD_ENET_RX_WRAP		((ushort)0x2000)
#define BD_ENET_RX_INTR		((ushort)0x1000)
#define BD_ENET_RX_RO2		BD_ENET_RX_INTR
#define BD_ENET_RX_LAST		((ushort)0x0800)
#define BD_ENET_RX_FIRST	((ushort)0x0400)
#define BD_ENET_RX_MISS		((ushort)0x0100)
#define BD_ENET_RX_BC		((ushort)0x0080)
#define BD_ENET_RX_MC		((ushort)0x0040)
#define BD_ENET_RX_LG		((ushort)0x0020)
#define BD_ENET_RX_NO		((ushort)0x0010)
#define BD_ENET_RX_SH		((ushort)0x0008)
#define BD_ENET_RX_CR		((ushort)0x0004)
#define BD_ENET_RX_OV		((ushort)0x0002)
#define BD_ENET_RX_CL		((ushort)0x0001)
#define BD_ENET_RX_TR		BD_ENET_RX_CL
#define BD_ENET_RX_STATS	((ushort)0x013f)	/* All status bits */

/* Buffer descriptor control/status used by Ethernet transmit.
*/
#define BD_ENET_TX_READY	((ushort)0x8000)
#define BD_ENET_TX_PAD		((ushort)0x4000)
#define BD_ENET_TX_TO1		BD_ENET_TX_PAD
#define BD_ENET_TX_WRAP		((ushort)0x2000)
#define BD_ENET_TX_INTR		((ushort)0x1000)
#define BD_ENET_TX_TO2		BD_ENET_TX_INTR_
#define BD_ENET_TX_LAST		((ushort)0x0800)
#define BD_ENET_TX_TC		((ushort)0x0400)
#define BD_ENET_TX_DEF		((ushort)0x0200)
#define BD_ENET_TX_ABC		BD_ENET_TX_DEF
#define BD_ENET_TX_HB		((ushort)0x0100)
#define BD_ENET_TX_LC		((ushort)0x0080)
#define BD_ENET_TX_RL		((ushort)0x0040)
#define BD_ENET_TX_RCMASK	((ushort)0x003c)
#define BD_ENET_TX_UN		((ushort)0x0002)
#define BD_ENET_TX_CSL		((ushort)0x0001)
#define BD_ENET_TX_STATS	((ushort)0x03ff)	/* All status bits */

/*********************************************************************
* Fast Ethernet Controller (FEC)
*********************************************************************/
/* FEC private information */
struct fec_info_s {
	int index;
	u32 iobase;
	int phy_addr;
	int dup_spd;
	char *phy_name;
	int phyname_init;
	cbd_t *rxbd;		/* Rx BD */
	cbd_t *txbd;		/* Tx BD */
	uint rxIdx;
	uint txIdx;
	char *txbuf;
#ifdef CONFIG_ARCH_MMU
	char *rxbuf[PKTBUFSRX];
#endif
	int initialized;
	struct fec_info_s *next;
};

/* Register read/write struct */
typedef struct fec {
	u32 resv0;
	u32 eir;
	u32 eimr;
	u32 resv1;
	u32 rdar;
	u32 tdar;
	u32 resv2[0x03];
	u32 ecr;
	u32 resv3[0x06];
	u32 mmfr;
	u32 mscr;
	u32 resv4[0x07];
	u32 mibc;
	u32 resv5[0x07];
	u32 rcr;
	u32 resv6[0x0F];
	u32 tcr;
	u32 resv7[0x07];
	u32 palr;
	u32 paur;
	u32 opd;
	u32 resv8[0x0A];
	u32 iaur;
	u32 ialr;
	u32 gaur;
	u32 galr;
	u32 resv9[0x07];
	u32 tfwr;
	u32 resv10;
	u32 frbr;
	u32 frsr;
	u32 resv11[0x0B];
	u32 erdsr;
	u32 etdsr;
	u32 emrbr;
	u32 resv12[93];
	u32 fec_miigsk_cfgr;
	u32 fec_reserved13;
	u32 fec_miigsk_enr;
} fec_t;

/*********************************************************************
* Fast Ethernet Controller (FEC)
*********************************************************************/
/* Bit definitions and macros for FEC_EIR */
#define FEC_EIR_CLEAR_ALL		(0xFFF80000)
#define FEC_EIR_HBERR			(0x80000000)
#define FEC_EIR_BABR			(0x40000000)
#define FEC_EIR_BABT			(0x20000000)
#define FEC_EIR_GRA			(0x10000000)
#define FEC_EIR_TXF			(0x08000000)
#define FEC_EIR_TXB			(0x04000000)
#define FEC_EIR_RXF			(0x02000000)
#define FEC_EIR_RXB			(0x01000000)
#define FEC_EIR_MII			(0x00800000)
#define FEC_EIR_EBERR			(0x00400000)
#define FEC_EIR_LC			(0x00200000)
#define FEC_EIR_RL			(0x00100000)
#define FEC_EIR_UN			(0x00080000)

/* Bit definitions and macros for FEC_RDAR */
#define FEC_RDAR_R_DES_ACTIVE		(0x01000000)

/* Bit definitions and macros for FEC_TDAR */
#define FEC_TDAR_X_DES_ACTIVE		(0x01000000)

/* Bit definitions and macros for FEC_ECR */
#define FEC_ECR_ETHER_EN		(0x00000002)
#define FEC_ECR_RESET			(0x00000001)

/* Bit definitions and macros for FEC_MMFR */
#define FEC_MMFR_DATA(x)		(((x)&0xFFFF))
#define FEC_MMFR_ST(x)			(((x)&0x03)<<30)
#define FEC_MMFR_ST_01			(0x40000000)
#define FEC_MMFR_OP_RD			(0x20000000)
#define FEC_MMFR_OP_WR			(0x10000000)
#define FEC_MMFR_PA(x)			(((x)&0x1F)<<23)
#define FEC_MMFR_RA(x)			(((x)&0x1F)<<18)
#define FEC_MMFR_TA(x)			(((x)&0x03)<<16)
#define FEC_MMFR_TA_10			(0x00020000)

/* Bit definitions and macros for FEC_MSCR */
#define FEC_MSCR_DIS_PREAMBLE		(0x00000080)
#define FEC_MSCR_MII_SPEED(x)		(((x)&0x3F)<<1)

/* Bit definitions and macros for FEC_MIBC */
#define FEC_MIBC_MIB_DISABLE		(0x80000000)
#define FEC_MIBC_MIB_IDLE		(0x40000000)

/* Bit definitions and macros for FEC_RCR */
#define FEC_RCR_GRS			(0x80000000)
#define FEC_RCR_NO_LGTH_CHECK		(0x40000000)
#define FEC_RCR_MAX_FL(x)		(((x)&0x7FF)<<16)
#define FEC_RCR_CNTL_FRM_ENA		(0x00008000)
#define FEC_RCR_CRC_FWD			(0x00004000)
#define FEC_RCR_PAUSE_FWD		(0x00002000)
#define FEC_RCR_PAD_EN			(0x00001000)
#define FEC_RCR_RMII_ECHO		(0x00000800)
#define FEC_RCR_RMII_LOOP		(0x00000400)
#define FEC_RCR_RMII_10T		(0x00000200)
#define FEC_RCR_RMII_MODE		(0x00000100)
#define FEC_RCR_SGMII_ENA		(0x00000080)
#define FEC_RCR_RGMII_ENA		(0x00000040)
#define FEC_RCR_FCE			(0x00000020)
#define FEC_RCR_BC_REJ			(0x00000010)
#define FEC_RCR_PROM			(0x00000008)
#define FEC_RCR_MII_MODE		(0x00000004)
#define FEC_RCR_DRT			(0x00000002)
#define FEC_RCR_LOOP			(0x00000001)

/* Bit definitions and macros for FEC_TCR */
#define FEC_TCR_RFC_PAUSE		(0x00000010)
#define FEC_TCR_TFC_PAUSE		(0x00000008)
#define FEC_TCR_FDEN			(0x00000004)
#define FEC_TCR_HBC			(0x00000002)
#define FEC_TCR_GTS			(0x00000001)

/* Bit definitions and macros for FEC_PAUR */
#define FEC_PAUR_PADDR2(x)		(((x)&0xFFFF)<<16)
#define FEC_PAUR_TYPE(x)		((x)&0xFFFF)

/* Bit definitions and macros for FEC_OPD */
#define FEC_OPD_PAUSE_DUR(x)		(((x)&0x0000FFFF)<<0)
#define FEC_OPD_OPCODE(x)		(((x)&0x0000FFFF)<<16)

/* Bit definitions and macros for FEC_TFWR */
#define FEC_TFWR_X_WMRK(x)		((x)&0x03)
#define FEC_TFWR_X_WMRK_64		(0x01)
#define FEC_TFWR_X_WMRK_128		(0x02)
#define FEC_TFWR_X_WMRK_192		(0x03)

/* Bit definitions and macros for FEC_FRBR */
#define FEC_FRBR_R_BOUND(x)		(((x)&0xFF)<<2)

/* Bit definitions and macros for FEC_FRSR */
#define FEC_FRSR_R_FSTART(x)		(((x)&0xFF)<<2)

/* Bit definitions and macros for FEC_ERDSR */
#define FEC_ERDSR_R_DES_START(x)	(((x)&0x3FFFFFFF)<<2)

/* Bit definitions and macros for FEC_ETDSR */
#define FEC_ETDSR_X_DES_START(x)	(((x)&0x3FFFFFFF)<<2)

/* Bit definitions and macros for FEC_EMRBR */
#define FEC_EMRBR_R_BUF_SIZE(x)		(((x)&0x7F)<<4)

#define	FEC_RESET_DELAY			100
#define FEC_RX_TOUT			100

#define FEC_MAX_TIMEOUT			50000
#define FEC_TIMEOUT_TICKET		2

/*
 * Functions
 */
int mxc_fec_initialize(bd_t *bis);

#endif				/* fec_h */
