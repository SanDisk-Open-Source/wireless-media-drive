/*
 * Copyright 2004-2011 Freescale Semiconductor, Inc. All Rights Reserved.
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
 * @file ipu_prp_sw.h
 *
 * @brief This file contains the IPU PRP use case driver header.
 *
 * @ingroup IPU
 */

#ifndef _INCLUDE_IPU__PRP_SW_H_
#define _INCLUDE_IPU__PRP_SW_H_

int csi_enc_select(void *private);
int csi_enc_deselect(void *private);
int prp_enc_select(void *private);
int prp_enc_deselect(void *private);
int prp_vf_sdc_select(void *private);
int prp_vf_sdc_select_bg(void *private);
int prp_vf_sdc_deselect(void *private);
int prp_vf_sdc_deselect_bg(void *private);
int prp_still_select(void *private);
int prp_still_deselect(void *private);

#endif
