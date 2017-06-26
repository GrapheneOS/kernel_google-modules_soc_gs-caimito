/*
 * linux/drivers/gpu/exynos/g2d/g2d.h
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *
 * Samsung Graphics 2D driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __EXYNOS_G2D_H__
#define __EXYNOS_G2D_H__

#include <linux/kernel.h>
#include <linux/clk.h>
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <media/exynos_repeater.h>

struct g2d_task; /* defined in g2d_task.h */

enum g2d_priority {
	G2D_LOW_PRIORITY,
	G2D_MEDIUM_PRIORITY,
	G2D_DEFAULT_PRIORITY = G2D_MEDIUM_PRIORITY,
	G2D_HIGH_PRIORITY,
	G2D_HIGHEST_PRIORITY,
	G2D_PRIORITY_END
};

/*
 * G2D_DEVICE_STATE_SUSPEND should be treated under g2d_dev->lock_task held
 * because it should be consistent with the state of all tasks attached to
 * g2d_dev->tasks_active.
 */
#define G2D_DEVICE_STATE_SUSPEND	1
#define G2D_DEVICE_STATE_IOVMM_DISABLED	2

struct g2d_device {
	unsigned long		state;

	struct miscdevice	misc;
	struct device		*dev;
	struct clk		*clock;
	void __iomem		*reg;

	u64			fence_context;
	atomic_t		fence_timeline;
	spinlock_t		fence_lock;

	/* task management */
	spinlock_t		lock_task;
	struct g2d_task		*tasks;
	struct list_head	tasks_free;
	struct list_head	tasks_free_hwfc;
	struct list_head	tasks_prepared;
	struct list_head	tasks_active;
	struct workqueue_struct	*schedule_workq;

	struct notifier_block	pm_notifier;
	wait_queue_head_t	freeze_wait;

	struct dentry *debug_root;
	struct dentry *debug;
	struct dentry *debug_logs;

	atomic_t	prior_stats[G2D_PRIORITY_END];
};

struct g2d_context {
	struct g2d_device	*g2d_dev;
	struct shared_buffer_info *hwfc_info;
	u32 priority;
};

int g2d_device_run(struct g2d_device *g2d_dev, struct g2d_task *task);
void g2d_hw_timeout_handler(unsigned long arg);

#endif /* __EXYNOS_G2D_H__ */
