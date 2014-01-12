/*
 * (C) Copyright 2003, Psyent Corporation <www.psyent.com>
 * Scott McNutt <smcnutt@psyent.com>
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
#ifndef __ASM_NIOS_IO_H_
#define __ASM_NIOS_IO_H_

#define __raw_writeb(v,a)       (*(volatile unsigned char  *)(a) = (v))
#define __raw_writew(v,a)       (*(volatile unsigned short *)(a) = (v))
#define __raw_writel(v,a)       (*(volatile unsigned int   *)(a) = (v))

#define __raw_readb(a)          (*(volatile unsigned char  *)(a))
#define __raw_readw(a)          (*(volatile unsigned short *)(a))
#define __raw_readl(a)          (*(volatile unsigned int   *)(a))

#define readb(addr)\
	({unsigned char val;\
	 asm volatile(  "	pfxio	0		\n"\
			"	ld	%0, [%1]	\n"\
			"	ext8d	%0, %1		\n"\
			:"=r"(val) : "r" (addr)); val;})

#define readw(addr)\
	({unsigned short val;\
	 asm volatile(  "	pfxio	0		\n"\
			"	ld	%0, [%1]	\n"\
			"	ext16d	%0, %1		\n"\
			:"=r"(val) : "r" (addr)); val;})

#define readl(addr)\
	({unsigned long val;\
	 asm volatile(  "	pfxio	0		\n"\
			"	ld	%0, [%1]	\n"\
			:"=r"(val) : "r" (addr)); val;})

#define writeb(addr,val)\
	asm volatile (	"	fill8	%%r0, %1	\n"\
			"	st8d	[%0], %%r0	\n"\
			: : "r" (addr), "r" (val) : "r0")

#define writew(addr,val)\
	asm volatile (	"	fill16	%%r0, %1	\n"\
			"	st16d	[%0], %%r0	\n"\
			: : "r" (addr), "r" (val) : "r0")

#define writel(addr,val)\
	asm volatile (	"	st	[%0], %1	\n"\
			: : "r" (addr), "r" (val))

#define inb(addr)	readb(addr)
#define inw(addr)	readw(addr)
#define inl(addr)	readl(addr)
#define outb(val,addr)	writeb(addr,val)
#define outw(val,addr)	writew(addr,val)
#define outl(val,addr)	writel(addr,val)

static inline void insb (unsigned long port, void *dst, unsigned long count)
{
	unsigned char *p = dst;
	while (count--) *p++ = inb (port);
}
static inline void insw (unsigned long port, void *dst, unsigned long count)
{
	unsigned short *p = dst;
	while (count--) *p++ = inw (port);
}
static inline void insl (unsigned long port, void *dst, unsigned long count)
{
	unsigned long *p = dst;
	while (count--) *p++ = inl (port);
}

static inline void outsb (unsigned long port, const void *src, unsigned long count)
{
	const unsigned char *p = src;
	while (count--) outb (*p++, port);
}

static inline void outsw (unsigned long port, const void *src, unsigned long count)
{
	const unsigned short *p = src;
	while (count--) outw (*p++, port);
}
static inline void outsl (unsigned long port, const void *src, unsigned long count)
{
	const unsigned long *p = src;
	while (count--) outl (*p++, port);
}

static inline void sync(void)
{
}

/*
 * Given a physical address and a length, return a virtual address
 * that can be used to access the memory range with the caching
 * properties specified by "flags".
 */
#define MAP_NOCACHE	(0)
#define MAP_WRCOMBINE	(0)
#define MAP_WRBACK	(0)
#define MAP_WRTHROUGH	(0)

static inline void *
map_physmem(phys_addr_t paddr, unsigned long len, unsigned long flags)
{
	return (void *)paddr;
}

/*
 * Take down a mapping set up by map_physmem().
 */
static inline void unmap_physmem(void *vaddr, unsigned long flags)
{

}

static inline phys_addr_t virt_to_phys(void * vaddr)
{
	return (phys_addr_t)(vaddr);
}

#endif /* __ASM_NIOS_IO_H_ */
