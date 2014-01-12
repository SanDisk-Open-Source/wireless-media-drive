/*
 * Copyright 2008 Freescale Semiconductor, Inc.
 *
 * (C) Copyright 2000
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
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

#include <common.h>
#include <asm/processor.h>
#include <asm/mmu.h>
#ifdef CONFIG_ADDR_MAP
#include <addr_map.h>
#endif

DECLARE_GLOBAL_DATA_PTR;

void set_tlb(u8 tlb, u32 epn, u64 rpn,
	     u8 perms, u8 wimge,
	     u8 ts, u8 esel, u8 tsize, u8 iprot)
{
	u32 _mas0, _mas1, _mas2, _mas3, _mas7;

	_mas0 = FSL_BOOKE_MAS0(tlb, esel, 0);
	_mas1 = FSL_BOOKE_MAS1(1, iprot, 0, ts, tsize);
	_mas2 = FSL_BOOKE_MAS2(epn, wimge);
	_mas3 = FSL_BOOKE_MAS3(rpn, 0, perms);
	_mas7 = rpn >> 32;

	mtspr(MAS0, _mas0);
	mtspr(MAS1, _mas1);
	mtspr(MAS2, _mas2);
	mtspr(MAS3, _mas3);
#ifdef CONFIG_ENABLE_36BIT_PHYS
	mtspr(MAS7, _mas7);
#endif
	asm volatile("isync;msync;tlbwe;isync");

#ifdef CONFIG_ADDR_MAP
	if ((tlb == 1) && (gd->flags & GD_FLG_RELOC))
		addrmap_set_entry(epn, rpn, (1UL << ((tsize * 2) + 10)), esel);
#endif
}

void disable_tlb(u8 esel)
{
	u32 _mas0, _mas1, _mas2, _mas3, _mas7;

	_mas0 = FSL_BOOKE_MAS0(1, esel, 0);
	_mas1 = 0;
	_mas2 = 0;
	_mas3 = 0;
	_mas7 = 0;

	mtspr(MAS0, _mas0);
	mtspr(MAS1, _mas1);
	mtspr(MAS2, _mas2);
	mtspr(MAS3, _mas3);
#ifdef CONFIG_ENABLE_36BIT_PHYS
	mtspr(MAS7, _mas7);
#endif
	asm volatile("isync;msync;tlbwe;isync");

#ifdef CONFIG_ADDR_MAP
	if (gd->flags & GD_FLG_RELOC)
		addrmap_set_entry(0, 0, 0, esel);
#endif
}

void invalidate_tlb(u8 tlb)
{
	if (tlb == 0)
		mtspr(MMUCSR0, 0x4);
	if (tlb == 1)
		mtspr(MMUCSR0, 0x2);
}

void init_tlbs(void)
{
	int i;

	for (i = 0; i < num_tlb_entries; i++) {
		set_tlb(tlb_table[i].tlb, tlb_table[i].epn, tlb_table[i].rpn,
			tlb_table[i].perms, tlb_table[i].wimge,
			tlb_table[i].ts, tlb_table[i].esel, tlb_table[i].tsize,
			tlb_table[i].iprot);
	}

	return ;
}

#ifdef CONFIG_ADDR_MAP
void init_addr_map(void)
{
	int i;
	unsigned int max_cam = (mfspr(SPRN_TLB1CFG) >> 16) & 0xff;

	/* walk all the entries */
	for (i = 0; i < max_cam; i++) {
		unsigned long epn;
		u32 tsize, _mas1;
		phys_addr_t rpn;

		mtspr(MAS0, FSL_BOOKE_MAS0(1, i, 0));

		asm volatile("tlbre;isync");
		_mas1 = mfspr(MAS1);

		/* if the entry isn't valid skip it */
		if (!(_mas1 & MAS1_VALID))
			continue;

		tsize = (_mas1 >> 8) & 0xf;
		epn = mfspr(MAS2) & MAS2_EPN;
		rpn = mfspr(MAS3) & MAS3_RPN;
#ifdef CONFIG_ENABLE_36BIT_PHYS
		rpn |= ((phys_addr_t)mfspr(MAS7)) << 32;
#endif

		addrmap_set_entry(epn, rpn, (1UL << ((tsize * 2) + 10)), i);
	}

	return ;
}
#endif

#ifndef CONFIG_SYS_DDR_TLB_START
#define CONFIG_SYS_DDR_TLB_START 8
#endif

unsigned int setup_ddr_tlbs(unsigned int memsize_in_meg)
{
	unsigned int tlb_size;
	unsigned int ram_tlb_index = CONFIG_SYS_DDR_TLB_START;
	unsigned int ram_tlb_address = (unsigned int)CONFIG_SYS_DDR_SDRAM_BASE;
	unsigned int max_cam = (mfspr(SPRN_TLB1CFG) >> 16) & 0xf;
	u64 size, memsize = (u64)memsize_in_meg << 20;

	size = min(memsize, CONFIG_MAX_MEM_MAPPED);

	/* Convert (4^max) kB to (2^max) bytes */
	max_cam = max_cam * 2 + 10;

	for (; size && ram_tlb_index < 16; ram_tlb_index++) {
		u32 camsize = __ilog2_u64(size) & ~1U;
		u32 align = __ilog2(ram_tlb_address) & ~1U;

		if (align == -2) align = max_cam;
		if (camsize > align)
			camsize = align;

		if (camsize > max_cam)
			camsize = max_cam;

		tlb_size = (camsize - 10) / 2;

		set_tlb(1, ram_tlb_address, ram_tlb_address,
			MAS3_SX|MAS3_SW|MAS3_SR, 0,
			0, ram_tlb_index, tlb_size, 1);

		size -= 1ULL << camsize;
		memsize -= 1ULL << camsize;
		ram_tlb_address += 1UL << camsize;
	}

	if (memsize)
		print_size(memsize, " left unmapped\n");

	/*
	 * Confirm that the requested amount of memory was mapped.
	 */
	return memsize_in_meg;
}
