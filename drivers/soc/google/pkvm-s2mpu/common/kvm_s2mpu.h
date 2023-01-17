/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2021 - Google LLC
 * Author: David Brazdil <dbrazdil@google.com>
 */

#ifndef __ARM64_KVM_S2MPU_H__
#define __ARM64_KVM_S2MPU_H__

#include <linux/bitfield.h>

#include <asm/kvm_mmu.h>

#define S2MPU_MMIO_SIZE				SZ_64K
#define SYSMMU_SYNC_MMIO_SIZE			SZ_64K
#define SYSMMU_SYNC_S2_OFFSET			SZ_32K
#define SYSMMU_SYNC_S2_MMIO_SIZE		(SYSMMU_SYNC_MMIO_SIZE - \
						 SYSMMU_SYNC_S2_OFFSET)

#define NR_VIDS					8
#define NR_CTX_IDS				8

#define ALL_VIDS_BITMAP				GENMASK(NR_VIDS - 1, 0)


/*
 * S2MPU V9 specific values (some new and some different from old versions)
 * to avoid any confusion all names are prefixed with V9.
 */
#define REG_NS_V9_CTRL_PROT_EN_PER_VID_SET	0x50
#define REG_NS_V9_CTRL_PROT_EN_PER_VID_CLR	0x54
#define REG_NS_V9_CTRL_ERR_RESP_T_PER_VID_SET	0x70
#define REG_NS_V9_CFG_MPTW_ATTRIBUTE		0x10

#define REG_NS_V9_READ_MPTC			0x3014
#define REG_NS_V9_READ_MPTC_TAG_PPN		0x3018
#define REG_NS_V9_READ_MPTC_TAG_OTHERS		0x301C
#define REG_NS_V9_READ_MPTC_DATA		0x3020
#define REG_NS_V9_READ_PTLB			0x3030
#define REG_NS_V9_READ_PTLB_TAG			0x3034
#define REG_NS_V9_READ_PTLB_DATA_S1_EN_PPN_AP	0x3040
#define REG_NS_V9_READ_PTLB_DATA_S1_DIS_AP_LIST 0x3044
#define REG_NS_V9_PMMU_INDICATOR		0x3050
#define REG_NS_V9_PMMU_INFO			0x3100
#define REG_NS_V9_PMMU_PTLB_INFO(n)		(0x3400 + (n)*0x4)
#define REG_NS_V9_SWALKER_INFO			0x3104
#define REG_NS_V9_MPTC_INFO			0x3C00

/* V9 Masks */
#define V9_READ_MPTC_TAG_PPN_VALID_MASK		BIT(28)
#define V9_READ_MPTC_TAG_PPN_TPN_PPN_MASK	GENMASK(23, 0)
#define V9_READ_MPTC_TAG_PPN_MASK		(V9_READ_MPTC_TAG_PPN_VALID_MASK | \
						 V9_READ_MPTC_TAG_PPN_TPN_PPN_MASK)

#define V9_READ_MPTC_TAG_OTHERS_VID_MASK	GENMASK(10, 8)
#define V9_READ_MPTC_TAG_OTHERS_PAGE_GRAN_MASK	GENMASK(5, 4)
#define V9_READ_MPTC_TAG_OTHERS_MASK		(V9_READ_MPTC_TAG_OTHERS_VID_MASK | \
						 V9_READ_MPTC_TAG_OTHERS_PAGE_GRAN_MASK)

#define V9_READ_PTLB_WAY_MASK			GENMASK(31, 24)
#define V9_READ_PTLB_SET_MASK			GENMASK(23, 16)
#define V9_READ_PTLB_PTLB_MASK			GENMASK(15, 4)
#define V9_READ_PTLB_PMMU_MASK			GENMASK(3, 0)
#define V9_READ_PTLB_MASK			(V9_READ_PTLB_WAY_MASK | V9_READ_PTLB_SET_MASK | \
						 V9_READ_PTLB_PTLB_MASK | V9_READ_PTLB_PMMU_MASK)

#define V9_READ_PTLB_TAG_VALID_MASK		BIT(31)
#define V9_READ_PTLB_TAG_PAGE_SIZE_MASK		GENMASK(30, 28)
#define V9_READ_PTLB_TAG_STAGE1_ENABLED_MASK	BIT(27)
#define V9_READ_PTLB_TAG_VID_MASK		GENMASK(26, 24)
#define V9_READ_PTLB_TAG_TPN_MASK		GENMASK(23, 0)
#define V9_READ_PTLB_TAG_MASK			(V9_READ_PTLB_TAG_VALID_MASK | \
						 V9_READ_PTLB_TAG_TPN_MASK | \
						 V9_READ_PTLB_TAG_VID_MASK | \
						 V9_READ_PTLB_TAG_PAGE_SIZE_MASK | \
						 V9_READ_PTLB_TAG_STAGE1_ENABLED_MASK)

#define V9_READ_PTLB_DTA_S1_EN_PPN_AP_S2AP_MASK	GENMASK(25, 24)
#define V9_READ_PTLB_DTA_S1_EN_PPN_AP_PPN_MASK	GENMASK(23, 0)

#define V9_READ_PTLB_DATA_S1_ENABLE_PPN_AP_MASK (V9_READ_PTLB_DTA_S1_EN_PPN_AP_S2AP_MASK | \
						 V9_READ_PTLB_DTA_S1_EN_PPN_AP_PPN_MASK)

#define V9_READ_MPTC_INFO_NUM_MPTC_SET		GENMASK(31, 16)
#define V9_READ_MPTC_INFO_NUM_MPTC_WAY		GENMASK(15, 12)
#define V9_READ_MPTC_INFO_MASK			(V9_READ_MPTC_INFO_NUM_MPTC_SET | \
						 V9_READ_MPTC_INFO_NUM_MPTC_SET)

#define V9_READ_PMMU_INFO_NUM_PTLB		GENMASK(15, 1)
#define V9_READ_PMMU_INFO_VA_WIDTH		BIT(0)
#define V9_READ_PMMU_INFO_NUM_STREAM_TABLE	GENMASK(31, 16)
#define V9_READ_PMMU_INFO_MASK			(V9_READ_PMMU_INFO_NUM_PTLB | \
						 V9_READ_PMMU_INFO_VA_WIDTH | \
						 V9_READ_PMMU_INFO_NUM_STREAM_TABLE)

#define V9_READ_PMMU_PTLB_INFO_NUM_WAY		GENMASK(31, 16)
#define V9_READ_PMMU_PTLB_INFO_NUM_SET		GENMASK(15, 0)
#define V9_READ_PMMU_PTLB_INFO_MASK		(V9_READ_PMMU_PTLB_INFO_NUM_WAY | \
						 V9_READ_PMMU_PTLB_INFO_NUM_SET)

#define V9_READ_PMMU_INDICATOR_PMMU_NUM		GENMASK(3, 0)
#define V9_READ_PMMU_INDICATOR_MASK		V9_READ_PMMU_INDICATOR_PMMU_NUM

#define V9_READ_MPTC_WAY_MASK			GENMASK(17, 16)
#define V9_READ_MPTC_SET_MASK			GENMASK(15, 0)
#define V9_READ_MPTC_MASK			(V9_READ_MPTC_WAY_MASK | \
						 V9_READ_MPTC_SET_MASK)
#define V9_READ_MPTC_WAY(way)			FIELD_PREP(V9_READ_MPTC_WAY_MASK, (way))
#define V9_READ_MPTC_SET(set)			FIELD_PREP(V9_READ_MPTC_SET_MASK, (set))
#define V9_READ_MPTC(set, way)			(V9_READ_MPTC_SET(set) | V9_READ_MPTC_WAY(way))

#define V9_READ_PTLB_WAY(x)			FIELD_PREP(V9_READ_PTLB_WAY_MASK, (x))
#define V9_READ_PTLB_SET(x)			FIELD_PREP(V9_READ_PTLB_SET_MASK, (x))
#define V9_READ_PTLB_PTLB(x)			FIELD_PREP(V9_READ_PTLB_PTLB_MASK, (x))
#define V9_READ_PTLB_PMMU(x)			FIELD_PREP(V9_READ_PTLB_PMMU_MASK, (x))
#define V9_READ_PTLB(pu_i, pb_i, s, w)		(V9_READ_PTLB_WAY(w) | V9_READ_PTLB_SET(s) | \
						 V9_READ_PTLB_PTLB(pb_i) | V9_READ_PTLB_PMMU(pu_i))

#define V9_READ_SLTB_INFO_SET_MASK		GENMASK(15, 0)
#define V9_READ_SLTB_INFO_WAY_MASK		GENMASK(31, 16)
#define V9_READ_SLTB_INFO_MASK			(V9_READ_SLTB_INFO_SET_MASK | \
						 V9_READ_SLTB_INFO_WAY_MASK)

#define V9_SWALKER_INFO_NUM_STLB_MASK		GENMASK(31, 16)
#define V9_SWALKER_INFO_NUM_PMMU_MASK		GENMASK(15, 0)
#define V9_SWALKER_INFO_MASK			(V9_SWALKER_INFO_NUM_STLB_MASK | \
						 V9_SWALKER_INFO_NUM_PMMU_MASK)

/*
 * STLB has 2 types: A,B based on how S2MPU is connected
 * registers or masks that vary based on type are suffixed with
 * either TYPEA or TYPEB.
 */
#define REG_NS_V9_READ_STLB			0x3000
#define REG_NS_V9_READ_STLB_TPN			0x3004
#define REG_NS_V9_READ_STLB_TAG_PPN		0x3008
#define REG_NS_V9_READ_STLB_TAG_OTHERS		0x300C
#define REG_NS_V9_READ_STLB_DATA		0x3010
#define REG_NS_V9_STLB_INFO(n)			(0x3800 + (n)*0x4)

#define V9_READ_STLB_SET_MASK_TYPEA		GENMASK(7, 0)
#define V9_READ_STLB_WAY_MASK_TYPEA		GENMASK(15, 8)
#define V9_READ_STLB_SUBLINE_MASK_TYPEA		GENMASK(17, 16)
#define V9_READ_STLB_STLBID_MASK_TYPEA		GENMASK(31, 20)
#define V9_READ_STLB_MASK_TYPEA			(V9_READ_STLB_SET_MASK_TYPEA | \
						 V9_READ_STLB_WAY_MASK_TYPEA | \
						 V9_READ_STLB_SUBLINE_MASK_TYPEA | \
						 V9_READ_STLB_STLBID_MASK_TYPEA)

#define V9_READ_STLB_SET_MASK_TYPEB		GENMASK(15, 0)
#define V9_READ_STLB_WAY_MASK_TYPEB		GENMASK(17, 16)
#define V9_READ_STLB_STLBID_MASK_TYPEB		GENMASK(31, 20)
#define V9_READ_STLB_MASK_TYPEB			(V9_READ_STLB_SET_MASK_TYPEB | \
						 V9_READ_STLB_WAY_MASK_TYPEB  | \
						 V9_READ_STLB_STLBID_MASK_TYPEB)

#define V9_READ_STLB_TPN_TPN_MASK		GENMASK(23, 0)
#define V9_READ_STLB_TPN_S2VALID_MASK		BIT(24)
#define V9_READ_STLB_TPN_STAGE1_ENABLED_MASK	BIT(27)
#define V9_READ_STLB_TPN_VALID_MASK		BIT(28)
#define V9_READ_STLB_TPN_MASK			(V9_READ_STLB_TPN_TPN_MASK | \
						 V9_READ_STLB_TPN_S2VALID_MASK | \
						 V9_READ_STLB_TPN_STAGE1_ENABLED_MASK | \
						 V9_READ_STLB_TPN_VALID_MASK)

#define V9_READ_STLB_TAG_PPN_VALID_MASK_TYPEB	BIT(28)
#define V9_READ_STLB_TAG_PPN_PPN_MASK		GENMASK(23, 0)
#define V9_READ_STLB_TAG_PPN_MASK		(V9_READ_STLB_TAG_PPN_PPN_MASK | \
						 V9_READ_STLB_TAG_PPN_VALID_MASK_TYPEB)

#define V9_READ_STLB_TAG_OTHERS_S2AP_MASK_TYPEA	GENMASK(1, 0)
#define V9_READ_STLB_TAG_OTHERS_PS_MASK		GENMASK(10, 8)
#define V9_READ_STLB_TAG_OTHERS_BPS_MASK	BIT(12)
#define V9_READ_STLB_TAG_OTHERS_VID_MASK	GENMASK(23, 20)
#define V9_READ_STLB_TAG_OTHERS_MASK		(V9_READ_STLB_TAG_OTHERS_S2AP_MASK_TYPEA | \
						 V9_READ_STLB_TAG_OTHERS_PS_MASK | \
						 V9_READ_STLB_TAG_OTHERS_BPS_MASK | \
						 V9_READ_STLB_TAG_OTHERS_VID_MASK)

#define V9_READ_STLB_WAY_TYPEA(x)		FIELD_PREP(V9_READ_STLB_WAY_MASK_TYPEA, (x))
#define V9_READ_STLB_SET_TYPEA(x)		FIELD_PREP(V9_READ_STLB_SET_MASK_TYPEA, (x))
#define V9_READ_STLB_STLBID_TYPEA(x)		FIELD_PREP(V9_READ_STLB_STLBID_MASK_TYPEA, (x))
#define V9_READ_STLB_SUBLINE_TYPEA(x)		FIELD_PREP(V9_READ_STLB_SUBLINE_MASK_TYPEA, (x))

#define V9_READ_STLB_TYPEA(s_i, sub, s, w)	(V9_READ_STLB_WAY_TYPEA(w) | \
						 V9_READ_STLB_SET_TYPEA(s) | \
						 V9_READ_STLB_STLBID_TYPEA(s_i) | \
						 V9_READ_STLB_SUBLINE_TYPEA(sub))

#define V9_READ_STLB_WAY_TYPEB(x)		FIELD_PREP(V9_READ_STLB_WAY_MASK_TYPEB, (x))
#define V9_READ_STLB_SET_TYPEB(x)		FIELD_PREP(V9_READ_STLB_SET_MASK_TYPEB, (x))
#define V9_READ_STLB_STLBID_TYPEB(x)		FIELD_PREP(V9_READ_STLB_STLBID_MASK_TYPEB, (x))

#define V9_READ_STLB_TYPEB(s_i,  s, w)		(V9_READ_STLB_WAY_TYPEB(w) | \
						 V9_READ_STLB_SET_TYPEB(s) | \
						 V9_READ_STLB_STLBID_TYPEB(s_i))

#define V9_MAX_PTLB_NUM				0x100
#define V9_MAX_STLB_NUM				0x100

#define V9_CTRL0_DIS_CHK_S1L1PTW_MASK		BIT(0)
#define V9_CTRL0_DIS_CHK_S1L2PTW_MASK		BIT(1)
#define V9_CTRL0_DIS_CHK_USR_MARCHED_REQ_MASK	BIT(3)
#define V9_CTRL0_FAULT_MODE_MASK		BIT(4)
#define V9_CTRL0_ENF_FLT_MODE_S1_NONSEC_MASK	BIT(5)
#define V9_CTRL0_DESTRUCTIVE_AP_CHK_MODE_MASK	BIT(6)
#define V9_CTRL0_MASK				(V9_CTRL0_DIS_CHK_S1L1PTW_MASK | \
						 V9_CTRL0_DESTRUCTIVE_AP_CHK_MODE_MASK | \
						 V9_CTRL0_DIS_CHK_USR_MARCHED_REQ_MASK | \
						 V9_CTRL0_DIS_CHK_S1L2PTW_MASK | \
						 V9_CTRL0_ENF_FLT_MODE_S1_NONSEC_MASK | \
						 V9_CTRL0_FAULT_MODE_MASK)

#define REG_NS_CTRL0				0x0
#define REG_NS_CTRL1				0x4
#define REG_NS_CFG				0x10
#define REG_NS_INTERRUPT_ENABLE_PER_VID_SET	0x20
#define REG_NS_INTERRUPT_CLEAR			0x2c
#define REG_NS_VERSION				0x60
#define REG_NS_INFO				0x64
#define REG_NS_STATUS				0x68
#define REG_NS_NUM_CONTEXT			0x100
#define REG_NS_CONTEXT_CFG_VALID_VID		0x104
#define REG_NS_ALL_INVALIDATION			0x1000
#define REG_NS_RANGE_INVALIDATION		0x1020
#define REG_NS_RANGE_INVALIDATION_START_PPN	0x1024
#define REG_NS_RANGE_INVALIDATION_END_PPN	0x1028
#define REG_NS_FAULT_STATUS			0x2000
#define REG_NS_FAULT_PA_LOW(vid)		(0x2004 + ((vid) * 0x20))
#define REG_NS_FAULT_PA_HIGH(vid)		(0x2008 + ((vid) * 0x20))
#define REG_NS_FAULT_INFO(vid)			(0x2010 + ((vid) * 0x20))
#define REG_NS_FAULT_INFO1(vid)			(0x2014 + ((vid) * 0x20))
#define REG_NS_FAULT_INFO2(vid)			(0x2018 + ((vid) * 0x20))

#define REG_NS_READ_MPTC			0x3000
#define REG_NS_READ_MPTC_TAG_PPN		0x3004
#define REG_NS_READ_MPTC_TAG_OTHERS		0x3008
#define REG_NS_READ_MPTC_DATA			0x3010
#define REG_NS_L1ENTRY_L2TABLE_ADDR(vid, gb)	(0x4000 + ((vid) * 0x200) + ((gb) * 0x8))
#define REG_NS_L1ENTRY_ATTR(vid, gb)		(0x4004 + ((vid) * 0x200) + ((gb) * 0x8))

#define CTRL0_ENABLE				BIT(0)
#define CTRL0_INTERRUPT_ENABLE			BIT(1)
#define CTRL0_FAULT_RESP_TYPE_SLVERR		BIT(2) /* for v1 */
#define CTRL0_FAULT_RESP_TYPE_DECERR		BIT(2) /* for v2 */
#define CTRL0_MASK				(CTRL0_ENABLE | \
						 CTRL0_INTERRUPT_ENABLE | \
						 CTRL0_FAULT_RESP_TYPE_SLVERR | \
						 CTRL0_FAULT_RESP_TYPE_DECERR)

#define CTRL1_DISABLE_CHK_S1L1PTW		BIT(0)
#define CTRL1_DISABLE_CHK_S1L2PTW		BIT(1)
#define CTRL1_ENABLE_PAGE_SIZE_AWARENESS	BIT(2)
#define CTRL1_DISABLE_CHK_USER_MATCHED_REQ	BIT(3)
#define CTRL1_MASK				(CTRL1_DISABLE_CHK_S1L1PTW | \
						 CTRL1_DISABLE_CHK_S1L2PTW | \
						 CTRL1_ENABLE_PAGE_SIZE_AWARENESS | \
						 CTRL1_DISABLE_CHK_USER_MATCHED_REQ)

#define CFG_MPTW_CACHE_OVERRIDE			BIT(0)
#define CFG_MPTW_CACHE_VALUE			GENMASK(7, 4)
#define CFG_MPTW_QOS_OVERRIDE			BIT(8)
#define CFG_MPTW_QOS_VALUE			GENMASK(15, 12)
#define CFG_MPTW_SHAREABLE			BIT(16)
#define CFG_MASK				(CFG_MPTW_CACHE_OVERRIDE | \
						 CFG_MPTW_CACHE_VALUE | \
						 CFG_MPTW_QOS_OVERRIDE | \
						 CFG_MPTW_QOS_VALUE | \
						 CFG_MPTW_SHAREABLE)

/* For use with hi_lo_readq_relaxed(). */
#define REG_NS_FAULT_PA_HIGH_LOW(vid)		REG_NS_FAULT_PA_LOW(vid)

/* Mask used for extracting VID from FAULT_* register offset. */
#define REG_NS_FAULT_VID_MASK			GENMASK(7, 5)

#define VERSION_MAJOR_ARCH_VER_MASK		GENMASK(31, 28)
#define VERSION_MINOR_ARCH_VER_MASK		GENMASK(27, 24)
#define VERSION_REV_ARCH_VER_MASK		GENMASK(23, 16)
#define VERSION_RTL_VER_MASK			GENMASK(7, 0)

/* Ignore RTL version in driver version check. */
#define VERSION_CHECK_MASK			(VERSION_MAJOR_ARCH_VER_MASK | \
						 VERSION_MINOR_ARCH_VER_MASK | \
						 VERSION_REV_ARCH_VER_MASK)

#define INFO_NUM_SET_MASK			GENMASK(15, 0)

#define STATUS_BUSY				BIT(0)
#define STATUS_ON_INVALIDATING			BIT(1)

#define NUM_CONTEXT_MASK			GENMASK(3, 0)

#define CONTEXT_CFG_VALID_VID_CTX_VALID(ctx)	BIT((4 * (ctx)) + 3)
#define CONTEXT_CFG_VALID_VID_CTX_VID(ctx, vid)	\
		FIELD_PREP(GENMASK((4 * (ctx) + 2), 4 * (ctx)), (vid))

#define INVALIDATION_INVALIDATE			BIT(0)
#define RANGE_INVALIDATION_PPN_SHIFT		12

#define NR_FAULT_INFO_REGS			8
#define FAULT_INFO_VID_MASK			GENMASK(26, 24)
#define FAULT_INFO_TYPE_MASK			GENMASK(23, 21)
#define FAULT_INFO_TYPE_CONTEXT			0x4 /* v2 only */
#define FAULT_INFO_TYPE_AP			0x2
#define FAULT_INFO_TYPE_MPTW			0x1
#define FAULT_INFO_RW_BIT			BIT(20)
#define FAULT_INFO_LEN_MASK			GENMASK(19, 16)
#define FAULT_INFO_ID_MASK			GENMASK(15, 0)
#define FAULT2_PMMU_ID_MASK			GENMASK(31, 24)
#define FAULT2_STREAM_ID_MASK			GENMASK(23, 0)

#define L1ENTRY_L2TABLE_ADDR_SHIFT		4
#define L1ENTRY_L2TABLE_ADDR(pa)		((pa) >> L1ENTRY_L2TABLE_ADDR_SHIFT)

#define READ_MPTC_WAY_MASK			GENMASK(18, 16)
#define READ_MPTC_SET_MASK			GENMASK(15, 0)
#define READ_MPTC_MASK				(READ_MPTC_WAY_MASK | READ_MPTC_SET_MASK)
#define READ_MPTC_WAY(way)			FIELD_PREP(READ_MPTC_WAY_MASK, (way))
#define READ_MPTC_SET(set)			FIELD_PREP(READ_MPTC_SET_MASK, (set))
#define READ_MPTC(set, way)			(READ_MPTC_SET(set) | READ_MPTC_WAY(way))
#define READ_MPTC_TAG_PPN_MASK			GENMASK(23, 0)
#define READ_MPTC_TAG_OTHERS_VID_MASK		GENMASK(10, 8)
#define READ_MPTC_TAG_OTHERS_GRAN_MASK		GENMASK(5, 4)
#define READ_MPTC_TAG_OTHERS_VALID_BIT		BIT(0)
#define READ_MPTC_TAG_OTHERS_MASK		(READ_MPTC_TAG_OTHERS_VID_MASK | \
						 READ_MPTC_TAG_OTHERS_GRAN_MASK | \
						 READ_MPTC_TAG_OTHERS_VALID_BIT)

#define L1ENTRY_ATTR_L2TABLE_EN			BIT(0)
#define L1ENTRY_ATTR_GRAN_4K			0x0
#define L1ENTRY_ATTR_GRAN_64K			0x1
#define L1ENTRY_ATTR_GRAN_2M			0x2
#define L1ENTRY_ATTR_GRAN(gran, msk)		FIELD_PREP(msk, gran)
#define L1ENTRY_ATTR_PROT_MASK			GENMASK(2, 1)
#define L1ENTRY_ATTR_PROT(prot)			FIELD_PREP(L1ENTRY_ATTR_PROT_MASK, prot)
#define L1ENTRY_ATTR_1G(prot)			L1ENTRY_ATTR_PROT(prot)
#define L1ENTRY_ATTR_L2(gran, msk)		(L1ENTRY_ATTR_GRAN(gran, msk) | \
						 L1ENTRY_ATTR_L2TABLE_EN)

#define NR_GIGABYTES				64
#define RO_GIGABYTES_FIRST			4
#define RO_GIGABYTES_LAST			33
#define NR_RO_GIGABYTES				(RO_GIGABYTES_LAST - RO_GIGABYTES_FIRST + 1)
#define NR_RW_GIGABYTES				(NR_GIGABYTES - NR_RO_GIGABYTES)

#ifdef CONFIG_ARM64_64K_PAGES
#define SMPT_GRAN				SZ_64K
#define SMPT_GRAN_ATTR				L1ENTRY_ATTR_GRAN_64K
#else
#define SMPT_GRAN				SZ_4K
#define SMPT_GRAN_ATTR				L1ENTRY_ATTR_GRAN_4K
#endif
static_assert(SMPT_GRAN <= PAGE_SIZE);


#define SMPT_WORD_SIZE				sizeof(u32)
#define SMPT_ELEMS_PER_BYTE(prot_bits)		(BITS_PER_BYTE / (prot_bits))
#define SMPT_ELEMS_PER_WORD(prot_bits)		(SMPT_WORD_SIZE * SMPT_ELEMS_PER_BYTE(prot_bits))
#define SMPT_WORD_BYTE_RANGE(prot_bits)		(SMPT_GRAN * SMPT_ELEMS_PER_WORD(prot_bits))
#define SMPT_NUM_ELEMS				(SZ_1G / SMPT_GRAN)
#define SMPT_SIZE(prot_bits)			(SMPT_NUM_ELEMS / SMPT_ELEMS_PER_BYTE(prot_bits))
#define SMPT_NUM_WORDS(prot_bits)		(SMPT_SIZE(prot_bits) / SMPT_WORD_SIZE)
#define SMPT_NUM_PAGES(prot_bits)		(SMPT_SIZE(prot_bits) / PAGE_SIZE)
#define SMPT_ORDER(prot_bits)			get_order(SMPT_SIZE(prot_bits))


#define SMPT_GRAN_MASK				GENMASK(1, 0)

/* SysMMU_SYNC registers, relative to SYSMMU_SYNC_S2_OFFSET. */
#define REG_NS_SYNC_CMD				0x0
#define REG_NS_SYNC_COMP			0x4

#define SYNC_CMD_SYNC				BIT(0)
#define SYNC_COMP_COMPLETE			BIT(0)

/*
 * Iterate over S2MPU gigabyte regions. Skip those that cannot be modified
 * (the MMIO registers are read only, with reset value MPT_PROT_NONE).
 */
#define for_each_gb_in_range(i, first, last) \
	for ((i) = (first); (i) <= (last) && (i) < NR_GIGABYTES; \
	     (i) = (((i) + 1 == RO_GIGABYTES_FIRST) ? RO_GIGABYTES_LAST : (i)) + 1)

#define for_each_gb(i)			for_each_gb_in_range(i, 0, NR_GIGABYTES - 1)
#define for_each_vid(i)			for ((i) = 0; (i) < NR_VIDS; (i)++)
#define for_each_gb_and_vid(gb, vid)	for_each_vid((vid)) for_each_gb((gb))

enum s2mpu_version {
	S2MPU_VERSION_1 = 0x11000000,
	S2MPU_VERSION_2 = 0x20000000,
	S2MPU_VERSION_9 = 0x90000000,
};

enum mpt_prot {
	MPT_PROT_NONE	= 0,
	MPT_PROT_R	= BIT(0),
	MPT_PROT_W	= BIT(1),
	MPT_PROT_RW	= MPT_PROT_R | MPT_PROT_W,
	MPT_PROT_MASK	= MPT_PROT_RW,
};

enum mpt_update_flags {
	MPT_UPDATE_L1 = BIT(0),
	MPT_UPDATE_L2 = BIT(1),
};

struct fmpt {
	u32 *smpt;
	bool gran_1g;
	enum mpt_prot prot;
	enum mpt_update_flags flags;
};

struct mpt {
	struct fmpt fmpt[NR_GIGABYTES];
};

/* Compile time configuration for S2MPU. */

#define GRAN_BYTE(gran)			(((gran) << MPT_PROT_BITS) | (gran))
#define GRAN_HWORD(gran)		((GRAN_BYTE(gran) << 8) | (GRAN_BYTE(gran)))
#define GRAN_WORD(gran)			(((u32)(GRAN_HWORD(gran) << 16) | (GRAN_HWORD(gran))))
#define GRAN_DWORD(gran)		((u64)((u64)GRAN_WORD(gran) << 32) | (u64)(GRAN_WORD(gran)))

#if defined(S2MPU_V9) && defined(S2MPU_V1)
#error "Both S2MPU defined at same time!"
#endif

#if defined(S2MPU_V9)
#define S2MPU_VERSION				0x90000000
#define L1ENTRY_ATTR_GRAN_MASK			BIT(3)
#define MPT_PROT_BITS				4
#define MPT_ACCESS_SHIFT			2
static const u64 mpt_prot_doubleword[] = {
	[MPT_PROT_NONE] = 0x0000000000000000 | GRAN_DWORD(SMPT_GRAN_ATTR),
	[MPT_PROT_R]    = 0x4444444444444444 | GRAN_DWORD(SMPT_GRAN_ATTR),
	[MPT_PROT_W]	= 0x8888888888888888 | GRAN_DWORD(SMPT_GRAN_ATTR),
	[MPT_PROT_RW]   = 0xcccccccccccccccc | GRAN_DWORD(SMPT_GRAN_ATTR),
};
#elif defined(S2MPU_V1)
/* V1,V2 variants, we use V1 to represent both. */
#define S2MPU_VERSION				0x10000000
#define MPT_ACCESS_SHIFT			0
#define L1ENTRY_ATTR_GRAN_MASK			GENMASK(5, 4)
#define MPT_PROT_BITS				2
static const u64 mpt_prot_doubleword[] = {
	[MPT_PROT_NONE] = 0x0000000000000000,
	[MPT_PROT_R]    = 0x5555555555555555,
	[MPT_PROT_W]	= 0xaaaaaaaaaaaaaaaa,
	[MPT_PROT_RW]   = 0xffffffffffffffff,
};
#else
#error "Unknown S2MPU version"
#endif

#endif /* __ARM64_KVM_S2MPU_H__ */
