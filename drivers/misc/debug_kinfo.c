// SPDX-License-Identifier: GPL-2.0
/*
 * debug_kinfo.c - backup kernel information for bootloader usage
 *
 * Copyright 2020 Google LLC
 */

#include <linux/platform_device.h>
#include <linux/kallsyms.h>
#include <linux/vmalloc.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/of_reserved_mem.h>
#include <linux/debug_kinfo.h>

/*
 * These will be re-linked against their real values
 * during the second link stage.
 */
extern const unsigned long kallsyms_addresses[] __weak;
extern const int kallsyms_offsets[] __weak;
extern const u8 kallsyms_names[] __weak;

/*
 * Tell the compiler that the count isn't in the small data section if the arch
 * has one (eg: FRV).
 */
extern const unsigned int kallsyms_num_syms __weak
__section(".rodata");

extern const unsigned long kallsyms_relative_base __weak
__section(".rodata");

extern const u8 kallsyms_token_table[] __weak;
extern const u16 kallsyms_token_index[] __weak;

extern const unsigned int kallsyms_markers[] __weak;

static phys_addr_t all_info_addr;
static u32 all_info_size;

static void update_kernel_all_info(struct kernel_all_info *all_info)
{
	int index;
	struct kernel_info *info;
	u32 *checksum_info;

	all_info->magic_number = DEBUG_KINFO_MAGIC;
	all_info->combined_checksum = 0;

	info = &(all_info->info);
	checksum_info = (u32 *)info;
	for (index = 0; index < sizeof(*info) / sizeof(u32); index++)
		all_info->combined_checksum ^= checksum_info[index];
}

static int build_info_set(const char *str, const struct kernel_param *kp)
{
	struct kernel_all_info *all_info;
	size_t build_info_size;
	int ret = 0;

	if (all_info_addr == 0 || all_info_size == 0) {
		ret = -EPERM;
		goto Exit;
	}

	all_info = (struct kernel_all_info *)all_info_addr;
	build_info_size = sizeof(all_info->info.build_info);

	memcpy(&all_info->info.build_info, str, min(build_info_size - 1, strlen(str)));
	update_kernel_all_info(all_info);

	if (strlen(str) > build_info_size) {
		pr_warn("%s: Build info buffer (len: %zd) can't hold entire string '%s'\n",
				__func__, build_info_size, str);
		ret = -ENOMEM;
	}

Exit:
	vunmap(all_info_addr);
	return ret;
}

static const struct kernel_param_ops build_info_op = {
	.set = build_info_set,
};

module_param_cb(build_info, &build_info_op, NULL, 0200);
MODULE_PARM_DESC(build_info, "Write build info to field 'build_info' of debug kinfo.");

static int debug_kinfo_probe(struct platform_device *pdev)
{
	struct device_node *mem_region;
	struct reserved_mem *rmem;
	struct kernel_all_info *all_info;
	struct kernel_info *info;

	mem_region = of_parse_phandle(pdev->dev.of_node, "memory-region", 0);
	if (!mem_region) {
		dev_warn(&pdev->dev, "no such memory-region\n");
		return 0;
	}

	rmem = of_reserved_mem_lookup(mem_region);
	if (!rmem) {
		dev_warn(&pdev->dev, "no such reserved mem of node name %s\n",
				&pdev->dev.of_node->name);
		return 0;
	}

	/* Deferred until dss.ko(dpm.o) and debug-snapshot-debug-kinfo.ko are ready */
	if (!rmem->priv) {
		return -EPROBE_DEFER;
	}

	if (!rmem->base || !rmem->size) {
		dev_warn(&pdev->dev, "unexpected reserved memory\n");
		return 0;
	}

	if (rmem->size < sizeof(struct kernel_all_info)) {
		dev_warn(&pdev->dev, "unexpected reserved memory size\n");
		return 0;
	}

	all_info_addr = rmem->priv;
	all_info_size = rmem->size;

	memset((void *)all_info_addr, 0, all_info_size);
	all_info = (struct kernel_all_info *)all_info_addr;
	info = &(all_info->info);
	info->enabled_all = IS_ENABLED(CONFIG_KALLSYMS_ALL);
	info->enabled_base_relative = IS_ENABLED(CONFIG_KALLSYMS_BASE_RELATIVE);
	info->enabled_absolute_percpu = IS_ENABLED(CONFIG_KALLSYMS_ABSOLUTE_PERCPU);
	info->enabled_cfi_clang = IS_ENABLED(CONFIG_CFI_CLANG);
	info->num_syms = kallsyms_num_syms;
	info->name_len = KSYM_NAME_LEN;
	info->bit_per_long = BITS_PER_LONG;
	info->module_name_len = MODULE_NAME_LEN;
	info->symbol_len = KSYM_SYMBOL_LEN;
	info->_addresses_pa = (u64)virt_to_phys(kallsyms_addresses);
	info->_relative_pa = (u64)virt_to_phys(kallsyms_relative_base);
	info->_stext_pa = (u64)virt_to_phys(_stext);
	info->_etext_pa = (u64)virt_to_phys(_etext);
	info->_sinittext_pa = (u64)virt_to_phys(_sinittext);
	info->_einittext_pa = (u64)virt_to_phys(_einittext);
	info->_end_pa = (u64)virt_to_phys(_end);
	info->_offsets_pa = (u64)virt_to_phys(kallsyms_offsets);
	info->_names_pa = (u64)virt_to_phys(kallsyms_names);
	info->_token_table_pa = (u64)virt_to_phys(kallsyms_token_table);
	info->_token_index_pa = (u64)virt_to_phys(kallsyms_token_index);
	info->_markers_pa = (u64)virt_to_phys(kallsyms_markers);
	info->thread_size = THREAD_SIZE;
	info->swapper_pg_dir_pa = (u64)virt_to_phys(swapper_pg_dir);
	strlcpy(info->last_uts_release, init_utsname()->release, sizeof(info->last_uts_release));

	update_kernel_all_info(all_info);

	return 0;
}

static const struct of_device_id debug_kinfo_of_match[] = {
	{ .compatible	= "google,debug-kinfo" },
	{},
};
MODULE_DEVICE_TABLE(of, debug_kinfo_of_match);

static struct platform_driver debug_kinfo_driver = {
	.probe = debug_kinfo_probe,
	.driver = {
		.name = "debug-kinfo",
		.of_match_table = of_match_ptr(debug_kinfo_of_match),
	},
};
module_platform_driver(debug_kinfo_driver);

MODULE_AUTHOR("Jone Chou <jonechou@google.com>");
MODULE_DESCRIPTION("Debug Kinfo Driver");
MODULE_LICENSE("GPL v2");
